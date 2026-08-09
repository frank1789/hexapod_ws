#!/usr/bin/env bash
# Runs once, right after the dev container is created (see devcontainer.json).
# Resolves package dependencies and installs the git hooks so that a fresh
# container is immediately ready to build and to commit.
# No `set -u`: sourcing the ROS setup scripts would trip over their own
# undefined variables.
set -o pipefail

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO}/setup.bash"

echo "==> Updating rosdep database"
rosdep update --rosdistro "${ROS_DISTRO}"

echo "==> Resolving package dependencies"
sudo apt-get update -qq
if ! rosdep install --from-paths src --ignore-src --rosdistro "${ROS_DISTRO}" -y; then
  echo "!! rosdep could not resolve every dependency — see the output above." >&2
  echo "!! Continuing: the container is still usable, but a package may fail to build." >&2
fi

echo "==> Installing pre-commit hooks"
pre-commit install

echo "==> Ready. Build with: colcon build --symlink-install"
