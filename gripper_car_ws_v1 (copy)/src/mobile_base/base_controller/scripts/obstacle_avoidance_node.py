#!/usr/bin/env python3
"""
Smart Turn Obstacle Avoidance Node
==================================
State machine-based obstacle avoidance with autonomous recovery.

State Machine:
    DRIVING -> OBSTACLE_DETECTED -> REVERSING -> STOPPED_AFTER_REVERSE
            -> SCANNING -> TURNING -> CHECK_CLEAR -> DRIVING

Subscribes:
    - /scan (sensor_msgs/LaserScan): LiDAR data
    - /cmd_vel_raw (geometry_msgs/Twist): Raw teleop commands

Publishes:
    - /cmd_vel (geometry_msgs/Twist): Filtered/generated velocity commands
    - /obstacle_avoidance/status (std_msgs/String): Current state for debugging
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import LaserScan
from geometry_msgs.msg import Twist
from std_msgs.msg import String
from enum import Enum
import math


class AvoidanceState(Enum):
    """Obstacle avoidance state machine states."""
    DRIVING = "driving"
    OBSTACLE_DETECTED = "obstacle_detected"
    REVERSING = "reversing"
    STOPPED_AFTER_REVERSE = "stopped"
    SCANNING = "scanning"
    TURNING = "turning"
    CHECK_CLEAR = "check_clear"


class SmartTurnObstacleAvoidance(Node):
    """State machine obstacle avoidance with smart turning."""

    def __init__(self):
        super().__init__('obstacle_avoidance')

        # === Declare all parameters ===
        self._declare_parameters()
        self._load_parameters()

        # === State machine variables ===
        self.state = AvoidanceState.DRIVING
        self.state_start_time = self.get_clock().now()
        self.avoidance_attempts = 0

        # === Motion tracking ===
        self.turn_direction = 0  # -1 for right, +1 for left
        self.turn_speed = 0.0

        # === Scan data storage ===
        self.latest_scan = None
        self.last_scan_time = None
        self.min_front_distance = float('inf')
        self.left_sector_avg = float('inf')
        self.right_sector_avg = float('inf')

        # === Teleop command storage ===
        self.latest_cmd = Twist()

        # === Setup communications ===
        self._setup_communications()

        self.get_logger().info(
            f'Smart Turn Obstacle Avoidance initialized\n'
            f'  Obstacle threshold: {self.obstacle_threshold}m\n'
            f'  Clear threshold: {self.clear_threshold}m\n'
            f'  Front sector: +/-{math.degrees(self.front_angle):.0f} deg\n'
            f'  Reverse: {abs(self.reverse_speed)}m/s for {self.reverse_distance}m'
        )

    def _declare_parameters(self):
        """Declare all ROS parameters."""
        # Detection thresholds
        self.declare_parameter('obstacle_threshold', 0.20)
        self.declare_parameter('clear_threshold', 0.30)
        self.declare_parameter('side_scan_threshold', 0.25)

        # Angular sectors (degrees)
        self.declare_parameter('front_angle', 30.0)
        self.declare_parameter('left_sector_start', -90.0)
        self.declare_parameter('left_sector_end', -30.0)
        self.declare_parameter('right_sector_start', 30.0)
        self.declare_parameter('right_sector_end', 90.0)

        # Motion parameters
        self.declare_parameter('reverse_speed', -0.10)
        self.declare_parameter('reverse_distance', 0.12)
        self.declare_parameter('turn_speed_max', 0.4)
        self.declare_parameter('turn_speed_min', 0.15)
        self.declare_parameter('turn_proportional_gain', 0.5)

        # Timing parameters (seconds)
        self.declare_parameter('stop_duration', 0.1)
        self.declare_parameter('pause_after_reverse', 0.2)
        self.declare_parameter('scan_duration', 0.3)
        self.declare_parameter('turn_min_duration', 0.3)
        self.declare_parameter('turn_max_duration', 5.0)

        # Safety limits
        self.declare_parameter('max_avoidance_attempts', 5)
        self.declare_parameter('reverse_timeout', 2.0)
        self.declare_parameter('lidar_timeout', 0.5)

        # Topics
        self.declare_parameter('scan_topic', '/scan')
        self.declare_parameter('cmd_vel_in', '/cmd_vel_raw')
        self.declare_parameter('cmd_vel_out', '/cmd_vel')

    def _load_parameters(self):
        """Load parameter values into instance variables."""
        # Detection thresholds
        self.obstacle_threshold = self.get_parameter('obstacle_threshold').value
        self.clear_threshold = self.get_parameter('clear_threshold').value
        self.side_scan_threshold = self.get_parameter('side_scan_threshold').value

        # Angular sectors (convert to radians)
        self.front_angle = math.radians(self.get_parameter('front_angle').value)
        self.left_sector_start = math.radians(self.get_parameter('left_sector_start').value)
        self.left_sector_end = math.radians(self.get_parameter('left_sector_end').value)
        self.right_sector_start = math.radians(self.get_parameter('right_sector_start').value)
        self.right_sector_end = math.radians(self.get_parameter('right_sector_end').value)

        # Motion parameters
        self.reverse_speed = self.get_parameter('reverse_speed').value
        self.reverse_distance = self.get_parameter('reverse_distance').value
        self.turn_speed_max = self.get_parameter('turn_speed_max').value
        self.turn_speed_min = self.get_parameter('turn_speed_min').value
        self.turn_proportional_gain = self.get_parameter('turn_proportional_gain').value

        # Timing parameters
        self.stop_duration = self.get_parameter('stop_duration').value
        self.pause_after_reverse = self.get_parameter('pause_after_reverse').value
        self.scan_duration = self.get_parameter('scan_duration').value
        self.turn_min_duration = self.get_parameter('turn_min_duration').value
        self.turn_max_duration = self.get_parameter('turn_max_duration').value

        # Safety limits
        self.max_avoidance_attempts = self.get_parameter('max_avoidance_attempts').value
        self.reverse_timeout = self.get_parameter('reverse_timeout').value
        self.lidar_timeout = self.get_parameter('lidar_timeout').value

    def _setup_communications(self):
        """Setup subscribers, publishers, and timers."""
        # QoS for LiDAR - MUST be Best Effort for YDLiDAR compatibility
        scan_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            depth=5
        )

        scan_topic = self.get_parameter('scan_topic').value
        cmd_in = self.get_parameter('cmd_vel_in').value
        cmd_out = self.get_parameter('cmd_vel_out').value

        # Subscribers
        self.scan_sub = self.create_subscription(
            LaserScan, scan_topic, self.scan_callback, scan_qos
        )
        self.cmd_sub = self.create_subscription(
            Twist, cmd_in, self.cmd_callback, 10
        )

        # Publishers
        self.cmd_pub = self.create_publisher(Twist, cmd_out, 10)
        self.status_pub = self.create_publisher(String, '/obstacle_avoidance/status', 10)

        # Main control loop timer (10 Hz)
        self.control_timer = self.create_timer(0.1, self.control_loop)

        # Status publishing timer (2 Hz)
        self.status_timer = self.create_timer(0.5, self.publish_status)

    # =========================================================================
    # SCAN PROCESSING
    # =========================================================================

    def scan_callback(self, msg: LaserScan):
        """Process incoming LiDAR scan data."""
        self.latest_scan = msg
        self.last_scan_time = self.get_clock().now()
        self._analyze_scan_sectors(msg)

    def _analyze_scan_sectors(self, msg: LaserScan):
        """Analyze scan data for front, left, and right sectors."""
        front_readings = []
        left_readings = []
        right_readings = []

        num_samples = len(msg.ranges)
        if num_samples == 0:
            return

        for i, distance in enumerate(msg.ranges):
            # Skip invalid readings
            if not (msg.range_min < distance < msg.range_max):
                continue
            if not math.isfinite(distance):
                continue

            angle = msg.angle_min + i * msg.angle_increment

            # Front sector (±front_angle from forward)
            if abs(angle) <= self.front_angle:
                front_readings.append(distance)

            # Left sector (negative angles in ROS convention)
            if self.left_sector_start <= angle <= self.left_sector_end:
                left_readings.append(distance)

            # Right sector (positive angles)
            if self.right_sector_start <= angle <= self.right_sector_end:
                right_readings.append(distance)

        # Calculate minimum/average values
        self.min_front_distance = min(front_readings) if front_readings else float('inf')
        self.left_sector_avg = (sum(left_readings) / len(left_readings)) if left_readings else 0.0
        self.right_sector_avg = (sum(right_readings) / len(right_readings)) if right_readings else 0.0

    def _is_scan_valid(self) -> bool:
        """Check if LiDAR data is fresh."""
        if self.last_scan_time is None:
            return False
        elapsed = (self.get_clock().now() - self.last_scan_time).nanoseconds / 1e9
        return elapsed < self.lidar_timeout

    # =========================================================================
    # STATE MACHINE
    # =========================================================================

    def control_loop(self):
        """Main state machine control loop - runs at 10 Hz."""
        # Safety check: if no valid LiDAR data, handle carefully
        if not self._is_scan_valid():
            if self.state != AvoidanceState.DRIVING:
                self.get_logger().warn('LiDAR data lost during avoidance - stopping')
                self._publish_stop()
            else:
                # In driving mode with stale data, block forward motion
                cmd = Twist()
                if self.latest_cmd.linear.x <= 0:
                    cmd = self.latest_cmd  # Allow reverse
                else:
                    cmd.angular.z = self.latest_cmd.angular.z  # Allow rotation only
                self.cmd_pub.publish(cmd)
            return

        # State machine dispatch
        if self.state == AvoidanceState.DRIVING:
            self._state_driving()
        elif self.state == AvoidanceState.OBSTACLE_DETECTED:
            self._state_obstacle_detected()
        elif self.state == AvoidanceState.REVERSING:
            self._state_reversing()
        elif self.state == AvoidanceState.STOPPED_AFTER_REVERSE:
            self._state_stopped_after_reverse()
        elif self.state == AvoidanceState.SCANNING:
            self._state_scanning()
        elif self.state == AvoidanceState.TURNING:
            self._state_turning()
        elif self.state == AvoidanceState.CHECK_CLEAR:
            self._state_check_clear()

    def _transition_to(self, new_state: AvoidanceState):
        """Transition to a new state with logging."""
        old_state = self.state
        self.state = new_state
        self.state_start_time = self.get_clock().now()
        self.get_logger().info(f'State: {old_state.value} -> {new_state.value}')

    def _time_in_state(self) -> float:
        """Return seconds since entering current state."""
        return (self.get_clock().now() - self.state_start_time).nanoseconds / 1e9

    # =========================================================================
    # STATE HANDLERS
    # =========================================================================

    def _state_driving(self):
        """DRIVING state: Normal operation, pass through teleop commands."""
        # Check for obstacle (only when moving forward)
        if self.min_front_distance < self.obstacle_threshold:
            if self.latest_cmd.linear.x > 0:
                self.get_logger().warn(
                    f'OBSTACLE at {self.min_front_distance:.2f}m - initiating avoidance'
                )
                self.avoidance_attempts = 0
                self._transition_to(AvoidanceState.OBSTACLE_DETECTED)
                self._publish_stop()
                return

        # No obstacle, pass through teleop command
        self.cmd_pub.publish(self.latest_cmd)

    def _state_obstacle_detected(self):
        """OBSTACLE_DETECTED state: Immediate stop, brief pause."""
        self._publish_stop()

        if self._time_in_state() >= self.stop_duration:
            self._transition_to(AvoidanceState.REVERSING)

    def _state_reversing(self):
        """REVERSING state: Back up approximately 12cm."""
        elapsed = self._time_in_state()

        # Calculate expected distance traveled (rough estimate)
        expected_distance = abs(self.reverse_speed) * elapsed

        # Check completion conditions
        reverse_complete = expected_distance >= self.reverse_distance
        reverse_timeout = elapsed >= self.reverse_timeout

        if reverse_complete or reverse_timeout:
            if reverse_timeout and not reverse_complete:
                self.get_logger().warn('Reverse timeout - may not have backed up fully')
            self._publish_stop()
            self._transition_to(AvoidanceState.STOPPED_AFTER_REVERSE)
        else:
            # Continue reversing
            cmd = Twist()
            cmd.linear.x = self.reverse_speed
            self.cmd_pub.publish(cmd)

    def _state_stopped_after_reverse(self):
        """STOPPED_AFTER_REVERSE state: Brief pause before scanning."""
        self._publish_stop()

        if self._time_in_state() >= self.pause_after_reverse:
            self._transition_to(AvoidanceState.SCANNING)

    def _state_scanning(self):
        """SCANNING state: Analyze left vs right sectors to decide turn direction."""
        # Wait for scan data to stabilize
        if self._time_in_state() < self.scan_duration:
            self._publish_stop()
            return

        # Get sector clearances
        left_clear = self.left_sector_avg
        right_clear = self.right_sector_avg

        self.get_logger().info(
            f'Scan results - Left: {left_clear:.2f}m, Right: {right_clear:.2f}m'
        )

        # Choose clearer side (positive angular.z = turn left in ROS)
        if left_clear > right_clear:
            self.turn_direction = 1  # Turn left (positive angular.z)
            self.get_logger().info('Turning LEFT (clearer)')
        else:
            self.turn_direction = -1  # Turn right (negative angular.z)
            self.get_logger().info('Turning RIGHT (clearer)')

        # Calculate proportional turn speed based on clearance difference
        clearance_diff = abs(left_clear - right_clear)
        self.turn_speed = self.turn_speed_min + \
            (self.turn_speed_max - self.turn_speed_min) * \
            min(clearance_diff * self.turn_proportional_gain, 1.0)

        self._transition_to(AvoidanceState.TURNING)

    def _state_turning(self):
        """TURNING state: Rotate toward the clearer side."""
        elapsed = self._time_in_state()

        # Check for timeout
        if elapsed >= self.turn_max_duration:
            self.get_logger().warn('Turn timeout reached')
            self._publish_stop()
            self._transition_to(AvoidanceState.CHECK_CLEAR)
            return

        # Check if front is now clear (early exit after minimum turn time)
        if elapsed >= self.turn_min_duration:
            if self.min_front_distance >= self.clear_threshold:
                self.get_logger().info('Front cleared during turn')
                self._publish_stop()
                self._transition_to(AvoidanceState.CHECK_CLEAR)
                return

        # Safety: if obstacle appears very close during turn, stop
        if self.min_front_distance < self.obstacle_threshold * 0.8:
            self.get_logger().warn('New obstacle during turn - stopping')
            self._publish_stop()
            self._transition_to(AvoidanceState.CHECK_CLEAR)
            return

        # Continue turning
        cmd = Twist()
        cmd.angular.z = self.turn_direction * self.turn_speed
        self.cmd_pub.publish(cmd)

    def _state_check_clear(self):
        """CHECK_CLEAR state: Verify front is clear before resuming."""
        self._publish_stop()

        # Allow a moment for scan to update
        if self._time_in_state() < 0.2:
            return

        if self.min_front_distance >= self.clear_threshold:
            self.get_logger().info('Path CLEAR - resuming DRIVING')
            self.avoidance_attempts = 0
            self._transition_to(AvoidanceState.DRIVING)
        else:
            self.avoidance_attempts += 1
            if self.avoidance_attempts >= self.max_avoidance_attempts:
                self.get_logger().warn(
                    f'Max attempts ({self.max_avoidance_attempts}) reached - '
                    'returning to DRIVING (manual intervention needed)'
                )
                self._transition_to(AvoidanceState.DRIVING)
            else:
                self.get_logger().info(
                    f'Front still blocked, attempt {self.avoidance_attempts}/{self.max_avoidance_attempts} - re-scanning'
                )
                self._transition_to(AvoidanceState.SCANNING)

    # =========================================================================
    # UTILITY METHODS
    # =========================================================================

    def _publish_stop(self):
        """Publish zero velocity command."""
        self.cmd_pub.publish(Twist())

    def cmd_callback(self, msg: Twist):
        """Store incoming teleop command."""
        self.latest_cmd = msg

    def publish_status(self):
        """Publish current state for debugging."""
        msg = String()
        msg.data = (
            f'state:{self.state.value},'
            f'front:{self.min_front_distance:.2f},'
            f'left:{self.left_sector_avg:.2f},'
            f'right:{self.right_sector_avg:.2f},'
            f'attempts:{self.avoidance_attempts}'
        )
        self.status_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = SmartTurnObstacleAvoidance()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # Ensure robot stops on shutdown
        node._publish_stop()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
