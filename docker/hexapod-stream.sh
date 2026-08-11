#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Run the RTSP streaming node by name rather than by path.
#
# MediaMTX starts its `runOnDemand` commands by looking them up on the PATH, and
# config/mediamtx.yml names this wrapper instead of spelling out where the
# workspace happens to be installed. Where that is belongs here, next to the
# entrypoint that already knows, rather than in a ROS config file that would
# then only be correct inside this image.
#
# ROS is already on the path: MediaMTX inherits its environment from the launch
# file, which inherits it from entrypoint.sh.

set -euo pipefail

readonly NODE=/opt/hexapod/share/hexapod_perception/scripts/stream_topic.py

if [ ! -f "${NODE}" ]; then
  echo "hexapod-stream: ${NODE} is missing, hexapod_perception was not installed" >&2
  exit 1
fi

exec python3 "${NODE}" "$@"
