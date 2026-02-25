# Quick Start: Obstacle Avoidance Testing

## ⚡ Fast Path to Testing (5 minutes)

### Prerequisites Check
```bash
# Run diagnostic script
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
./test_obstacle_avoidance.sh
```

---

## 🚀 4-Terminal Setup

### Terminal 1: LiDAR (RPi)
```bash
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true
```

### Terminal 2: Base Bridge (RPi)
```bash
ssh pi@192.168.1.7
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash
ros2 run base_controller base_hardware_bridge.py
```

### Terminal 3: Obstacle Avoidance (Local)
```bash
source /opt/ros/humble/setup.bash
source /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/install/setup.bash
ros2 run base_controller obstacle_avoidance_node.py
```

### Terminal 4: Teleop (Local)
```bash
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=/cmd_vel_raw
```

---

## ✅ Verification Checklist

**In a new terminal:**
```bash
source /opt/ros/humble/setup.bash

# 1. Check nodes (should see 4)
ros2 node list

# 2. Check new topic (should exist)
ros2 topic list | grep obstacle_detected

# 3. Monitor status
ros2 topic echo /obstacle_avoidance/status
```

---

## 🧪 Simple Test

1. **Press 'i'** in teleop → Robot drives forward
2. **Place hand 15cm in front** → Robot stops automatically
3. **Watch console** → Should see:
   ```
   [WARN] [obstacle_avoidance]: OBSTACLE at 0.18m - initiating avoidance
   [INFO] [base_hardware_bridge]: Obstacle flag: 1
   ```
4. **Robot reverses, turns, resumes** → Success! ✅

---

## 🐛 Quick Fixes

**RPi not reachable?**
```bash
# Find it on network
sudo nmap -sn 192.168.1.0/24 | grep -B2 Raspberry
```

**Motors don't move?**
```bash
ssh pi@192.168.1.7
echo "UNLOCK,ALL" > /dev/ttyACM0
```

**Always detecting obstacle?**
```bash
# Increase threshold temporarily
ros2 run base_controller obstacle_avoidance_node.py --ros-args -p obstacle_threshold:=0.50
```

---

## 📊 Monitor Everything (Optional)

**5 monitoring windows:**
```bash
# Window 1: State machine
ros2 topic echo /obstacle_avoidance/status

# Window 2: Obstacle flag
ros2 topic echo /obstacle_detected

# Window 3: Velocity commands
ros2 topic echo /cmd_vel

# Window 4: Scan rate
ros2 topic hz /scan

# Window 5: Odometry
ros2 topic echo /odom
```

---

## 📖 Full Documentation

See **OBSTACLE_AVOIDANCE_FIXED.md** for complete details.
