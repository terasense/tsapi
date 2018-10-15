import sys, os, time, random
import serial

sys.path.append('..')
import ts_usb
if os.name == 'nt':
	import dev_tools

TIMEOUT = 1
MAX_DELAY = 10
PORT = sys.argv[1]
dev = None

def open_port():
	return serial.Serial(PORT, timeout=TIMEOUT, writeTimeout=TIMEOUT)

def send_req(com, req):
	try:
		com.write(ts_usb.cmd_serialize(req))
	except:
		print >> sys.stderr, '\nwrite failed'
		return False
	try:
		resp = com.read(ts_usb.PKT_LEN)
		if len(resp) != ts_usb.PKT_LEN:
			print >> sys.stderr, '\nread', len(resp), 'bytes'
			return False
		if not ts_usb.chk_cmd_response(resp, req):
			print >> sys.stderr, '\nbad response'
			return False
		r = ts_usb.cmd_deserialize(resp)
		if r.err != 0:
			print >> sys.stderr, '\nerr', r.err
			return False
		if (r.status & ts_usb.ERR_ANY) != 0:
			print >> sys.stderr, '\nstatus', r.status
			return False
		return True
	except:
		print >> sys.stderr, '\nread failed'
		return False

def reset_port(com):
	req = ts_usb.cmd_request(ts_usb.RESET, False, random.randint(0, 255), '', need_resp=True)
	return send_req(com, req)

def reconnect_port(com):
	delay = .1
	while True:
		if com.isOpen():
			com.close()
		time.sleep(delay)
		if dev is not None:
			try:
				dev_tools.dev_touch(dev)
				time.sleep(delay)
			except:
				pass
		try:
			com = open_port()
			w = com.inWaiting()
			if w:
				com.read(w)
			return com
		except:
			delay = min(MAX_DELAY, delay*2)
			print >> sys.stderr, 'reopen failed'

com = open_port()
if os.name == 'nt':
	dev = dev_tools.dev_find_port(PORT)
	assert dev is not None

while True:
	sn = 0
	if reset_port(com):
		while True:
			sn = (sn + 1) & 0xff
			req = ts_usb.cmd_request(ts_usb.INFO, True, sn, '', need_resp=True)
			if not send_req(com, req):
				break
			print >> sys.stderr, '*',
	else:
		print >> sys.stderr, 'reset failed'
	com = reconnect_port(com)

