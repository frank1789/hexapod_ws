"""Launch the servomotor node alone, with the parameters from config/."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    parameters = os.path.join(
        get_package_share_directory("hexapod_servomotor"),
        "config",
        "servoconfiguration.yaml",
    )

    return LaunchDescription(
        [
            Node(
                package="hexapod_servomotor",
                executable="hexapod_servomotor_node",
                name="servomotors_node",
                output="screen",
                parameters=[parameters],
            ),
        ]
    )
