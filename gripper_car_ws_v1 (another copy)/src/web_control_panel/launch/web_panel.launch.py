"""
Web Control Panel Launch File
=============================
Launches all services required for the web control panel:
- rosbridge_websocket (ROS2 to WebSocket bridge)
- web_video_server (Camera streaming)
- camera_ros (Camera driver)
- web_server (Flask web server)

Usage:
    ros2 launch web_control_panel web_panel.launch.py
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description."""

    # Launch arguments
    web_port_arg = DeclareLaunchArgument(
        'web_port',
        default_value='5000',
        description='Flask web server port'
    )

    rosbridge_port_arg = DeclareLaunchArgument(
        'rosbridge_port',
        default_value='9090',
        description='ROSBridge WebSocket port'
    )

    video_port_arg = DeclareLaunchArgument(
        'video_port',
        default_value='8080',
        description='Web video server port'
    )

    camera_format_arg = DeclareLaunchArgument(
        'camera_format',
        default_value='RGB888',
        description='Camera pixel format'
    )

    # ROSBridge WebSocket Server
    rosbridge_node = Node(
        package='rosbridge_server',
        executable='rosbridge_websocket',
        name='rosbridge_websocket',
        output='screen',
        parameters=[{
            'port': LaunchConfiguration('rosbridge_port'),
            'address': '',
            'retry_startup_delay': 5.0,
        }]
    )

    # Web Video Server
    video_server_node = Node(
        package='web_video_server',
        executable='web_video_server',
        name='web_video_server',
        output='screen',
        parameters=[{
            'port': LaunchConfiguration('video_port'),
            'address': '0.0.0.0',
            'server_threads': 2,
            'ros_threads': 2,
        }]
    )

    # Camera Node (camera_ros for Pi Camera)
    camera_node = Node(
        package='camera_ros',
        executable='camera_node',
        name='camera',
        output='screen',
        parameters=[{
            'format': LaunchConfiguration('camera_format'),
            'width': 640,
            'height': 480,
        }],
        remappings=[
            ('image_raw', '/camera/image_raw'),
            ('camera_info', '/camera/camera_info'),
        ]
    )

    # Flask Web Server Node
    web_server_node = Node(
        package='web_control_panel',
        executable='web_server',
        name='web_server',
        output='screen',
        parameters=[{
            'port': LaunchConfiguration('web_port'),
            'host': '0.0.0.0',
            'rosbridge_port': LaunchConfiguration('rosbridge_port'),
            'video_server_port': LaunchConfiguration('video_port'),
        }]
    )

    return LaunchDescription([
        web_port_arg,
        rosbridge_port_arg,
        video_port_arg,
        camera_format_arg,
        rosbridge_node,
        video_server_node,
        camera_node,
        web_server_node,
    ])
