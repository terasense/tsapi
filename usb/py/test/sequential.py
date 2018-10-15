import sys, serial, random, threading, time

sys.path.append('..')
import ts_usb

TIMEOUT = 1

com = serial.Serial(sys.argv[1], timeout=TIMEOUT, writeTimeout=TIMEOUT)
resp_fraction = .01
mutex = threading.Lock()

def read_last_sn(com):
	com.write(ts_usb.cmd_serialize(ts_usb.cmd_request(ts_usb.INFO, False, 0, '', need_resp=True)))
	resp = ts_usb.cmd_deserialize(com.read(ts_usb.PKT_LEN))
	assert (resp.status & ts_usb.ERR_ANY) == 0
	return ts_usb.parse_info(resp.data).last_sn

def read_responses(com):
	last_read_sn = 0
	while True:
		resp = com.read(ts_usb.PKT_LEN)
		if not resp:
			print >> sys.stderr, '\nread timeout'
			break
		assert ts_usb.is_cmd_response(resp)
		r = ts_usb.cmd_deserialize(resp)
		assert ord(r.data[3]) == last_read_sn
		assert r.seq == True
		assert r.err == 0
		assert (r.status & ts_usb.ERR_ANY) == 0
		last_read_sn = r.sn
		with mutex:
			print >> sys.stderr, '*',

sn = read_last_sn(com)
print 'last sn', sn

t = threading.Thread(target=read_responses, args=(com,))
t.start()

last_resp_sn = 0

while True:
	sn = (sn + 1) & 0xff
	with_resp = (random.random() < resp_fraction)
	# Link responses via reserved data field
	data = '' if not with_resp else '\0\0\0' + chr(last_resp_sn)
	cmd = ts_usb.cmd_request(ts_usb.INFO, True, sn, data, need_resp=with_resp)
	req = ts_usb.cmd_serialize(cmd)
	com.write(req)
	if with_resp:
		last_resp_sn = sn
	with mutex:
		print >> sys.stderr, '.',

