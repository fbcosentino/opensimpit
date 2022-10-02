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
#       This is a very simple python example demonstrating how to use the OpenSimPit
#       python SDK module. Included in this example are handler functions for receiving
#       axes and button events, as well as controlling a 16x2 character display, 
#       and a servo.
#
#       For documentation and license, check the repository at 
#       https://github.com/fbcosentino/opensimpit
#
#       (TL;DR: license is MIT)



# Change the argument to your serial port,
# e.g. 'COM4' or '/dev/ttyACM0'
SERIAL_PORT = 'COM7'



import time
from opensimpit import OpenSimPit



def my_axis_func(axis_number, value):
    print("Axis: "+str(axis_number)+" Value: "+str(value))


def on_button_press(btn_number):
    print("Button "+str(btn_number)+" was pressed")


def on_button_release(btn_number):
    print("Button "+str(btn_number)+" was released")




osp = OpenSimPit(SERIAL_PORT) 





osp.add_function_for_axis(my_axis_func)

osp.add_function_for_button_press(on_button_press)

osp.add_function_for_button_release(on_button_release)

# add_function_for_button_state() also exists. The function  
# given to it must have 2 arguments: button number and value, 
# similar to add_function_for_axis()




# Clear LCD 0, set backlight ON, and write messages
osp.lcd16x2_clear(0)                                 # lcd_num
osp.lcd16x2_set_backlight(0,1)                       # lcd_num, active(1/0)
osp.lcd16x2_message(0,0,1,"- OpenSimPit -")          # lcd num, row, col, message
osp.lcd16x2_message(0,1,2,"Hello World!")            # lcd num, row, col, message




# Set servo 4 to center (50%)
osp.servo_set_position(4,50)





print("Starting loop - to exit use Ctrl+C")

osp.run_forever()

# Instead of letting the OpenSimPit class run forever as the main app,
# you can instead call osp.check() periodically at your convenience,
# along with whatever code you might have in your software.

print("Done.")
