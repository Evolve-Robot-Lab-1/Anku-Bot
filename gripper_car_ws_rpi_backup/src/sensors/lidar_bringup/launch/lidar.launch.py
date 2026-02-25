"""Launch file for YDLiDAR."""

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    """Generate launch description for LiDAR."""
    ydlidar_pkg = get_package_share_directory('ydlidar_ros2_driver')

    # Include the ydlidar launch
    ydlidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ydlidar_pkg, 'launch', 'ydlidar_launch.py')
        )
    )

    return LaunchDescription([
        ydlidar_launch,
    ])
