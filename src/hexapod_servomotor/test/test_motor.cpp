/**
 * @file test_motor.cpp
 * @brief Unit tests for hexapod::Motor and the angle mapping helper. Nothing
 * here touches the I2C bus.
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include <sstream>

#include "motor.h"
#include "utility_function.h"

namespace {

constexpr double kTolerance{1e-9};

}  // namespace

TEST(Motor, DefaultConstructedHasNoChannel) {
  const hexapod::Motor motor;
  EXPECT_TRUE(motor.GetNameMotor().empty());
  EXPECT_EQ(motor.GetPinMotor(), -1);
  EXPECT_NEAR(motor.GetAngle(), 0.0, kTolerance);
}

TEST(Motor, KeepsNameAndChannel) {
  const hexapod::Motor motor{"L_coxaA", 3};
  EXPECT_EQ(motor.GetNameMotor(), "L_coxaA");
  EXPECT_EQ(motor.GetPinMotor(), 3);
}

TEST(Motor, ClampsAnglesBelowZero) {
  hexapod::Motor motor{"L_femurA", 1};
  motor.SetAngle(-10.0);
  EXPECT_NEAR(motor.GetAngle(), 0.0, kTolerance);
}

TEST(Motor, ClampsAnglesAtTheTopOfTheTravel) {
  hexapod::Motor motor{"L_femurA", 1};
  motor.SetAngle(200.0);
  EXPECT_NEAR(motor.GetAngle(), 179.0, kTolerance);

  motor.SetAngle(180.0);
  EXPECT_NEAR(motor.GetAngle(), 179.0, kTolerance);
}

TEST(Motor, KeepsAnglesInsideTheTravel) {
  hexapod::Motor motor{"L_tibiaA", 2};
  motor.SetAngle(90.0);
  EXPECT_NEAR(motor.GetAngle(), 90.0, kTolerance);
}

TEST(Motor, EqualityComparesIdentityNotPosition) {
  // Two objects describing the same physical servo are equal whatever angle
  // they currently hold.
  const hexapod::Motor resting{"R_coxaB", 4, 10.0};
  const hexapod::Motor moved{"R_coxaB", 4, 120.0};
  EXPECT_EQ(resting, moved);

  const hexapod::Motor other_channel{"R_coxaB", 5, 10.0};
  EXPECT_NE(resting, other_channel);

  const hexapod::Motor other_name{"L_coxaB", 4, 10.0};
  EXPECT_NE(resting, other_name);
}

TEST(Motor, StreamsItsIdentity) {
  const hexapod::Motor motor{"R_tibiaC", 8};
  std::ostringstream stream;
  stream << motor;
  EXPECT_NE(stream.str().find("R_tibiaC"), std::string::npos);
}

// --- angle to pulse width ---------------------------------------------------

TEST(MapRange, MapsTheEndsOfTheTravel) {
  EXPECT_NEAR(hexapod::map(0.0, 0.0, 180.0, 650.0, 2350.0), 650.0, kTolerance);
  EXPECT_NEAR(hexapod::map(180.0, 0.0, 180.0, 650.0, 2350.0), 2350.0, kTolerance);
}

TEST(MapRange, MapsTheMiddleOfTheTravel) {
  EXPECT_NEAR(hexapod::map(90.0, 0.0, 180.0, 650.0, 2350.0), 1500.0, kTolerance);
}
