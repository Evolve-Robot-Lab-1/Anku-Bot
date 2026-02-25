"""
Robot Bringup Launch File
==========================
Launches the complete mobile manipulator stack:
- Serial Bridge (Arduino communication)
- LiDAR driver
- Web Control Panel (rosbridge, camera, video server, flask)
- Obstacle Avoidance

Usage:
    ros2 launch robot_bringup robot.launch.py
    ros2 launch robot_bringup robot.launch.py use_obstacle_avoidance:=false
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for robot."""

    # ===== LAUNCH ARGUMENTS =====
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyACM0',
        description='Arduino serial port'
    )

    lidar_port_arg = DeclareLaunchArgument(
        'lidar_port',
        default_value='/dev/ttyUSB0',
        description='LiDAR serial port'
    )

    use_lidar_arg = DeclareLaunchArgument(
        'use_lidar',
        default_value='true',
        description='Launch LiDAR driver'
    )

    use_camera_arg = DeclareLaunchArgument(
        'use_camera',
        default_value='true',
        description='Launch camera driver'
    )

    use_obstacle_avoidance_arg = DeclareLaunchArgument(
        'use_obstacle_avoidance',
        default_value='true',
        description='Launch obstacle avoidance'
    )

    use_web_panel_arg = DeclareLaunchArgument(
        'use_web_panel',
        default_value='true',
        description='Launch web control panel'
    )

    # ===== SERIAL BRIDGE (Arduino Communication) =====
    serial_bridge_node = Node(
        package='base_controller',
        executable='serial_bridge_node.py',
        name='serial_bridge',
        output='screen',
        parameters=[{
            'serial_port': LaunchConfiguration('serial_port'),
            'baud_rate': 115200,
        }]
    )

    # ===== LIDAR DRIVER =====
    lidar_node = Node(
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
        }],
        condition=IfCondition(LaunchConfiguration('use_lidar'))
    )

    # ===== OBSTACLE AVOIDANCE =====
    obstacle_avoidance_node = Node(
        package='base_controller',
        executable='obstacle_avoidance_node.py',
        name='obstacle_avoidance',
        output='screen',
        parameters=[{
            'obstacle_threshold': 0.2,
            'clear_threshold': 0.3,
            'front_sector_angle': 30.0,
        }],
        condition=IfCondition(LaunchConfiguration('use_obstacle_avoidance'))
    )

    # ===== WEB CONTROL PANEL =====
    # ROSBridge WebSocket Server
    rosbridge_node = Node(
        package='rosbridge_server',
        executable='rosbridge_websocket',
        name='rosbridge_websocket',
        output='screen',
        parameters=[{
            'port': 9090,
            'address': '',
            'retry_startup_delay': 5.0,
        }],
        condition=IfCondition(LaunchConfiguration('use_web_panel'))
    )

    # Web Video Server
    video_server_node = Node(
        package='web_video_server',
        executable='web_video_server',
        name='web_video_server',
        output='screen',
        parameters=[{
            'port': 8080,
            'address': '0.0.0.0',
            'server_threads': 2,
            'ros_threads': 2,
        }],
        condition=IfCondition(LaunchConfiguration('use_web_panel'))
    )

    # Camera Node
    camera_node = Node(
        package='camera_ros',
        executable='camera_node',
        name='camera',
        output='screen',
        parameters=[{
            'format': 'RGB888',
            'width': 640,
            'height': 480,
        }],
        remappings=[
            ('image_raw', '/camera/image_raw'),
            ('camera_info', '/camera/camera_info'),
        ],
        condition=IfCondition(LaunchConfiguration('use_camera'))
    )

    # Flask Web Server
    web_server_node = Node(
        package='web_control_panel',
        executable='web_server',
        name='web_server',
        output='screen',
        parameters=[{
            'port': 5000,
            'host': '0.0.0.0',
            'rosbridge_port': 9090,
            'video_server_port': 8080,
        }],
        condition=IfCondition(LaunchConfiguration('use_web_panel'))
    )

    return LaunchDescription([
        # Arguments
        serial_port_arg,
        lidar_port_arg,
        use_lidar_arg,
        use_camera_arg,
        use_obstacle_avoidance_arg,
        use_web_panel_arg,

        # Core nodes
        serial_bridge_node,
        lidar_node,
        obstacle_avoidance_node,

        # Web panel nodes
        rosbridge_node,
        video_server_node,
        camera_node,
        web_server_node,
    ])
