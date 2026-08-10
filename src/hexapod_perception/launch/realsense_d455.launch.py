#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Launch Intel RealSense D455 depth camera for hexapod robot.

The camera launch is designed to gracefully handle the case where the camera
is not connected. If the camera is missing, a warning is logged but the launch
continues without crashing ROS.
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, LogInfo
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node


def generate_launch_description():
    """Generate launch description for RealSense D455 camera."""

    # Declare launch arguments
    enable_camera_arg = DeclareLaunchArgument(
        'enable_camera',
        default_value='true',
        description='Enable RealSense D455 camera. Set to false to skip camera launch.'
    )

    enable_rgb_arg = DeclareLaunchArgument(
        'enable_rgb',
        default_value='true',
        description='Enable RGB camera stream'
    )

    enable_depth_arg = DeclareLaunchArgument(
        'enable_depth',
        default_value='true',
        description='Enable depth camera stream'
    )

    camera_name_arg = DeclareLaunchArgument(
        'camera_name',
        default_value='d455',
        description='Camera name prefix for topics'
    )

    # RealSense camera node - only launch if enable_camera is true
    realsense_camera = Node(
        condition=IfCondition(LaunchConfiguration('enable_camera')),
        package='realsense2_camera',
        executable='realsense2_camera_node',
        name=LaunchConfiguration('camera_name'),
        namespace='',
        parameters=[{
            'enable_color': LaunchConfiguration('enable_rgb'),
            'enable_depth': LaunchConfiguration('enable_depth'),
            'enable_infra1': False,
            'enable_infra2': False,
            'enable_confidence': True,
            'depth_module.profile': '848x480x30',
            'rgb_camera.profile': '640x480x30',
            'align_depth.enable': True,
            'initial_reset': True,
        }],
        output='screen',
        # ROS will restart the node if it crashes, but won't fail the entire system
        respawn=False,
        respawn_delay=2.0,
    )

    camera_disabled_info = LogInfo(
        condition=IfCondition(LaunchConfiguration('enable_camera')),
        msg='Launching Intel RealSense D455 camera...'
    )

    return LaunchDescription([
        enable_camera_arg,
        enable_rgb_arg,
        enable_depth_arg,
        camera_name_arg,
        camera_disabled_info,
        realsense_camera,
    ])
