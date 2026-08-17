#!/usr/bin/env bash
# Runs once, right after the dev container is created (see devcontainer.json).
# Resolves package dependencies and installs the git hooks so that a fresh
# container is immediately ready to build and to commit.
# No `set -u`: sourcing the ROS setup scripts would trip over their own
# undefined variables.
set -o pipefail

# Set AMENT_TRACE_SETUP_FILES to empty if unset to prevent 'unbound variable' errors
export AMENT_TRACE_SETUP_FILES="${AMENT_TRACE_SETUP_FILES:-}"

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO}/setup.bash"

# Both rosdep steps below fetch from raw.githubusercontent.com, which answers
# with a read timeout or an HTTP 429 often enough that the image build used to
# run them and die (see .devcontainer/Dockerfile.ros2). They belong here, where
# a failure costs a warning instead of the whole image.
#
# One bad response is not a broken container, so retry with a widening pause.
# The caller decides what an exhausted retry means; nothing here aborts.
retry() {
  local attempts="$1" label="$2"
  shift 2

  local attempt
  for ((attempt = 1; attempt <= attempts; attempt++)); do
    if "$@"; then
      return 0
    fi
    echo "!! ${label} failed (attempt ${attempt}/${attempts})" >&2
    if [ "${attempt}" -lt "${attempts}" ]; then
      sleep $((attempt * 5))
    fi
  done

  return 1
}

# The image is allowed to build without this file, so it may genuinely be absent.
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  echo "==> Initialising rosdep"
  if ! retry 3 "rosdep init" sudo rosdep init; then
    echo "!! rosdep init failed three times: the default sources list is absent," >&2
    echo "!! so the update below cannot succeed either." >&2
  fi
else
  echo "==> rosdep is already initialised"
fi

echo "==> Updating rosdep database"
if ! retry 3 "rosdep update" rosdep update --rosdistro "${ROS_DISTRO}"; then
  echo "!! rosdep update failed three times: the database is stale or absent." >&2
  echo "!! Continuing — apt and vcpkg already carry every build dependency." >&2
  echo "!! Re-run 'rosdep update' by hand once the network is reachable." >&2
fi

echo "==> Resolving package dependencies"
sudo apt-get update -qq
if ! rosdep install --from-paths src --ignore-src --rosdistro "${ROS_DISTRO}" -y; then
  echo "!! rosdep could not resolve every dependency — see the output above." >&2
  echo "!! Continuing: the container is still usable, but a package may fail to build." >&2
fi

echo "==> Installing pre-commit hooks"
pre-commit install

echo "==> Ready. Build with: colcon build --symlink-install"
