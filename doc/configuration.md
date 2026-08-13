# Configuring the robot

Three files decide how the robot behaves, and none of them requires a rebuild to
change — but they are installed, so re-run `colcon build` (or build once with
`--symlink-install`) after editing them.

| File | Decides |
|---|---|
| `config/servoconfiguration.yaml` | Node parameters: bus, addresses, frequency, pulse limits |
| `config/motors.lua` | Which motors exist and which channel each one uses |
| `config/homing.lua` | The rest angle of every motor |

## Node parameters

All of them live under `/**: ros__parameters:` in
`src/hexapod_servomotor/config/servoconfiguration.yaml`.

| Parameter | Type | Default | Meaning |
|---|---|---|---|
| `i2c_bus` | string | `/dev/i2c-1` | Bus both boards are wired to |
| `left_driver_address` | int | 64 (0x40) | Board driving motors whose name starts with `L` |
| `right_driver_address` | int | 65 (0x41) | Board driving the rest |
| `pwm_frequency` | double | 50.0 | Refresh rate in hertz, 24 to 1526 |
| `min_pulse_width_us` | double | 650.0 | Pulse that commands 0° |
| `max_pulse_width_us` | double | 2350.0 | Pulse that commands 180° |
| `settle_time_ms` | int | 50 | Pause after each servo command |
| `motors_per_side` | int | 9 | Motors generated per side |
| `perform_startup_test` | bool | `false` | Sweep every joint at startup |
| `motors_script` | string | `motors.lua` | Motor table script |
| `homing_script` | string | `homing.lua` | Rest position script |

Override one without editing the file:

```sh
ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args \
    -p pwm_frequency:=50.0 -p settle_time_ms:=20
```

The node validates every value at startup and refuses to run on a bad one,
rather than driving the servos with it. Addresses must differ, the minimum pulse
must be below the maximum, and the maximum pulse must fit inside one period —
2350 µs is fine in a 20 ms period, but not in the 4 ms period of 250 Hz.

### Tuning the pulse limits

This is the setting worth spending time on. 1000–2000 µs is the nominal
standard; most hobby servos travel further, and the defaults here cover the full
180° of the models fitted to the robot.

Narrow the range if a joint **buzzes, strains or gets hot at one end**: that is a
servo being commanded past its mechanical stop, where it stalls, draws its full
current and cooks itself. Widen it only while watching the joint.

### `perform_startup_test`

Off by default, deliberately. It sweeps every joint across its whole travel one
at a time — which knocks an assembled robot over, and stalls any joint that is
mechanically blocked. Enable it on a bench, with the body supported and the legs
hanging free:

```sh
ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args \
    -p perform_startup_test:=true
```

## Which Lua runs the scripts

Two interpreters can execute them, and CMake decides at configure time:

| | |
|---|---|
| **LuaJIT** | Preferred. Found through pkg-config, from vcpkg or from `libluajit-5.1-dev`. sol2 is told about it with `SOL_LUAJIT=1` |
| Reference Lua | The fallback, `liblua5.3-dev`. Also where `luac5.3` comes from |

The configure log says which one won:

```
-- Lua runtime: LuaJIT 2.1.1748459687
```

Force the fallback with `-DHEXAPOD_ENABLE_LUAJIT=OFF` if a script ever needs
something LuaJIT does not have.

> **Keep the scripts Lua 5.1.** LuaJIT tracks 5.1, so integer division `//`,
> `goto`, the bitwise operators and the 5.3 integer subtype are all unavailable
> on the robot. `luac5.3 -p` — what the `lua-syntax` pre-commit hook runs —
> accepts them happily, so the hook will not catch the mistake. The two scripts
> in `config/` use nothing outside 5.1 today; keep it that way.

What LuaJIT buys here is honest but small: both scripts run once, at start-up,
to build an eighteen-entry table. The gain matters if Lua ever moves into the
write loop, and costs nothing until then.

## The motor table: `motors.lua`

The C++ side does not know the robot's anatomy. It calls
`generate_motors_configuration(count, side)` twice, once with `"L"` and once
with `"R"`, and then reads the global `Motors` table, whose entries are
`{name, channel}` pairs.

Naming is a contract shared by three places — the generator, the homing table
and the driver dispatch — so renaming a motor means touching all three:

```
<L|R>_<coxa|femur|tibia><A|B|C>        e.g. L_femurB
```

The first letter selects the board: `L` goes to `left_driver_address`, anything
else to `right_driver_address`. Channels must be 0–15; the node rejects the
table otherwise.

## The rest position: `homing.lua`

`homing(name)` returns the angle in degrees for one motor. Every registered
motor must have an entry, otherwise startup fails with the name of the offender
rather than silently leaving that joint at zero.

```lua
local HOMINGPOSITION = {
  ["L_coxaA"] = 45,
  ["L_femurA"] = 90,
  -- ...
}

function homing(t_legname)
  return HOMINGPOSITION[t_legname]
end
```

Retuning the stance is editing this table. Check it parses before running the
robot:

```sh
luac5.3 -p src/hexapod_servomotor/config/*.lua
```

## What the logs tell you

The node reports its configuration and what the hardware actually did, which is
usually where a problem shows up first:

```
[INFO] [servomotors_node]: configuration: bus /dev/i2c-1, boards 0x40 and 0x41,
                           50.0 Hz, pulse 650-2350 us, settle 50 ms
[INFO] [pca9685]: opening board 0x40 on /dev/i2c-1
[INFO] [pca9685]: board 0x40 ready: prescale 121, output frequency 50.03 Hz
[INFO] [servomotors_node]: registered 18 motors from "motors.lua"
[INFO] [servomotors_node]: all 18 motors moved to their rest position
```

Raise the verbosity to see every register write:

```sh
ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args --log-level debug
```

Warnings worth reading rather than ignoring:

- `requested 400.00 Hz, prescale 14 gives 406.90 Hz` — the prescaler is an
  integer and the rounding bites hard at high frequencies. Only logged when
  the gap exceeds 0.5 Hz; at the 50 Hz servos use it is 0.03 Hz, so the line
  stays at INFO. Pulse widths are computed from the real value either way.
- `angle X degrees is outside [0, 180], clamped` — something asked for an
  impossible joint angle.
- `write to register N failed (attempt 1 of 3), retrying` — a flaky bus. Check
  the wiring and the supply before it becomes a hard failure.

## Related

- [The PCA9685 servo board](pca9685.md) — the hardware and its registers
- [Architecture](architecture.md) — the nodes and topics
