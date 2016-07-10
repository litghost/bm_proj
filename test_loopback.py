import serial
import time
from set_baudrate import set_baudrate

s = serial.Serial('/dev/ttyACM0', baudrate=9600, timeout=1)

set_baudrate(s, 250000)

idx = 0
cycles = 0
t0 = time.time()

# Drain out random garbage
while True:
    try:
        buf = s.read(64)
        if len(buf) == 0:
            break
    except:
        break

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
