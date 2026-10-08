#!/usr/bin/env python3
"""Move Bot 2 Adeept HAT CH0 from vendor 0 to 180 degrees in small steps."""

import fcntl
import os
import time


I2C_SLAVE = 0x0703
ADDRESS = 0x5F
LED_BASE = 0x06
STEP_S = 0.02


def count_for_angle(degrees):
    pulse_us = 500 + 1900 * degrees / 180
    return round(pulse_us * 50 * 4096 / 1_000_000)


fd = os.open('/dev/i2c-1', os.O_RDWR)
try:
    fcntl.ioctl(fd, I2C_SLAVE, ADDRESS)

    def read(register):
        os.write(fd, bytes((register,)))
        return os.read(fd, 1)[0]

    def set_count(count):
        os.write(fd, bytes((LED_BASE, 0, 0, count & 0xFF, count >> 8)))

    if (read(0x00), read(0x01), read(0xFE)) != (0x21, 0x04, 0x79):
        raise RuntimeError('Unexpected HAT mode or frequency; CH0 unchanged')
    ch0 = [read(LED_BASE + offset) for offset in range(4)]
    initial_count = count_for_angle(0)
    if ch0 != [0, 0, initial_count & 0xFF, initial_count >> 8]:
        raise RuntimeError(f'CH0 is not at the recorded 0-degree hold: {ch0}')
    for channel in range(1, 16):
        if not read(LED_BASE + 4 * channel + 3) & 0x10:
            raise RuntimeError(f'CH{channel} is active; CH0 unchanged')

    for degrees in range(1, 181):
        set_count(count_for_angle(degrees))
        time.sleep(STEP_S)

    final_count = count_for_angle(180)
    readback = [read(LED_BASE + offset) for offset in range(4)]
    if readback != [0, 0, final_count & 0xFF, final_count >> 8]:
        raise RuntimeError(f'CH0 readback mismatch: {readback}')
    print(f'HOLDING,CH0,DEG180,PULSE2400US,COUNT{final_count}')
    print(f'READBACK,MODE1=0x{read(0x00):02X},PRE=0x{read(0xFE):02X},CH0={readback}')
finally:
    # PCA9685 continues holding the last commanded position after process exit.
    os.close(fd)
