# LiDAR Findings - 2026-01-03

## Hardware Configuration

### LiDAR Model: YDLiDAR X2L

| Parameter | Value |
|-----------|-------|
| Model | X2L (Model 6) |
| Type | 2D Triangle Ranging |
| Baudrate | 115200 |
| Sample Rate | 3K-4K |
| Range | 0.10 - 8.0m |
| Frequency | 4-8 Hz (PWM controlled) |
| SingleChannel | true |
| DTR Motor Support | true |
| Intensity | false |
| Reversion | false |
| Voltage | 4.8-5.2V |

### Connection Details

| Setting | Value |
|---------|-------|
| Device | `/dev/ttyUSB0` |
| USB Chip | Silicon Labs CP210x (ID: 10c4:ea60) |
| Host | Raspberry Pi 4 (192.168.1.7) |
| OS | Ubuntu 22.04 |
| ROS | ROS2 Humble |

---

## Network Setup

```
Local PC (192.168.1.14) <---> RPi (192.168.1.7)
     |                            |
   RViz                      LiDAR Driver
   SLAM                      /dev/ttyUSB0
```

### SSH Access
```bash
ssh pi@192.168.1.7
```

---

## ROS2 Topics

| Topic | Type | Publisher |
|-------|------|-----------|
| `/scan` | sensor_msgs/LaserScan | ydlidar_ros2_driver_node |
| `/map` | nav_msgs/OccupancyGrid | slam_toolbox |
| `/tf` | tf2_msgs/TFMessage | static_transform_publisher |
| `/tf_static` | tf2_msgs/TFMessage | static_transform_publisher |

### LaserScan Message Details
```
frame_id: laser_frame
angle_min: -3.14159 rad (-180 deg)
angle_max: 3.14159 rad (+180 deg)
range_min: 0.1 m
range_max: 8.0 m (spec) / 16.0 m (driver default)
```

---

## File Locations

### YDLidar Driver Package
**Path:** `src/ydlidar_ros2_driver/`

| File | Purpose |
|------|---------|
| `launch/ydlidar_launch.py` | Main launch file |
| `launch/ydlidar_launch_view.py` | Launch with RViz |
| `launch/lidar_rviz.launch.py` | RViz + URDF visualization |
| `launch/ydlidar.py` | Simple launch |
| `params/X2L.yaml` | **X2L configuration** |
| `params/ydlidar.yaml` | Default config |
| `config/ydlidar.rviz` | RViz config for LiDAR |
| `urdf/lidar.xacro` | LiDAR URDF model |

### LiDAR Bringup Wrapper
**Path:** `src/sensors/lidar_bringup/`

| File | Purpose |
|------|---------|
| `launch/lidar.launch.py` | Simple launch wrapper |

### SLAM/Navigation Configuration
**Path:** `src/mobile_manipulator/mobile_manipulator_navigation/`

| File | Purpose |
|------|---------|
| `config/mapping_params.yaml` | SLAM toolbox config |
| `config/slam_params.yaml` | Alternative SLAM config |
| `config/localisation_params.yaml` | Localization config |
| `config/nav2_params.yaml` | Navigation2 params |
| `launch/slam_mapping.launch.py` | Full SLAM launch |
| `launch/navigation.launch.py` | Navigation launch |

### Robot Bringup
**Path:** `src/bringup/robot_bringup/`

| File | Purpose |
|------|---------|
| `launch/slam.launch.py` | Robot + SLAM |
| `launch/robot.launch.py` | Full robot launch |
| `launch/navigation.launch.py` | Navigation launch |

### RViz Configurations
| File | Purpose |
|------|---------|
| `src/ydlidar_ros2_driver/config/ydlidar.rviz` | LiDAR visualization |
| `src/mobile_base/mobile_base_gazebo/config/nav.rviz` | Navigation view |
| `src/mobile_base/mobile_base_gazebo/config/sim.rviz` | Simulation view |

### Saved Maps
**Path:** `maps/`

| File | Purpose |
|------|---------|
| `test_map.pgm` | Occupancy grid image |
| `test_map.yaml` | Map metadata |

---

## X2L.yaml Configuration

```yaml
ydlidar_ros2_driver_node:
  ros__parameters:
    port: /dev/ttyUSB0
    frame_id: laser_frame
    ignore_array: ""
    baudrate: 115200
    lidar_type: 1              # Triangle ranging
    device_type: 0             # Serial
    sample_rate: 3
    abnormal_check_count: 4
    fixed_resolution: true
    reversion: false
    inverted: true
    auto_reconnect: true
    isSingleChannel: true
    intensity: false
    support_motor_dtr: true
    angle_max: 180.0
    angle_min: -180.0
    range_max: 8.0
    range_min: 0.1
    frequency: 7.0
    invalid_range_is_inf: false
```

---

## SLAM Configuration (mapping_params.yaml)

```yaml
slam_toolbox:
  ros__parameters:
    solver_plugin: solver_plugins::CeresSolver
    ceres_linear_solver: SPARSE_NORMAL_CHOLESKY
    ceres_preconditioner: SCHUR_JACOBI
    ceres_trust_strategy: LEVENBERG_MARQUARDT

    # Frame configuration
    odom_frame: odom
    map_frame: map
    base_frame: body_link
    scan_topic: /scan

    # Mapping parameters
    mode: mapping
    resolution: 0.05
    max_laser_range: 20.0
    minimum_time_interval: 0.5
    transform_publish_period: 0.02
    map_update_interval: 5.0

    # Loop closure
    do_loop_closing: true
    loop_search_maximum_distance: 3.0
    loop_match_minimum_chain_size: 10
```

---

## Commands Reference

### Start LiDAR on RPi
```bash
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 \
  -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true
```

### Start SLAM (Local PC)
```bash
source /opt/ros/humble/setup.bash

# TF frames
ros2 run tf2_ros static_transform_publisher --frame-id odom --child-frame-id body_link
ros2 run tf2_ros static_transform_publisher --frame-id body_link --child-frame-id laser_frame --z 0.1

# SLAM
ros2 launch slam_toolbox online_async_launch.py \
  slam_params_file:=/path/to/mapping_params.yaml use_sim_time:=false
```

### Visualize in RViz
```bash
# Using YDLidar config
rviz2 -d src/ydlidar_ros2_driver/config/ydlidar.rviz

# Or custom SLAM config
rviz2 -d slam_rviz.rviz
```

### Save Map
```bash
ros2 run nav2_map_server map_saver_cli -f maps/my_map
```

### Check Topics
```bash
ros2 topic list
ros2 topic echo /scan --once
ros2 topic hz /scan
```

---

## TF Frame Tree

```
map
 └── odom
      └── body_link
           └── laser_frame
```

### Required Static Transforms
```bash
# odom -> body_link (robot base)
ros2 run tf2_ros static_transform_publisher --frame-id odom --child-frame-id body_link

# body_link -> laser_frame (LiDAR mount)
ros2 run tf2_ros static_transform_publisher --frame-id body_link --child-frame-id laser_frame --z 0.1
```

---

## Troubleshooting

### LiDAR Not Detected
```bash
# Check USB device
lsusb | grep "10c4:ea60"
ls -la /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyUSB0
sudo usermod -a -G dialout $USER
```

### USB Over-Current Error
- Use powered USB hub
- Connect directly to RPi (not through unpowered hub)
- Try different USB cable (must be data cable, not charge-only)

### SLAM TF Errors
- Ensure all TF frames are published
- Check frame names match config (body_link vs base_link)
- Verify timestamps are synchronized

### QoS Mismatch in RViz
- Change LaserScan reliability to "Best Effort"
- Or change in driver params

---

## Hardware Issues Encountered

### Issue: USB Over-Current
**Symptom:** `over-current change` in dmesg
**Cause:** RPi USB port can't supply enough current
**Solution:** Use powered USB hub or external power

### Issue: No ttyUSB Device
**Symptom:** LiDAR motor spins but no /dev/ttyUSB0
**Cause:** Bad USB cable (power-only, no data lines)
**Solution:** Use data-capable USB cable

---

## Available Launch Files Summary

### For Real Hardware
| Launch | Command |
|--------|---------|
| LiDAR + RViz | `ros2 launch ydlidar_ros2_driver ydlidar_launch_view.py` |
| LiDAR only | `ros2 launch lidar_bringup lidar.launch.py` |
| SLAM Mapping | `ros2 launch mobile_manipulator_navigation slam_mapping.launch.py` |
| Full Robot | `ros2 launch robot_bringup robot.launch.py` |

### For Simulation
| Launch | Command |
|--------|---------|
| Full Sim | `ros2 launch mobile_manipulator_sim sim.launch.py` |
| Sim + SLAM | `ros2 launch navigation_sim sim_mapping.launch.py` |

---

## Notes

1. **2D LiDAR Limitation:** X2L is 2D only, cannot create 3D maps
2. **Static TF:** Without odometry, SLAM relies on scan-matching only
3. **Map Updates:** Move LiDAR physically to expand the map
4. **RPi Workspace:** `/home/pi/pi_ros_ws/gripper_car_ws/`
5. **Local Workspace:** `/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/`

---

## Session Date: 2026-01-03
