extends Panel

onready var bar = $Bar
onready var label = $Label

func setup(axis_num, board_pin):
	label.text = "Pin: A%s        Axis: %s" % [str(board_pin), str(axis_num)]

func set_value(value: int):
	bar.value = clamp(value, 0, 1023)
