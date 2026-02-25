#!/usr/bin/env python3
"""
Naval Serial Bridge Node
=========================
Bridges ROS2 topics to Arduino serial commands for mecanum wheel control.

Arduino accepts single-char commands:
    F=Forward, B=Backward, L=Left, R=Right, W=RotateCW, U=RotateCCW
    S=Stop, 1=Speed1, 2=Speed2, 3=Speed3

Topics:
    Subscribe:
        /cmd_vel (geometry_msgs/Twist): Velocity commands -> F/B/L/R/W/U/S
        /naval/speed (std_msgs/Int32): Speed level -> 1/2/3
        /naval/estop (std_msgs/Bool): Emergency stop -> S
    Publish:
        /naval/status (std_msgs/String): Status messages from Arduino

Usage:
    ros2 run naval_web_control serial_bridge
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Int32, Bool, String

import serial
import threading
import time


class NavalSerialBridge(Node):
    def __init__(self):
        super().__init__('naval_serial_bridge')

        # Parameters
        self.declare_parameter('serial_port', '/dev/ttyACM0')
        self.declare_parameter('baud_rate', 115200)
        self.declare_parameter('reconnect_interval', 2.0)

        self.serial_port = self.get_parameter('serial_port').value
        self.baud_rate = self.get_parameter('baud_rate').value
        self.reconnect_interval = self.get_parameter('reconnect_interval').value

        # Serial connection
        self.serial_conn = None
        self.serial_lock = threading.Lock()
        self.running = True
        self.first_connect = True

        # Deduplication: track last command to avoid flooding Arduino
        self.last_cmd = None
        self.last_cmd_time = 0.0
        self.cmd_min_interval = 0.05  # 50ms minimum between duplicate commands

        # E-stop state
        self.estop_active = False

        # Publishers
        self.status_pub = self.create_publisher(String, '/naval/status', 10)

        # Subscribers
        self.cmd_vel_sub = self.create_subscription(
            Twist, '/cmd_vel', self.cmd_vel_callback, 10)
        self.speed_sub = self.create_subscription(
            Int32, '/naval/speed', self.speed_callback, 10)
        self.estop_sub = self.create_subscription(
            Bool, '/naval/estop', self.estop_callback, 10)

        # Start serial reader thread
        self.reader_thread = threading.Thread(
            target=self.serial_reader_loop, daemon=True)
        self.reader_thread.start()

        # Command timeout: send STOP if no cmd_vel received for 0.5s
        self.last_vel_time = time.time()
        self.vel_active = False
        self.timeout_timer = self.create_timer(0.1, self.check_cmd_timeout)

        self.get_logger().info(f'Naval Serial Bridge started')
        self.get_logger().info(f'  Port: {self.serial_port}')
        self.get_logger().info(f'  Baud: {self.baud_rate}')

    def connect_serial(self):
        """Attempt to connect to Arduino."""
        try:
            with self.serial_lock:
                if self.serial_conn is not None:
                    try:
                        self.serial_conn.close()
                    except Exception:
                        pass

                self.serial_conn = serial.Serial(
                    port=self.serial_port,
                    baudrate=self.baud_rate,
                    timeout=0.1,
                    dsrdtr=False,
                    rtscts=False,
                    exclusive=True
                )

                if self.first_connect:
                    self.serial_conn.dtr = False
                    time.sleep(0.1)
                    self.serial_conn.dtr = True
                    time.sleep(2.5)  # Wait for Arduino bootloader
                    self.first_connect = False
                else:
                    time.sleep(0.5)

                self.serial_conn.reset_input_buffer()
                self.serial_conn.reset_output_buffer()

                self.get_logger().info(f'Connected to {self.serial_port}')
                self.publish_status('Serial connected')
                return True

        except serial.SerialException as e:
            self.get_logger().warn(f'Failed to connect: {e}')
            return False

    def send_command(self, cmd):
        """Send a single-char command to Arduino with deduplication."""
        now = time.time()

        # Dedup: skip if same command sent recently (except S which always goes through)
        if cmd != 'S' and cmd == self.last_cmd:
            if now - self.last_cmd_time < self.cmd_min_interval:
                return True

        with self.serial_lock:
            if self.serial_conn is None or not self.serial_conn.is_open:
                return False
            try:
                self.serial_conn.write(f'{cmd}\n'.encode('utf-8'))
                self.serial_conn.flush()
                self.last_cmd = cmd
                self.last_cmd_time = now
                self.get_logger().debug(f'Sent: {cmd}')
                return True
            except serial.SerialException as e:
                self.get_logger().error(f'Serial write error: {e}')
                return False

    def twist_to_command(self, msg):
        """Map Twist message to Arduino single-char command.

        linear.x > 0  -> F (forward)
        linear.x < 0  -> B (backward)
        linear.y > 0  -> L (strafe left)
        linear.y < 0  -> R (strafe right)
        angular.z > 0 -> U (rotate CCW / left turn)
        angular.z < 0 -> W (rotate CW / right turn)
        all zero       -> S (stop)
        """
        lx = msg.linear.x
        ly = msg.linear.y
        az = msg.angular.z

        # Dead zone threshold
        threshold = 0.01

        # Priority: rotation > linear
        if abs(az) > threshold:
            return 'U' if az > 0 else 'W'
        elif abs(lx) > threshold or abs(ly) > threshold:
            # Choose dominant axis
            if abs(lx) >= abs(ly):
                return 'F' if lx > 0 else 'B'
            else:
                return 'L' if ly > 0 else 'R'
        else:
            return 'S'

    def cmd_vel_callback(self, msg):
        """Handle velocity commands from rosbridge."""
        if self.estop_active:
            return

        cmd = self.twist_to_command(msg)
        self.send_command(cmd)

        if cmd != 'S':
            self.vel_active = True
            self.last_vel_time = time.time()
        else:
            self.vel_active = False

    def speed_callback(self, msg):
        """Handle speed level changes (1/2/3)."""
        level = msg.data
        if level in (1, 2, 3):
            self.send_command(str(level))
            self.publish_status(f'Speed set to {level}')

    def estop_callback(self, msg):
        """Handle emergency stop."""
        self.estop_active = msg.data
        if self.estop_active:
            self.send_command('S')
            self.publish_status('E-STOP ACTIVE')
        else:
            self.publish_status('E-STOP released')

    def check_cmd_timeout(self):
        """Send STOP if no velocity command received recently."""
        if self.vel_active and (time.time() - self.last_vel_time) > 0.5:
            self.send_command('S')
            self.vel_active = False

    def publish_status(self, text):
        """Publish a status message."""
        msg = String()
        msg.data = text
        self.status_pub.publish(msg)

    def serial_reader_loop(self):
        """Background thread to read serial data from Arduino."""
        error_count = 0
        max_errors = 5

        while self.running:
            if self.serial_conn is None or not self.serial_conn.is_open:
                if not self.connect_serial():
                    time.sleep(self.reconnect_interval)
                    continue
                error_count = 0

            try:
                with self.serial_lock:
                    if self.serial_conn.in_waiting > 0:
                        line = self.serial_conn.readline().decode(
                            'utf-8', errors='ignore').strip()
                    else:
                        line = None

                if line:
                    self.get_logger().info(f'Arduino: {line}')
                    self.publish_status(line)
                    error_count = 0
                else:
                    time.sleep(0.01)

            except serial.SerialException as e:
                error_count += 1
                if error_count >= max_errors:
                    self.get_logger().error(
                        f'Serial read error ({error_count}x): {e}')
                    with self.serial_lock:
                        try:
                            self.serial_conn.close()
                        except Exception:
                            pass
                        self.serial_conn = None
                    time.sleep(self.reconnect_interval)
                    error_count = 0
                else:
                    time.sleep(0.1)

            except Exception as e:
                self.get_logger().error(f'Unexpected error in reader: {e}')
                time.sleep(0.1)

    def destroy_node(self):
        """Cleanup on shutdown."""
        self.running = False
        self.send_command('S')
        with self.serial_lock:
            if self.serial_conn is not None:
                try:
                    self.serial_conn.close()
                except Exception:
                    pass
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = NavalSerialBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
