# Mobile Manipulator Connection Guide

## System Overview

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              LOCAL PC (192.168.1.14)                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │    RViz      │  │ Obstacle     │  │   Teleop     │  │    SLAM      │   │
│  │              │  │ Avoidance    │  │  Keyboard    │  │  Toolbox     │   │
│  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘   │
└────────────────────────────────────────────────────────────────────────────┘
                                    │
                         Network (192.168.1.x)
                                    │
┌────────────────────────────────────────────────────────────────────────────┐
│                         RASPBERRY PI 4 (192.168.1.7)                        │
│  ┌──────────────┐  ┌──────────────┐                                        │
│  │ YDLiDAR      │  │ Base Hardware│                                        │
│  │ Driver       │  │ Bridge       │                                        │
│  │ /dev/ttyUSB0 │  │ /dev/ttyACM0 │                                        │
│  └──────┬───────┘  └──────┬───────┘                                        │
└─────────┼─────────────────┼────────────────────────────────────────────────┘
          │                 │
     ┌────┴────┐       ┌────┴────────────────────────────┐
     │ YDLiDAR │       │        Arduino Mega 2560        │
     │   X2L   │       │  ┌─────────────┬──────────────┐ │
     └─────────┘       │  │ Base Motors │  6-DOF Arm   │ │
                       │  │ (4x L298N)  │  (PCA9685)   │ │
                       │  └─────────────┴──────────────┘ │
                       └─────────────────────────────────┘
```

---

## Hardware Connections

### RPi USB Ports

| Device | Port | Baudrate | Description |
|--------|------|----------|-------------|
| YDLiDAR X2L | `/dev/ttyUSB0` | 115200 | CP210x USB-Serial |
| Arduino Mega | `/dev/ttyACM0` | 115200 | USB CDC |

### Arduino Pin Mapping

**Motor Drivers (2x L298N):**
| Motor | IN1 | IN2 | EN (PWM) |
|-------|-----|-----|----------|
| Front Left | 22 | 23 | 12 |
| Front Right | 24 | 25 | 11 |
| Back Left | 26 | 27 | 10 |
| Back Right | 28 | 29 | 9 |

**Encoders:**
| Motor | Channel A | Channel B |
|-------|-----------|-----------|
| Front Left | 18 (INT) | 31 |
| Front Right | 19 (INT) | 33 |
| Back Left | 2 (INT) | 35 |
| Back Right | 3 (INT) | 37 |

**Arm Servos (PCA9685 I2C 0x40):**
| Joint | Channel | Default Angle |
|-------|---------|---------------|
| Base | 0 | 90° |
| Shoulder | 1 | 90° |
| Elbow | 2 | 90° |
| Wrist Pitch | 4 | 90° |
| Wrist Roll | 5 | 90° |
| Gripper | 6 | 90° |

---

## Network Configuration

### IP Addresses
| Device | IP | User |
|--------|-----|------|
| Local PC | 192.168.1.14 | bhuvanesh |
| Raspberry Pi | 192.168.1.7 | pi |

### SSH Access
```bash
ssh pi@192.168.1.7
```

### ROS2 Domain
Both machines must be on same ROS_DOMAIN_ID (default: 0)

---

## ROS2 Distributed System (How RPi ↔ Local PC Connect)

### Automatic Discovery

ROS2 uses **DDS (Data Distribution Service)** with multicast UDP for automatic node discovery. No manual IP configuration needed!

```
┌─────────────────┐                    ┌─────────────────┐
│   Local PC      │◄── Same Network ──►│      RPi        │
│  192.168.1.14   │   (192.168.1.x)    │  192.168.1.7    │
├─────────────────┤                    ├─────────────────┤
│ obstacle_avoid  │◄─── /scan ─────────│ ydlidar_driver  │
│ teleop_keyboard │                    │                 │
│                 │──── /cmd_vel ─────►│ base_hardware   │
│ rviz2           │◄─── /odom ─────────│    _bridge      │
└─────────────────┘                    └─────────────────┘
        │                                      │
        └──────── Auto-discover via ───────────┘
                  DDS multicast UDP
```

### Requirements for ROS2 Network

1. **Same subnet** - Both on 192.168.1.x
2. **Same ROS_DOMAIN_ID** - Default is 0
3. **Firewall open** - UDP multicast allowed

### Verify Connection

```bash
# On Local PC - see all nodes (including RPi nodes)
ros2 node list

# Expected output:
#   /ydlidar_ros2_driver_node    ← Running on RPi
#   /base_hardware_bridge        ← Running on RPi
#   /obstacle_avoidance          ← Running on Local PC

# See all topics
ros2 topic list

# Check topic publishers
ros2 topic info /scan
# Should show publisher on RPi
```

### If Discovery Fails

```bash
# 1. Check same domain ID
echo $ROS_DOMAIN_ID          # Should match on both (empty = 0)

# 2. Check network connectivity
ping 192.168.1.7             # Ping RPi from Local PC

# 3. Check firewall
sudo ufw status              # Should be disabled or allow UDP

# 4. Force same DDS implementation
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
```

### Environment Variables (Optional)

Add to `~/.bashrc` on both machines if needed:
```bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
```

---

## Arduino Unified Protocol

**Firmware:** `mobile_manipulator_unified_obstacle.ino`
**Baudrate:** 115200

### Commands (ROS → Arduino)

| Command | Format | Example | Description |
|---------|--------|---------|-------------|
| Velocity | `VEL,<linear>,<angular>` | `VEL,0.15,0.0` | Base velocity (m/s, rad/s) |
| Arm | `ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>` | `ARM,90,45,90,90,90,90` | Set all servo angles |
| Single Servo | `SERVO,<idx>,<angle>` | `SERVO,1,45` | Set one servo |
| Obstacle | `OBS,<0\|1>` | `OBS,1` | Set obstacle flag |
| Stop All | `STOP` | `STOP` | Emergency stop |
| Stop Arm | `STOP_ARM` | `STOP_ARM` | Stop arm only |
| Stop Base | `STOP_BASE` | `STOP_BASE` | Stop base only |
| PID Tune | `PID,<kp>,<ki>,<kd>` | `PID,2.0,0.5,0.1` | Update gains |
| Reset Enc | `RST` | `RST` | Reset encoders |
| Status | `STATUS` | `STATUS` | Get status |
| Unlock | `UNLOCK,ALL` | `UNLOCK,ALL` | Unlock motors |

### Responses (Arduino → ROS)

| Response | Format | Description |
|----------|--------|-------------|
| Encoder | `ENC,<fl>,<fr>,<bl>,<br>` | Encoder ticks @ 50Hz |
| Arm Position | `ARM_POS,<a0>,...,<a5>` | Current servo angles |
| Heartbeat | `HEARTBEAT,<ms>,OBS:<0\|1>` | Every 1 second |
| Status | `STATUS,<message>` | Status info |
| OK | `OK,<command>` | Command acknowledged |
| Error | `ERROR,<message>` | Error message |

---

## Robot Parameters

| Parameter | Value | Unit |
|-----------|-------|------|
| Wheel Radius | 0.075 | m |
| Wheel Base | 0.35 | m |
| Encoder CPR | 360 | ticks/rev |
| Max Linear Vel | ~0.5 | m/s |
| Max Angular Vel | ~2.0 | rad/s |
| Command Timeout | 2000 | ms |

---

## Establishing Connection

### Step 1: Connect to RPi

```bash
# From Local PC
ssh pi@192.168.1.7
```

### Step 2: Verify Hardware

```bash
# On RPi - Check USB devices
ls -la /dev/ttyUSB* /dev/ttyACM*

# Expected:
# /dev/ttyUSB0 - LiDAR
# /dev/ttyACM0 - Arduino
```

### Step 3: Test Arduino Connection

```bash
# On RPi
stty -F /dev/ttyACM0 115200 raw -echo -hupcl
echo "STATUS" > /dev/ttyACM0
timeout 3 cat /dev/ttyACM0
```

**Expected output:**
```
STATUS,Unified Controller + Obstacle Avoidance Ready
HEARTBEAT,xxxxx,OBS:0
```

### Step 4: Test LiDAR Connection

```bash
# On RPi
ls -la /dev/ttyUSB0
# Should show CP210x device
```

---

## Full System Launch

### Terminal 1 - LiDAR (RPi)
```bash
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 \
  -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true
```

### Terminal 2 - Base Controller (RPi)
```bash
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run base_controller base_hardware_bridge.py
```

### Terminal 3 - Obstacle Avoidance (Local)
```bash
source /opt/ros/humble/setup.bash
source /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/install/setup.bash
ros2 run base_controller obstacle_avoidance_node.py
```

### Terminal 4 - Teleop (Local)
```bash
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=/cmd_vel_raw
```

---

## Arm Control

### Direct Serial (Testing)
```bash
# On RPi
stty -F /dev/ttyACM0 115200 raw -echo -hupcl
echo "ARM,90,45,90,90,90,90" > /dev/ttyACM0  # Move shoulder to 45°
echo "ARM,90,90,90,90,90,90" > /dev/ttyACM0  # Return to home
```

### ROS2 (via hardware bridge)
```bash
# Publish arm command
ros2 topic pub /arm/joint_commands std_msgs/Float64MultiArray \
  "{data: [90, 45, 90, 90, 90, 90]}" -1
```

### Arm Joint Limits
| Joint | Min | Max | Home |
|-------|-----|-----|------|
| Base | 0° | 180° | 90° |
| Shoulder | 0° | 180° | 90° |
| Elbow | 0° | 180° | 90° |
| Wrist Pitch | 0° | 180° | 90° |
| Wrist Roll | 0° | 180° | 90° |
| Gripper | 0° | 180° | 90° |

---

## Quick Tests

### Test Base Motors
```bash
# On RPi via serial
echo "VEL,0.1,0" > /dev/ttyACM0   # Forward
sleep 2
echo "STOP" > /dev/ttyACM0        # Stop
```

### Test Arm
```bash
# On RPi via serial
echo "ARM,90,60,90,90,90,90" > /dev/ttyACM0  # Move shoulder
sleep 2
echo "ARM,90,90,90,90,90,90" > /dev/ttyACM0  # Home
```

### Test Obstacle Avoidance
```bash
# On Local PC
ros2 topic pub /cmd_vel_raw geometry_msgs/Twist "{linear: {x: 0.15}}" -r 5 &
# Put hand within 20cm of LiDAR - motors should stop
# Remove hand - motors resume
kill %1
```

---

## Workspaces

| Location | Path |
|----------|------|
| Local PC | `/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1` |
| RPi | `/home/pi/pi_ros_ws/gripper_car_ws` |
| Arduino Code | `arduino/mobile_manipulator_unified_obstacle/` |

---

## Troubleshooting

### Arduino Not Found
```bash
# Check USB
lsusb | grep Arduino
ls /dev/ttyACM*

# Fix permissions
sudo chmod 666 /dev/ttyACM0
sudo usermod -a -G dialout $USER
```

### LiDAR Not Found
```bash
# Check USB
lsusb | grep CP210
ls /dev/ttyUSB*

# Fix permissions
sudo chmod 666 /dev/ttyUSB0
```

### Motors Not Moving
1. Check Arduino connection
2. Verify `base_hardware_bridge` is running
3. Check `/cmd_vel` topic is publishing
4. Ensure motors are not locked (send `UNLOCK,ALL`)

### Arm Not Moving
1. Check PCA9685 I2C connection
2. Verify 5V power to servo board
3. Send `STATUS` to check arm angles

---

## Session Date: 2026-01-03
