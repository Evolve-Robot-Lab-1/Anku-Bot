# Smart Turn Obstacle Avoidance - Session 2026-01-04

## Overview

LiDAR-based Smart Turn obstacle avoidance for differential drive robot. When an obstacle is detected, the robot stops, reverses, scans left/right sectors, and turns toward the clearer side.

## State Machine

```
DRIVING → OBSTACLE_DETECTED → REVERSING → STOPPED → SCANNING → TURNING → CHECK_CLEAR → DRIVING
                                                         ↑                      |
                                                         +--- (if still blocked) +
```

| State | Action | Duration |
|-------|--------|----------|
| DRIVING | Pass through teleop commands | Until obstacle <20cm |
| OBSTACLE_DETECTED | Immediate stop | 100ms |
| REVERSING | Back up at -0.10 m/s | ~12cm or 2s timeout |
| STOPPED | Brief pause | 200ms |
| SCANNING | Compare left vs right sectors | 300ms |
| TURNING | Rotate toward clearer side | Until clear or 5s timeout |
| CHECK_CLEAR | Verify front >30cm clear | Retry up to 5 attempts |

## Parameters

```yaml
obstacle_threshold: 0.20m      # Trigger distance
clear_threshold: 0.30m         # Resume distance
front_sector: ±30°             # Detection cone
left_sector: -90° to -30°      # Left scan zone
right_sector: +30° to +90°     # Right scan zone
reverse_speed: -0.10 m/s
reverse_distance: 0.12m
turn_speed: 0.15-0.4 rad/s     # Proportional to clearance difference
max_attempts: 5
```

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        LOCAL PC (192.168.1.14)                   │
│  ┌─────────────────┐    ┌──────────────────┐    ┌─────────────┐ │
│  │ teleop_keyboard │───>│ obstacle_avoid.  │───>│   RViz      │ │
│  │  /cmd_vel_raw   │    │  /cmd_vel        │    │ (3D view)   │ │
│  └─────────────────┘    └────────┬─────────┘    └─────────────┘ │
│                                  │ subscribes /scan              │
└──────────────────────────────────┼──────────────────────────────┘
                                   │
                    ───────────────┼─────────────── Network
                                   │
┌──────────────────────────────────┼──────────────────────────────┐
│                        RPi (192.168.1.7)                         │
│  ┌─────────────────┐    ┌────────┴─────────┐    ┌─────────────┐ │
│  │ YDLiDAR Driver  │    │ base_hardware_   │───>│  Arduino    │ │
│  │    /scan        │    │    bridge        │    │  Mega 2560  │ │
│  └─────────────────┘    │  /cmd_vel sub    │    └──────┬──────┘ │
│                         └──────────────────┘           │        │
└────────────────────────────────────────────────────────┼────────┘
                                                         │
                                                    ┌────┴────┐
                                                    │ Motors  │
                                                    └─────────┘
```

## File Locations

| Component | Path |
|-----------|------|
| Smart Turn Node | `src/mobile_base/base_controller/scripts/obstacle_avoidance_node.py` |
| Base Controller | `src/mobile_base/base_controller/scripts/base_hardware_bridge.py` |
| LiDAR Config | `src/ydlidar_ros2_driver/params/X2L.yaml` |
| RViz 3D Config | `src/ydlidar_ros2_driver/config/ydlidar.rviz` |
| Documentation | `OBSTACLE_AVOIDANCE.md` (basic), `LIDAR_FINDINGS.md` |

## Quick Start Commands

### On RPi (usually already running):
```bash
# LiDAR driver
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 \
  -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true

# Base hardware bridge
ros2 run base_controller base_hardware_bridge.py
```

### On Local PC:
```bash
# Terminal 1 - Obstacle avoidance
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash && source install/setup.bash
ros2 run base_controller obstacle_avoidance_node.py

# Terminal 2 - Send velocity (motors move)
ros2 topic pub /cmd_vel_raw geometry_msgs/Twist "{linear: {x: 0.15}}" -r 5

# Terminal 3 - Monitor status
ros2 topic echo /obstacle_avoidance/status

# Stop motors
ros2 topic pub /cmd_vel_raw geometry_msgs/Twist "{}" -1
```

### RViz 3D View:
```bash
ros2 run rviz2 rviz2 -d src/ydlidar_ros2_driver/config/ydlidar.rviz
```

## Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/scan` | LaserScan | LiDAR data from YDLiDAR X2L |
| `/cmd_vel_raw` | Twist | Input from teleop |
| `/cmd_vel` | Twist | Output to base controller |
| `/obstacle_avoidance/status` | String | State machine status |

### Status Topic Format:
```
state:driving,front:0.51,left:0.60,right:1.63,attempts:0
```

## Sector Analysis

The node analyzes 3 sectors of the LiDAR scan:

```
        Front (±30°)
            │
   Left     │     Right
  (-90°    ─┼─    (+30°
   to       │      to
  -30°)     │    +90°)
```

- **Front sector**: Obstacle detection zone
- **Left sector**: Average distance for turn decision
- **Right sector**: Average distance for turn decision
- Robot turns toward sector with greater average distance

## Troubleshooting

### Motors Not Moving
1. Check Arduino connection: `ssh pi@192.168.1.7 "ls /dev/ttyACM*"`
2. Restart base_hardware_bridge on RPi
3. Test Arduino directly: `echo 'STATUS' > /dev/ttyACM0`

### Obstacle Always Detected
1. Increase threshold: `--ros-args -p obstacle_threshold:=0.30`
2. Check LiDAR scan: `ros2 topic echo /scan --once`

### No /scan Topic
1. Check LiDAR USB: `ssh pi@192.168.1.7 "ls /dev/ttyUSB*"`
2. Restart LiDAR driver on RPi

## Test Results (2026-01-04)

Successfully tested Smart Turn avoidance:
- Obstacle detection at <20cm: Working
- Reverse motion (~12cm): Working
- Sector scanning (left vs right): Working
- Turn toward clearer side: Working
- Resume driving when clear: Working
- Status topic for debugging: Working

Example state transitions observed:
```
driving → obstacle_detected → reversing → stopped → scanning → turning → check_clear → driving
```

## Hardware

| Component | Details |
|-----------|---------|
| LiDAR | YDLiDAR X2L, 115200 baud, /dev/ttyUSB0 |
| Arduino | Mega 2560, /dev/ttyACM0 |
| Motors | 4x DC motors, differential drive |
| RPi | Raspberry Pi 4, 192.168.1.7 |
| Local PC | 192.168.1.14 |

---

## Session Date: 2026-01-04
