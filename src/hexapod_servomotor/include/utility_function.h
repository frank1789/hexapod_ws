/**
 * @file utility_function.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HEXAPOD_UTILITY_FUNCTION_H_
#define HEXAPOD_UTILITY_FUNCTION_H_

#define MIN_PULSE_WIDTH 650
#define MAX_PULSE_WIDTH 2350

template <typename T, typename C>
inline T map(T x, const C in_min, const C in_max, const C out_min, const C out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif  // HEXAPOD_UTILITY_FUNCTION_H_
