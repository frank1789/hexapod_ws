"""Bring up the whole robot: joystick, remapper, ZeroMQ bridge and servos.

This is the launch file the container runs. The chain it starts is:

    joy_node ──▶ hexapod_joypad ──┐
                                  ├──▶ hexapod_bridge ──▶ hexapod_servomotor
    Maya / Blender ──ZeroMQ───────┘        (joypad freezes the stream)

Every part can be left out, because every part depends on hardware that may not
be attached: `with_servos:=false` drops the I2C boards, `with_joypad:=false` the
joystick, `with_camera:=false` the depth camera, and `with_bridge:=false` the
animation link. Nothing here fails because a device is missing — a node that
needs one either is not started or exits and is respawned.

The camera defaults to off. It is the only part whose absence is the normal
case rather than the exception, and starting it costs USB bandwidth and CPU
that the rest of the stack would rather have.

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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    device_id = LaunchConfiguration("device_id")
    deadzone = LaunchConfiguration("deadzone")
    with_servos = LaunchConfiguration("with_servos")
    with_joypad = LaunchConfiguration("with_joypad")
    with_bridge = LaunchConfiguration("with_bridge")
    with_camera = LaunchConfiguration("with_camera")

    # Resolved when the include runs rather than now, unlike the two paths
    # below: with with_camera:=false the condition is false, the substitution
    # is never evaluated, and hexapod_perception need not be installed at all.
    camera_launch = PathJoinSubstitution(
        [FindPackageShare("hexapod_perception"), "launch", "realsense_d455.launch.py"]
    )

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
            DeclareLaunchArgument(
                "with_camera",
                default_value="false",
                description="start the RealSense D455 (needs the camera on USB 3)",
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
                # The node exits on any I2C failure, by design. Without a delay
                # a board that does not answer — unpowered, unplugged, wrong
                # address — is retried about four times a second for as long as
                # the container runs, which floods the log and hammers the bus.
                respawn_delay=2.0,
                condition=IfCondition(with_servos),
                parameters=[servo_parameters],
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(camera_launch),
                condition=IfCondition(with_camera),
            ),
        ]
    )
