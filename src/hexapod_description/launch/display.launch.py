"""Publish the hexapod URDF and show it in RViz."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    gui = LaunchConfiguration("gui")
    rviz = LaunchConfiguration("rviz")
    rviz_config = LaunchConfiguration("rviz_config")

    package_share = get_package_share_directory("hexapod_description")
    urdf_path = os.path.join(package_share, "urdf", "Hexapod.urdf")
    rviz_config_path = os.path.join(package_share, "rviz", "hexapod.rviz")

    with open(urdf_path, "r", encoding="utf-8") as urdf_file:
        robot_description = urdf_file.read()

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "gui",
                default_value="true",
                description="start joint_state_publisher_gui to move the joints",
            ),
            DeclareLaunchArgument(
                "rviz",
                default_value="true",
                description="start RViz",
            ),
            DeclareLaunchArgument(
                # Without a configuration RViz opens with no RobotModel display
                # and a Fixed Frame this model does not have, so the scene is
                # empty until it is set up by hand.
                "rviz_config",
                default_value=rviz_config_path,
                description="RViz configuration file to open",
            ),
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                name="robot_state_publisher",
                output="screen",
                parameters=[{"robot_description": robot_description}],
            ),
            Node(
                package="joint_state_publisher_gui",
                executable="joint_state_publisher_gui",
                name="joint_state_publisher_gui",
                condition=IfCondition(gui),
            ),
            Node(
                package="rviz2",
                executable="rviz2",
                name="rviz2",
                output="screen",
                arguments=["-d", rviz_config],
                condition=IfCondition(rviz),
            ),
        ]
    )
