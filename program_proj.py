import serial
import argparse
from xbee import XBee
import binascii
import time
import Queue
from set_baudrate import set_baudrate
import struct
import crcmod

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

crc16 = crcmod.mkCrcFun(0x18005, 0xFFFF, True, 0)

class MyXbee(object):
    def __init__(self, port, timeout=1):
        self.queue = Queue.Queue()
        self.timeout = 1
        self.ser = serial.Serial(port, 9600, rtscts=True, timeout=1, writeTimeout=1)
        set_baudrate(self.ser, 250000)
        while True:
            try:
                s = self.ser.read(1024)
                if len(s) == 0:
                    break

                continue
            except:
                break

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

def write_application(xbee, remote_addr, data, retries):
    def send_msg(op, data='', timeout=None):
        xbee.xbee.tx_long_addr(frame_id='\x01', dest_addr=remote_addr, data=chr(op)+data)
        tx_status = xbee.get(timeout=timeout)
        assert tx_status['status'] == '\x00'
        reply = xbee.get(timeout=timeout)
        return reply['rf_data']

    def write_page(address, page):
        offset = 0
        while offset < PAGESIZE:
            assert send_msg(OP_WRITE_PAGE_BUF, struct.pack('!B64s', 
                offset, page[offset:offset+BYTES_PER_WRITE_PAGE])).strip() == 'ok'

            offset += BYTES_PER_WRITE_PAGE

        expected_crc = crc16(page)
        assert send_msg(OP_CHECK_CRC_PAGE_BUF, struct.pack('!H', expected_crc)).strip() == 'ok'

        assert send_msg(OP_WRITE_PAGE, struct.pack('!IH', address, expected_crc)).strip() == 'ok'

    def crc_flash(address, length, expected_crc):
        msg = struct.pack('!IIH', address, length, expected_crc)
        reply = send_msg(OP_CHECK_CRC_FLASH, msg, timeout=20).strip()
        print reply
        assert reply == 'ok'

    while True:
        try:
            if send_msg(OP_NOP).strip() == 'stage0':
                break
        except AssertionError:
            print 'All quiet on the western front, waiting'
            time.sleep(10)
            continue
        except Queue.Empty:
            print 'All quiet on the western front, waiting'
            time.sleep(10)
            continue

        print 'Not in stage0, jumping'
        print send_msg(OP_JUMP_TO_STAGE0)
        time.sleep(1)

    # Pad to PAGESIZE with FF
    if len(data) % PAGESIZE != 0:
        data = data + '\xff'*(PAGESIZE - (len(data) % PAGESIZE))

    offset = 0

    while offset < len(data):
        print 'Writing page at offset 0x%08x, %.2f %%' % (offset, 100*float(offset)/len(data))
        page = data[offset:offset+PAGESIZE]

        retry = 0
        while True:
            try:
                write_page(0x10000+offset, page)
                break
            except:
                retry += 1
                if retry >= retries:
                    raise

                print 'Failed to write page at offset 0x%08x, retry %d' % (offset, retry)

        offset += PAGESIZE

    crc_flash(0x10000, len(data), crc16(data))

def start_application(xbee, remote_addr):
    def send_msg(op, data='', timeout=None):
        xbee.xbee.tx_long_addr(frame_id='\x01', dest_addr=remote_addr, data=chr(op)+data)
        tx_status = xbee.get(timeout=timeout)
        assert tx_status['status'] == '\x00'
        reply = xbee.get(timeout=timeout)
        return reply['rf_data']

    assert send_msg(OP_JUMP_TO_APP).strip() == 'ok'

    while True:
        try:
            reply = send_msg(OP_NOP).strip()
            if reply == 'stage0':
                raise RuntimeError('Still in bootloader????')
            else:
                print 'Entered app %s' % reply
        except:
            print 'All quiet on the western front, waiting'
            time.sleep(10)
            continue

def main():
    parser = argparse.ArgumentParser(description='Load application on ATMEGA via XBee')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--remote_addr', default='0013A20040FC8CCB')
    parser.add_argument('--retries', type=int, default=10)
    parser.add_argument('--write-app')
    #def write_application(port, remote_addr, data, retries):

    args = parser.parse_args()

    with MyXbee(args.port) as xbee:
        if args.write_app:
            with open(args.write_app, 'rb') as f:
                data = f.read()

            write_application(
                    xbee=xbee,
                    remote_addr=binascii.unhexlify(args.remote_addr),
                    data=data,
                    retries=args.retries,
                    )

        start_application(
                xbee=xbee,
                remote_addr=binascii.unhexlify(args.remote_addr),
                )

if __name__ == '__main__':
    main()
