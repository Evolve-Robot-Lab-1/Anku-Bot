# ydlidar_ros2_driver

ROS2 driver package for YDLIDAR LiDAR sensors, providing 2D laser scan data for navigation and obstacle detection.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Supported LiDAR Models](#supported-lidar-models)
- [Driver Node](#driver-node)
- [Launch Files](#launch-files)
- [Configuration](#configuration)
- [Usage](#usage)
- [Hardware Setup](#hardware-setup)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides a ROS2 driver for YDLIDAR 2D LiDAR sensors. It interfaces with the YDLIDAR SDK to produce standard `sensor_msgs/LaserScan` messages used by navigation stacks like Nav2.

### What is a LiDAR?

A LiDAR (Light Detection and Ranging) sensor uses laser pulses to measure distances to surrounding objects. A 2D LiDAR spins continuously, producing a 360-degree scan of the environment:

```
                    0 deg (front)
                      |
                      |
         .-----------' '-----------.
         |           |           |
   90 deg ----------------------------  -90 deg
  (left) |           |           |  (right)
         |           v           |
         '-----------------------'
                   180 deg (back)
```

### Data Flow

```
+-------------+      USB/Serial      +-------------+      /scan       +-------------+
|   YDLIDAR   | ------------------->  |   Driver    | --------------->  |    Nav2     |
|   Sensor    |    laser points      |    Node     |   LaserScan      |   SLAM      |
+-------------+                      +-------------+                  +-------------+
```

### Version

This package includes a Humble-patched version (1.0.1) optimized for X2/X2L LiDARs.

---

## Package Structure

```
ydlidar_ros2_driver/
├── CMakeLists.txt
├── package.xml
├── README.md
├── src/
│   ├── ydlidar_ros2_driver_node.cpp    # Main driver node
│   ├── ydlidar_ros2_driver_client.cpp  # Client utilities
│   └── CYdLidar.h                      # SDK header
├── launch/
│   ├── ydlidar_launch.py               # Standard launch
│   ├── ydlidar_launch_view.py          # Launch with visualization
│   ├── lidar_rviz.launch.py            # RViz visualization
│   └── ydlidar.py                      # Simple launch
├── params/
│   ├── ydlidar.yaml                    # Default parameters
│   ├── X2L.yaml                        # YDLIDAR X2L config
│   ├── X2.yaml                         # YDLIDAR X2 config
│   ├── X4.yaml                         # YDLIDAR X4 config
│   ├── X4-Pro.yaml                     # YDLIDAR X4-Pro config
│   ├── G1.yaml                         # YDLIDAR G1 config
│   ├── G2.yaml                         # YDLIDAR G2 config
│   ├── G6.yaml                         # YDLIDAR G6 config
│   ├── GS2.yaml                        # YDLIDAR GS2 config
│   ├── TG.yaml                         # YDLIDAR TG config
│   ├── TEA.yaml                        # YDLIDAR TEA config
│   └── TminiPro.yaml                   # YDLIDAR TminiPro config
├── urdf/
│   └── lidar.xacro                     # LiDAR URDF model
└── config/
    └── ydlidar.rviz                    # RViz configuration
```

---

## Supported LiDAR Models

This driver supports multiple YDLIDAR models with different specifications:

### Model Comparison

| Model | Range | Scan Rate | Baud Rate | Intensity | Use Case |
|-------|-------|-----------|-----------|-----------|----------|
| **X2L** | 0.1-8m | 5 Hz | 115200 | No | Indoor, low-cost |
| **X2** | 0.1-8m | 5 Hz | 115200 | No | Indoor, low-cost |
| **X4** | 0.1-12m | 10 Hz | 128000 | No | Indoor navigation |
| **X4-Pro** | 0.1-12m | 10 Hz | 128000 | No | Indoor navigation |
| **G1** | 0.1-12m | 10 Hz | 230400 | Yes | Indoor/outdoor |
| **G2** | 0.1-16m | 10 Hz | 230400 | Yes | Indoor/outdoor |
| **G6** | 0.1-25m | 10 Hz | 512000 | Yes | Long range |
| **TminiPro** | 0.1-12m | 10 Hz | 230400 | Yes | Compact design |

### Choosing a LiDAR

- **Budget projects**: X2L, X2 - Basic scanning, short range
- **Indoor navigation**: X4, G1 - Good balance of range and cost
- **Outdoor/long range**: G2, G6 - Extended range, intensity data
- **Compact applications**: TminiPro - Small form factor

---

## Driver Node

### ydlidar_ros2_driver_node

The main driver executable that communicates with the LiDAR hardware.

#### Published Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/scan` | `sensor_msgs/LaserScan` | 360-degree laser scan data |

#### Services

| Service | Type | Description |
|---------|------|-------------|
| `/stop_scan` | `std_srvs/Empty` | Stop LiDAR scanning |
| `/start_scan` | `std_srvs/Empty` | Start LiDAR scanning |

#### Parameters

**String Parameters:**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `port` | `/dev/ttyUSB0` | Serial port or IP address |
| `frame_id` | `laser_frame` | TF frame for scans |
| `ignore_array` | `""` | Angle ranges to ignore (e.g., `-90, -80, 30, 40`) |

**Integer Parameters:**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `baudrate` | `230400` | Serial baud rate |
| `lidar_type` | `1` | 0=TOF, 1=Triangle, 2=TOF_NET |
| `device_type` | `0` | 0=Serial, 1=TCP, 2=UDP |
| `sample_rate` | `9` | Sample rate (kHz) |
| `abnormal_check_count` | `4` | Abnormal startup data attempts |
| `intensity_bit` | `0` | Intensity data bits |

**Boolean Parameters:**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `fixed_resolution` | `true` | Fixed angular resolution |
| `reversion` | `true` | Reverse scan direction |
| `inverted` | `true` | Invert scan (ClockWise vs CounterClockWise) |
| `auto_reconnect` | `true` | Auto-reconnect on disconnect |
| `isSingleChannel` | `false` | Single channel mode |
| `intensity` | `false` | Enable intensity data |
| `support_motor_dtr` | `false` | Motor DTR control |
| `debug` | `false` | Enable debug output |
| `invalid_range_is_inf` | `false` | Invalid ranges as inf (vs 0.0) |

**Float Parameters:**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `angle_max` | `180.0` | Maximum scan angle (degrees) |
| `angle_min` | `-180.0` | Minimum scan angle (degrees) |
| `range_max` | `64.0` | Maximum range (meters) |
| `range_min` | `0.01` | Minimum range (meters) |
| `frequency` | `10.0` | Scan frequency (Hz) |

---

## Launch Files

### ydlidar_launch.py

Standard launch file for the LiDAR driver with static TF.

**Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `params_file` | `params/ydlidar.yaml` | Parameter file path |

**Launches:**
- `ydlidar_ros2_driver_node` - Main driver (lifecycle node)
- Static TF: `base_link` -> `laser_frame` (2cm above)

**Usage:**

```bash
# Default parameters
ros2 launch ydlidar_ros2_driver ydlidar_launch.py

# With specific LiDAR model
ros2 launch ydlidar_ros2_driver ydlidar_launch.py \
  params_file:=$(ros2 pkg prefix ydlidar_ros2_driver)/share/ydlidar_ros2_driver/params/X2L.yaml
```

### ydlidar_launch_view.py

Launch with RViz visualization.

```bash
ros2 launch ydlidar_ros2_driver ydlidar_launch_view.py
```

### lidar_rviz.launch.py

Launch RViz with robot state publisher for URDF visualization.

```bash
ros2 launch ydlidar_ros2_driver lidar_rviz.launch.py
```

### Launch File Summary

| Launch File | Features |
|-------------|----------|
| `ydlidar.py` | Connect with default parameters, publish `/scan` |
| `ydlidar_launch.py` | Connect with YAML config file, publish `/scan` |
| `ydlidar_launch_view.py` | Connect with YAML config + RViz visualization |
| `lidar_rviz.launch.py` | RViz with robot state publisher |

---

## Configuration

### Default Configuration (ydlidar.yaml)

```yaml
ydlidar_ros2_driver_node:
  ros__parameters:
    port: /dev/ttyUSB0
    frame_id: laser_frame
    ignore_array: ""
    baudrate: 230400
    lidar_type: 1
    device_type: 0
    isSingleChannel: false
    intensity: false
    intensity_bit: 0
    sample_rate: 9
    abnormal_check_count: 4
    fixed_resolution: true
    reversion: false
    inverted: false
    auto_reconnect: true
    support_motor_dtr: false
    angle_max: 180.0
    angle_min: -180.0
    range_max: 64.0
    range_min: 0.01
    frequency: 10.0
    invalid_range_is_inf: false
    debug: false
```

### Model-Specific Configurations

#### X2L Configuration (X2L.yaml)

```yaml
ydlidar_ros2_driver_node:
  ros__parameters:
    port: /dev/ttyUSB0
    frame_id: laser_frame
    baudrate: 115200           # X2L baud rate
    lidar_type: 1              # Triangle ranging
    isSingleChannel: true      # Single channel mode
    support_motor_dtr: true    # Motor control via DTR
    range_max: 8.0             # 8m max range
    range_min: 0.1             # 10cm min range
    frequency: 5.0             # 5 Hz scan rate
```

#### G2 Configuration (G2.yaml)

```yaml
ydlidar_ros2_driver_node:
  ros__parameters:
    port: /dev/ttyUSB0
    frame_id: laser_frame
    baudrate: 230400           # Higher baud for G2
    lidar_type: 1              # Triangle ranging
    intensity: true            # Enable intensity data
    intensity_bit: 10          # 10-bit intensity
    range_max: 16.0            # 16m max range
    frequency: 10.0            # 10 Hz scan rate
    reversion: true
    inverted: true
```

### Creating Custom Configuration

1. Copy the closest model configuration:
```bash
cp params/X4.yaml params/my_lidar.yaml
```

2. Modify parameters as needed

3. Launch with custom config:
```bash
ros2 launch ydlidar_ros2_driver ydlidar_launch.py \
  params_file:=$(ros2 pkg prefix ydlidar_ros2_driver)/share/ydlidar_ros2_driver/params/my_lidar.yaml
```

---

## Usage

### Quick Start

```bash
# Terminal 1: Launch LiDAR driver
ros2 launch ydlidar_ros2_driver ydlidar_launch.py

# Terminal 2: View scan data
ros2 topic echo /scan

# Terminal 3: Monitor scan rate
ros2 topic hz /scan
```

### Using with Specific LiDAR Model

```bash
# For X2L
ros2 launch ydlidar_ros2_driver ydlidar_launch.py \
  params_file:=$(ros2 pkg prefix ydlidar_ros2_driver)/share/ydlidar_ros2_driver/params/X2L.yaml

# For G2
ros2 launch ydlidar_ros2_driver ydlidar_launch.py \
  params_file:=$(ros2 pkg prefix ydlidar_ros2_driver)/share/ydlidar_ros2_driver/params/G2.yaml
```

### Visualize in RViz

```bash
# Use included RViz launch
ros2 launch ydlidar_ros2_driver ydlidar_launch_view.py
```

Or add LaserScan display manually:
1. Open RViz2: `rviz2`
2. Add -> By Topic -> /scan -> LaserScan
3. Set Fixed Frame to `laser_frame`

### Control Scanning

```bash
# Stop scanning
ros2 service call /stop_scan std_srvs/srv/Empty

# Start scanning
ros2 service call /start_scan std_srvs/srv/Empty
```

### Echo Scan Data

```bash
# Using client utility
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_client

# Or using topic echo
ros2 topic echo /scan
```

### Integration with Navigation

This driver is typically launched via the `lidar_bringup` package:

```bash
ros2 launch lidar_bringup lidar.launch.py
```

---

## Hardware Setup

### Physical Connection

1. **USB Connection**: Connect LiDAR to USB port
2. **Power**: Most models are USB-powered; some require external 5V

### Serial Port Setup

```bash
# Find LiDAR device
ls -la /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyUSB0

# Permanent fix: add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

### Creating udev Rule (Recommended)

Use the included setup script:

```bash
chmod 0777 src/ydlidar_ros2_driver/startup/*
sudo sh src/ydlidar_ros2_driver/startup/initenv.sh
```

Then replug the LiDAR. This creates `/dev/ydlidar` symlink.

Or create manually:

```bash
# Create udev rule
sudo nano /etc/udev/rules.d/99-ydlidar.rules
```

Add:
```
KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE:="0666", SYMLINK+="ydlidar"
```

Then:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Verifying Connection

```bash
# Check device is recognized
dmesg | tail -20

# Test serial communication
cat /dev/ttyUSB0
# Should see binary data streaming
```

---

## Understanding LaserScan Message

The `/scan` topic publishes `sensor_msgs/LaserScan`:

```
header:
  frame_id: "laser_frame"     # TF frame
  stamp: <timestamp>          # Scan time

angle_min: -3.14159           # Start angle (radians)
angle_max: 3.14159            # End angle (radians)
angle_increment: 0.0175       # Angle between rays (~1 degree)

range_min: 0.05               # Minimum valid range
range_max: 16.0               # Maximum valid range

ranges: [...]                 # Distance measurements (meters)
intensities: [...]            # Intensity values (if enabled)
```

### Interpreting Ranges

```
ranges[0]   -> angle_min direction (-180 deg, behind-right)
ranges[90]  -> front-right (~-90 deg)
ranges[180] -> front (0 deg)
ranges[270] -> front-left (~90 deg)
ranges[359] -> angle_max direction (180 deg, behind-left)

Value = inf    -> No obstacle detected
Value = 0.0    -> Invalid reading
Value = 1.5    -> Obstacle at 1.5 meters
```

---

## Troubleshooting

### LiDAR Not Starting

```bash
# Check device exists
ls -la /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyUSB0

# Verify driver is running
ros2 node list | grep ydlidar

# Check for errors with debug mode
ros2 launch ydlidar_ros2_driver ydlidar_launch.py
# Look for error messages in output
```

### "YDLidar initialize failed" Error

1. Install YDLidar SDK first:
```bash
git clone https://github.com/YDLIDAR/YDLidar-SDK
cd YDLidar-SDK
mkdir build && cd build
cmake ..
make
sudo make install
```

2. Rebuild the package:
```bash
cd ~/gripper_car_ws
colcon build --packages-select ydlidar_ros2_driver
```

### No Scan Data

1. Verify motor is spinning (listen for motor sound)
2. Check baud rate matches your model
3. Verify serial port is correct:
```bash
ros2 param get /ydlidar_ros2_driver_node port
```

### Wrong Orientation

Scans appear rotated or mirrored:

```yaml
# Try these parameters
reversion: true     # Reverse scan direction
inverted: true      # Mirror scan data
```

### Poor Scan Quality

1. Clean the optical window
2. Check for electrical interference
3. Reduce scan frequency:
```yaml
frequency: 5.0  # Lower frequency, better quality
```

### TF Errors

```bash
# Check TF tree
ros2 run tf2_tools view_frames

# Verify laser_frame exists
ros2 run tf2_ros tf2_echo base_link laser_frame
```

### Motor Not Spinning

For models with DTR motor control:
```yaml
support_motor_dtr: true
```

Check USB power is sufficient (some hubs don't provide enough current).

---

## Understanding the Code

### Main Loop (ydlidar_ros2_driver_node.cpp)

```cpp
// Initialize LiDAR
CYdLidar laser;
laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
// ... set other options from parameters ...

laser.initialize();
laser.turnOn();

// Create publisher
auto pub = node->create_publisher<sensor_msgs::msg::LaserScan>("scan", rclcpp::SensorDataQoS());

// Main scan loop at 20 Hz
rclcpp::WallRate loop_rate(20);
while (rclcpp::ok()) {
    LaserScan scan;
    if (laser.doProcessSimple(scan)) {
        // Convert SDK scan to ROS message
        auto msg = std::make_shared<sensor_msgs::msg::LaserScan>();
        msg->header.frame_id = frame_id;
        msg->angle_min = scan.config.min_angle;
        msg->angle_max = scan.config.max_angle;
        // ... fill other fields ...

        pub->publish(*msg);
    }
    rclcpp::spin_some(node);
    loop_rate.sleep();
}

// Cleanup
laser.turnOff();
laser.disconnecting();
```

### Static TF (from ydlidar_launch.py)

```python
# Publishes: base_link -> laser_frame
tf2_node = Node(
    package='tf2_ros',
    executable='static_transform_publisher',
    arguments=['0', '0', '0.02', '0', '0', '0', '1', 'base_link', 'laser_frame']
)
```

This places the laser 2cm above base_link.

---

## Dependencies

- `rclcpp` - ROS2 C++ client
- `sensor_msgs` - LaserScan message
- `geometry_msgs` - Transform messages
- `std_srvs` - Empty service
- `visualization_msgs` - Marker messages
- `tf2_ros` - Static transform publisher
- `ydlidar_sdk` - YDLIDAR SDK (must be installed separately)

---

## Building from Source

### Prerequisites

1. Install YDLidar SDK:
```bash
git clone https://github.com/YDLIDAR/YDLidar-SDK
cd YDLidar-SDK
mkdir build && cd build
cmake ..
make
sudo make install
```

2. Build the package:
```bash
cd ~/gripper_car_ws
colcon build --packages-select ydlidar_ros2_driver --symlink-install
source install/setup.bash
```

---

## Related Packages

- [lidar_bringup](../sensors/lidar_bringup/README.md) - LiDAR launch wrapper
- [mobile_manipulator_navigation](../mobile_manipulator/mobile_manipulator_navigation/README.md) - Uses LiDAR for SLAM
- [mobile_base_gazebo](../mobile_base/mobile_base_gazebo/README.md) - Simulated LiDAR

---

## External Resources

- [YDLIDAR Official Website](https://www.ydlidar.com/)
- [YDLIDAR SDK GitHub](https://github.com/YDLIDAR/YDLidar-SDK)
- [YDLIDAR ROS2 Driver GitHub](https://github.com/YDLIDAR/ydlidar_ros2_driver)

---

## Contact

For YDLIDAR support: [Contact YDLIDAR](http://www.ydlidar.cn/cn/contact)
