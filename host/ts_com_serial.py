"""
Copyright (C) 2023 TeraSense
You may use, distribute and modify this code under the terms of the MIT license
Author: Oleg Volkov
"""

from serial import SerialException

class ts_com_serial:
	"""COM port communications helper"""

	def __init__(self, com):
		self.com = com

	def purge(self):
		self.com.flushOutput()
		self.com.flushInput()

	def read(self, sz):
		"""Read given amount of data"""
		buff, sz_ = '', sz
		zlimit = 2
		while True:
			s = self.com.read(sz_)
			read_sz = len(s)
			if read_sz == sz:
				return s
			if not read_sz:
				if sz_ == sz:
					zlimit -= 1
					if zlimit <= 0:
						raise SerialException("failed to read %d byte(s)" % (sz_,))
					else:
						continue
				else:
					raise SerialException("failed to read %d out of %d byte(s)" % (sz_, sz))				
			buff += s
			sz_ -= read_sz
			if not sz_:
				return buff

	def write(self, data):
		"""Write data"""
		self.com.write(data)

	def disconnect(self):
		"""Disconnect communication channel"""
		if self.is_connected():
			self.com.flushOutput()
			self.com.close()
			self.com = None

	def is_connected(self):
		"""Return True if connected, False otherwise"""
		return self.com is not None
