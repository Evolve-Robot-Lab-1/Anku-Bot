cmake_minimum_required(VERSION 3.8)
project(mobile_base_description)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

# Install launch, config directories
install(DIRECTORY launch urdf config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
