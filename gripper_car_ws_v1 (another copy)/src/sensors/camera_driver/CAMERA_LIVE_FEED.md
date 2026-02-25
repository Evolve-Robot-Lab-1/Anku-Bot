# Camera Live Feed Guide

## Overview

This guide explains how to launch the Pi Camera and view the live video feed on your robot system.

## Hardware Setup

- **Camera**: Pi Camera v1.3 (OV5647) connected via CSI
- **Robot Computer**: Raspberry Pi (Ubuntu 22.04 + ROS2 Humble)
- **IP Address**: `192.168.1.7`

## Prerequisites

Install required packages on the Pi:

```bash
# Camera ROS driver (libcamera-based)
sudo apt-get install ros-humble-camera-ros ros-humble-libcamera

# For QR scanning (optional)
sudo apt-get install python3-opencv python3-pyzbar
```

---

## Launching the Camera

### On the Raspberry Pi

SSH into the Pi and run:

```bash
ssh pi@192.168.1.7
```

Launch the camera node:

```bash
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash

# Launch with default settings (800x600)
ros2 run camera_ros camera_node --ros-args -r image_raw:=/camera/image_raw

# Launch with custom resolution (640x480, RGB888 format for cv_bridge compatibility)
ros2 run camera_ros camera_node --ros-args \
    -p format:=RGB888 \
    -p width:=640 \
    -p height:=480 \
    -r image_raw:=/camera/image_raw
```

### Verify Camera is Running

```bash
# Check topics
ros2 topic list | grep camera

# Expected output:
# /camera/camera_info
# /camera/image_raw
# /camera/image_raw/compressed

# Check frame rate
ros2 topic hz /camera/image_raw
# Expected: ~15 FPS
```

---

## Viewing Live Feed

### Option 1: rqt_image_view (Recommended)

On a machine with display (local workstation):

```bash
source /opt/ros/humble/setup.bash
ros2 run rqt_image_view rqt_image_view /camera/image_raw
```

A window will open showing the live camera feed.

### Option 2: Web Video Server

Install on Pi:
```bash
sudo apt-get install ros-humble-web-video-server
```

Launch:
```bash
ros2 run web_video_server web_video_server
```

View in browser:
```
http://192.168.1.7:8080/stream?topic=/camera/image_raw
```

### Option 3: rviz2

```bash
ros2 run rviz2 rviz2
```

Add Image display, set topic to `/camera/image_raw`.

---

## Camera Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/camera/image_raw` | `sensor_msgs/Image` | Raw camera images (RGB888) |
| `/camera/camera_info` | `sensor_msgs/CameraInfo` | Camera calibration info |
| `/camera/image_raw/compressed` | `sensor_msgs/CompressedImage` | JPEG compressed images |

---

## Camera Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `format` | `NV21` | Pixel format (use `RGB888` for cv_bridge) |
| `width` | `800` | Image width |
| `height` | `600` | Image height |

### Supported Resolutions (OV5647)

- 640x480
- 800x600
- 1280x720
- 1920x1080
- 2592x1944 (full sensor)

### Supported Formats

- `RGB888` - Best for ROS/OpenCV (cv_bridge compatible)
- `BGR888`
- `YUYV`
- `NV21` (default, not cv_bridge compatible)

---

## Quick Start Commands

```bash
# === On Pi (192.168.1.7) ===

# Start camera
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run camera_ros camera_node --ros-args -p format:=RGB888 -p width:=640 -p height:=480 -r image_raw:=/camera/image_raw


# === On Local Machine ===

# View live feed
source /opt/ros/humble/setup.bash
ros2 run rqt_image_view rqt_image_view /camera/image_raw
```

---

## Troubleshooting

### Camera Not Detected

Check camera connection:
```bash
vcgencmd get_camera
# Should show: supported=1 detected=1
```

If `detected=0`:
- Check ribbon cable connection
- Ensure camera is enabled in `/boot/firmware/config.txt`
- Reboot the Pi

### NV21 Format Error with cv_bridge

If you see `Unrecognized image encoding [nv21]`:

Restart camera with RGB888 format:
```bash
ros2 run camera_ros camera_node --ros-args -p format:=RGB888 -r image_raw:=/camera/image_raw
```

### No Video in rqt_image_view

1. Ensure ROS_DOMAIN_ID matches on both machines
2. Check network connectivity: `ping 192.168.1.7`
3. Verify topic is publishing: `ros2 topic hz /camera/image_raw`

### Low Frame Rate

Reduce resolution:
```bash
ros2 run camera_ros camera_node --ros-args -p width:=320 -p height:=240 -r image_raw:=/camera/image_raw
```

---

## Integration with QR Scanner

After camera is running with RGB888 format:

```bash
# On Pi
ros2 run camera_driver qr_scanner

# Monitor detections
ros2 topic echo /qr_scanner/data
```
