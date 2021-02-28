#ifndef MAP_BUTTON_H
#define MAP_BUTTON_H
// ros msgs form ps3joypad need maps
// axes: [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
// buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
//           X, O, T, Q,L1,R1,L2,R2,SE,ST,PS,L3,R3,UP,DW,RT,LT 

// T -> trinagolo
// Q -> quadrato
// SE -> select
// ST -> start
// PS -> PS button
// DW -> direction cross down
// RT -> direction cross right
// LT -> direction cross left

// axes [-0.0, -0.0, 1.0, -0.0, -0.0, 1.0]
//         |    |     |     |     |    +---> R2 axis (form 1.0 to -1.0(pressed)) remap as 0 to 1.0 (pressed)
//         |    |     |     |     +---> right stick y axis 
//         |    |     |     |
//         |    |     |     +---> right stick x axis on left position == 1, on right position == -1 ( rember to invert rispect actual value)
//         |    |     |
//         |    |     +---> L2 axis (form 1.0 to -1.0(pressed)) remap as 0 to 1.0 (pressed)
//         |    |
//         |    +---> left stick y axis 
//         |
//         +---> left stick x axis on left position == 1, on right position == -1 ( rember to invert rispect actual value)
//
//
//
//
//

// remember button 0 -> not pressed, 1 -> pressed


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