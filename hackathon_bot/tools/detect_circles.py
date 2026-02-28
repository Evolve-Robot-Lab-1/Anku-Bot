#!/usr/bin/env python3
"""Find circular candidates in a frame; does not command robot hardware."""
import argparse
import glob
import json
import time
from pathlib import Path

import cv2
import numpy as np


def detect(frame, minimum_radius, maximum_radius, threshold, minimum_distance):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (9, 9), 2)
    circles = cv2.HoughCircles(
        gray, cv2.HOUGH_GRADIENT, dp=1.2, minDist=minimum_distance,
        param1=100, param2=threshold,
        minRadius=minimum_radius, maxRadius=maximum_radius,
    )
    if circles is None:
        return []
    return sorted([
        {"x_px": round(float(x), 2), "y_px": round(float(y), 2),
         "radius_px": round(float(r), 2)}
        for x, y, r in circles[0]
    ], key=lambda item: (item["x_px"], item["y_px"]))


def annotate(frame, candidates):
    result = frame.copy()
    for index, circle in enumerate(candidates, 1):
        center = (round(circle["x_px"]), round(circle["y_px"]))
        cv2.circle(result, center, round(circle["radius_px"]), (0, 255, 0), 2)
        cv2.circle(result, center, 3, (0, 0, 255), -1)
        cv2.putText(result, str(index), (center[0] + 8, center[1]),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--image", help="Test an existing image instead of a camera")
    source.add_argument("--camera", help="USB camera device path or index")
    source.add_argument("--self-test", action="store_true", help="Test synthetic circles")
    parser.add_argument("--output", default="circle_detection", help="Output directory")
    parser.add_argument("--min-radius", type=int, default=10)
    parser.add_argument("--max-radius", type=int, default=150)
    parser.add_argument("--threshold", type=float, default=35)
    parser.add_argument("--min-distance", type=float, default=40)
    args = parser.parse_args()
    if not 0 < args.min_radius <= args.max_radius or args.threshold <= 0 or args.min_distance <= 0:
        parser.error("Use positive thresholds and 0 < min-radius <= max-radius")

    if args.self_test:
        frame = np.zeros((480, 640, 3), dtype=np.uint8)
        cv2.circle(frame, (160, 200), 45, (255, 255, 255), -1)
        cv2.circle(frame, (450, 250), 60, (255, 255, 255), 3)
        origin = "synthetic"
    elif args.image:
        frame = cv2.imread(args.image)
        if frame is None:
            parser.exit(1, f"Cannot read image: {args.image}\n")
        origin = args.image
    else:
        devices = sorted(glob.glob("/dev/v4l/by-id/*video-index0"))
        device = args.camera or (devices[0] if devices else "/dev/video0")
        origin = device
        camera = cv2.VideoCapture(int(device) if device.isdigit() else device, cv2.CAP_V4L2)
        try:
            if not camera.isOpened():
                parser.exit(1, f"Cannot open USB camera {device}. Connect the webcam or specify --camera.\n")
            camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            frame = None
            ready_frames = 0
            deadline = time.monotonic() + 8
            while time.monotonic() < deadline:
                ok, captured = camera.read()
                if not ok:
                    continue
                if float(captured.mean()) < 8:
                    ready_frames = 0
                    continue
                frame = captured
                ready_frames += 1
                if ready_frames >= 3:
                    break
            if frame is None or ready_frames < 3:
                parser.exit(1, "Camera returned no usable image within 8 seconds. Check lens and lighting.\n")
        finally:
            camera.release()

    candidates = detect(frame, args.min_radius, args.max_radius,
                        args.threshold, args.min_distance)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    for name, picture in [("frame.jpg", frame), ("annotated.jpg", annotate(frame, candidates))]:
        if not cv2.imwrite(str(output / name), picture):
            parser.exit(1, f"Could not save {output / name}\n")
    result = {"source": origin, "width_px": frame.shape[1], "height_px": frame.shape[0],
              "candidates": candidates,
              "note": "Circle candidates only: disk/hole identity and arm coordinates are not calibrated."}
    (output / "circles.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if args.self_test:
        for x, y, radius in [(160, 200, 45), (450, 250, 60)]:
            if not any(abs(c["x_px"] - x) < 6 and abs(c["y_px"] - y) < 6
                       and abs(c["radius_px"] - radius) < 8 for c in candidates):
                parser.exit(1, "Synthetic circle detection test failed.\n")
        print("SELF_TEST_PASSED")


if __name__ == "__main__":
    main()
