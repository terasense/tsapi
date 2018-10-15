from collections import namedtuple
import struct

# Packet related constants
PKT_LEN    = 64
CMD_BUF_SZ = 60
CMD_RESP   = 0x80 
CMD_MASK   = 0x7f
SEQ_MODE   = 0x80
ERR_MASK   = 0x7f

# Commands
INFO      = 0
RESET     = 1
EEPROM_RD = 0x10
EEPROM_WR = 0x11

# Error codes
ERR_PROTO = 1
ERR_SEQ   = 2
ERR_ANY = ERR_PROTO | ERR_SEQ

# The command packet representation
Cmd = namedtuple('Cmd', ('cmd', 'resp', 'seq', 'sn', 'err', 'status', 'data'))

def cmd_request(cmd, seq, sn, data, need_resp):
	"""Create request command"""
	return Cmd(cmd, need_resp, seq, sn, 0, 0, data)	

def cmd_serialize(cmd):
	"""Convert command to the packet string"""
	command = cmd.cmd
	if cmd.resp:
		command |= CMD_RESP
	s_err = 0 if not cmd.seq else SEQ_MODE
	h = struct.pack('BBBB', command, cmd.sn, s_err, 0)
	assert len(cmd.data) <= CMD_BUF_SZ
	return h + cmd.data + '\0' * (CMD_BUF_SZ - len(cmd.data))

def is_cmd_response(resp):
	"""Check if the string is command response"""
	return (ord(resp[0]) & CMD_RESP) != 0

def chk_cmd_response(resp, req):
	"""Check if the string is the response to the particular request"""
	if len(resp) != PKT_LEN:
		return False
	cmd, sn = ord(resp[0]), ord(resp[1])
	if (cmd & CMD_RESP) == 0:
		return False
	if (cmd & CMD_MASK) != req.cmd:
		return False
	if sn != req.sn:
		return False
	return True

def cmd_deserialize(resp):
	"""Create command response tuple from response string"""
	assert len(resp) == PKT_LEN
	command, sn, s_err, status = struct.unpack('BBBB', resp[:4])
	assert (command & CMD_RESP) != 0
	return Cmd(command & CMD_MASK, True, (s_err & SEQ_MODE) != 0, sn, s_err & ERR_MASK, status, resp[4:])

# The controller info data tuple
Info = namedtuple('Info', ('vmaj', 'vmin', 'last_sn', 'build'))

def parse_info(data):
	assert len(data) == CMD_BUF_SZ
	vmaj, vmin, last_sn = struct.unpack('BBB', data[:3])
	return Info(vmaj, vmin, last_sn, data[36:56])

