#!/usr/bin/env bash
# -*- coding: utf-8 -*-
#
# Check RealSense D455 camera permissions and connectivity.
# This script verifies that the system has proper permissions to access the camera.
#
# Exit codes:
#   0 - Camera detected and accessible
#   1 - Camera not detected but permissions are OK
#   2 - Permission issues detected

set -euo pipefail

readonly C_OK=$'\033[1;32m'
readonly C_WARN=$'\033[1;33m'
readonly C_ERR=$'\033[1;31m'
readonly C_OFF=$'\033[0m'

ok() { printf '%s✓%s %s\n' "${C_OK}" "${C_OFF}" "$*"; }
warn() { printf '%s⚠%s %s\n' "${C_WARN}" "${C_OFF}" "$*"; }
error() { printf '%s✗%s %s\n' "${C_ERR}" "${C_OFF}" "$*"; }

echo "RealSense D455 Camera Permission Check"
echo "======================================"
echo

# Check if user is in video group
if groups | grep -q '\bvideo\b'; then
    ok "User is in 'video' group"
else
    error "User is NOT in 'video' group"
    echo "  Fix: sudo usermod -aG video $USER"
    echo "  Then log out and back in"
    exit 2
fi

# Check if video devices exist
if ls /dev/video* >/dev/null 2>&1; then
    ok "Video devices found: $(find /dev -maxdepth 1 -name 'video*' | wc -l) device(s)"
    for dev in /dev/video*; do
        echo "    - $dev"
    done
else
    warn "No /dev/video* devices found"
    echo "  This is normal if the camera is not connected"
fi

# Check USB bus access
if [ -d /dev/bus/usb ]; then
    ok "USB bus accessible at /dev/bus/usb"
else
    error "USB bus not accessible"
    exit 2
fi

# Try to enumerate RealSense devices if rs-enumerate-devices is available
if command -v rs-enumerate-devices >/dev/null 2>&1; then
    echo
    echo "Enumerating RealSense devices..."
    if rs-enumerate-devices 2>/dev/null | grep -q "Intel RealSense"; then
        ok "Intel RealSense camera detected"
        rs-enumerate-devices 2>/dev/null | grep "Device info" || true
        exit 0
    else
        warn "No RealSense cameras detected"
        echo "  This is expected if the camera is not connected"
        echo "  The ROS node will launch but won't publish data"
        exit 1
    fi
else
    warn "rs-enumerate-devices not found (install librealsense2-utils)"
    echo "  Cannot verify camera detection, but permissions look OK"
    exit 1
fi
