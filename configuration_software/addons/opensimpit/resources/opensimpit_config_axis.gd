extends Resource
class_name OSPAxisConfig

export(int) var board_pin := 0
export(int) var axis_number := 0
export(int) var value_min := 0
export(int) var value_max := 4095

func _init(data: Array = [0,0,0,4095]):
	if (not data) or (not data is Array) or (data.size() == 0):
		data = [0,0,0,4095]
	
	board_pin   = data[0]
	axis_number = data[1] if data.size() > 1 else 0
	value_min   = data[2] if data.size() > 2 else 0
	value_max   = data[3] if data.size() > 3 else 4095
