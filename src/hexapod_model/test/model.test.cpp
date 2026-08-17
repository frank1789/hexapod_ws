/**
 * @file model.test.cpp
 * @brief Kinematics and configuration tests. None of them needs the robot.
 *
 * The geometry used here is a fixture, not the robot's: the real measurements
 * live in config/legs.lua and are deliberately absent from the C++. What these
 * tests establish is that the maths is self-consistent — that the inverse
 * really inverts the forward, and that the Jacobian really differentiates it —
 * which holds for any leg the configuration describes.
 */

#include "model.h"

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <numbers>
#include <string>
#include <vector>

#include "configuration_loader.h"

namespace {

using hexapod::model::BodyPose;
using hexapod::model::FootArray;
using hexapod::model::HexapodModel;
using hexapod::model::HipMount;
using hexapod::model::InverseKinematics;
using hexapod::model::InverseKinematicsStatus;
using hexapod::model::JointArray;
using hexapod::model::kLegCount;
using hexapod::model::LegConfiguration;
using hexapod::model::LegGeometry;
using hexapod::model::RobotConfiguration;
using hexapod::model::StatusArray;

// --- fixture constants ------------------------------------------------------
// A plausible leg, in the same shape as a configured one but no robot's.
constexpr double kCoxaLength{0.03};
constexpr double kFemurLength{0.08};
constexpr double kTibiaLength{0.12};
constexpr double kLateralOffset{-0.001};
constexpr double kTibiaOffset{-1.6};
constexpr double kJointLimit{std::numbers::pi / 2.0};
constexpr double kMountRadius{0.09};
constexpr std::size_t kLeftLegs{3};
constexpr int kAxes{3};

constexpr double kTolerance{1e-9};
constexpr double kJointTolerance{1e-8};
constexpr double kPoseTolerance{1e-7};
constexpr double kDerivativeTolerance{1e-6};
constexpr double kDerivativeStep{1e-6};

/// A plausible leg, in the same shape as a configured one.
LegGeometry TestGeometry(double sigma_coxa = -1.0) {
  LegGeometry geometry{};
  geometry.coxa_length = kCoxaLength;
  geometry.femur_length = kFemurLength;
  geometry.tibia_length = kTibiaLength;
  geometry.lateral_offset = kLateralOffset;
  geometry.tibia_offset = kTibiaOffset;
  geometry.sigma_coxa = sigma_coxa;
  geometry.joint_min = -kJointLimit;
  geometry.joint_max = kJointLimit;
  return geometry;
}

/// Six identical legs on a hexagon, enough to exercise the whole-robot paths.
RobotConfiguration TestRobot() {
  RobotConfiguration configuration{};
  for (std::size_t leg = 0; leg < kLegCount; ++leg) {
    const bool left = leg < kLeftLegs;
    const double yaw = (2.0 * std::numbers::pi * static_cast<double>(leg)) / static_cast<double>(kLegCount);

    Eigen::Matrix3d rotation{Eigen::Matrix3d::Identity()};
    rotation(0, 0) = std::cos(yaw);
    rotation(0, 1) = -std::sin(yaw);
    rotation(1, 0) = std::sin(yaw);
    rotation(1, 1) = std::cos(yaw);

    configuration.at(leg).geometry = TestGeometry(left ? -1.0 : 1.0);
    configuration.at(leg).mount =
        HipMount{Eigen::Vector3d{kMountRadius * std::cos(yaw), kMountRadius * std::sin(yaw), 0.0}, rotation};
  }
  return configuration;
}

/// A spread of joint triples across the configured travel.
std::vector<Eigen::Vector3d> RoundTripSamples() {
  constexpr std::array<double, 5> kCoxaSamples{-1.2, -0.4, 0.0, 0.4, 1.2};
  constexpr std::array<double, 5> kFemurSamples{-1.2, -0.3, 0.0, 0.5, 1.3};
  constexpr std::array<double, 5> kTibiaSamples{-1.0, -0.2, 0.0, 0.6, 1.4};

  std::vector<Eigen::Vector3d> samples;
  samples.reserve(kCoxaSamples.size() * kFemurSamples.size() * kTibiaSamples.size());
  for (const double coxa : kCoxaSamples) {
    for (const double femur : kFemurSamples) {
      for (const double tibia : kTibiaSamples) {
        samples.emplace_back(coxa, femur, tibia);
      }
    }
  }
  return samples;
}

/// The outward reach ForwardKinematics() produces for a pose.
///
/// Negative means the leg has folded back across its own coxa axis. The foot is
/// still somewhere, but InverseKinematics() documents that it returns the
/// outward branch, so a pose like that cannot round-trip through the joints.
/// Deriving the figure here rather than asserting a threshold keeps the test
/// honest if the fixture geometry changes.
double OutwardReach(const LegGeometry& geometry, const Eigen::Vector3d& joints) {
  const double femur = LegGeometry::SigmaFemur * joints.y();
  const double tibia = femur + (LegGeometry::SigmaTibia * joints.z()) + geometry.tibia_offset;
  return geometry.coxa_length + (geometry.femur_length * std::cos(femur)) + (geometry.tibia_length * std::cos(tibia));
}

// --- single leg -------------------------------------------------------------

TEST(LegModel, WrapToPiFoldsIntoRange) {
  constexpr double kTurn{2.0 * std::numbers::pi};
  constexpr double kSmall{0.5};

  EXPECT_NEAR(hexapod::model::WrapToPi(0.0), 0.0, kTolerance);
  EXPECT_NEAR(hexapod::model::WrapToPi(kSmall), kSmall, kTolerance);
  EXPECT_NEAR(hexapod::model::WrapToPi(kTurn + kSmall), kSmall, kTolerance);
  EXPECT_NEAR(hexapod::model::WrapToPi(-kTurn - kSmall), -kSmall, kTolerance);

  // Exactly half a turn is the one angle with two spellings in [-pi, pi], and
  // std::remainder resolves the tie to even rather than to a sign. Assert what
  // is actually guaranteed — the magnitude — instead of picking one.
  EXPECT_NEAR(std::abs(hexapod::model::WrapToPi(3.0 * std::numbers::pi)), std::numbers::pi, kTolerance);
}

TEST(LegModel, ForwardKinematicsAtZeroFollowsTheLinkLengths) {
  const LegGeometry geometry = TestGeometry();
  const Eigen::Vector3d foot = hexapod::model::ForwardKinematics(geometry, Eigen::Vector3d::Zero());

  // At the zero pose the leg is straight out along x, bent only by the constant
  // tibia offset, and displaced sideways by lateral_offset.
  const double expected_reach =
      geometry.coxa_length + geometry.femur_length + (geometry.tibia_length * std::cos(geometry.tibia_offset));
  const double expected_height = geometry.tibia_length * std::sin(geometry.tibia_offset);

  EXPECT_NEAR(foot.x(), expected_reach, kTolerance);
  EXPECT_NEAR(foot.y(), geometry.lateral_offset, kTolerance);
  EXPECT_NEAR(foot.z(), expected_height, kTolerance);
  EXPECT_LT(foot.z(), 0.0) << "the tibia bend should put the foot below the hip";
}

TEST(LegModel, InverseKinematicsRecoversTheJointsOnTheOutwardBranch) {
  constexpr std::size_t kMinimumChecked{150};

  std::size_t checked = 0;
  for (const double sigma : {-1.0, 1.0}) {
    const LegGeometry geometry = TestGeometry(sigma);
    for (const Eigen::Vector3d& joints : RoundTripSamples()) {
      if (OutwardReach(geometry, joints) <= 0.0) {
        continue;  // folded back across the coxa axis; see the test below
      }

      const Eigen::Vector3d foot = hexapod::model::ForwardKinematics(geometry, joints);
      const auto solved = InverseKinematics(geometry, foot);
      ++checked;

      ASSERT_EQ(solved.status, InverseKinematicsStatus::Success)
          << "sigma " << sigma << " joints " << joints.transpose();
      EXPECT_NEAR(solved.q.x(), joints.x(), kJointTolerance) << "coxa, joints " << joints.transpose();
      EXPECT_NEAR(solved.q.y(), joints.y(), kJointTolerance) << "femur, joints " << joints.transpose();
      EXPECT_NEAR(solved.q.z(), joints.z(), kJointTolerance) << "tibia, joints " << joints.transpose();
    }
  }
  EXPECT_GT(checked, kMinimumChecked) << "the filter should skip a minority of poses, not most of them";
}

TEST(LegModel, WhereverInverseKinematicsSucceedsTheFootLandsOnTheTarget) {
  // The property that holds for every pose, folded ones included. A folded pose
  // may put the foot somewhere the outward branch genuinely cannot go, and that
  // is reported as Unreachable; but whenever a solution *is* returned, forward
  // kinematics of it must land back on the target.
  for (const double sigma : {-1.0, 1.0}) {
    const LegGeometry geometry = TestGeometry(sigma);
    for (const Eigen::Vector3d& joints : RoundTripSamples()) {
      const Eigen::Vector3d target = hexapod::model::ForwardKinematics(geometry, joints);
      const auto solved = InverseKinematics(geometry, target);

      if (solved.status == InverseKinematicsStatus::Unreachable) {
        EXPECT_LE(OutwardReach(geometry, joints), 0.0)
            << "only a folded pose may be unreachable; joints " << joints.transpose();
        continue;
      }

      const Eigen::Vector3d landed = hexapod::model::ForwardKinematics(geometry, solved.q);
      EXPECT_TRUE(landed.isApprox(target, kPoseTolerance))
          << "target " << target.transpose() << " landed " << landed.transpose();
    }
  }
}

TEST(LegModel, InverseKinematicsReportsTargetsBeyondReach) {
  constexpr double kFarAway{10.0};
  const LegGeometry geometry = TestGeometry();
  const auto solved = InverseKinematics(geometry, Eigen::Vector3d{kFarAway, 0.0, 0.0});
  EXPECT_EQ(solved.status, InverseKinematicsStatus::Unreachable);
}

TEST(LegModel, InverseKinematicsReportsTargetsInsideTheDeadCylinder) {
  constexpr double kWideOffset{0.05};
  constexpr double kNearAxis{0.01};
  constexpr double kBelow{-0.1};

  LegGeometry geometry = TestGeometry();
  geometry.lateral_offset = kWideOffset;  // a leg plane well off the coxa axis
  const auto solved = InverseKinematics(geometry, Eigen::Vector3d{kNearAxis, 0.0, kBelow});
  EXPECT_EQ(solved.status, InverseKinematicsStatus::Unreachable);
}

TEST(LegModel, InverseKinematicsRefusesAnUnconfiguredLeg) {
  // The whole point of leaving the measurements out of the C++: a leg that was
  // never configured must not produce joint angles made of NaNs.
  constexpr double kSomewhere{0.1};

  const LegGeometry unconfigured{};
  const auto solved = InverseKinematics(unconfigured, Eigen::Vector3d{kSomewhere, 0.0, -kSomewhere});

  EXPECT_EQ(solved.status, InverseKinematicsStatus::Unreachable);
  EXPECT_TRUE(solved.q.allFinite());
}

TEST(LegModel, InverseKinematicsSeparatesOutOfRangeFromUnreachable) {
  constexpr double kNarrowLimit{0.5};
  const Eigen::Vector3d joints{1.4, 0.2, 0.2};

  LegGeometry geometry = TestGeometry();
  const Eigen::Vector3d foot = hexapod::model::ForwardKinematics(geometry, joints);

  // The same target, with the travel narrowed so the pose is geometrically fine
  // but mechanically out of range.
  geometry.joint_min = -kNarrowLimit;
  geometry.joint_max = kNarrowLimit;
  const auto solved = InverseKinematics(geometry, foot);

  EXPECT_EQ(solved.status, InverseKinematicsStatus::LimitViolation);
  EXPECT_FALSE(solved.Ok());
  EXPECT_NEAR(solved.q.x(), joints.x(), kJointTolerance) << "the offending angle is still reported";
}

TEST(LegModel, JacobianMatchesFiniteDifferences) {
  const LegGeometry geometry = TestGeometry();
  const std::array<Eigen::Vector3d, kAxes> poses{Eigen::Vector3d{0.0, 0.0, 0.0}, Eigen::Vector3d{0.3, -0.4, 0.5},
                                                 Eigen::Vector3d{-0.9, 0.7, -0.2}};

  for (const Eigen::Vector3d& joints : poses) {
    const auto state = hexapod::model::ForwardKinematicsWithJacobian(geometry, joints);
    EXPECT_TRUE(state.foot.isApprox(hexapod::model::ForwardKinematics(geometry, joints)));

    for (int column = 0; column < kAxes; ++column) {
      Eigen::Vector3d forward = joints;
      Eigen::Vector3d backward = joints;
      forward(column) += kDerivativeStep;
      backward(column) -= kDerivativeStep;

      const Eigen::Vector3d numeric = (hexapod::model::ForwardKinematics(geometry, forward) -
                                       hexapod::model::ForwardKinematics(geometry, backward)) /
                                      (2.0 * kDerivativeStep);

      for (int row = 0; row < kAxes; ++row) {
        EXPECT_NEAR(state.jacobian(row, column), numeric(row), kDerivativeTolerance)
            << "entry (" << row << ", " << column << ") at joints " << joints.transpose();
      }
    }
  }
}

// --- whole robot ------------------------------------------------------------

TEST(HexapodModel, SolveJointsInvertsFeetFromJoints) {
  const HexapodModel model{TestRobot()};
  ASSERT_TRUE(model.IsUsable());

  // A body that is neither at the origin nor level, so the frame changes are
  // actually exercised rather than cancelling out.
  const Eigen::Vector3d body_position{0.05, -0.02, 0.11};
  constexpr double kYaw{0.15};
  constexpr double kPitch{0.08};
  const BodyPose body{body_position, Eigen::Quaterniond{Eigen::AngleAxisd{kYaw, Eigen::Vector3d::UnitZ()} *
                                                        Eigen::AngleAxisd{kPitch, Eigen::Vector3d::UnitY()}}};

  const Eigen::Vector3d nominal_joints{0.2, -0.3, 0.4};
  JointArray joints{};
  joints.fill(nominal_joints);

  FootArray feet{};
  model.FeetFromJoints(body, joints, &feet);

  JointArray solved{};
  StatusArray status{};
  ASSERT_TRUE(model.SolveJoints(body, feet, &solved, &status));

  for (std::size_t leg = 0; leg < kLegCount; ++leg) {
    EXPECT_EQ(status.at(leg), InverseKinematicsStatus::Success) << "leg " << leg;
    EXPECT_TRUE(solved.at(leg).isApprox(joints.at(leg), kPoseTolerance))
        << "leg " << leg << " expected " << joints.at(leg).transpose() << " got " << solved.at(leg).transpose();
  }
}

TEST(HexapodModel, SolveJointsRejectsNullOutputs) {
  const HexapodModel model{TestRobot()};
  const BodyPose body{};
  const FootArray feet{};
  StatusArray status{};
  JointArray joints{};

  EXPECT_FALSE(model.SolveJoints(body, feet, nullptr, &status));
  EXPECT_FALSE(model.SolveJoints(body, feet, &joints, nullptr));
}

TEST(HexapodModel, AnUnconfiguredRobotIsNotUsable) {
  const HexapodModel model{RobotConfiguration{}};
  EXPECT_FALSE(model.IsUsable());
}

TEST(HexapodModel, StabilityMarginIsPositiveInsideAndNegativeOutside) {
  constexpr double kHalfWidth{0.10};
  constexpr double kInside{0.05};
  constexpr double kOutside{0.20};

  FootArray feet{};
  feet.at(0) = Eigen::Vector3d{kHalfWidth, kHalfWidth, 0.0};
  feet.at(1) = Eigen::Vector3d{-kHalfWidth, kHalfWidth, 0.0};
  feet.at(2) = Eigen::Vector3d{-kHalfWidth, -kHalfWidth, 0.0};
  feet.at(3) = Eigen::Vector3d{kHalfWidth, -kHalfWidth, 0.0};

  const std::vector<std::size_t> supporting{0, 1, 2, 3};

  // Dead centre of a square: the margin is the distance to any side.
  EXPECT_NEAR(HexapodModel::StabilityMargin(Eigen::Vector3d::Zero(), feet, supporting), kHalfWidth, kTolerance);
  // Shifted towards one side: the margin is the distance to that side.
  EXPECT_NEAR(HexapodModel::StabilityMargin(Eigen::Vector3d{kInside, 0.0, 0.0}, feet, supporting), kHalfWidth - kInside,
              kTolerance);
  EXPECT_LT(HexapodModel::StabilityMargin(Eigen::Vector3d{kOutside, 0.0, 0.0}, feet, supporting), 0.0);
}

TEST(HexapodModel, StabilityMarginRefusesADegenerateSupportPolygon) {
  constexpr std::size_t kOutOfRangeLeg{99};
  constexpr double kStep{0.1};
  constexpr double kFarStep{0.2};

  FootArray feet{};
  feet.at(0) = Eigen::Vector3d{0.0, 0.0, 0.0};
  feet.at(1) = Eigen::Vector3d{kStep, 0.0, 0.0};
  feet.at(2) = Eigen::Vector3d{kFarStep, 0.0, 0.0};

  const std::vector<std::size_t> two{0, 1};
  const std::vector<std::size_t> collinear{0, 1, 2};
  const std::vector<std::size_t> out_of_range{0, 1, kOutOfRangeLeg};

  EXPECT_FALSE(std::isfinite(HexapodModel::StabilityMargin(Eigen::Vector3d::Zero(), feet, two)));
  EXPECT_FALSE(std::isfinite(HexapodModel::StabilityMargin(Eigen::Vector3d::Zero(), feet, collinear)));
  EXPECT_FALSE(std::isfinite(HexapodModel::StabilityMargin(Eigen::Vector3d::Zero(), feet, out_of_range)));
}

// --- configuration ----------------------------------------------------------

/// Write a Lua script to a scratch file and hand back its path.
std::string WriteScript(const std::string& name, const std::string& body) {
  const std::string path = testing::TempDir() + name;
  std::ofstream out{path};
  out << body;
  out.close();
  return path;
}

/// A complete leg entry, so each test below can break exactly one thing.
std::string LegEntry(const std::string& overrides = "") {
  return R"(  { coxa_length = 0.03, femur_length = 0.08, tibia_length = 0.12,
      lateral_offset = -0.001, tibia_offset = -1.6, sigma_coxa = -1.0,
      joint_min = -1.5708, joint_max = 1.5708,
      mount = { x = 0.09, y = 0.0, z = 0.0, yaw = 0.0 })" +
         overrides + " }";
}

std::string CompleteScript(const std::string& overrides = "") {
  return "Legs = {\n  L_front = " + LegEntry(overrides) + ",\n  L_mid = " + LegEntry() + ",\n  L_back = " + LegEntry() +
         ",\n  R_front = " + LegEntry() + ",\n  R_mid = " + LegEntry() + ",\n  R_back = " + LegEntry() + ",\n}\n";
}

/// Loading must fail. The result is named rather than discarded because
/// LoadRobotConfiguration is [[nodiscard]], and dropping it inside EXPECT_THROW
/// makes the compiler warn about the ignored return value.
void ExpectLoadThrows(const std::string& path) {
  SCOPED_TRACE("loading " + path);
  EXPECT_THROW(
      { [[maybe_unused]] const RobotConfiguration unused = hexapod::model::LoadRobotConfiguration(path); },
      std::runtime_error);
}

TEST(ConfigurationLoader, LoadsACompleteScript) {
  const std::string path = WriteScript("hexapod_legs_complete.lua", CompleteScript());
  const RobotConfiguration configuration = hexapod::model::LoadRobotConfiguration(path);

  for (const LegConfiguration& leg : configuration) {
    EXPECT_TRUE(leg.geometry.IsUsable());
    EXPECT_NEAR(leg.geometry.femur_length, kFemurLength, kTolerance);
  }
  EXPECT_TRUE(HexapodModel{configuration}.IsUsable());
}

TEST(ConfigurationLoader, RejectsAMissingFile) { ExpectLoadThrows(testing::TempDir() + "no_such_legs.lua"); }

TEST(ConfigurationLoader, RejectsAScriptWithoutTheLegsTable) {
  ExpectLoadThrows(WriteScript("hexapod_legs_empty.lua", "Other = {}\n"));
}

TEST(ConfigurationLoader, RejectsAMissingLeg) {
  ExpectLoadThrows(WriteScript("hexapod_legs_partial.lua", "Legs = {\n  L_front = " + LegEntry() + ",\n}\n"));
}

TEST(ConfigurationLoader, RejectsAMissingField) {
  // The same entry with femur_length taken out.
  ExpectLoadThrows(WriteScript("hexapod_legs_no_femur.lua",
                               "Legs = {\n  L_front = { coxa_length = 0.03, tibia_length = 0.12,\n"
                               "    lateral_offset = 0.0, tibia_offset = -1.6, sigma_coxa = -1.0,\n"
                               "    joint_min = -1.5, joint_max = 1.5,\n"
                               "    mount = { x = 0.0, y = 0.0, z = 0.0, yaw = 0.0 } },\n}\n"));
}

TEST(ConfigurationLoader, RejectsAnUnsolvableLeg) {
  // sigma_coxa must be exactly +1 or -1; 0 would divide by zero in the solver.
  ExpectLoadThrows(WriteScript("hexapod_legs_bad_sigma.lua", CompleteScript(", sigma_coxa = 0.0")));
}

TEST(ConfigurationLoader, RejectsAMissingMount) {
  ExpectLoadThrows(WriteScript("hexapod_legs_nomount.lua",
                               "Legs = {\n  L_front = { coxa_length = 0.03, femur_length = 0.08,\n"
                               "    tibia_length = 0.12, lateral_offset = 0.0, tibia_offset = -1.6,\n"
                               "    sigma_coxa = -1.0, joint_min = -1.5, joint_max = 1.5 },\n}\n"));
}

}  // namespace
