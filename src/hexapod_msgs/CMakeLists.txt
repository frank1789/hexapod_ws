cmake_minimum_required(VERSION 3.16)
project(hexapod_msgs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# The three messages use primitive field types only, so no interface
# dependencies are declared.
rosidl_generate_interfaces(${PROJECT_NAME} msg/JoypadButton.msg
                           msg/JoypadThumbstick.msg msg/JoypadTrigger.msg)

ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_messages test/test_messages.cpp)
  if(TARGET test_messages)
    # Link the C++ interfaces this package generates, so the test uses the very
    # headers the consumers include.
    rosidl_get_typesupport_target(cpp_typesupport ${PROJECT_NAME}
                                  rosidl_typesupport_cpp)
    target_link_libraries(test_messages ${cpp_typesupport})
  endif()
endif()

ament_package()
