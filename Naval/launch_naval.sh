#!/bin/bash
# Launch Naval Robot from PC -> RPi
# Usage: bash launch_naval.sh [--no-lidar] [--joystick]

RPI="pi@10.0.0.2"
PASS="123"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LIDAR_FLAG=""
USE_JOYSTICK=false

for arg in "$@"; do
    case "$arg" in
        --no-lidar) LIDAR_FLAG="--no-lidar" ;;
        --joystick) USE_JOYSTICK=true ;;
    esac
done

echo "=== Naval Robot Launch ==="

# Check RPi is reachable
if ! ping -c 1 -W 2 10.0.0.2 >/dev/null 2>&1; then
    echo "ERROR: RPi not reachable at 10.0.0.2"
    echo "Check ethernet cable and RPi power."
    exit 1
fi

# Wait for SSH to be ready
echo "[1/4] Waiting for SSH..."
for i in $(seq 1 10); do
    sshpass -p "$PASS" ssh -o ConnectTimeout=3 -o StrictHostKeyChecking=no $RPI "echo ok" >/dev/null 2>&1 && break
    sleep 3
done

# Stop systemd service and kill old processes
echo "[2/4] Killing old processes..."
sshpass -p "$PASS" ssh $RPI 'echo "123" | sudo -S systemctl stop naval 2>/dev/null; killall -9 python3 web_server serial_bridge obstacle_avoidance ydlidar_ros2_driver_node 2>/dev/null; sleep 1'

# Launch
echo "[3/4] Launching naval system..."
sshpass -p "$PASS" ssh $RPI "cd ~/naval_ws/src/naval_web_control && nohup bash start_naval.sh $LIDAR_FLAG > /tmp/naval_launch.log 2>&1 &"

# Wait and verify - poll every 2s instead of fixed sleep
echo "[4/4] Waiting for services..."
for i in $(seq 1 8); do
    sleep 2
    PORTS=$(sshpass -p "$PASS" ssh -o ConnectTimeout=2 $RPI "ss -tlnp | grep -cE '5000|8090|9090'" 2>/dev/null)
    if [ "$PORTS" -ge 3 ] 2>/dev/null; then
        echo ""
        echo "=== READY (${i}x2s) ==="
        echo "Web UI:  http://10.0.0.2:5000"
        echo ""
        break
    fi
done

if [ "$PORTS" -lt 3 ] 2>/dev/null; then
    echo ""
    echo "WARNING: Not all services started. Check log:"
    echo "  sshpass -p '123' ssh pi@10.0.0.2 'tail -30 /tmp/naval_launch.log'"
fi

# Launch joystick bridge on PC if requested
if $USE_JOYSTICK; then
    if [ -f "$SCRIPT_DIR/joystick_bridge.py" ] && [ -e /dev/input/js0 ]; then
        echo ""
        echo "=== Starting Joystick Bridge ==="
        python3 "$SCRIPT_DIR/joystick_bridge.py"
    else
        [ ! -e /dev/input/js0 ] && echo "WARNING: No gamepad found at /dev/input/js0"
        [ ! -f "$SCRIPT_DIR/joystick_bridge.py" ] && echo "WARNING: joystick_bridge.py not found"
    fi
fi
