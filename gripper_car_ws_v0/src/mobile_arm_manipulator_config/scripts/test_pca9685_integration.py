#!/usr/bin/env python3

"""
Test script for PCA9685 Arduino Integration
This script tests the communication between ROS2 and Arduino Uno with PCA9685 servo driver
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
from sensor_msgs.msg import JointState
from std_msgs.msg import Int32MultiArray, Float32MultiArray, String
import time
import math

class PCA9685IntegrationTester(Node):
    def __init__(self):
        super().__init__('pca9685_integration_tester')
        
        # QoS profile
        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.RELIABLE,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10
        )
        
        # Publishers
        self.servo_commands_pub = self.create_publisher(
            JointState,
            '/arduino/servo_commands',
            qos_profile
        )
        
        self.digital_outputs_pub = self.create_publisher(
            Int32MultiArray,
            '/arduino/digital_outputs',
            qos_profile
        )
        
        # Subscribers
        self.joint_states_sub = self.create_subscription(
            JointState,
            '/arduino/joint_states',
            self.joint_states_callback,
            qos_profile
        )
        
        self.analog_sensors_sub = self.create_subscription(
            Int32MultiArray,
            '/arduino/analog_sensors',
            self.analog_sensors_callback,
            qos_profile
        )
        
        self.digital_sensors_sub = self.create_subscription(
            Int32MultiArray,
            '/arduino/digital_sensors',
            self.digital_sensors_callback,
            qos_profile
        )
        
        self.status_sub = self.create_subscription(
            String,
            '/arduino/status',
            self.status_callback,
            qos_profile
        )
        
        # Data storage
        self.joint_states = None
        self.analog_sensors = None
        self.digital_sensors = None
        self.arduino_status = None
        
        # Test parameters
        self.test_start_time = None
        self.current_test = 0
        
        self.get_logger().info('PCA9685 Integration Tester initialized')
    
    def joint_states_callback(self, msg):
        """Callback for joint states from Arduino"""
        self.joint_states = msg
        positions = [f"{name}: {pos:.1f}°" for name, pos in zip(msg.name, msg.position)]
        self.get_logger().info(f'PCA9685 Joint states: {positions}')
    
    def analog_sensors_callback(self, msg):
        """Callback for analog sensors from Arduino"""
        self.analog_sensors = msg
        self.get_logger().info(f'Analog sensors: {msg.data}')
    
    def digital_sensors_callback(self, msg):
        """Callback for digital sensors from Arduino"""
        self.digital_sensors = msg
        self.get_logger().info(f'Digital sensors: {msg.data}')
    
    def status_callback(self, msg):
        """Callback for Arduino status"""
        self.arduino_status = msg
        self.get_logger().info(f'Arduino status: {msg.data}')
    
    def test_smooth_movement(self):
        """Test smooth servo movement with sinusoidal pattern"""
        if self.joint_states is None:
            return False
        
        msg = JointState()
        msg.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
        
        # Create sinusoidal movement
        current_time = time.time()
        if self.test_start_time is None:
            self.test_start_time = current_time
        
        elapsed_time = current_time - self.test_start_time
        
        # Base positions (90 degrees = center)
        base_positions = [90.0, 90.0, 90.0, 90.0, 90.0, 90.0]
        amplitudes = [30.0, 20.0, 25.0, 15.0, 10.0, 20.0]  # degrees
        frequencies = [0.5, 0.7, 0.3, 0.9, 1.2, 0.6]  # Hz
        
        positions = []
        for i in range(6):
            angle = base_positions[i] + amplitudes[i] * math.sin(2 * math.pi * frequencies[i] * elapsed_time)
            positions.append(max(0, min(180, angle)))  # Constrain to servo range
        
        msg.position = positions
        msg.velocity = [0.0] * 6
        msg.effort = [0.0] * 6
        
        self.servo_commands_pub.publish(msg)
        self.get_logger().info(f'Published PCA9685 servo commands: {[f"{pos:.1f}°" for pos in positions]}')
        return True
    
    def test_individual_servo_control(self):
        """Test individual servo control"""
        if self.joint_states is None:
            return False
        
        # Test each servo individually with different angles
        test_angles = [
            [0, 90, 90, 90, 90, 90],    # Test joint_1
            [90, 0, 90, 90, 90, 90],    # Test joint_2
            [90, 90, 180, 90, 90, 90],  # Test joint_3
            [90, 90, 90, 0, 90, 90],    # Test joint_4
            [90, 90, 90, 90, 180, 90],  # Test gripper_base_joint
            [90, 90, 90, 90, 90, 45],   # Test left_gear_joint
        ]
        
        current_time = time.time()
        if self.test_start_time is None:
            self.test_start_time = current_time
        
        # Change test every 3 seconds
        test_index = int((current_time - self.test_start_time) / 3.0) % len(test_angles)
        
        msg = JointState()
        msg.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
        msg.position = test_angles[test_index]
        msg.velocity = [0.0] * 6
        msg.effort = [0.0] * 6
        
        self.servo_commands_pub.publish(msg)
        self.get_logger().info(f'Individual servo test {test_index + 1}: {msg.position}')
        return True
    
    def test_coordinated_movement(self):
        """Test coordinated movement of multiple servos"""
        if self.joint_states is None:
            return False
        
        msg = JointState()
        msg.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
        
        # Create coordinated movement pattern
        current_time = time.time()
        if self.test_start_time is None:
            self.test_start_time = current_time
        
        elapsed_time = current_time - self.test_start_time
        
        # Wave-like coordinated movement
        positions = []
        for i in range(6):
            phase_offset = i * math.pi / 3  # 60 degree phase offset between servos
            angle = 90 + 20 * math.sin(2 * math.pi * 0.2 * elapsed_time + phase_offset)
            positions.append(max(0, min(180, angle)))
        
        msg.position = positions
        msg.velocity = [0.0] * 6
        msg.effort = [0.0] * 6
        
        self.servo_commands_pub.publish(msg)
        self.get_logger().info(f'Coordinated movement: {[f"{pos:.1f}°" for pos in positions]}')
        return True
    
    def test_emergency_stop_simulation(self):
        """Test emergency stop functionality"""
        self.get_logger().info('Testing emergency stop simulation...')
        
        # Send emergency stop command (this would be handled by the hardware button)
        # In a real test, you would press the emergency stop button
        self.get_logger().info('Emergency stop test: Press hardware emergency stop button on pin 2')
        return True
    
    def run_comprehensive_test(self):
        """Run comprehensive PCA9685 integration test"""
        self.get_logger().info('Starting PCA9685 integration test...')
        
        # Wait for initial data
        self.get_logger().info('Waiting for Arduino data...')
        timeout = 10.0
        start_time = time.time()
        
        while (self.joint_states is None or self.analog_sensors is None or 
               self.digital_sensors is None or self.arduino_status is None):
            if time.time() - start_time > timeout:
                self.get_logger().error('Timeout waiting for Arduino data!')
                return False
            
            rclpy.spin_once(self, timeout_sec=0.1)
        
        self.get_logger().info('✅ PCA9685 communication established!')
        
        # Run different tests
        test_duration = 60.0  # seconds
        test_start = time.time()
        
        while time.time() - test_start < test_duration:
            elapsed = time.time() - test_start
            current_test = int(elapsed / 15.0) % 4  # 15 seconds per test
            
            if current_test != self.current_test:
                self.current_test = current_test
                self.test_start_time = None  # Reset for new test
            
            if current_test == 0:
                self.test_smooth_movement()
                if elapsed % 15.0 < 1.0:  # Log once per test
                    self.get_logger().info('Running smooth movement test...')
            elif current_test == 1:
                self.test_individual_servo_control()
                if elapsed % 15.0 < 1.0:
                    self.get_logger().info('Running individual servo control test...')
            elif current_test == 2:
                self.test_coordinated_movement()
                if elapsed % 15.0 < 1.0:
                    self.get_logger().info('Running coordinated movement test...')
            else:
                self.test_emergency_stop_simulation()
                if elapsed % 15.0 < 1.0:
                    self.get_logger().info('Emergency stop test phase...')
            
            rclpy.spin_once(self, timeout_sec=0.1)
        
        self.get_logger().info('✅ PCA9685 integration test completed successfully!')
        return True
    
    def run_quick_test(self):
        """Run a quick connectivity test"""
        self.get_logger().info('Running quick PCA9685 connectivity test...')
        
        # Wait for any Arduino data
        timeout = 5.0
        start_time = time.time()
        
        while self.arduino_status is None:
            if time.time() - start_time > timeout:
                self.get_logger().error('❌ No Arduino PCA9685 communication detected!')
                return False
            rclpy.spin_once(self, timeout_sec=0.1)
        
        self.get_logger().info('✅ Arduino PCA9685 connectivity confirmed!')
        return True

def main(args=None):
    rclpy.init(args=args)
    
    tester = PCA9685IntegrationTester()
    
    try:
        # Run quick test first
        if tester.run_quick_test():
            # If quick test passes, run full test
            success = tester.run_comprehensive_test()
            if success:
                print("🎉 PCA9685 integration test PASSED")
            else:
                print("❌ PCA9685 integration test FAILED")
        else:
            print("❌ PCA9685 connectivity test FAILED")
            
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
    except Exception as e:
        print(f"❌ Test failed with exception: {e}")
    finally:
        tester.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
