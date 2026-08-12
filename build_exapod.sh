#!/bin/bash -e
# -*- coding: utf-8 -*-
#
# Build the whole workspace. colcon resolves the inter-package order itself, so
# hexapod_msgs is generated before the nodes that depend on it.

# shellcheck source=/dev/null
source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"

# vcpkg is the intended source of the C++ dependencies and is what the dev
# container and docker/Dockerfile both install. Use it when it is there, and
# fall back to the system packages when it is not — a bare host, or a Pi set up
# by scripts/install-raspberrypi.sh, still builds.
#
# VCPKG_MANIFEST_MODE is off on purpose: the manifest was resolved once, for the
# whole workspace, when the image was built. Left on, every colcon package would
# resolve it again for itself.
VCPKG_ARGS=()
if [ -n "${VCPKG_ROOT:-}" ] && [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
  VCPKG_TRIPLET="$(cat /opt/vcpkg_triplet 2>/dev/null \
    || uname -m | sed 's/aarch64/arm64-linux/; s/x86_64/x64-linux/')"
  VCPKG_ARGS=(
    "-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    "-DVCPKG_MANIFEST_MODE=OFF"
    "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED:-/opt/vcpkg_installed}"
    "-DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET}"
  )
  echo "==> building against vcpkg (${VCPKG_TRIPLET})"
else
  echo "==> vcpkg not found, building against the system packages"
fi

colcon build \
  --symlink-install \
  --cmake-args -DCMAKE_BUILD_TYPE=Release "${VCPKG_ARGS[@]}" \
  "$@"

# clangd and a standalone clang-tidy both want a single database at the root.
"$(dirname "${BASH_SOURCE[0]}")/scripts/merge-compile-commands.sh"

echo
echo "Done. Source the overlay with:  source install/setup.bash"
