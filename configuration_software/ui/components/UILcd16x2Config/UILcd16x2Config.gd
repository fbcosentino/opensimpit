extends Control
class_name UILcd16x2Config

export(int) var index = 0

onready var enabled = $BtnEnabled
onready var address = $BtnAddress
onready var lcd_num = $BtnLcdNum

func setup(lcd_cfg: OSPLcd16x2Config):
	enabled.pressed = (lcd_cfg.enabled)
	address.select(lcd_cfg.lcd_address_index)
	lcd_num.value = lcd_cfg.lcd_number

func reset():
	enabled.pressed = false
	address.select(index)
	lcd_num.value = index

func get_values() -> Array:
	return [
		enabled.pressed,
		address.selected,
	]
