/**
 * @file utility_function.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Small numeric helpers shared by the servomotor node.
 * @version 0.2.0
 * @date 2026-08-09
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef HEXAPOD_UTILITY_FUNCTION_H_
#define HEXAPOD_UTILITY_FUNCTION_H_

namespace hexapod {

/**
 * @brief Default pulse width, in microseconds, that commands 0 degrees.
 *
 * Hobby servos are usually specified for 1000 us to 2000 us, but most of them
 * travel further; 650 us to 2350 us covers the full 180 degrees of the models
 * fitted to this robot. Both ends are node parameters, because a servo driven
 * past its mechanical stop stalls and overheats.
 */
inline constexpr double kDefaultMinPulseWidthUs{650.0};

/** @brief Default pulse width, in microseconds, that commands 180 degrees. */
inline constexpr double kDefaultMaxPulseWidthUs{2350.0};

/** @brief Smallest angle a joint accepts, in degrees. */
inline constexpr double kMinAngleDegree{0.0};

/** @brief Largest angle a joint accepts, in degrees. */
inline constexpr double kMaxAngleDegree{180.0};

/**
 * @brief Rescale a value from one interval onto another, linearly.
 *
 * No clamping is applied: an input outside `[in_min, in_max]` maps outside
 * `[out_min, out_max]` as well. Callers that must not overrun their output
 * range have to clamp themselves.
 *
 * @tparam T type of the value being rescaled
 * @tparam C type of the interval bounds
 * @param value value to rescale
 * @param in_min lower bound of the source interval
 * @param in_max upper bound of the source interval
 * @param out_min lower bound of the destination interval
 * @param out_max upper bound of the destination interval
 * @return @p value expressed in the destination interval
 */
template <typename T, typename C>
constexpr T map(T value, const C in_min, const C in_max, const C out_min, const C out_max) {
  return ((value - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}

}  // namespace hexapod

#endif  // HEXAPOD_UTILITY_FUNCTION_H_
