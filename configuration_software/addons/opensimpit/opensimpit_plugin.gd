#       ____                  _____ _           ____  _ __ 
#      / __ \____  ___  ____ / ___/(_)___ ___  / __ \(_) /_
#     / / / / __ \/ _ \/ __ \\__ \/ / __ `__ \/ /_/ / / __/
#    / /_/ / /_/ /  __/ / / /__/ / / / / / / / ____/ / /_  
#    \____/ .___/\___/_/ /_/____/_/_/ /_/ /_/_/   /_/\__/  
#        /_/                                              
#
#        https://github.com/fbcosentino/opensimpit
#        Fernando Cosentino
#
#
#
#       OpenSimPit is a set of resources for building simpits (simulation cockpits),
#       for both flight simulators and space games (realistic or fantasy), using
#       Arduino IDE and boards and modules from aliexpress.
#
#       The strategy in OpenSimPit is to have a one-size-fits-all package of
#       firmware/software, where the main Arduino is entirely configurable
#       via PC software to match your cockpit design.
#
#       This is the Godot addon plugin which enables software and game developers
#       to make Godot-based applications and games communicating to an
#       OpenSimPit hardware directly (without any middleware drivers).
#
#       For documentation and license, check the repository at 
#       https://github.com/fbcosentino/opensimpit
#
#       (TL;DR: license is MIT)

tool
extends EditorPlugin


func _enter_tree():
	add_autoload_singleton("OpenSimPit", "res://addons/opensimpit/opensimpit_autoload.gd")


func _exit_tree():
	remove_autoload_singleton("OpenSimPit")
