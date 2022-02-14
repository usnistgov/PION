import argparse
import time
from typing import Optional

import serial


class DeviceConn:
    def __init__(self, port: str):
        self.ser = serial.Serial(port=port, baudrate=115200, timeout=8)

    def hard_reset(self):
        self.ser.setDTR(0)
        self.ser.setRTS(1)
        time.sleep(0.1)
        self.ser.setRTS(0)

    def readline(self) -> Optional[str]:
        line = d.ser.readline()
        if len(line) > 0 and line[-1] == 0x0A and b' [pion' in line:
            return str(line, encoding='utf8').strip()
        return None


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='NDN onboarding device connector.')
    parser.add_argument('--port', type=str,
                        default='/dev/ttyUSB0', help='serial port')
    opts = parser.parse_args()

    d = DeviceConn(port=opts.port)
    d.hard_reset()
    while True:
        line = d.readline()
        if line is not None:
            print('%d %s' % (time.time() * 1000, line))
