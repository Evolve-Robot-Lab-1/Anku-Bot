#!/usr/bin/env python3

"""
Test script for servo mapping between MoveIt joint values and Arduino servo positions
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
from sensor_msgs.msg import JointState
import time
import math

class ServoMappingTester(Node):
    def __init__(self):
        super().__init__('servo_mapping_tester')
        
        # QoS profile
        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.RELIABLE,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10
        )
        
        # Publisher for servo commands
        self.servo_commands_pub = self.create_publisher(
            JointState,
            '/arduino/servo_commands',
            qos_profile
        )
        
        # Subscriber for joint states
        self.joint_states_sub = self.create_subscription(
            JointState,
            '/arduino/joint_states',
            self.joint_states_callback,
            qos_profile
        )
        
        self.get_logger().info('Servo Mapping Tester initialized')
    
    def joint_states_callback(self, msg):
        """Callback for joint states from Arduino"""
        self.get_logger().info(f'Received joint states: {[f"{name}: {pos:.3f} rad" for name, pos in zip(msg.name, msg.position)]}')
    
    def test_joint_mapping(self):
        """Test mapping between joint positions and servo angles"""
        self.get_logger().info('Testing joint to servo mapping...')
        
        # Test cases: joint positions in radians
        test_cases = [
            # Test case: [joint_1, joint_2, joint_3, joint_4, gripper_base_joint, left_gear_joint]
            [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],           # All at center
            [1.57, 0.0, -1.57, 0.0, 0.0, 0.785],      # Some movement
            [0.0, 1.57, 0.0, -1.57, 0.785, 0.0],      # Different movement
            [-1.57, 0.0, 1.57, 0.0, -0.785, 0.0],     # Negative angles
            [3.14159, 0.0, -3.14159, 0.0, 1.57, 1.57], # Max angles
        ]
        
        for i, test_case in enumerate(test_cases):
            self.get_logger().info(f'Test case {i+1}: {test_case}')
            
            msg = JointState()
            msg.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
            msg.position = test_case
            msg.velocity = [0.0] * 6
            msg.effort = [0.0] * 6
            
            self.servo_commands_pub.publish(msg)
            self.get_logger().info(f'Published test case {i+1}')
            
            # Wait for response
            time.sleep(2)
    
    def test_individual_joints(self):
        """Test individual joint movements"""
        self.get_logger().info('Testing individual joint movements...')
        
        joint_names = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
        
        for i, joint_name in enumerate(joint_names):
            self.get_logger().info(f'Testing {joint_name}...')
            
            # Create test positions for this joint only
            test_positions = [0.0] * 6
            test_positions[i] = 1.57  # 90 degrees in radians
            
            msg = JointState()
            msg.name = joint_names
            msg.position = test_positions
            msg.velocity = [0.0] * 6
            msg.effort = [0.0] * 6
            
            self.servo_commands_pub.publish(msg)
            self.get_logger().info(f'Published {joint_name} = {test_positions[i]:.3f} rad')
            
            # Wait for movement
            time.sleep(3)
            
            # Reset to center
            test_positions[i] = 0.0
            msg.position = test_positions
            self.servo_commands_pub.publish(msg)
            self.get_logger().info(f'Reset {joint_name} to center')
            
            time.sleep(2)
    
    def run_comprehensive_test(self):
        """Run comprehensive servo mapping test"""
        self.get_logger().info('Starting comprehensive servo mapping test...')
        
        # Wait a moment for subscribers to connect
        time.sleep(2)
        
        # Test joint mapping
        self.test_joint_mapping()
        
        # Wait between tests
        time.sleep(5)
        
        # Test individual joints
        self.test_individual_joints()
        
        self.get_logger().info('Servo mapping test completed!')

def main(args=None):
    rclpy.init(args=args)
    
    tester = ServoMappingTester()
    
    try:
        tester.run_comprehensive_test()
        print("🎉 Servo mapping test completed!")
        
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
    except Exception as e:
        print(f"❌ Test failed with exception: {e}")
    finally:
        tester.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
