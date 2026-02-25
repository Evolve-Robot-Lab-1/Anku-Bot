#!/usr/bin/env python3
"""
Naval Serial Bridge Node
=========================
Bridges ROS2 topics to Arduino serial commands for mecanum wheel control
and camera tilt servos.

Arduino commands:
    F/B/L/R/W/U/S = movement, 1/2/3 = speed
    T<0-90> = front camera tilt, G<0-90> = rear camera tilt

Topics:
    Subscribe:
        /cmd_vel (geometry_msgs/Twist): Velocity -> F/B/L/R/W/U/S
        /naval/speed (std_msgs/Int32): Speed level -> 1/2/3
        /naval/estop (std_msgs/Bool): Emergency stop -> S
        /naval/tilt_front (std_msgs/Int32): Front camera tilt 0-90
        /naval/tilt_rear (std_msgs/Int32): Rear camera tilt 0-90
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
        self.declare_parameter('serial_port', 'auto')
        self.declare_parameter('baud_rate', 115200)
        self.declare_parameter('reconnect_interval', 2.0)

        self.serial_port_param = self.get_parameter('serial_port').value
        self.baud_rate = self.get_parameter('baud_rate').value
        self.reconnect_interval = self.get_parameter('reconnect_interval').value

        # Auto-detect Arduino port
        self.serial_port = self._find_arduino_port()

        # Serial connection
        self.serial_conn = None
        self.serial_lock = threading.Lock()
        self.running = True

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
        self.tilt_front_sub = self.create_subscription(
            Int32, '/naval/tilt_front', self.tilt_front_callback, 10)
        self.tilt_rear_sub = self.create_subscription(
            Int32, '/naval/tilt_rear', self.tilt_rear_callback, 10)

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

    def _find_arduino_port(self):
        """Auto-detect Arduino serial port."""
        if self.serial_port_param != 'auto':
            return self.serial_port_param
        import glob
        ports = sorted(glob.glob('/dev/ttyACM*'))
        if ports:
            self.get_logger().info(f'Auto-detected Arduino port: {ports[0]}')
            return ports[0]
        self.get_logger().warn('No ttyACM device found, will retry on connect')
        return '/dev/ttyACM0'

    def connect_serial(self):
        """Attempt to connect to Arduino."""
        try:
            with self.serial_lock:
                if self.serial_conn is not None:
                    try:
                        self.serial_conn.close()
                    except Exception:
                        pass
                    self.serial_conn = None

                # Re-detect port on each connect attempt
                self.serial_port = self._find_arduino_port()

                self.serial_conn = serial.Serial(
                    port=self.serial_port,
                    baudrate=self.baud_rate,
                    timeout=0.5,
                )

                # Wait for Arduino bootloader reset
                time.sleep(3.0)

                self.serial_conn.reset_input_buffer()
                self.serial_conn.reset_output_buffer()

                self.get_logger().info(f'Connected to {self.serial_port}')
                self.publish_status('Serial connected')
                return True

        except (serial.SerialException, OSError) as e:
            self.get_logger().warn(f'Failed to connect: {e}')
            with self.serial_lock:
                self.serial_conn = None
            return False

    def send_command(self, cmd):
        """Send command to Arduino with deduplication."""
        now = time.time()

        # Dedup: skip if same command sent recently (except S which always goes)
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
        """Map Twist to single-char command."""
        lx = msg.linear.x
        ly = msg.linear.y
        az = msg.angular.z
        threshold = 0.01

        if abs(az) > threshold:
            return 'U' if az > 0 else 'W'
        elif abs(lx) > threshold or abs(ly) > threshold:
            if abs(lx) >= abs(ly):
                return 'F' if lx > 0 else 'B'
            else:
                return 'L' if ly > 0 else 'R'
        else:
            return 'S'

    def cmd_vel_callback(self, msg):
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
        level = msg.data
        if level in (1, 2, 3):
            self.send_command(str(level))
            self.publish_status(f'Speed set to {level}')

    def estop_callback(self, msg):
        self.estop_active = msg.data
        if self.estop_active:
            self.send_command('S')
            self.publish_status('E-STOP ACTIVE')
        else:
            self.publish_status('E-STOP released')

    def tilt_front_callback(self, msg):
        """Set front camera tilt (0=forward, 90=down)."""
        angle = max(0, min(90, msg.data))
        self.send_command(f'T{angle}')

    def tilt_rear_callback(self, msg):
        """Set rear camera tilt (0=forward, 90=down)."""
        angle = max(0, min(90, msg.data))
        self.send_command(f'G{angle}')

    def check_cmd_timeout(self):
        if self.vel_active and (time.time() - self.last_vel_time) > 0.5:
            self.send_command('S')
            self.vel_active = False

    def publish_status(self, text):
        msg = String()
        msg.data = text
        self.status_pub.publish(msg)

    def serial_reader_loop(self):
        read_errors = 0

        while self.running:
            if self.serial_conn is None or not self.serial_conn.is_open:
                if not self.connect_serial():
                    time.sleep(self.reconnect_interval)
                    continue
                read_errors = 0

            try:
                with self.serial_lock:
                    if self.serial_conn is None:
                        time.sleep(0.1)
                        continue
                    if self.serial_conn.in_waiting > 0:
                        line = self.serial_conn.readline().decode(
                            'utf-8', errors='ignore').strip()
                    else:
                        line = None

                if line:
                    self.get_logger().info(f'Arduino: {line}')
                    self.publish_status(line)
                    read_errors = 0
                else:
                    time.sleep(0.05)

            except (serial.SerialException, OSError) as e:
                read_errors += 1
                if read_errors <= 3:
                    self.get_logger().warn(f'Serial read error: {e}')
                elif read_errors == 4:
                    self.get_logger().warn(
                        'Read errors continue, suppressing (writes still work)')
                # Don't close connection - writes may still work
                time.sleep(0.5)

            except Exception as e:
                read_errors += 1
                if read_errors <= 3:
                    self.get_logger().warn(f'Reader error: {e}')
                elif read_errors == 4:
                    self.get_logger().warn(
                        'Read errors continue, suppressing (writes still work)')
                time.sleep(0.5)

    def destroy_node(self):
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
