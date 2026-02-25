#!/usr/bin/env python3

"""
Test script for Arduino ROS2 communication
This script tests the communication between ROS2 and Arduino Uno
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
from sensor_msgs.msg import JointState
from std_msgs.msg import Int32MultiArray, Float32MultiArray, String
import time
import math

class ArduinoCommunicationTester(Node):
    def __init__(self):
        super().__init__('arduino_communication_tester')
        
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
        
        self.pwm_commands_pub = self.create_publisher(
            Float32MultiArray,
            '/arduino/pwm_commands',
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
        self.test_step = 0
        self.test_start_time = None
        
        self.get_logger().info('Arduino Communication Tester initialized')
    
    def joint_states_callback(self, msg):
        """Callback for joint states from Arduino"""
        self.joint_states = msg
        self.get_logger().info(f'Joint states: {[f"{name}: {pos:.1f}°" for name, pos in zip(msg.name, msg.position)]}')
    
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
    
    def test_servo_movement(self):
        """Test servo movement with sinusoidal pattern"""
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
        self.get_logger().info(f'Published servo commands: {[f"{pos:.1f}°" for pos in positions]}')
        return True
    
    def test_digital_outputs(self):
        """Test digital outputs"""
        msg = Int32MultiArray()
        # Send pin 13 HIGH, then LOW (LED blink test)
        if int(time.time()) % 2 == 0:
            msg.data = [13, 1]  # Pin 13, HIGH
        else:
            msg.data = [13, 0]  # Pin 13, LOW
        
        self.digital_outputs_pub.publish(msg)
        self.get_logger().info(f'Published digital output: pin {msg.data[0]} = {msg.data[1]}')
    
    def test_pwm_outputs(self):
        """Test PWM outputs"""
        msg = Float32MultiArray()
        # Create breathing LED effect on pin 9
        current_time = time.time()
        pwm_value = int(127 + 127 * math.sin(2 * math.pi * 0.5 * current_time))
        msg.data = [9.0, float(pwm_value)]  # Pin 9, PWM value
        
        self.pwm_commands_pub.publish(msg)
        self.get_logger().info(f'Published PWM command: pin {int(msg.data[0])} = {int(msg.data[1])}')
    
    def run_communication_test(self):
        """Run comprehensive communication test"""
        self.get_logger().info('Starting Arduino communication test...')
        
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
        
        self.get_logger().info('✅ Arduino communication established!')
        
        # Run tests
        test_duration = 30.0  # seconds
        test_start = time.time()
        
        while time.time() - test_start < test_duration:
            current_step = int((time.time() - test_start) / 5.0) % 4
            
            if current_step == 0:
                self.test_servo_movement()
            elif current_step == 1:
                self.test_digital_outputs()
            elif current_step == 2:
                self.test_pwm_outputs()
            else:
                # Monitor mode
                pass
            
            rclpy.spin_once(self, timeout_sec=0.1)
        
        self.get_logger().info('✅ Arduino communication test completed successfully!')
        return True
    
    def run_quick_test(self):
        """Run a quick connectivity test"""
        self.get_logger().info('Running quick Arduino connectivity test...')
        
        # Wait for any Arduino data
        timeout = 5.0
        start_time = time.time()
        
        while self.arduino_status is None:
            if time.time() - start_time > timeout:
                self.get_logger().error('❌ No Arduino communication detected!')
                return False
            rclpy.spin_once(self, timeout_sec=0.1)
        
        self.get_logger().info('✅ Arduino connectivity confirmed!')
        return True

def main(args=None):
    rclpy.init(args=args)
    
    tester = ArduinoCommunicationTester()
    
    try:
        # Run quick test first
        if tester.run_quick_test():
            # If quick test passes, run full test
            success = tester.run_communication_test()
            if success:
                print("🎉 Arduino communication test PASSED")
            else:
                print("❌ Arduino communication test FAILED")
        else:
            print("❌ Arduino connectivity test FAILED")
            
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
    except Exception as e:
        print(f"❌ Test failed with exception: {e}")
    finally:
        tester.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
