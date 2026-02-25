from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'mobile_manipulator_hardware'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Ved',
    maintainer_email='ved@todo.todo',
    description='Unified hardware interface for mobile manipulator',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'hardware_node = mobile_manipulator_hardware.hardware_node:main',
        ],
    },
)
