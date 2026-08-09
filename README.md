# hexapod_ws

ROS 2 workspace for an 18-DoF hexapod: a PS3 joypad drives 18 servos through two Adafruit PCA9685
boards on the I²C bus.

- [hexapod_ws](#hexapod_ws)
  - [Build](#build)
  - [Package hexapod_joypad](#package-hexapod_joypad)
    - [Map buttons](#map-buttons)
    - [Map axes](#map-axes)
    - [Connect PS3 joypad](#connect-ps3-joypad)
  - [Hexapod Messages](#hexapod-messages)
  - [Package hexapod_servomotor](#package-hexapod_servomotor)
  - [Package hexapod_description](#package-hexapod_description)

## Build

The supported environment is the dev container in `.devcontainer/` (Ubuntu 24.04, ROS 2 Jazzy, clang).
Upgrading ROS 2 or clang means changing `ROS_DISTRO` / `LLVM_VERSION` in `devcontainer.json` and
rebuilding the image — nothing else is version-specific.

```sh
./build_exapod.sh          # colcon build of the whole workspace, Release
source install/setup.bash
```

## Package hexapod_joypad

The hexapod_joypad package remaps the raw values published by the ROS 2
[joy](https://index.ros.org/p/joy/) node into semantic messages.

> The ROS 1 setup used [ps3joy](http://wiki.ros.org/ps3joy), which has no ROS 2 release. The generic
> `joy` node reads the controller from `/dev/input/js<N>` once it is paired over Bluetooth; the
> button and axis layout below is unchanged.

- The values of the back triggers vary between 0.0 and 1.0
- Thumbsticks now follow the Cartesian axis convention, their values vary between -1.0 and 1.0
moving them from left to right. Likewise from bottom to top. Consequently the neutral position corresponds to 0.0.

To run the package just run the command from the terminal.

```sh
ros2 launch hexapod_joypad hexapod_joypad.launch.py

# without the servomotor node, when the I2C hardware is not connected
ros2 launch hexapod_joypad hexapod_joypad.launch.py with_servos:=false

# a different joystick device, i.e. /dev/input/js1
ros2 launch hexapod_joypad hexapod_joypad.launch.py device_id:=1
```

### Map buttons

The messages from the joy node:

```sh
buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
```

| Position  | Symbol | Description |
|:---------:|:------:|:------------|
|    1      | X      | cross button |
|    2      | O      | circle button |
|    3      | T      | triangle button |
|    4      | Q      | square button |
|    5      | L1     |
|    6      | R1     |
|    7      | L2     |
|    8      | R2     |
|    9      | SE     | select button |
|   10      | ST     | start button |
|   11      | PS     | PS button |
|   12      | L3     |   |
|   13      | R3     |   |
|   14      | UP     | cross directional up
|   15      | DW     | cross directional down
|   16      | RT     | cross directional right
|   17      | LT     | cross directional left

### Map axes

The messages from the joy node:

```sh
axes: [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
```

| Position  | Symbol | Description |
|:---------:|:------:|:------------|
|   1       | L3     | x axis   |
|   2       | L3     | y axis   |
|   3       | L2     | trigger  |
|   4       | R3     | x axis   |
|   5       | R3     | y axis   |
|   6       | R2     | trigger  |

### Connect PS3 joypad

Follow the [guide](https://pimylifeup.com/raspberry-pi-playstation-controllers/).

## Hexapod Messages

To subscribe to topics use the following strings:

- Joypad
  - joypad/button
  - joypad/thumbstick
  - joypad/trigger

example for reading a custom message from the joypad:

```cpp
#include "hexapod_msgs/msg/joypad_trigger.hpp"

const std::string topic_btn{"joypad/button"};
const std::string topic_tbs{"joypad/thumbstick"};
const std::string topic_trg{"joypad/trigger"};

// inside a class deriving from rclcpp::Node
subscriber_ = create_subscription<hexapod_msgs::msg::JoypadTrigger>(
    topic_trg, 10, std::bind(&Clss::functionCallback, this, std::placeholders::_1));
```

Inspect them at runtime with:

```sh
ros2 topic echo /joypad/thumbstick
```

## Package hexapod_servomotor

Drives the servos. The motor table (name → pin) is generated at runtime by `config/motors.lua` and
the rest angles come from `config/homing.lua`, so both can be retuned without recompiling. Node
parameters live in `config/servoconfiguration.yaml`: `i2c_bus`, `left_driver_address`,
`right_driver_address`, `pwm_frequency`.

```sh
ros2 launch hexapod_servomotor hexapod_servomotor.launch.py
ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args --log-level debug
```

The node opens the I²C bus on startup and fails with a clear error when the hardware is missing.

## Package hexapod_description

URDF model and meshes.

```sh
ros2 launch hexapod_description display.launch.py           # RViz + joint sliders
ros2 launch hexapod_description display.launch.py gui:=false rviz:=false
```
