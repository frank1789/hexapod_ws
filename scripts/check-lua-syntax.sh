#!/usr/bin/env bash
# Parse the Lua configuration scripts. They are executed at runtime by
# hexapod_servomotor, so a syntax error would only surface on the robot.
#
# Skips with a notice when no Lua interpreter is installed, which is the case on
# a host without the dev container.
set -e

for candidate in luac5.3 luac5.4 luac; do
  if command -v "${candidate}" >/dev/null 2>&1; then
    exec "${candidate}" -p "$@"
  fi
done

echo "note: no luac found, skipping the Lua syntax check (run it in the dev container)" >&2
exit 0
