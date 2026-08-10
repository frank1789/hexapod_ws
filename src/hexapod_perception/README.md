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

## Validation Scripts

The package includes scripts to validate camera permissions and Docker access:

### On the host system

```sh
# Check camera permissions and detection
./src/hexapod_perception/scripts/check_camera_permissions.sh
```

### Inside Docker container

```sh
# First, enter the running container
docker compose -f docker/compose.yaml exec hexapod bash

# Then run the validation script
./src/hexapod_perception/scripts/test_docker_camera.sh
```

## Troubleshooting

### Camera not detected

The ROS node will **not crash** if the camera is not connected. It will launch successfully
but won't publish any data. To diagnose camera issues:

```sh
# Check if camera is connected (host)
rs-enumerate-devices

# Check permissions (host)
ls -l /dev/video*
groups  # user should be in 'video' group

# Run validation script
./src/hexapod_perception/scripts/check_camera_permissions.sh
```

### Permission denied

Add your user to the video group:

```sh
sudo usermod -aG video $USER
# Log out and back in for changes to take effect
```

### Camera works on host but not in Docker

1. Check device cgroup rules in `docker/compose.yaml`
2. Verify `/dev/bus/usb` is mounted
3. Run the Docker validation script:

```sh
docker compose -f docker/compose.yaml exec hexapod \
  ./src/hexapod_perception/scripts/test_docker_camera.sh
```

### ROS node crashes with camera error

The launch file is designed to prevent crashes. If ROS still crashes:

1. Disable the camera: `enable_camera:=false`
2. Check ROS logs: `ros2 topic list` and `ros2 node list`
3. Verify RealSense packages: `ros2 pkg list | grep realsense`
