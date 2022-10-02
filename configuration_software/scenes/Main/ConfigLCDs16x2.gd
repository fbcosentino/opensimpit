extends Control

onready var items = $Scroll/VBox.get_children()

func clear_ui():
	for item in items:
		item.reset()

func update_from_data():
	for lcd_num in OpenSimPit.board_config.lcds16x2:
		var lcd_cfg : OSPLcd16x2Config = OpenSimPit.board_config.lcds16x2[lcd_num]
		items[lcd_num].setup(lcd_cfg)

func apply_data():
	for lcd_num in range(items.size()):
		var lcd_item : UILcd16x2Config = items[lcd_num]
		OpenSimPit.board_config.lcds16x2[lcd_num] = OSPLcd16x2Config.new(lcd_num, lcd_item.get_values())
