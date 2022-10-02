extends Resource
class_name OSPLcd16x2Config

export(bool) var enabled := false

# Address index in the list of addresses for 16x2 LCDs: [0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20]
export(int) var lcd_address_index := 0  # 0=0x27, 1=0x26, ...7=0x20

# Logical number of the LCD in the config - this is arbitrary and manually mapped by user
export(int) var lcd_number := 0


func _init(p_lcd_number: int = 0, data: Array = [0,0]):
	if (not data) or (not data is Array) or (data.size() == 0):
		data = [0,0]
	
	lcd_number        = p_lcd_number
	enabled           = bool(data[0])
	lcd_address_index = int(data[1]) if data.size() > 1 else 0
