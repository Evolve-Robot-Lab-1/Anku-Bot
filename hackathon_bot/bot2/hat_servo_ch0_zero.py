#!/usr/bin/env python3
"""Set Bot 2 Adeept HAT servo channel 0 to vendor nominal 0 degrees."""

import fcntl
import os
import time


I2C_SLAVE = 0x0703
ADDRESS = 0x5F
MODE1 = 0x00
MODE2 = 0x01
PRE_SCALE = 0xFE
LED_BASE = 0x06
CH0_PULSE_US = 500  # Adeept 01_Servo.py: 0 deg at min_pulse=500 us.
PRE_50_HZ = 0x79
CH0_COUNT = round(CH0_PULSE_US * 50 * 4096 / 1_000_000)


fd = os.open('/dev/i2c-1', os.O_RDWR)
activated = False
try:
    fcntl.ioctl(fd, I2C_SLAVE, ADDRESS)

    def read(register):
        os.write(fd, bytes((register,)))
        return os.read(fd, 1)[0]

    def write(register, *values):
        os.write(fd, bytes((register, *values)))

    if read(MODE2) != 0x04:
        raise RuntimeError('Unexpected PCA9685 output mode; no servo command sent')
    for channel in range(16):
        off_high = read(LED_BASE + 4 * channel + 3)
        if not off_high & 0x10:
            raise RuntimeError(f'CH{channel} is already active; no servo command sent')

    # All other outputs are off. Change this PCA9685 from its motor test
    # frequency to the 50 Hz servo frequency used by Adeept's example.
    write(MODE1, 0x11)
    write(PRE_SCALE, PRE_50_HZ)
    write(MODE1, 0x01)
    time.sleep(0.005)
    write(MODE1, 0x21)

    # The HAT keeps this PWM output active after the script exits to hold CH0.
    activated = True
    write(LED_BASE, 0, 0, CH0_COUNT & 0xFF, CH0_COUNT >> 8)
    time.sleep(0.5)
    values = [read(LED_BASE + offset) for offset in range(4)]
    if values != [0, 0, CH0_COUNT & 0xFF, CH0_COUNT >> 8]:
        raise RuntimeError(f'CH0 readback mismatch: {values}')
    print(f'SET,CH0,DEG0,PULSE{CH0_PULSE_US}US,COUNT{CH0_COUNT},HOLDING')
    print(f'READBACK,MODE1=0x{read(MODE1):02X},PRE=0x{read(PRE_SCALE):02X},CH0={values}')
except BaseException:
    # If setup or readback fails, remove CH0 drive. Do not touch other channels.
    if activated:
        os.write(fd, bytes((LED_BASE + 3, 0x10)))
    raise
finally:
    os.close(fd)
