extends Panel

onready var label_func = $LabelRadioFunc
onready var label_active = $PanelActive/Label
onready var label_standby = $PanelStandby/Label

export(String) var RadioLabel = "COM1"

func _ready():
		label_func.text = RadioLabel

func freq_to_str(freq: float) -> String:
	var part_int = int(floor(freq))
	var part_dec = int(round( (freq-part_int)*1000 ))
	return "%03d.%03d" % [part_int, part_dec]

func set_frequencies(active, standby):
	label_active.text =  freq_to_str(active)
	label_standby.text = freq_to_str(standby)
