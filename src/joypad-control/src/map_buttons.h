#ifndef MAP_BUTTON_H
#define MAP_BUTTON_H
// ros msgs form ps3joypad need maps
// axes: [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
// buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

#include <iostream>

namespace joypad {
namespace trigger {}  // namespace trigger

namespace stick {}  // namespace stick

namespace button {}  // namespace button
}  // namespace joypad


std::ostream& operator<<(std::ostream& os, const T& b){
    return os;
}

#endif  // MAP_BUTTON_H