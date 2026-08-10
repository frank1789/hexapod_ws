"""Launch the joystick driver, the joypad remapper and (optionally) the servos.

The ROS 1 setup used the ps3joy driver; ROS 2 has no release of it, so the
generic `joy` node is used instead — it reads the controller through the Linux
joystick device once it is paired over Bluetooth.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    device_id = LaunchConfiguration("device_id")
    deadzone = LaunchConfiguration("deadzone")
    with_servos = LaunchConfiguration("with_servos")

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
                description="also start the servomotor node (needs the I2C hardware)",
            ),
            Node(
                package="joy",
                executable="joy_node",
                name="joy_node",
                respawn=True,
                parameters=[
                    {
                        # A LaunchConfiguration substitutes to a string, so the
                        # target type has to be stated explicitly: joy declares
                        # device_id as an integer and deadzone as a double.
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
            ),
            Node(
                package="hexapod_servomotor",
                executable="hexapod_servomotor_node",
                name="servomotors_node",
                output="screen",
                respawn=True,
                parameters=[servo_parameters],
                condition=IfCondition(with_servos),
            ),
        ]
    )
