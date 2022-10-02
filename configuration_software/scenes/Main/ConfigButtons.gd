extends Control

signal items_changed

var template_ui_buttonconfig = preload("res://ui/components/UIButtonConfig/UIButtonConfig.tscn")

onready var vbox = $Scroll/VBox

func clear_ui():
	var previous_items = vbox.get_children()
	for item in previous_items:
		vbox.remove_child(item)
		item.queue_free()


func update_from_data():
	clear_ui()
	
	for btn_id in OpenSimPit.board_config.buttons:
		var btn_cfg : OSPButtonConfig = OpenSimPit.board_config.buttons[btn_id]
		_insert_new_item(btn_cfg)
	
	emit_signal("items_changed")


func apply_data():
	OpenSimPit.board_config.buttons.clear()
	OpenSimPit.board_config.used_buttons = vbox.get_child_count()
	var highest_expander_index = null
	
	for btn_id in range(OpenSimPit.board_config.used_buttons):
		var btn_item : UIButtonConfig = vbox.get_child(btn_id)
		var btn_values = btn_item.get_values()
		OpenSimPit.board_config.buttons[btn_id] = OSPButtonConfig.new(btn_values)
		# Check the expander index to make sure we use it
		if (not highest_expander_index) or (btn_values[0] > highest_expander_index):
			highest_expander_index = btn_values[0]
	
	OpenSimPit.board_config.used_expanders = highest_expander_index+1 if (highest_expander_index != null) else 0


func add_new(btn_cfg: OSPButtonConfig = OSPButtonConfig.new()):
	if current_number_of_items() >= OpenSimPit.MAX_NUMBER_OF_BUTTONS:
		return
	_insert_new_item(btn_cfg)
	emit_signal("items_changed")


func _insert_new_item(btn_cfg: OSPButtonConfig):
	var new_item = template_ui_buttonconfig.instance()
	vbox.add_child(new_item)
	new_item.connect("item_deleted", self, "_on_item_deleted")
	new_item.setup(btn_cfg)


func _on_item_deleted():
	emit_signal("items_changed")


func current_number_of_items() -> int:
	return vbox.get_child_count()
