/**
 * @file override_policy.h
 * @author Lorenzo Argentieri (uni.lorenzo.a@gmail.com)
 * @brief Decides whether the joypad or the streamed animation owns the robot.
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

#ifndef HEXAPOD_BRIDGE_OVERRIDE_POLICY_H_
#define HEXAPOD_BRIDGE_OVERRIDE_POLICY_H_

#include <chrono>

namespace hexapod::bridge {

/**
 * @brief Which source is allowed to command the joints.
 *
 * Two sources want the robot: a pose stream arriving over ZeroMQ from an
 * animation package, and the operator holding the joypad. The joypad wins.
 *
 * Taking hold of the joypad **freezes** the robot: the streamed poses stop
 * being forwarded, so the servos keep the last angles they were given. This is
 * the manual stop for a machine being driven by an animation, and it is
 * deliberately the whole of the behaviour — the joypad commands no angles of
 * its own, because nothing in this workspace knows how to turn a thumbstick
 * into eighteen joint angles. A gait node can publish poses later without this
 * class changing.
 *
 * Control returns to the stream once the joypad has been quiet for
 * `release_after`, so an operator who lets go does not leave the robot frozen
 * for ever. Releasing is not silent: the node logs the handover in both
 * directions.
 *
 * The class holds no clock of its own. Every method takes the current time,
 * which keeps the decision pure and lets the tests run a whole handover
 * sequence without waiting for real time to pass.
 */
class OverridePolicy {
 public:
  /** @brief Clock the caller is expected to sample. */
  using Clock = std::chrono::steady_clock;

  /** @brief Instant handed to every method. */
  using TimePoint = Clock::time_point;

  /**
   * @brief Build a policy.
   *
   * @param t_release_after how long the joypad must stay quiet before the
   * stream is allowed to drive again; must be positive
   *
   * @throws std::invalid_argument if the timeout is zero or negative, which
   * would either release the override immediately or never release it.
   */
  explicit OverridePolicy(std::chrono::milliseconds t_release_after);

  /**
   * @brief Record that the operator touched the joypad.
   *
   * Called for any input judged deliberate by the node: a button press, or a
   * thumbstick pushed past its dead zone. Noise around the centre of a worn
   * stick must not seize control, which is why the node filters before calling
   * this.
   *
   * @param t_now current time
   */
  void NoteJoypadActivity(TimePoint t_now) noexcept;

  /**
   * @brief Tell whether the joypad currently owns the robot.
   *
   * @param t_now current time
   * @return true while the override holds, false once it has lapsed
   */
  [[nodiscard]] bool OverrideActive(TimePoint t_now) const noexcept;

  /**
   * @brief Tell whether a streamed pose may be passed on to the servos.
   *
   * The exact complement of @ref OverrideActive, named for the decision the
   * caller is actually making.
   *
   * @param t_now current time
   * @return true when the stream is in charge
   */
  [[nodiscard]] bool StreamMayDrive(TimePoint t_now) const noexcept;

  /**
   * @brief Give up the override at once, without waiting for the timeout.
   *
   * Used when the node is asked to hand control back deliberately.
   */
  void Release() noexcept;

  /** @brief How long the joypad must be quiet before the stream resumes. */
  [[nodiscard]] std::chrono::milliseconds ReleaseAfter() const noexcept;

 private:
  /** @brief When the joypad was last touched; the epoch until it is. */
  TimePoint last_activity_;

  /** @brief Timeout after which the override lapses. */
  std::chrono::milliseconds release_after_{0};

  /** @brief Whether the joypad has ever been touched since the last release. */
  bool engaged_{false};
};

}  // namespace hexapod::bridge

#endif  // HEXAPOD_BRIDGE_OVERRIDE_POLICY_H_
