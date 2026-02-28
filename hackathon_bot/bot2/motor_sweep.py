#!/usr/bin/env python3
"""Bot2 safe 4-motor sweep — no encoders.

HAT: Adeept Robot HAT V3.3, PCA9685 @ 0x5f over /dev/i2c-1.
Channels (from vendor 05_Motor.py, see bot2/PLAN.md):
  M1 IN1=15 IN2=14 | M2 IN1=12 IN2=13 | M3 IN1=11 IN2=10 | M4 IN1=8 IN2=9

Safety:
- Wheels must be RAISED, power stop accessible before running.
- One motor at a time, low duty (~30%), 1.0 s each, 0.5 s gap.
- All motor channels forced to 0 at start, between motors, at end,
  in finally, and on SIGINT/SIGTERM.
- Read-only probe first; no prescale change (keeps ~200 Hz from 0x1E).
- Pure stdlib (fcntl/ioctl), no pip/vendor setup needed.
"""
import os
import fcntl
import signal
import sys
import time

I2C_DEV = "/dev/i2c-1"
ADDR = 0x5F
I2C_SLAVE = 0x0703

REG_MODE1 = 0x00
REG_PRE = 0xFE
LED_BASE = 0x06

MOTORS = [
    ("M1", 15, 14),
    ("M2", 12, 13),
    ("M3", 11, 10),
    ("M4", 8, 9),
]
MOTOR_CHS = [c for _, a, b in MOTORS for c in (a, b)]

DUTY = 1228  # ~30% of 4096
PULSE_S = 1.0
GAP_S = 0.5

_fd = None
stopped = False


def i2c_open():
    global _fd
    _fd = os.open(I2C_DEV, os.O_RDWR)
    fcntl.ioctl(_fd, I2C_SLAVE, ADDR)
    return _fd


def i2c_write(reg, data):
    os.write(_fd, bytes([reg] + list(data)))


def i2c_read(reg, n=1):
    os.write(_fd, bytes([reg]))
    time.sleep(0.002)
    return os.read(_fd, n)


def set_pwm(ch, on, off):
    base = LED_BASE + 4 * ch
    i2c_write(base, [on & 0xFF, (on >> 8) & 0x0F, off & 0xFF, (off >> 8) & 0x0F])


def all_stop(reason=""):
    global stopped
    try:
        for ch in MOTOR_CHS:
            set_pwm(ch, 0, 0)
    except OSError as e:
        print(f"WARN stop failed ({reason}): {e}", flush=True)
    stopped = True
    if reason:
        print(f"STOPPED {reason}", flush=True)


def on_signal(signum, frame):
    all_stop(f"SIGNAL {signum}")
    sys.exit(130 if signum == signal.SIGINT else 143)


def main():
    signal.signal(signal.SIGINT, on_signal)
    signal.signal(signal.SIGTERM, on_signal)
    i2c_open()
    try:
        m1 = i2c_read(REG_MODE1)[0]
        pre = i2c_read(REG_PRE)[0]
        print(f"PROBE MODE1=0x{m1:02X} PRE=0x{pre:02X}", flush=True)
        if pre != 0x1E:
            print(f"WARN PRE_SCALE 0x{pre:02X} != 0x1E, leaving as-is", flush=True)
        # Wake: clear SLEEP, keep ALLCALL, enable AI
        i2c_write(REG_MODE1, [0x01])
        time.sleep(0.005)
        i2c_write(REG_MODE1, [0x21])
        time.sleep(0.005)
        print(f"INIT WAKE OK (kept PRE=0x{pre:02X})", flush=True)

        all_stop()
        print("ALL_ZERO start", flush=True)
        time.sleep(0.3)

        for name, ina, inb in MOTORS:
            # Forward: INA=PWM, INB=0
            set_pwm(ina, 0, DUTY)
            set_pwm(inb, 0, 0)
            print(f"START,MASK,{name},F,DUTY{DUTY},{PULSE_S}S ch{ina}=PWM ch{inb}=0", flush=True)
            t0 = time.monotonic()
            while time.monotonic() - t0 < PULSE_S:
                time.sleep(0.05)
            all_stop()
            print(f"STOPPED,{name} observe wheel + stops", flush=True)
            time.sleep(GAP_S)

        print("SWEEP_COMPLETE all 4 pulsed once FWD only, outputs zero", flush=True)
        print("NEXT: report per-motor spin/stop; R direction not yet tested", flush=True)
    finally:
        try:
            all_stop("FINALLY")
        except Exception:
            pass
        try:
            if _fd is not None:
                os.close(_fd)
        except Exception:
            pass


if __name__ == "__main__":
    main()
