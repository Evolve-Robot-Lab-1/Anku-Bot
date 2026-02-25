# Obstacle Avoidance System - Manual Operation Guide

## System Overview

This document describes how to manually run the obstacle avoidance system for the mobile manipulator robot.

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        LOCAL PC                                  │
│  ┌──────────────────┐    ┌──────────────────┐                   │
│  │ obstacle_        │    │ teleop_twist_    │                   │
│  │ avoidance_node   │    │ keyboard         │                   │
│  └────────┬─────────┘    └────────┬─────────┘                   │
│           │                       │                              │
│           │ /cmd_vel              │ /cmd_vel_raw                 │
│           │ /obstacle_detected    │                              │
└───────────┼───────────────────────┼──────────────────────────────┘
            │                       │
            │      ROS2 Network     │
            │     (192.168.1.x)     │
┌───────────┼───────────────────────┼──────────────────────────────┐
│           ▼                       ▼          RASPBERRY PI        │
│  ┌──────────────────┐    ┌──────────────────┐   (192.168.1.27)  │
│  │ base_hardware_   │    │ ydlidar_ros2_    │                   │
│  │ bridge           │    │ driver_node      │                   │
│  └────────┬─────────┘    └────────┬─────────┘                   │
│           │                       │                              │
│           │ Serial                │ Serial                       │
│           ▼                       ▼                              │
│     ┌──────────┐           ┌──────────┐                         │
│     │ Arduino  │           │ YDLiDAR  │                         │
│     │ Mega2560 │           │ X2L      │                         │
│     └──────────┘           └──────────┘                         │
└─────────────────────────────────────────────────────────────────┘
```

### Components

| Component | Location | Device | Purpose |
|-----------|----------|--------|---------|
| YDLiDAR X2L | RPi | /dev/ttyUSB0 | 360° laser scanner |
| Arduino Mega 2560 | RPi | /dev/ttyACM0 | Motor controller |
| Base Hardware Bridge | RPi | - | ROS-Arduino interface |
| Obstacle Avoidance Node | Local PC | - | Obstacle detection & avoidance logic |
| Teleop Keyboard | Local PC | - | Manual control |

---

## Prerequisites

### Hardware Connections

1. **YDLiDAR X2L** connected to RPi USB port (usually `/dev/ttyUSB0`)
2. **Arduino Mega 2560** connected to RPi USB port (usually `/dev/ttyACM0`)
3. **Motor power supply** (12V) connected and ON
4. **L298N motor drivers** powered and connected to Arduino

### Network Setup

- **Raspberry Pi IP:** 192.168.1.27
- **Local PC:** Same network (192.168.1.x)
- Both machines must be able to communicate via ROS2

### ROS2 Domain Setup

On both machines, ensure same ROS_DOMAIN_ID (default 0):
```bash
export ROS_DOMAIN_ID=0
```

---

## Step-by-Step Manual Launch

### Step 1: SSH into Raspberry Pi

```bash
ssh pi@192.168.1.27
```

Password: (your pi password)

---

### Step 2: Start LiDAR Driver (on RPi)

Open a terminal on RPi (via SSH):

```bash
# Source ROS2
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

# Run LiDAR driver
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p lidar_type:=1 \
  -p isSingleChannel:=true \
  -p support_motor_dtr:=true
```

**Expected output:**
```
[YDLIDAR INFO] Current SDK Version: 1.x.x
[YDLIDAR INFO] Lidar running correctly! The health status is good
[YDLIDAR INFO] Now YDLIDAR is scanning...
```

**Verify LiDAR is publishing:**
```bash
# In another terminal
ros2 topic hz /scan
# Should show ~7 Hz
```

---

### Step 3: Start Base Hardware Bridge (on RPi)

Open another terminal on RPi (via SSH):

```bash
# Source ROS2
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

# Run bridge
ros2 run base_controller base_hardware_bridge.py
```

**Expected output:**
```
[INFO] [base_hardware_bridge]: Connected to Arduino
[INFO] [base_hardware_bridge]: Base Hardware Bridge initialized
[INFO] [base_hardware_bridge]: Serial: /dev/ttyACM0 @ 115200
[INFO] [base_hardware_bridge]: Wheel: radius=0.075m, base=0.67m
```

---

### Step 4: Start Obstacle Avoidance Node (on Local PC)

Open a terminal on your local PC:

```bash
# Navigate to workspace
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1

# Source ROS2
source /opt/ros/humble/setup.bash
source install/setup.bash

# Run obstacle avoidance
ros2 run base_controller obstacle_avoidance_node.py
```

**Expected output:**
```
[INFO] [obstacle_avoidance]: Smart Turn Obstacle Avoidance initialized
  Obstacle threshold: 0.2m
  Clear threshold: 0.3m
  Front sector: +/-30 deg
  Reverse: 0.1m/s for 0.12m
```

---

### Step 5: Start Teleop Keyboard (on Local PC)

Open another terminal on your local PC:

```bash
# Source ROS2
source /opt/ros/humble/setup.bash

# Run teleop (publishes to /cmd_vel_raw, obstacle node forwards to /cmd_vel)
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/cmd_vel_raw
```

**Controls:**
```
Moving around:
   u    i    o
   j    k    l
   m    ,    .

i/k : forward/backward
j/l : rotate left/right
u/o : forward + rotate
m/. : backward + rotate
k   : stop
q/z : increase/decrease max speeds
```

---

## Verification Commands

### Check All Nodes Are Running

```bash
ros2 node list
```

**Expected:**
```
/base_hardware_bridge
/obstacle_avoidance
/teleop_twist_keyboard
/ydlidar_ros2_driver_node
```

### Check Topics

```bash
ros2 topic list
```

**Key topics:**
```
/scan                        # LiDAR data
/cmd_vel_raw                 # Raw teleop commands
/cmd_vel                     # Filtered commands (after obstacle check)
/obstacle_detected           # Boolean obstacle flag
/obstacle_avoidance/status   # Avoidance state info
/odom                        # Odometry
```

### Monitor Obstacle Status

```bash
ros2 topic echo /obstacle_avoidance/status
```

**Example output:**
```
data: state:driving,front:0.81,left:2.25,right:0.79,attempts:0
```

### Check LiDAR Data Rate

```bash
ros2 topic hz /scan
```

**Expected:** ~7 Hz

### Test Arduino Communication

```bash
# On RPi
stty -F /dev/ttyACM0 115200 raw -echo
echo "STATUS" > /dev/ttyACM0
timeout 2 cat /dev/ttyACM0
```

**Expected:**
```
STATUS,Mode:NORMAL,OBS:0,Vel:[0.00,0.00,0.00,0.00]
```

---

## Testing Procedure

### Basic Motor Test

1. Start all nodes as described above
2. In teleop terminal, press `i` to move forward
3. Wheels should spin forward
4. Press `k` to stop

### Obstacle Detection Test

1. Start all nodes
2. Monitor status: `ros2 topic echo /obstacle_avoidance/status`
3. Place hand or object ~15-20cm in front of LiDAR
4. `front:` value should drop below 0.2
5. State should change to `obstacle_detected`

### Full Avoidance Test

1. Start all nodes
2. Press `i` to drive forward
3. Place obstacle in front
4. Robot should:
   - Stop
   - Reverse ~12cm
   - Scan left and right
   - Turn toward clearer direction
   - Resume driving

---

## Configuration Parameters

### Obstacle Avoidance Node Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `obstacle_threshold` | 0.20 m | Distance to trigger obstacle detection |
| `clear_threshold` | 0.30 m | Distance to consider path clear |
| `front_sector_angle` | 30.0 deg | Half-angle of front detection sector |
| `reverse_speed` | 0.10 m/s | Speed when reversing |
| `reverse_distance` | 0.12 m | Distance to reverse |
| `turn_speed` | 0.50 rad/s | Angular velocity when turning |
| `scan_duration` | 0.40 s | Time to scan each direction |

### Modify Parameters at Runtime

```bash
ros2 param set /obstacle_avoidance obstacle_threshold 0.35
ros2 param set /obstacle_avoidance clear_threshold 0.50
```

### Arduino Firmware Parameters

Edit in `mobile_manipulator_unified_obstacle.ino`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `WHEEL_RADIUS` | 0.075 m | Wheel radius |
| `WHEEL_BASE` | 0.35 m | Distance between wheels |
| `CMD_TIMEOUT` | 2000 ms | Stop if no command received |
| `OBS_TIMEOUT` | 500 ms | Auto-clear obstacle flag |

---

## Troubleshooting

### LiDAR Not Starting

```bash
# Check device exists
ls -la /dev/ttyUSB0

# Set permissions
sudo chmod 666 /dev/ttyUSB0

# Check if another process is using it
fuser /dev/ttyUSB0
```

### Arduino Not Responding

```bash
# Check device exists
ls -la /dev/ttyACM0

# Set permissions
sudo chmod 666 /dev/ttyACM0

# Test communication
stty -F /dev/ttyACM0 115200 raw -echo
echo "STATUS" > /dev/ttyACM0
cat /dev/ttyACM0
```

### Motors Not Moving

1. **Check power supply is ON** (12V to L298N)
2. **Check Arduino firmware:**
   ```bash
   echo "STATUS" > /dev/ttyACM0
   # Should see: STATUS,Unified Controller + Obstacle Avoidance Ready
   ```
3. **Test direct motor command:**
   ```bash
   echo "VEL,0.2,0.0" > /dev/ttyACM0
   sleep 2
   echo "STOP" > /dev/ttyACM0
   ```

### Nodes Not Discovering Each Other

```bash
# Check ROS_DOMAIN_ID is same on both machines
echo $ROS_DOMAIN_ID

# Check network connectivity
ping 192.168.1.27

# Check firewall allows UDP multicast
sudo ufw allow from 192.168.1.0/24
```

### Obstacle Not Detecting

1. **Check LiDAR range:** YDLiDAR X2L minimum range is 0.12m
2. **Object must be between 0.12m and 0.20m** to trigger detection
3. **Check front sector angle** is pointing correctly
4. **Verify with:** `ros2 topic echo /obstacle_avoidance/status`

---

## File Locations

| File | Path |
|------|------|
| Arduino Firmware | `/media/bhuvanesh/.../arduino/mobile_manipulator_unified_obstacle/mobile_manipulator_unified_obstacle.ino` |
| Base Hardware Bridge | `/media/bhuvanesh/.../src/mobile_base/base_controller/scripts/base_hardware_bridge.py` |
| Obstacle Avoidance Node | `/media/bhuvanesh/.../src/mobile_base/base_controller/scripts/obstacle_avoidance_node.py` |
| This Documentation | `/media/bhuvanesh/.../docs/OBSTACLE_AVOIDANCE_MANUAL.md` |

Full workspace path: `/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1`

---

## Quick Start Script

Create this script for one-command startup:

### On RPi (`~/start_robot_base.sh`):

```bash
#!/bin/bash
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

# Start LiDAR in background
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 \
  -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true &

sleep 3

# Start bridge
ros2 run base_controller base_hardware_bridge.py
```

### On Local PC (`~/start_obstacle_avoidance.sh`):

```bash
#!/bin/bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
source install/setup.bash

# Start obstacle avoidance in background
ros2 run base_controller obstacle_avoidance_node.py &

sleep 2

# Start teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/cmd_vel_raw
```

---

## State Machine Reference

The obstacle avoidance node uses a state machine:

```
DRIVING ──(obstacle <0.2m)──► OBSTACLE_DETECTED
                                     │
                                     ▼ (200ms delay)
                                 REVERSING
                                     │
                                     ▼ (reverse 0.12m)
                                  STOPPED
                                     │
                                     ▼ (300ms delay)
                                 SCANNING
                                     │
                    ┌────────────────┴────────────────┐
                    ▼                                 ▼
            (left clearer)                    (right clearer)
                    │                                 │
                    └────────────┬───────────────────┘
                                 ▼
                             TURNING
                                 │
                                 ▼ (front >0.3m)
                            CHECK_CLEAR
                                 │
                    ┌────────────┴────────────┐
                    ▼                         ▼
              (clear)                   (not clear)
                    │                         │
                    ▼                         ▼
               DRIVING ◄─────────────── SCANNING
```

---

## Arduino Serial Protocol

### Commands (ROS → Arduino)

| Command | Format | Description |
|---------|--------|-------------|
| Velocity | `VEL,<linear_x>,<angular_z>` | Set velocity (m/s, rad/s) |
| Obstacle Flag | `OBS,<0\|1>` | Set obstacle detected flag |
| Stop | `STOP` | Emergency stop all |
| Status | `STATUS` | Request status |
| Unlock Motors | `UNLOCK,ALL` | Unlock all motors |

### Responses (Arduino → ROS)

| Response | Format | Description |
|----------|--------|-------------|
| OK | `OK,<CMD>` | Command acknowledged |
| Status | `STATUS,Mode:<mode>,OBS:<0\|1>,Vel:[...]` | Current status |
| Encoder | `ENC,<m0>,<m1>,<m2>,<m3>` | Encoder counts |
| Heartbeat | `HEARTBEAT,<time>,OBS:<0\|1>` | Periodic heartbeat |

---

*Document created: January 2026*
*Last updated: January 8, 2026*
