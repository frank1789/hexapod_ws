cmake_minimum_required(VERSION 3.26)
project(hexapod_model LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(eigen3 CONFIG REQUIRED)

set(LIBRARY_TARGET_NAME hexapod_model)
add_library(${LIBRARY_TARGET_NAME} STATIC)
target_sources(
  ${LIBRARY_TARGET_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model.cc
          ${CMAKE_CURRENT_SOURCE_DIR}/src/leg_model.cc
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_SOURCE_DIR}/include
         FILES
         ${CMAKE_CURRENT_SOURCE_DIR}/include/body_pose.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/hip_mount.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/leg_model.h)

set(EXECUTABLE_TARGET_NAME "${LIBRARY_TARGET_NAME}_node")
add_executable(${EXECUTABLE_TARGET_NAME}
               ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc)
target_link_libraries(
  ${EXECUTABLE_TARGET_NAME} PRIVATE rclcpp::rclcpp ${sensor_msgs_TARGETS}
                                    ${hexapod_msgs_TARGETS} hexapod_model)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  set(TEST_TARGET_NAME test_${PROJECT_NAME})
  ament_add_gtest(${TEST_TARGET_NAME} test/model.test.cpp)
  if(${TEST_TARGET_NAME})
    target_link_libraries(${TEST_TARGET_NAME} ${LIBRARY_TARGET_NAME})
  endif()
endif()

ament_package()
