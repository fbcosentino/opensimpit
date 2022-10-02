extends Control

signal connection_failed
signal connection_successful
signal no_ports_detected

onready var btn_port = $Panel/BtnPort
onready var btn_connect = $Panel/BtnConnect

func _ready():
	var __
	__= OpenSimPit.connect("serial_ports_listed", self, "_on_SerialPort_ports_listed")

func _on_SerialPort_ports_listed(port_list):
	btn_port.clear()
	for port in port_list:
		btn_port.add_item(port)





func _on_BtnConnect_pressed():
	var port = btn_port.get_item_text(btn_port.selected)
	
	if port == "":
		emit_signal("no_ports_detected")
		return
	
	btn_connect.text = "Connecting..."
	btn_connect.disabled = true
	
	print("Connecting to serial port: ", port)
	var is_connected: bool = yield(OpenSimPit.test_port(port), "completed")
	if is_connected:
		var res = OpenSimPit.open_port(port)
		if res is GDScriptFunctionState:
			yield(res, "completed")

		emit_signal("connection_successful")
		btn_connect.text = "Connect"
		btn_connect.disabled = false
	else:
		emit_signal("connection_failed")
		btn_connect.text = "Connect"
		btn_connect.disabled = false
