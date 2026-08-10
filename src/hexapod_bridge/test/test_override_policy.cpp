/**
 * @file test_override_policy.cpp
 * @author Lorenzo Argentieri (uni.lorenzo.a@gmail.com)
 * @brief Tests for the joypad override rule.
 *
 * SPDX-License-Identifier: MIT
 *
 * The policy takes the current time as an argument instead of reading a clock,
 * so a whole handover sequence runs here in no time at all.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "hexapod_bridge/override_policy.h"

namespace {

using hexapod::bridge::OverridePolicy;
using std::chrono::milliseconds;

/** @brief A fixed instant to measure everything else from. */
OverridePolicy::TimePoint Origin() { return OverridePolicy::TimePoint{} + milliseconds{100000}; }

TEST(OverridePolicy, RefusesATimeoutOfZero) { EXPECT_THROW(OverridePolicy{milliseconds{0}}, std::invalid_argument); }

TEST(OverridePolicy, RefusesANegativeTimeout) { EXPECT_THROW(OverridePolicy{milliseconds{-1}}, std::invalid_argument); }

TEST(OverridePolicy, KeepsTheConfiguredTimeout) {
  const OverridePolicy policy{milliseconds{1500}};
  EXPECT_EQ(policy.ReleaseAfter(), milliseconds{1500});
}

// Until somebody touches the joypad the stream owns the robot; a bridge that
// started out frozen would look like a dead link.
TEST(OverridePolicy, LetsTheStreamDriveBeforeAnyInput) {
  const OverridePolicy policy{milliseconds{1000}};

  EXPECT_TRUE(policy.StreamMayDrive(Origin()));
  EXPECT_FALSE(policy.OverrideActive(Origin()));
}

TEST(OverridePolicy, TakesTheOverrideOnInput) {
  OverridePolicy policy{milliseconds{1000}};
  policy.NoteJoypadActivity(Origin());

  EXPECT_TRUE(policy.OverrideActive(Origin()));
  EXPECT_FALSE(policy.StreamMayDrive(Origin()));
}

TEST(OverridePolicy, HoldsTheOverrideUntilTheTimeoutElapses) {
  OverridePolicy policy{milliseconds{1000}};
  policy.NoteJoypadActivity(Origin());

  EXPECT_TRUE(policy.OverrideActive(Origin() + milliseconds{1}));
  EXPECT_TRUE(policy.OverrideActive(Origin() + milliseconds{999}));
}

TEST(OverridePolicy, ReleasesExactlyAtTheTimeout) {
  OverridePolicy policy{milliseconds{1000}};
  policy.NoteJoypadActivity(Origin());

  EXPECT_FALSE(policy.OverrideActive(Origin() + milliseconds{1000}));
  EXPECT_TRUE(policy.StreamMayDrive(Origin() + milliseconds{1000}));
}

// Holding the stick keeps refreshing the activity, so the override must not
// lapse under a continuous input.
TEST(OverridePolicy, ContinuedInputKeepsTheOverride) {
  OverridePolicy policy{milliseconds{1000}};

  for (int elapsed = 0; elapsed <= 5000; elapsed += 500) {
    policy.NoteJoypadActivity(Origin() + milliseconds{elapsed});
    EXPECT_TRUE(policy.OverrideActive(Origin() + milliseconds{elapsed + 499}))
        << "released while the joypad was still being used, at " << elapsed << " ms";
  }
}

TEST(OverridePolicy, ReleasesOnDemand) {
  OverridePolicy policy{milliseconds{10000}};
  policy.NoteJoypadActivity(Origin());
  ASSERT_TRUE(policy.OverrideActive(Origin()));

  policy.Release();

  EXPECT_FALSE(policy.OverrideActive(Origin()));
  EXPECT_TRUE(policy.StreamMayDrive(Origin()));
}

TEST(OverridePolicy, CanBeTakenAgainAfterARelease) {
  OverridePolicy policy{milliseconds{1000}};
  policy.NoteJoypadActivity(Origin());
  policy.Release();
  ASSERT_TRUE(policy.StreamMayDrive(Origin()));

  policy.NoteJoypadActivity(Origin() + milliseconds{50});

  EXPECT_TRUE(policy.OverrideActive(Origin() + milliseconds{50}));
}

// A sample from before the last activity means the clock ran backwards. The
// robot stays frozen rather than being handed back on a bad comparison.
TEST(OverridePolicy, StaysActiveIfTheClockGoesBackwards) {
  OverridePolicy policy{milliseconds{1000}};
  policy.NoteJoypadActivity(Origin());

  EXPECT_TRUE(policy.OverrideActive(Origin() - milliseconds{5000}));
}

}  // namespace
