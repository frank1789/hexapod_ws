# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ROS workspace for a 18-DoF hexapod robot (6 legs × coxa/femur/tibia), driven by a PS3 joypad and
actuated through two Adafruit PCA9685 PWM boards over I²C on a Raspberry Pi.

## Critical: ROS 1 code, ROS 2 toolchain

The packages are **ROS 1 (catkin)**: `package.xml` format 2 with `<buildtool_depend>catkin`,
`roscpp`/`message_generation` deps, `ros/ros.h` + `ros::NodeHandle` in every node, and
`src/CMakeLists.txt` is a symlink to `/opt/ros/noetic/share/catkin/cmake/toplevel.cmake`.

But the build/lint tooling is **ROS 2**: `.devcontainer` builds ROS 2 Jazzy, `build_exapod.sh` calls
`colcon`, and `.pre-commit-config.yaml` runs `ament_*` linters. A migration is in progress and is
**not** done — the current sources cannot build against Jazzy without being ported (`roscpp` →
`rclcpp`, `ros::param` → node parameters, `.msg` generation via `rosidl`).

Expect build failures if you just run the scripts; decide explicitly whether a task is "keep ROS 1
working" or "continue the port" before touching build files.

The symlinked `src/CMakeLists.txt` is dangling on any machine without ROS Noetic installed.

## Build & run

```sh
# inside the devcontainer (ROS 2 Jazzy, /workspace)
./build_exapod.sh              # colcon build of hexapod_joypad + hexapod_servomotor (Release)
colcon build --packages-select hexapod_msgs   # msgs are NOT in build_exapod.sh — build first if changed

# ROS 1 equivalent (needs a Noetic environment)
catkin_make                    # or: catkin build

# run (ROS 1 launch files, as documented in README.md)
roslaunch hexapod_joypad hexapod_joypad.launch      # joy_node + controller + servos
roslaunch hexapod_servomotor hexapod_servomotor.launch
```

`hexapod_servomotor` needs Lua 5.3 and sol2; sol2 is fetched via `FetchContent` at configure time
if not found, so the first configure needs network access.

**There are no tests.** All `catkin_add_gtest` blocks are commented-out boilerplate. If you add
tests, you also add the first test infrastructure — don't assume a runner exists.

## Lint & commit

```sh
pre-commit install      # done automatically by the devcontainer postCreateCommand
pre-commit run --all-files
```

Hooks: `clang-format` (project `.clang-format`, 120-col, 2-space, left pointers) **and**
`ament_uncrustify --reformat` on the same C/C++ files, plus `black --line-length=100`,
`ament_flake8`, `ament_copyright`. Run pre-commit before committing or the hooks will rewrite files
under you. Commit messages follow Conventional Commits (`feat:`, `chore:`); `commitizen` is in the image.

## Architecture

Intended data flow:

```
joy_node (/joy, sensor_msgs/Joy)
    → hexapod_joypad/controller_node   remaps raw PS3 values to semantic messages
    → hexapod_msgs on  joypad/button, joypad/thumbstick, joypad/trigger
    → hexapod_servomotor/servomotors_node   → PCA9685 ×2 over /dev/i2c-1
```

**The last link is not connected.** `ServoController` declares `m_abs_sub`/`m_drive_sub` but the
subscription is commented out (`servocontroller.cc`), so the servo node currently only self-tests and
homes. Wiring joypad → servos is unimplemented work, not a regression.

### Packages

- **`hexapod_msgs`** — three messages (`JoypadButton`, `JoypadThumbstick`, `JoypadTrigger`). Changing
  a `.msg` requires rebuilding both consumers.
- **`hexapod_joypad`** — `Joypad` subscribes `/joy` and republishes. Raw `ps3joy` indices live in
  `buttonsmap_ps3joy.h`, human names in `buttonsname.h`; the remap normalizes triggers to 0..1 and
  thumbsticks to Cartesian −1..1, adding magnitude/angle. C++17.
- **`hexapod_servomotor`** — C++20. `ServoController` owns two `adafruit::PCA9685` on `/dev/i2c-1`:
  `0x40` for motors whose name starts with `L`, `0x41` for the rest (`WriteOnMotor` dispatches purely
  on the name prefix). Angle→PWM: `map(angle, 0..180 → 650..2350 µs)` then scaled by frequency × 4096.
- **`hexapod_description`** — URDF + Blender/DAE meshes. `display.launch` points at
  `urdf/crab_model.urdf`, which does not exist (the file is `urdf/Hexapod.urdf`) — the launch is broken.
- **`src/adafruit`** — **not a ROS package** (no `package.xml`/`CMakeLists.txt`), so it is never built.
  It is a divergent duplicate of the I²C/PCA9685 drivers vendored inside `hexapod_servomotor`.
  Edit `hexapod_servomotor/{include,src}` — changes to `src/adafruit` have no effect.

### Motor configuration lives in Lua, not C++

`config/motors.lua` generates the motor table (name + pin) and `config/homing.lua` maps each motor to
its rest angle; both are executed through sol2 at startup. Retuning homing angles or pin assignment
needs no recompilation.

Two gotchas:

- The Lua paths in `servocontroller.cc` are **hardcoded absolute** (`/home/pi/hexapod_ws/src/...`).
  The node only runs from that exact path on the Pi; anywhere else it silently fails to load and
  every motor gets angle 0.
- `config/servoconfiguration.yaml` is loaded as `rosparam` by the launch file, but the code reads only
  `servomotors/pwm_frequency` from it. Its motor table is stale and wrong (the `right` list contains
  `L_`-prefixed names) and is ignored. **Lua is the source of truth.**

Motor naming is `<L|R>_<coxa|femur|tibia><A|B|C>`, pins 0–8 per driver — this convention is relied on
by the Lua generator, the homing table, and the driver dispatch, so renaming a motor touches all three.

### Known-broken files

`hexapod_joypad/src/body.h` does not compile (malformed `operator<<`, shadowed template parameter,
missing semicolon). It is included nowhere, so the build passes; fix it before using it.

## Constitution — how to work in this repository

These rules are binding. They exist because this workspace has no tests, no CI, and a build that is
mid-migration: nothing downstream will catch a mistake for you.

### Be analytical, assume nothing

- **Verify before you state.** Do not infer behaviour from a file name, a comment, or a `.yaml` that
  looks authoritative. This repo actively punishes assumptions: `servoconfiguration.yaml` reads like
  the motor configuration but is dead data, `src/adafruit` reads like a driver package but is never
  compiled, and `display.launch` names a URDF that does not exist. Open the file that actually
  executes and follow the call chain.
- **Read the whole path, not the symbol.** Before changing anything in `hexapod_servomotor`, trace
  `main.cc → ServoController → Lua script → Motor → PCA9685 → i2cPeripheral`. Before changing a
  message, find every publisher and subscriber of that topic.
- **A hypothesis is not a finding.** If you cannot prove a claim from the code, say it is unverified
  and say what would verify it. Never present a plausible reconstruction as fact.
- **Report reality.** If the build fails, paste the failure. If a step was skipped, say so. Never
  describe work as done when it was only attempted.

### Error handling

- **No silent failures.** The existing code demonstrates the anti-pattern: when a Lua script fails to
  load, `ServoController` prints to `stderr` and then *continues*, driving every motor to angle 0.
  New code must fail loudly — throw, or log at `ERROR`/`FATAL` and abort the operation. Do not
  degrade silently into a default.
- **Validate what comes from outside C++.** Lua tables, ROS parameters and YAML are untyped and
  unversioned here. Check that a motor name resolved, that an angle is within 0–180, that a pin is in
  range, that a parameter was actually present — then act.
- **Hardware is stateful and unforgiving.** Code touching `/dev/i2c-1` or PWM registers can damage
  servos. Never change pulse-width bounds, frequency, or homing angles as a side effect of another
  change, and state explicitly when a change alters what the hardware physically does.
- **Preserve the existing contracts.** `main.cc` catches `std::runtime_error`, `std::exception` and
  `...` separately and returns non-zero; keep exceptions escaping to that boundary rather than
  swallowing them mid-stack.

### Workflow (in this order, every time)

1. **Understand first.** Locate the real code path and read it. State which ROS version the task
   targets (ROS 1 as written, or continuing the ROS 2 port) before editing any build file — the two
   answers produce incompatible changes.
2. **Change minimally.** One concern per change. Do not reformat, rename, or "tidy" unrelated code:
   with two formatters in the hook chain, incidental reformatting produces diffs that hide the real
   edit.
3. **Check syntax before committing — always.** Formatters do not compile. Run a real parse on every
   C/C++ file you touched:
   ```sh
   clang++ -fsyntax-only -std=c++20 -Isrc/hexapod_servomotor/include -I/opt/ros/$ROS_DISTRO/include <file>
   clang++ -fsyntax-only -std=c++17 -I/opt/ros/$ROS_DISTRO/include src/hexapod_joypad/src/<file>
   luac5.3 -p src/hexapod_servomotor/config/*.lua      # Lua is executed at runtime; parse it up front
   ```
   Then build the affected package (`colcon build --packages-select <pkg>`, or `catkin_make` under
   ROS 1). A change that has not been parsed by a compiler is not finished.
4. **Run the hooks.** `pre-commit run --all-files` and fix what it reports. Never `--no-verify`.
5. **Verify the behaviour, or declare it unverified.** Much of this code only runs on the Pi against
   real I²C hardware. If you could not execute it, say exactly what remains untested.

> **Known gap:** `.pre-commit-config.yaml` currently contains formatters and style linters only — no
> hook parses C++ or Lua. That is how the broken `body.h` was committed. Until a syntax hook is added
> (`clang++ -fsyntax-only`, `cppcheck` and `luac -p` are all already available in the devcontainer
> image), step 3 must be performed manually and must not be skipped.
