/**
 * @file override_policy.cc
 * @author Lorenzo Argentieri (uni.lorenzo.a@gmail.com)
 * @brief Implementation of the joypad override rule.
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

#include "hexapod_bridge/override_policy.h"

#include <stdexcept>

namespace hexapod::bridge {

OverridePolicy::OverridePolicy(const std::chrono::milliseconds t_release_after) : release_after_{t_release_after} {
  // A zero timeout releases the override in the same instant it is taken, which
  // would make the joypad useless; a negative one never releases it, which
  // would strand the robot after a single button press. Neither is a default
  // worth degrading into.
  if (t_release_after <= std::chrono::milliseconds::zero()) {
    throw std::invalid_argument("the joypad release timeout must be greater than zero");
  }
}

void OverridePolicy::NoteJoypadActivity(const TimePoint t_now) noexcept {
  last_activity_ = t_now;
  engaged_ = true;
}

bool OverridePolicy::OverrideActive(const TimePoint t_now) const noexcept {
  if (!engaged_) {
    return false;
  }

  // A sample taken before the last recorded activity means the clock went
  // backwards, which steady_clock forbids; treat it as "still active" rather
  // than handing the robot back on the strength of a time comparison.
  if (t_now < last_activity_) {
    return true;
  }

  return (t_now - last_activity_) < release_after_;
}

bool OverridePolicy::StreamMayDrive(const TimePoint t_now) const noexcept { return !OverrideActive(t_now); }

void OverridePolicy::Release() noexcept { engaged_ = false; }

std::chrono::milliseconds OverridePolicy::ReleaseAfter() const noexcept { return release_after_; }

}  // namespace hexapod::bridge
