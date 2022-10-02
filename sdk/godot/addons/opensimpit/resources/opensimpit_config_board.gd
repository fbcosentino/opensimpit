extends Resource
class_name OSPBoardConfig

export(String) var version := "0.0"
export(String) var board_name := ""
export(bool) var usb_connected := false

export(int) var used_expanders := 0
export(int) var used_axes := 0
export(int) var used_buttons := 0

export(Dictionary) var axes := {}    # Items are: {"int": OSPAxisConfig}
export(Dictionary) var buttons := {} # Items are: {"int": OSPButtonConfig}
export(Dictionary) var lcds16x2 := {} # Items are {"int": OSPLcd16x2Config}
