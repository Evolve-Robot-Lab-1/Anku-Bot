#!/usr/bin/env python3
"""
Arm Servo Channel Test Script
==============================
Tests each servo channel individually to identify faulty servos.

Usage (on RPi):
    python3 test_arm_servos.py

Serial port: /dev/ttyACM0 (change if needed)
"""

import serial
import time
import sys

# Configuration
SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

# Servo mapping - matches Arduino firmware: SERVO_CH[6] = {0, 1, 8, 4, 5, 6}
# Servo index -> PCA9685 channel
SERVOS = [
    {"idx": 0, "pca_ch": 0, "joint": "Base rotation",    "center": 90},
    {"idx": 1, "pca_ch": 1, "joint": "Shoulder",         "center": 90},
    {"idx": 2, "pca_ch": 8, "joint": "Elbow",            "center": 90},  # CH8!
    {"idx": 3, "pca_ch": 4, "joint": "Wrist",            "center": 90},
    {"idx": 4, "pca_ch": 5, "joint": "Gripper rotation", "center": 90},
    {"idx": 5, "pca_ch": 6, "joint": "Gripper",          "center": 90},
]


def connect_serial(port, baud):
    """Connect to Arduino."""
    print(f"Connecting to {port}...")
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2)  # Wait for Arduino reset

        # Read any startup messages
        while ser.in_waiting:
            line = ser.readline().decode().strip()
            print(f"  Arduino: {line}")

        print("Connected!\n")
        return ser
    except Exception as e:
        print(f"ERROR: Could not connect - {e}")
        sys.exit(1)


def send_command(ser, cmd):
    """Send command and read response."""
    ser.write((cmd + '\n').encode())
    ser.flush()
    time.sleep(0.1)

    responses = []
    while ser.in_waiting:
        line = ser.readline().decode().strip()
        if line:
            responses.append(line)
    return responses


def test_single_servo(ser, idx, angle):
    """Test a single servo."""
    cmd = f"SERVO,{idx},{angle}"
    responses = send_command(ser, cmd)
    return responses


def test_all_servos(ser, angles):
    """Test all servos at once."""
    angles_str = ",".join(str(a) for a in angles)
    cmd = f"ARM,{angles_str}"
    responses = send_command(ser, cmd)
    return responses


def center_all(ser):
    """Move all servos to center position."""
    print("Centering all servos to 90 degrees...")
    responses = test_all_servos(ser, [90, 90, 90, 90, 90, 90])
    for r in responses:
        print(f"  {r}")
    time.sleep(1)


def run_individual_test(ser):
    """Test each servo individually."""
    print("\n" + "="*60)
    print("INDIVIDUAL SERVO TEST")
    print("="*60)
    print("Each servo will move: 90 -> 60 -> 120 -> 90")
    print("Watch for movement and note any that don't respond.\n")

    results = {}

    for servo in SERVOS:
        idx = servo["idx"]
        joint = servo["joint"]
        pca_ch = servo["pca_ch"]

        print(f"\n[Servo {idx}] {joint} (PCA9685 CH{pca_ch})")
        print("-" * 40)

        input(f"  Press ENTER to test servo {idx}...")

        # Move to 60
        print(f"  Moving to 60°...")
        test_single_servo(ser, idx, 60)
        time.sleep(1)

        # Move to 120
        print(f"  Moving to 120°...")
        test_single_servo(ser, idx, 120)
        time.sleep(1)

        # Return to center
        print(f"  Returning to 90°...")
        test_single_servo(ser, idx, 90)
        time.sleep(0.5)

        # Ask user for result
        response = input(f"  Did servo {idx} ({joint}) move? [Y/n]: ").strip().lower()
        results[idx] = response != 'n'

        if not results[idx]:
            print(f"  *** SERVO {idx} MARKED AS NOT WORKING ***")

    return results


def run_sweep_test(ser):
    """Sweep all servos together."""
    print("\n" + "="*60)
    print("SWEEP TEST - ALL SERVOS")
    print("="*60)

    print("\nMoving all to 45°...")
    test_all_servos(ser, [45, 45, 45, 45, 45, 45])
    time.sleep(2)

    print("Moving all to 90°...")
    test_all_servos(ser, [90, 90, 90, 90, 90, 90])
    time.sleep(2)

    print("Moving all to 135°...")
    test_all_servos(ser, [135, 135, 135, 135, 135, 135])
    time.sleep(2)

    print("Returning to center (90°)...")
    test_all_servos(ser, [90, 90, 90, 90, 90, 90])


def run_quick_test(ser):
    """Quick automated test of all servos."""
    print("\n" + "="*60)
    print("QUICK TEST - AUTOMATED")
    print("="*60)
    print("Testing each servo: center -> -30° -> +30° -> center\n")

    for servo in SERVOS:
        idx = servo["idx"]
        joint = servo["joint"]
        pca_ch = servo["pca_ch"]

        print(f"Testing Servo {idx} ({joint}, CH{pca_ch})...", end=" ", flush=True)

        # Sweep
        test_single_servo(ser, idx, 60)
        time.sleep(0.5)
        test_single_servo(ser, idx, 120)
        time.sleep(0.5)
        test_single_servo(ser, idx, 90)
        time.sleep(0.3)

        print("Done")

    print("\nQuick test complete. Check if all servos moved.")


def print_menu():
    """Print test menu."""
    print("\n" + "="*60)
    print("ARM SERVO TEST MENU")
    print("="*60)
    print("1. Quick test (all servos automatically)")
    print("2. Individual servo test (interactive)")
    print("3. Sweep test (all servos together)")
    print("4. Test single servo by index")
    print("5. Send custom command")
    print("6. Center all servos")
    print("7. Get status")
    print("0. Exit")
    print("-"*60)


def main():
    print("\n" + "="*60)
    print("ARM SERVO CHANNEL TESTER")
    print("="*60)
    print(f"Serial Port: {SERIAL_PORT}")
    print(f"Baud Rate: {BAUD_RATE}")
    print("\nServo Channel Mapping (from Arduino SERVO_CH[6] = {0,1,8,4,5,6}):")
    for s in SERVOS:
        print(f"  Servo {s['idx']}: {s['joint']:20s} -> PCA9685 CH{s['pca_ch']}")
    print("="*60 + "\n")

    ser = connect_serial(SERIAL_PORT, BAUD_RATE)

    # Center all servos first
    center_all(ser)

    results = None

    while True:
        print_menu()
        choice = input("Select option: ").strip()

        if choice == '1':
            run_quick_test(ser)

        elif choice == '2':
            results = run_individual_test(ser)

            # Print summary
            print("\n" + "="*60)
            print("TEST RESULTS SUMMARY")
            print("="*60)
            for servo in SERVOS:
                idx = servo["idx"]
                status = "✓ WORKING" if results.get(idx, False) else "✗ NOT WORKING"
                print(f"  Servo {idx} ({servo['joint']}): {status}")

            failed = [s for s in SERVOS if not results.get(s["idx"], False)]
            if failed:
                print("\n*** FAILED SERVOS ***")
                for s in failed:
                    print(f"  - Servo {s['idx']}: {s['joint']} on PCA9685 CH{s['pca_ch']}")
                print("\nCheck:")
                print("  1. Wiring to PCA9685 channel")
                print("  2. Servo power supply")
                print("  3. Servo itself (try swapping with working one)")

        elif choice == '3':
            run_sweep_test(ser)

        elif choice == '4':
            try:
                idx = int(input("Enter servo index (0-5): "))
                angle = int(input("Enter angle (0-180): "))
                if 0 <= idx <= 5 and 0 <= angle <= 180:
                    print(f"Moving servo {idx} to {angle}°...")
                    responses = test_single_servo(ser, idx, angle)
                    for r in responses:
                        print(f"  {r}")
                else:
                    print("Invalid input!")
            except ValueError:
                print("Invalid input!")

        elif choice == '5':
            cmd = input("Enter command: ").strip()
            if cmd:
                print(f"Sending: {cmd}")
                responses = send_command(ser, cmd)
                for r in responses:
                    print(f"  Response: {r}")

        elif choice == '6':
            center_all(ser)

        elif choice == '7':
            print("Requesting status...")
            responses = send_command(ser, "STATUS")
            for r in responses:
                print(f"  {r}")

        elif choice == '0':
            print("\nCentering servos before exit...")
            center_all(ser)
            print("Goodbye!")
            break

        else:
            print("Invalid option!")

    ser.close()


if __name__ == '__main__':
    main()
