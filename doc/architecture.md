# Architecture

Four ROS 2 packages, built with `ament_cmake` and `rclcpp`.

![From the joypad to the joints](images/data-flow.svg)

## Packages

| Package | Role |
|---|---|
| `hexapod_msgs` | Three messages generated with `rosidl` |
| `hexapod_joypad` | Remaps `sensor_msgs/msg/Joy` into those messages |
| `hexapod_servomotor` | Drives the eighteen servos through two PCA9685 boards |
| `hexapod_description` | URDF model and meshes |

`src/adafruit` is **not** a package: it has no `package.xml` and is never built.
It is an older copy of the I²C and PCA9685 drivers that now live inside
`hexapod_servomotor`, and the two have drifted apart. Edit
`hexapod_servomotor/{include,src}`.

## The joypad chain

`joy_node` publishes raw axes and buttons on `/joy`. `controller_node` turns
them into something with names and units:

| Topic | Message | Content |
|---|---|---|
| `joypad/button` | `JoypadButton` | Button name and pressed state |
| `joypad/thumbstick` | `JoypadThumbstick` | Cartesian axes, magnitude, angle |
| `joypad/trigger` | `JoypadTrigger` | Trigger name and travel, 0 to 1 |

The remap normalises triggers to 0…1 and thumbsticks to −1…1 with the usual
Cartesian sign convention, and adds the magnitude and angle of the stick vector
so consumers do not each recompute them.

The PS3 axis and button indices live in `buttonsmap_ps3joy.h`, the human names
in `buttonsname.h`. The ROS 1 `ps3joy` driver has no ROS 2 release, so the
generic `joy` node reads the controller from `/dev/input/js<N>` once it is
paired over Bluetooth; the layout is unchanged.

A `Joy` message carrying fewer axes than the remap indexes is rejected with an
error rather than read out of bounds — a different controller, or one that is
not fully initialised, produces exactly that.

## The servo chain

`servomotors_node` owns two `adafruit::PCA9685` objects. `WriteOnMotor`
dispatches on the first letter of the motor name: `L` to the left board,
anything else to the right one.

```
main.cc
  └── hexapod::ServoController            rclcpp::Node
        ├── DeclareParameters()           validates the configuration
        ├── OpenDrivers()                 adafruit::PCA9685 ×2
        │     └── i2cPeripheral           /dev/i2c-1
        ├── RegisterMotors()              runs motors.lua through sol2
        └── RestoreDefaultPosition()      runs homing.lua
```

Angles become pulse widths, pulse widths become ticks of the 12-bit counter, and
the conversion uses the frequency the board actually produces — see
[the hardware notes](pca9685.md).

**The two chains are not joined yet.** `ServoController` does not subscribe to
the joypad topics: it homes the joints and then waits. Making the robot walk
means adding that subscription and a gait, which is unimplemented work rather
than a regression.

## Failure behaviour

The node is meant to stop loudly rather than move a joint on a guess:

| Situation | Behaviour |
|---|---|
| I²C bus missing or unreadable | `std::system_error`, node exits with 1 |
| Bad parameter | `std::invalid_argument`, node exits with 1 |
| Lua script missing, broken, or silent about a motor | `std::runtime_error`, node exits with 1 |
| Transient bus error | retried three times, then thrown |
| Process ends, for any reason | boards park every output and sleep |

The last row is the important one: a crash leaves the servos unpowered rather
than holding their last command.

## Related

- [The PCA9685 servo board](pca9685.md)
- [Configuring the robot](configuration.md)
