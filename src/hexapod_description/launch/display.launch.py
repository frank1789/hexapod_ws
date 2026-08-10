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

    package_share = get_package_share_directory("hexapod_description")
    urdf_path = os.path.join(package_share, "urdf", "Hexapod.urdf")

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
                condition=IfCondition(rviz),
            ),
        ]
    )
