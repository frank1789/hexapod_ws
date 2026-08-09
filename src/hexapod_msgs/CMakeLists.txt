cmake_minimum_required(VERSION 3.16)
project(hexapod_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# The three messages use primitive field types only, so no interface
# dependencies are declared.
rosidl_generate_interfaces(${PROJECT_NAME} msg/JoypadButton.msg
                           msg/JoypadThumbstick.msg msg/JoypadTrigger.msg)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
