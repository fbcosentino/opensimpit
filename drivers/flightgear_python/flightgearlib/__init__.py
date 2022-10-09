# This is a simplified FlightGear telnet driver.
# It only works running from the root virtual directory (/>)
# and all addresses are absolute
#
# Written by Fernando Cosentino for the OpenSimPit driver
#   https://github.com/fbcosentino/opensimpit



from telnetlib import Telnet
import re

ENCODING = 'ascii'
BR = b'\r\n'

class FGConnection:
    tn = None
    
    re_variable_parser = re.compile(r"(?P<var>[\w/\-_]+) *= *'(?P<value>.*)' \((?P<type>.+)\)")
    re_number_parser = re.compile(r"^-?[0-9]+(\.[0-9]+)?$")
    
    def __init__(self, port, host="localhost"):
        self.tn = Telnet(host, port)

    def _write(self, data):
        if self.tn is None:
            print("tn is None")
            return ""
        
        self.tn.write(data+BR)
        
    
    def _write_and_read_back(self, data):
        if self.tn is None:
            print("tn is None")
            return ""
        
        self.tn.read_eager()
        self.tn.write(data+BR)
        res = self.tn.read_until(b'/>', 1.0)
        return res

    def _decode_variable(self, data, expected_variable = None):
        data_str = data.decode(ENCODING)
        match_res = self.re_variable_parser.search(data_str)
        if match_res:
            var = match_res.group('var')
            value = match_res.group('value')
            value_type = match_res.group('type')
            
            if (expected_variable is not None) and (expected_variable != var):
                return None
            
            match value_type:
                case 'double':
                    return float(value)
                case _:
                    return value
        
        # Variable not understood
        else:
            return None

    def set(self, variable, value):
        res = self._write(  ('set '+str(variable)+' '+str(value)).encode(ENCODING) )
        #res = self._write_and_read_back(  ('set '+str(variable)+' '+str(value)).encode(ENCODING) )
        # TODO: confirm updated variable
    
    def get(self, variable):
        res = self._write_and_read_back(  ('get '+variable).encode(ENCODING) )
        value = self._decode_variable(res, variable)
        if value is not None:
            return value
        else:
            print("Error reading variable", variable, res)
            return None




