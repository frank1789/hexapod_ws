#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Start a headless X server inside the dev container and export it to a browser
# through noVNC. Open http://localhost:6080/vnc.html once it reports ready.
#
# Docker Desktop on macOS passes neither a GPU nor an X socket into its virtual
# machine, so the container brings its own display. RViz then renders through
# Mesa's llvmpipe on the CPU and only the changed regions of the screen cross
# the boundary — which is far less traffic than forwarding X11 to XQuartz, and
# needs nothing installed on the Mac.
#
# Nothing here touches the robot: this is a display, not a node. It is started
# by the postStartCommand in devcontainer.json and is safe to re-run — every
# step checks whether it is already running.
#
# No `set -u`: sourcing anything from ROS trips over its own undefined
# variables, and this script is sourced by no-one but must stay consistent with
# the others in this directory.
set -eo pipefail

DISPLAY_NUMBER="${DISPLAY_NUMBER:-99}"
# 1280x800 by default rather than something larger: every pixel is drawn by the
# CPU, and this is the size a 2016-2017 MacBook Pro keeps smooth. Override with
# GEOMETRY=1920x1200x24 on a machine that can afford it.
GEOMETRY="${GEOMETRY:-1280x800x24}"
VNC_PORT="${VNC_PORT:-5901}"
WEB_PORT="${WEB_PORT:-6080}"

export DISPLAY=":${DISPLAY_NUMBER}"

require() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "!! $1 is not installed: rebuild the dev container rather than reopening it" >&2
    exit 1
  }
}

require Xvfb
require x11vnc
require websockify
require xdpyinfo

# --- the display ------------------------------------------------------------
if xdpyinfo >/dev/null 2>&1; then
  echo "==> display ${DISPLAY} is already up"
else
  echo "==> starting Xvfb on ${DISPLAY} at ${GEOMETRY}"
  Xvfb ":${DISPLAY_NUMBER}" -screen 0 "${GEOMETRY}" >/tmp/xvfb.log 2>&1 &

  # Wait for the server to answer rather than sleeping a fixed amount: a guess
  # that is long enough on an idle laptop is not long enough on a busy one.
  for _ in $(seq 1 50); do
    xdpyinfo >/dev/null 2>&1 && break
    sleep 0.2
  done

  xdpyinfo >/dev/null 2>&1 || {
    echo "!! Xvfb did not come up; see /tmp/xvfb.log" >&2
    exit 1
  }
fi

# --- window manager ---------------------------------------------------------
# Without one, RViz and the joint slider window both open at the top-left corner
# with no title bar, overlapping and impossible to move.
if pgrep -x fluxbox >/dev/null; then
  echo "==> window manager already running"
else
  echo "==> starting fluxbox"
  fluxbox >/tmp/fluxbox.log 2>&1 &
fi

# --- VNC --------------------------------------------------------------------
# -nopw is deliberate. The port is published to the host loopback only, by the
# forwardPorts entry in devcontainer.json; it is not reachable from the network.
#
# -wait and -defer are both in milliseconds and cap how often the screen is
# polled and how long an update is held back. 30 ms is about 33 frames a second,
# which is above what RViz produces under software rendering and well below what
# an older laptop can spend on polling.
if pgrep -x x11vnc >/dev/null; then
  echo "==> x11vnc already running"
else
  echo "==> starting x11vnc on ${VNC_PORT}"
  x11vnc -display "${DISPLAY}" \
    -forever -shared -nopw -quiet \
    -rfbport "${VNC_PORT}" \
    -wait 30 -defer 30 \
    >/tmp/x11vnc.log 2>&1 &
fi

# --- noVNC ------------------------------------------------------------------
if pgrep -f "websockify.*${WEB_PORT}" >/dev/null; then
  echo "==> noVNC already running"
else
  echo "==> starting noVNC on ${WEB_PORT}"
  websockify --web=/usr/share/novnc "${WEB_PORT}" "localhost:${VNC_PORT}" \
    >/tmp/websockify.log 2>&1 &
fi

# --- report -----------------------------------------------------------------
for _ in $(seq 1 25); do
  pgrep -f "websockify.*${WEB_PORT}" >/dev/null && break
  sleep 0.2
done

cat <<EOF

    Display  ${DISPLAY}  (${GEOMETRY})
    Browser  http://localhost:${WEB_PORT}/vnc.html
             http://localhost:${WEB_PORT}/vnc_lite.html   lighter, no toolbar

    Then, in a terminal:
      ros2 launch hexapod_description display.launch.py

EOF
