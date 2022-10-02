extends Control

onready var panel_active = $Panel/PanelActive
onready var label_btn_num = $Panel/LabelBtn
onready var label_pin = $LabelPin

func setup(exp_index: int, exp_pin: int, btn_num: int, _invert: bool = false, _toggle: bool = false):
	label_pin.text = "%d/%d" % [exp_index, exp_pin]
	label_btn_num.text = str(btn_num)
	panel_active.hide()


func set_value(active: bool):
	panel_active.visible = active
