import os, sys, time, json
from threading import Lock


from serial import Serial
from serial.serialutil import SerialException
from serial.threaded import ReaderThread, LineReader


# ============================================================================
# SERIAL THREAD

received_list = []
serial_mutex = Lock()
received_response = False
received_ok = False
received_error = False
received_init = False
received_data = False
received_dump = False

is_debug = False

class OSPReader(LineReader):
    TERMINATOR = b'\n'
    
    def connection_made(self, transport):
        """Called when reader thread is started"""
        super(OSPReader, self).connection_made(transport)
        if is_debug:
            print("Serial connection open")

    def handle_line(self, packet):
        """Process packet"""
        decoded = _decode_packet(packet)
        if decoded is not None:
            serial_mutex.acquire()
            received_list.append(decoded)
            serial_mutex.release()

    def connection_lost(self, exc):
        if is_debug:
            print("Serial connection lost: "+str(exc))
        


def _decode_packet(packet):
    global serial_mutex
    global received_data, received_response, received_ok, received_error, received_init, received_dump

    try:
        data = json.loads(packet)
    except:
        return None
    
    if not (type(data) is dict):
        return None
    
    if is_debug:
        print("> Received: "+packet)
    
    serial_mutex.acquire()
    
    received_data = True
    if "msg" in data:
        msg = data["msg"]
        
        if msg == "OK":
            received_ok = True
            received_response = True
        
        elif msg == "ERROR":
            received_error = True
            received_response = True
            
        elif msg == "INIT":
            received_init = True
    
    if "ver" in data:
        received_dump = True
    
    serial_mutex.release()

    return data


def wait_for_flag_or_timeout(flag = 'ok'):
    global serial_mutex
    global received_data, received_response, received_ok, received_error, received_init, received_dump

    for i in range(20):
        serial_mutex.acquire()
        flag_value = False
        match flag:
            case 'ok':
                flag_value = received_ok
            case 'error':
                flag_value = received_error
            case 'init':
                flag_value = received_init
            case 'dump':
                flag_value = received_dump
        serial_mutex.release()
        
        if flag_value:
            return True
        
        time.sleep(0.1)
    return False

def check_packet():
    """Returns the next item from the received queue. If there are no items, returns None"""
    serial_mutex.acquire()
    item = received_list.pop(0) if len(received_list) > 0 else None
    serial_mutex.release()
    return item


ser = Serial()
ser.baudrate = 115200
reader = ReaderThread(ser, OSPReader)

def open_port(_port):
    global ser
    
    ser.port = _port
    try:
        ser.open()
        reader.start()
        return ser.is_open
    except SerialException:
        return False

def close_port():
    reader.close()

def send_command(data):
    if is_debug:
        print("> Sending: "+data)
    reader.write((data+'\n').encode('utf-8'))
    time.sleep(0.002) # To avoid overloading the serial port in case of too many calls
    
