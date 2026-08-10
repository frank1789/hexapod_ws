#!/usr/bin/env bash
# Every `docker run` / `docker exec` lands in a fully sourced ROS 2 shell,
# including the workspace overlay once it has been built.
#
# No `set -u`: the ROS setup scripts read variables such as AMENT_TRACE_SETUP_FILES
# without defining them first, and would abort the entrypoint.
set -e

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO}/setup.bash"

if [ -f /workspace/install/setup.bash ]; then
  # shellcheck source=/dev/null
  source /workspace/install/setup.bash
fi

exec "$@"
