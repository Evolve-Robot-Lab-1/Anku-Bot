#!/bin/bash
# Start Naval Inspection Robot control panel
#
# Usage:
#   bash start_naval.sh                    # with LiDAR
#   bash start_naval.sh --no-lidar         # without LiDAR
#
# This script:
#   1. Sources all required ROS2 workspaces
#   2. Kills any old processes on ports 5000/8090/9090
#   3. Starts the camera server in background (standalone, no ROS2)
#   4. Launches ROS2 nodes (rosbridge + lidar + obstacle_avoidance + serial_bridge + web_server)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CAM_SERVER="$SCRIPT_DIR/scripts/naval_cam_server.py"

# Parse args
USE_LIDAR="true"
if [[ "$1" == "--no-lidar" ]]; then
    USE_LIDAR="false"
fi

echo "=== Naval Inspection Robot ==="
echo "  LiDAR: $USE_LIDAR"
echo ""

# Source ROS2 workspaces
source /opt/ros/humble/setup.bash
# YDLiDAR driver lives in gripper_car_ws
if [[ -f "$HOME/pi_ros_ws/gripper_car_ws/install/setup.bash" ]]; then
    source "$HOME/pi_ros_ws/gripper_car_ws/install/setup.bash"
fi
# Naval workspace (overlays gripper_car_ws)
if [[ -f "$HOME/naval_ws/install/setup.bash" ]]; then
    source "$HOME/naval_ws/install/setup.bash"
fi
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# Kill any old processes
echo "[0/2] Cleaning up old processes..."
pkill -f "naval_cam_server.py" 2>/dev/null || true
pkill -f "rosbridge_websocket" 2>/dev/null || true
pkill -f "ydlidar_ros2_driver" 2>/dev/null || true
pkill -f "naval_serial_bridge" 2>/dev/null || true
pkill -f "naval_web_server" 2>/dev/null || true
pkill -f "naval_obstacle_avoidance" 2>/dev/null || true
sleep 1

# Start camera server in background
echo "[1/2] Starting camera server on port 8090..."
python3 "$CAM_SERVER" &
CAM_PID=$!
echo "  Camera server PID: $CAM_PID"
sleep 1

# Cleanup on exit
cleanup() {
    echo ""
    echo "Shutting down..."
    kill $CAM_PID 2>/dev/null || true
    wait $CAM_PID 2>/dev/null || true
    echo "Done."
}
trap cleanup EXIT INT TERM

# Launch ROS2 nodes
echo "[2/2] Launching ROS2 nodes..."
echo "  - rosbridge_websocket (port 9090)"
if [[ "$USE_LIDAR" == "true" ]]; then
    echo "  - ydlidar_ros2_driver (/scan)"
    echo "  - naval_obstacle_avoidance"
fi
echo "  - naval_serial_bridge (serial)"
echo "  - naval_web_server (port 5000)"
echo ""
echo "Access: http://$(hostname -I | awk '{print $1}'):5000"
echo ""

ros2 launch naval_web_control naval_panel.launch.py use_lidar:=$USE_LIDAR
