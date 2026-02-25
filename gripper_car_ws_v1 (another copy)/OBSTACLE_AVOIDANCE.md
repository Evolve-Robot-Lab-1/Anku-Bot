# Obstacle Avoidance System

## Overview

LiDAR-based obstacle avoidance for the mobile manipulator robot. When an obstacle is detected within the threshold distance in front of the robot, forward motion is blocked while rotation and reverse remain enabled.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        LOCAL PC                                  │
│  ┌─────────────────┐    ┌──────────────────┐    ┌─────────────┐ │
│  │ teleop_keyboard │───>│ obstacle_avoid.  │───>│   RViz      │ │
│  │  /cmd_vel_raw   │    │  /cmd_vel        │    │ (optional)  │ │
│  └─────────────────┘    └────────┬─────────┘    └─────────────┘ │
│                                  │ subscribes /scan              │
└──────────────────────────────────┼──────────────────────────────┘
                                   │
                    ───────────────┼─────────────── Network
                                   │
┌──────────────────────────────────┼──────────────────────────────┐
│                        RPi (192.168.1.7)                         │
│  ┌─────────────────┐    ┌────────┴─────────┐    ┌─────────────┐ │
│  │ YDLiDAR Driver  │───>│ base_hardware_   │───>│  Arduino    │ │
│  │    /scan        │    │    bridge        │    │  Mega 2560  │ │
│  └─────────────────┘    │  /cmd_vel sub    │    └──────┬──────┘ │
│                         └──────────────────┘           │        │
└────────────────────────────────────────────────────────┼────────┘
                                                         │
                                                    ┌────┴────┐
                                                    │ Motors  │
                                                    └─────────┘
```

## Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `obstacle_threshold` | 0.20m | Distance to detect obstacle (20cm) |
| `front_angle` | 30° | Detection cone (±30° = 60° total) |
| `scan_topic` | /scan | LiDAR scan topic |
| `cmd_vel_in` | /cmd_vel_raw | Input velocity topic |
| `cmd_vel_out` | /cmd_vel | Output velocity topic |

## Hardware Requirements

- **LiDAR:** YDLiDAR X2L connected to RPi via USB (/dev/ttyUSB0)
- **Arduino:** Mega 2560 connected to RPi via USB (/dev/ttyACM0)
- **Network:** Local PC and RPi on same network (192.168.1.x)

---

## Quick Start

### Step 1: SSH to RPi and Start LiDAR

```bash
# Terminal 1 - LiDAR
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 \
  -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true
```

### Step 2: Start Base Controller on RPi

```bash
# Terminal 2 - Base Controller
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run base_controller base_hardware_bridge.py --ros-args -p serial_port:=/dev/ttyACM0
```

### Step 3: Start Obstacle Avoidance on Local PC

```bash
# Terminal 3 - Obstacle Avoidance
source /opt/ros/humble/setup.bash
source /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/install/setup.bash
ros2 run base_controller obstacle_avoidance_node.py
```

### Step 4: Start Teleop on Local PC

```bash
# Terminal 4 - Teleop
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=/cmd_vel_raw
```

---

## Testing

### Quick Motor Test (No Obstacle Avoidance)

```bash
# Direct motor control - bypasses obstacle avoidance
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.15}}" -1
sleep 3
ros2 topic pub /cmd_vel geometry_msgs/Twist "{}" -1
```

### Hand Test (With Obstacle Avoidance)

```bash
# Start forward motion for 15 seconds
ros2 topic pub /cmd_vel_raw geometry_msgs/Twist "{linear: {x: 0.15}}" -r 5 &
PUB_PID=$!
sleep 15
kill $PUB_PID
ros2 topic pub /cmd_vel_raw geometry_msgs/Twist "{}" -1
```

**During the test:**
- Put hand within 20cm of LiDAR → Motors STOP
- Remove hand → Motors RESUME

### Verify Topics

```bash
# Check topics are active
ros2 topic list | grep -E "(cmd_vel|scan)"

# Check scan data
ros2 topic hz /scan

# Check velocity commands
ros2 topic echo /cmd_vel --once
```

---

## Custom Threshold

### Runtime (temporary)

```bash
ros2 run base_controller obstacle_avoidance_node.py --ros-args -p obstacle_threshold:=0.30
```

### Permanent Change

Edit `src/mobile_base/base_controller/scripts/obstacle_avoidance_node.py`:
```python
self.declare_parameter('obstacle_threshold', 0.20)  # Change this value
```

Then rebuild:
```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
colcon build --packages-select base_controller --symlink-install
```

---

## Troubleshooting

### Motors Not Responding

1. Check Arduino connection:
   ```bash
   ssh pi@192.168.1.7 "ls /dev/ttyACM*"
   ```

2. Test Arduino directly:
   ```bash
   ssh pi@192.168.1.7 "stty -F /dev/ttyACM0 115200 && echo 'STATUS' > /dev/ttyACM0"
   ```

### Obstacle Always Detected

1. Check LiDAR scan:
   ```bash
   ros2 topic echo /scan --once | grep ranges | head -1
   ```

2. Increase threshold:
   ```bash
   ros2 run base_controller obstacle_avoidance_node.py --ros-args -p obstacle_threshold:=0.50
   ```

### No /scan Topic

1. Check LiDAR USB:
   ```bash
   ssh pi@192.168.1.7 "ls /dev/ttyUSB*"
   ```

2. Restart LiDAR driver on RPi

---

## File Locations

| Component | Path |
|-----------|------|
| Obstacle Node | `src/mobile_base/base_controller/scripts/obstacle_avoidance_node.py` |
| Base Controller | `src/mobile_base/base_controller/scripts/base_hardware_bridge.py` |
| Arduino Firmware | `arduino/mobile_manipulator_unified_obstacle/` |
| LiDAR Config | `src/ydlidar_ros2_driver/params/X2L.yaml` |

---

## Arduino Protocol

The Arduino firmware (`mobile_manipulator_unified_obstacle.ino`) supports:

| Command | Description |
|---------|-------------|
| `VEL,<linear>,<angular>` | Set velocity (m/s, rad/s) |
| `OBS,<0\|1>` | Set obstacle flag (blocks forward if 1) |
| `STOP` | Emergency stop |
| `ARM,<a0>,...,<a5>` | Set arm servo angles |

---

## Session Date: 2026-01-03
