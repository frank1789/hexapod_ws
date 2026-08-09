cmake_minimum_required(VERSION 3.16)
project(hexapod_joypad)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
# Strict ISO C++20: no GNU extensions, so the code stays portable across
# compilers (-std=c++20 rather than -std=gnu++20).
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(
  ${PROJECT_NAME}_node
  src/controller_node.cc src/joypad.cc src/joypad_button.cc
  src/joypad_thumbstick.cc src/joypad_trigger.cc)

target_include_directories(${PROJECT_NAME}_node PRIVATE src)

target_link_libraries(
  ${PROJECT_NAME}_node PRIVATE rclcpp::rclcpp ${sensor_msgs_TARGETS}
                               ${hexapod_msgs_TARGETS})

install(TARGETS ${PROJECT_NAME}_node DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
