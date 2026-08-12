# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ROS 2 workspace for a 18-DoF hexapod robot (6 legs × coxa/femur/tibia), driven by a PS3 joypad and
actuated through two Adafruit PCA9685 PWM boards over I²C on a Raspberry Pi.

Everything is ROS 2 (`ament_cmake` + `rclcpp`). The workspace was ported from ROS 1 Noetic on the
`feature/upgrade-ros-2` branch; no catkin/roscpp code remains. The ROS 1 history is still in git if
you need to compare behaviour.

## Build & run

The supported environment is the dev container (`.devcontainer/`, Ubuntu 24.04 + ROS 2 Jazzy).
`ROS_DISTRO` is exported inside the image, so nothing below hard-codes a distro.

```sh
./build_exapod.sh                              # colcon build of the whole workspace (Release)
colcon build --symlink-install --packages-select hexapod_servomotor   # single package
source install/setup.bash

ros2 launch hexapod_joypad hexapod_joypad.launch.py                   # joy + remapper + servos
ros2 launch hexapod_joypad hexapod_joypad.launch.py with_servos:=false  # no I²C hardware needed
ros2 launch hexapod_servomotor hexapod_servomotor.launch.py
ros2 launch hexapod_description display.launch.py                     # RViz + joint sliders

ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args --log-level debug
```

Upgrading ROS 2 or clang is a one-line change in `.devcontainer/devcontainer.json`
(`ROS_DISTRO`, `LLVM_VERSION`); the Dockerfile derives everything else from those ARGs.

### Dependencies

**vcpkg first, `FetchContent` only when a dependency is not packaged.** `vcpkg.json` is the
manifest; the dev container and `docker/Dockerfile` both bootstrap vcpkg and resolve it once while
the image is built, into `/opt/vcpkg_installed`. `build_exapod.sh` picks the toolchain up from
`VCPKG_ROOT` on its own and falls back to the system packages when vcpkg is absent, so a bare host
and a Pi set up by `scripts/install-raspberrypi.sh` still build.

Every CMake dependency lookup follows the same shape: config package first, then a fetch. Do not
add a `FetchContent` for something vcpkg already carries.

**Neither image installs a manifest dependency from apt on purpose.** `.devcontainer/Dockerfile.ros2`
and `docker/Dockerfile` ask for the same short list — `libi2c-dev` and `liblua5.3-dev`, neither of
which vcpkg can supply — and take the other seven from the manifest. Adding `libfmt-dev` or
`libzmq3-dev` back "so it builds without vcpkg" gives `find_package()` a second, differently
versioned answer and stops the dev container predicting what the robot builds. The system-package
fallback is for a bare host and the Pi, not for the images.

"On purpose" is the operative phrase: `libfmt-dev` 9.1.0 is still in the dev image as a transitive
dependency of `libspdlog-dev`, which ROS itself pulls in. It loses anyway — the vcpkg toolchain
puts `/opt/vcpkg_installed` first, and `fmt_DIR` resolves there — but do not read a `dpkg -l` hit
as proof the policy was broken. Check `build/<pkg>/CMakeCache.txt` for the `*_DIR` value instead.

Two facts about manifest mode, both verified by running it, both easy to get wrong:

- `-DVCPKG_MANIFEST_MODE=OFF` in `build_exapod.sh` and both Dockerfiles does **not** mean the
  manifest is unused. It is resolved once per image, for the whole workspace; `OFF` stops every
  colcon package re-resolving it into its own build directory.
- **Do not add a `builtin-baseline` to `vcpkg.json`.** Both images clone vcpkg with `--depth 1`,
  and vcpkg cannot read a baseline commit that shallow clone does not contain — it fails with
  `failed to git show versions/baseline.json` rather than fetching it. Since the clone follows
  `VCPKG_REF`, defaulting to `master`, a baseline breaks the build as soon as master moves. The
  version pin is `VCPKG_REF`, which is an `ARG` in both files.

**Architecture.** The vcpkg triplet is derived from `uname -m`, so x86-64 and arm64 hosts — an
Intel Mac, an Apple Silicon Mac, a Pi — need no configuration. `VCPKG_FORCE_SYSTEM_BINARIES` must
stay unset: vcpkg's port scripts need CMake ≥ 3.31 (`string(JSON ... STRING_ENCODE)`) and Ubuntu
24.04 has 3.28, so setting it fails every port on *every* architecture. vcpkg publishes its own
CMake and Ninja for `linux/amd64` and `linux/arm64` alike. `docker/Dockerfile` still sets it; that
is a known defect recorded in [doc/simulation.md](doc/simulation.md), not a pattern to copy.

`hexapod_servomotor` needs a Lua runtime, `libi2c-dev` and sol2. **LuaJIT is preferred** — CMake
finds it through pkg-config and defines `SOL_LUAJIT=1` — and the reference interpreter is the
fallback, selected automatically or forced with `-DHEXAPOD_ENABLE_LUAJIT=OFF`. The configure log
says which one was chosen:

```
-- Lua runtime: LuaJIT 2.1.x
```

**Consequence for `config/*.lua`: keep them Lua 5.1.** LuaJIT tracks 5.1, so `//`, `goto`, bitwise
operators and the integer subtype build against the reference interpreter and then fail on the
robot. The `lua-syntax` hook runs `luac5.3 -p`, which accepts 5.3-only syntax — it will not catch
this for you.

### Graphical tools without a GPU

Docker passes no GPU and no X socket into its virtual machine on macOS, so the container carries
its own display: `scripts/start-gui.sh` starts Xvfb, fluxbox, x11vnc and noVNC, and the
`postStartCommand` in `devcontainer.json` runs it. Point a browser at `http://localhost:6080`.
Rendering is Mesa llvmpipe on the CPU — usable for this model, and the reason
`src/hexapod_description/rviz/hexapod.rviz` keeps the TF display off and the grid small.

`display.launch.py` opens that configuration by default. Without it RViz starts with no
RobotModel display and a Fixed Frame of `map`, which this model does not have, and draws nothing.
See [doc/simulation.md](doc/simulation.md).

### Tests

```sh
colcon test --event-handlers console_direct-        # run everything
colcon test --packages-select hexapod_servomotor    # one package
colcon test-result --all --verbose                  # what passed and what did not
```

Every package has tests and **none of them needs the robot**: they cover the joypad value remapping,
the motor model and angle mapping, the PCA9685 frequency arithmetic and its argument validation, the
generated message fields, and the URDF with the meshes it refers to. Anything that would open
`/dev/i2c-*` is out of scope by design — keep it that way, so the suite stays runnable on a laptop.

`colcon test` succeeds silently when a package builds nothing; always confirm with `test-result`,
which reports the actual counts.

## Language standard

**C++20, strict ISO, no compiler extensions.** Every package sets `CMAKE_CXX_STANDARD 20`,
`CMAKE_CXX_STANDARD_REQUIRED ON` and `CMAKE_CXX_EXTENSIONS OFF`, so the compiler is invoked with
`-std=c++20` rather than `-std=gnu++20`. Do not reach for GNU extensions (statement expressions,
`typeof`, zero-length arrays, nested functions, anonymous structs); if a construct only builds with
extensions on, it does not belong here.

Prose — comments, documentation, identifiers — uses **British English**. `cspell.config.yaml` sets
`language: en-GB` and carries the domain word list; the spell check runs as a pre-commit hook and the
editor reads the same file.

## Lint & commit

```sh
pre-commit install          # installs BOTH the pre-commit and commit-msg hooks
pre-commit run --all-files
pre-commit autoupdate       # refresh the pinned hook revisions
cz commit                   # guided Conventional Commit
```

`pre-commit install` runs automatically via the devcontainer `postCreateCommand`.

Hooks: file hygiene (end-of-file, trailing whitespace, line endings, XML/YAML/TOML, symlinks),
`clang-format`, `cmake-format`, `black --line-length=100`, `flake8`, `shellcheck`, `cspell`,
`commitizen`, plus three local checks that parse rather than format — Lua, C++ and the commit
message layout.

`clang-format` is the **only** C++ formatter. `ament_uncrustify` used to run alongside it with
incompatible rules, so the two rewrote every file in turn; do not add a second formatter back.

### Static analysis

**clang-tidy runs during the compilation, never as a git hook** — it needs the full include path, so
it belongs where that path exists. The build is expected to be free of findings; treat a new one as
something to fix, not to silence.

```sh
colcon build --cmake-args -DHEXAPOD_ENABLE_CLANG_TIDY=OFF   # skip it for a quick build
./scripts/merge-compile-commands.sh                          # refresh compile_commands.json
```

Each package exports its own `compile_commands.json`; `build_exapod.sh` merges them into one at the
root of the workspace, which is what clangd is pointed at.

Two exclusions in `.clang-tidy` are deliberate and documented there: `cppcoreguidelines-pro-type-vararg`
(the `RCLCPP_*` macros are printf-style, there is no alternative API) and `modernize-use-trailing-return-type`
(taste, and it would rewrite every signature). `readability-function-cognitive-complexity` is kept but
set to `IgnoreMacros`, because the logging macros expand into branches — a three-line destructor
scored 83 against a threshold of 25.

`src/main.cc` and `src/servocontroller.cc` are excluded from the analysis via `SKIP_LINTING`: the
pinned sol2 revision does not parse with clang, only with GCC. Everything else is analysed.

The two local syntax checks skip with a notice when their prerequisites are missing (no clang, no
sourced ROS, no `install/`), so committing works on a bare host — but then nothing has parsed your
C++. Run them in the dev container before you consider a change finished.

### Commit messages

Conventional Commits (`feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `build:`, `ci:`, `test:`),
with a hard layout enforced by `scripts/check-commit-message.sh` at the commit-msg stage:

- subject at most **52** characters
- one blank line after the subject
- body wrapped at **72** characters (trailers and bare URLs are exempt)

`git config commit.template .gitmessage` is set locally, so the ruler appears in the editor.
Commitizen is configured in `.cz.toml` (`cz commit`, `cz bump`).

## Architecture

Intended data flow:

```
joy_node (/joy, sensor_msgs/msg/Joy)
    → hexapod_joypad/controller_node   remaps raw PS3 values to semantic messages
    → hexapod_msgs on  joypad/button, joypad/thumbstick, joypad/trigger
    → hexapod_bridge/hexapod_bridge    joypad freezes the stream; otherwise it forwards
    ↑
Maya/Blender → ZeroMQ (JSON, degrees) → hexapod_bridge
    → sensor_msgs/msg/JointState on joint_command (radians)
    → hexapod_servomotor/servomotors_node   → PCA9685 ×2 over /dev/i2c-1
```

**The chain is connected, but only from the ZeroMQ side.** `ServoController` subscribes to
`joint_command` and writes whatever pose arrives. The joypad can only *freeze* that stream — it
commands no angles of its own, because there is still no gait engine or inverse kinematic model in
this workspace. A node that turns a thumbstick into eighteen joint angles is unimplemented work, not
a regression; when it exists it publishes onto `joint_command` and needs no change to the bridge.

Writing a pose is slow: `WriteOnMotor` sleeps `settle_time_ms` per motor, so eighteen joints take
`18 × settle_time_ms` — 900 ms at the shipped default. The servo node therefore stores the newest
pose and writes it on a `write_rate_hz` timer; a pose overtaken before the timer runs is never
written. Any work on making the robot follow an animation smoothly starts there, not in the
transport.

### Packages

- **`hexapod_msgs`** — three messages (`JoypadButton`, `JoypadThumbstick`, `JoypadTrigger`), all
  primitive fields, generated with `rosidl`. Changing a `.msg` requires rebuilding both consumers.
- **`hexapod_joypad`** — the `Joypad` node subscribes `/joy` and republishes. Raw PS3 axis/button
  indices live in `buttonsmap_ps3joy.h`, human names in `buttonsname.h`; the remap normalizes
  triggers to 0..1 and thumbsticks to Cartesian −1..1, adding magnitude/angle. The ROS 1
  `ps3joy` driver has no ROS 2 release — the generic `joy` node is used instead, reading the
  controller from `/dev/input/js<N>` once it is paired over Bluetooth.
- **`hexapod_bridge`** — `BridgeNode` receives JSON poses over a ZeroMQ `SUB` socket opened with
  `ZMQ_CONFLATE` (only the newest message survives, so a stalled link cannot make the robot replay
  stale motion), validates every field, and republishes as `sensor_msgs/msg/JointState`. It also
  subscribes to the joypad topics: any deliberate input freezes the stream until the joypad has been
  quiet for `override_timeout_ms`. The wire format is degrees, `JointState` is radians — the
  conversion happens here. `WireFormat` and `OverridePolicy` are free of ROS and of ZeroMQ so the
  tests exercise them without opening a socket.
- **`hexapod_servomotor`** — `ServoController` is an `rclcpp::Node` owning two
  `adafruit::PCA9685`: the `left_driver_address` board (default `0x40`) drives every motor whose name
  starts with `L`, `right_driver_address` (default `0x41`) drives the rest — `WriteOnMotor` dispatches
  purely on the name prefix. Angle→PWM: `map(angle, 0..180 → 650..2350 µs)` scaled by frequency × 4096.
  Parameters: `i2c_bus`, `left_driver_address`, `right_driver_address`, `pwm_frequency`.
- **`hexapod_description`** — URDF + Blender/DAE meshes + the RViz configuration, all installed to
  the package share directory because the URDF refers to its meshes through `package://` URIs and
  `display.launch.py` resolves the RViz file the same way. Its joint names are
  `<L|R>_<front|mid|back>_<coxa|femur|tibia>_jnt` and **do not match the motor names in
  `motors.lua`** (`<L|R>_<coxa|femur|tibia><A|B|C>`), nor the angle convention: the URDF centres
  each joint on 0 rad within ±1.5708, the wire format runs 0–180° with 90° as the rest pose.
  Nothing bridges the two, so a pose on `joint_command` cannot drive the model. Which of `A`, `B`,
  `C` is the front, middle and back leg is **not recorded anywhere in this repository** — do not
  guess it.
- **`src/adafruit`** — **not a ROS package** (no `package.xml`/`CMakeLists.txt`), so it is never built.
  It is a divergent duplicate of the I²C/PCA9685 drivers vendored inside `hexapod_servomotor`.
  Edit `hexapod_servomotor/{include,src}` — changes to `src/adafruit` have no effect.

### Motor configuration lives in Lua, not C++

`config/motors.lua` generates the motor table (name + pin) and `config/homing.lua` maps each motor to
its rest angle; both are executed through sol2 at startup, loaded from the package share directory via
`ament_index_cpp::get_package_share_directory()`. Retuning homing angles or pin assignment needs no
recompilation — but the scripts are installed, so re-run `colcon build` (or use `--symlink-install`)
after editing them.

`config/servoconfiguration.yaml` holds ROS 2 node parameters only. It does **not** describe the
motors: Lua is the single source of truth for the motor table.

Motor naming is `<L|R>_<coxa|femur|tibia><A|B|C>`, pins 0–8 per driver — this convention is relied on
by the Lua generator, the homing table, and the driver dispatch, so renaming a motor touches all three.

## Constitution — how to work in this repository

These rules are binding. They exist because this workspace has no CI and drives real hardware: the
test suite is the only automatic check, and it deliberately stops at the edge of the I²C bus.

### Be analytical, assume nothing

- **Verify before you state.** Do not infer behaviour from a file name, a comment, or a `.yaml` that
  looks authoritative. This repo has a history of punishing assumptions: a `.yaml` that read like the
  motor configuration but was dead data, an `ENV` in the Dockerfile that silently disabled a whole
  block of setup, a launch file naming a URDF that did not exist. Open the file that actually
  executes and follow the call chain.
- **Read the whole path, not the symbol.** Before changing anything in `hexapod_servomotor`, trace
  `main.cc → ServoController → Lua script → Motor → PCA9685 → i2cPeripheral`. Before changing a
  message, find every publisher and subscriber of that topic.
- **A hypothesis is not a finding.** If you cannot prove a claim from the code, say it is unverified
  and say what would verify it. Never present a plausible reconstruction as fact.
- **Report reality.** If the build fails, paste the failure. If a step was skipped, say so. Never
  describe work as done when it was only attempted.

### Error handling

- **No silent failures.** Lua script loading, script execution and every lookup into a Lua table are
  checked and raise `std::runtime_error`; the joy callback rejects messages carrying fewer axes than
  the remap indexes. Keep that discipline: fail loudly rather than degrading into a default. The
  original code printed to `stderr` and carried on, which drove every motor to angle 0.
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

1. **Understand first.** Locate the real code path and read it.
2. **Change minimally.** One concern per change. Do not reformat, rename, or "tidy" unrelated code:
   with two formatters in the hook chain, incidental reformatting produces diffs that hide the real
   edit.
3. **Check syntax before committing — always.** Formatters do not compile. Run a real parse on every
   C/C++ file you touched, then build the affected package:
   ```sh
   colcon build --symlink-install --packages-select <pkg>
   colcon test --packages-select <pkg> && colcon test-result --all --verbose
   luac5.3 -p src/hexapod_servomotor/config/*.lua   # Lua is executed at runtime; parse it up front
   python3 -m py_compile src/*/launch/*.launch.py   # launch files fail only when launched
   ```
   A change that has not been parsed by a compiler is not finished. Outside the container, where
   `rclcpp` headers are absent, `clang++ -fsyntax-only` reports a cascade of
   `'rclcpp/rclcpp.hpp' file not found` errors — that is a missing environment, not a code defect;
   build in the container instead of chasing it.
4. **Run the hooks.** `pre-commit run --all-files` and fix what it reports. Never `--no-verify`.
5. **Verify the behaviour, or declare it unverified.** Much of this code only runs on the Pi against
   real I²C hardware. If you could not execute it, say exactly what remains untested.

> The hook chain now parses as well as formats (`cpp-syntax`, `lua-syntax`), which closes the gap that
> once let a `body.h` that could not compile reach the repository. Those two hooks **skip silently on a
> host without ROS or without a built workspace** — a pass on the host is not evidence that anything
> compiled. Step 3 is still yours to run in the dev container.
