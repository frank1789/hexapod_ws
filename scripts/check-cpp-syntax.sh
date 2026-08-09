#!/usr/bin/env bash
# Parse every C++ translation unit without generating code, so a file that
# cannot compile is rejected before it reaches the repository. Formatters do not
# do this: clang-format happily reformats code that does not build.
#
# The nodes include rclcpp, the generated hexapod_msgs headers and sol2, so the
# check needs a sourced ROS installation and a workspace that has been built at
# least once. It skips with a notice otherwise, which is the case on a bare host.
set -euo pipefail

readonly STANDARD=c++20

# Prefer the compiler colcon actually builds with, so the check agrees with the
# build. clang++ is a fallback: the pinned sol2 revision does not parse with it.
if command -v g++ >/dev/null 2>&1; then
  COMPILER=g++
elif command -v clang++ >/dev/null 2>&1; then
  COMPILER=clang++
else
  echo "note: no C++ compiler found, skipping the C++ syntax check" >&2
  exit 0
fi
readonly COMPILER

if [ -z "${ROS_DISTRO:-}" ] || [ ! -d "/opt/ros/${ROS_DISTRO}/include" ]; then
  echo "note: no sourced ROS installation, skipping the C++ syntax check" >&2
  echo "      run it in the dev container: pre-commit run cpp-syntax --all-files" >&2
  exit 0
fi

if [ ! -d install ]; then
  echo "note: no install/ directory, skipping the C++ syntax check" >&2
  echo "      build the workspace once first: ./build_exapod.sh" >&2
  exit 0
fi

# ROS 2 keeps one include directory per package, and rosidl nests the generated
# headers one level deeper still: install/<pkg>/include/<pkg>/<pkg>/msg/*.hpp.
# Both levels have to be on the search path.
includes=("-I/usr/include/lua5.3")
while IFS= read -r directory; do
  includes+=("-isystem" "${directory}")
done < <(
  find "/opt/ros/${ROS_DISTRO}/include" -maxdepth 1 -mindepth 1 -type d
  find install -maxdepth 2 -mindepth 2 -type d -name include
  find install -maxdepth 3 -mindepth 3 -type d -path '*/include/*'
)

status=0

check_package() {
  local include_dir="$1"
  local source_dir="$2"
  local source

  while IFS= read -r source; do
    if ! "${COMPILER}" -fsyntax-only "-std=${STANDARD}" -I"${include_dir}" \
      "${includes[@]}" "${source}"; then
      status=1
    fi
  done < <(find "${source_dir}" -name '*.cc')
}

check_package src/hexapod_joypad/src src/hexapod_joypad/src
check_package src/hexapod_servomotor/include src/hexapod_servomotor/src

if [ "${status}" -eq 0 ]; then
  echo "C++ syntax check passed"
fi

exit "${status}"
