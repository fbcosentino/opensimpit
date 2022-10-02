extends ColorRect

signal response_clicked(confirmed)

onready var label_title = $Panel/LabelTitle
onready var label_message = $Panel/LabelMessage

func _init():
	visible = false

func ask_confirmation(title: String, message: String):
	label_title.text = title
	label_message.bbcode_text = message
	show()
	return yield(self, "response_clicked")


func _on_BtnOk_pressed():
	hide()
	emit_signal("response_clicked", true)


func _on_BtnCancel_pressed():
	hide()
	emit_signal("response_clicked", false)
