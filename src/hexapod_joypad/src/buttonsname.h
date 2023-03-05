#ifndef BUTTONS_NAME_H
#define BUTTONS_NAME_H
// clang-format off
// ros msgs form ps3joypad need maps
// axes: [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
// buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
//           X, O, T, Q,L1,R1,L2,R2,SE,ST,PS,L3,R3,UP,DW,RT,LT 

// X  -> cross button
// X  -> circle button
// T  -> triangle button
// Q  -> square button
// SE -> select button
// ST -> start button
// PS -> PS button
// UP -> cross directional up
// DW -> cross directional down
// RT -> cross directional right
// LT -> cross directional left
//
// remember button 0 -> not pressed, 1 -> pressed
//
// axes [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
//         |    |     |     |     |    +---> R2 axis (form 1.0 to -1.0(pressed)) remap as 0 to 1.0 (pressed)
//         |    |     |     |     +---> right stick y axis 
//         |    |     |     |
//         |    |     |     +---> right stick x axis on left position == 1, on right position == -1 ( remember to invert respects actual value)
//         |    |     |
//         |    |     +---> L2 axis (form 1.0 to -1.0(pressed)) remap as 0 to 1.0 (pressed)
//         |    |
//         |    +---> left stick y axis 
//         |
//         +---> left stick x axis on left position == 1, on right position == -1 ( remember to invert respects actual value)
//
// clang-format on

using cstring_t = const char*;

namespace trigger {
constexpr cstring_t kL2 = "L2";
constexpr cstring_t kR2 = "R2";
}  // namespace trigger

namespace thumbstick {
constexpr cstring_t kL3 = "L3";
constexpr cstring_t kR3 = "R3";
}  // namespace thumbstick

namespace button {
constexpr cstring_t kCross = "Cross";
constexpr cstring_t kCircle = "Circle";
constexpr cstring_t kTriangle = "Triangle";
constexpr cstring_t kSquare = "Square";
constexpr cstring_t kL1 = "L1";
constexpr cstring_t KR1 = "R1";
constexpr cstring_t kSelect = "Select";
constexpr cstring_t kStart = "Start";
constexpr cstring_t kPlaystation = "Playstation";
constexpr cstring_t kUp = "Up";
constexpr cstring_t kDown = "Down";
constexpr cstring_t kRight = "Right";
constexpr cstring_t kLeft = "Left";
}  // namespace button

#endif  // BUTTONS_NAME_H