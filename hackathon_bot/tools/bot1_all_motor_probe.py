import sys
import time

import serial


command = sys.argv[1]
if command != "F":
    raise SystemExit("Use F for the one-second all-motor bench test")

port = "/dev/serial/by-id/usb-Arduino__www.arduino.cc__0042_1344A47413035101E7D9-if00"
ser = serial.Serial(port, 115200, timeout=0.2)


def send(value):
    ser.write((value + "\n").encode())
    ser.flush()


def until(prefix, seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        line = ser.readline().decode(errors="replace").strip()
        if line:
            if line.startswith(("READY,", "I2C,", "PCA9685,", "START,", "STOPPED", "RESET", "ERROR,", "ENC,")):
                print(line, flush=True)
            if line.startswith(prefix):
                return line
    raise TimeoutError(f"Timed out waiting for {prefix}")


try:
    ready = until("READY,FOUR_MOTOR_PCA_ARM", 8)
    send("STOP")
    until("STOPPED", 2)
    send("RST")
    until("RESET", 2)
    send(command)
    until("START,", 2)
    until("STOPPED,TIMEOUT", 3)
    # Capture the encoder report immediately following the automatic stop.
    seen = set()
    deadline = time.monotonic() + 2
    while len(seen) < 4 and time.monotonic() < deadline:
        line = ser.readline().decode(errors="replace").strip()
        if line.startswith("ENC,M"):
            motor = line.split(",", 2)[1]
            if motor not in seen:
                print(line, flush=True)
                seen.add(motor)
finally:
    send("STOP")
    try:
        until("STOPPED", 2)
    finally:
        ser.close()
