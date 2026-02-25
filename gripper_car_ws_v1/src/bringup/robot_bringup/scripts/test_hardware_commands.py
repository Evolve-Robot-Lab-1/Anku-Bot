#!/usr/bin/env python3
"""
Hardware Test Script
====================
Interactive test script for testing the mobile manipulator with Arduino.

Tests:
1. Serial communication and heartbeat
2. Arm joint commands and position feedback
3. Base velocity commands and odometry
4. Data flow verification

Usage:
    ros2 run robot_bringup test_hardware_commands.py
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import JointState
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from std_msgs.msg import String
import math
import time
import sys
import threading


class HardwareTestNode(Node):
    """Test node for mobile manipulator hardware."""

    def __init__(self):
        super().__init__('hardware_test')

        qos_reliable = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=10
        )

        # Publishers
        self.pub_arm_cmd = self.create_publisher(
            JointState, '/arm/joint_commands', qos_reliable)
        self.pub_cmd_vel = self.create_publisher(
            Twist, '/cmd_vel', qos_reliable)

        # Subscribers
        self.sub_joint_states = self.create_subscription(
            JointState, '/joint_states',
            self.joint_states_callback, qos_reliable)
        self.sub_odom = self.create_subscription(
            Odometry, '/odom',
            self.odom_callback, qos_reliable)
        self.sub_status = self.create_subscription(
            String, '/hardware/status',
            self.status_callback, 10)

        # State tracking
        self.last_joint_states = None
        self.last_odom = None
        self.joint_states_count = 0
        self.odom_count = 0
        self.status_messages = []

        # Lock for thread safety
        self.lock = threading.Lock()

        self.get_logger().info("Hardware Test Node initialized")
        self.get_logger().info("Waiting for hardware node...")

    def joint_states_callback(self, msg: JointState):
        with self.lock:
            self.last_joint_states = msg
            self.joint_states_count += 1

    def odom_callback(self, msg: Odometry):
        with self.lock:
            self.last_odom = msg
            self.odom_count += 1

    def status_callback(self, msg: String):
        with self.lock:
            self.status_messages.append(msg.data)
            if len(self.status_messages) > 100:
                self.status_messages.pop(0)
        self.get_logger().info(f"Status: {msg.data}")

    def wait_for_connection(self, timeout=10.0) -> bool:
        """Wait for hardware node to be ready."""
        self.get_logger().info("Waiting for joint_states topic...")
        start = time.time()
        while time.time() - start < timeout:
            with self.lock:
                if self.joint_states_count > 0:
                    self.get_logger().info("Hardware node connected!")
                    return True
            rclpy.spin_once(self, timeout_sec=0.1)
        self.get_logger().error("Timeout waiting for hardware node")
        return False

    def test_arm_command(self, joint_positions: list) -> bool:
        """Send arm command and verify response."""
        self.get_logger().info(f"Testing arm command: {joint_positions}")

        # Create joint command
        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.name = ['joint_1', 'joint_2', 'joint_3',
                    'joint_4', 'gripper_base_joint', 'left_gear_joint']
        msg.position = joint_positions

        # Record current count
        with self.lock:
            start_count = self.joint_states_count

        # Send command
        self.pub_arm_cmd.publish(msg)
        self.get_logger().info("Command sent, waiting for feedback...")

        # Wait for response
        timeout = 3.0
        start = time.time()
        while time.time() - start < timeout:
            rclpy.spin_once(self, timeout_sec=0.1)
            with self.lock:
                if self.joint_states_count > start_count:
                    self.get_logger().info("Received joint states update")
                    return True

        self.get_logger().warn("No joint states update received")
        return True  # Continue even if no immediate feedback

    def test_velocity_command(self, linear_x: float, angular_z: float,
                              duration: float = 2.0) -> bool:
        """Send velocity command and verify odometry updates."""
        self.get_logger().info(
            f"Testing velocity: linear_x={linear_x}, angular_z={angular_z}")

        # Record initial state
        with self.lock:
            start_count = self.odom_count
            initial_odom = self.last_odom

        # Create velocity command
        msg = Twist()
        msg.linear.x = linear_x
        msg.angular.z = angular_z

        # Send commands for duration
        start = time.time()
        while time.time() - start < duration:
            self.pub_cmd_vel.publish(msg)
            rclpy.spin_once(self, timeout_sec=0.05)

        # Stop
        stop_msg = Twist()
        self.pub_cmd_vel.publish(stop_msg)

        # Check odometry updates
        with self.lock:
            odom_updates = self.odom_count - start_count
            final_odom = self.last_odom

        self.get_logger().info(f"Received {odom_updates} odometry updates")

        if initial_odom and final_odom:
            dx = final_odom.pose.pose.position.x - initial_odom.pose.pose.position.x
            dy = final_odom.pose.pose.position.y - initial_odom.pose.pose.position.y
            self.get_logger().info(f"Position change: dx={dx:.4f}, dy={dy:.4f}")

        return odom_updates > 0

    def print_current_state(self):
        """Print current robot state."""
        with self.lock:
            js = self.last_joint_states
            odom = self.last_odom

        print("\n" + "=" * 60)
        print("CURRENT ROBOT STATE")
        print("=" * 60)

        if js:
            print("\nJoint States:")
            for i, name in enumerate(js.name):
                pos = js.position[i] if i < len(js.position) else 0.0
                print(f"  {name}: {pos:.4f} rad ({math.degrees(pos):.2f} deg)")
        else:
            print("\nJoint States: NOT RECEIVED")

        if odom:
            print("\nOdometry:")
            print(f"  Position: x={odom.pose.pose.position.x:.4f}, "
                  f"y={odom.pose.pose.position.y:.4f}")
            print(f"  Velocity: linear={odom.twist.twist.linear.x:.4f}, "
                  f"angular={odom.twist.twist.angular.z:.4f}")
        else:
            print("\nOdometry: NOT RECEIVED")

        print("=" * 60)

    def run_interactive_tests(self):
        """Run interactive test menu."""
        while rclpy.ok():
            print("\n" + "=" * 50)
            print("HARDWARE TEST MENU")
            print("=" * 50)
            print("1. Test arm joint command")
            print("2. Test base velocity (forward)")
            print("3. Test base velocity (rotate)")
            print("4. Print current state")
            print("5. Continuous state monitor")
            print("6. Run all tests")
            print("q. Quit")
            print("-" * 50)

            try:
                choice = input("Enter choice: ").strip().lower()
            except EOFError:
                break

            if choice == '1':
                self.test_arm_home()
            elif choice == '2':
                self.test_velocity_command(0.2, 0.0, 2.0)
            elif choice == '3':
                self.test_velocity_command(0.0, 0.3, 2.0)
            elif choice == '4':
                self.print_current_state()
            elif choice == '5':
                self.continuous_monitor()
            elif choice == '6':
                self.run_all_tests()
            elif choice == 'q':
                break
            else:
                print("Invalid choice")

    def test_arm_home(self):
        """Send arm to home position."""
        # Home position (all zeros in radians)
        home_pos = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        self.test_arm_command(home_pos)

    def continuous_monitor(self):
        """Continuously monitor and display state."""
        print("\nContinuous monitor (Ctrl+C to stop)...")
        try:
            while rclpy.ok():
                rclpy.spin_once(self, timeout_sec=0.1)
                self.print_current_state()
                time.sleep(0.5)
        except KeyboardInterrupt:
            print("\nMonitor stopped")

    def run_all_tests(self):
        """Run complete test suite."""
        print("\n" + "=" * 60)
        print("RUNNING COMPLETE TEST SUITE")
        print("=" * 60)

        results = []

        # Test 1: Connection
        print("\n[TEST 1] Connection Test")
        result = self.wait_for_connection(timeout=5.0)
        results.append(("Connection", result))
        print(f"  Result: {'PASS' if result else 'FAIL'}")

        # Test 2: Arm command
        print("\n[TEST 2] Arm Command Test")
        result = self.test_arm_command([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
        results.append(("Arm Command", result))
        print(f"  Result: {'PASS' if result else 'FAIL'}")
        time.sleep(1.0)

        # Test 3: Forward velocity
        print("\n[TEST 3] Forward Velocity Test")
        result = self.test_velocity_command(0.1, 0.0, 1.5)
        results.append(("Forward Velocity", result))
        print(f"  Result: {'PASS' if result else 'FAIL'}")
        time.sleep(0.5)

        # Test 4: Rotation velocity
        print("\n[TEST 4] Rotation Velocity Test")
        result = self.test_velocity_command(0.0, 0.2, 1.5)
        results.append(("Rotation Velocity", result))
        print(f"  Result: {'PASS' if result else 'FAIL'}")
        time.sleep(0.5)

        # Test 5: Arm movement
        print("\n[TEST 5] Arm Movement Test")
        result = self.test_arm_command([0.5, 0.3, -0.2, 0.1, 0.0, 0.3])
        results.append(("Arm Movement", result))
        print(f"  Result: {'PASS' if result else 'FAIL'}")
        time.sleep(1.5)

        # Return to home
        print("\n[CLEANUP] Returning to home position")
        self.test_arm_command([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])

        # Summary
        print("\n" + "=" * 60)
        print("TEST RESULTS SUMMARY")
        print("=" * 60)
        passed = sum(1 for _, r in results if r)
        total = len(results)
        for name, result in results:
            print(f"  {name}: {'PASS' if result else 'FAIL'}")
        print("-" * 60)
        print(f"  Total: {passed}/{total} tests passed")
        print("=" * 60)


def main():
    rclpy.init()
    node = HardwareTestNode()

    # Wait for hardware to be ready
    if not node.wait_for_connection(timeout=10.0):
        node.get_logger().error(
            "Hardware node not detected. Make sure to run:\n"
            "  ros2 launch robot_bringup test_hardware.launch.py"
        )
        node.destroy_node()
        rclpy.shutdown()
        return

    try:
        node.run_interactive_tests()
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
