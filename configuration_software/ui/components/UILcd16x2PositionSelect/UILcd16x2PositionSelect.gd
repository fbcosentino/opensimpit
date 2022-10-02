extends Control

signal position_selected(col, row)

onready var btn00 = $Btn00

# btns[row][col]
onready var btns = [[btn00]]

func _ready():
	setup(16,2)

func setup(cols: int = 16, rows: int = 2):
	btns = [[btn00]]
	
	# Build first row
	for i in range(1, cols):
		var btn = btn00.duplicate()
		add_child(btn)
		btn.rect_position.x += (btn.rect_size.x + 2) * i
		btns[0].append(btn)
	
	# Build following rows
	for r in range(1, rows):
		btns.append([])
		for i in range(0, cols):
			var btn = btn00.duplicate()
			add_child(btn)
			btn.rect_position.x += (btn.rect_size.x + 2) * i
			btn.rect_position.y += (btn.rect_size.y + 2) * r
			btns[r].append(btn)
	
	# Connect signals
	for row in range(btns.size()):
		for col in range(btns[row].size()):
			if not btns[row][col].is_connected("pressed", self, "_on_btn_pressed"):
				btns[row][col].connect("pressed", self, "_on_btn_pressed", [col, row])
	
	rect_size = Vector2(
		6 + (btn00.rect_size.x + 2) * cols,
		6 + (btn00.rect_size.y + 2) * rows
	)

func _on_btn_pressed(col, row):
	emit_signal("position_selected", col, row)
	hide()

func show_buttons(global_position: Vector2):
	rect_global_position = global_position
	show()
	
