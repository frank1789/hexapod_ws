cmake_minimum_required(VERSION 3.16)
project(hexapod_servomotor)

# std::string::starts_with is used by the driver dispatch.
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
find_package(ament_index_cpp REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(Lua 5.3 REQUIRED)

# sol2 is header-only: use the system package when available, otherwise fetch
# the pinned revision.
find_package(sol2 QUIET)
if(NOT sol2_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG eba86625b707e3c8c99bbfc4624e51f42dc9e561)
  FetchContent_MakeAvailable(sol2)
endif()

add_executable(
  ${PROJECT_NAME}_node src/main.cc src/servocontroller.cc src/i2c-peripheral.cc
                       src/PCA9685_driver.cc src/motor.cc)

target_include_directories(${PROJECT_NAME}_node PRIVATE include
                                                        ${LUA_INCLUDE_DIR})

target_link_libraries(
  ${PROJECT_NAME}_node
  PRIVATE rclcpp::rclcpp ament_index_cpp::ament_index_cpp
          ${hexapod_msgs_TARGETS} ${LUA_LIBRARIES} sol2 i2c)

install(TARGETS ${PROJECT_NAME}_node DESTINATION lib/${PROJECT_NAME})

# The Lua scripts are read at runtime through
# ament_index_cpp::get_package_share_directory(), so they must be installed.
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_package()
