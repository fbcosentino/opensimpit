extends Control
class_name UIButtonConfig

signal item_deleted

onready var expander = $BtnExpander
onready var exp_pin = $BtnPin
onready var btn_num = $BtnButtonNum
onready var invert = $BtnInvert
onready var toggle = $BtnToggle

func setup(btn_cfg: OSPButtonConfig):
	expander.select(btn_cfg.expander_index)
	exp_pin.value = btn_cfg.expander_pin
	btn_num.value = btn_cfg.button_number
	invert.select(btn_cfg.invert)
	toggle.select(btn_cfg.toggle)


func _on_BtnDelete_pressed():
	get_parent().remove_child(self)
	emit_signal("item_deleted")
	queue_free()



func get_values() -> Array:
	return [
		expander.selected,
		exp_pin.value,
		btn_num.value,
		invert.selected,
		toggle.selected,
	]
