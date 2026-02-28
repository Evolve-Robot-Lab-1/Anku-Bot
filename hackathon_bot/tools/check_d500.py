#!/usr/bin/env python3
"""Read and validate D500 packets without transmitting commands.

CRC algorithm: ldrobotSensorTeam/ldlidar_sdk/src/ldlidar_protocol.cpp.
"""
import json
import statistics
import struct
import time
import serial


def crc8(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ (0x4D if crc & 0x80 else 0)) & 255
    return crc


def main():
    data = bytearray()
    with serial.Serial('/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0', 230400, timeout=0.2) as port:
        end = time.monotonic() + 4
        while time.monotonic() < end:
            data.extend(port.read(4096))
    valid = bad = 0
    speeds, distances, bins = [], [], set()
    offset = 0
    while True:
        start = data.find(b'\x54\x2c', offset)
        if start < 0 or start + 47 > len(data):
            break
        packet = data[start:start+47]
        if crc8(packet[:-1]) != packet[-1]:
            bad += 1
            offset = start + 1
            continue
        valid += 1
        offset = start + 47
        speed, angle_start = struct.unpack_from('<HH', packet, 2)
        angle_end = struct.unpack_from('<H', packet, 42)[0]
        speeds.append(speed / 360)
        for i in range(12):
            distance = struct.unpack_from('<H', packet, 6 + i*3)[0]
            angle = (angle_start + ((angle_end-angle_start) % 36000)*i/11) % 36000 / 100
            if 30 <= distance <= 12000:
                distances.append(distance)
                bins.add(int(angle // 10))
    result = {'bytes': len(data), 'valid_packets': valid, 'crc_failures': bad,
              'points': valid*12, 'plausible_points': len(distances), 'angle_bins_of_36': len(bins)}
    if distances:
        result.update(min_mm=min(distances), median_mm=statistics.median(distances), max_mm=max(distances))
    if speeds:
        result['median_rotation_hz'] = round(statistics.median(speeds), 2)
    print(json.dumps(result, indent=2))
    if not valid or not distances:
        raise SystemExit('No valid ranging data')


if __name__ == '__main__':
    main()
