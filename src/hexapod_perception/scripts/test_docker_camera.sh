#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Test RealSense D455 camera access from within Docker container.
# This script should be run inside the hexapod Docker container.

set -euo pipefail

readonly C_OK=$'\033[1;32m'
readonly C_WARN=$'\033[1;33m'
readonly C_ERR=$'\033[1;31m'
readonly C_OFF=$'\033[0m'

ok() { printf '%s✓%s %s\n' "${C_OK}" "${C_OFF}" "$*"; }
warn() { printf '%s⚠%s %s\n' "${C_WARN}" "${C_OFF}" "$*"; }
error() { printf '%s✗%s %s\n' "${C_ERR}" "${C_OFF}" "$*"; }

echo "Docker Container Camera Access Test"
echo "===================================="
echo

# Check if running in container
if [ -f /.dockerenv ]; then
    ok "Running inside Docker container"
else
    error "Not running in Docker container"
    echo "  Run this script inside the container:"
    echo "  docker compose exec hexapod bash"
    echo "  Then run: ./src/hexapod_perception/scripts/test_docker_camera.sh"
    exit 1
fi

# Check user groups
echo "Current user: $(whoami)"
echo "Groups: $(groups)"
if groups | grep -q '\bvideo\b'; then
    ok "User is in 'video' group"
else
    error "User is NOT in 'video' group"
    exit 2
fi

# Check video devices
if ls /dev/video* >/dev/null 2>&1; then
    ok "Video devices accessible: $(find /dev -maxdepth 1 -name 'video*' | wc -l) device(s)"
else
    warn "No /dev/video* devices found in container"
    echo "  Check compose.yaml device_cgroup_rules for 'c 81:* rmw'"
fi

# Check USB bus
if [ -d /dev/bus/usb ]; then
    ok "USB bus mounted at /dev/bus/usb"
    usb_count=$(find /dev/bus/usb -type c 2>/dev/null | wc -l)
    echo "  USB devices accessible: $usb_count"
else
    error "USB bus not mounted"
    echo "  Check compose.yaml volumes for /dev/bus/usb"
    exit 2
fi

# Try to detect RealSense
if command -v rs-enumerate-devices >/dev/null 2>&1; then
    echo
    echo "Attempting to enumerate RealSense devices..."
    if timeout 5 rs-enumerate-devices 2>&1 | grep -q "Intel RealSense"; then
        ok "RealSense camera successfully detected from container"
        exit 0
    else
        warn "No RealSense camera detected"
        echo "  If camera is connected to host, check:"
        echo "  1. USB device passthrough in compose.yaml"
        echo "  2. Device permissions on host system"
        exit 1
    fi
else
    warn "rs-enumerate-devices not available in container"
    echo "  Install with: apt-get install librealsense2-utils"
    exit 1
fi
