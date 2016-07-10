from fcntl import ioctl
from ctypes import Structure, c_uint, c_ubyte

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
