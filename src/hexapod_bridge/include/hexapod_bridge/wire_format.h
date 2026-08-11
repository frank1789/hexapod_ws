/**
 * @file wire_format.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Decoding and validation of the pose messages carried over ZeroMQ.
 * @version 0.3.0
 * @date 2026-08-10
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

#ifndef HEXAPOD_BRIDGE_WIRE_FORMAT_H_
#define HEXAPOD_BRIDGE_WIRE_FORMAT_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hexapod::bridge {

/**
 * @brief One pose as it arrives from the animation package.
 *
 * Angles are in **degrees**, which is the unit the rest of the robot's
 * configuration is written in: `homing.lua` returns degrees and the servo
 * driver maps 0-180 degrees onto its pulse range. The ROS side of the bridge
 * converts to radians, because `sensor_msgs/msg/JointState` is defined in
 * radians and the URDF model has to agree with it.
 */
struct JointPose {
  std::vector<std::string> names; /**< Motor names, as in `motors.lua`. */
  std::vector<double> degrees;    /**< Angle of each motor, same order as names. */
  std::uint64_t sequence{0};      /**< Sender's counter, for spotting gaps. */
  std::uint32_t schema{0};        /**< Schema version the sender used. */
};

/**
 * @brief The message format spoken over the ZeroMQ socket.
 *
 * A payload is a single JSON object:
 *
 * ```
 * {"schema": 1, "seq": 42, "units": "deg",
 *  "joints": {"L_coxaA": 90.0, "L_femurA": 45.0}}
 * ```
 *
 * JSON was chosen over a packed binary layout because it is self-describing:
 * the joint names travel with the values, so the two ends cannot silently
 * disagree about the order of eighteen numbers. At fifty poses per second the
 * verbosity costs a few tens of kilobytes per second, which is nothing on this
 * link.
 *
 * Every field is checked. Nothing reaching the servos may be taken on trust:
 * this payload crosses a network from a machine running a program that has no
 * idea a robot is on the other end.
 */
class WireFormat {
 public:
  /** @brief Schema version this build produces and accepts. */
  static constexpr std::uint32_t kSchemaVersion{1};

  /** @brief Lowest angle the servos accept, in degrees. */
  static constexpr double kMinAngleDegree{0.0};

  /** @brief Highest angle the servos accept, in degrees. */
  static constexpr double kMaxAngleDegree{180.0};

  /**
   * @brief Decode and validate one payload.
   *
   * @param t_payload the bytes taken off the socket
   * @return the decoded pose, guaranteed to hold at least one joint, with every
   * angle finite and inside `[kMinAngleDegree, kMaxAngleDegree]`
   *
   * @throws std::invalid_argument if the payload is not JSON, is not an object,
   * carries an unknown schema version or unit, has no joints, names a joint
   * with an empty string, or gives an angle that is not a finite number inside
   * the accepted range. The message says which field was wrong.
   */
  [[nodiscard]] static JointPose Decode(std::string_view t_payload);

  /**
   * @brief Encode a pose into the same format `Decode` accepts.
   *
   * Used by the tests to prove the two directions agree, and it documents the
   * format for anyone writing another sender.
   *
   * @param t_pose pose to encode; `names` and `degrees` must be the same length
   * @return the JSON payload
   *
   * @throws std::invalid_argument if the two vectors differ in length.
   */
  [[nodiscard]] static std::string Encode(const JointPose& t_pose);

  /** @brief Convert degrees to radians, the unit `JointState` is defined in. */
  [[nodiscard]] static double DegreesToRadians(double t_degrees) noexcept;

  /** @brief Convert radians back to the degrees the servo driver works in. */
  [[nodiscard]] static double RadiansToDegrees(double t_radians) noexcept;
};

}  // namespace hexapod::bridge

#endif  // HEXAPOD_BRIDGE_WIRE_FORMAT_H_
