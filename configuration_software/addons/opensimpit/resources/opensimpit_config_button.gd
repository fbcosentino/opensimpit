extends Resource
class_name OSPButtonConfig

export(int) var expander_index := 0
export(int) var expander_pin := 0
export(int) var button_number := 0
export(bool) var invert := false
export(bool) var toggle := false

func _init(data: Array = [0,0,0,0,0]):
	if (not data) or (not data is Array) or (data.size() == 0):
		data = [0,0,0,0,0]
	
	expander_index = data[0]
	expander_pin   = data[1] if data.size() > 1 else 0
	button_number  = data[2] if data.size() > 2 else 0
	invert         = true if (data.size() > 3) and (data[3] > 0) else false
	toggle         = true if (data.size() > 4) and (data[4] > 0) else false
