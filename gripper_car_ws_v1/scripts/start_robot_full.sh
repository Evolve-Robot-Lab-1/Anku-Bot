#!/bin/bash
# ============================================
# FULL ROBOT STARTUP SCRIPT (Run on RPi)
# ============================================
# This script starts all robot nodes:
# - Serial bridge (motor + arm control)
# - LiDAR driver
# - Camera + video server
# - Obstacle avoidance
# - Web control panel (with rosbridge)
#
# Usage: ~/start_robot_full.sh
# ============================================

echo "============================================"
echo "  Mobile Manipulator - Full Startup"
echo "============================================"

# Source ROS2
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

# ROS2 Network Configuration
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# Set device permissions
echo "[1/7] Setting device permissions..."
sudo chmod 666 /dev/ttyUSB0 2>/dev/null || echo "  Warning: /dev/ttyUSB0 not found"
sudo chmod 666 /dev/ttyACM0 2>/dev/null || echo "  Warning: /dev/ttyACM0 not found"

# Start Serial Bridge (Motor + Arm Control)
echo "[2/7] Starting Serial Bridge (Motor + Arm)..."
ros2 run base_controller serial_bridge_node.py > /tmp/serial.log 2>&1 &
SERIAL_PID=$!
sleep 2

# Start LiDAR
echo "[3/7] Starting LiDAR driver..."
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p lidar_type:=1 \
  -p isSingleChannel:=true \
  -p support_motor_dtr:=true > /tmp/lidar.log 2>&1 &
LIDAR_PID=$!
sleep 3

# Start Camera (BGR888 format for web_video_server compatibility)
echo "[4/7] Starting Camera..."
ros2 run camera_ros camera_node --ros-args -p format:=BGR888 -p width:=640 -p height:=480 -r image_raw:=/camera/image_raw > /tmp/camera.log 2>&1 &
CAMERA_PID=$!
sleep 2

# Start Video Server
echo "[5/7] Starting Video Server..."
ros2 run web_video_server web_video_server --ros-args -p port:=8080 > /tmp/video.log 2>&1 &
VIDEO_PID=$!
sleep 2

# Start Obstacle Avoidance (on RPi to reduce latency)
echo "[6/7] Starting Obstacle Avoidance..."
ros2 run base_controller obstacle_avoidance_node.py > /tmp/obstacle.log 2>&1 &
OBS_PID=$!
sleep 2

# Start Web Control Panel
echo "[7/7] Starting Web Control Panel..."
ros2 launch web_control_panel web_panel.launch.py > /tmp/web.log 2>&1 &
WEB_PID=$!
sleep 3

# Get IP address
IP_ADDR=$(hostname -I | awk '{print $1}')

echo ""
echo "============================================"
echo "  ROBOT READY!"
echo "============================================"
echo ""
echo "  Web Control Panel: http://${IP_ADDR}:5000"
echo "  Video Stream:      http://${IP_ADDR}:8080"
echo ""
echo "  Running processes:"
echo "    Serial Bridge: $SERIAL_PID"
echo "    LiDAR:         $LIDAR_PID"
echo "    Camera:        $CAMERA_PID"
echo "    Video Server:  $VIDEO_PID"
echo "    Obstacle:      $OBS_PID"
echo "    Web Panel:     $WEB_PID"
echo ""
echo "  Log files: /tmp/{serial,lidar,camera,video,obstacle,web}.log"
echo ""
echo "  Press Ctrl+C to stop all nodes"
echo "============================================"

# Cleanup function
cleanup() {
    echo ""
    echo "Stopping all nodes..."
    kill $SERIAL_PID $LIDAR_PID $CAMERA_PID $VIDEO_PID $OBS_PID $WEB_PID 2>/dev/null
    pkill -f serial_bridge 2>/dev/null
    pkill -f ydlidar_ros2_driver 2>/dev/null
    pkill -f camera_node 2>/dev/null
    pkill -f obstacle_avoidance 2>/dev/null
    pkill -f rosbridge 2>/dev/null
    pkill -f web_video_server 2>/dev/null
    pkill -f web_server 2>/dev/null
    echo "All nodes stopped."
    exit 0
}

trap cleanup INT TERM

# Wait forever
wait
