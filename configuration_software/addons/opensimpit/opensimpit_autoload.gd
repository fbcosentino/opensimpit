extends Node

signal serial_ports_listed(port_list)
signal data_received(data)
signal message_received(message)
signal command_response_received(response)
signal error(bbcode_message)

signal response_received_or_timeout(response)
signal data_received_or_timeout(data)
signal dump_received_or_timeout
signal at_ok_received
signal serial_connection_status(is_connected)

signal dump_decoded
signal config_write_result(successful)
signal axis_changed(axis_number, value)
signal button_changed(button_number, value)
signal radio_changed(radio_number, active, standby)




const MAX_NUMBER_OF_AXES = 6
const MAX_NUMBER_OF_BUTTONS = 32



var board_config := OSPBoardConfig.new()

# ============================================================================
#         PC -> BOARD
# ============================================================================



# test_port() causes the serial_connection_status signal to be emitted, and
# the game/app can have methods connected. But if a synchronous check is
# desired, use (from anywhere in the game/app):
#
#     var is_connected: bool = yield(OpenSimPit.test_port(port), "completed")
#
# where port is a String with the desired port
#
func test_port(port: String):
	# Always coroutine, so it's safe to rely on yield() for it
	var was_open = is_open()
	var res = open_port(port)
	if res is GDScriptFunctionState:
		yield(res, "completed")
	
	send_at()
	var result = yield(self, "serial_connection_status")
	if not was_open:
		close_port()
	return result


func send_at():
	# Always coroutine, so it's safe to rely on yield() for it
	write_command("AT")
	_start_timeout()
	var response = yield(self, "response_received_or_timeout")
	_stop_timeout()
	emit_signal("serial_connection_status", (response == "AT OK"))


func send_command_check_response(cmd: String) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	write_command(cmd)
	_start_timeout()
	var response = yield(self, "response_received_or_timeout")
	_stop_timeout()
	return (response == "OK")



# === CONFIG

func add_axis(axis_pin_number: int, axis_joystick_number: int, value_min: int = 0, value_max: int = 4095) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "AXIS=%d,%d,%d,%d" % [
		axis_pin_number, 
		axis_joystick_number, 
		value_min, value_max
	]
	return yield(send_command_check_response(cmd), "completed")


func set_exp(num_expanders) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "EXP=%d" % num_expanders
	return yield(send_command_check_response(cmd), "completed")


func add_button(expander_index: int, expander_pin_number: int, button_joystick_number: int, invert: bool = false, toggle: bool = false) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "BTN=%d,%d,%d,%d,%d" % [
		expander_index, 
		expander_pin_number, 
		button_joystick_number, 
		1 if invert else 0, 
		1 if toggle else 0,
	]
	return yield(send_command_check_response(cmd), "completed")


func add_lcd16x2(lcd_address_index: int, lcd_number: int) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "LCD16X2=%d,%d" % [
		lcd_address_index, 
		lcd_number
	]
	return yield(send_command_check_response(cmd), "completed")


func config_clear() -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	return yield(send_command_check_response("CLEAR"), "completed")


func config_dump() -> Dictionary:
	# Always coroutine, so it's safe to rely on yield() for it
	write_command("DUMP")
	_start_timeout()
	var result = yield(self, "dump_received_or_timeout")
	_stop_timeout()
	return result


func config_reload() -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	return yield(send_command_check_response("RELOAD"), "completed")


func config_save() -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	return yield(send_command_check_response("SAVE"), "completed")


func write_config_to_board(save_to_eeprom: bool = false) -> bool:
	if not yield(config_clear(), "completed"):
		emit_signal("config_write_result", false)
		return false
	
	for axis_id in board_config.axes:
		var axis_cfg : OSPAxisConfig = board_config.axes[axis_id]
		if not yield(add_axis(
								axis_cfg.board_pin, axis_cfg.axis_number, 
								axis_cfg.value_min, axis_cfg.value_max
		), "completed"):
			emit_signal("config_write_result", false)
			return false
	
	for btn_id in board_config.buttons:
		var btn_cfg : OSPButtonConfig = board_config.buttons[btn_id]
		if not yield(add_button(
								btn_cfg.expander_index, btn_cfg.expander_pin, btn_cfg.button_number, 
								btn_cfg.invert, btn_cfg.toggle
		), "completed"):
			emit_signal("config_write_result", false)
			return false
			
	if not yield(set_exp(board_config.used_expanders), "completed"):
		emit_signal("config_write_result", false)
		return false
	
	for lcd_id in board_config.lcds16x2:
		var lcd_cfg : OSPLcd16x2Config = board_config.lcds16x2[lcd_id]
		if lcd_cfg.enabled:
			if not yield(add_lcd16x2(
								lcd_cfg.lcd_address_index, lcd_cfg.lcd_number
			), "completed"):
				emit_signal("config_write_result", false)
				return false
	
	if save_to_eeprom:
		if not yield(config_save(), "completed"):
			emit_signal("config_write_result", false)
			return false
	
	return true


# === CONTROL

func lcd16x2_clear(lcd_number: int) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "!LCC=%d" % lcd_number
	return yield(send_command_check_response(cmd), "completed")


func lcd16x2_backlight(lcd_number: int, backlight_state: bool = true) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "!LCB=%d,%d" % [lcd_number, 1 if backlight_state else 0]
	return yield(send_command_check_response(cmd), "completed")


func lcd16x2_message(lcd_number: int, row: int, col: int, message: String) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "!LC=%d,%d,%d,%s" % [
		lcd_number, 
		row, col,
		message.left(16)
	]
	return yield(send_command_check_response(cmd), "completed")


func bat_driver_display(value: int, show_frame: bool, blink_bar: bool = false, blink_frame: bool = false) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "!BT=%d,%d,%d,%d" % [
		value, 
		1 if show_frame else 0,
		1 if blink_bar else 0,
		1 if blink_frame else 0
	]
	return yield(send_command_check_response(cmd), "completed")


func servo_set_position(servo_number: int, value: int) -> bool:
	# Always coroutine, so it's safe to rely on yield() for it
	var cmd = "!S=%d,%d" % [
		servo_number,
		value, 
	]
	return yield(send_command_check_response(cmd), "completed")




# ============================================================================
#         BOARD -> PC
# ============================================================================

func _process_opensimpit_dict(data: Dictionary):
	#print("Recv: ", data)
	emit_signal("data_received", data)
	emit_signal("data_received_or_timeout", data)
	if (data.size() == 0):
		return
	
	if ((data.size() == 1) and ("msg" in data)):
		var msg = data["msg"]
		match msg:
			"AT OK":
				emit_signal("at_ok_received")
				emit_signal("response_received_or_timeout", msg)
			"OK","ERROR":
				emit_signal("command_response_received", msg)
				emit_signal("response_received_or_timeout", msg)
		emit_signal("message_received", msg)
	
	elif ("msg" in data) and (data["msg"] == "INIT"):
		pass
	
	elif ("ver" in data):
		_decode_dump(data)
		emit_signal("dump_decoded")
		emit_signal("dump_received_or_timeout", data)
	
	else:
		if "axis" in data:
			var axes_data = data["axis"]
			for axis_number_str in axes_data:
				var axis_number = int(axis_number_str)
				emit_signal("axis_changed", axis_number, axes_data[axis_number_str])
		
		if "btn" in data:
			var btn_data = data["btn"]
			for btn_number_str in btn_data:
				var btn_number = int(btn_number_str)
				emit_signal("button_changed", btn_number, btn_data[btn_number_str])
		
		if "rad" in data:
			var rad_data = {}
			for key_str in data["rad"]:
				var freqs = data["rad"][key_str]
				if freqs.size() == 2:
					var radio_number = int(key_str)
					emit_signal("radio_changed", radio_number, freqs[0], freqs[1])

func _decode_dump(data) -> bool:
	if (not "ver" in data) or (not "board" in data):
		return false
	
	# Version & status
	if ("ver" in data):
		board_config.version = str(data["ver"])
	if ("board" in data):
		board_config.board_name = str(data["board"])
	if ("usb_connected" in data):
		board_config.usb_connected = bool(data["usb_connected"])
	
	# Counters
	if ("used_axes" in data):
		board_config.used_axes = int(data["used_axes"])
	if ("used_expanders" in data):
		board_config.used_expanders = int(data["used_expanders"])
	if ("used_buttons" in data):
		board_config.used_buttons = int(data["used_buttons"])
	
	# Axes
	board_config.axes.clear()
	if ("axes" in data):
		for axis_index_str in data["axes"]:
			if (not axis_index_str is String) or (not axis_index_str.is_valid_integer()):
				emit_signal("error", "[color=#ffff88]Invalid axis index:[/color] [color=#ffff55]"+str(axis_index_str)+"[/color]")
				continue
			
			var axis_index = int(axis_index_str)
			var axis_array = data["axes"][axis_index_str]
			
			board_config.axes[axis_index] = OSPAxisConfig.new(axis_array)
	
	# Buttons
	board_config.buttons.clear()
	if ("buttons" in data):
		for btn_index_str in data["buttons"]:
			if (not btn_index_str is String) or (not btn_index_str.is_valid_integer()):
				emit_signal("error", "[color=#ffff88]Invalid button index:[/color] [color=#ffff55]"+str(btn_index_str)+"[/color]")
				continue
		
			var btn_index = int(btn_index_str)
			var btn_array = data["buttons"][btn_index_str]
			
			board_config.buttons[btn_index] = OSPButtonConfig.new(btn_array)
	
	# LCDs 16x2
	board_config.lcds16x2.clear()
	if ("lcd16x2" in data):
		for lcd_index_str in data["lcd16x2"]:
			if (not lcd_index_str is String) or (not lcd_index_str.is_valid_integer()):
				emit_signal("error", "[color=#ffff88]Invalid LCD 16x2 index:[/color] [color=#ffff55]"+str(lcd_index_str)+"[/color]")
				continue
		
			var lcd_number = int(lcd_index_str)
			var lcd_array = data["lcd16x2"][lcd_index_str]
			
			board_config.lcds16x2[lcd_number] = OSPLcd16x2Config.new(lcd_number, lcd_array)
	
	# Radios
	if ("rad" in data):
		for radio_index_str in data["rad"]:
			if (not radio_index_str is String) or (not radio_index_str.is_valid_integer()):
				emit_signal("error", "[color=#ffff88]Invalid Radio index:[/color] [color=#ffff55]"+str(radio_index_str)+"[/color]")
				continue
		
			var radio_index = int(radio_index_str)
			var freqs = data["rad"][radio_index_str]
			emit_signal("radio_changed", radio_index, freqs[0], freqs[1])



	print("Done")
	return true


# ============================================================================
#         LOW LEVEL
# ============================================================================

var gdsercomm
var received_buffer: String = ""
var timeout_timer = Timer.new()


func _init():
	gdsercomm = Node.new()
	gdsercomm.set_script(preload("res://addons/opensimpit/GDSerComm/GDSerComm.gd"))
	add_child(gdsercomm)
	
	var __
	__= gdsercomm.connect("ports_listed", self, "_on_ports_listed")
	__= gdsercomm.connect("data_received", self, "_on_data_received")
	
	add_child(timeout_timer)
	timeout_timer.wait_time = 1.0
	timeout_timer.process_mode = Timer.TIMER_PROCESS_PHYSICS
	__= timeout_timer.connect("timeout", self, "_on_timeout_timer_timeout")

func _start_timeout():
	timeout_timer.start()

func _stop_timeout():
	timeout_timer.stop()

func _on_timeout_timer_timeout():
	emit_signal("response_received_or_timeout", null)
	emit_signal("data_received_or_timeout", null)
	emit_signal("dump_received_or_timeout", null)

func _on_ports_listed(port_list):
	emit_signal("serial_ports_listed", port_list)


func open_port(port: String):
	if gdsercomm.is_open:
		return
	
	
	gdsercomm.open_port(port)
	yield(get_tree().create_timer(2.0), "timeout")
	received_buffer = ""


func close_port():
	if not gdsercomm.is_open:
		return
	
	gdsercomm.close_port()


func is_open():
	return gdsercomm.is_open



func _on_data_received(data: PoolByteArray):
	# data here is a PoolByteArray with partial data, that is, it might 
	# contain just a few characters and is not related to the concept of a
	# line-break terminated complete text line
	
	# For each byte received
	for i in range(data.size()):
		var c = data[i]
		
		# If this is a new line or carriage return, process the received content
		if c in [13, 10]: # \r or \n
			# On windows, lines are terminated by \r\n, which means the
			# second byte (\n) will trigger this with an empty buffer
			# We just ignore it
			if received_buffer.length() > 0:
				var line = received_buffer
				received_buffer = ""
				_process_buffer(line)
		
		# Any other character is added to buffer
		else:
			received_buffer += char(c)


func _process_buffer(line):
	var json_res = JSON.parse(line)
	if (json_res.error == OK) and (json_res.result is Dictionary):
		_process_opensimpit_dict(json_res.result)
	
	else:
		emit_signal("error", "[color=#ffaa88]Error in received message:[/color] [color=#ffff99]"+line+"[/color]")


func write_command(cmd: String):
	# Some methods call this and then yield waiting for a signal emitted by
	# receiving a message. It is critical for the message to arrive after the
	# yield has stated. Waiting for next frame ensures that.
	yield(get_tree(), "idle_frame")
	
	print("Writing command: ", cmd)
	gdsercomm.send_data(cmd+"\n")


func _is_error_message(data: Dictionary):
	if (data.size() == 1) and ("msg" in data) and (data["msg"] == "ERROR"):
		return true
	else:
		return false
