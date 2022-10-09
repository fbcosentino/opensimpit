#       ____                  _____ _           ____  _ __ 
#      / __ \____  ___  ____ / ___/(_)___ ___  / __ \(_) /_
#     / / / / __ \/ _ \/ __ \\__ \/ / __ `__ \/ /_/ / / __/
#    / /_/ / /_/ /  __/ / / /__/ / / / / / / / ____/ / /_  
#    \____/ .___/\___/_/ /_/____/_/_/ /_/ /_/_/   /_/\__/  
#        /_/                                              
#      _____ _ _       _     _    ____                 
#     |  ___| (_) __ _| |__ | |_ / ___| ___  __ _ _ __ 
#     | |_  | | |/ _` | '_ \| __| |  _ / _ \/ _` | '__|
#     |  _| | | | (_| | | | | |_| |_| |  __/ (_| | |   
#     |_|   |_|_|\__, |_| |_|\__|\____|\___|\__,_|_|   
#                |___/                                 
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
#       This is the PC driver to connect the OpenSimPit hardware directly
#       to FlightGear using the Telnet protocol
#
#       To enable this, start FlightGear using the --telnet argument with
#       the port configured below. E.g. if the port configured below is 5403
#       start FlightGear with the option --telnet=5403 before launching this driver.
#
#       For documentation and license, check the repository at 
#       https://github.com/fbcosentino/opensimpit
#
#       (TL;DR: license is MIT)
#




FLIGHTGEAR_TELNET_PORT = 5403
OPENSIMPIT_SERIAL_PORT = 'COM7'

UPDATE_FREQUENCY =     10 # Hz
LCD_UPDATE_FREQUENCY = 1.0 # Hz


axis_map = {
    # Example: to map analog axis 0 to controls/engines/engine/throttle:
    # 0: "controls/engines/engine/throttle",
    
    0: "controls/flight/flaps",
}

axis_options = {
    # This can be used to modify the data coming from axes. Optional.
    # Possible options are shown below, e.g. for axis 0:
    # 0: {
    #     "remap": [input start, input end, output start, output end],
    #     "deadzone": [start, end, output value],
    # }
    
    0: {
        "remap": [0.0, 1.0, 1.0, 0.0], # invert
    },
}


button_state_map = {
    # Example: to connect button 3 to controls/switches/starter:
    # 3: "controls/switches/starter",
    
    1: "controls/switches/starter",
}

button_press_map = {
    # The same as button_state_map, but happens only when pressing, not when releasing
    # Also, to specify the value instead of true/false, use a dictionary {}
    # Example: pressing (not releasing) button 4 sets "controls/switches/magnetos" to 3:
    # 4: {"controls/switches/magnetos": 3}
    # You can have multiple keys set in a single buton, separate them with comma:
    # {"var1": value1, "var2": value2, ...} etc
    
    0: {"controls/switches/magnetos": 3},
    2: {"controls/gear/brake-parking": 1},
}

button_release_map = {
    # The same as button_press_map, but happens only when releasing instead of pressing
    
    0: {"controls/switches/magnetos": 0},
    2: {"controls/gear/brake-parking": 0},
}

lcd16x2_setup = [
    # Printed to the displays when the script starts
    # Each line is a list with 1, 2 or 4 items, in the format:
    # [lcd_number], -> clears the display
    # [lcd_number, True or False], -> sets the backlight
    # [lcd_number, row, col, "message"], -> writes message

    #[0],
    #[0,1],
    #[0,0,0,"Airspeed:"],
]

lcd16x2_variable_messages = {
    # Each line in format:
    # "variable/node": [lcd_number, row, col, "format string"],
    # format string is in python .format() syntax, where the value has the key "value"
    # Example: "{value:.2f} kt"
    
    #"velocities/airspeed-kt": [0,1,0,"{value:.2f} kt  "],
    
}



radio_map = {
    0: [
        "instrumentation/comm[0]/frequencies/selected-mhz",
        "instrumentation/comm[0]/frequencies/standby-mhz",
    ],
    1: [
        "instrumentation/comm[1]/frequencies/selected-mhz",
        "instrumentation/comm[1]/frequencies/standby-mhz",
    ],
    2: [
        "instrumentation/nav[0]/frequencies/selected-mhz",
        "instrumentation/nav[0]/frequencies/standby-mhz",
    ],
    3: [
        "instrumentation/nav[1]/frequencies/selected-mhz",
        "instrumentation/nav[1]/frequencies/standby-mhz",
    ],
}


##############################################################################
#
#    DO NOT TOUCH BELOW THIS LINE
#
##############################################################################


import time
from opensimpit import OpenSimPit
from flightgearlib import FGConnection


# To avoid spamming FlightGear with axes values, we buffer them and send only the last one
axes_buffer = {}

def remap_value(value, params):
    i_min = params[0]
    i_max = params[1]
    o_min = params[2]
    o_max = params[3]
    
    if (value <= i_min):
        return o_min
    if (value >= i_max):
        return o_max
    
    i_range = (i_max - i_min)
    o_range = (o_max - o_min)
    ratio = (o_range / i_range)
    
    return (value - i_min) * ratio + o_min


def apply_deadzone(value, params):
    start = params[0]
    end = params[1]
    dead_value = params[2]
    
    if (value >= start) and (value <= end):
        return dead_value
    else:
        return value
    


def on_axis_value(axis_number, value):
    global osp, fg, axis_map
    if (type(axis_number) == str):
        if not axis_number.isnumeric():
            return
        axis_number = int(axis_number)
    
    if axis_number in axis_map:
        if axis_number in axis_options:
            opts = axis_options[axis_number]
            if "remap" in opts:
                value = remap_value(value, opts["remap"])
            if "deadzone" in opts:
                value = apply_deadzone(value, opts["deadzone"])
        
        fg_var = axis_map[axis_number]
        axes_buffer[fg_var] = value


def on_button_state(btn_number, value):
    global osp, fg, button_state_map
    if (type(btn_number) == str):
        if not btn_number.isnumeric():
            return
        btn_number = int(btn_number)
    
    if btn_number in button_state_map:
        fg_var = button_state_map[btn_number]
        fg.set(fg_var, 'true' if value else 'false')


def on_button_press(btn_number):
    global osp, fg, button_press_map
    if (type(btn_number) == str):
        if not btn_number.isnumeric():
            return
        btn_number = int(btn_number)
    
    if btn_number in button_press_map:
        btn_map = button_press_map[btn_number]
        if type(btn_map) == str:
            # Single variable, convert to single-item dict
            btn_map = {btn_map: True}
        # If it wasn't dict or string(now dict), abort
        if type(btn_map) != dict:
            return
        
        # Apply vars
        for fg_var in btn_map:
            value = btn_map[fg_var]
            if type(value) == bool:
                value = 'true' if value else 'false'
            fg.set(fg_var, value)
    


def on_button_release(btn_number):
    global osp, fg, button_release_map
    if (type(btn_number) == str):
        if not btn_number.isnumeric():
            return
        btn_number = int(btn_number)
    
    if btn_number in button_release_map:
        btn_map = button_release_map[btn_number]
        if type(btn_map) == str:
            # Single variable, convert to single-item dict
            btn_map = {btn_map: False}
        # If it wasn't dict or string(now dict), abort
        if type(btn_map) != dict:
            return
        
        # Apply vars
        for fg_var in btn_map:
            value = btn_map[fg_var]
            if type(value) == bool:
                value = 'true' if value else 'false'
            fg.set(fg_var, value)


def on_radio_data(radio_number, active, standby):
    global osp, fg
    if (type(radio_number) == str):
        if not radio_number.isnumeric():
            return
        radio_number = int(radio_number)
        if radio_number in radio_map:
            freq_var_active  = radio_map[radio_number][0]
            fg.set(freq_var_active, active)
            
            freq_var_standby = radio_map[radio_number][1]
            fg.set(freq_var_standby, standby)
    

def send_axes():
    for fg_var in axes_buffer:
        value = axes_buffer[fg_var]
        fg.set(fg_var, value)
    axes_buffer.clear()


def send_lcd16x2_setup():
    for item in lcd16x2_setup:
        item_len = len(item)
        match item_len:
            case 1:
                osp.lcd16x2_clear(item[0])
            
            case 2:
                osp.lcd16x2_set_backlight(item[0], item[1])
            
            case 4:
                osp.lcd16x2_message(item[0], item[1], item[2], item[3])
            

def send_lcd16x2_variables():
    for fg_var in lcd16x2_variable_messages:
        item = lcd16x2_variable_messages[fg_var]
        item_value = fg.get(fg_var)
        if item_value is not None:
            print(item_value)
            osp.lcd16x2_message(item[0], item[1], item[2], item[3].format(value=item_value))





fg = FGConnection(FLIGHTGEAR_TELNET_PORT)
osp = OpenSimPit(OPENSIMPIT_SERIAL_PORT) 

osp.add_function_for_axis(on_axis_value)
osp.add_function_for_button_state(on_button_state)
osp.add_function_for_button_press(on_button_press)
osp.add_function_for_button_release(on_button_release)
osp.add_function_for_radio_data(on_radio_data)

update_period = 1.0/UPDATE_FREQUENCY if (UPDATE_FREQUENCY > 0 and UPDATE_FREQUENCY < 50) else 0.02 # Max 50 Hz
lcd_update_period = 1.0/LCD_UPDATE_FREQUENCY if (LCD_UPDATE_FREQUENCY > 0 and LCD_UPDATE_FREQUENCY < 10) else 0.1 # Max 10 Hz

send_lcd16x2_setup()

lcd_time_counter = lcd_update_period
while True:
    try:
        lcd_time_counter -= update_period
        if (lcd_time_counter <= 0):
            lcd_time_counter = lcd_update_period
            send_lcd16x2_variables()
        
    
        osp.check()
        send_axes()
        
        
        time.sleep(update_period)
    except KeyboardInterrupt:
        break

