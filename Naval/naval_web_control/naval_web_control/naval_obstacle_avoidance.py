#!/usr/bin/env python3
"""
Naval Obstacle Avoidance Node
===============================
State machine obstacle avoidance using YDLiDAR.
Intercepts movement commands and prevents collisions.

Flow: Web UI -> /cmd_vel_raw -> [this node] -> /cmd_vel -> serial_bridge

State Machine:
    DRIVING -> OBSTACLE_DETECTED -> REVERSING -> STOPPED_AFTER_REVERSE
            -> SCANNING -> TURNING -> CHECK_CLEAR -> DRIVING

Subscribes:
    /scan (sensor_msgs/LaserScan): LiDAR data
    /cmd_vel_raw (geometry_msgs/Twist): Raw teleop commands from web UI

Publishes:
    /cmd_vel (geometry_msgs/Twist): Filtered velocity commands
    /naval/obstacle_status (std_msgs/String): State for web UI

Usage:
    ros2 run naval_web_control obstacle_avoidance
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import LaserScan
from geometry_msgs.msg import Twist
from std_msgs.msg import String, Bool
from enum import Enum
import math


class AvoidanceState(Enum):
    DRIVING = "driving"
    OBSTACLE_DETECTED = "obstacle_detected"
    REVERSING = "reversing"
    STOPPED_AFTER_REVERSE = "stopped"
    SCANNING = "scanning"
    TURNING = "turning"
    CHECK_CLEAR = "check_clear"


class NavalObstacleAvoidance(Node):

    def __init__(self):
        super().__init__('naval_obstacle_avoidance')

        # Parameters
        self.declare_parameter('obstacle_threshold', 0.20)
        self.declare_parameter('clear_threshold', 0.30)
        self.declare_parameter('front_angle', 30.0)
        self.declare_parameter('reverse_speed', -0.10)
        self.declare_parameter('reverse_distance', 0.12)
        self.declare_parameter('turn_speed_max', 0.4)
        self.declare_parameter('turn_speed_min', 0.15)
        self.declare_parameter('turn_proportional_gain', 0.5)
        self.declare_parameter('stop_duration', 0.1)
        self.declare_parameter('pause_after_reverse', 0.2)
        self.declare_parameter('scan_duration', 0.3)
        self.declare_parameter('turn_min_duration', 0.3)
        self.declare_parameter('turn_max_duration', 5.0)
        self.declare_parameter('max_avoidance_attempts', 5)
        self.declare_parameter('reverse_timeout', 2.0)
        self.declare_parameter('lidar_timeout', 0.5)
        # LiDAR is inside robot body, 0° faces body wall, 180° faces
        # out through cutout. Offset rotates scan so robot front = 0°.
        self.declare_parameter('lidar_angle_offset', 165.0)
        # Minimum range to ignore (cutout walls at ~15cm)
        self.declare_parameter('min_valid_range', 0.18)

        self._load_parameters()

        # State machine
        self.state = AvoidanceState.DRIVING
        self.state_start_time = self.get_clock().now()
        self.avoidance_attempts = 0

        # Turn tracking
        self.turn_direction = 0
        self.turn_speed = 0.0

        # Scan data
        self.min_front_distance = float('inf')
        self.left_sector_avg = float('inf')
        self.right_sector_avg = float('inf')
        self.last_scan_time = None

        # Enabled flag - starts DISABLED, web UI toggles ON when needed
        self.enabled = False

        # Teleop command
        self.latest_cmd = Twist()

        # QoS for LiDAR - MUST be Best Effort for YDLiDAR
        scan_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            depth=5
        )

        # Subscribers
        self.scan_sub = self.create_subscription(
            LaserScan, '/scan', self.scan_callback, scan_qos)
        self.cmd_sub = self.create_subscription(
            Twist, '/cmd_vel_raw', self.cmd_callback, 10)
        self.enable_sub = self.create_subscription(
            Bool, '/naval/obstacle_enabled', self.enable_callback, 10)

        # Publishers
        self.cmd_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.status_pub = self.create_publisher(
            String, '/naval/obstacle_status', 10)

        # Timers
        self.control_timer = self.create_timer(0.1, self.control_loop)
        self.status_timer = self.create_timer(0.5, self.publish_status)

        self.get_logger().info(
            f'Naval Obstacle Avoidance started\n'
            f'  Obstacle threshold: {self.obstacle_threshold}m\n'
            f'  Clear threshold: {self.clear_threshold}m\n'
            f'  Front sector: +/-{math.degrees(self.front_angle):.0f} deg\n'
            f'  LiDAR offset: {math.degrees(self.lidar_angle_offset):.0f} deg\n'
            f'  Min valid range: {self.min_valid_range}m (cutout filter)')

    def _load_parameters(self):
        self.obstacle_threshold = self.get_parameter('obstacle_threshold').value
        self.clear_threshold = self.get_parameter('clear_threshold').value
        self.front_angle = math.radians(
            self.get_parameter('front_angle').value)
        self.reverse_speed = self.get_parameter('reverse_speed').value
        self.reverse_distance = self.get_parameter('reverse_distance').value
        self.turn_speed_max = self.get_parameter('turn_speed_max').value
        self.turn_speed_min = self.get_parameter('turn_speed_min').value
        self.turn_proportional_gain = self.get_parameter(
            'turn_proportional_gain').value
        self.stop_duration = self.get_parameter('stop_duration').value
        self.pause_after_reverse = self.get_parameter(
            'pause_after_reverse').value
        self.scan_duration = self.get_parameter('scan_duration').value
        self.turn_min_duration = self.get_parameter('turn_min_duration').value
        self.turn_max_duration = self.get_parameter('turn_max_duration').value
        self.max_avoidance_attempts = self.get_parameter(
            'max_avoidance_attempts').value
        self.reverse_timeout = self.get_parameter('reverse_timeout').value
        self.lidar_timeout = self.get_parameter('lidar_timeout').value
        self.lidar_angle_offset = math.radians(
            self.get_parameter('lidar_angle_offset').value)
        self.min_valid_range = self.get_parameter('min_valid_range').value

    # === Scan Processing ===

    def _normalize_angle(self, angle):
        """Normalize angle to [-pi, pi]."""
        while angle > math.pi:
            angle -= 2 * math.pi
        while angle < -math.pi:
            angle += 2 * math.pi
        return angle

    def scan_callback(self, msg):
        self.last_scan_time = self.get_clock().now()
        front_readings = []
        left_readings = []
        right_readings = []

        left_start = math.radians(-90.0)
        left_end = math.radians(-30.0)
        right_start = math.radians(30.0)
        right_end = math.radians(90.0)

        for i, distance in enumerate(msg.ranges):
            if not math.isfinite(distance):
                continue
            if distance < self.min_valid_range:
                continue
            if distance > msg.range_max:
                continue
            # Apply angle offset: LiDAR is mounted so robot front
            # is at lidar_angle_offset degrees in LiDAR frame
            raw_angle = msg.angle_min + i * msg.angle_increment
            angle = self._normalize_angle(raw_angle - self.lidar_angle_offset)

            if abs(angle) <= self.front_angle:
                front_readings.append(distance)
            if left_start <= angle <= left_end:
                left_readings.append(distance)
            if right_start <= angle <= right_end:
                right_readings.append(distance)

        self.min_front_distance = (
            min(front_readings) if front_readings else float('inf'))
        self.left_sector_avg = (
            sum(left_readings) / len(left_readings)
            if left_readings else float('inf'))
        self.right_sector_avg = (
            sum(right_readings) / len(right_readings)
            if right_readings else float('inf'))

    def _is_scan_valid(self):
        if self.last_scan_time is None:
            return False
        elapsed = (
            self.get_clock().now() - self.last_scan_time).nanoseconds / 1e9
        return elapsed < self.lidar_timeout

    # === State Machine ===

    def control_loop(self):
        if not self.enabled:
            # Disabled: pure passthrough, no obstacle checking
            self.cmd_pub.publish(self.latest_cmd)
            return
        if not self._is_scan_valid():
            if self.state != AvoidanceState.DRIVING:
                self._publish_stop()
            else:
                # No lidar: pass through but block forward
                cmd = Twist()
                if self.latest_cmd.linear.x <= 0:
                    cmd = self.latest_cmd
                else:
                    cmd.angular.z = self.latest_cmd.angular.z
                self.cmd_pub.publish(cmd)
            return

        handler = {
            AvoidanceState.DRIVING: self._state_driving,
            AvoidanceState.OBSTACLE_DETECTED: self._state_obstacle_detected,
            AvoidanceState.REVERSING: self._state_reversing,
            AvoidanceState.STOPPED_AFTER_REVERSE: self._state_stopped_after_reverse,
            AvoidanceState.SCANNING: self._state_scanning,
            AvoidanceState.TURNING: self._state_turning,
            AvoidanceState.CHECK_CLEAR: self._state_check_clear,
        }
        handler[self.state]()

    def _transition_to(self, new_state):
        old = self.state
        self.state = new_state
        self.state_start_time = self.get_clock().now()
        self.get_logger().info(f'State: {old.value} -> {new_state.value}')

    def _time_in_state(self):
        return (
            self.get_clock().now() - self.state_start_time).nanoseconds / 1e9

    # === State Handlers ===

    def _state_driving(self):
        if self.min_front_distance < self.obstacle_threshold:
            emergency = self.obstacle_threshold * 0.7
            if (self.latest_cmd.linear.x > 0
                    or self.min_front_distance < emergency):
                self.get_logger().warn(
                    f'OBSTACLE at {self.min_front_distance:.2f}m')
                self.avoidance_attempts = 0
                self._transition_to(AvoidanceState.OBSTACLE_DETECTED)
                self._publish_stop()
                return
        self.cmd_pub.publish(self.latest_cmd)

    def _state_obstacle_detected(self):
        self._publish_stop()
        if self._time_in_state() >= self.stop_duration:
            self._transition_to(AvoidanceState.REVERSING)

    def _state_reversing(self):
        elapsed = self._time_in_state()
        expected_distance = abs(self.reverse_speed) * elapsed
        if (expected_distance >= self.reverse_distance
                or elapsed >= self.reverse_timeout):
            self._publish_stop()
            self._transition_to(AvoidanceState.STOPPED_AFTER_REVERSE)
        else:
            cmd = Twist()
            cmd.linear.x = self.reverse_speed
            self.cmd_pub.publish(cmd)

    def _state_stopped_after_reverse(self):
        self._publish_stop()
        if self._time_in_state() >= self.pause_after_reverse:
            self._transition_to(AvoidanceState.SCANNING)

    def _state_scanning(self):
        if self._time_in_state() < self.scan_duration:
            self._publish_stop()
            return

        left_clear = self.left_sector_avg
        right_clear = self.right_sector_avg
        self.get_logger().info(
            f'Scan: Left={left_clear:.2f}m, Right={right_clear:.2f}m')

        if left_clear > right_clear:
            self.turn_direction = 1
        else:
            self.turn_direction = -1

        clearance_diff = abs(left_clear - right_clear)
        self.turn_speed = self.turn_speed_min + \
            (self.turn_speed_max - self.turn_speed_min) * \
            min(clearance_diff * self.turn_proportional_gain, 1.0)
        self._transition_to(AvoidanceState.TURNING)

    def _state_turning(self):
        elapsed = self._time_in_state()
        if elapsed >= self.turn_max_duration:
            self._publish_stop()
            self._transition_to(AvoidanceState.CHECK_CLEAR)
            return
        if elapsed >= self.turn_min_duration:
            if self.min_front_distance >= self.clear_threshold:
                self._publish_stop()
                self._transition_to(AvoidanceState.CHECK_CLEAR)
                return
        if (elapsed >= 1.0
                and self.min_front_distance < self.obstacle_threshold * 0.5):
            self._publish_stop()
            self._transition_to(AvoidanceState.CHECK_CLEAR)
            return

        cmd = Twist()
        cmd.angular.z = self.turn_direction * self.turn_speed
        self.cmd_pub.publish(cmd)

    def _state_check_clear(self):
        self._publish_stop()
        if self._time_in_state() < 0.2:
            return
        if self.min_front_distance >= self.clear_threshold:
            self.get_logger().info('Path CLEAR - resuming')
            self.avoidance_attempts = 0
            self._transition_to(AvoidanceState.DRIVING)
        else:
            self.avoidance_attempts += 1
            if self.avoidance_attempts >= self.max_avoidance_attempts:
                self.get_logger().warn('Max attempts reached')
                self._transition_to(AvoidanceState.DRIVING)
            else:
                self._transition_to(AvoidanceState.SCANNING)

    # === Utility ===

    def _publish_stop(self):
        self.cmd_pub.publish(Twist())

    def cmd_callback(self, msg):
        self.latest_cmd = msg

    def enable_callback(self, msg):
        if self.enabled != msg.data:
            self.enabled = msg.data
            self.get_logger().info(
                f'Obstacle avoidance {"ENABLED" if self.enabled else "DISABLED"}')
            if self.enabled:
                self.state = AvoidanceState.DRIVING
                self.avoidance_attempts = 0

    def publish_status(self):
        msg = String()
        msg.data = (
            f'state:{self.state.value},'
            f'front:{self.min_front_distance:.2f},'
            f'left:{self.left_sector_avg:.2f},'
            f'right:{self.right_sector_avg:.2f},'
            f'attempts:{self.avoidance_attempts}')
        self.status_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = NavalObstacleAvoidance()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node._publish_stop()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
