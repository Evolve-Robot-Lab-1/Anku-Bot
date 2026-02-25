#!/usr/bin/env python3
"""
Serial Bridge Node (Unified)
============================

Handles serial communication with Arduino for mobile base AND arm control.

Topics:
    Subscribe:
        /cmd_vel (geometry_msgs/Twist): Velocity commands for base
        /arm/joint_commands (sensor_msgs/JointState): Joint commands for arm

    Publish:
        /encoder_ticks (std_msgs/Int64MultiArray): Raw encoder counts [FL, FR, BL, BR]
        /arm/joint_states (sensor_msgs/JointState): Current arm joint positions
        /base/status (std_msgs/String): Status messages from Arduino
        /arduino_ack (std_msgs/String): Command acknowledgments

Parameters:
    serial_port: Arduino serial port (default: /dev/ttyACM0)
    baud_rate: Serial baud rate (default: 115200)
    cmd_timeout: Command timeout in seconds (default: 0.5)
    reconnect_interval: Reconnect interval in seconds (default: 2.0)

Usage:
    ros2 run base_controller serial_bridge_node.py
    ros2 run base_controller serial_bridge_node.py --ros-args -p serial_port:=/dev/ttyACM1
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from sensor_msgs.msg import JointState
from std_msgs.msg import Int64MultiArray, String
from std_srvs.srv import SetBool
import math
import numpy as np

import serial
import threading
import time


class SerialBridgeNode(Node):
    def __init__(self):
        super().__init__('serial_bridge')

        # Declare parameters
        self.declare_parameter('serial_port', '/dev/ttyACM0')
        self.declare_parameter('baud_rate', 115200)
        self.declare_parameter('cmd_timeout', 0.5)
        self.declare_parameter('reconnect_interval', 2.0)

        # Get parameters
        self.serial_port = self.get_parameter('serial_port').value
        self.baud_rate = self.get_parameter('baud_rate').value
        self.cmd_timeout = self.get_parameter('cmd_timeout').value
        self.reconnect_interval = self.get_parameter('reconnect_interval').value

        # Serial connection
        self.serial_conn = None
        self.serial_lock = threading.Lock()
        self.running = True
        self.first_connect = True  # Track if this is first connection

        # Arm joint configuration
        self.joint_names = [
            'joint_1', 'joint_2', 'joint_3',
            'joint_4', 'gripper_base_joint', 'left_gear_joint'
        ]
        self.joint_positions = [0.0] * 6  # Current positions in radians
        self.servo_angles = [90] * 6  # Current servo angles in degrees

        # Publishers
        self.encoder_pub = self.create_publisher(Int64MultiArray, '/encoder_ticks', 10)
        self.status_pub = self.create_publisher(String, '/base/status', 10)
        self.arm_joint_pub = self.create_publisher(JointState, '/arm/joint_states', 10)
        self.ack_pub = self.create_publisher(String, '/arduino_ack', 10)

        # Subscribers
        self.cmd_vel_sub = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_callback,
            10
        )
        self.arm_cmd_sub = self.create_subscription(
            JointState,
            '/arm/joint_commands',
            self.arm_cmd_callback,
            10
        )

        # Services
        self.test_mode_srv = self.create_service(
            SetBool,
            '/base/set_test_mode',
            self.set_test_mode_callback
        )

        # Command timeout tracking
        self.last_cmd_time = time.time()
        self.cmd_sent = False
        self.test_mode = False

        # Start serial reader thread
        self.reader_thread = threading.Thread(target=self.serial_reader_loop, daemon=True)
        self.reader_thread.start()

        # Create timer for command timeout check
        self.timeout_timer = self.create_timer(0.1, self.check_cmd_timeout)

        # Create timer for publishing arm joint states
        self.arm_state_timer = self.create_timer(0.02, self.publish_arm_joint_states)  # 50Hz

        self.get_logger().info(f'Serial Bridge Node (Unified) started')
        self.get_logger().info(f'  Port: {self.serial_port}')
        self.get_logger().info(f'  Baud: {self.baud_rate}')

    def connect_serial(self):
        """Attempt to connect to Arduino."""
        try:
            with self.serial_lock:
                if self.serial_conn is not None:
                    try:
                        self.serial_conn.close()
                    except:
                        pass

                self.serial_conn = serial.Serial(
                    port=self.serial_port,
                    baudrate=self.baud_rate,
                    timeout=0.1,
                    dsrdtr=False,
                    rtscts=False,
                    exclusive=True  # Prevent multiple access
                )

                # Only reset Arduino on first connection
                if self.first_connect:
                    self.serial_conn.dtr = False
                    time.sleep(0.1)
                    self.serial_conn.dtr = True
                    time.sleep(2.5)  # Wait for Arduino bootloader
                    self.first_connect = False
                else:
                    time.sleep(0.5)  # Short delay for reconnect

                # Clear any buffered data
                self.serial_conn.reset_input_buffer()
                self.serial_conn.reset_output_buffer()

                self.get_logger().info(f'Connected to {self.serial_port}')
                return True

        except serial.SerialException as e:
            self.get_logger().warn(f'Failed to connect to {self.serial_port}: {e}')
            return False

    def send_command(self, command):
        """Send command to Arduino."""
        with self.serial_lock:
            if self.serial_conn is None or not self.serial_conn.is_open:
                return False

            try:
                cmd = f'{command}\n'.encode('utf-8')
                self.serial_conn.write(cmd)
                self.serial_conn.flush()
                return True
            except serial.SerialException as e:
                self.get_logger().error(f'Serial write error: {e}')
                return False

    def cmd_vel_callback(self, msg):
        """Handle velocity command."""
        linear_x = msg.linear.x
        angular_z = msg.angular.z

        # Debug log
        self.get_logger().debug(f'cmd_vel received: linear={linear_x:.3f}, angular={angular_z:.3f}')

        # Send to Arduino
        command = f'VEL,{linear_x:.4f},{angular_z:.4f}'
        result = self.send_command(command)

        if result:
            self.last_cmd_time = time.time()
            self.cmd_sent = True

    def arm_cmd_callback(self, msg):
        """Handle arm joint command."""
        # Convert JointState to servo angles
        servo_angles = list(self.servo_angles)  # Start with current angles

        for i, joint_name in enumerate(msg.name):
            if joint_name in self.joint_names:
                idx = self.joint_names.index(joint_name)
                position_rad = msg.position[i]
                angle_deg = self.joint_to_servo_angle(idx, position_rad)
                servo_angles[idx] = angle_deg
                self.joint_positions[idx] = position_rad

        # Send SET_ALL_SERVOS command
        angles_str = ",".join(str(int(a)) for a in servo_angles)
        command = f"SET_ALL_SERVOS,{angles_str}"
        self.send_command(command)
        self.get_logger().info(f'Arm command: {command}')

    def joint_to_servo_angle(self, joint_idx, position_rad):
        """Convert joint position (rad) to servo angle (0-180 deg)."""
        # Joint limits (from URDF)
        joint_limits = {
            0: (-3.1416, 3.1416),   # joint_1
            1: (-1.5708, 1.5708),   # joint_2
            2: (-1.5708, 1.5708),   # joint_3
            3: (-1.5708, 1.5708),   # joint_4
            4: (-3.1416, 3.1416),   # gripper_base_joint
            5: (0.0, 1.0),          # left_gear_joint
        }
        lower, upper = joint_limits[joint_idx]
        position_rad = np.clip(position_rad, lower, upper)

        if joint_idx == 5:  # Gripper
            angle = 90.0 + (position_rad * 180.0 / math.pi)
        else:
            joint_range = upper - lower
            center = (upper + lower) / 2.0
            angle = 90.0 + ((position_rad - center) / (joint_range / 2.0)) * 90.0

        return int(np.clip(angle, 0, 180))

    def publish_arm_joint_states(self):
        """Publish current arm joint states."""
        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.name = self.joint_names
        msg.position = self.joint_positions
        msg.velocity = [0.0] * 6
        msg.effort = [0.0] * 6
        self.arm_joint_pub.publish(msg)

    def set_test_mode_callback(self, request, response):
        """Handle test mode service request."""
        if request.data:
            # Enable test mode
            if self.send_command('TEST,ON'):
                self.test_mode = True
                response.success = True
                response.message = 'Test mode ENABLED - Arduino simulating encoders'
                self.get_logger().info('Test mode ENABLED')
            else:
                response.success = False
                response.message = 'Failed to send TEST,ON command'
        else:
            # Disable test mode
            if self.send_command('TEST,OFF'):
                self.test_mode = False
                response.success = True
                response.message = 'Test mode DISABLED - using real encoders'
                self.get_logger().info('Test mode DISABLED')
            else:
                response.success = False
                response.message = 'Failed to send TEST,OFF command'

        return response

    def check_cmd_timeout(self):
        """Check if we need to send STOP command due to timeout."""
        if self.cmd_sent and (time.time() - self.last_cmd_time) > self.cmd_timeout:
            # Send STOP command
            self.send_command('STOP')
            self.cmd_sent = False

    def serial_reader_loop(self):
        """Background thread to read serial data from Arduino."""
        error_count = 0
        max_errors = 5  # Only reconnect after multiple consecutive errors

        while self.running:
            # Try to connect if not connected
            if self.serial_conn is None or not self.serial_conn.is_open:
                if not self.connect_serial():
                    time.sleep(self.reconnect_interval)
                    continue
                error_count = 0

            try:
                # Read line from Arduino
                with self.serial_lock:
                    if self.serial_conn.in_waiting > 0:
                        line = self.serial_conn.readline().decode('utf-8', errors='ignore').strip()
                    else:
                        line = None

                if line:
                    self.process_arduino_message(line)
                    error_count = 0  # Reset on successful read
                else:
                    time.sleep(0.01)  # Small delay if no data

            except serial.SerialException as e:
                error_count += 1
                if error_count >= max_errors:
                    self.get_logger().error(f'Serial read error ({error_count}x): {e}')
                    with self.serial_lock:
                        try:
                            self.serial_conn.close()
                        except:
                            pass
                        self.serial_conn = None
                    time.sleep(self.reconnect_interval)
                    error_count = 0
                else:
                    time.sleep(0.1)  # Brief pause before retry

            except Exception as e:
                self.get_logger().error(f'Unexpected error in reader: {e}')
                time.sleep(0.1)

    def process_arduino_message(self, line):
        """Process a message from Arduino."""
        try:
            if line.startswith('ENC,'):
                # Parse encoder counts: ENC,fl,fr,bl,br
                parts = line.split(',')
                if len(parts) == 5:
                    counts = [int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])]

                    msg = Int64MultiArray()
                    msg.data = counts
                    self.encoder_pub.publish(msg)

            elif line.startswith('OK,'):
                # Command acknowledgment
                ack_msg = String()
                ack_msg.data = line
                self.ack_pub.publish(ack_msg)

            elif line.startswith('STATUS,'):
                # Status message
                status = line[7:]  # Remove "STATUS," prefix
                self.get_logger().info(f'Arduino: {status}')

                msg = String()
                msg.data = status
                self.status_pub.publish(msg)

            elif line.startswith('ERROR,'):
                # Error message
                error = line[6:]  # Remove "ERROR," prefix
                self.get_logger().error(f'Arduino error: {error}')

                msg = String()
                msg.data = f'ERROR: {error}'
                self.status_pub.publish(msg)

        except Exception as e:
            self.get_logger().warn(f'Failed to parse Arduino message: {line}, error: {e}')

    def destroy_node(self):
        """Cleanup on shutdown."""
        self.running = False

        # Send stop command
        self.send_command('STOP')

        # Close serial connection
        with self.serial_lock:
            if self.serial_conn is not None:
                try:
                    self.serial_conn.close()
                except:
                    pass

        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = SerialBridgeNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
