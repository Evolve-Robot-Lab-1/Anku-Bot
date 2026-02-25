cmake_minimum_required(VERSION 3.8)
project(base_controller)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)



# Install launch and config files
install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)
# Install helper scripts (Python)
install(PROGRAMS
  scripts/base_hardware_bridge.py
  scripts/serial_bridge_node.py
  scripts/odom_publisher_node.py
  scripts/teleop_keyboard.py
  scripts/obstacle_avoidance_node.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
