cmake_minimum_required(VERSION 3.26)
project(hexapod_model LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
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
set(CMAKE_COLOR_DIAGNOSTICS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- ROS dependencies -------------------------------------------------------
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)

# --- Third-party libraries --------------------------------------------------
# Eigen is the linear algebra behind every function in this package. vcpkg is
# the intended source: vcpkg.json pins eigen3, and a build configured with the
# vcpkg toolchain resolves it out of /opt/vcpkg_installed.
#
# Configuring without vcpkg still works — ROS pulls lib eigen3-dev in
# transitively, and it installs the same Eigen3 config package under
# /usr/share/eigen3/cmake — so a native build on the Pi needs nothing extra.
# With the vcpkg toolchain the manifest copy wins, which is the point.
find_package(Eigen3 CONFIG REQUIRED)
message(STATUS "Eigen3 from ${Eigen3_DIR}")

# --- Lua runtime ------------------------------------------------------------
# The leg geometry lives in config/legs.lua, for the same reason the motor table
# lives in config/motors.lua: re-tuning a chassis must not need a rebuild. The
# arrangement below is deliberately identical to hexapod_servomotor's, so a
# dependency behaves the same way in both packages.
#
# config/legs.lua is plain Lua 5.1 — no integer division, no goto, no bitwise
# operators, no integer subtype — so it runs unchanged on either interpreter.
# Keep it that way: LuaJIT tracks 5.1, so a 5.3-only construct would build here
# and fail on the robot.
option(HEXAPOD_ENABLE_LUAJIT "Use LuaJIT when it is available" ON)

set(HEXAPOD_LUA_INCLUDE_DIRS "")
set(HEXAPOD_LUA_LIBRARIES "")
set(HEXAPOD_LUA_FLAVOUR "")

if(HEXAPOD_ENABLE_LUAJIT)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(LUAJIT QUIET IMPORTED_TARGET luajit)
  endif()
  if(LUAJIT_FOUND)
    set(HEXAPOD_LUA_LIBRARIES PkgConfig::LUAJIT)
    set(HEXAPOD_LUA_FLAVOUR "LuaJIT ${LUAJIT_VERSION}")
  endif()
endif()

if(NOT HEXAPOD_LUA_FLAVOUR)
  # No LuaJIT, or it was turned off: the reference interpreter, which is what
  # apt provides on the Raspberry Pi.
  find_package(Lua 5.3 REQUIRED)
  set(HEXAPOD_LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
  set(HEXAPOD_LUA_LIBRARIES ${LUA_LIBRARIES})
  set(HEXAPOD_LUA_FLAVOUR "Lua ${LUA_VERSION_STRING}")
endif()

message(STATUS "Lua runtime: ${HEXAPOD_LUA_FLAVOUR}")

# sol2 is header-only: use the packaged version when there is one — vcpkg
# installs it — otherwise fetch the pinned revision.
find_package(sol2 CONFIG QUIET)
if(sol2_FOUND)
  # A packaged sol2 exports the namespaced target; the fetched tree defines the
  # bare one. Both spellings are in the wild, so ask rather than assume.
  if(TARGET sol2::sol2)
    set(HEXAPOD_SOL2_TARGET sol2::sol2)
  else()
    set(HEXAPOD_SOL2_TARGET sol2)
  endif()
  message(STATUS "sol2 found through its CMake config")
else()
  include(FetchContent)
  FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG eba86625b707e3c8c99bbfc4624e51f42dc9e561)
  FetchContent_MakeAvailable(sol2)
  set(HEXAPOD_SOL2_TARGET sol2)
  message(STATUS "sol2 fetched at the pinned revision")
endif()

# --- Libraries --------------------------------------------------------------
# The kinematics are free of ROS, of Lua and of any measurement of a particular
# robot, so the tests exercise exactly the maths the robot runs with nothing to
# stand up first.
set(LIBRARY_TARGET_NAME hexapod_model)
add_library(${LIBRARY_TARGET_NAME} STATIC)
target_sources(${LIBRARY_TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leg_model.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/model.cc
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/body_pose.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hip_mount.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/leg_model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/model.h)

# PUBLIC: the headers include <Eigen/Core> and <Eigen/Geometry>, so anything
# linking this library needs the same include path. Linking the namespaced
# target is what propagates it — finding Eigen without linking it leaves
# "Eigen/Dense: No such file or directory" at the first compile.
target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC Eigen3::Eigen)

# The only part of the package that reads a file or knows about ROS.
set(CONFIG_TARGET_NAME ${LIBRARY_TARGET_NAME}_config)
add_library(${CONFIG_TARGET_NAME} STATIC)
target_sources(${CONFIG_TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration_loader.cc
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration_loader.h)
target_include_directories(${CONFIG_TARGET_NAME}
                           PRIVATE ${HEXAPOD_LUA_INCLUDE_DIRS})
target_link_libraries(
  ${CONFIG_TARGET_NAME} PUBLIC ${LIBRARY_TARGET_NAME}
                               ament_index_cpp::ament_index_cpp)
target_link_libraries(${CONFIG_TARGET_NAME}
                      PRIVATE ${HEXAPOD_LUA_LIBRARIES} ${HEXAPOD_SOL2_TARGET})

# sol2 keys its LuaJIT support off this macro. Its own detection reads
# LUAJIT_VERSION, which is only defined once <luajit.h> has been included — and
# sol2 only includes that header after deciding it is on LuaJIT. Saying so here
# breaks the circle, rather than depending on which lua.hpp the include path
# happens to resolve first on a machine carrying both interpreters.
if(LUAJIT_FOUND)
  target_compile_definitions(${CONFIG_TARGET_NAME} PRIVATE SOL_LUAJIT=1)
endif()

# --- Executable -------------------------------------------------------------
set(EXECUTABLE_TARGET_NAME "${LIBRARY_TARGET_NAME}_node")
add_executable(${EXECUTABLE_TARGET_NAME}
               ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc)
target_link_libraries(
  ${EXECUTABLE_TARGET_NAME} PRIVATE ${CONFIG_TARGET_NAME} rclcpp::rclcpp)

# clang-tidy re-parses each unit with clang, and the pinned sol2 revision does
# not compile with it: optional_implementation.hpp calls this->construct() in a
# template, which clang rejects under two-phase lookup. GCC builds it fine, so
# the unit that includes sol2 is left out of the analysis rather than moving to
# an unpinned dependency. The kinematics are still analysed in full.
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/configuration_loader.cc
                            PROPERTIES SKIP_LINTING ON)

install(TARGETS ${EXECUTABLE_TARGET_NAME} DESTINATION lib/${PROJECT_NAME})
install(
  TARGETS ${LIBRARY_TARGET_NAME} ${CONFIG_TARGET_NAME}
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  FILE_SET HEADERS
  DESTINATION include/${PROJECT_NAME})
# legs.lua is read from the share directory at run time, so it has to be there.
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  set(TEST_TARGET_NAME test_${PROJECT_NAME})
  ament_add_gtest(${TEST_TARGET_NAME} test/model.test.cpp)
  # if(TARGET ...), not if(${TEST_TARGET_NAME}): the bare form tests the *value*
  # of a variable that was never set, so it is always false and the test would
  # link against nothing.
  if(TARGET ${TEST_TARGET_NAME})
    target_link_libraries(${TEST_TARGET_NAME} ${CONFIG_TARGET_NAME})
  endif()
endif()

ament_package()
