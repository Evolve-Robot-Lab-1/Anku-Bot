#!/usr/bin/env python3
"""
Joint State Aggregator Node
============================
Subscribes to separate joint state topics and publishes unified /joint_states.

This is a TEMPORARY solution until migrating to ros2_control.
In production, use joint_state_broadcaster from ros2_control.

Subscribes to:
  /base/joint_states   (wheel joints from base controller)
  /arm/joint_states    (arm joints from arm controller)

Publishes to:
  /joint_states        (combined, synchronized)
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy


class JointStateAggregator(Node):
    
    def __init__(self):
        super().__init__('joint_state_aggregator')
        
        # QoS profile for sensor data
        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.BEST_EFFORT,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10
        )
        
        # Latest joint states from each subsystem
        self.base_joint_state = None
        self.arm_joint_state = None
        
        # Subscribers
        self.base_sub = self.create_subscription(
            JointState,
            '/base/joint_states',
            self.base_callback,
            qos_profile
        )
        
        self.arm_sub = self.create_subscription(
            JointState,
            '/arm/joint_states',
            self.arm_callback,
            qos_profile
        )
        
        # Publisher for unified joint states
        self.joint_state_pub = self.create_publisher(
            JointState,
            '/joint_states',
            10
        )
        
        # Publish rate (should match fastest input)
        self.timer = self.create_timer(0.02, self.publish_combined)  # 50 Hz
        
        self.get_logger().info('Joint State Aggregator started')
        self.get_logger().warn('Using aggregator - migrate to ros2_control for production!')
    
    def base_callback(self, msg):
        """Store latest base joint states"""
        self.base_joint_state = msg
    
    def arm_callback(self, msg):
        """Store latest arm joint states"""
        self.arm_joint_state = msg
    
    def publish_combined(self):
        """Combine and publish unified joint states"""
        
        # Wait until we have data from both subsystems
        if self.base_joint_state is None or self.arm_joint_state is None:
            return
        
        # Create combined message
        combined = JointState()
        combined.header.stamp = self.get_clock().now().to_msg()
        
        # Merge base joints
        combined.name.extend(self.base_joint_state.name)
        combined.position.extend(self.base_joint_state.position)
        combined.velocity.extend(self.base_joint_state.velocity)
        combined.effort.extend(self.base_joint_state.effort)
        
        # Merge arm joints
        combined.name.extend(self.arm_joint_state.name)
        combined.position.extend(self.arm_joint_state.position)
        combined.velocity.extend(self.arm_joint_state.velocity)
        combined.effort.extend(self.arm_joint_state.effort)
        
        # Publish unified message
        self.joint_state_pub.publish(combined)


def main(args=None):
    rclpy.init(args=args)
    node = JointStateAggregator()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
