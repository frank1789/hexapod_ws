# -*- coding: utf-8 -*-
"""Serve the camera topics over RTSP, HLS and WebRTC.

This starts one process: MediaMTX. It listens, and on the first viewer it runs
`hexapod-stream` for the path being requested, which subscribes to the topic and
feeds an ffmpeg. Nothing encodes until somebody watches.

    rtsp://<pi>:8554/color     rtsp://<pi>:8554/depth
    http://<pi>:8889/color/    http://<pi>:8889/depth/     (browser)

The camera is a separate switch. Starting this without `with_camera:=true`
brings the server up over topics that nobody publishes: the paths exist, and a
viewer asking for one gets a command that finds no images and gives up. That is
a deliberate ordering rather than an error to guard against — the server is
cheap, and it means the camera can come and go underneath it.

Ports are published by compose; the addresses here are the ones inside the
container.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory("hexapod_perception"),
        "config",
        "mediamtx.yml",
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "with_streaming",
                default_value="true",
                description="start the RTSP server for the camera topics",
            ),
            ExecuteProcess(
                cmd=["mediamtx", config],
                name="mediamtx",
                output="screen",
                # The server holds no state worth preserving and the streams it
                # serves are re-created on demand, so restarting it is always
                # safe and always the right answer.
                respawn=True,
                respawn_delay=2.0,
                condition=IfCondition(LaunchConfiguration("with_streaming")),
            ),
        ]
    )
