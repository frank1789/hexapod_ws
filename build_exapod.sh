#!/bin/bash -e
# -*- coding: utf-8 -*-
#
# Build the whole workspace. colcon resolves the inter-package order itself, so
# hexapod_msgs is generated before the nodes that depend on it.

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"

colcon build \
  --symlink-install \
  --cmake-args -DCMAKE_BUILD_TYPE=Release \
  "$@"

# clangd and a standalone clang-tidy both want a single database at the root.
"$(dirname "${BASH_SOURCE[0]}")/scripts/merge-compile-commands.sh"

echo
echo "Done. Source the overlay with:  source install/setup.bash"
