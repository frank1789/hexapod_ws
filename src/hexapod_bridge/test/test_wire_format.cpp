/**
 * @file test_wire_format.cpp
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Tests for the ZeroMQ payload format and its validation.
 *
 * SPDX-License-Identifier: MIT
 *
 * Nothing here opens a socket. The point of keeping the format in its own
 * translation unit is that every rejection can be provoked directly.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

#include "hexapod_bridge/wire_format.h"

namespace {

using hexapod::bridge::JointPose;
using hexapod::bridge::WireFormat;

/** @brief A payload that must always be accepted. */
std::string ValidPayload() { return R"({"schema":1,"seq":7,"units":"deg","joints":{"L_coxaA":90.0,"R_tibiaC":12.5}})"; }

TEST(WireFormat, DecodesAValidPayload) {
  const auto pose = WireFormat::Decode(ValidPayload());

  EXPECT_EQ(pose.schema, 1U);
  EXPECT_EQ(pose.sequence, 7U);
  ASSERT_EQ(pose.names.size(), 2U);
  ASSERT_EQ(pose.degrees.size(), 2U);
}

TEST(WireFormat, KeepsNamesAndAnglesTogether) {
  const auto pose = WireFormat::Decode(ValidPayload());

  for (std::size_t index = 0; index < pose.names.size(); ++index) {
    if (pose.names[index] == "L_coxaA") {
      EXPECT_DOUBLE_EQ(pose.degrees[index], 90.0);
    } else if (pose.names[index] == "R_tibiaC") {
      EXPECT_DOUBLE_EQ(pose.degrees[index], 12.5);
    } else {
      FAIL() << "unexpected joint name: " << pose.names[index];
    }
  }
}

TEST(WireFormat, RoundTripsThroughEncode) {
  JointPose pose{};
  pose.names = {"L_femurB", "R_coxaA"};
  pose.degrees = {45.0, 135.0};
  pose.sequence = 99;
  pose.schema = WireFormat::kSchemaVersion;

  const auto decoded = WireFormat::Decode(WireFormat::Encode(pose));

  EXPECT_EQ(decoded.sequence, 99U);
  EXPECT_EQ(decoded.names.size(), 2U);
}

TEST(WireFormat, EncodeRefusesMismatchedVectors) {
  JointPose pose{};
  pose.names = {"L_coxaA", "L_coxaB"};
  pose.degrees = {10.0};

  EXPECT_THROW(static_cast<void>(WireFormat::Encode(pose)), std::invalid_argument);
}

// --- every rejection path ---------------------------------------------------

TEST(WireFormat, RejectsAnEmptyPayload) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode("")), std::invalid_argument);
}

TEST(WireFormat, RejectsSomethingThatIsNotJson) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode("not json at all")), std::invalid_argument);
}

TEST(WireFormat, RejectsJsonThatIsNotAnObject) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode("[1, 2, 3]")), std::invalid_argument);
}

TEST(WireFormat, RejectsAMissingSchema) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"units":"deg","joints":{"L_coxaA":90.0}})")),
               std::invalid_argument);
}

// A sender built against a later format must be refused rather than
// half-understood: the robot would otherwise act on fields it cannot read.
TEST(WireFormat, RejectsAnUnknownSchemaVersion) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":99,"units":"deg","joints":{"L_coxaA":90.0}})")),
               std::invalid_argument);
}

// The unit field exists precisely so that a sender working in radians is
// refused instead of driving every joint to a fraction of its intended angle.
TEST(WireFormat, RejectsRadians) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"rad","joints":{"L_coxaA":1.57}})")),
               std::invalid_argument);
}

TEST(WireFormat, RejectsAMissingUnits) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"joints":{"L_coxaA":90.0}})")),
               std::invalid_argument);
}

TEST(WireFormat, RejectsMissingJoints) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg"})")), std::invalid_argument);
}

TEST(WireFormat, RejectsAnEmptyJointSet) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{}})")),
               std::invalid_argument);
}

TEST(WireFormat, RejectsAJointThatIsNotANumber) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{"L_coxaA":"ninety"}})")),
               std::invalid_argument);
}

TEST(WireFormat, RejectsAnAngleBelowTheRange) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{"L_coxaA":-0.5}})")),
               std::invalid_argument);
}

TEST(WireFormat, RejectsAnAngleAboveTheRange) {
  EXPECT_THROW(static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{"L_coxaA":180.5}})")),
               std::invalid_argument);
}

TEST(WireFormat, AcceptsBothEndsOfTheRange) {
  EXPECT_NO_THROW(
      static_cast<void>(WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{"low":0.0,"high":180.0}})")));
}

TEST(WireFormat, ToleratesAMissingSequence) {
  const auto pose = WireFormat::Decode(R"({"schema":1,"units":"deg","joints":{"L_coxaA":90.0}})");
  EXPECT_EQ(pose.sequence, 0U);
}

// --- unit conversion --------------------------------------------------------

TEST(WireFormat, ConvertsDegreesToRadians) {
  EXPECT_DOUBLE_EQ(WireFormat::DegreesToRadians(180.0), std::numbers::pi);
  EXPECT_DOUBLE_EQ(WireFormat::DegreesToRadians(0.0), 0.0);
  EXPECT_DOUBLE_EQ(WireFormat::DegreesToRadians(90.0), std::numbers::pi / 2.0);
}

TEST(WireFormat, ConvertsRadiansBackToDegrees) {
  EXPECT_DOUBLE_EQ(WireFormat::RadiansToDegrees(std::numbers::pi), 180.0);
  EXPECT_NEAR(WireFormat::RadiansToDegrees(WireFormat::DegreesToRadians(37.25)), 37.25, 1e-9);
}

}  // namespace
