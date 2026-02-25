# camera_driver

ROS2 package for camera integration with QR code detection capabilities using v4l2_camera and pyzbar.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Nodes](#nodes)
- [Usage](#usage)
- [QR Code Detection](#qr-code-detection)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides:

- **Camera driver integration** using v4l2_camera for USB/Pi cameras
- **QR code scanner** node using pyzbar library
- **Detection visualization** with annotated output images

### System Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   USB Camera    │     │  v4l2_camera    │     │   QR Scanner    │
│   /dev/video0   │────▶│     Node        │────▶│     Node        │
│                 │     │                 │     │                 │
└─────────────────┘     └────────┬────────┘     └────────┬────────┘
                                 │                        │
                                 ▼                        ▼
                        /camera/image_raw         /qr_scanner/data
                        /camera/camera_info       /qr_scanner/detection
```

---

## Package Structure

```
camera_driver/
├── package.xml             # Package manifest
├── setup.py                # Python build configuration
├── setup.cfg               # Setup configuration
├── README.md               # This documentation
├── resource/
│   └── camera_driver       # Ament resource marker
├── camera_driver/
│   ├── __init__.py         # Python module init
│   └── qr_scanner.py       # QR code detection node
├── launch/
│   └── camera.launch.py    # Camera + QR scanner launcher
└── config/
    └── camera_params.yaml  # Camera parameters
```

---

## Nodes

### qr_scanner.py

QR code detection node that processes camera images in real-time.

#### Purpose
- Subscribes to camera images
- Detects QR codes using pyzbar library
- Publishes decoded data as strings
- Provides annotated visualization

#### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/camera/image_raw` | `sensor_msgs/Image` | Raw camera images |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/qr_scanner/data` | `std_msgs/String` | Decoded QR code text |
| `/qr_scanner/detection` | `sensor_msgs/Image` | Annotated image with bounding boxes |

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `camera_topic` | string | `/camera/image_raw` | Input image topic |
| `scan_rate` | float | `10.0` | Maximum scans per second |

#### Behavior

1. Receives camera image
2. Applies rate limiting (default: 10 Hz max)
3. Converts to OpenCV format
4. Detects QR codes using pyzbar
5. If new QR detected (different from last):
   - Publishes decoded string to `/qr_scanner/data`
   - Logs detection
6. Draws bounding box on image
7. Publishes annotated image

---

## Usage

### Launch Camera and QR Scanner

```bash
# Default USB camera (/dev/video0)
ros2 launch camera_driver camera.launch.py

# Specific camera device
ros2 launch camera_driver camera.launch.py video_device:=/dev/video2
```

### Run QR Scanner Only

If camera is already running:

```bash
ros2 run camera_driver qr_scanner
```

### Monitor QR Detections

```bash
# Watch for detected QR codes
ros2 topic echo /qr_scanner/data

# View annotated detection image
ros2 run rqt_image_view rqt_image_view /qr_scanner/detection
```

### Test Camera

```bash
# Check camera is publishing
ros2 topic hz /camera/image_raw

# View raw camera feed
ros2 run rqt_image_view rqt_image_view /camera/image_raw
```

---

## QR Code Detection

### How It Works

The scanner uses the `pyzbar` library which wraps the ZBar barcode reader:

```python
from pyzbar import pyzbar

# Detect QR codes in image
qr_codes = pyzbar.decode(cv_image)

for qr in qr_codes:
    data = qr.data.decode('utf-8')  # Decoded text
    polygon = qr.polygon             # Corner points for visualization
```

### Supported Codes

pyzbar supports:
- QR Codes (primary use)
- Barcodes (EAN, UPC, Code128, etc.)
- Data Matrix codes

### Detection Performance

| Factor | Impact |
|--------|--------|
| Lighting | Good lighting improves detection |
| Distance | QR must be readable size in frame |
| Angle | Works at moderate angles |
| Resolution | Higher resolution = smaller codes detectable |

### Rate Limiting

To prevent excessive CPU usage and duplicate detections:

```python
self.scan_interval = 1.0 / scan_rate  # 10 Hz = 100ms interval

if elapsed < self.scan_interval:
    return  # Skip this frame
```

---

## Configuration

### Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `video_device` | `/dev/video0` | Camera device path |

### Camera Node Parameters

The v4l2_camera node accepts:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `video_device` | `/dev/video0` | Device path |
| `image_size` | `[640, 480]` | Resolution [width, height] |
| `camera_frame_id` | `camera_link_optical` | TF frame |
| `pixel_format` | `YUYV` | Camera pixel format |
| `fps` | `30` | Frames per second |

### camera_params.yaml

```yaml
qr_scanner:
  ros__parameters:
    camera_topic: "/camera/image_raw"
    scan_rate: 10.0  # Hz

camera:
  ros__parameters:
    video_device: "/dev/video0"
    image_size: [640, 480]
    camera_frame_id: "camera_link_optical"
```

---

## Understanding the Code

### qr_scanner.py - Key Functions

#### `image_callback(self, msg)`

Main processing function:

```python
def image_callback(self, msg: Image):
    # 1. Rate limiting
    if elapsed < self.scan_interval:
        return

    # 2. Convert ROS Image to OpenCV
    cv_image = self.bridge.imgmsg_to_cv2(msg, 'bgr8')

    # 3. Detect QR codes
    qr_codes = pyzbar.decode(cv_image)

    for qr in qr_codes:
        data = qr.data.decode('utf-8')

        # 4. Only publish new detections
        if data != self.last_qr_data:
            self.last_qr_data = data
            self.pub_data.publish(String(data=data))

        # 5. Draw visualization
        points = qr.polygon
        for i in range(4):
            cv2.line(cv_image, pts[i], pts[(i+1)%4], (0,255,0), 2)

    # 6. Publish annotated image
    self.pub_detection.publish(annotated_msg)
```

#### Dependency Handling

```python
try:
    import cv2
    from pyzbar import pyzbar
    DEPS_AVAILABLE = True
except ImportError:
    DEPS_AVAILABLE = False

# Graceful degradation if dependencies missing
if not DEPS_AVAILABLE:
    self.get_logger().error('Missing dependencies: opencv-python, pyzbar')
```

---

## Installation

### Dependencies

```bash
# System dependencies (for pyzbar)
sudo apt install libzbar0

# Python dependencies
pip3 install opencv-python pyzbar

# ROS2 camera driver
sudo apt install ros-humble-v4l2-camera

# Image transport
sudo apt install ros-humble-cv-bridge ros-humble-image-transport
```

### Build

```bash
cd ~/gripper_car_ws
colcon build --packages-select camera_driver
source install/setup.bash
```

---

## Troubleshooting

### Camera Not Found

```bash
# List video devices
ls -la /dev/video*

# Check camera works
v4l2-ctl --list-devices

# Test with OpenCV
python3 -c "import cv2; cap = cv2.VideoCapture(0); print(cap.isOpened())"
```

### Permission Denied

```bash
# Add user to video group
sudo usermod -a -G video $USER
# Log out and back in
```

### No QR Detections

1. Check camera topic is publishing:
```bash
ros2 topic hz /camera/image_raw
```

2. View camera to verify QR is visible:
```bash
ros2 run rqt_image_view rqt_image_view
```

3. Ensure QR code is:
   - Well lit
   - In focus
   - Large enough in frame
   - Not too angled

### pyzbar Import Error

```bash
# Install ZBar library
sudo apt install libzbar0 libzbar-dev

# Reinstall pyzbar
pip3 install --force-reinstall pyzbar
```

### Image Conversion Errors

```bash
# Check cv_bridge is installed
ros2 pkg list | grep cv_bridge

# Install if missing
sudo apt install ros-humble-cv-bridge
```

---

## Integration Example

### Using QR Data in Your Application

```python
from rclpy.node import Node
from std_msgs.msg import String

class MyApplication(Node):
    def __init__(self):
        super().__init__('my_app')

        # Subscribe to QR detections
        self.qr_sub = self.create_subscription(
            String, '/qr_scanner/data',
            self.qr_callback, 10)

    def qr_callback(self, msg):
        qr_data = msg.data
        self.get_logger().info(f'Detected: {qr_data}')

        # Parse QR content (example: "PICKUP:location1")
        if qr_data.startswith('PICKUP:'):
            location = qr_data.split(':')[1]
            self.navigate_to(location)
```

---

## Dependencies

- `rclpy` - ROS2 Python client
- `sensor_msgs` - Image messages
- `std_msgs` - String messages
- `cv_bridge` - ROS-OpenCV bridge
- `opencv-python` - Image processing
- `pyzbar` - QR/barcode detection
- `v4l2_camera` - Camera driver

---

## Related Packages

- [pick_and_place](../../applications/pick_and_place/README.md) - Uses QR scanner for task locations
- [mobile_manipulator_application](../../mobile_manipulator/mobile_manipulator_application/README.md) - Application integration
