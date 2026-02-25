"""
Launch file for Naval Inspection Robot web control panel.

Launches:
  1. rosbridge_websocket (port 9090)
  2. YDLiDAR driver (/scan topic)
  3. naval_obstacle_avoidance (/cmd_vel_raw -> /cmd_vel)
  4. naval_serial_bridge (serial -> Arduino)
  5. naval_web_server (Flask on port 5000)

Camera server is started separately via start_naval.sh (standalone, no ROS2).
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, LifecycleNode
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_share = get_package_share_directory('naval_web_control')
    params_file = os.path.join(pkg_share, 'config', 'naval_params.yaml')

    use_lidar_arg = DeclareLaunchArgument(
        'use_lidar', default_value='true',
        description='Launch YDLiDAR driver and obstacle avoidance')

    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port', default_value='/dev/ttyUSB0',
        description='YDLiDAR serial port')

    return LaunchDescription([
        use_lidar_arg,
        lidar_port_arg,

        # ROSBridge WebSocket server
        Node(
            package='rosbridge_server',
            executable='rosbridge_websocket',
            name='rosbridge_websocket',
            parameters=[{'port': 9090}],
            output='screen'
        ),

        # YDLiDAR driver
        LifecycleNode(
            package='ydlidar_ros2_driver',
            executable='ydlidar_ros2_driver_node',
            name='ydlidar',
            output='screen',
            parameters=[{
                'port': LaunchConfiguration('lidar_port'),
                'baudrate': 115200,
                'lidar_type': 1,
                'isSingleChannel': True,
                'support_motor_dtr': True,
                'frame_id': 'laser_frame',
                'angle_max': 180.0,
                'angle_min': -180.0,
                'range_max': 64.0,
                'range_min': 0.01,
                'frequency': 10.0,
            }],
            namespace='/',
            condition=IfCondition(LaunchConfiguration('use_lidar'))
        ),

        # Obstacle avoidance (intercepts /cmd_vel_raw -> /cmd_vel)
        Node(
            package='naval_web_control',
            executable='obstacle_avoidance',
            name='naval_obstacle_avoidance',
            parameters=[params_file],
            output='screen',
            condition=IfCondition(LaunchConfiguration('use_lidar'))
        ),

        # Serial bridge to Arduino
        Node(
            package='naval_web_control',
            executable='serial_bridge',
            name='naval_serial_bridge',
            parameters=[params_file],
            output='screen'
        ),

        # Flask web server
        Node(
            package='naval_web_control',
            executable='web_server',
            name='naval_web_server',
            parameters=[params_file],
            output='screen'
        ),
    ])
