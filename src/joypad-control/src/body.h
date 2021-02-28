#ifndef BODY_H
#define BODY_H

#include <iostream>

template <typename T>
struct Body {
  struct position {
    T x;
    T y;
    T z;
  };

  struct orientation {
    T row;
    T pitch;
    T yaw;
  };

  template <typename T>
  friend std::ostream& operator<<(std::ostream& os, const Body<T>& b)
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Body<T>& b) {
  return os << "Body position "
            << "{" << b.x << ", " << b.y << ", " << b.z "} "
            << "orientation"
            << "{" << b.row << ", " << b.pitch << ", " << b.yaw "}\n";
}

#endif  // BODY_H