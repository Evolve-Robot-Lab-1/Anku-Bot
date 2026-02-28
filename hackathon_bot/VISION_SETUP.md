# Disk and hole vision setup

Pi 5 environment: Python 3, OpenCV 4.6.0, NumPy 1.26.4, and v4l2-ctl
are already installed. No additional Python package installation is needed
for this initial USB camera detector.

With a USB webcam attached to the Pi:

```bash
cd /home/sanjeev/hackathon_bot
python3 tools/detect_circles.py --output vision/latest
```

This captures a frame, detects circular candidates, and writes `frame.jpg`,
`annotated.jpg`, and `circles.json`. The camera is released after capture.
Specify `--camera /dev/video0` or a stable `/dev/v4l/by-id/` path if needed.
It uses V4L2 USB capture; a ribbon camera may require another capture backend.

To tune against an existing frame without recapturing:

```bash
python3 tools/detect_circles.py --image vision/latest/frame.jpg --min-radius 10 --max-radius 150 --threshold 35 --output vision/tuned
```

Radius limits are pixels. Lowering the threshold can find more circles but
also produces more false detections. Keep the camera fixed, use even lighting,
and choose limits appropriate for the disk and hole in the image.

Installation check without a camera:

```bash
python3 tools/detect_circles.py --self-test --output /tmp/vision-self-test
```

The detector does not identify which candidate is the disk versus the hole,
convert pixels to arm positions, or command any actuator. Next steps are a
real camera test, disk/hole classification using the actual scene, camera
calibration and a work-plane mapping, followed by calibrated arm positions.
