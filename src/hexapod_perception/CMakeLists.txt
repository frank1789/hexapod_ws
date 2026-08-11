cmake_minimum_required(VERSION 3.8)
project(hexapod_perception)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

# Install launch files and scripts
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME}/)

install(PROGRAMS scripts/check_camera_permissions.sh
                 scripts/test_docker_camera.sh scripts/stream_topic.py
        DESTINATION share/${PROJECT_NAME}/scripts)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
