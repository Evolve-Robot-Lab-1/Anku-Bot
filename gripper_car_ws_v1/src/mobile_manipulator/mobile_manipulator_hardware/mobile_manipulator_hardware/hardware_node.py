#!/usr/bin/env python3
"""
Unified Mobile Manipulator Hardware Interface
==============================================

ROS2 node that bridges the entire robot to the unified Arduino firmware.
Handles both arm servo control and mobile base velocity/encoder feedback.

Topics:
  Subscribe:
    - /arm/joint_commands (sensor_msgs/JointState): Arm joint position commands
    - /cmd_vel (geometry_msgs/Twist): Base velocity commands

  Publish:
    - /joint_states (sensor_msgs/JointState): All joint states (arm + base wheels)
    - /odom (nav_msgs/Odometry): Odometry from wheel encoders
    - /hardware/status (std_msgs/String): Hardware status messages

  Broadcast TF:
    - odom -> body_link

Parameters:
  - serial_port: Arduino serial port (default: /dev/ttyACM0)
  - baud_rate: Serial baud rate (default: 115200)
  - See config/hardware_params.yaml for full parameter list
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import JointState
from geometry_msgs.msg import Twist, TransformStamped, Quaternion
from nav_msgs.msg import Odometry
from std_msgs.msg import String
from tf2_ros import TransformBroadcaster

import serial
import threading
import math
import time


def quaternion_from_euler(roll, pitch, yaw):
    """Convert Euler angles to quaternion."""
    cy = math.cos(yaw * 0.5)
    sy = math.sin(yaw * 0.5)
    cp = math.cos(pitch * 0.5)
    sp = math.sin(pitch * 0.5)
    cr = math.cos(roll * 0.5)
    sr = math.sin(roll * 0.5)

    q = Quaternion()
    q.w = cr * cp * cy + sr * sp * sy
    q.x = sr * cp * cy - cr * sp * sy
    q.y = cr * sp * cy + sr * cp * sy
    q.z = cr * cp * sy - sr * sp * cy
    return q


class MobileManipulatorHardware(Node):
    """Unified hardware interface for mobile manipulator."""

    def __init__(self):
        super().__init__('hardware_node')

        # ===== PARAMETERS =====
        self.declare_parameter('serial_port', '/dev/ttyACM0')
        self.declare_parameter('baud_rate', 115200)

        # Arm parameters
        self.declare_parameter('arm_joints', [
            'joint_1', 'joint_2', 'joint_3',
            'joint_4', 'gripper_base_joint', 'left_gear_joint'
        ])

        # Wheel parameters
        self.declare_parameter('wheel_joints', [
            'front_left_joint', 'front_right_joint',
            'back_left_joint', 'back_right_joint'
        ])
        self.declare_parameter('wheel_radius', 0.075)
        self.declare_parameter('wheel_base', 0.35)
        self.declare_parameter('encoder_cpr', 360)

        # Frame parameters
        self.declare_parameter('base_frame', 'body_link')
        self.declare_parameter('odom_frame', 'odom')
        self.declare_parameter('publish_odom_tf', True)

        # Rate parameters
        self.declare_parameter('joint_state_rate', 50.0)

        # Covariance
        self.declare_parameter('pose_covariance_diagonal',
                               [0.01, 0.01, 0.01, 0.01, 0.01, 0.03])
        self.declare_parameter('twist_covariance_diagonal',
                               [0.01, 0.01, 0.01, 0.01, 0.01, 0.03])

        # Get parameters
        self.serial_port = self.get_parameter('serial_port').value
        self.baud_rate = self.get_parameter('baud_rate').value
        self.arm_joints = self.get_parameter('arm_joints').value
        self.wheel_joints = self.get_parameter('wheel_joints').value
        self.wheel_radius = self.get_parameter('wheel_radius').value
        self.wheel_base = self.get_parameter('wheel_base').value
        self.encoder_cpr = self.get_parameter('encoder_cpr').value
        self.base_frame = self.get_parameter('base_frame').value
        self.odom_frame = self.get_parameter('odom_frame').value
        self.publish_odom_tf = self.get_parameter('publish_odom_tf').value
        self.joint_state_rate = self.get_parameter('joint_state_rate').value
        self.pose_cov_diag = self.get_parameter('pose_covariance_diagonal').value
        self.twist_cov_diag = self.get_parameter('twist_covariance_diagonal').value

        # ===== ARM STATE =====
        # Joint limits (radians) - from URDF
        self.arm_joint_limits = {
            'joint_1': (-3.1416, 3.1416),
            'joint_2': (-1.5708, 1.5708),
            'joint_3': (-1.5708, 1.5708),
            'joint_4': (-1.5708, 1.5708),
            'gripper_base_joint': (-3.1416, 3.1416),
            'left_gear_joint': (0.0, 1.0),
        }

        # Mimic joints for gripper
        self.mimic_joints = {
            'right_gear_joint': ('left_gear_joint', -1.0),
            'left_finger_joint': ('left_gear_joint', 1.0),
            'right_finger_joint': ('left_gear_joint', 1.0),
            'left_joint': ('left_gear_joint', -1.0),
            'right_joint': ('left_gear_joint', 1.0),
        }

        self.arm_positions = [0.0] * 6  # Current arm joint positions (radians)
        self.arm_target_angles = [90.0] * 6  # Target servo angles (degrees)

        # ===== BASE STATE =====
        # Odometry state
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.vx = 0.0
        self.vth = 0.0

        # Encoder state
        self.wheel_positions = [0.0] * 4  # radians
        self.last_encoder_counts = None
        self.last_odom_time = None

        # Calculated constants
        self.meters_per_tick = (2.0 * math.pi * self.wheel_radius) / self.encoder_cpr
        self.radians_per_tick = (2.0 * math.pi) / self.encoder_cpr

        # ===== SERIAL COMMUNICATION =====
        self.serial = None
        self.serial_lock = threading.Lock()
        self.connected = False
        self.running = True

        # ===== QOS =====
        qos_reliable = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=10
        )

        # ===== PUBLISHERS =====
        self.pub_joint_states = self.create_publisher(
            JointState, '/joint_states', qos_reliable)
        self.pub_odom = self.create_publisher(
            Odometry, '/odom', qos_reliable)
        self.pub_status = self.create_publisher(
            String, '/hardware/status', 10)

        # ===== SUBSCRIBERS =====
        self.sub_arm_cmd = self.create_subscription(
            JointState, '/arm/joint_commands',
            self.arm_cmd_callback, qos_reliable)
        self.sub_cmd_vel = self.create_subscription(
            Twist, '/cmd_vel',
            self.cmd_vel_callback, qos_reliable)
        self.sub_raw_cmd = self.create_subscription(
            String, '/hardware/command',
            self.raw_command_callback, 10)

        # ===== TF BROADCASTER =====
        if self.publish_odom_tf:
            self.tf_broadcaster = TransformBroadcaster(self)

        # ===== CONNECT TO ARDUINO =====
        self.connect_serial()

        # Start serial reader thread
        self.reader_thread = threading.Thread(target=self.serial_loop, daemon=True)
        self.reader_thread.start()

        # Timer for publishing joint states
        self.timer = self.create_timer(
            1.0 / self.joint_state_rate,
            self.publish_joint_states)

        self.get_logger().info('Mobile Manipulator Hardware Node initialized')
        self.get_logger().info(f'  Serial port: {self.serial_port}')
        self.get_logger().info(f'  Arm joints: {self.arm_joints}')
        self.get_logger().info(f'  Wheel joints: {self.wheel_joints}')

    def connect_serial(self):
        """Connect to Arduino via serial port."""
        try:
            self.serial = serial.Serial(
                self.serial_port,
                self.baud_rate,
                timeout=1.0
            )
            time.sleep(2.0)  # Wait for Arduino reset
            self.connected = True
            self.get_logger().info(f'Connected to Arduino at {self.serial_port}')

            # Publish status
            status = String()
            status.data = 'CONNECTED'
            self.pub_status.publish(status)

        except Exception as e:
            self.get_logger().error(f'Serial connection failed: {e}')
            self.connected = False

    def arm_cmd_callback(self, msg: JointState):
        """Handle arm joint position commands."""
        if not self.connected:
            return

        # Convert joint positions to servo angles
        servo_angles = list(self.arm_target_angles)  # Start with current targets

        for i, joint_name in enumerate(msg.name):
            if joint_name in self.arm_joints:
                idx = self.arm_joints.index(joint_name)
                position_rad = msg.position[i]

                # Convert to servo angle
                angle_deg = self.joint_to_servo_angle(idx, position_rad)
                servo_angles[idx] = angle_deg

                # Update internal state
                self.arm_positions[idx] = position_rad

        # Send ARM command to Arduino
        angles_str = ",".join(f"{a:.1f}" for a in servo_angles)
        command = f"ARM,{angles_str}"
        self.send_command(command)

        # Update target angles
        self.arm_target_angles = servo_angles

    def cmd_vel_callback(self, msg: Twist):
        """Handle velocity commands for mobile base."""
        if not self.connected:
            return

        linear_x = msg.linear.x
        angular_z = msg.angular.z

        # Send VEL command to Arduino
        command = f"VEL,{linear_x:.3f},{angular_z:.3f}"
        self.send_command(command)

    def raw_command_callback(self, msg: String):
        """Forward raw commands to Arduino (for testing/debugging)."""
        if not self.connected:
            return

        command = msg.data.strip()
        self.get_logger().info(f'Sending raw command: {command}')
        self.send_command(command)

    def send_command(self, command: str) -> bool:
        """Send command to Arduino."""
        if not self.connected or not self.serial:
            return False

        try:
            with self.serial_lock:
                self.serial.write((command + '\n').encode())
                self.serial.flush()
            return True
        except Exception as e:
            self.get_logger().warn(f'Serial write error: {e}')
            return False

    def serial_loop(self):
        """Background serial reader thread."""
        while self.running and self.serial:
            try:
                with self.serial_lock:
                    if self.serial.in_waiting > 0:
                        line = self.serial.readline().decode().strip()
                        if line:
                            self.process_message(line)
            except Exception as e:
                self.get_logger().error(f'Serial read error: {e}')
            time.sleep(0.005)  # 200 Hz check rate

    def process_message(self, message: str):
        """Process messages from Arduino."""
        # ARM_POS,<6 values> - arm servo positions
        if message.startswith('ARM_POS,'):
            parts = message.split(',')
            if len(parts) == 7:
                try:
                    servo_angles = [float(x) for x in parts[1:7]]
                    self.update_arm_positions(servo_angles)
                except ValueError:
                    pass

        # ENC,<fl>,<fr>,<bl>,<br> - encoder ticks
        elif message.startswith('ENC,'):
            parts = message.split(',')
            if len(parts) == 5:
                try:
                    counts = [int(x) for x in parts[1:5]]
                    self.update_odometry(counts)
                except ValueError:
                    pass

        # STATUS,<message>
        elif message.startswith('STATUS,'):
            status = String()
            status.data = message
            self.pub_status.publish(status)
            self.get_logger().info(f'Arduino: {message}')

        # HEARTBEAT,<millis>
        elif message.startswith('HEARTBEAT,'):
            pass  # Heartbeat received, connection is alive

        # OK,<command> - acknowledgment
        elif message.startswith('OK,'):
            self.get_logger().debug(f'Arduino ACK: {message}')

        # ERROR,<message>
        elif message.startswith('ERROR,'):
            self.get_logger().warn(f'Arduino error: {message}')

    def update_arm_positions(self, servo_angles: list):
        """Convert servo angles to joint positions."""
        for i, deg in enumerate(servo_angles):
            self.arm_positions[i] = self.servo_to_joint_angle(i, deg)

    def update_odometry(self, counts: list):
        """Update odometry from encoder counts."""
        current_time = self.get_clock().now()

        # First message - just store and return
        if self.last_encoder_counts is None:
            self.last_encoder_counts = counts
            self.last_odom_time = current_time
            return

        # Calculate time delta
        dt_ns = (current_time - self.last_odom_time).nanoseconds
        dt = dt_ns / 1e9

        if dt <= 0:
            return

        # Calculate encoder deltas
        delta_counts = [counts[i] - self.last_encoder_counts[i] for i in range(4)]

        # Average left and right sides
        left_delta = (delta_counts[0] + delta_counts[2]) / 2.0
        right_delta = (delta_counts[1] + delta_counts[3]) / 2.0

        # Convert to distance
        left_dist = left_delta * self.meters_per_tick
        right_dist = right_delta * self.meters_per_tick

        # Differential drive kinematics
        distance = (left_dist + right_dist) / 2.0
        delta_theta = (right_dist - left_dist) / self.wheel_base

        # Update pose
        if abs(delta_theta) < 1e-6:
            self.x += distance * math.cos(self.theta)
            self.y += distance * math.sin(self.theta)
        else:
            radius = distance / delta_theta
            self.x += radius * (math.sin(self.theta + delta_theta) - math.sin(self.theta))
            self.y += -radius * (math.cos(self.theta + delta_theta) - math.cos(self.theta))

        self.theta += delta_theta
        self.theta = math.atan2(math.sin(self.theta), math.cos(self.theta))

        # Calculate velocities
        self.vx = distance / dt
        self.vth = delta_theta / dt

        # Update wheel positions
        for i in range(4):
            self.wheel_positions[i] += delta_counts[i] * self.radians_per_tick

        # Store for next iteration
        self.last_encoder_counts = counts
        self.last_odom_time = current_time

        # Publish odometry
        self.publish_odometry(current_time)

    def publish_odometry(self, stamp):
        """Publish odometry message and TF."""
        q = quaternion_from_euler(0.0, 0.0, self.theta)

        # Create odometry message
        odom = Odometry()
        odom.header.stamp = stamp.to_msg()
        odom.header.frame_id = self.odom_frame
        odom.child_frame_id = self.base_frame

        # Pose
        odom.pose.pose.position.x = self.x
        odom.pose.pose.position.y = self.y
        odom.pose.pose.position.z = 0.0
        odom.pose.pose.orientation = q

        # Pose covariance
        odom.pose.covariance = [0.0] * 36
        for i, v in enumerate(self.pose_cov_diag):
            odom.pose.covariance[i * 7] = v

        # Twist
        odom.twist.twist.linear.x = self.vx
        odom.twist.twist.angular.z = self.vth

        # Twist covariance
        odom.twist.covariance = [0.0] * 36
        for i, v in enumerate(self.twist_cov_diag):
            odom.twist.covariance[i * 7] = v

        self.pub_odom.publish(odom)

        # Publish TF
        if self.publish_odom_tf:
            t = TransformStamped()
            t.header.stamp = stamp.to_msg()
            t.header.frame_id = self.odom_frame
            t.child_frame_id = self.base_frame
            t.transform.translation.x = self.x
            t.transform.translation.y = self.y
            t.transform.translation.z = 0.0
            t.transform.rotation = q

            self.tf_broadcaster.sendTransform(t)

    def publish_joint_states(self):
        """Publish all joint states."""
        stamp = self.get_clock().now()

        js = JointState()
        js.header.stamp = stamp.to_msg()

        # Arm joints
        all_names = list(self.arm_joints)
        all_positions = list(self.arm_positions)

        # Mimic joints
        for mimic_name, (parent_name, multiplier) in self.mimic_joints.items():
            parent_idx = self.arm_joints.index(parent_name)
            mimic_pos = self.arm_positions[parent_idx] * multiplier
            all_names.append(mimic_name)
            all_positions.append(mimic_pos)

        # Wheel joints
        all_names.extend(self.wheel_joints)
        all_positions.extend(self.wheel_positions)

        js.name = all_names
        js.position = all_positions
        js.velocity = [0.0] * len(all_names)
        js.effort = [0.0] * len(all_names)

        self.pub_joint_states.publish(js)

    def joint_to_servo_angle(self, joint_idx: int, position_rad: float) -> float:
        """Convert joint position (rad) to servo angle (0-180 deg)."""
        joint_name = self.arm_joints[joint_idx]
        lower, upper = self.arm_joint_limits[joint_name]

        # Clamp to limits
        position_rad = max(lower, min(upper, position_rad))

        if joint_idx == 5:  # Gripper gear
            # Servo 90° = closed (0 rad), Servo 135° = open (pi/4 rad)
            angle = 90.0 + (position_rad * 180.0 / math.pi)
        else:
            # General mapping: joint center -> servo 90°
            joint_range = upper - lower
            center = (upper + lower) / 2.0
            angle = 90.0 + ((position_rad - center) / (joint_range / 2.0)) * 90.0

        return max(0.0, min(180.0, angle))

    def servo_to_joint_angle(self, joint_idx: int, angle_deg: float) -> float:
        """Convert servo angle (0-180 deg) to joint position (rad)."""
        joint_name = self.arm_joints[joint_idx]
        lower, upper = self.arm_joint_limits[joint_name]

        if joint_idx == 5:  # Gripper gear
            position_rad = (angle_deg - 90.0) * (math.pi / 180.0)
        else:
            joint_range = upper - lower
            center = (upper + lower) / 2.0
            position_rad = center + ((angle_deg - 90.0) / 90.0) * (joint_range / 2.0)

        return max(lower, min(upper, position_rad))

    def destroy_node(self):
        """Cleanup on shutdown."""
        self.running = False

        # Send stop command
        if self.connected:
            self.send_command("STOP")

        if self.serial:
            self.serial.close()

        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = MobileManipulatorHardware()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
