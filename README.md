# hexapod_ws

- [hexapod_ws](#hexapod_ws)
  - [Package hexapod_joypad](#package-hexapod_joypad)
    - [Map buttons](#map-buttons)
    - [Map axes](#map-axes)
    - [Connect PS3 joypad](#connect-ps3-joypad)
  - [Hexapod Messages](#hexapod-messages)

## Package hexapod_joypad

At present, the hexapod_joypad package remaps the values of the buttons issued by the well-known [ps3joy](http://wiki.ros.org/ps3joy) package.

> Compared to the original package, the documentation does not seem to correspond so the new map of buttons and values is attached.

- The values of the back triggers vary between 0.0 and 1.0
- Thumsticks now follow the Cartesian axis convention, their values vary between -1.0 and 1.0
moving them from left to right. Likewise from bottom to top. Consequently the neutral position corresponds to 0.0.

To run the package just run the command from the terminal.

```sh
roslaunch hexapod_joypad hexapod_joypad.launch
```

### Map buttons

The messages form ps3joypad:

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

The messages form ps3joypad:

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

example for read custom message from joypad:

```cpp
const std::string topic_btn{"joypad/button"};
const std::string topic_tbs{"joypad/thumbstick"};
const std::string topic_trg{"joypad/trigger"};

// stuff
subscriber_ = node_handler_.subscribe<hexapod_msgs::JoypadTrigger>(
      topic_btn, 1, &Clss::functionCallback, this);
```
