cmake_minimum_required(VERSION 3.8)
project(mobile_arm_manipulator)

find_package(ament_cmake REQUIRED)

# No binaries to build; just install assets
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY urdf 
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY meshes
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
