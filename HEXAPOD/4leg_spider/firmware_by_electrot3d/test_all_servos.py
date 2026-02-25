#!/usr/bin/env python3
"""
Automated servo test - tests each servo and records positions
"""
import serial
import time
import sys

PORT = '/dev/ttyUSB0'
BAUD = 9600

def send_cmd(ser, cmd, wait=0.3):
    ser.write(cmd.encode())
    time.sleep(wait)
    response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
    return response

def test_servo(ser, channel):
    """Test a single servo and return observations"""
    print(f"\n{'='*50}")
    print(f"TESTING CH{channel}")
    print('='*50)

    # Select servo
    send_cmd(ser, str(channel), 0.5)

    # Center first
    print("Setting to 90 (center)...")
    send_cmd(ser, 'f', 0.5)
    input("  >> Look at servo. Press Enter to continue...")

    # Test 45 degrees
    print("Setting to 45 (min)...")
    send_cmd(ser, 'a', 0.5)
    at_45 = input("  >> Which direction did it move? (type observation): ")

    # Test 135 degrees
    print("Setting to 135 (max)...")
    send_cmd(ser, 'j', 0.5)
    at_135 = input("  >> Which direction did it move? (type observation): ")

    # Back to center
    print("Back to 90...")
    send_cmd(ser, 'f', 0.5)

    # Ask for physical center
    print("\nNow find the PHYSICAL CENTER (where leg looks neutral)")
    print("Use: + (add 5), - (sub 5), ] (add 1), [ (sub 1)")

    while True:
        cmd = input("  >> Enter command (+/-/]/[) or 'done' when centered: ")
        if cmd.lower() == 'done':
            break
        if cmd in ['+', '-', '[', ']']:
            resp = send_cmd(ser, cmd, 0.3)
            # Extract angle from response
            if 'Angle:' in resp:
                print(f"     {resp.strip()}")

    # Get final angle
    resp = send_cmd(ser, 'p', 0.5)

    # Ask for the center value
    center = input("  >> What angle is the physical center? ")

    return {
        'channel': channel,
        'at_45': at_45,
        'at_135': at_135,
        'center': center
    }

def main():
    print("="*50)
    print("SPIDER ROBOT - FULL SERVO TEST")
    print("="*50)
    print("\nThis will test each servo (0-7)")
    print("You'll observe movement and record directions\n")

    input("Press Enter when robot is powered and ready...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)  # Wait for Arduino reset

        # Clear buffer
        ser.read(ser.in_waiting)

        results = []

        servo_names = [
            "Front Right HIP",
            "Front Left HIP",
            "Front Right LEG",
            "Front Left LEG",
            "Back Right HIP",
            "Back Left HIP",
            "Back Right LEG",
            "Back Left LEG"
        ]

        for i in range(8):
            print(f"\n>>> Next: CH{i} - {servo_names[i]}")
            cont = input("Press Enter to test (or 'skip' to skip): ")
            if cont.lower() == 'skip':
                continue

            result = test_servo(ser, i)
            result['name'] = servo_names[i]
            results.append(result)

        # Print summary
        print("\n" + "="*60)
        print("TEST RESULTS SUMMARY")
        print("="*60)
        print(f"{'CH':<4} {'Name':<20} {'Center':<8} {'@45':<15} {'@135':<15}")
        print("-"*60)

        for r in results:
            print(f"{r['channel']:<4} {r['name']:<20} {r['center']:<8} {r['at_45']:<15} {r['at_135']:<15}")

        # Print as code
        print("\n// Code format:")
        centers = ['90'] * 8
        for r in results:
            centers[r['channel']] = str(r['center'])
        print(f"int servoCenter[8] = {{{', '.join(centers)}}};")

        ser.close()

    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
