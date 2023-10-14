import sys
import serial

last_sn = None
abort_on_error = True

def validate_sn(sn):
	global last_sn
	#print('{0:b}'.format(sn))
	if last_sn is not None and sn != (last_sn + 1) & 0xffff:
		if abort_on_error:
			print('bad sequence: {0:b}, {1:b}'.format(last_sn, sn))
			sys.exit(255)
		else:
			print('!', end='', flush=True)
	last_sn = sn

def validate(chunk):
	n, j = 0, 0
	assert len(chunk) % 32 == 0
	for i, b in enumerate(chunk):
		if i % 2:
			continue
		n |= ((b & 1) << j)
		j += 1
		if j >= 16:
			validate_sn(n)
			n, j = 0, 0


def run_test(port):
	com = serial.Serial(port)
	last_buff = None
	while True:
		chunk = com.read(1024)
		if last_buff:
			chunk, last_buff = last_buff + chunk, None
		tail = len(chunk) % 32
		if tail:
			chunk, last_buff = chunk[:-tail], chunk[-tail:]
		print('.', end='', flush=True)
		validate(chunk)

if __name__ == '__main__':
	run_test(sys.argv[1])
