import argparse
import sys
from PyQt4.QtGui import QDialog, QVBoxLayout, QApplication, QSpinBox, QPushButton
from PyQt4.QtCore import QSize, QThread, pyqtSignal, pyqtSlot, QObject
from program_proj import MyXbee, OP_APP_OP, OP_JUMP_TO_APP, OP_JUMP_TO_STAGE0, \
        DEFAULT_ADDR, DEFAULT_PORT
import struct
import binascii
import traceback
import time

class Worker(QObject):
    error = pyqtSignal(tuple, name='error')
    reply = pyqtSignal(str, name='reply')

    def __init__(self, xbee, remote_addr):
        QObject.__init__(self, None)
        self.xbee = xbee
        self.remote_addr = remote_addr
    
    @pyqtSlot(bytearray)
    def command(self, data):
        data = str(data)
        try:
            self.xbee.xbee.tx_long_addr(
                frame_id='\x01',
                dest_addr=self.remote_addr,
                data=str(data))
            tx_status = self.xbee.get()
            assert tx_status['status'] == '\x00'
            #reply = self.xbee.get()
            #self.reply.emit(reply['rf_data'])
        except Exception:
            self.error.emit(sys.exc_info())
            time.sleep(1)

class MainWindow(QDialog):
    send_command = pyqtSignal(bytearray, name='send_command')

    @pyqtSlot(str)
    def reply_from_app(self, reply):
        print reply

    @pyqtSlot(tuple)
    def error_from_app(self, e):
        traceback.print_exception(*e)

    @pyqtSlot()
    def go_app(self):
        s = struct.pack('!B', OP_JUMP_TO_APP)
        self.send_command.emit(bytearray(s))

    @pyqtSlot(int)
    def go_boot(self):
        s = struct.pack('!B', OP_JUMP_TO_STAGE0)
        self.send_command.emit(bytearray(s))

    @pyqtSlot(int)
    def change_value(self, v):
        s = struct.pack('!BH', OP_APP_OP, v)
        self.send_command.emit(bytearray(s))

    def stop_thread(self):
        self.thread.quit()
        self.thread.wait()

    def __init__(self, xbee, remote_addr):
        QDialog.__init__(self, None)
        self.resize(QSize(320, 240))
        self.setWindowTitle('Main window')
        layout = QVBoxLayout()
        self.setLayout(layout)
        self.box = QSpinBox()
        self.box.setMinimum(0)
        self.box.setMaximum(299)
        self.box.valueChanged.connect(self.change_value)

        layout.addWidget(self.box)

        b1 = QPushButton('Go App')
        b1.clicked.connect(self.go_app)
        layout.addWidget(b1)

        b1 = QPushButton('Go Boot')
        b1.clicked.connect(self.go_boot)
        layout.addWidget(b1)

        self.worker = Worker(xbee, remote_addr)
        self.thread = QThread()
        self.worker.moveToThread(self.thread)
        self.worker.reply.connect(self.reply_from_app)
        self.worker.error.connect(self.error_from_app)
        self.send_command.connect(self.worker.command)
        self.finished.connect(self.stop_thread)

        self.thread.start()

def main():
    parser = argparse.ArgumentParser(description='Load application on ATMEGA via XBee')
    parser.add_argument('--port', default=DEFAULT_PORT)
    parser.add_argument('--remote_addr', default=DEFAULT_ADDR)

    args = parser.parse_args()

    with MyXbee(args.port) as xbee:
        app = QApplication(sys.argv)
        mainWindow = MainWindow(xbee, binascii.unhexlify(args.remote_addr))
        mainWindow.show()
        app.exec_()

if __name__ == "__main__":
    main()
