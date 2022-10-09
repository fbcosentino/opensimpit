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
#       This is the python SDK module which enables software and game developers
#       to make python-based applications and games communicating to an
#       OpenSimPit hardware directly (without any middleware drivers). It also
#       allows to write drivers/middleware in python.
#
#       For documentation and license, check the repository at 
#       https://github.com/fbcosentino/opensimpit
#
#       (TL;DR: license is MIT)
#


import time
import osp_serial as osps


class OpenSimPit:
    """
    OpenSimPit Python SDK
    For documentation, check the repository at
    https://github.com/fbcosentino/opensimpit
    """
    
    axes_callbacks = []
    buttons_callbacks = []
    press_callbacks = []
    release_callbacks = []
    radio_callbacks = []

    def __init__(self, serial_port):
        osps.open_port(serial_port)
        time.sleep(2.5) # Some arduinos have flow control reset routine when opening the port

    def __del__(self):
        osps.close_port()

    # ========================================================================
    # PC -> BOARD

    def lcd16x2_clear(self, lcd_num):
        osps.send_command("!LCC="+str(int(lcd_num)))
        

    def lcd16x2_set_backlight(self, lcd_num, active):
        osps.send_command("!LCB="+str(int(lcd_num))+","+('1' if active else '0'))

    def lcd16x2_message(self, lcd_num, row, col, message):
        osps.send_command("!LC="+str(int(lcd_num))+","+str(int(row))+","+str(int(col))+","+message)


    def servo_set_position(self, servo_num, percentage):
        osps.send_command("!S="+str(int(servo_num))+","+str(int(percentage)))


    
    # ========================================================================
    # BOARD -> PC
    
    def check(self):
        """Called periodically to check for incoming messages"""
        
        data = osps.check_packet()
        while data != None:
            if "axis" in data:
                self._process_axes(data["axis"])
            
            if "btn" in data:
                self._process_buttons(data["btn"])

            if "rad" in data:
                self._process_radios(data["rad"])
            
            data = osps.check_packet()
    
    
    def run_forever(self):
        while True:
            try:
                self.check()
                time.sleep(0.01) # 100 Hz processing
            except KeyboardInterrupt:
                break
    
    
    def add_function_for_axis(self, func):
        self.axes_callbacks.append(func)
    

    def add_function_for_button_state(self, func):
        self.buttons_callbacks.append(func)


    def add_function_for_button_press(self, func):
        self.press_callbacks.append(func)


    def add_function_for_button_release(self, func):
        self.release_callbacks.append(func)


    def add_function_for_radio_data(self, func):
        self.radio_callbacks.append(func)


    def _process_axes(self, axes_data):
        # axes_data is a dictionary where keys are axis numbers and values are
        # integers in range 0-1023
        for axis_num in axes_data:
            axis_value = float(axes_data[axis_num]) / 1023.0
            for callback in self.axes_callbacks:
                callback(axis_num, axis_value)
        
    def _process_buttons(self, buttons_data):
        # buttons_data is a dictionary where keys are button numbers and values are
        # an integer as 0 or 1
        for btn_num in buttons_data:
            btn_value = (buttons_data[btn_num] != 0)
            # State callbacks
            for callback in self.buttons_callbacks:
                callback(btn_num, btn_value)
            # Press callbacks - only on rising edge
            if btn_value:
                for callback in self.press_callbacks:
                    callback(btn_num)
            # Release callbacks - only on falling edge
            else:
                for callback in self.release_callbacks:
                    callback(btn_num)
    
    def _process_radios(self, radio_data):
        # radio_data is a dictionary where keys are radio indices:
        # 0: COM1  1: COM2  2: NAV1  3: NAV2
        # and values are a list with 2 items: active and standby frequencies (float)
        # E.g. {"0":[101.5,108.25],"1":[120.0,121.0],"2":[98.5, 110.5],"3":[130.0,131.5]}
        for radio_num in radio_data:
            freqs = radio_data[radio_num]
            for callback in self.radio_callbacks:
                callback(radio_num, freqs[0], freqs[1]) # Number, active, standby