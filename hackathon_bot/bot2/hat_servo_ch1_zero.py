#!/usr/bin/env python3
"""Set Bot 2 Adeept HAT servo CH1 to vendor nominal 0 degrees."""

import fcntl
import os
import time


I2C_SLAVE = 0x0703
ADDRESS = 0x5F
LED_BASE = 0x06
CH0_BASE = LED_BASE
CH1_BASE = LED_BASE + 4
ZERO_COUNT = round(500 * 50 * 4096 / 1_000_000)


fd = os.open('/dev/i2c-1', os.O_RDWR)
activated = False
try:
    fcntl.ioctl(fd, I2C_SLAVE, ADDRESS)

    def read(register):
        os.write(fd, bytes((register,)))
        return os.read(fd, 1)[0]

    if (read(0x00), read(0x01), read(0xFE)) != (0x21, 0x04, 0x79):
        raise RuntimeError('Unexpected HAT mode or frequency; CH1 unchanged')
    expected_zero = [0, 0, ZERO_COUNT & 0xFF, ZERO_COUNT >> 8]
    ch0 = [read(CH0_BASE + offset) for offset in range(4)]
    if ch0 != expected_zero:
        raise RuntimeError(f'CH0 is not at its recorded 0-degree hold: {ch0}')
    if not read(CH1_BASE + 3) & 0x10:
        raise RuntimeError('CH1 is already active; no command sent')
    for channel in range(2, 16):
        if not read(LED_BASE + 4 * channel + 3) & 0x10:
            raise RuntimeError(f'CH{channel} is active; CH1 unchanged')

    activated = True
    os.write(fd, bytes((CH1_BASE, 0, 0, ZERO_COUNT & 0xFF, ZERO_COUNT >> 8)))
    time.sleep(0.5)
    ch1 = [read(CH1_BASE + offset) for offset in range(4)]
    if ch1 != expected_zero:
        raise RuntimeError(f'CH1 readback mismatch: {ch1}')
    print(f'HOLDING,CH1,DEG0,PULSE500US,COUNT{ZERO_COUNT}')
    print(f'READBACK,CH0={ch0},CH1={ch1},PRE=0x{read(0xFE):02X}')
except BaseException:
    if activated:
        os.write(fd, bytes((CH1_BASE + 3, 0x10)))
    raise
finally:
    os.close(fd)
