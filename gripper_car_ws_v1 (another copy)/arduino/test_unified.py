#!/usr/bin/env python3
"""
Unified Mobile Manipulator Test Script
Tests base motors and arm servos via serial commands
"""

import serial
import time
import sys

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200

def send_cmd(ser, cmd, wait=0.1):
    """Send command and read response"""
    ser.write((cmd + '\n').encode())
    time.sleep(wait)
    response = ""
    while ser.in_waiting:
        response += ser.read(ser.in_waiting).decode()
    return response.strip()

def test_base_motors(ser):
    """Test base motors"""
    print("\n=== BASE MOTOR TEST ===")

    # Reset encoders
    print("Resetting encoders...")
    print(send_cmd(ser, "RST"))

    # Test forward
    input("Press Enter to move FORWARD...")
    print("Moving forward (0.3 m/s)...")
    print(send_cmd(ser, "VEL,0.3,0.0"))
    time.sleep(2)
    print(send_cmd(ser, "STOP_BASE"))

    # Test backward
    input("Press Enter to move BACKWARD...")
    print("Moving backward (-0.3 m/s)...")
    print(send_cmd(ser, "VEL,-0.3,0.0"))
    time.sleep(2)
    print(send_cmd(ser, "STOP_BASE"))

    # Test rotate left
    input("Press Enter to ROTATE LEFT...")
    print("Rotating left (0.5 rad/s)...")
    print(send_cmd(ser, "VEL,0.0,0.5"))
    time.sleep(2)
    print(send_cmd(ser, "STOP_BASE"))

    # Test rotate right
    input("Press Enter to ROTATE RIGHT...")
    print("Rotating right (-0.5 rad/s)...")
    print(send_cmd(ser, "VEL,0.0,-0.5"))
    time.sleep(2)
    print(send_cmd(ser, "STOP_BASE"))

    print("\nBase motor test complete!")

def test_individual_motors(ser):
    """Test each motor individually using direct PWM"""
    print("\n=== INDIVIDUAL MOTOR TEST (Direct PWM) ===")
    motors = ["FL (Front-Left)", "FR (Front-Right)", "BL (Back-Left)", "BR (Back-Right)"]

    for idx, name in enumerate(motors):
        print(f"\nTesting Motor {idx}: {name}")

        input(f"  Press Enter to run {name} FORWARD...")
        print(send_cmd(ser, f"MOT,{idx},150"))
        time.sleep(1)
        print(send_cmd(ser, f"MOT,{idx},0"))

        input(f"  Press Enter to run {name} REVERSE...")
        print(send_cmd(ser, f"MOT,{idx},-150"))
        time.sleep(1)
        print(send_cmd(ser, f"MOT,{idx},0"))

    print("\nIndividual motor test complete!")

def test_arm_servos(ser):
    """Test arm servos"""
    print("\n=== ARM SERVO TEST ===")

    # Home position
    input("Press Enter to move arm to HOME (90,90,90,90,90,90)...")
    print(send_cmd(ser, "ARM,90,90,90,90,90,90", wait=0.5))
    time.sleep(2)

    # Test each joint
    joints = ["Base", "Shoulder", "Elbow", "Wrist1", "Wrist2", "Gripper"]

    for i, name in enumerate(joints):
        print(f"\nTesting Joint {i}: {name}")
        angles = [90, 90, 90, 90, 90, 90]

        input(f"  Press Enter to move {name} to 45 deg...")
        angles[i] = 45
        cmd = "ARM," + ",".join(str(a) for a in angles)
        print(send_cmd(ser, cmd, wait=0.5))
        time.sleep(1.5)

        input(f"  Press Enter to move {name} to 135 deg...")
        angles[i] = 135
        cmd = "ARM," + ",".join(str(a) for a in angles)
        print(send_cmd(ser, cmd, wait=0.5))
        time.sleep(1.5)

        # Return to center
        angles[i] = 90
        cmd = "ARM," + ",".join(str(a) for a in angles)
        print(send_cmd(ser, cmd, wait=0.5))
        time.sleep(1)

    print("\nArm servo test complete!")

def monitor_encoders(ser, duration=5):
    """Monitor encoder values"""
    print(f"\n=== ENCODER MONITOR ({duration}s) ===")
    print("Rotate wheels manually to see encoder changes...")

    send_cmd(ser, "RST")
    start = time.time()

    while time.time() - start < duration:
        while ser.in_waiting:
            line = ser.readline().decode().strip()
            if line.startswith("ENC,"):
                print(f"\r{line}                    ", end="", flush=True)
        time.sleep(0.05)

    print("\n")

def interactive_mode(ser):
    """Send custom commands"""
    print("\n=== INTERACTIVE MODE ===")
    print("Commands: VEL,x,z | ARM,a0,a1,a2,a3,a4,a5 | MOT,idx,pwm | STOP | RST | STATUS | quit")

    while True:
        cmd = input("\n> ").strip()
        if cmd.lower() == 'quit':
            break
        if cmd:
            ser.write((cmd + '\n').encode())
            time.sleep(0.2)
            while ser.in_waiting:
                print(ser.readline().decode().strip())

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT

    print("=" * 50)
    print("UNIFIED MOBILE MANIPULATOR TEST")
    print("=" * 50)
    print(f"Connecting to {port}...")

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)  # Wait for Arduino reset

        # Clear buffer and read startup messages
        while ser.in_waiting:
            print(ser.readline().decode().strip())

        print("\nConnected!\n")

        while True:
            print("\n" + "=" * 40)
            print("SELECT TEST:")
            print("  1. Test base motors (velocity commands)")
            print("  2. Test individual motors (direct PWM)")
            print("  3. Test arm servos")
            print("  4. Monitor encoders")
            print("  5. Interactive mode (custom commands)")
            print("  6. Quick base test (all directions)")
            print("  7. Emergency STOP")
            print("  q. Quit")
            print("=" * 40)

            choice = input("Choice: ").strip()

            if choice == '1':
                test_base_motors(ser)
            elif choice == '2':
                test_individual_motors(ser)
            elif choice == '3':
                test_arm_servos(ser)
            elif choice == '4':
                monitor_encoders(ser)
            elif choice == '5':
                interactive_mode(ser)
            elif choice == '6':
                print("\nQuick base test...")
                send_cmd(ser, "VEL,0.2,0.0"); time.sleep(1)
                send_cmd(ser, "VEL,-0.2,0.0"); time.sleep(1)
                send_cmd(ser, "VEL,0.0,0.3"); time.sleep(1)
                send_cmd(ser, "VEL,0.0,-0.3"); time.sleep(1)
                send_cmd(ser, "STOP_BASE")
                print("Done!")
            elif choice == '7':
                print(send_cmd(ser, "STOP"))
                print("STOPPED!")
            elif choice.lower() == 'q':
                send_cmd(ser, "STOP")
                break
            else:
                print("Invalid choice")

        ser.close()
        print("\nDisconnected.")

    except serial.SerialException as e:
        print(f"Error: {e}")
        print("\nCheck:")
        print("  - Arduino connected?")
        print("  - Correct port? (ls /dev/ttyACM* /dev/ttyUSB*)")
        print("  - Permission? (sudo chmod 666 /dev/ttyACM0)")
        sys.exit(1)

if __name__ == "__main__":
    main()
