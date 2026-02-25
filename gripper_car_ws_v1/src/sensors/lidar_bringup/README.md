# lidar_bringup

ROS2 package providing launch wrapper for YDLiDAR integration.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides a convenient launch wrapper for the YDLiDAR ROS2 driver. It simplifies bringing up the LiDAR sensor by providing a single launch point.

### Why a Wrapper Package?

- **Simplicity**: Single launch command for LiDAR
- **Customization**: Easy to add project-specific configurations
- **Modularity**: Clean separation from driver package
- **Maintainability**: Independent of driver updates

---

## Package Structure

```
lidar_bringup/
├── CMakeLists.txt      # Build configuration
├── package.xml         # Package manifest
├── README.md           # This documentation
└── launch/
    └── lidar.launch.py # LiDAR launch wrapper
```

---

## Usage

### Launch LiDAR

```bash
ros2 launch lidar_bringup lidar.launch.py
```

This includes the `ydlidar_ros2_driver` launch file with default parameters.

### Verify LiDAR is Running

```bash
# Check scan topic
ros2 topic echo /scan --once

# Check scan rate
ros2 topic hz /scan

# List LiDAR node
ros2 node list | grep lidar
```

### Visualize in RViz

```bash
# Launch RViz
rviz2

# In RViz:
# 1. Set Fixed Frame to "laser_frame"
# 2. Add LaserScan display
# 3. Set Topic to "/scan"
```

---

## Configuration

### LiDAR Parameters

The actual parameters are configured in the `ydlidar_ros2_driver` package. Common parameters include:

| Parameter | Description |
|-----------|-------------|
| `port` | Serial port (e.g., `/dev/ttyUSB0`) |
| `frame_id` | TF frame name |
| `baudrate` | Serial baud rate |
| `lidar_type` | LiDAR model type |
| `range_min` | Minimum range (m) |
| `range_max` | Maximum range (m) |
| `angle_min` | Minimum scan angle (rad) |
| `angle_max` | Maximum scan angle (rad) |

See [ydlidar_ros2_driver documentation](../ydlidar_ros2_driver/README.md) for full parameter list.

### Customizing This Wrapper

To add custom parameters, modify `lidar.launch.py`:

```python
ydlidar_launch = IncludeLaunchDescription(
    PythonLaunchDescriptionSource(
        os.path.join(ydlidar_pkg, 'launch', 'ydlidar_launch.py')
    ),
    launch_arguments={
        'port': '/dev/ttyUSB0',
        'frame_id': 'laser_frame',
    }.items()
)
```

---

## Topics

The LiDAR driver publishes:

| Topic | Type | Description |
|-------|------|-------------|
| `/scan` | `sensor_msgs/LaserScan` | 2D laser scan data |

### LaserScan Message

```
header:
  stamp: <time>
  frame_id: "laser_frame"
angle_min: -3.14159        # -180 degrees
angle_max: 3.14159         # +180 degrees
angle_increment: 0.017453  # ~1 degree
time_increment: 0.0
scan_time: 0.1             # 10 Hz
range_min: 0.12            # 12 cm minimum
range_max: 10.0            # 10 m maximum
ranges: [...]              # Distance measurements
intensities: [...]         # Signal strength (if supported)
```

---

## Troubleshooting

### LiDAR Not Found

```bash
# Check USB device
ls -la /dev/ttyUSB*

# Check device permissions
sudo chmod 666 /dev/ttyUSB0

# Add user to dialout group
sudo usermod -a -G dialout $USER
```

### No Scan Data

1. Verify driver is running:
```bash
ros2 node list | grep ydlidar
```

2. Check for errors:
```bash
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node
```

3. Verify serial port:
```bash
dmesg | grep tty
```

### TF Frame Issues

Ensure the laser frame is published in TF:

```bash
ros2 run tf2_tools view_frames
```

The frame should be connected to the robot's TF tree.

---

## Dependencies

- `ydlidar_ros2_driver` - YDLiDAR ROS2 driver
- `sensor_msgs` - LaserScan message

---

## Related Packages

- [ydlidar_ros2_driver](../../ydlidar_ros2_driver/README.md) - Actual LiDAR driver
- [mobile_manipulator_navigation](../../mobile_manipulator/mobile_manipulator_navigation/README.md) - Navigation using LiDAR
