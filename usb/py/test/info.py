import sys, serial

sys.path.append('..')
import ts_usb

com = serial.Serial(sys.argv[1])

cmd = ts_usb.cmd_request(ts_usb.INFO, False, 0, '', need_resp=True)
req = ts_usb.cmd_serialize(cmd)

com.write(req)
resp = com.read(ts_usb.PKT_LEN)
assert ts_usb.is_cmd_response(resp)

r = ts_usb.cmd_deserialize(resp)
print r
print ts_usb.parse_info(r.data)

