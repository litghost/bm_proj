import serial
from xbee import XBee
import binascii
import time
import Queue
from set_baudrate import set_baudrate


class MyXbee(object):
    def __init__(self, port, timeout=1):
        self.queue = Queue.Queue()
        self.timeout = 1
        self.ser = serial.Serial(port, 9600, rtscts=True, timeout=1, writeTimeout=1)
        set_baudrate(self.ser, 250000)

        try:
            self.try_at()
        except AssertionError:
            self.ser.baudrate = 9600
            self.try_at()

        check_fields = {
            'AP': '\x02',
            'D7': '\x01',
            'D6': '\x01',
            }
        self.xbee = XBee(self.ser, callback=self.queue.put, escaped=True)

        for field in check_fields:
            self.xbee.at(frame_id='\x01', command=field)
            self.get()['parameter'] == check_fields[field]

        self.update_baud_rate()
    
    def try_at(self):
        time.sleep(1)
        self.ser.write('+++')
        time.sleep(1)

        s = self.ser.read(3)
        assert s == 'OK\r'

        self.ser.write('ATAP 2\rATD7 1\rATD6 1\rATCN\r')
        time.sleep(1)
        for _ in xrange(4):
            s = self.ser.read(3)
            assert s == 'OK\r'

    def get(self, timeout=None):
        if timeout is None:
            timeout = self.timeout

        return self.queue.get(timeout=timeout)

    def update_baud_rate(self, baudrate=250000):
        baudstr = struct.pack('!I', baudrate)
        self.xbee.at(frame_id='\x01', command='BD')
        if self.get()['parameter'] == baudstr:
            return

        self.xbee.at(frame_id='\x01', command='BD', parameter=baudstr)
        self.get()
        self.xbee.at(frame_id='\x00', command='AC')

        set_baudrate(self.ser, baudrate)

        self.xbee.at(frame_id='\x01', command='BD')
        assert self.get()['parameter'] == baudstr

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.xbee.halt()

OP_NOP = 0
OP_WRITE_PAGE_BUF = 1
OP_CHECK_CRC_PAGE_BUF = 2
OP_WRITE_PAGE = 3
OP_CHECK_CRC_FLASH = 4
OP_JUMP_TO_APP = 5
OP_JUMP_TO_STAGE0 = 6
OP_APP_OP = 7

BYTES_PER_WRITE_PAGE = 64
PAGESIZE = 256

ADDRESS = binascii.unhexlify('0013A20040FC8CCB')

import random
import struct
import crcmod

ccitt = crcmod.predefined.mkCrcFun('crc-ccitt-false')

def main():
    with MyXbee('/dev/ttyUSB0') as xbee:
        def send_msg(op, data='', timeout=None):
            xbee.xbee.tx_long_addr(frame_id='\x01', dest_addr=ADDRESS, data=chr(op)+data)
            tx_status = xbee.get(timeout=timeout)
            assert tx_status['status'] == '\x00'
            reply = xbee.get(timeout=timeout)
            return reply['rf_data']

        while True:
            try:
                if send_msg(OP_NOP).strip() == 'stage0':
                    break
            except Queue.Empty:
                print 'All quiet on the western front, waiting'
                time.sleep(1)

            print 'Not in stage0, jumping'
            print send_msg(OP_JUMP_TO_STAGE0)
            time.sleep(1)

        page = '\x00'*PAGESIZE #str(bytearray(random.getrandbits(8) for _ in xrange(PAGESIZE)))

        offset = 0
        while offset < PAGESIZE:
            print 'Sending page buf at offset %d' % offset
            print send_msg(OP_WRITE_PAGE_BUF, struct.pack('!B64s', 
                offset, page[offset:offset+BYTES_PER_WRITE_PAGE])).strip()

            offset += BYTES_PER_WRITE_PAGE

        expected_crc = ccitt(page)
        print send_msg(OP_CHECK_CRC_PAGE_BUF, struct.pack('!H', expected_crc)).strip()



main()
