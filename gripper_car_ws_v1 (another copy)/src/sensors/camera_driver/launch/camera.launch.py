"""
Camera Launch File
==================
Launches v4l2_camera driver and QR scanner node.

Usage:
    ros2 launch camera_driver camera.launch.py
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for camera."""

    pkg_share = get_package_share_directory('camera_driver')

    # Arguments
    video_device_arg = DeclareLaunchArgument(
        'video_device',
        default_value='/dev/video0',
        description='Camera device path'
    )

    # v4l2_camera node
    camera_node = Node(
        package='v4l2_camera',
        executable='v4l2_camera_node',
        name='camera',
        output='screen',
        parameters=[{
            'video_device': LaunchConfiguration('video_device'),
            'image_size': [640, 480],
            'camera_frame_id': 'camera_link_optical',
        }],
        remappings=[
            ('image_raw', '/camera/image_raw'),
            ('camera_info', '/camera/camera_info'),
        ]
    )

    # QR Scanner node
    qr_scanner_node = Node(
        package='camera_driver',
        executable='qr_scanner',
        name='qr_scanner',
        output='screen',
        parameters=[{
            'camera_topic': '/camera/image_raw',
            'scan_rate': 10.0,
        }]
    )

    return LaunchDescription([
        video_device_arg,
        camera_node,
        qr_scanner_node,
    ])
