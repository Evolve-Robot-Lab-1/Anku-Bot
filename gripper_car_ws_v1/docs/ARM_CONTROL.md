# Arm Control - Quick Reference Guide

## Overview
5-DOF robotic arm controlled via ROS2 through serial bridge to Arduino Mega with PCA9685 servo driver.

## Hardware
- Arduino Mega 2560
- PCA9685 16-channel PWM servo driver (I2C: 0x40)
- 5x Servo motors on channels 0-4

### Servo Mapping
| Joint | PCA Channel | Description |
|-------|-------------|-------------|
| joint_1 | Ch 0 | Shoulder |
| joint_2 | Ch 1 | Elbow |
| joint_3 | Ch 2 | Wrist |
| joint_4 | Ch 3 | Base rotation |
| gripper_base_joint | Ch 4 | Gripper |

---

## Prerequisites
- RPi and Local PC on same network
- Serial bridge node running on RPi
- Arduino Mega connected to RPi (/dev/ttyACM0)

---

## Step 1: Start Serial Bridge on RPi

```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller serial_bridge_node.py
```

**Expected output:**
```
[INFO] Serial Bridge Node (Unified) started
[INFO]   Port: /dev/ttyACM0
[INFO]   Baud: 115200
[INFO] Connected to /dev/ttyACM0
```

---

## Step 2: Verify Node Running

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 node list
```

**Expected:**
```
/serial_bridge
```

---

## Step 3: Control Arm via ROS2 Topics

### Available Topics
| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_commands` | sensor_msgs/JointState | Send commands |
| `/arm/joint_states` | sensor_msgs/JointState | Read positions |

### Move Arm to Position
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['joint_1','joint_2','joint_3','joint_4','gripper_base_joint','left_gear_joint'], \
    position: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}"
```

### Home Position (All joints to center)
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['joint_1','joint_2','joint_3','joint_4','gripper_base_joint','left_gear_joint'], \
    position: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}"
```

### Monitor Arm Position
```bash
ros2 topic echo /arm/joint_states
```

---

## Joint Limits (Radians)

| Joint | Min | Max | Center |
|-------|-----|-----|--------|
| joint_1 | -3.14 | 3.14 | 0.0 |
| joint_2 | -1.57 | 1.57 | 0.0 |
| joint_3 | -1.57 | 1.57 | 0.0 |
| joint_4 | -1.57 | 1.57 | 0.0 |
| gripper_base_joint | -3.14 | 3.14 | 0.0 |
| left_gear_joint | 0.0 | 1.0 | 0.0 |

---

## Example Commands

### Move Shoulder (joint_1) to 45 degrees
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['joint_1'], position: [0.785]}"
```

### Open Gripper
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['left_gear_joint'], position: [0.8]}"
```

### Close Gripper
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['left_gear_joint'], position: [0.0]}"
```

### Multiple Joints
```bash
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
  "{name: ['joint_1','joint_2','joint_3'], position: [0.5, -0.3, 0.3]}"
```

---

## Direct Arduino Commands (via Serial)

If bypassing ROS2, send directly to Arduino:

```bash
# Home all servos (90 degrees each)
echo "ARM,90,90,90,90,90,90" > /dev/ttyACM0

# Move specific angles (0-180 degrees)
echo "ARM,45,90,90,90,90,90" > /dev/ttyACM0

# Stop arm
echo "STOP_ARM" > /dev/ttyACM0
```

**Arduino Serial Protocol:**
```
ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>  - Set servo angles (0-180)
STOP_ARM                           - Stop arm movement
```

---

## Troubleshooting

### Serial bridge can't connect?
```bash
# Check port exists
ls -la /dev/ttyACM*

# Check permissions
sudo chmod 666 /dev/ttyACM0

# Check if port in use
lsof /dev/ttyACM0
```

### Arm not responding?
```bash
# Test Arduino directly
echo "STATUS" > /dev/ttyACM0
cat /dev/ttyACM0
```

### Joint not moving?
- Check servo power (external 5-6V supply)
- Verify PCA9685 I2C connection
- Check channel mapping matches physical wiring

---

## Quick Reference - All Commands

```bash
# === RPi: Start Serial Bridge ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run base_controller serial_bridge_node.py

# === Local PC: Setup Environment ===
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# === Control Commands ===
# Home position
ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState "{name: ['joint_1','joint_2','joint_3','joint_4','gripper_base_joint','left_gear_joint'], position: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}"

# Monitor position
ros2 topic echo /arm/joint_states
```

---

## Integration Notes

- Serial bridge handles both base (`/cmd_vel`) and arm (`/arm/joint_commands`)
- Joint positions are in radians, converted to servo angles (0-180) internally
- Smooth motion: servos step 1 degree at a time with 30ms delay
