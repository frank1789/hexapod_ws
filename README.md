# hexapod_ws

ROS 2 workspace for an eighteen degree-of-freedom hexapod: a PS3 joypad drives
eighteen servos through two Adafruit PCA9685 boards on the I²C bus of a
Raspberry Pi.

![From the joypad to the joints](doc/images/data-flow.svg)

- [Requirements](#requirements)
- [Build](#build)
- [Test](#test)
- [Install](#install)
- [Run](#run)
- [Packages](#packages)
- [Documentation](#documentation)
- [Licence](#licence)

## Requirements

| | |
|---|---|
| ROS 2 | Jazzy (any distribution works; nothing hard-codes one) |
| Compiler | C++20, strict ISO, no GNU extensions |
| System libraries | `liblua5.3-dev`, `libi2c-dev` |
| Fetched at configure time | [sol2](https://github.com/ThePhD/sol2), when not already installed |
| Hardware | Raspberry Pi with I²C enabled, two PCA9685 boards, separate 5 V servo supply |

The supported environment is the dev container in
[`.devcontainer`](.devcontainer), which pins Ubuntu 24.04, ROS 2 Jazzy and
clang. Upgrading either means changing `ROS_DISTRO` or `LLVM_VERSION` in
`devcontainer.json` and rebuilding the image; nothing else is version-specific.

Outside the container, install the dependencies with `rosdep`:

```sh
source /opt/ros/$ROS_DISTRO/setup.bash
rosdep install --from-paths src --ignore-src -y
```

## Build

```sh
./build_exapod.sh                 # colcon build of the whole workspace, Release
```

or, for one package at a time:

```sh
source /opt/ros/$ROS_DISTRO/setup.bash
colcon build --symlink-install --packages-select hexapod_msgs
colcon build --symlink-install --packages-select hexapod_servomotor
```

`--symlink-install` is worth having: the Lua scripts and the launch files are
read from the install tree, so with symlinks an edit takes effect without
rebuilding.

Install the git hooks once per clone (the dev container does it for you):

```sh
pre-commit install
pre-commit run --all-files
```

## Test

```sh
colcon test --event-handlers console_direct-
colcon test-result --all --verbose
```

**No test needs the robot.** They cover the joypad value remapping, the motor
model and the angle-to-pulse mapping, the PCA9685 frequency arithmetic taken
from the data sheet and its argument validation, the generated message fields,
and the URDF together with every mesh it refers to. Anything that would open
`/dev/i2c-*` is deliberately out of scope, so the suite runs on a laptop.

Run one package on its own with `--packages-select`, for instance:

```sh
colcon test --packages-select hexapod_servomotor
```

`colcon test` exits quietly when a package has nothing to run, so read the
counts from `colcon test-result` rather than trusting the exit code.

## Install

The workspace installs into `install/` and is used as an overlay:

```sh
source install/setup.bash
```

Add it to your shell profile to get it in every terminal:

```sh
echo "source $(pwd)/install/setup.bash" >> ~/.bashrc
```

### On the robot

The servo node needs access to the I²C bus. Enable the interface and add
yourself to the `i2c` group — logging out and back in is required for the group
to take effect:

```sh
sudo raspi-config          # Interface Options -> I2C -> enable
sudo usermod -aG i2c "$USER"
```

Check that both boards answer before running anything:

```sh
sudo apt install i2c-tools
i2cdetect -y 1             # expect 40 and 41
```

If only `40` appears, the second board still has its stock address: solder its
`A0` jumper. See [the wiring notes](doc/pca9685.md#wiring).

## Run

Everything at once — joystick driver, remapper and servos:

```sh
ros2 launch hexapod_joypad hexapod_joypad.launch.py
```

Without the hardware, on a development machine:

```sh
ros2 launch hexapod_joypad hexapod_joypad.launch.py with_servos:=false
ros2 topic echo /joypad/thumbstick
```

Individual pieces:

```sh
# servos only, with the parameters from config/
ros2 launch hexapod_servomotor hexapod_servomotor.launch.py

# the URDF model in RViz, with joint sliders
ros2 launch hexapod_description display.launch.py

# a different joystick device, i.e. /dev/input/js1
ros2 launch hexapod_joypad hexapod_joypad.launch.py device_id:=1

# verbose, showing every register write
ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args --log-level debug
```

> **Bench test.** `perform_startup_test:=true` sweeps every joint across its
> whole travel at startup. It is off by default because it knocks an assembled
> robot over. Only enable it with the body supported and the legs free.

The node validates its configuration and the I²C bus before moving anything, and
exits with a message rather than a guess if either is wrong. On exit — including
a crash — both boards switch their outputs off, so the joints are left
unpowered instead of holding their last command.

## Packages

| Package | Role |
|---|---|
| `hexapod_msgs` | `JoypadButton`, `JoypadThumbstick`, `JoypadTrigger` |
| `hexapod_joypad` | Remaps `sensor_msgs/msg/Joy` into those messages |
| `hexapod_servomotor` | Drives the servos through two PCA9685 boards |
| `hexapod_description` | URDF model and meshes |

Subscribing to the remapped topics:

```cpp
#include "hexapod_msgs/msg/joypad_trigger.hpp"

// inside a class deriving from rclcpp::Node
subscriber_ = create_subscription<hexapod_msgs::msg::JoypadTrigger>(
    "joypad/trigger", 10,
    std::bind(&Clss::functionCallback, this, std::placeholders::_1));
```

The PS3 button and axis layout is documented in
[the architecture notes](doc/architecture.md#the-joypad-chain). To pair the
controller, follow [this guide](https://pimylifeup.com/raspberry-pi-playstation-controllers/).

## Documentation

| Document | Contents |
|---|---|
| [The PCA9685 servo board](doc/pca9685.md) | PWM generation, registers, timing, wiring, driver validation |
| [Configuring the robot](doc/configuration.md) | Node parameters, the Lua scripts, tuning, reading the logs |
| [Architecture](doc/architecture.md) | Packages, topics, failure behaviour |
| [CLAUDE.md](CLAUDE.md) | Coding standard and working rules for this repository |

## Licence

MIT — see [LICENSE](LICENSE). Copyright © 2021-2026 Francesco Argentieri.
