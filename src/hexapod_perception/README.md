# hexapod_perception

Intel RealSense D455 depth camera integration for the hexapod robot.

## Features

- Intel RealSense D455 depth camera support
- Graceful handling of missing camera (ROS won't crash if camera is not connected)
- Configurable RGB and depth streams
- Aligned depth to color frame

## Dependencies

- `realsense2_camera` - ROS 2 wrapper for Intel RealSense SDK
- `librealsense2` - Intel RealSense SDK 2.0

## Installation

The RealSense SDK is installed automatically by `scripts/install-raspberrypi.sh`.

For manual installation:

```sh
# Add Intel RealSense repository
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE
sudo add-apt-repository "deb https://librealsense.intel.com/Debian/apt-repo $(lsb_release -cs) main"
sudo apt update

# Install RealSense SDK and ROS wrapper
sudo apt install librealsense2-dkms librealsense2-utils librealsense2-dev
sudo apt install ros-${ROS_DISTRO}-realsense2-camera ros-${ROS_DISTRO}-realsense2-description
```

## Usage

### Launch with camera enabled (default)

```sh
ros2 launch hexapod_perception realsense_d455.launch.py
```

### Launch without camera (for testing)

```sh
ros2 launch hexapod_perception realsense_d455.launch.py enable_camera:=false
```

### Configure streams

```sh
# RGB only
ros2 launch hexapod_perception realsense_d455.launch.py enable_depth:=false

# Depth only
ros2 launch hexapod_perception realsense_d455.launch.py enable_rgb:=false
```

## Topics

When the camera is running, it publishes to:

- `/d455/color/image_raw` - RGB camera stream
- `/d455/depth/image_rect_raw` - Depth image
- `/d455/aligned_depth_to_color/image_raw` - Depth aligned to RGB frame
- `/d455/color/camera_info` - RGB camera intrinsics
- `/d455/depth/camera_info` - Depth camera intrinsics

## Docker

The camera requires USB device access. See `docker/compose.yaml` for the configuration.

## Troubleshooting

### Camera not detected

```sh
# Check if camera is connected
rs-enumerate-devices

# Check permissions
ls -l /dev/video*
groups  # user should be in 'video' group
```

### Permission denied

Add your user to the video group:

```sh
sudo usermod -aG video $USER
# Log out and back in for changes to take effect
```
