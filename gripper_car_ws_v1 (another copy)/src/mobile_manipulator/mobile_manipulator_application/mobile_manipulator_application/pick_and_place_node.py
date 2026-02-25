#!/usr/bin/env python3
"""
Pick and Place Coordinator Node
===============================
Coordinates QR code detection with arm manipulation for pick-and-place tasks.

State Machine:
    IDLE -> SEARCHING -> APPROACHING -> PICKING -> PLACING -> IDLE

Topics:
    Subscribe:
        - /qr_scanner/data (std_msgs/String): Detected QR code data
        - /joint_states (sensor_msgs/JointState): Current joint positions

    Publish:
        - /arm/joint_commands (trajectory_msgs/JointTrajectoryPoint): Arm commands
        - /pick_and_place/status (std_msgs/String): Current state

Services:
    - /pick_and_place/start (std_srvs/Trigger): Start pick-and-place sequence
    - /pick_and_place/stop (std_srvs/Trigger): Stop and return to home
"""

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from std_msgs.msg import String
from sensor_msgs.msg import JointState
from trajectory_msgs.msg import JointTrajectoryPoint
from geometry_msgs.msg import PoseStamped
from std_srvs.srv import Trigger
from builtin_interfaces.msg import Duration
import math
from enum import Enum
from typing import Optional


class State(Enum):
    """Pick and place state machine states."""
    IDLE = "idle"
    SEARCHING = "searching"
    APPROACHING = "approaching"
    PICKING = "picking"
    PLACING = "placing"
    RETURNING = "returning"


class PickAndPlaceNode(Node):
    """Coordinates pick-and-place operations."""

    # Predefined arm poses (joint angles in radians)
    POSES = {
        'home': [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        'search': [0.0, -0.5, 0.3, 0.0, 0.0, 0.0],
        'pre_pick': [0.0, -0.8, 0.6, 0.0, 0.0, 0.0],
        'pick': [0.0, -1.0, 0.8, 0.0, 0.0, 0.0],
        'carry': [0.0, -0.3, 0.2, 0.0, 0.0, 0.0],
        'pre_place': [1.57, -0.8, 0.6, 0.0, 0.0, 0.0],
        'place': [1.57, -1.0, 0.8, 0.0, 0.0, 0.0],
    }

    GRIPPER_OPEN = 0.5   # radians
    GRIPPER_CLOSED = 0.0  # radians

    def __init__(self):
        super().__init__('pick_and_place')

        # Parameters
        self.declare_parameter('arm_joints', [
            'joint_1', 'joint_2', 'joint_3', 'joint_4',
            'gripper_base_joint', 'left_gear_joint'
        ])
        self.declare_parameter('motion_time', 2.0)
        self.declare_parameter('gripper_time', 1.0)

        self.arm_joints = self.get_parameter('arm_joints').value
        self.motion_time = self.get_parameter('motion_time').value
        self.gripper_time = self.get_parameter('gripper_time').value

        # State
        self.state = State.IDLE
        self.current_joint_positions = [0.0] * 6
        self.detected_qr_data: Optional[str] = None
        self.target_qr_code: Optional[str] = None

        # Publishers
        self.pub_arm_cmd = self.create_publisher(
            JointTrajectoryPoint, '/arm/joint_commands', 10)
        self.pub_status = self.create_publisher(
            String, '/pick_and_place/status', 10)

        # Subscribers
        self.sub_qr = self.create_subscription(
            String, '/qr_scanner/data', self.qr_callback, 10)
        self.sub_joints = self.create_subscription(
            JointState, '/joint_states', self.joint_state_callback, 10)

        # Services
        self.srv_start = self.create_service(
            Trigger, '/pick_and_place/start', self.start_callback)
        self.srv_stop = self.create_service(
            Trigger, '/pick_and_place/stop', self.stop_callback)

        # Timer for state machine
        self.state_timer = self.create_timer(0.1, self.state_machine_tick)

        # Timer for status publishing
        self.status_timer = self.create_timer(1.0, self.publish_status)

        # Motion state
        self.motion_in_progress = False
        self.motion_start_time = None
        self.motion_duration = 0.0

        self.get_logger().info('Pick and Place node initialized')
        self.publish_status()

    def qr_callback(self, msg: String):
        """Handle detected QR codes."""
        self.detected_qr_data = msg.data
        self.get_logger().info(f'QR detected: {msg.data}')

        # If searching and we found our target, proceed
        if self.state == State.SEARCHING:
            if self.target_qr_code is None or msg.data == self.target_qr_code:
                self.get_logger().info(f'Target QR found: {msg.data}')
                self.state = State.APPROACHING

    def joint_state_callback(self, msg: JointState):
        """Update current joint positions."""
        for i, joint_name in enumerate(self.arm_joints):
            if joint_name in msg.name:
                idx = msg.name.index(joint_name)
                if idx < len(msg.position):
                    self.current_joint_positions[i] = msg.position[idx]

    def start_callback(self, request, response):
        """Start pick-and-place sequence."""
        if self.state != State.IDLE:
            response.success = False
            response.message = f'Cannot start: currently in {self.state.value} state'
            return response

        self.state = State.SEARCHING
        self.detected_qr_data = None
        response.success = True
        response.message = 'Pick and place sequence started'
        self.get_logger().info('Starting pick and place sequence')
        return response

    def stop_callback(self, request, response):
        """Stop and return to home position."""
        self.get_logger().info('Stopping pick and place, returning home')
        self.state = State.RETURNING
        self.send_arm_to_pose('home')
        response.success = True
        response.message = 'Returning to home position'
        return response

    def publish_status(self):
        """Publish current state."""
        msg = String()
        msg.data = self.state.value
        self.pub_status.publish(msg)

    def send_arm_to_pose(self, pose_name: str, duration: float = None):
        """Send arm to a predefined pose."""
        if pose_name not in self.POSES:
            self.get_logger().error(f'Unknown pose: {pose_name}')
            return False

        if duration is None:
            duration = self.motion_time

        positions = self.POSES[pose_name]
        return self.send_arm_command(positions, duration)

    def send_arm_command(self, positions: list, duration: float):
        """Send joint trajectory command to arm."""
        msg = JointTrajectoryPoint()
        msg.positions = positions
        msg.time_from_start = Duration(
            sec=int(duration),
            nanosec=int((duration % 1) * 1e9)
        )
        self.pub_arm_cmd.publish(msg)

        self.motion_in_progress = True
        self.motion_start_time = self.get_clock().now()
        self.motion_duration = duration

        self.get_logger().debug(f'Sent arm command: {positions}')
        return True

    def set_gripper(self, open_gripper: bool):
        """Open or close the gripper."""
        positions = list(self.current_joint_positions)
        positions[5] = self.GRIPPER_OPEN if open_gripper else self.GRIPPER_CLOSED
        return self.send_arm_command(positions, self.gripper_time)

    def is_motion_complete(self) -> bool:
        """Check if current motion is complete."""
        if not self.motion_in_progress:
            return True

        if self.motion_start_time is None:
            return True

        elapsed = (self.get_clock().now() - self.motion_start_time).nanoseconds / 1e9
        if elapsed >= self.motion_duration:
            self.motion_in_progress = False
            return True

        return False

    def state_machine_tick(self):
        """Execute state machine logic."""
        # Wait for motion to complete before transitioning
        if not self.is_motion_complete():
            return

        if self.state == State.IDLE:
            # Do nothing, wait for start command
            pass

        elif self.state == State.SEARCHING:
            # Move to search pose and look for QR codes
            if not self.motion_in_progress:
                self.send_arm_to_pose('search')
            # QR callback will transition to APPROACHING when target found

        elif self.state == State.APPROACHING:
            # Move to pre-pick position
            self.get_logger().info('Moving to pre-pick position')
            self.send_arm_to_pose('pre_pick')
            self.state = State.PICKING

        elif self.state == State.PICKING:
            # Execute pick sequence
            self._execute_pick_sequence()

        elif self.state == State.PLACING:
            # Execute place sequence
            self._execute_place_sequence()

        elif self.state == State.RETURNING:
            # Check if returned to home
            self.get_logger().info('Returned to home position')
            self.state = State.IDLE

    def _execute_pick_sequence(self):
        """Execute the picking motion sequence."""
        # This is a simplified sequence - in practice you'd track substates
        self.get_logger().info('Executing pick sequence')

        # Open gripper
        self.set_gripper(True)

        # Wait and move to pick
        self.create_timer(
            self.gripper_time + 0.5,
            lambda: self._pick_step_2(),
            one_shot=True
        )

    def _pick_step_2(self):
        """Continue pick sequence."""
        self.send_arm_to_pose('pick')
        self.create_timer(
            self.motion_time + 0.5,
            lambda: self._pick_step_3(),
            one_shot=True
        )

    def _pick_step_3(self):
        """Close gripper on object."""
        self.set_gripper(False)
        self.create_timer(
            self.gripper_time + 0.5,
            lambda: self._pick_step_4(),
            one_shot=True
        )

    def _pick_step_4(self):
        """Lift object."""
        self.send_arm_to_pose('carry')
        self.state = State.PLACING

    def _execute_place_sequence(self):
        """Execute the placing motion sequence."""
        self.get_logger().info('Executing place sequence')

        # Move to pre-place
        self.send_arm_to_pose('pre_place')
        self.create_timer(
            self.motion_time + 0.5,
            lambda: self._place_step_2(),
            one_shot=True
        )

    def _place_step_2(self):
        """Move to place position."""
        self.send_arm_to_pose('place')
        self.create_timer(
            self.motion_time + 0.5,
            lambda: self._place_step_3(),
            one_shot=True
        )

    def _place_step_3(self):
        """Release object."""
        self.set_gripper(True)
        self.create_timer(
            self.gripper_time + 0.5,
            lambda: self._place_step_4(),
            one_shot=True
        )

    def _place_step_4(self):
        """Return to home after placing."""
        self.send_arm_to_pose('home')
        self.state = State.RETURNING


def main(args=None):
    rclpy.init(args=args)
    node = PickAndPlaceNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
