# Obstacle Avoidance - Quick Start Guide

## Prerequisites
- RPi and Local PC on same network
- Motor power supply (12V) ON
- LiDAR and Arduino connected to RPi

---

## Step 1: Start RPi Nodes

### Option A: Using Script
```bash
ssh pi@192.168.1.27
~/start_rpi_base.sh
```

### Option B: Manual (2 SSH terminals)

**Terminal 1 - LiDAR:**
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p lidar_type:=1 \
  -p isSingleChannel:=true \
  -p support_motor_dtr:=true
```

**Terminal 2 - Bridge:**
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller base_hardware_bridge.py
```

---

## Step 2: Start Local PC Nodes

### Option A: Using Script
```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
./scripts/start_obstacle_avoidance.sh
```

### Option B: Manual (2 terminals)

**Terminal 1 - Obstacle Avoidance:**
```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
source install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller obstacle_avoidance_node.py
```

**Terminal 2 - Teleop:**
```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/cmd_vel_raw
```

---

## Step 3: Verify All Nodes Running

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 node list
```

**Expected output:**
```
/base_hardware_bridge
/obstacle_avoidance
/teleop_twist_keyboard
/ydlidar_ros2_driver_node
```

---

## Step 4: Drive!

In the teleop terminal, use these keys:

```
   u    i    o
   j    k    l
   m    ,    .
```

| Key | Action |
|-----|--------|
| `i` | Forward |
| `k` | Stop |
| `j` | Turn Left |
| `l` | Turn Right |
| `u` | Forward + Left |
| `o` | Forward + Right |
| `,` | Backward |
| `q/z` | Increase/Decrease speed |

---

## Monitor Obstacle Status

```bash
ros2 topic echo /obstacle_avoidance/status
```

---

## Environment Variables Reference

| Variable | Value | Purpose |
|----------|-------|---------|
| `ROS_DOMAIN_ID` | `0` | Must match on RPi and Local PC |
| `RMW_IMPLEMENTATION` | `rmw_fastrtps_cpp` | ROS2 DDS middleware |

---

## Troubleshooting

### Nodes not seeing each other?
- Check both machines have same `ROS_DOMAIN_ID`
- Check network: `ping 192.168.1.27`

### Permission denied on serial port?
```bash
sudo chmod 666 /dev/ttyUSB0
sudo chmod 666 /dev/ttyACM0
```

### Motors not moving?
- Check 12V power supply is ON
- Test Arduino: `echo "STATUS" > /dev/ttyACM0`

---

## Quick Reference - All Commands

```bash
# === RPi Terminal 1 ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true

# === RPi Terminal 2 ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run base_controller base_hardware_bridge.py

# === Local PC Terminal 1 ===
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash && source install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run base_controller obstacle_avoidance_node.py

# === Local PC Terminal 2 ===
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=/cmd_vel_raw
```
