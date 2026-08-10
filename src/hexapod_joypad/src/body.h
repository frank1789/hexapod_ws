#ifndef BODY_H
#define BODY_H

#include <ostream>

template <typename T>
struct Body {
  struct Position {
    T x;
    T y;
    T z;
  };

  struct Orientation {
    T roll;
    T pitch;
    T yaw;
  };

  Position position;
  Orientation orientation;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const Body<T>& b) {
  return os << "Body position "
            << "{" << b.position.x << ", " << b.position.y << ", " << b.position.z << "} "
            << "orientation "
            << "{" << b.orientation.roll << ", " << b.orientation.pitch << ", " << b.orientation.yaw << "}\n";
}

#endif  // BODY_H
