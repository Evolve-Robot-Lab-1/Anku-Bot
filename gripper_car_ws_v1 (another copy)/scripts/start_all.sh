#!/bin/bash
# Complete Robot Launch Script
# Starts: Control Panel, Serial Bridge, Camera, LiDAR, Obstacle Avoidance

PI_IP="192.168.1.27"
PI_USER="pi"
PI_WS="/home/pi/pi_ros_ws/gripper_car_ws"

echo "=== Robot Full System Launcher ==="

# Kill any existing processes on RPi
echo "[1/6] Cleaning up old processes on RPi..."
ssh ${PI_USER}@${PI_IP} "pkill -9 -f ros2 2>/dev/null; pkill -9 -f python3 2>/dev/null; sleep 2" 2>/dev/null
echo "      Done"

# Start LiDAR
echo "[2/6] Starting LiDAR..."
ssh ${PI_USER}@${PI_IP} "nohup bash -c 'source /opt/ros/humble/setup.bash && source ${PI_WS}/install/setup.bash && export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200 -p lidar_type:=1 -p isSingleChannel:=true -p support_motor_dtr:=true' > /tmp/lidar.log 2>&1 &"
sleep 2
echo "      Done"

# Start Serial Bridge (motor + arm control)
echo "[3/6] Starting Serial Bridge..."
ssh ${PI_USER}@${PI_IP} "nohup bash -c 'source /opt/ros/humble/setup.bash && source ${PI_WS}/install/setup.bash && export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && ros2 run base_controller serial_bridge_node.py' > /tmp/serial.log 2>&1 &"
sleep 2
echo "      Done"

# Start Control Panel (includes camera and rosbridge)
echo "[4/6] Starting Control Panel (camera + web server + rosbridge)..."
ssh ${PI_USER}@${PI_IP} "nohup bash -c 'source /opt/ros/humble/setup.bash && source ${PI_WS}/install/setup.bash && export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && ros2 launch web_control_panel web_panel.launch.py' > /tmp/panel.log 2>&1 &"
sleep 5
echo "      Done"

# Start Obstacle Avoidance (locally or on RPi)
echo "[5/6] Starting Obstacle Avoidance..."
source /opt/ros/humble/setup.bash
LOCAL_WS="/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1"
if [ -f "${LOCAL_WS}/install/setup.bash" ]; then
    source ${LOCAL_WS}/install/setup.bash
fi
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
nohup ros2 run base_controller obstacle_avoidance_node.py > /tmp/obstacle.log 2>&1 &
sleep 2
echo "      Done"

# Verify nodes
echo "[6/6] Verifying nodes..."
sleep 3
ssh ${PI_USER}@${PI_IP} "source /opt/ros/humble/setup.bash && export ROS_DOMAIN_ID=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && ros2 node list 2>/dev/null"

echo ""
echo "=== All Systems Started ==="
echo ""
echo "Control Panel: http://${PI_IP}:5000"
echo "Camera Stream: http://${PI_IP}:8080/stream?topic=/camera/image_raw"
echo ""
echo "Features:"
echo "  - Base motors (D-pad)"
echo "  - Arm control (sliders + presets)"
echo "  - Camera feed"
echo "  - Obstacle avoidance (Auto mode)"
echo ""
echo "Logs:"
echo "  ssh pi@${PI_IP} 'cat /tmp/lidar.log'"
echo "  ssh pi@${PI_IP} 'cat /tmp/serial.log'"
echo "  ssh pi@${PI_IP} 'cat /tmp/panel.log'"
echo "  cat /tmp/obstacle.log"
