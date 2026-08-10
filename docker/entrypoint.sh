#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Put ROS and the workspace overlay on the path, then run whatever the image was
# asked to run. Sourcing here rather than in the Dockerfile means `docker run`
# with an arbitrary command gets the same environment the default one does.

set -euo pipefail

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO}/setup.bash"

# --merge-install puts everything under one prefix, so there is a single file to
# source rather than one per package.
if [ -f /opt/hexapod/setup.bash ]; then
  # shellcheck source=/dev/null
  source /opt/hexapod/setup.bash
else
  echo "entrypoint: /opt/hexapod/setup.bash is missing, the workspace was not installed" >&2
  exit 1
fi

exec "$@"
