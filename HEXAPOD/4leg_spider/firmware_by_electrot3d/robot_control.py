#!/usr/bin/env python3
"""
Robot Control Script
Commands: F=forward, B=backward, L=left, R=right, W=hello, H=home, Q=quit
"""

import serial
import time

PORT = '/dev/ttyACM1'
BAUD = 9600

def main():
    print("Connecting to robot...")
    s = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)  # Wait for Arduino reset

    print("Robot Control - Connected!")
    print("Commands: F=forward, B=backward, L=left, R=right, W=hello, H=home, Q=quit")
    print("")

    while True:
        cmd = input(">> ").strip().upper()

        if cmd == 'Q':
            print("Bye!")
            break
        elif cmd in ['F', 'B', 'L', 'R', 'W', 'H']:
            s.write(cmd.encode())
            print(f"Sent: {cmd}")
        elif cmd.startswith('F') and cmd[1:].isdigit():
            # Multiple forward: F5 = 5 steps
            count = int(cmd[1:])
            for i in range(count):
                s.write(b'F')
                time.sleep(0.8)
            print(f"Sent: F x{count}")
        else:
            print("Commands: F, B, L, R, W, H, Q (or F5 for 5 steps)")

    s.close()

if __name__ == "__main__":
    main()
