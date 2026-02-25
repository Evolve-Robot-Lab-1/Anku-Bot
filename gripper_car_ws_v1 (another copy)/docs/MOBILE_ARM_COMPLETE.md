# Mobile Manipulator - Complete Setup Guide

## System Overview

A 4-wheel mecanum mobile robot with:
- **5-DOF Robotic Arm** with gripper (PCA9685 servo driver)
- **YDLiDAR** for obstacle detection
- **Raspberry Pi Camera** for live video feed
- **Web Control Panel** for browser-based control
- **Obstacle Avoidance** for autonomous navigation

---

## Hardware Configuration

### Components
| Component | Connection | Description |
|-----------|------------|-------------|
| Raspberry Pi 4 | - | Main controller |
| Arduino Mega 2560 | `/dev/ttyACM0` | Motor & servo controller |
| YDLiDAR G2 | `/dev/ttyUSB0` | 360° laser scanner |
| RPi Camera | CSI connector | Video feed |
| PCA9685 | I2C (0x40) | 16-ch PWM servo driver |
| 4x Mecanum Motors | Arduino | Mobile base |
| 5x Servos | PCA9685 Ch 0-4 | Arm joints |

### Physical Layout
```
        [ARM]
          |
    [RPi Camera]
          |
  +-------+-------+
  |               |
  |   ROBOT TOP   |
  |               |
  +-------+-------+
          |
       [LiDAR]    <-- Forward direction for obstacle detection
```

**Note:** LiDAR side is configured as "forward" for obstacle avoidance.

### Servo Mapping (PCA9685)
| Joint | Channel | Range |
|-------|---------|-------|
| Shoulder (joint_1) | Ch 0 | 0-180° |
| Elbow (joint_2) | Ch 1 | 0-180° |
| Wrist (joint_3) | Ch 2 | 0-180° |
| Base Rotation (joint_4) | Ch 3 | 0-180° |
| Gripper | Ch 4 | 0-180° |

---

## Network Configuration

| Device | IP Address | Hostname |
|--------|------------|----------|
| Raspberry Pi | 192.168.1.27 | pi |
| Local PC | 192.168.1.x | - |

**Required Ports:**
| Port | Service |
|------|---------|
| 5000 | Web Control Panel (Flask) |
| 8080 | Video Stream Server |
| 9090 | ROSBridge WebSocket |

---

## Prerequisites

### Hardware Checklist
- [ ] 12V power supply connected and ON
- [ ] Arduino connected to RPi via USB (`/dev/ttyACM0`)
- [ ] LiDAR connected to RPi via USB (`/dev/ttyUSB0`)
- [ ] Camera connected to RPi CSI port
- [ ] All servos powered (external 5-6V supply)
- [ ] RPi and PC on same network

### Software Requirements
- ROS2 Humble on both RPi and PC
- Python 3.10+
- Required ROS2 packages: `rosbridge_server`, `web_video_server`, `camera_ros`, `ydlidar_ros2_driver`

---

## AUTO-START (Default - No Commands Needed!)

Robot starts automatically when RPi powers on.

### Normal Usage:
1. **Power ON** RPi
2. **Wait ~40 seconds** (boot + service startup)
3. **Open browser:** http://192.168.1.27:5000
4. **Drive!**

### What Starts Automatically:
- Serial Bridge (motor + arm control)
- LiDAR driver
- Camera (BGR888 format)
- Video server
- Obstacle avoidance
- Web control panel (rosbridge + Flask)

---

## SERVICE COMMANDS (Only if needed)

| Action | Command (SSH to RPi) |
|--------|----------------------|
| Start | `sudo systemctl start robot` |
| Stop | `sudo systemctl stop robot` |
| Restart | `sudo systemctl restart robot` |
| Status | `sudo systemctl status robot` |
| View logs | `journalctl -u robot -f` |

### Reboot RPi:
```bash
ssh pi@192.168.1.27
sudo reboot
```

---

## WEB PANEL URL

```
http://192.168.1.27:5000
```

---

## MANUAL STARTUP (Step by Step)

### Step 1: Start Core Services on RPi

SSH into RPi and run each in separate terminals (or use tmux/screen):

#### 1.1 Serial Bridge (Motor + Arm Control)
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller serial_bridge_node.py
```

#### 1.2 ROSBridge WebSocket
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

#### 1.3 Web Control Panel
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run web_control_panel web_server
```

#### 1.4 LiDAR Driver
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

#### 1.5 Camera
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run camera_ros camera_node --ros-args -r image_raw:=/camera/image_raw
```

#### 1.6 Video Server
```bash
ssh pi@192.168.1.27
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run web_video_server web_video_server --ros-args -p port:=8080
```

### Step 2: Start Obstacle Avoidance on Local PC

```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
source install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller obstacle_avoidance_node.py
```

### Step 3: Open Web Control Panel

Open browser and navigate to:
```
http://192.168.1.27:5000
```

---

## Web Control Panel Features

### Control Modes
| Mode | Description |
|------|-------------|
| **Manual** | Direct control, NO obstacle avoidance |
| **Auto** | Control WITH obstacle avoidance enabled |
| **Nav2** | Autonomous navigation (future) |

### Movement Controls
| Control | Action |
|---------|--------|
| Cruise Forward | Continuous forward motion |
| Stop Cruise | Stop cruise mode |
| D-Pad | Manual directional control |
| STOP | Immediate stop |
| E-STOP | Emergency stop (toggle) |
| Speed Slider | Adjust speed (0.1-1.0 m/s) |

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| `W` | Forward |
| `S` | Backward |
| `A` | Turn Left |
| `D` | Turn Right |
| `Space` | Stop |
| `Esc` | E-Stop |

### Arm Control
| Button | Description |
|--------|-------------|
| Home | Arm folded/compact position |
| Search | Arm up and forward to look around |
| Pick | Arm extended down to grab (base at 0°) |
| Carry | Base rotated 45°, arm raised (for carrying) |
| Place | Base at 45°, arm down to place level |
| Open/Close | Gripper control (stays at current arm position) |
| Joint Sliders | Manual joint control |

### Arm Workflow (Pick and Place)
```
1. Home      → Starting position
2. Pick      → Arm extends down (gripper open)
3. Close     → Gripper closes to grab object
4. Carry     → Arm moves to carry position
5. Place     → Arm moves to place position
6. Open      → Gripper opens to release object
7. Home      → Return to starting position
```

**Note:** Gripper Open/Close buttons only move the gripper - arm stays at current position.

### Arm Preset Values (Radians)
| Preset | J1 (Shoulder) | J2 (Elbow) | J3 (Wrist) | J4 (Base) | Gripper |
|--------|---------------|------------|------------|-----------|---------|
| Home   | 0 | 0.524 | 1.57 | -1.57 | Closed |
| Search | 0 | 0 | 0 | 0 | Closed |
| Pick   | 0 | -0.785 | 0.785 | 0 | Open |
| Carry  | 1.57 | -0.89 | -0.052 | -0.855 | Closed |
| Place  | -1.34 | -1.20 | -0.087 | -1.33 | Open |

---

## ROS2 Topics Reference

### Motor Control
| Topic | Type | Description |
|-------|------|-------------|
| `/cmd_vel` | geometry_msgs/Twist | Final velocity commands |
| `/cmd_vel_raw` | geometry_msgs/Twist | Raw velocity (before obstacle avoidance) |

### Arm Control
| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_commands` | sensor_msgs/JointState | Send joint positions |
| `/arm/joint_states` | sensor_msgs/JointState | Read joint positions |

### Sensors
| Topic | Type | Description |
|-------|------|-------------|
| `/scan` | sensor_msgs/LaserScan | LiDAR data |
| `/camera/image_raw` | sensor_msgs/Image | Camera feed |

### Status
| Topic | Type | Description |
|-------|------|-------------|
| `/obstacle_avoidance/status` | std_msgs/String | Obstacle avoidance state |

---

## Obstacle Avoidance Behavior

When in **Auto** mode:

1. **DRIVING** - Normal forward motion
2. **OBSTACLE_DETECTED** - Obstacle within 15-20cm, stops
3. **REVERSING** - Backs up ~12cm
4. **SCANNING** - Scans left and right
5. **TURNING** - Turns toward clearer direction
6. **CHECK_CLEAR** - Verifies path is clear
7. Returns to **DRIVING**

---

## Verify System Status

### Check All Nodes Running
```bash
ros2 node list
```

**Expected nodes:**
```
/serial_bridge
/rosbridge_websocket
/web_server
/ydlidar_ros2_driver_node
/obstacle_avoidance
/camera
/web_video_server
```

### Check Topics
```bash
ros2 topic list
```

### Check LiDAR Rate
```bash
ros2 topic hz /scan
```
Expected: ~9-10 Hz

### Check Camera
```bash
ros2 topic hz /camera/image_raw
```

---

## Troubleshooting

### "ROS: Disconnected" in Browser
- Check ROSBridge is running: `ros2 node list | grep rosbridge`
- Verify RPi IP: `ping 192.168.1.27`
- Check port 9090 is open

### Camera Feed Not Showing
- Verify camera node: `ros2 topic list | grep camera`
- Check web_video_server: `curl http://192.168.1.27:8080`
- Check camera log: `ssh pi@192.168.1.27 "cat /tmp/camera.log"`

### Motors Not Moving
- Check 12V power supply is ON
- Verify serial_bridge: `ros2 node list | grep serial`
- Check Arduino connection: `ls /dev/ttyACM0`
- Test cmd_vel: `ros2 topic echo /cmd_vel`

### Arm Not Responding
- Check serial_bridge is running
- Verify servo power supply (5-6V)
- Test direct command:
  ```bash
  ros2 topic pub --once /arm/joint_commands sensor_msgs/msg/JointState \
    "{name: ['joint_1'], position: [0.5]}"
  ```

### LiDAR Not Working
- Check connection: `ls /dev/ttyUSB0`
- Set permissions: `sudo chmod 666 /dev/ttyUSB0`
- Verify driver: `ros2 node list | grep lidar`
- Check scan data: `ros2 topic echo /scan --once`

### Obstacle Avoidance Not Working
- Ensure **Auto** mode selected in panel
- Verify obstacle_avoidance node is running
- Check LiDAR data: `ros2 topic hz /scan`
- Monitor status: `ros2 topic echo /obstacle_avoidance/status`

### Serial Port Permission Denied
```bash
sudo chmod 666 /dev/ttyACM0
sudo chmod 666 /dev/ttyUSB0
# Or add user to dialout group:
sudo usermod -a -G dialout pi
```

### Duplicate Nodes / Stale Processes
```bash
# Kill all ROS2 processes on RPi
ssh pi@192.168.1.27 "pkill -9 -f ros2; pkill -9 -f python3"
# Then restart services
```

---

## File Locations

### On RPi (`/home/pi/pi_ros_ws/gripper_car_ws/`)
| File | Path |
|------|------|
| Serial Bridge | `src/base_controller/scripts/serial_bridge_node.py` |
| Web Server | `src/web_control_panel/web_control_panel/web_server.py` |
| HTML Template | `src/web_control_panel/templates/index.html` |
| JavaScript | `src/web_control_panel/static/js/control.js` |
| CSS | `src/web_control_panel/static/css/style.css` |

### On Local PC
| File | Path |
|------|------|
| Obstacle Avoidance | `src/mobile_base/base_controller/scripts/obstacle_avoidance_node.py` |
| Arduino Firmware | `arduino/mobile_manipulator_unified/mobile_manipulator_unified.ino` |
| Documentation | `docs/` |

---

## Arduino Serial Protocol

### Motor Commands
```
# Velocity command (linear_x, linear_y, angular_z)
VEL,0.5,0.0,0.0

# Stop all motors
STOP
```

### Arm Commands
```
# Set servo angles (0-180 degrees)
ARM,<s0>,<s1>,<s2>,<s3>,<s4>,<s5>

# Example: Home position (all 90°)
ARM,90,90,90,90,90,90

# Stop arm
STOP_ARM
```

---

## Quick Copy-Paste Commands

### Start Everything on RPi (Background)
```bash
ssh pi@192.168.1.27 '
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# Start all services in background
nohup ros2 run base_controller serial_bridge_node.py > /tmp/serial.log 2>&1 &
sleep 2
nohup ros2 launch rosbridge_server rosbridge_websocket_launch.xml > /tmp/rosbridge.log 2>&1 &
nohup ros2 run web_control_panel web_server > /tmp/web.log 2>&1 &
nohup ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true > /tmp/lidar.log 2>&1 &
sleep 2
nohup ros2 run camera_ros camera_node --ros-args -r image_raw:=/camera/image_raw > /tmp/camera.log 2>&1 &
nohup ros2 run web_video_server web_video_server --ros-args -p port:=8080 > /tmp/video.log 2>&1 &
echo "All services started"
'
```

### Stop Everything on RPi
```bash
ssh pi@192.168.1.27 "pkill -9 -f ros2; pkill -9 -f python3; echo 'All stopped'"
```

### Check Logs on RPi
```bash
ssh pi@192.168.1.27 "tail -20 /tmp/serial.log"
ssh pi@192.168.1.27 "tail -20 /tmp/camera.log"
ssh pi@192.168.1.27 "tail -20 /tmp/video.log"
```

---

## Version History

| Date | Changes |
|------|---------|
| 2026-01-09 | Initial complete documentation |
| 2026-01-09 | Camera format fix (NV21 → BGR888 remap) |
| 2026-01-09 | Motor direction flipped (LiDAR side = forward) |
| 2026-01-09 | Arm presets updated for pick-carry-place workflow |
| 2026-01-09 | Gripper fix: Open/Close no longer moves arm position |
| 2026-01-09 | User-defined Carry/Place positions calibrated via sliders |
| 2026-01-09 | Auto-start service installed (systemd) |
| 2026-01-09 | Camera BGR888 format fix for web streaming |

---

*Document created: January 9, 2026*
*Last updated: January 9, 2026*
