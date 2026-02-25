#!/bin/bash
# Kill Naval Robot from PC -> RPi
# Usage: bash kill_naval.sh

RPI="pi@10.0.0.2"
PASS="123"

echo "=== Naval Robot Shutdown ==="

if ! ping -c 1 -W 2 10.0.0.2 >/dev/null 2>&1; then
    echo "RPi not reachable. Already off?"
    exit 0
fi

# Stop systemd service first (prevents auto-restart)
sshpass -p "$PASS" ssh $RPI 'echo "123" | sudo -S systemctl stop naval 2>/dev/null'

# Send stop to Arduino
sshpass -p "$PASS" ssh $RPI 'source /opt/ros/humble/setup.bash && source ~/naval_ws/install/setup.bash && ros2 topic pub --once /cmd_vel geometry_msgs/Twist "{linear: {x: 0, y: 0, z: 0}, angular: {x: 0, y: 0, z: 0}}" 2>/dev/null' &
sleep 2

# Kill all processes
sshpass -p "$PASS" ssh $RPI 'killall -9 python3 web_server serial_bridge obstacle_avoidance ydlidar_ros2_driver_node 2>/dev/null'
sleep 1

# Verify
PORTS=$(sshpass -p "$PASS" ssh $RPI "ss -tlnp | grep -cE '5000|8090|9090'" 2>/dev/null)
if [ "$PORTS" -eq 0 ] 2>/dev/null; then
    echo "All stopped."
else
    echo "Some processes still running, force killing..."
    sshpass -p "$PASS" ssh $RPI 'killall -9 python3 2>/dev/null'
    echo "Done."
fi
