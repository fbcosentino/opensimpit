extends Control
class_name UIAxisConfig

signal item_deleted

onready var board_pin = $BtnBoardPin
onready var axis_num = $BtnAxisNum
onready var remap_bar = $PanelRemapPreview/Bar
onready var remap_var_full_range = remap_bar.rect_size.x
onready var value_min = $BtnValueMin
onready var value_max = $BtnValueMax

func setup(axis_cfg: OSPAxisConfig):
	board_pin.select(axis_cfg.board_pin)
	axis_num.select(axis_cfg.axis_number)
	value_min.value = axis_cfg.value_min
	value_max.value = axis_cfg.value_max
	update_remap_bar()

func update_remap_bar():
	var size = value_max.value - value_min.value
	remap_bar.rect_size.x = range_lerp(size, 0, 1023, 0, remap_var_full_range)
	remap_bar.rect_position.x = range_lerp(value_min.value, 0, 1023, 0, remap_var_full_range)


func _on_BtnValueMin_value_changed(_value):
	update_remap_bar()


func _on_BtnValueMax_value_changed(_value):
	update_remap_bar()


func _on_BtnDelete_pressed():
	get_parent().remove_child(self)
	emit_signal("item_deleted")
	queue_free()


func get_values() -> Array:
	return [
		board_pin.selected,
		axis_num.selected,
		value_min.value,
		value_max.value,
	]
