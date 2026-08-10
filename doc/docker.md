# Running in a container

The image builds the workspace and ships only what is needed to run it. It is
the intended way to put the robot on a Raspberry Pi 4 Model B or a Raspberry
Pi 5 without installing a toolchain there.

```sh
docker compose up -d --build      # the robot
docker compose --profile dry up   # no I2C hardware
```

## Two stages

```
┌─ builder ────────────────────────────────┐    ┌─ runtime ──────────────────┐
│ ros:jazzy-ros-base                       │    │ ros:jazzy-ros-base         │
│  + compiler, cmake, ninja, git           │    │  + libi2c0, liblua5.3-0    │
│  + vcpkg  ── fmt, ZeroMQ, cppzmq,        │──▶ │  + joy, robot_state_pub    │
│              Eigen, nlohmann-json        │    │                            │
│  + colcon build --merge-install          │    │  COPY /ws/install          │
└──────────────────────────────────────────┘    └────────────────────────────┘
      discarded: sources, objects, vcpkg              shipped to the robot
```

The compiler, vcpkg's build trees and the workspace sources never reach the
runtime image. vcpkg links its libraries statically, so fmt, ZeroMQ and the rest
need no runtime packages at all; only Lua and I2C come from the distribution.

## Layer order is the maintenance story

`vcpkg.json` is copied **before** `src/`, on purpose. Editing a source file
invalidates the layers after the copy, but not the one that compiled the
third-party libraries — so a rebuild after changing the bridge is quick, while a
rebuild after changing a dependency is not. That is the right way round.

## Building for the Pi from a workstation

The Dockerfile picks the vcpkg triplet from `uname -m`, so the same file builds
natively on the robot and cross-builds under emulation:

```sh
docker buildx build --platform linux/arm64 -f docker/Dockerfile -t hexapod:latest .
```

Building on the Pi itself is simpler and needs no emulation, but compiling
ZeroMQ and the ROS packages there takes a while — and on a 2 GB Pi 4B the
parallelism has to be capped, which
[`install-raspberrypi.sh`](../scripts/install-raspberrypi.sh) does.

## What the container is given

| Given | Why |
|---|---|
| `/dev/i2c-1` | The two PCA9685 boards |
| `/dev/input` (read-only) + cgroup rule `c 13:*` | The joystick, whose device number changes when it is re-paired |
| Port 5556 | Where the bridge binds for the animation package |
| `shm_size: 256m` | DDS shared-memory transport; the 64 MB default is tight |

The devices are listed one by one rather than running the container privileged,
so its reach stops at what the robot actually needs. The process inside runs as
an unprivileged user in the `i2c` and `input` groups.

`ROS_LOCALHOST_ONLY=1` keeps DDS inside the container. The only traffic on the
network is the ZeroMQ link — one unicast TCP connection, which behaves over
WiFi in a way DDS discovery does not.

## Configuration

Nothing robot-specific is baked into the image. Every value in the compose file
is an environment variable that the launch file turns into a ROS parameter
override:

| Variable | Overrides | Default |
|---|---|---|
| `HEXAPOD_BRIDGE_ENDPOINT` | `endpoint` | `tcp://0.0.0.0:5556` |
| `HEXAPOD_BRIDGE_BIND` | `bind` | `true` |
| `HEXAPOD_BRIDGE_OUTPUT_TOPIC` | `output_topic` | `joint_command` |
| `HEXAPOD_BRIDGE_POLL_PERIOD_MS` | `poll_period_ms` | `5` |
| `HEXAPOD_BRIDGE_OVERRIDE_TIMEOUT_MS` | `override_timeout_ms` | `1500` |
| `HEXAPOD_BRIDGE_THUMBSTICK_DEADZONE` | `thumbstick_deadzone` | `0.25` |

Changing one is an edit and a restart, not a rebuild. The defaults behind them
live in `src/hexapod_bridge/config/bridge.yaml`, which is the single source of
truth — the compose file only overrides what varies between deployments.

A variable that is set but cannot be read stops the launch instead of falling
back to the file value, so a typo in the compose file surfaces immediately.

## The dry profile

`--profile dry` runs the same image with `with_servos:=false` and
`with_joypad:=false`. It opens the ZeroMQ port and publishes `joint_command`
without touching the I2C bus, which is what to use on a workstation, or on a Pi
before anything is wired.

## Health

The health check asks the ROS graph, not the process table:

```sh
ros2 topic list | grep -q joint_command
```

A node that has crashed on start-up still leaves a running container if you only
check for a process; checking that the topic exists catches it.

## Upgrading ROS

One line:

```sh
ROS_DISTRO=kilted docker compose build
```

`ROS_DISTRO` is a build argument threaded through both stages, and nothing in
the Dockerfile hard-codes a distribution — the same property the dev container
has.
