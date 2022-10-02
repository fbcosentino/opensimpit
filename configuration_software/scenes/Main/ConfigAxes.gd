extends Control

signal items_changed

var template_ui_axisconfig = preload("res://ui/components/UIAxisConfig/UIAxisConfig.tscn")

onready var vbox = $Scroll/VBox

func clear_ui():
	var previous_items = vbox.get_children()
	for item in previous_items:
		vbox.remove_child(item)
		item.queue_free()


func update_from_data():
	clear_ui()
	
	for axis_id in OpenSimPit.board_config.axes:
		var axis_cfg : OSPAxisConfig = OpenSimPit.board_config.axes[axis_id]
		_insert_new_item(axis_cfg)
	
	emit_signal("items_changed")


func apply_data():
	OpenSimPit.board_config.axes.clear()
	OpenSimPit.board_config.used_axes = vbox.get_child_count()
	for axis_id in range(OpenSimPit.board_config.used_axes):
		var axis_item : UIAxisConfig = vbox.get_child(axis_id)
		OpenSimPit.board_config.axes[axis_id] = OSPAxisConfig.new(axis_item.get_values())


func add_new(axis_cfg: OSPAxisConfig = OSPAxisConfig.new()):
	if current_number_of_items() >= OpenSimPit.MAX_NUMBER_OF_AXES:
		return
	_insert_new_item(axis_cfg)
	emit_signal("items_changed")


func _insert_new_item(axis_cfg: OSPAxisConfig):
	var new_item = template_ui_axisconfig.instance()
	vbox.add_child(new_item)
	new_item.connect("item_deleted", self, "_on_item_deleted")
	new_item.setup(axis_cfg)


func _on_item_deleted():
	emit_signal("items_changed")


func current_number_of_items() -> int:
	return vbox.get_child_count()
