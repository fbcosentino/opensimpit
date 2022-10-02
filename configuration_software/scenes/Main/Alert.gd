extends ColorRect

onready var label_title = $Panel/LabelTitle
onready var label_message = $Panel/LabelMessage

func _init():
	visible = false

func show_alert(title: String, message: String):
	label_title.text = title
	label_message.bbcode_text = message
	show()


func _on_BtnOk_pressed():
	hide()
