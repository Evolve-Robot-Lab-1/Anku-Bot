from setuptools import setup
import os
from glob import glob

package_name = 'naval_web_control'

setup(
    name=package_name,
    version='0.2.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # Launch files
        (os.path.join('share', package_name, 'launch'),
            glob('launch/*.py')),
        # Config files
        (os.path.join('share', package_name, 'config'),
            glob('config/*.yaml')),
        # Static files - CSS
        (os.path.join('share', package_name, 'static', 'css'),
            glob('static/css/*.css')),
        # Static files - JS
        (os.path.join('share', package_name, 'static', 'js'),
            glob('static/js/*.js')),
        # Templates
        (os.path.join('share', package_name, 'templates'),
            glob('templates/*.html')),
        # Scripts
        (os.path.join('share', package_name, 'scripts'),
            glob('scripts/*.py')),
    ],
    install_requires=['setuptools', 'flask', 'flask-cors'],
    zip_safe=True,
    maintainer='User',
    maintainer_email='user@todo.todo',
    description='Web control panel for Naval inspection robot',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'web_server = naval_web_control.naval_web_server:main',
            'serial_bridge = naval_web_control.naval_serial_bridge:main',
            'obstacle_avoidance = naval_web_control.naval_obstacle_avoidance:main',
        ],
    },
)
