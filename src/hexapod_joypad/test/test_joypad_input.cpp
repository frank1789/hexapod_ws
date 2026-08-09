/**
 * @file test_joypad_input.cpp
 * @brief Unit tests for the joypad value remapping. No hardware and no ROS
 * graph are involved: these are pure functions of the raw axis values.
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

#include "joypad_button.h"
#include "joypad_thumbstick.h"
#include "joypad_trigger.h"

namespace {

constexpr double kTolerance{1e-9};

}  // namespace

// --- Button -----------------------------------------------------------------

TEST(Button, DefaultConstructedIsReleased) {
  // Joypad creates buttons through a map lookup, so the default state has to be
  // defined rather than whatever happened to be on the stack.
  const Button button;
  EXPECT_EQ(button.getValue(), 0);
  EXPECT_TRUE(button.getName().empty());
}

TEST(Button, KeepsNameAndState) {
  Button button{"Cross"};
  EXPECT_EQ(button.getName(), "Cross");
  EXPECT_EQ(button.getValue(), 0);

  button.setButton(1);
  EXPECT_EQ(button.getValue(), 1);
}

TEST(Button, StreamsItsState) {
  Button button{"Circle"};
  std::ostringstream released;
  released << button;
  EXPECT_NE(released.str().find("Circle"), std::string::npos);

  button.setButton(1);
  std::ostringstream pressed;
  pressed << button;
  EXPECT_NE(pressed.str().find("pressed"), std::string::npos);
}

// --- Trigger ----------------------------------------------------------------
// The driver reports a trigger as an axis running from 1.0 (released) to -1.0
// (fully pressed). The remap turns that into 0.0 to 1.0.

TEST(Trigger, ReleasedAxisBecomesZero) {
  Trigger trigger{"L2"};
  trigger.setValue(1.0);
  EXPECT_NEAR(trigger.getValue(), 0.0, kTolerance);
}

TEST(Trigger, FullyPressedAxisBecomesOne) {
  Trigger trigger{"L2"};
  trigger.setValue(-1.0);
  EXPECT_NEAR(trigger.getValue(), 1.0, kTolerance);
}

TEST(Trigger, MidTravelIsHalf) {
  Trigger trigger{"R2"};
  trigger.setValue(0.0);
  EXPECT_NEAR(trigger.getValue(), 0.5, kTolerance);
  EXPECT_EQ(trigger.getName(), "R2");
}

TEST(Trigger, ConstructorNormalisesToo) {
  const Trigger trigger{"L2", -1.0};
  EXPECT_NEAR(trigger.getValue(), 1.0, kTolerance);
}

// --- ThumbStick -------------------------------------------------------------
// The driver reports the x axis positive to the left; the remap inverts it so
// that the values follow the usual Cartesian convention.

TEST(ThumbStick, NeutralStickIsZero) {
  ThumbStick stick{"L3"};
  stick.setAxes(0.0, 0.0);

  const auto [x, y] = stick.getAxesValues();
  EXPECT_NEAR(x, 0.0, kTolerance);
  EXPECT_NEAR(y, 0.0, kTolerance);
  EXPECT_NEAR(stick.getMagnitude(), 0.0, kTolerance);
}

TEST(ThumbStick, InvertsTheRawHorizontalAxis) {
  ThumbStick stick{"L3"};
  stick.setAxes(-1.0, 0.0);  // raw "full right"

  const auto [x, y] = stick.getAxesValues();
  EXPECT_NEAR(x, 1.0, kTolerance);
  EXPECT_NEAR(y, 0.0, kTolerance);
  EXPECT_NEAR(stick.getAngle(), 0.0, kTolerance);
}

TEST(ThumbStick, FullLeftPointsAtPi) {
  ThumbStick stick{"L3"};
  stick.setAxes(1.0, 0.0);  // raw "full left"

  const auto [x, y] = stick.getAxesValues();
  EXPECT_NEAR(x, -1.0, kTolerance);
  EXPECT_NEAR(std::abs(stick.getAngle()), M_PI, kTolerance);
}

TEST(ThumbStick, FullUpPointsAtHalfPi) {
  ThumbStick stick{"L3"};
  stick.setAxes(0.0, 1.0);

  EXPECT_NEAR(stick.getAngle(), M_PI / 2.0, kTolerance);
  EXPECT_NEAR(stick.getMagnitude(), 1.0, kTolerance);
}

TEST(ThumbStick, DiagonalMagnitudeIsTheHypotenuse) {
  ThumbStick stick{"R3"};
  stick.setAxes(-1.0, 1.0);
  EXPECT_NEAR(stick.getMagnitude(), std::sqrt(2.0), kTolerance);
}

TEST(ThumbStick, NeverReportsNegativeZero) {
  // A negative zero prints as "-0.00000" and compares oddly; the remap folds it
  // to positive zero on purpose.
  ThumbStick stick{"L3"};
  stick.setAxes(-0.0, -0.0);

  const auto [x, y] = stick.getAxesValues();
  EXPECT_FALSE(std::signbit(x));
  EXPECT_FALSE(std::signbit(y));
}

TEST(ThumbStick, RawValuesAreKeptUntouched) {
  ThumbStick stick{"L3"};
  stick.setAxes(-0.5, 0.25);

  const auto [raw_x, raw_y] = stick.getRawAxesValues();
  EXPECT_NEAR(raw_x, -0.5, kTolerance);
  EXPECT_NEAR(raw_y, 0.25, kTolerance);
}
