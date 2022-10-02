extends Control

onready var label_lcdid = $LabelLcdID
onready var lcd_addresses = $LcdAddress.get_children()
onready var position_select = $UILCD16X2PositionSelect
onready var position_label = $PanelPosition/Label

var lcd_position : Array
var lcd_number: int = 0

func _ready():
	# Must assign a new array on ready to avoid sharing the Array address across instances
	lcd_position = [0,0] 

func setup(lcd_address: int, lcd_num: int):
	lcd_number = lcd_num
	label_lcdid.text = "LCD %d        Address:" % lcd_num
	_set_lcd_address_icon(lcd_address)
	position_select.setup(16,2)


func _set_lcd_address_icon(addr: int):
	for i in range(lcd_addresses.size()):
		lcd_addresses[i].visible = (i == addr)



func _on_BtnBL_toggled(button_pressed):
	OpenSimPit.lcd16x2_backlight(lcd_number, button_pressed)


func _on_BtnPosition_pressed():
	position_select.show()

func _on_UILCD16X2PositionSelect_position_selected(col, row):
	lcd_position[0] = col
	lcd_position[1] = row
	position_label.text = "%d, %d" % [col, row]


func _on_BtnClear_pressed():
	OpenSimPit.lcd16x2_clear(lcd_number)


func _on_TextTestEdit_text_entered(new_text):
	OpenSimPit.lcd16x2_message(lcd_number, lcd_position[1], lcd_position[0], new_text)
