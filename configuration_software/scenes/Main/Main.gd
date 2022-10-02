#       ____                  _____ _           ____  _ __ 
#      / __ \____  ___  ____ / ___/(_)___ ___  / __ \(_) /_
#     / / / / __ \/ _ \/ __ \\__ \/ / __ `__ \/ /_/ / / __/
#    / /_/ / /_/ /  __/ / / /__/ / / / / / / / ____/ / /_  
#    \____/ .___/\___/_/ /_/____/_/_/ /_/ /_/_/   /_/\__/  
#        /_/                                              
#
#        https://github.com/fbcosentino/opensimpit
#        Fernando Cosentino
#
#
#
#       OpenSimPit is a set of resources for building simpits (simulation cockpits),
#       for both flight simulators and space games (realistic or fantasy), using
#       Arduino IDE and boards and modules from aliexpress.
#
#       The strategy in OpenSimPit is to have a one-size-fits-all package of
#       firmware/software, where the main Arduino is entirely configurable
#       via PC software to match your cockpit design.
#
#       This is the main configuration software used to modify the internal
#       parameters in an OpenSimPit hardware, to match the physical layout.
#       If you just want to use this software, you don't need this source code.
#       You can just download exported binaries for your platform of choice
#       from the repository. 
#
#       For exported releases, documentation and license, check the repository at 
#       https://github.com/fbcosentino/opensimpit
#
#       (TL;DR: license is MIT)


extends Control



onready var alert = $Alert
onready var confirmation = $Confirmation

onready var test_area = $MainScreen/TestArea

onready var config_axes = $MainScreen/ConfigBox/Axes
onready var btn_config_axes_add = $MainScreen/ConfigBox/Axes/BtnAdd

onready var config_buttons = $MainScreen/ConfigBox/Buttons
onready var btn_config_buttons_add = $MainScreen/ConfigBox/Buttons/BtnAdd

onready var config_lcd16x2 = $MainScreen/ConfigBox/LCDs16x2

onready var main_config_pages = [
	config_axes,
	config_buttons,
	config_lcd16x2,
]


func _ready():
	OS.min_window_size = Vector2(1024, 600)
	
	var __
	__= OpenSimPit.connect("data_received", self, "_on_OSP_data_received")
	__= OpenSimPit.connect("error", self, "_on_OSP_error")
	__= OpenSimPit.connect("axis_changed", test_area, "_on_axis_changed")
	__= OpenSimPit.connect("button_changed", test_area, "_on_button_changed")
	
	$PageSerialPort.show()
	$MainScreen.hide()
	
	$MainScreen/ConfigBox/ConfigMenu/MenuList.select(0)
	_on_MenuList_item_selected(0)


func _on_OSP_data_received(data):
	pass
	#$PanelLog/Log.append_bbcode(str(data)+"\n")
	print(data)


func _on_OSP_error(bbcode_message):
	$PanelLog/Log.append_bbcode(bbcode_message+"\n")
	

# ============================================================================
# PAGE: SERIAL PORT CONNECTION

func _on_PageSerialPort_connection_successful():
	$PageSerialPort.hide()
	$MainScreen.show()
	
	print("DUMP: ", yield(OpenSimPit.config_dump(), "completed"))
	update_ui_from_config()


func _on_PageSerialPort_connection_failed():
	alert.show_alert("Connection error", "[center]\n\nThe simpit did not respond. Either the serial port is in use by another application, or this is the wrong serial port.[/center]")

func _on_PageSerialPort_no_ports_detected():
	alert.show_alert("Connection error", "[center]\nNo serial ports detected. Ensure your OpenSimPit hardware is connected to the computer.\nYou have to start this app [i]after[/i] connecting your OpenSimPit.[/center]")





# ============================================================================
# MAIN SCREEN



func _on_BtnRepo_pressed():
	OS.shell_open("https://github.com/fbcosentino/opensimpit")


func _on_MenuList_item_selected(index):
	for i in range(main_config_pages.size()):
		main_config_pages[i].visible = (i == index)


func clear_ui():
	config_axes.clear_ui()
	config_buttons.clear_ui()
	config_lcd16x2.clear_ui()


func update_ui_from_config():
	config_axes.update_from_data()
	config_buttons.update_from_data()
	config_lcd16x2.update_from_data()
	test_area.update_axes()
	test_area.update_buttons()
	test_area.update_lcds16x2()


func _on_Axes_BtnAdd_pressed():
	config_axes.add_new()


func _on_Axes_items_changed():
	btn_config_axes_add.visible = (config_axes.current_number_of_items() < OpenSimPit.MAX_NUMBER_OF_AXES)


func _on_Buttons_BtnAdd_pressed():
	config_buttons.add_new()


func _on_Buttons_items_changed():
	btn_config_buttons_add.visible = (config_buttons.current_number_of_items() < OpenSimPit.MAX_NUMBER_OF_BUTTONS)




func _on_BtnReload_pressed():
	if yield(confirmation.ask_confirmation(
		"Restore Config",
		"\n[center][color=#ffcc99]WARNING:[/color]\nThis will discard all your changes and restore\nthe setup to what was last read from the board.\nAre you sure?[/center]"
	), "completed"):
		update_ui_from_config()


func _on_BtnTest_pressed():
	config_axes.apply_data()
	config_buttons.apply_data()
	config_lcd16x2.apply_data()
	OpenSimPit.write_config_to_board(false)
	test_area.update_axes()
	test_area.update_buttons()
	test_area.update_lcds16x2()

func _on_BtnSave_pressed():
	if yield(confirmation.ask_confirmation(
		"Save Config",
		"[center][color=#ffcc99]WARNING:[/color]\nThis will overwrite the config currently in the board.\nAre you sure?\nIf you want to run this config without saving\nit permanently, use the [b]Test[/b] button instead.[/center]"
	), "completed"):
		config_axes.apply_data()
		config_buttons.apply_data()
		config_lcd16x2.apply_data()
		OpenSimPit.write_config_to_board(true)
		test_area.update_axes()
		test_area.update_buttons()
		test_area.update_lcds16x2()






