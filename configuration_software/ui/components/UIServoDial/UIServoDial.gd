extends Control

signal released

onready var dial = $Dial
onready var label_percent = $LabelPercent

var is_pressed := false
var current_value := 50

func _on_UIServoDial_gui_input(event):
	if (event is InputEventMouseButton) and (event.button_index == BUTTON_LEFT) and (event.pressed):
		is_pressed = true

func _input(event):
	if is_pressed:
		if (event is InputEventMouseButton) and (event.button_index == BUTTON_LEFT) and (not event.pressed):
			is_pressed = false
			emit_signal("released")
		
		elif (event is InputEventMouseMotion):
			 # Both up and right increases, down or left decreases
			var delta_value = int(-event.relative.x - event.relative.y)
			current_value = clamp(current_value + delta_value, 0, 100)
			dial.rect_rotation = range_lerp(current_value, 0, 100, 45, -45)
			label_percent.text = "%d%%" % current_value

func set_value(value: int = 50):
	current_value = clamp(value, 0, 100)
	dial.rect_rotation = range_lerp(current_value, 0, 100, 45, -45)
	label_percent.text = "%d%%" % current_value

func get_value() -> int:
	return current_value
