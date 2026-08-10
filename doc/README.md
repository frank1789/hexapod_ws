# Documentation

| Document | Contents |
|---|---|
| [Setting up a Raspberry Pi](raspberry-pi.md) | The install script, which image to flash, enabling I²C, sizing the build, and what to do when a step fails |
| [The PCA9685 servo board](pca9685.md) | How the hardware generates PWM, its registers and timing, the wiring, and what the driver validates |
| [Configuring the robot](configuration.md) | Every node parameter, the two Lua scripts, how to tune the pulse limits, and how to read the logs |
| [Architecture](architecture.md) | The four packages, the topics between them, and how failures are handled |

Build, install and run instructions are in the [top-level README](../README.md).
Working rules for this repository — coding standard, error handling and the
commit workflow — are in [CLAUDE.md](../CLAUDE.md).

## Generating the API reference

Every source file carries Doxygen comments. There is no Doxyfile in the
repository yet; the quickest way to read the API is:

```sh
cd src/hexapod_servomotor
doxygen -g            # writes a default Doxyfile
doxygen               # writes html/index.html
```

## Photographs

Reference photographs of the assembled robot are in
[`src/reference_img`](../src/reference_img): [front](../src/reference_img/Front.JPG)
and [top](../src/reference_img/Top.JPG).
