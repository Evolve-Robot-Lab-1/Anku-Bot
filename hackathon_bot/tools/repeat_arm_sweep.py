#!/usr/bin/env python3
"""Repeat the verified CH0/CH1 sweep until a stop file is created."""
from pathlib import Path
import time
import serial

stop = Path('/home/sanjeev/hackathon_bot/STOP_ARM_SWEEP')
port = '/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A5069RR4-if00-port0'
with serial.Serial(port, 115200, timeout=0.1) as s:
    try:
        time.sleep(2)
        s.reset_input_buffer()
        cycle = 0
        while not stop.exists():
            s.write(b'C')
            s.flush()
            data = bytearray()
            end = time.monotonic() + 5
            while time.monotonic() < end and not stop.exists():
                data.extend(s.read(1024))
                if b'SERVO CYCLE COMPLETE' in data and b'OUTPUT DISABLED' in data[data.find(b'SERVO CYCLE COMPLETE'):]:
                    break
            print(data.decode(errors='replace')[-1000:], flush=True)
            if stop.exists():
                break
            if b'SERVO CYCLE COMPLETE' not in data:
                raise RuntimeError('Sweep completion missing; stopping')
            cycle += 1
            print(f'Completed cycle {cycle}', flush=True)
            end = time.monotonic() + 0.4
            while time.monotonic() < end and not stop.exists():
                time.sleep(0.05)
    finally:
        s.write(b'S')
        s.flush()
        time.sleep(0.2)
        print('STOP sent; outputs disabled', flush=True)
