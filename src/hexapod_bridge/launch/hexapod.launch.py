"""Bring up the whole robot: joystick, remapper, ZeroMQ bridge and servos.

This is the launch file the container runs. The chain it starts is:

    joy_node ──▶ hexapod_joypad ──┐
                                  ├──▶ hexapod_bridge ──▶ hexapod_servomotor
    Maya / Blender ──ZeroMQ───────┘        (joypad freezes the stream)

Each part can be left out: `with_servos:=false` runs everything but the I2C
hardware, `with_joypad:=false` drops the joystick when no controller is paired,
and `with_bridge:=false` leaves the animation link out entirely.

The bridge is included from its own launch file rather than declared again
here, so the environment overrides docker compose relies on are defined in one
place only.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    device_id = LaunchConfiguration("device_id")
    deadzone = LaunchConfiguration("deadzone")
    with_servos = LaunchConfiguration("with_servos")
    with_joypad = LaunchConfiguration("with_joypad")
    with_bridge = LaunchConfiguration("with_bridge")

    bridge_launch = os.path.join(
        get_package_share_directory("hexapod_bridge"),
        "launch",
        "hexapod_bridge.launch.py",
    )
    servo_parameters = os.path.join(
        get_package_share_directory("hexapod_servomotor"),
        "config",
        "servoconfiguration.yaml",
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "device_id",
                default_value="0",
                description="index of the joystick device (/dev/input/js<N>)",
            ),
            DeclareLaunchArgument(
                "deadzone",
                default_value="0.1",
                description="amount of the thumbstick range treated as neutral",
            ),
            DeclareLaunchArgument(
                "with_servos",
                default_value="true",
                description="start the servomotor node (needs the I2C hardware)",
            ),
            DeclareLaunchArgument(
                "with_joypad",
                default_value="true",
                description="start the joystick driver and the remapper",
            ),
            DeclareLaunchArgument(
                "with_bridge",
                default_value="true",
                description="start the ZeroMQ bridge that receives animation poses",
            ),
            Node(
                package="joy",
                executable="joy_node",
                name="joy_node",
                respawn=True,
                condition=IfCondition(with_joypad),
                parameters=[
                    {
                        # A LaunchConfiguration substitutes to a string, so the
                        # target type has to be stated explicitly.
                        "device_id": ParameterValue(device_id, value_type=int),
                        "deadzone": ParameterValue(deadzone, value_type=float),
                        "autorepeat_rate": 20.0,
                    }
                ],
            ),
            Node(
                package="hexapod_joypad",
                executable="hexapod_joypad_node",
                name="controller_node",
                output="screen",
                condition=IfCondition(with_joypad),
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(bridge_launch),
                condition=IfCondition(with_bridge),
            ),
            Node(
                package="hexapod_servomotor",
                executable="hexapod_servomotor_node",
                name="servomotors_node",
                output="screen",
                respawn=True,
                condition=IfCondition(with_servos),
                parameters=[servo_parameters],
            ),
        ]
    )
