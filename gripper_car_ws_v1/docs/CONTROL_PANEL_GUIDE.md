# Web Control Panel - Quick Start Guide

## Overview

The Web Control Panel provides a browser-based interface to control the mobile manipulator robot with:
- **Camera Feed** - Live video stream
- **Movement Controls** - Manual, Auto (with obstacle avoidance), Nav2
- **Cruise Control** - Continuous forward drive for testing obstacle avoidance
- **Arm Control** - Preset poses and gripper
- **Status Display** - Obstacle detection, velocities, navigation state

---

## ONE-COMMAND STARTUP (Easiest)

### On RPi:
```bash
ssh pi@192.168.1.27
~/start_robot_full.sh
```

### Then open browser:
```
http://192.168.1.27:5000
```

**That's it!** All nodes start automatically:
- LiDAR driver
- Base hardware bridge
- Obstacle avoidance
- Web control panel

---

## Prerequisites

- RPi and Local PC on same network
- Motor power supply (12V) ON
- LiDAR and Arduino connected to RPi
- Camera connected to RPi (optional, for video feed)

---

## Step 1: Start RPi Nodes (SSH into RPi)

### Terminal 1 - LiDAR:
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

### Terminal 2 - Base Hardware Bridge:
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller base_hardware_bridge.py
```

### Terminal 3 - Web Control Panel:
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 launch web_control_panel web_panel.launch.py
```

---

## Step 2: Start Obstacle Avoidance (on Local PC)

```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
source install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller obstacle_avoidance_node.py
```

---

## Step 3: Open Control Panel in Browser

Open your web browser and go to:

```
http://192.168.1.27:5000
```

You should see "ROS: Connected" in green at the top right.

---

## Control Panel Features

### Control Modes

| Mode | Button | Behavior |
|------|--------|----------|
| **Manual** | Manual | Direct control, NO obstacle avoidance |
| **Auto** | Auto | Control WITH obstacle avoidance enabled |
| **Nav2** | Nav2 | Autonomous navigation (click on map) |

### Movement Controls

| Control | Action |
|---------|--------|
| **▲ Cruise Forward** | Click once to start continuous forward motion |
| **■ Stop Cruise** | Click to stop cruise |
| **D-Pad ▲▼◄►** | Hold to move (releases = stop) |
| **STOP** | Immediate stop |
| **E-STOP** | Emergency stop (toggle) |
| **Speed Slider** | Adjust speed (0.1 - 1.0 m/s) |

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `W` | Forward |
| `S` | Backward |
| `A` | Turn Left |
| `D` | Turn Right |
| `Space` | Stop |
| `Esc` | E-Stop Toggle |

### Arm Control

| Button | Position |
|--------|----------|
| Home | Neutral position |
| Search | Looking pose |
| Pick | Picking pose |
| Place | Placing pose |
| Carry | Carrying pose |
| Open/Close | Gripper control |

### Status Display

| Status | Description |
|--------|-------------|
| **Mode** | Current control mode |
| **Arm** | Arm state |
| **Obstacle** | Clear / OBSTACLE! / REVERSING / SCANNING / TURNING |
| **Nav** | Navigation state |
| **Linear Vel** | Current linear velocity (m/s) |
| **Angular Vel** | Current angular velocity (rad/s) |

---

## Testing Obstacle Avoidance

1. Select **Auto** mode
2. Click **▲ Cruise Forward**
3. Robot starts moving forward continuously
4. Place hand ~15cm in front of LiDAR
5. Robot should:
   - Stop immediately
   - Reverse ~12cm
   - Scan left and right
   - Turn toward clearer direction
   - Resume forward motion
6. Click **■ Stop Cruise** to stop

---

## Verify All Nodes Running

```bash
ros2 node list
```

**Expected nodes:**
```
/base_hardware_bridge
/obstacle_avoidance
/ydlidar_ros2_driver_node
/web_server
/rosbridge_websocket
/web_video_server
```

---

## Troubleshooting

### "ROS: Disconnected" in browser
- Check rosbridge is running: `ros2 node list | grep rosbridge`
- Check RPi IP is correct (192.168.1.27)
- Check firewall allows port 9090

### Camera feed not showing
- Check web_video_server is running
- Check camera is connected: `ls /dev/video*`
- Check camera topic: `ros2 topic list | grep camera`

### Motors not responding
- Check base_hardware_bridge is running
- Check Arduino connection: `ls /dev/ttyACM0`
- Check 12V power supply is ON

### Obstacle avoidance not working
- Make sure **Auto** mode is selected (not Manual)
- Check obstacle_avoidance_node is running
- Check LiDAR is publishing: `ros2 topic hz /scan`

---

## Network Ports

| Port | Service | Protocol |
|------|---------|----------|
| 5000 | Web Panel (Flask) | HTTP |
| 9090 | ROSBridge | WebSocket |
| 8080 | Video Server | HTTP/MJPEG |

---

## Quick Reference - All Commands

```bash
# === RPi Terminal 1 - LiDAR ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true

# === RPi Terminal 2 - Bridge ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run base_controller base_hardware_bridge.py

# === RPi Terminal 3 - Web Panel ===
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash && source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 launch web_control_panel web_panel.launch.py

# === Local PC - Obstacle Avoidance ===
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash && source install/setup.bash
export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run base_controller obstacle_avoidance_node.py

# === Browser ===
# Open: http://192.168.1.27:5000
```

---

## File Locations

| File | Location (RPi) |
|------|----------------|
| Web Server | `/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/web_control_panel/web_server.py` |
| HTML Template | `/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/templates/index.html` |
| JavaScript | `/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/static/js/control.js` |
| CSS | `/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/static/css/style.css` |
| Launch File | `/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/launch/web_panel.launch.py` |

---

*Document created: January 2026*
