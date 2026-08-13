cmake_minimum_required(VERSION 3.16)
project(hexapod_bridge)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
# Strict ISO C++20: no GNU extensions, so the code stays portable across
# compilers (-std=c++20 rather than -std=gnu++20).
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Static analysis --------------------------------------------------------
# clang-tidy runs as part of the compilation, never as a git hook. Disable it
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hexapod_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

# --- Third-party libraries --------------------------------------------------
# vcpkg is the intended source: vcpkg.json at the root of the workspace pins
# fmt, nlohmann-json, ZeroMQ and cppzmq, and a build configured with the vcpkg
# toolchain finds all four through their CMake config packages.
#
# Configuring without vcpkg still works, so a native build on the Pi does not
# have to compile the world: fmt and nlohmann-json come from apt (libfmt-dev,
# nlohmann-json3-dev), ZeroMQ from libzmq3-dev through pkg-config, and cppzmq —
# a single header that Debian does not package — is fetched, the same treatment
# sol2 already gets in hexapod_servomotor.
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(HEXAPOD_ZMQ_INCLUDE_DIRS "")

find_package(cppzmq CONFIG QUIET)
if(cppzmq_FOUND)
  set(HEXAPOD_ZMQ_TARGET cppzmq)
  message(STATUS "cppzmq found through its CMake config")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBZMQ REQUIRED IMPORTED_TARGET libzmq)

  include(FetchContent)
  FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG v4.10.0)
  set(CPPZMQ_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(cppzmq)

  # cppzmq is a single header, and its own CMakeLists puts the include directory
  # on an INTERFACE target that also links a `libzmq` CMake target. That target
  # only exists when ZeroMQ's config package was found — which is precisely the
  # case this branch is here to cover — so linking it would fail. Take the
  # header directory and pair it with the pkg-config target instead.
  #
  # Without this the header is downloaded and then never reached: the build
  # failed with "fatal error: zmq.hpp: No such file or directory" while the file
  # sat in _deps/cppzmq-src.
  set(HEXAPOD_ZMQ_TARGET PkgConfig::LIBZMQ)
  set(HEXAPOD_ZMQ_INCLUDE_DIRS "${cppzmq_SOURCE_DIR}")
  message(
    STATUS "cppzmq fetched into ${cppzmq_SOURCE_DIR}; ZeroMQ from pkg-config")
endif()

# --- Libraries --------------------------------------------------------------
# The message format and the override rule are free of ROS and of ZeroMQ, so the
# tests exercise exactly the code the robot runs without opening a socket.
add_library(bridge_core STATIC src/wire_format.cc src/override_policy.cc)
target_include_directories(bridge_core PUBLIC include)
target_link_libraries(bridge_core PUBLIC fmt::fmt nlohmann_json::nlohmann_json)

add_executable(${PROJECT_NAME}_node src/main.cc src/bridge_node.cc)
target_include_directories(${PROJECT_NAME}_node
                           PRIVATE include ${HEXAPOD_ZMQ_INCLUDE_DIRS})
target_link_libraries(
  ${PROJECT_NAME}_node
  PRIVATE bridge_core rclcpp::rclcpp ${hexapod_msgs_TARGETS}
          ${sensor_msgs_TARGETS} ${HEXAPOD_ZMQ_TARGET})

install(TARGETS ${PROJECT_NAME}_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  # Neither test opens a socket or spins a node: they cover the payload
  # validation and the override rule, which is where the decisions are made.
  ament_add_gtest(test_bridge test/test_wire_format.cpp
                  test/test_override_policy.cpp)
  if(TARGET test_bridge)
    target_link_libraries(test_bridge bridge_core)
  endif()
endif()

ament_package()
