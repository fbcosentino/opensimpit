extends ReferenceRect

var template_UITestAxis = preload("res://ui/components/UITestAxis/UITestAxis.tscn")
var template_UITestButton = preload("res://ui/components/UITestButton/UITestButton.tscn")
var template_UITestLcd16x2 = preload("res://ui/components/UITestLcd16x2/UITestLcd16x2.tscn")

onready var axes_hbox = $Panel/Scroll/VBox/Axes/Scroll/HBox
onready var axes_label_none = $Panel/Scroll/VBox/Axes/LabelNone

onready var buttons_hbox = $Panel/Scroll/VBox/Buttons/HBox
onready var buttons_label_none = $Panel/Scroll/VBox/Buttons/LabelNone
onready var buttons_panel = $Panel/Scroll/VBox/Buttons

onready var lcds16x2_hbox = $Panel/Scroll/VBox/Lcd16x2/HBox
onready var lcds16x2_panel = $Panel/Scroll/VBox/Lcd16x2
onready var lcds16x2_label_none = $Panel/Scroll/VBox/Lcd16x2/LabelNone

onready var servo_num_edit = $Panel/Scroll/VBox/Servos/UIServos/ServoNum
onready var servo_dial = $Panel/Scroll/VBox/Servos/UIServos/UIServoDial
onready var servo_board_label = $Panel/Scroll/VBox/Servos/UIServos/LabelBoardNum

onready var radios = [
	$Panel/Scroll/VBox/Radios/UIRadioCOM1,
	$Panel/Scroll/VBox/Radios/UIRadioCOM2,
	$Panel/Scroll/VBox/Radios/UIRadioNAV1,
	$Panel/Scroll/VBox/Radios/UIRadioNAV2,
]

var test_axes = {}
var test_buttons = {}
var test_lcds16x2 = {}


func _ready():
	buttons_hbox.connect("resized", self, "_on_buttons_hbox_resized")
	lcds16x2_hbox.connect("resized", self, "_on_lcds16x2_hbox_resized")


func _on_buttons_hbox_resized():
	buttons_panel.rect_min_size.y = max(64, buttons_hbox.rect_size.y + 32)

func _on_lcds16x2_hbox_resized():
	lcds16x2_panel.rect_min_size.y = max(64, lcds16x2_hbox.rect_size.y + 32)


func update_axes():
	test_axes.clear()
	for item in axes_hbox.get_children():
		axes_hbox.remove_child(item)
		item.queue_free()
	
	for axis_id in OpenSimPit.board_config.axes:
		var axis_cfg: OSPAxisConfig = OpenSimPit.board_config.axes[axis_id]
		var new_ui = template_UITestAxis.instance()
		axes_hbox.add_child(new_ui)
		new_ui.setup(axis_id, axis_cfg.board_pin)
		test_axes[axis_id] = new_ui

	axes_label_none.visible = (test_axes.size() == 0)

func update_buttons():
	test_buttons.clear()
	for item in buttons_hbox.get_children():
		buttons_hbox.remove_child(item)
		item.queue_free()
	
	for btn_id in OpenSimPit.board_config.buttons:
		var btn_cfg: OSPButtonConfig = OpenSimPit.board_config.buttons[btn_id]
		var new_ui = template_UITestButton.instance()
		buttons_hbox.add_child(new_ui)
		new_ui.setup(btn_cfg.expander_index, btn_cfg.expander_pin, btn_cfg.button_number, btn_cfg.invert, btn_cfg.toggle)
		test_buttons[btn_cfg.button_number] = new_ui
	
	buttons_label_none.visible = (test_buttons.size() == 0)


func update_lcds16x2():
	test_lcds16x2.clear()
	for item in lcds16x2_hbox.get_children():
		lcds16x2_hbox.remove_child(item)
		item.queue_free()
	
	for lcd_id in OpenSimPit.board_config.lcds16x2:
		var lcd_cfg: OSPLcd16x2Config = OpenSimPit.board_config.lcds16x2[lcd_id]
		if lcd_cfg.enabled:
			var new_ui = template_UITestLcd16x2.instance()
			lcds16x2_hbox.add_child(new_ui)
			new_ui.setup(lcd_cfg.lcd_address_index, lcd_cfg.lcd_number)
			test_lcds16x2[lcd_cfg.lcd_number] = new_ui
	
	lcds16x2_label_none.visible = (test_lcds16x2.size() == 0)


func _on_axis_changed(axis_id, value):
	if axis_id in test_axes:
		test_axes[axis_id].set_value(value)
	#else:
	#	print("Axis id %s not found" % axis_id)

func _on_button_changed(button_number, value):
	if button_number in test_buttons:
		test_buttons[button_number].set_value(value)

func _on_radio_changed(radio_number, active, standby):
	if radio_number >= radios.size():
		return
	
	radios[radio_number].set_frequencies(active, standby)


func _on_Servo_BtnSet_pressed():
	var servo_num = servo_num_edit.value
	var servo_value = servo_dial.get_value()
	OpenSimPit.servo_set_position(servo_num, servo_value)


func _on_Servo_BtnCenter_pressed():
	servo_dial.set_value(50)
	_on_Servo_BtnSet_pressed()


func _on_ServoNum_value_changed(value: int):
	servo_board_label.text = "Board %d output %d" % [floor(value / 16), (value % 16)]
