import serial
from collections import deque
import time
from fcntl import ioctl
from ctypes import Structure, c_uint, c_ubyte

s = serial.Serial('/dev/ttyACM0', baudrate=9600, timeout=1)

cc_t = c_ubyte # /usr/include/asm-generic/termbits.h: 6

speed_t = c_uint # /usr/include/asm-generic/termbits.h: 7

tcflag_t = c_uint # /usr/include/asm-generic/termbits.h: 8

# /usr/include/asm-generic/termbits.h: 20
class termios2(Structure):
    pass

termios2.__slots__ = [
    'c_iflag',
    'c_oflag',
    'c_cflag',
    'c_lflag',
    'c_line',
    'c_cc',
    'c_ispeed',
    'c_ospeed',
]
termios2._fields_ = [
    ('c_iflag', tcflag_t),
    ('c_oflag', tcflag_t),
    ('c_cflag', tcflag_t),
    ('c_lflag', tcflag_t),
    ('c_line', cc_t),
    ('c_cc', cc_t * 19),
    ('c_ispeed', speed_t),
    ('c_ospeed', speed_t),
]

TCGETS2 = 0x802c542a
TCSETS2 = 0x402c542b
CBAUD = 0x100f
BOTHER = 0x1000

def set_baudrate(s, baud):
    tio = termios2()
    ioctl(s.fileno(), TCGETS2, tio)
    tio.c_cflag &= ~CBAUD
    tio.c_cflag |= BOTHER
    tio.c_ispeed = baud
    tio.c_ospeed = baud
    ioctl(s.fileno(), TCSETS2, tio)

    tio_read = termios2()
    ioctl(s.fileno(), TCGETS2, tio_read)

set_baudrate(s, 250000)

idx = 0
cycles = 0
t0 = time.time()
while True:
    try:
        buf = s.read(64)
        if len(buf) == 0:
            break
    except:
        break

idx = ord('A')

"""
while True:
    s.write(' ')
    c = s.read(1) 
    if c == 'o':
        print 'Got overflow!'
    if c == 'f':
        print 'Got frame error!'

    if c == '\n':
        break

print 'At start'
"""

while True:
    #pattern = (chr(idx)+chr(idx+1))*32
    pattern = ''.join(map(chr,range(ord('A'), ord('Z')+1))) + '\r\n'
    #print repr(pattern)
    s.write(pattern)
    buf = s.read(len(pattern))
    #print len(pattern), len(buf), pattern, buf
    assert buf == pattern
    idx = (idx + 2) % 255
    if idx == 0:
        cycles += 1
        tf = time.time()
        print 'Cycles %d took %f' % (cycles, tf-t0)
        t0 = tf
