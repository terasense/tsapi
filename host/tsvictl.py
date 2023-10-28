#!/usr/bin/python3

"""
Copyright (C) 2023 TeraSense
You may use, distribute and modify this code under the terms of the MIT license
Author: Oleg Volkov

TeraSense Smart Vision API control tool
"""

import sys
import time
import serial
import random

from serial.tools.list_ports import comports
from ts_com_serial import ts_com_serial
from serial import SerialException
from collections import namedtuple

valid_controllers = [
	'USB VID:PID=0483:5740',
]

def port_valid(descr):
	for prefix in valid_controllers:
		if descr.startswith(prefix):
			return True
	return False

def find_ports():
	for port, info, descr in comports():
		if port_valid(descr):
			yield port

class error(RuntimeError):
	def __init__(self, code, remote=False, more_info=None):
		RuntimeError.__init__(self, code)
		self.remote = remote
		self.more_info = more_info
	def __str__(self):
		e = self.code()
		s = '%s%04d: %s' % (controller.err_pref.decode(), e, controller.get_err_text(e))
		if self.remote:
			s += ' [remote]'
		if self.more_info:
			s += ' [' + self.more_info + ']'
		return s
	def code(self):
		return self.args[0]
	def is_remote(self):
		return self.remote

comm_errors  = (SerialException, IOError)
valid_errors = comm_errors + (error,)

class controller:
	"""Controller interface implementation"""
	vendor = b'TeraSense'
	family = b'SmartVision'
	timeout = 1.
	eol = b'\r'
	err_pref = b'#'
	# error codes
	err_ok          = 0
	err_state       = 1
	err_param       = 2
	err_cmd         = 3
	err_proto       = 4
	err_internal    = 5
	err_timeout     = 6
	err_not_found   = -1
	err_text = {
		err_ok          : 'No error',
		err_state       : 'Improper state to execute command',
		err_param       : 'Invalid command parameter',
		err_cmd         : 'Invalid command',
		err_proto       : 'Protocol error',
		err_internal    : 'Unexpected software error',
		err_timeout     : 'Timeout waiting paired device response',
		err_not_found   : 'Controller not found'
	}
	max_req_size  = 0x1100
	max_resp_size = 0x1100

	@staticmethod
	def get_err_text(code):
		try:
			return controller.err_text[code]
		except KeyError:
			return '<unknown>'

	def __init__(self):
		self.com  = None
		self.port = None
		self.product = None
		self.sn      = None
		self.ver_maj = None
		self.ver_min = None

	def open_port(self, port):
		com = serial.Serial(port,
				timeout=controller.timeout,
				writeTimeout=controller.timeout
			)
		if not com.isOpen():
			return False
		self.com = ts_com_serial(com)
		self.port = port
		return True

	def protocol_reset(self):
		self.com.purge()
		self.cmd_tx_request(b'-')
		time.sleep(.1)

	def read_idn(self):
		try:
			idn = self.send_command(b'*IDN?')
			if not idn.startswith(controller.vendor + b','):
				return False
			vend, prod, sn, ver = idn.split(b',')
			self.product = prod.decode()
			self.sn = int(sn, 16)
			ver_maj, ver_min = ver.split(b'.')
			self.ver_maj = int(ver_maj)
			self.ver_min = int(ver_min)
			return True
		except:
			return False

	def __str__(self):
		if not self.is_open():
			return "disconnected"
		return "[%s] %s #%x v.%d.%d" % (self.port, self.product, self.sn, self.ver_maj, self.ver_min)

	def initialize(self):
		if self.read_idn():
			return True
		self.protocol_reset()
		return self.read_idn()

	def close(self):
		if self.com:
			self.com.disconnect()
			self.com = None

	def connect_serial(self, port = None):
		assert not self.is_open()
		if port:
			if not self.open_port(port):
				return self
			if not self.initialize():
				self.close()
				return self
		else:
			for port in find_ports():
				if not self.open_port(port):
					continue
				if not self.initialize():
					self.close()
					continue
				break
			else:
				return self
		return self

	def is_open(self):
		return self.com is not None and self.com.is_connected()

	def __bool__(self):
		return self.is_open()

	def __enter__(self):
		if not self.is_open():
			raise error(controller.err_not_found)
		return self

	def __exit__(self, type, value, traceback):
		self.close()

	def cmd_tx_request(self, cmd):
		assert self.is_open()
		assert len(cmd) < controller.max_req_size
		self.com.write(cmd + controller.eol)

	def cmd_rx_response(self):
		assert self.is_open()
		first = self.com.read(1)
		if first == controller.eol:
			return b''
		if first == controller.err_pref:
			msg = self.com.read(5)
			if msg[4:] != controller.eol:
				raise error(controller.err_proto, more_info='err tail='+repr(msg[4:]))
			raise error(int(msg[:4]), remote=True)
		buff = first
		while True:
			c = self.com.read(1)
			if c == controller.eol:
				return buff
			buff += c
			if len(buff) > controller.max_resp_size:
				raise error(controller.err_proto, more_info='err resp too large')

	def send_command(self, cmd):
		if isinstance(cmd, str):
			cmd = cmd.encode()
		self.cmd_tx_request(cmd)
		return self.cmd_rx_response()

def random_str(sz):
	codes = [ord(' ')] + [random.randrange(ord('a'), ord('z') + 1) for _ in range(sz-1)]
	return bytearray(codes)

def echo_test(dev, echo_len = 0x1000):
	i, nbytes = 0, 0
	started = time.time()
	try:
		while True:
			s = random_str(random.randrange(1, echo_len))
			r = dev.send_command(b'TEST:ECHO' + s)
			if r != s:
				print ('invalid response', file=sys.stderr)
				print ('>', repr(s), file=sys.stderr)
				print ('<', repr(r), file=sys.stderr)
				rlen = len(r) if r is not None else 0
				print ('%d bytes sent, %d bytes received' % (len(s), rlen), file=sys.stderr)
				break
			i += 1
			nbytes += len(s)
			if i % 100 == 0:
				print ('.', end='', flush=True),
	except valid_errors as e:
		print (e, file=sys.stderr)
	except (EOFError, KeyboardInterrupt):
		pass
	elapsed = time.time() - started
	if nbytes and elapsed:
		print ('%u messages sent (%u bytes), %u bytes/sec' % (i, nbytes, nbytes / elapsed))

def do_test(args):
	c = controller()
	with c.connect_serial(args.port) as dev:
		print ('Found', dev)
		echo_test(dev)
	return 0

def do_terminal(args):
	c = controller()
	with c.connect_serial(args.port) as dev:
		try:
			while True:
				cmd = input('>')
				try:
					print (dev.send_command(cmd).decode())
				except error as e:
					print (e)
		except KeyboardInterrupt:
			print
	return 0

def do_send(args):
	c = controller()
	with c.connect_serial(args.port) as dev:
		print (dev.send_command(args.command).decode())
	return 0

def do_version(args):
	c = controller()
	with c.connect_serial(args.port) as dev:
		print (dev.send_command(b'SYST:VERS?').decode())
	return 0

if __name__ == '__main__':
	import traceback
	import argparse
	descr = controller.vendor.decode() + ' ' + controller.family.decode() + ' control tool'
	parser = argparse.ArgumentParser(description=descr)

	def get_help(args):
		parser.print_help()
		return 1

	parser.set_defaults(func=get_help)
	parser.add_argument('-p', '--port', help="serial port to use", default=None)
	subparsers = parser.add_subparsers()

	parser_vers = subparsers.add_parser('version', help='retrieve controller firmware version')
	parser_vers.set_defaults(func=do_version)

	parser_test = subparsers.add_parser('test', help='run echo test')
	parser_test.set_defaults(func=do_test)

	parser_term = subparsers.add_parser('terminal', help='interactive terminal')
	parser_term.set_defaults(func=do_terminal)

	parser_send = subparsers.add_parser('send', help='send command to controller')
	parser_send.set_defaults(func=do_send)
	parser_send.add_argument('command', help='command to send')

	args = parser.parse_args()
	try:
		res = args.func(args)
	except error as e:
		print ('error:', e, file=sys.stderr)
		res = 254
	except SerialException as e:
		print ('error:', e, file=sys.stderr)
		res = 254
	except:
		traceback.print_exc(file=sys.stderr)
		res = 255

	sys.exit(res)