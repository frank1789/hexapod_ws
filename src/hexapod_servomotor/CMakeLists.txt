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
find_package(ament_index_cpp REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
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

# Hardware access and the motor model, without the ROS node around them, so the
# tests can link the same code the robot runs.
add_library(servomotor_core STATIC src/motor.cc src/i2c-peripheral.cc
                                   src/PCA9685_driver.cc)
target_include_directories(servomotor_core PUBLIC include)
target_link_libraries(servomotor_core PUBLIC rclcpp::rclcpp i2c)

add_executable(${PROJECT_NAME}_node src/main.cc src/servocontroller.cc)

# clang-tidy re-parses each unit with clang, and the pinned sol2 revision does
# not compile with it: optional_implementation.hpp calls this->construct() in a
# template, which clang rejects under two-phase lookup. GCC builds it fine, so
# the two units that include sol2 are left out of the analysis rather than
# moving to an unpinned dependency. Everything else, including the whole driver,
# is still analysed.
set_source_files_properties(src/main.cc src/servocontroller.cc
                            PROPERTIES SKIP_LINTING ON)

target_include_directories(${PROJECT_NAME}_node PRIVATE include
                                                        ${LUA_INCLUDE_DIR})

target_link_libraries(
  ${PROJECT_NAME}_node
  PRIVATE servomotor_core ament_index_cpp::ament_index_cpp
          ${hexapod_msgs_TARGETS} ${sensor_msgs_TARGETS} ${LUA_LIBRARIES} sol2)

install(TARGETS ${PROJECT_NAME}_node DESTINATION lib/${PROJECT_NAME})

# The Lua scripts are read at runtime through
# ament_index_cpp::get_package_share_directory(), so they must be installed.
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  # Neither test opens the I2C bus: they cover the motor model, the angle
  # mapping and the argument validation, which all run before any transfer.
  ament_add_gtest(test_servomotor test/test_motor.cpp test/test_pca9685.cpp)
  if(TARGET test_servomotor)
    target_link_libraries(test_servomotor servomotor_core)
  endif()
endif()

ament_package()
