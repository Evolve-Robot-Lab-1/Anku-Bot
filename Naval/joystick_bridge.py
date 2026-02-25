#!/usr/bin/env python3
"""
Joystick Bridge for Naval Inspection Robot

Reads a Linux joystick (/dev/input/js0) and controls the robot via:
  - roslibpy: publishes Twist to /cmd_vel_raw through rosbridge (ws://10.0.0.2:9090)
  - requests: calls REST API at http://10.0.0.2:5000/api/{speed,estop,obstacle}

Mirrors the web UI control scheme exactly. Runs on PC, no ROS2 needed locally.

Dependencies: pip install roslibpy requests
Usage:       python3 joystick_bridge.py [--js /dev/input/js1] [--host 10.0.0.2]
"""

import struct
import threading
import time
import argparse
import sys
import requests
import roslibpy

# --- Joystick event format (Linux input API) ---
# struct js_event { __u32 time; __s16 value; __u8 type; __u8 number; }
JS_EVENT_FMT = 'IhBB'
JS_EVENT_SIZE = struct.calcsize(JS_EVENT_FMT)
JS_EVENT_BUTTON = 0x01
JS_EVENT_AXIS = 0x02
JS_EVENT_INIT = 0x80

# --- Config ---
DEADZONE = 0.15
PUBLISH_RATE = 10  # Hz - how often to publish Twist
API_TIMEOUT = 2    # seconds for REST calls

# Axis mapping
AXIS_LEFT_Y = 1   # Front camera tilt (slider)
AXIS_RIGHT_Y = 3  # Rear camera tilt (slider)
AXIS_DPAD_X = 6   # D-pad left/right (strafe)
AXIS_DPAD_Y = 7   # D-pad up/down (fwd/back)

# Button mapping
BTN_A = 0      # Stop all movement
BTN_B = 1      # Rotate right (hold)
BTN_X = 3      # Rotate left (hold)
BTN_Y = 4      # Cycle speed
BTN_LB = 6     # E-STOP toggle
BTN_RB = 7     # Obstacle avoidance toggle


class JoystickBridge:
    def __init__(self, js_device, host):
        self.js_device = js_device
        self.host = host
        self.api_base = f'http://{host}:5000/api'
        self.ws_url = f'ws://{host}:9090'

        # Axis states (raw, -1.0 to 1.0)
        self.axes = {}
        # Button states
        self.buttons = {}

        # Robot state
        self.speed_level = 1
        self.estop_active = False
        self.obstacle_enabled = False

        # Twist output
        self.linear_x = 0.0
        self.linear_y = 0.0
        self.angular_z = 0.0
        self.rotate_left = False
        self.rotate_right = False

        # Servo tilt state
        self.front_tilt = 0
        self.rear_tilt = 0

        # ROS connection
        self.ros = None
        self.twist_pub = None
        self.twist_pub_direct = None
        self.tilt_front_pub = None
        self.tilt_rear_pub = None
        self.connected = False

        self.running = True

    def connect_ros(self):
        """Connect to rosbridge websocket with auto-reconnect."""
        while self.running:
            try:
                print(f'[ROS] Connecting to {self.ws_url}...')
                self.ros = roslibpy.Ros(host=self.host, port=9090)
                self.ros.run()

                self.twist_pub = roslibpy.Topic(
                    self.ros, '/cmd_vel_raw', 'geometry_msgs/Twist')
                self.twist_pub.advertise()
                self.twist_pub_direct = roslibpy.Topic(
                    self.ros, '/cmd_vel', 'geometry_msgs/Twist')
                self.twist_pub_direct.advertise()
                self.tilt_front_pub = roslibpy.Topic(
                    self.ros, '/naval/tilt_front', 'std_msgs/Int32')
                self.tilt_front_pub.advertise()
                self.tilt_rear_pub = roslibpy.Topic(
                    self.ros, '/naval/tilt_rear', 'std_msgs/Int32')
                self.tilt_rear_pub.advertise()
                self.connected = True
                print('[ROS] Connected!')
                return
            except Exception as e:
                print(f'[ROS] Connection failed: {e}')
                self.connected = False
                time.sleep(3)

    def publish_twist(self):
        """Publish current twist values at fixed rate."""
        while self.running:
            if self.connected and self.twist_pub:
                try:
                    msg = roslibpy.Message({
                        'linear': {'x': self.linear_x, 'y': self.linear_y, 'z': 0.0},
                        'angular': {'x': 0.0, 'y': 0.0, 'z': self.angular_z}
                    })
                    self.twist_pub.publish(msg)
                    self.twist_pub_direct.publish(msg)
                except Exception:
                    self.connected = False
                    # Reconnect in background
                    threading.Thread(target=self.connect_ros, daemon=True).start()
            time.sleep(1.0 / PUBLISH_RATE)

    def api_call(self, endpoint, data):
        """Make a REST API call (non-blocking, fire-and-forget)."""
        def _call():
            try:
                r = requests.post(
                    f'{self.api_base}/{endpoint}',
                    json=data, timeout=API_TIMEOUT)
                if r.status_code == 200:
                    print(f'[API] {endpoint}: {data} -> OK')
                else:
                    print(f'[API] {endpoint}: HTTP {r.status_code}')
            except requests.RequestException as e:
                print(f'[API] {endpoint} failed: {e}')
        threading.Thread(target=_call, daemon=True).start()

    def apply_deadzone(self, value):
        """Apply deadzone and return -1.0, 0.0, or 1.0 (binary like web UI)."""
        if abs(value) < DEADZONE:
            return 0.0
        return 1.0 if value > 0 else -1.0

    def publish_tilt(self, topic_pub, angle):
        """Publish a tilt angle to a servo topic."""
        if self.connected and topic_pub:
            try:
                topic_pub.publish(roslibpy.Message({'data': angle}))
            except Exception:
                pass

    def update_twist(self):
        """Compute twist from D-pad only."""
        # D-pad Y (axis 7): up is negative
        dpad_y = self.apply_deadzone(self.axes.get(AXIS_DPAD_Y, 0.0))
        self.linear_x = -dpad_y

        # D-pad X (axis 6): right is positive
        dpad_x = self.apply_deadzone(self.axes.get(AXIS_DPAD_X, 0.0))
        self.linear_y = -dpad_x

        # Rotate from face buttons (X=left, B=right)
        if self.rotate_left:
            self.angular_z = 1.0
        elif self.rotate_right:
            self.angular_z = -1.0
        else:
            self.angular_z = 0.0

        # Left stick Y (axis 1): front camera tilt (slider)
        # Full up (-1.0) = 90°, full down (+1.0) = 0°, deadzone = hold
        left_y = self.axes.get(AXIS_LEFT_Y, 0.0)
        if abs(left_y) >= DEADZONE:
            # Remap: clamp to -1..+1, then scale deadzone..1.0 → full 0..90 range
            clamped = max(-1.0, min(1.0, left_y))
            if clamped < 0:
                # Up: map -DEADZONE..-1.0 → 45..90
                new_angle = int(45 + (abs(clamped) - DEADZONE) / (1.0 - DEADZONE) * 45)
            else:
                # Down: map +DEADZONE..+1.0 → 45..0
                new_angle = int(45 - (clamped - DEADZONE) / (1.0 - DEADZONE) * 45)
            new_angle = max(0, min(90, new_angle))
            if new_angle != self.front_tilt:
                self.front_tilt = new_angle
                self.publish_tilt(self.tilt_front_pub, new_angle)
                print(f'\n[TILT] Front: {new_angle}°')

        # Right stick Y (axis 3): rear camera tilt (slider)
        right_y = self.axes.get(AXIS_RIGHT_Y, 0.0)
        if abs(right_y) >= DEADZONE:
            clamped = max(-1.0, min(1.0, right_y))
            if clamped < 0:
                new_angle = int(45 + (abs(clamped) - DEADZONE) / (1.0 - DEADZONE) * 45)
            else:
                new_angle = int(45 - (clamped - DEADZONE) / (1.0 - DEADZONE) * 45)
            new_angle = max(0, min(90, new_angle))
            if new_angle != self.rear_tilt:
                self.rear_tilt = new_angle
                self.publish_tilt(self.tilt_rear_pub, new_angle)
                print(f'\n[TILT] Rear: {new_angle}°')

    def handle_button(self, number, pressed):
        """Handle button events. Rotate buttons respond to press+release."""
        # Rotate buttons: hold to turn, release to stop
        if number == BTN_X:
            self.rotate_left = pressed
            self.update_twist()
            return
        if number == BTN_B:
            self.rotate_right = pressed
            self.update_twist()
            return

        # Other buttons: act on press only
        if not pressed:
            return

        if number == BTN_A:
            # Stop all movement
            self.linear_x = 0.0
            self.linear_y = 0.0
            self.angular_z = 0.0
            self.rotate_left = False
            self.rotate_right = False
            print('[STOP] All movement stopped')

        elif number == BTN_Y:
            # Cycle speed: 1 -> 2 -> 3 -> 1
            self.speed_level = (self.speed_level % 3) + 1
            self.api_call('speed', {'level': self.speed_level})
            print(f'[SPEED] Level {self.speed_level}')

        elif number == BTN_LB:
            # Toggle E-STOP
            self.estop_active = not self.estop_active
            self.api_call('estop', {'active': self.estop_active})
            state = 'ACTIVATED' if self.estop_active else 'released'
            print(f'[ESTOP] {state}')

        elif number == BTN_RB:
            # Toggle obstacle avoidance
            self.obstacle_enabled = not self.obstacle_enabled
            self.api_call('obstacle', {'enabled': self.obstacle_enabled})
            state = 'ON' if self.obstacle_enabled else 'OFF'
            print(f'[OBSTACLE] {state}')

    def read_joystick(self):
        """Read joystick events (blocking loop)."""
        try:
            with open(self.js_device, 'rb') as js:
                print(f'[JS] Opened {self.js_device}')
                while self.running:
                    event = js.read(JS_EVENT_SIZE)
                    if not event:
                        break

                    timestamp, value, event_type, number = struct.unpack(
                        JS_EVENT_FMT, event)

                    # Filter out init events
                    if event_type & JS_EVENT_INIT:
                        continue

                    if event_type == JS_EVENT_AXIS:
                        # Normalize to -1.0 .. 1.0
                        self.axes[number] = value / 32767.0
                        self.update_twist()

                    elif event_type == JS_EVENT_BUTTON:
                        self.buttons[number] = value
                        self.handle_button(number, value == 1)

        except FileNotFoundError:
            print(f'[JS] Device not found: {self.js_device}')
            print('[JS] Is the gamepad connected? Check: ls /dev/input/js*')
            self.running = False
        except PermissionError:
            print(f'[JS] Permission denied: {self.js_device}')
            print('[JS] Try: sudo chmod 666 /dev/input/js0')
            self.running = False

    def print_status(self):
        """Periodically print current state."""
        while self.running:
            if self.connected:
                direction = ''
                if self.linear_x > 0:
                    direction += 'FWD '
                elif self.linear_x < 0:
                    direction += 'BWD '
                if self.linear_y > 0:
                    direction += 'LEFT '
                elif self.linear_y < 0:
                    direction += 'RIGHT '
                if self.angular_z > 0:
                    direction += 'ROT-L '
                elif self.angular_z < 0:
                    direction += 'ROT-R '
                if not direction:
                    direction = 'STOP'

                estop = ' [E-STOP]' if self.estop_active else ''
                obst = ' [OBS]' if self.obstacle_enabled else ''
                tilt = f' F:{self.front_tilt} R:{self.rear_tilt}'
                print(f'\r[NAV] Spd:{self.speed_level} {direction.strip()}{estop}{obst}{tilt}    ', end='', flush=True)
            time.sleep(0.2)

    def run(self):
        """Main entry point."""
        print('=== Naval Joystick Bridge ===')
        print(f'Joystick: {self.js_device}')
        print(f'Robot:    {self.host}')
        print()
        print('Controls:')
        print('  D-pad        : Move (fwd/back/strafe)')
        print('  X (left)     : Rotate left (hold)')
        print('  B (right)    : Rotate right (hold)')
        print('  A (bottom)   : Stop all movement')
        print('  Y (top)      : Cycle speed (1/2/3)')
        print('  LB           : E-STOP toggle')
        print('  RB           : Obstacle avoidance toggle')
        print('  Left stick   : Front camera tilt (slider)')
        print('  Right stick  : Rear camera tilt (slider)')
        print()

        # Start ROS connection in background
        ros_thread = threading.Thread(target=self.connect_ros, daemon=True)
        ros_thread.start()

        # Start twist publisher
        pub_thread = threading.Thread(target=self.publish_twist, daemon=True)
        pub_thread.start()

        # Start status printer
        status_thread = threading.Thread(target=self.print_status, daemon=True)
        status_thread.start()

        # Read joystick (blocks)
        try:
            self.read_joystick()
        except KeyboardInterrupt:
            print('\n[EXIT] Shutting down...')
        finally:
            self.running = False
            # Send stop before exit
            if self.connected and self.twist_pub:
                try:
                    stop_msg = roslibpy.Message({
                        'linear': {'x': 0.0, 'y': 0.0, 'z': 0.0},
                        'angular': {'x': 0.0, 'y': 0.0, 'z': 0.0}
                    })
                    self.twist_pub.publish(stop_msg)
                    if self.twist_pub_direct:
                        self.twist_pub_direct.publish(stop_msg)
                    time.sleep(0.1)
                    self.twist_pub.unadvertise()
                    if self.twist_pub_direct:
                        self.twist_pub_direct.unadvertise()
                    if self.tilt_front_pub:
                        self.tilt_front_pub.unadvertise()
                    if self.tilt_rear_pub:
                        self.tilt_rear_pub.unadvertise()
                except Exception:
                    pass
            if self.ros:
                try:
                    self.ros.terminate()
                except Exception:
                    pass
            print('[EXIT] Done.')


def main():
    parser = argparse.ArgumentParser(description='Naval Robot Joystick Bridge')
    parser.add_argument('--js', default='/dev/input/js0',
                        help='Joystick device (default: /dev/input/js0)')
    parser.add_argument('--host', default='10.0.0.2',
                        help='Robot IP address (default: 10.0.0.2)')
    args = parser.parse_args()

    bridge = JoystickBridge(args.js, args.host)
    bridge.run()


if __name__ == '__main__':
    main()
