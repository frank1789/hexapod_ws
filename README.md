# hexapod_ws

ROS 2 workspace for an eighteen degree-of-freedom hexapod. Poses come from an
animation package over ZeroMQ or from a PS3 joypad, and drive eighteen servos
through two Adafruit PCA9685 boards on the I²C bus of a Raspberry Pi.

![How the nodes interact](doc/images/data-flow.svg)

The joypad has the last word: touching it freezes the joints wherever the last
pose left them, and the streamed poses resume once it has been quiet. See
[the ZeroMQ bridge](doc/zeromq-bridge.md).

- [Requirements](#requirements)
- [Build](#build)
- [Container](#container)
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
| C++ packages | [vcpkg](https://vcpkg.io) manages `fmt`, `zeromq`, `cppzmq`, `eigen3`, `nlohmann-json` — see [`vcpkg.json`](vcpkg.json) |
| System libraries | `liblua5.3-dev`, `libi2c-dev`, and `libzmq3-dev`, `libfmt-dev`, `nlohmann-json3-dev` when building without vcpkg |
| Fetched at configure time | [sol2](https://github.com/ThePhD/sol2) and [cppzmq](https://github.com/zeromq/cppzmq), when not already installed |
| Workstation | Python with `pyzmq`, inside Maya or Blender |
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

### On a Raspberry Pi

One command takes a freshly flashed Raspberry Pi 4 Model B or 5 to the point
where the workspace builds — packages, ROS 2, `rosdep`, the I²C interface, and a
build sized for the memory the board actually has:

```sh
./scripts/install-raspberrypi.sh          # --dry-run first, if you prefer
```

Run it as your normal user, not with `sudo`. It is idempotent, so re-running it
after a failure or a reboot only does the work still missing.

The robot must run **Ubuntu Server 24.04 LTS, 64-bit**. Raspberry Pi OS is
Debian based and `packages.ros.org` publishes no suite for it, so ROS 2 cannot
be installed from apt there at all — the script checks and stops rather than
leaving a half-configured system. See
[the Raspberry Pi notes](doc/raspberry-pi.md) for the flags, what resolves
itself, and what to do when a step fails.

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

To build against vcpkg rather than the system packages, point CMake at its
toolchain — this is what the container image does:

```sh
colcon build --cmake-args \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

Install the git hooks once per clone (the dev container does it for you):

```sh
pre-commit install
pre-commit run --all-files
```

## Container

The image builds the workspace and ships only what is needed to run it, which
is the intended way to put the robot on a Pi without installing a toolchain
there:

```sh
docker compose up -d --build          # the robot
docker compose --profile dry up       # no I2C hardware
```

Configuration is environment variables in the compose file, turned into ROS
parameters by the launch file, so changing the ZeroMQ endpoint or the joypad
override timeout is an edit and a restart rather than a rebuild. Details in
[running in a container](doc/docker.md).

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

The servo node needs access to the I²C bus.
[`scripts/install-raspberrypi.sh`](scripts/install-raspberrypi.sh) does this as
part of the setup: it enables the interface, loads `i2c-dev` at boot and adds
you to the `i2c` and `input` groups. By hand, the same thing is:

```sh
sudo raspi-config          # Interface Options -> I2C -> enable
sudo usermod -aG i2c "$USER"
```

Either way a reboot — or at least a fresh login — is required before the group
takes effect.

Check that both boards answer before running anything:

```sh
sudo apt install i2c-tools
i2cdetect -y 1             # expect 40 and 41
```

If only `40` appears, the second board still has its stock address: solder its
`A0` jumper. See [the wiring notes](doc/pca9685.md#wiring).

## Run

Everything at once — joystick driver, remapper, ZeroMQ bridge and servos:

```sh
ros2 launch hexapod_bridge hexapod.launch.py
```

Each part can be left out with `with_servos:=false`, `with_joypad:=false` or
`with_bridge:=false`. The older joypad-only launch still exists:

```sh
ros2 launch hexapod_joypad hexapod_joypad.launch.py
```

Sending poses from a workstation, with the sender that runs inside Maya or
Blender:

```sh
python3 tools/hexapod_pose_sender.py --endpoint tcp://raspberrypi.local:5556
ros2 topic echo /joint_command          # on the robot, to watch them arrive
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
| `hexapod_bridge` | Receives poses over ZeroMQ, arbitrates with the joypad |
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
| [Setting up a Raspberry Pi](doc/raspberry-pi.md) | The install script, the supported image, I²C, build memory, troubleshooting |
| [Running in a container](doc/docker.md) | The multistage image, compose, vcpkg, what the container is given |
| [The ZeroMQ bridge](doc/zeromq-bridge.md) | The animation link, the joypad override, the message format, parameters |
| [Maya and Blender transport](doc/maya-blender-bridge.md) | Why ZeroMQ rather than gRPC, and where the bridge belongs |
| [The PCA9685 servo board](doc/pca9685.md) | PWM generation, registers, timing, wiring, driver validation |
| [Configuring the robot](doc/configuration.md) | Node parameters, the Lua scripts, tuning, reading the logs |
| [Architecture](doc/architecture.md) | Packages, topics, failure behaviour |
| [CLAUDE.md](CLAUDE.md) | Coding standard and working rules for this repository |

## Licence

MIT — see [LICENSE](LICENSE). Copyright © 2021-2026 Francesco Argentieri.
