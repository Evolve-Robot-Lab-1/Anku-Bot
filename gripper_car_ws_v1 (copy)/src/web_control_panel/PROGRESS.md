# Web Control Panel - Progress Update

## Date: 2026-01-04

---

## Summary

Successfully developed and tested a web-based control panel for the mobile manipulator robot. The system provides remote access to camera feed and robot controls via any web browser.

---

## Completed Tasks

### 1. Camera Setup and Testing

**Pi Camera Configuration:**
- Camera: OV5647 (Pi Camera v1.3) connected via CSI
- Driver: `camera_ros` (libcamera-based) - works better than `v4l2_camera` for CSI cameras
- Resolution: 640x480 @ RGB888 format
- Frame Rate: ~15 FPS

**Camera Launch Command:**
```bash
ros2 run camera_ros camera_node --ros-args \
    -p format:=RGB888 \
    -p width:=640 \
    -p height:=480 \
    -r image_raw:=/camera/image_raw
```

**Camera Topics:**
| Topic | Type | Description |
|-------|------|-------------|
| `/camera/image_raw` | sensor_msgs/Image | Raw camera images |
| `/camera/camera_info` | sensor_msgs/CameraInfo | Camera calibration |
| `/camera/image_raw/compressed` | sensor_msgs/CompressedImage | JPEG compressed |

**Test Results:**
- Camera feed captured and displayed successfully
- Image quality verified (low light conditions noted during testing)
- Live streaming tested via `rqt_image_view`

---

### 2. Web Control Panel Development

**Package Created:** `web_control_panel`

**Location:** `src/web_control_panel/`

**Files Created:**
| File | Purpose |
|------|---------|
| `package.xml` | ROS2 package manifest |
| `setup.py` | Python package configuration |
| `setup.cfg` | Install configuration |
| `web_control_panel/__init__.py` | Module init |
| `web_control_panel/web_server.py` | Flask web server as ROS2 node |
| `templates/index.html` | Control panel UI |
| `static/css/style.css` | UI styling (dark theme) |
| `static/js/control.js` | ROS2 communication via roslibjs |
| `launch/web_panel.launch.py` | Launch file for all services |
| `config/web_params.yaml` | Configuration parameters |

**Architecture:**
```
Browser (http://192.168.1.7:5000)
    │
    ├── Flask Server (port 5000) ──── Static files (HTML/CSS/JS)
    │
    ├── rosbridge_websocket (port 9090) ──── ROS2 Topics
    │
    └── web_video_server (port 8080) ──── Camera MJPEG stream
```

---

### 3. Web UI Features Implemented

**Control Panel Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│                  Web Control Panel                          │
├──────────────┬──────────────────────────────────────────────┤
│ Camera Feed  │  Mode: [Manual] [Auto] [Nav]                 │
│ (MJPEG)      ├──────────────────────────────────────────────┤
│              │  Joystick (D-pad)     Speed Slider           │
├──────────────┤      ↑                                       │
│ Nav2 Map     │    ← ● →              [E-STOP]               │
│ (click nav)  │      ↓                                       │
├──────────────┼──────────────────────────────────────────────┤
│ Arm Control  │  Arm: [Home][Search][Pick][Place][Carry]     │
│              │  Gripper: [Open] [Close]                     │
├──────────────┼──────────────────────────────────────────────┤
│              │  Status: Mode | Arm | Obstacle | Nav         │
└──────────────┴──────────────────────────────────────────────┘
```

**Features:**
1. **Live Camera Feed** - MJPEG stream from web_video_server
2. **Movement Controls** - D-pad buttons + WASD keyboard support
3. **Speed Control** - Slider (0.1 - 1.0 m/s)
4. **Mode Selection:**
   - Manual: Direct `/cmd_vel` publishing
   - Auto: `/cmd_vel_raw` with obstacle avoidance
   - Nav2: Click-on-map navigation goals
5. **Arm Preset Poses:** Home, Search, Pick, Place, Carry
6. **Gripper Control:** Open/Close buttons
7. **E-STOP Button** - Emergency stop
8. **Status Display** - Mode, arm state, obstacles, navigation

---

### 4. Testing Carried Out

**Test 1: Camera Live Feed**
- Status: PASSED
- Method: Launched camera_ros, viewed via rqt_image_view
- Result: Camera streaming at ~15 FPS

**Test 2: Web Server Launch**
- Status: PASSED
- Method: `ros2 launch web_control_panel web_panel.launch.py`
- Result: Flask server accessible at http://192.168.1.7:5000

**Test 3: ROSBridge Connection**
- Status: PASSED
- Method: Browser console shows "ROS: Connected"
- Result: WebSocket connection to port 9090 established

**Test 4: Video Stream in Browser**
- Status: PASSED
- Method: web_video_server streaming to `<img>` element
- Result: Live camera feed visible in control panel

**Test 5: UI Responsiveness**
- Status: PASSED
- Method: Tested button clicks, mode switching
- Result: UI responsive on desktop browser

---

### 5. Dependencies Installed on Pi

**ROS2 Packages:**
```bash
sudo apt-get install ros-humble-rosbridge-server
sudo apt-get install ros-humble-web-video-server
sudo apt-get install ros-humble-camera-ros
sudo apt-get install ros-humble-libcamera
```

**Python Packages:**
```bash
sudo apt-get install python3-flask
# Optional: pip3 install flask-cors
```

---

## Current Status

### Working:
- Camera live feed in web browser
- Web control panel UI fully rendered
- ROSBridge WebSocket connection
- Video streaming via web_video_server
- All buttons and controls rendered

### Pending Integration:
- Movement controls → `/cmd_vel` topic
- Arm controls → `/arm/joint_commands` topic
- Nav2 goal sending → `/navigate_to_pose` action
- Status feedback subscriptions

---

## Access Information

**Web Control Panel URL:**
```
http://192.168.1.7:5000
```

**Ports Used:**
| Port | Service | Purpose |
|------|---------|---------|
| 5000 | Flask | Web UI |
| 8080 | web_video_server | Camera stream |
| 9090 | rosbridge_websocket | ROS2 bridge |

---

## Quick Start Commands

**On Raspberry Pi (192.168.1.7):**

```bash
# Source workspace
source /opt/ros/humble/setup.bash
source ~/pi_ros_ws/gripper_car_ws/install/setup.bash

# Launch web control panel (includes camera, rosbridge, video server)
ros2 launch web_control_panel web_panel.launch.py

# Or launch components separately:
ros2 run rosbridge_server rosbridge_websocket --ros-args -p port:=9090
ros2 run web_video_server web_video_server --ros-args -p port:=8080
ros2 run camera_ros camera_node --ros-args -p format:=RGB888 -r image_raw:=/camera/image_raw
ros2 run web_control_panel web_server
```

**Access from any device on same network:**
```
http://192.168.1.7:5000
```

---

## Files Location

```
/home/pi/pi_ros_ws/gripper_car_ws/src/web_control_panel/
├── package.xml
├── setup.py
├── setup.cfg
├── PROGRESS.md          # This file
├── web_control_panel/
│   ├── __init__.py
│   └── web_server.py
├── templates/
│   └── index.html
├── static/
│   ├── css/
│   │   └── style.css
│   └── js/
│       └── control.js
├── launch/
│   └── web_panel.launch.py
└── config/
    └── web_params.yaml
```

---

## Next Steps

1. Test movement controls with actual robot hardware
2. Verify arm pose commands work correctly
3. Test Nav2 integration (map display, goal sending)
4. Add error handling and reconnection logic
5. Mobile device testing
6. Performance optimization

---

## Known Issues

1. **Camera Pipeline Conflict:** If camera_ros shows "Pipeline handler in use", kill any existing camera processes first
2. **Port Conflicts:** Ensure ports 5000, 8080, 9090 are free before launching
3. **flask_cors Optional:** Made CORS support optional to avoid import errors

---

## Documentation Created

1. `src/sensors/camera_driver/CAMERA_LIVE_FEED.md` - Camera setup guide
2. `src/web_control_panel/PROGRESS.md` - This progress update
3. Plan file: `/home/bhuvanesh/.claude/plans/stateful-seeking-dragon.md`
