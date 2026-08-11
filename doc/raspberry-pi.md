# Setting up a Raspberry Pi

[`scripts/install-raspberrypi.sh`](../scripts/install-raspberrypi.sh) takes a
freshly flashed Raspberry Pi 4 Model B or Raspberry Pi 5 to the point where
`./build_exapod.sh` succeeds. It installs the packages, resolves the workspace
dependencies with `rosdep`, enables the I²C bus and sizes the build for the
memory the board actually has.

```sh
git clone https://github.com/frank1789/hexapod_ws.git
cd hexapod_ws
./scripts/install-raspberrypi.sh
sudo reboot                       # only if the script asks for it
./build_exapod.sh
```

Run it as your normal user, not with `sudo`: it calls `sudo` for the individual
steps that need it, and the group memberships, `~/.ros` and `~/.colcon` have to
belong to the account that will run the robot.

## Which operating system

**Ubuntu Server 24.04 LTS, 64-bit.** This is not a preference, it is where the
packages exist:

| Image | ROS 2 Jazzy from apt |
|---|---|
| Ubuntu Server 24.04 LTS 64-bit (`noble`) | Yes — `packages.ros.org/ros2/ubuntu` carries a `noble` suite |
| Raspberry Pi OS Bookworm 64-bit | **No** — `packages.ros.org/ros2/debian` has no `bookworm` suite at all |
| Any 32-bit image | **No** — ROS 2 publishes no 32-bit ARM binaries |

Raspberry Pi OS is the trap worth knowing about. The ROS project *does* publish a
`ros-apt-source` package for `bookworm`, so adding the apt source appears to
work; there is simply nothing behind it, and the failure only surfaces later as
`Unable to locate package ros-jazzy-ros-base`. The script checks the repository
index itself rather than the apt source, so it stops immediately and says why.

Flash the card with Raspberry Pi Imager, choosing *Other general-purpose OS →
Ubuntu → Ubuntu Server 24.04 LTS (64-bit)*.

## What the script does

| Step | Detail |
|---|---|
| Checks the machine | Board model, 64-bit userspace, distribution, and that it is run from a `hexapod_ws` checkout |
| Base packages | `build-essential`, `cmake`, `curl`, `git`, `pkg-config`, `python3` |
| Project libraries | `liblua5.3-dev`, `lua5.3`, `libi2c-dev`, `i2c-tools` |
| ROS 2 | Adds the apt source, then `ros-base`, `joy`, `robot_state_publisher`, `xacro`, `ros-dev-tools` |
| `rosdep` | `init`, `update`, then `install --from-paths src` against every `package.xml` |
| I²C | `dtparam=i2c_arm=on`, loads `i2c-dev` at boot, adds you to `i2c` and `input` |
| Bus probe | `i2cdetect` on every bus, looking for the two boards at `0x40` and `0x41` |
| Build size | Writes `~/.colcon/defaults.yaml` with a worker count the memory can support |

`liblua5.3-dev` rather than `lua5.3` alone is deliberate:
`find_package(Lua 5.3 REQUIRED)` needs `lua.h`, which only the development
package ships. sol2 is not installed — `hexapod_servomotor` fetches the pinned
revision at configure time, so the **first** `colcon build` needs the network.

### Options

| Flag | Effect |
|---|---|
| `--with-gui` | Also installs `rviz2` and `joint_state_publisher_gui` |
| `--with-dev-tools` | Also installs clang, clang-tidy, clang-format, cppcheck, `pre-commit` |
| `--skip-hardware` | Leaves the boot configuration, the modules and the groups alone |
| `--ros-distro NAME` | Installs a distribution other than `jazzy` |
| `--dry-run` | Prints every command it would run, changes nothing |
| `--force` | Continues on an unrecognised board or distribution |

The default is the lean set a headless robot needs. Add `--with-gui` only on a
machine with a screen: RViz pulls in a large part of the desktop stack.

### Running it again is safe

Every step checks its own state first, so re-running after a failure, a reboot
or a change of flags only does the work that is still missing. Use `--dry-run`
first if you want to see the plan without committing to it.

## What resolves itself, and what does not

The script is written so that the usual first-boot problems do not need a human:

- **The dpkg lock.** A freshly flashed Pi runs `unattended-upgrades` on first
  boot and holds the lock for minutes. The script waits for it instead of dying
  on *"could not get lock"*.
- **A half-configured dpkg.** If an install was interrupted, it runs
  `dpkg --configure -a` and `apt-get --fix-broken install`, then retries once.
- **Missing ROS dependencies.** `rosdep` maps the `<depend>` keys in every
  `package.xml` onto apt packages, so a dependency added to a manifest later is
  installed by re-running the script — nothing here has to be kept in step by
  hand.
- **Raspberry Pi OS reporting itself as `raspbian`.** `rosdep` has no rules for
  that name, so the script resolves against the Debian release it is built from.

What it deliberately does **not** do:

- **Change the I²C bus speed.** The PCA9685 timing depends on it. The script
  switches the interface on and nothing more.
- **Create swap.** It warns when the board has under 4 GiB of RAM and under
  1 GiB of swap, and prints the three commands, but writing a swap file and
  editing `/etc/fstab` is left to you.
- **Build ROS 2 from source.** On an unsupported image it stops and says what to
  flash rather than starting a multi-hour build.

## After the reboot

A reboot is needed when the script enabled I²C or added you to a group — it
says so at the end. Afterwards:

```sh
i2cdetect -y 1            # expect 40 and 41
```

If only `40` appears the second board still has its stock address: solder its
`A0` jumper, see [the wiring notes](pca9685.md#wiring). If the bus number is not
`1`, set `i2c_bus` to the right device in
[`servoconfiguration.yaml`](configuration.md#node-parameters).

The probe is read-only. `i2cdetect` uses an SMBus read for the `0x40`–`0x41`
range, so it does not write to the boards.

## Build memory

Compiling several `rclcpp` translation units at once is what exhausts a small
Pi: the OOM killer stops the compiler and `colcon` reports an error that blames
the code. The script sizes the workers from the memory that exists and writes
the result to `~/.colcon/defaults.yaml`, so every later `colcon build` picks it
up without a flag.

| Board | Typical result |
|---|---|
| Pi 4B 2 GB | 1 worker — expect a long build, and add swap |
| Pi 4B / Pi 5 4 GB | 1–2 workers |
| Pi 4B / Pi 5 8 GB | 3 workers |

An existing `~/.colcon/defaults.yaml` is never overwritten; the script prints
the value it would have used instead.

## When something fails

| Message | Cause and fix |
|---|---|
| `ROS 2 publishes no binary packages for …` | The image is not Ubuntu 24.04. Flash Ubuntu Server 24.04 LTS 64-bit. |
| `this is a 32-bit userspace` | A 32-bit image. ROS 2 has no 32-bit ARM builds; flash a 64-bit one. |
| `the dpkg lock is still held after 900s` | Something is stuck. `ps aux \| grep -E 'apt\|dpkg'`, end it, re-run. |
| `rosdep could not resolve every dependency` | It names the key. Install it by hand, then re-run the script. |
| `no PCA9685 answered on any I2C bus` | Expected before wiring, or before the reboot. Check with `i2cdetect -y 1`. |
| `has no installation candidate` | The apt source is there but the release carries no such distribution. Check with `apt-cache policy`. |
| Build killed with no error | Out of memory. Lower `parallel-workers`, or add swap. |

The script never continues past a step it could not complete: a run that reaches
`Done` has done everything it reported.
