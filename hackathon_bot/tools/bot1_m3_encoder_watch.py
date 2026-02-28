import time
import serial

port = "/dev/serial/by-id/usb-Arduino__www.arduino.cc__0042_1344A47413035101E7D9-if00"
s = serial.Serial(port, 115200, timeout=0.25)

def wait_for(prefix, limit):
    deadline = time.monotonic() + limit
    while time.monotonic() < deadline:
        line = s.readline().decode(errors="replace").strip()
        if line.startswith(prefix):
            return line
    raise TimeoutError(prefix)

try:
    print(wait_for("READY,FOUR_MOTOR_PCA_ARM", 8), flush=True)
    s.write(b"STOP\n")
    s.flush()
    wait_for("STOPPED", 2)
    s.write(b"RST\n")
    s.flush()
    wait_for("RESET", 2)
    deadline = time.monotonic() + 15
    last = None
    while time.monotonic() < deadline:
        line = s.readline().decode(errors="replace").strip()
        if line.startswith("ENC,M3,") and line != last:
            print(line, flush=True)
            last = line
finally:
    s.write(b"STOP\n")
    s.flush()
    s.close()
