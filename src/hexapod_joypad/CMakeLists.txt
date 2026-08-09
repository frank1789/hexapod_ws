cmake_minimum_required(VERSION 3.16)
project(hexapod_joypad)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
# Strict ISO C++20: no GNU extensions, so the code stays portable across
# compilers (-std=c++20 rather than -std=gnu++20).
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Static analysis --------------------------------------------------------
# clang-tidy runs as part of the compilation, never as a git hook: a check that
# needs the full include path belongs where the include path exists. Disable it
# with -DHEXAPOD_ENABLE_CLANG_TIDY=OFF for a quick build.
option(HEXAPOD_ENABLE_CLANG_TIDY "Run clang-tidy while compiling" ON)
if(HEXAPOD_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
  else()
    message(STATUS "clang-tidy not found: static analysis is disabled")
  endif()
endif()

# One compile_commands.json per package; scripts/merge-compile-commands.sh joins
# them into a single file at the root of the workspace for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

# The value remapping is plain arithmetic with no ROS in it, so it lives in its
# own library that both the node and the tests link against.
add_library(joypad_input STATIC src/joypad_button.cc src/joypad_thumbstick.cc
                                src/joypad_trigger.cc)
target_include_directories(joypad_input PUBLIC src)

add_executable(${PROJECT_NAME}_node src/controller_node.cc src/joypad.cc)

target_include_directories(${PROJECT_NAME}_node PRIVATE src)

target_link_libraries(
  ${PROJECT_NAME}_node PRIVATE joypad_input rclcpp::rclcpp
                               ${sensor_msgs_TARGETS} ${hexapod_msgs_TARGETS})

install(TARGETS ${PROJECT_NAME}_node DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_joypad_input test/test_joypad_input.cpp)
  if(TARGET test_joypad_input)
    target_link_libraries(test_joypad_input joypad_input)
  endif()
endif()

ament_package()
