#!/usr/bin/env python3
"""Move Bot 2 Adeept HAT CH1 from vendor 0 to 45 degrees."""

import fcntl
import os
import time


I2C_SLAVE = 0x0703
ADDRESS = 0x5F
LED_BASE = 0x06
CH0_BASE = LED_BASE
CH1_BASE = LED_BASE + 4


def count_for_angle(degrees):
    pulse_us = 500 + 1900 * degrees / 180
    return round(pulse_us * 50 * 4096 / 1_000_000)


fd = os.open('/dev/i2c-1', os.O_RDWR)
try:
    fcntl.ioctl(fd, I2C_SLAVE, ADDRESS)

    def read(register):
        os.write(fd, bytes((register,)))
        return os.read(fd, 1)[0]

    zero_count = count_for_angle(0)
    expected_zero = [0, 0, zero_count & 0xFF, zero_count >> 8]
    if (read(0x00), read(0x01), read(0xFE)) != (0x21, 0x04, 0x79):
        raise RuntimeError('Unexpected HAT mode or frequency; CH1 unchanged')
    if [read(CH0_BASE + offset) for offset in range(4)] != expected_zero:
        raise RuntimeError('CH0 is not at its recorded 0-degree hold')
    if [read(CH1_BASE + offset) for offset in range(4)] != expected_zero:
        raise RuntimeError('CH1 is not at its recorded 0-degree hold')
    for channel in range(2, 16):
        if not read(LED_BASE + 4 * channel + 3) & 0x10:
            raise RuntimeError(f'CH{channel} is active; CH1 unchanged')

    for degrees in range(1, 46):
        count = count_for_angle(degrees)
        os.write(fd, bytes((CH1_BASE, 0, 0, count & 0xFF, count >> 8)))
        time.sleep(0.03)

    final_count = count_for_angle(45)
    ch1 = [read(CH1_BASE + offset) for offset in range(4)]
    expected_final = [0, 0, final_count & 0xFF, final_count >> 8]
    if ch1 != expected_final:
        raise RuntimeError(f'CH1 readback mismatch: {ch1}')
    print(f'HOLDING,CH1,DEG45,PULSE975US,COUNT{final_count}')
    print(f'READBACK,CH0={expected_zero},CH1={ch1},PRE=0x{read(0xFE):02X}')
finally:
    # Keep PWM holding the last position, including if movement is interrupted.
    os.close(fd)
