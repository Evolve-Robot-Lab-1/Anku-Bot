#!/usr/bin/env python3

"""
Arduino ROS2 Bridge Node
This node provides bidirectional communication between ROS2 and Arduino Uno
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy
import serial
import threading
import time
from typing import Optional, List, Dict

# ROS2 message types
from sensor_msgs.msg import JointState
from std_msgs.msg import Int32MultiArray, Float32MultiArray, String
from geometry_msgs.msg import Twist

class ArduinoROS2Bridge(Node):
    def __init__(self):
        super().__init__('arduino_ros2_bridge')
        
        # Declare parameters
        self.declare_parameter('serial_port', '/dev/ttyUSB0')
        self.declare_parameter('baud_rate', 115200)
        self.declare_parameter('timeout', 1.0)
        self.declare_parameter('publish_rate', 50.0)
        self.declare_parameter('emergency_stop_enabled', True)
        
        # Get parameters
        self.serial_port = self.get_parameter('serial_port').get_parameter_value().string_value
        self.baud_rate = self.get_parameter('baud_rate').get_parameter_value().integer_value
        self.timeout = self.get_parameter('timeout').get_parameter_value().double_value
        self.publish_rate = self.get_parameter('publish_rate').get_parameter_value().double_value
        self.emergency_stop_enabled = self.get_parameter('emergency_stop_enabled').get_parameter_value().bool_value
        
        # QoS profile
        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.RELIABLE,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10
        )
        
        # Publishers
        self.joint_state_pub = self.create_publisher(
            JointState,
            '/arduino/joint_states',
            qos_profile
        )
        
        self.analog_sensors_pub = self.create_publisher(
            Int32MultiArray,
            '/arduino/analog_sensors',
            qos_profile
        )
        
        self.digital_sensors_pub = self.create_publisher(
            Int32MultiArray,
            '/arduino/digital_sensors',
            qos_profile
        )
        
        self.status_pub = self.create_publisher(
            String,
            '/arduino/status',
            qos_profile
        )
        
        # Subscribers
        self.servo_commands_sub = self.create_subscription(
            JointState,
            '/arduino/servo_commands',
            self.servo_commands_callback,
            qos_profile
        )
        
        self.digital_outputs_sub = self.create_subscription(
            Int32MultiArray,
            '/arduino/digital_outputs',
            self.digital_outputs_callback,
            qos_profile
        )
        
        self.pwm_commands_sub = self.create_subscription(
            Float32MultiArray,
            '/arduino/pwm_commands',
            self.pwm_commands_callback,
            qos_profile
        )
        
        # Serial communication
        self.serial_connection: Optional[serial.Serial] = None
        self.serial_thread: Optional[threading.Thread] = None
        self.running = False
        
        # Data storage
        self.joint_states = JointState()
        self.joint_states.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
        self.joint_states.position = [0.0] * 6
        self.joint_states.velocity = [0.0] * 6
        self.joint_states.effort = [0.0] * 6
        
        self.analog_values = [0] * 6
        self.digital_values = [0] * 6
        self.emergency_stop = False
        
        # Timer for publishing
        self.timer = self.create_timer(1.0 / self.publish_rate, self.publish_data)
        
        # Initialize serial connection
        self.init_serial()
        
        self.get_logger().info('Arduino ROS2 Bridge initialized')
    
    def init_serial(self):
        """Initialize serial connection to Arduino"""
        try:
            # Close any existing connection
            if self.serial_connection and self.serial_connection.is_open:
                self.serial_connection.close()
                time.sleep(1)
            
            self.serial_connection = serial.Serial(
                port=self.serial_port,
                baudrate=self.baud_rate,
                timeout=self.timeout,
                write_timeout=self.timeout,
                rtscts=False,  # Disable hardware flow control
                dsrdtr=False   # Disable hardware flow control
            )
            
            # Clear any existing data
            self.serial_connection.flushInput()
            self.serial_connection.flushOutput()
            
            # Wait for Arduino to initialize
            time.sleep(3)
            
            # Start serial communication thread
            self.running = True
            self.serial_thread = threading.Thread(target=self.serial_communication_loop)
            self.serial_thread.daemon = True
            self.serial_thread.start()
            
            self.get_logger().info(f'Serial connection established on {self.serial_port}')
            
        except Exception as e:
            self.get_logger().error(f'Failed to initialize serial connection: {e}')
            self.serial_connection = None
    
    def serial_communication_loop(self):
        """Main serial communication loop"""
        consecutive_errors = 0
        max_consecutive_errors = 5
        
        while self.running and self.serial_connection:
            try:
                if not self.serial_connection.is_open:
                    self.get_logger().error('Serial connection lost, attempting to reconnect...')
                    self.init_serial()
                    consecutive_errors += 1
                    if consecutive_errors >= max_consecutive_errors:
                        self.get_logger().error('Too many consecutive errors, stopping communication')
                        break
                    continue
                
                # Check if data is available with timeout
                if self.serial_connection.in_waiting > 0:
                    try:
                        raw_data = self.serial_connection.readline()
                        if raw_data:  # Only process non-empty data
                            try:
                                line = raw_data.decode('utf-8').strip()
                                if line:  # Only process non-empty lines
                                    self.process_serial_data(line)
                                    consecutive_errors = 0  # Reset error counter on successful read
                            except UnicodeDecodeError:
                                # Skip corrupted data and continue
                                self.get_logger().debug(f'Skipped corrupted data: {raw_data.hex()}')
                                continue
                        else:
                            # Empty read - connection might be unstable
                            consecutive_errors += 1
                    except (serial.SerialException, OSError) as e:
                        self.get_logger().error(f'Serial read error: {e}')
                        consecutive_errors += 1
                        if consecutive_errors >= max_consecutive_errors:
                            self.get_logger().error('Too many read errors, stopping communication')
                            break
                        time.sleep(0.5)
                        continue
                else:
                    time.sleep(0.02)  # Small delay to prevent busy waiting
                    
            except Exception as e:
                self.get_logger().error(f'Serial communication error: {e}')
                consecutive_errors += 1
                time.sleep(0.5)
                if consecutive_errors >= max_consecutive_errors:
                    self.get_logger().error('Too many consecutive errors, stopping communication')
                    break
    
    def process_serial_data(self, data: str):
        """Process incoming serial data from Arduino"""
        parts = data.split(',')
        
        if len(parts) < 2:
            return
        
        command = parts[0]
        
        try:
            if command == 'ANALOG':
                # Format: ANALOG,val1,val2,val3,val4,val5,val6
                if len(parts) == 7:
                    self.analog_values = [int(x) for x in parts[1:7]]
                    
            elif command == 'DIGITAL':
                # Format: DIGITAL,val1,val2,val3,val4,val5,val6
                if len(parts) == 7:
                    self.digital_values = [int(x) for x in parts[1:7]]
                    
            elif command == 'SERVO_POS':
                # Format: SERVO_POS,pos1,pos2,pos3,pos4,pos5,pos6
                if len(parts) == 7:
                    servo_positions = [float(x) for x in parts[1:7]]
                    
                    # Convert servo angles back to joint positions (radians)
                    joint_positions = []
                    joint_names = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint']
                    
                    for i, servo_angle in enumerate(servo_positions):
                        joint_name = joint_names[i]
                        
                        # Convert servo angle back to joint position
                        if joint_name in ['joint_1', 'joint_2', 'joint_3', 'joint_4']:
                            # Full range joints: map 0° to 180° -> -180° to +180°
                            angle_deg = (servo_angle * 2.0) - 180.0
                        elif joint_name == 'gripper_base_joint':
                            # Gripper base: map 0° to 180° -> -90° to +90°
                            angle_deg = (servo_angle / 180.0) * 180.0 - 90.0
                        elif joint_name == 'left_gear_joint':
                            # Left gear: map 90° to 180° -> 0° to +90°
                            angle_deg = servo_angle - 90.0
                        else:
                            # Default mapping
                            angle_deg = (servo_angle * 2.0) - 180.0
                        
                        # Convert degrees to radians
                        joint_position = angle_deg * 3.14159 / 180.0
                        joint_positions.append(joint_position)
                    
                    self.joint_states.position = joint_positions
                    
            elif command == 'EMERGENCY_STOP':
                self.emergency_stop = True
                self.get_logger().warn('Emergency stop activated!')
                
            elif command == 'HEARTBEAT':
                # Arduino is alive
                pass
                
            elif command == 'OK':
                # Command acknowledged
                self.get_logger().debug(f'Arduino acknowledged: {data}')
                
            elif command == 'ERROR':
                self.get_logger().error(f'Arduino error: {data}')
                
        except ValueError as e:
            self.get_logger().warn(f'Failed to parse serial data: {data}, error: {e}')
    
    def send_serial_command(self, command: str):
        """Send command to Arduino via serial"""
        if self.serial_connection and self.serial_connection.is_open:
            try:
                # Ensure command ends with newline
                if not command.endswith('\n'):
                    command += '\n'
                
                # Send command
                bytes_written = self.serial_connection.write(command.encode('utf-8'))
                self.serial_connection.flush()
                
                if bytes_written > 0:
                    self.get_logger().debug(f'Sent to Arduino: {command.strip()}')
                else:
                    self.get_logger().warn(f'Failed to send command: {command.strip()}')
                    
            except (serial.SerialException, OSError) as e:
                self.get_logger().error(f'Failed to send serial command: {e}')
            except Exception as e:
                self.get_logger().error(f'Unexpected error sending command: {e}')
    
    def servo_commands_callback(self, msg: JointState):
        """Handle servo commands from ROS2"""
        if self.emergency_stop and self.emergency_stop_enabled:
            self.get_logger().warn('Emergency stop active - ignoring servo commands')
            return
        
        # Map ROS2 joint names to Arduino servo indices
        joint_mapping = {
            'joint_1': 0,
            'joint_2': 1,
            'joint_3': 2,
            'joint_4': 3,
            'gripper_base_joint': 4,
            'left_gear_joint': 5
        }
        
        # Joint limits and mapping (in radians)
        joint_limits = {
            'joint_1': (-3.14159, 3.14159),      # -180° to +180°
            'joint_2': (-3.14159, 3.14159),      # -180° to +180°
            'joint_3': (-3.14159, 3.14159),      # -180° to +180°
            'joint_4': (-3.14159, 3.14159),      # -180° to +180°
            'gripper_base_joint': (-1.57, 1.57), # -90° to +90°
            'left_gear_joint': (0, 1.57)         # 0° to +90°
        }
        
        # Send individual servo commands
        for i, joint_name in enumerate(msg.name):
            if joint_name in joint_mapping:
                servo_index = joint_mapping[joint_name]
                joint_position = msg.position[i]
                
                # Get joint limits
                if joint_name in joint_limits:
                    min_rad, max_rad = joint_limits[joint_name]
                    # Clamp joint position to limits
                    joint_position = max(min_rad, min(max_rad, joint_position))
                
                # Convert from radians to degrees
                angle_deg = joint_position * 180.0 / 3.14159
                
                # Map to servo range (0-180 degrees)
                if joint_name in ['joint_1', 'joint_2', 'joint_3', 'joint_4']:
                    # Full range joints: map -180° to +180° -> 0° to 180°
                    servo_angle = (angle_deg + 180.0) / 2.0
                elif joint_name == 'gripper_base_joint':
                    # Gripper base: map -90° to +90° -> 0° to 180°
                    servo_angle = (angle_deg + 90.0) / 180.0 * 180.0
                elif joint_name == 'left_gear_joint':
                    # Left gear: map 0° to +90° -> 90° to 180° (or adjust as needed)
                    servo_angle = 90.0 + angle_deg
                else:
                    # Default mapping
                    servo_angle = (angle_deg + 180.0) / 2.0
                
                # Constrain to servo range (0-180 degrees)
                servo_angle = max(0, min(180, servo_angle))
                
                command = f'SERVO,{servo_index},{servo_angle:.1f}'
                self.send_serial_command(command)
                
                self.get_logger().info(f'Joint {joint_name}: {joint_position:.3f} rad -> Servo {servo_index}: {servo_angle:.1f}°')
    
    def digital_outputs_callback(self, msg: Int32MultiArray):
        """Handle digital output commands"""
        if self.emergency_stop and self.emergency_stop_enabled:
            return
        
        # Send digital commands (assuming pin,value pairs)
        for i in range(0, len(msg.data), 2):
            if i + 1 < len(msg.data):
                pin = msg.data[i]
                value = msg.data[i + 1]
                command = f'DIGITAL,{pin},{value}'
                self.send_serial_command(command)
    
    def pwm_commands_callback(self, msg: Float32MultiArray):
        """Handle PWM commands"""
        if self.emergency_stop and self.emergency_stop_enabled:
            return
        
        # Send PWM commands (assuming pin,value pairs)
        for i in range(0, len(msg.data), 2):
            if i + 1 < len(msg.data):
                pin = int(msg.data[i])
                value = int(msg.data[i + 1])
                command = f'PWM,{pin},{value}'
                self.send_serial_command(command)
    
    def publish_data(self):
        """Publish sensor data to ROS2"""
        # Publish joint states
        self.joint_states.header.stamp = self.get_clock().now().to_msg()
        self.joint_state_pub.publish(self.joint_states)
        
        # Publish analog sensors
        analog_msg = Int32MultiArray()
        analog_msg.data = self.analog_values
        self.analog_sensors_pub.publish(analog_msg)
        
        # Publish digital sensors
        digital_msg = Int32MultiArray()
        digital_msg.data = self.digital_values
        self.digital_sensors_pub.publish(digital_msg)
        
        # Publish status
        status_msg = String()
        status_msg.data = f'emergency_stop:{self.emergency_stop},connected:{self.serial_connection is not None}'
        self.status_pub.publish(status_msg)
    
    def reset_emergency_stop(self):
        """Reset emergency stop"""
        self.emergency_stop = False
        self.send_serial_command('RESET_EMERGENCY')
        self.get_logger().info('Emergency stop reset')
    
    def destroy_node(self):
        """Cleanup on node destruction"""
        self.running = False
        if self.serial_thread:
            self.serial_thread.join(timeout=1.0)
        if self.serial_connection:
            self.serial_connection.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    
    bridge = ArduinoROS2Bridge()
    
    try:
        rclpy.spin(bridge)
    except KeyboardInterrupt:
        pass
    finally:
        bridge.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
