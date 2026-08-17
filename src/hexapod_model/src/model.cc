/**
 * @file model.cc
 * @brief The whole-robot kinematics, built on the single-leg maths.
 */

#include "model.h"

#include <Eigen/Geometry>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace hexapod::model {

namespace {

/// A supporting foot, flattened onto the ground plane.
struct GroundPoint {
  double x{};
  double y{};
};

/// Positive when c lies to the left of the directed line a->b.
[[nodiscard]] double Cross(const GroundPoint& origin, const GroundPoint& first, const GroundPoint& second) noexcept {
  return ((first.x - origin.x) * (second.y - origin.y)) - ((first.y - origin.y) * (second.x - origin.x));
}

/**
 * @brief Convex hull of the supporting feet, counter-clockwise.
 *
 * Andrew's monotone chain. There are never more than six points, so the sort
 * dominates and neither is worth optimising.
 */
[[nodiscard]] std::vector<GroundPoint> ConvexHull(std::vector<GroundPoint> points) {
  std::sort(points.begin(), points.end(), [](const GroundPoint& lhs, const GroundPoint& rhs) {
    return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
  });

  const std::size_t count = points.size();
  std::vector<GroundPoint> hull(2 * count);
  std::size_t size = 0;

  for (std::size_t i = 0; i < count; ++i) {  // lower hull
    while (size >= 2 && Cross(hull.at(size - 2), hull.at(size - 1), points.at(i)) <= 0.0) {
      --size;
    }
    hull.at(size++) = points.at(i);
  }

  const std::size_t lower = size + 1;
  for (std::size_t i = count - 1; i > 0; --i) {  // upper hull
    while (size >= lower && Cross(hull.at(size - 2), hull.at(size - 1), points.at(i - 1)) <= 0.0) {
      --size;
    }
    hull.at(size++) = points.at(i - 1);
  }

  hull.resize(size == 0 ? 0 : size - 1);  // the first point is repeated at the end
  return hull;
}

}  // namespace

HexapodModel::HexapodModel(const RobotConfiguration& configuration) noexcept : configuration_{configuration} {}

bool HexapodModel::IsUsable() const noexcept {
  return std::all_of(configuration_.begin(), configuration_.end(),
                     [](const LegConfiguration& leg) { return leg.geometry.IsUsable(); });
}

bool HexapodModel::SolveJoints(const BodyPose& body, const FootArray& feet_world, JointArray* joints,
                               StatusArray* status) const noexcept {
  if (joints == nullptr || status == nullptr) {
    return false;
  }

  const Eigen::Matrix3d world_to_body = body.WorldToBody();

  bool all_ok = true;
  for (std::size_t leg = 0; leg < kLegCount; ++leg) {
    const Eigen::Vector3d foot_body = world_to_body * (feet_world.at(leg) - body.position());
    const LegConfiguration& configured = configuration_.at(leg);

    const InverseKinematicsResult solved = InverseKinematics(configured.geometry, configured.mount.ToHip(foot_body));

    joints->at(leg) = solved.q;
    status->at(leg) = solved.status;
    all_ok = all_ok && solved.Ok();
  }

  return all_ok;
}

void HexapodModel::FeetFromJoints(const BodyPose& body, const JointArray& joints,
                                  FootArray* feet_world) const noexcept {
  if (feet_world == nullptr) {
    return;
  }

  const Eigen::Matrix3d body_to_world = body.orientation().toRotationMatrix();

  for (std::size_t leg = 0; leg < kLegCount; ++leg) {
    const LegConfiguration& configured = configuration_.at(leg);
    const Eigen::Vector3d foot_hip = ForwardKinematics(configured.geometry, joints.at(leg));
    const Eigen::Vector3d foot_body = configured.mount.ToBody(foot_hip);
    feet_world->at(leg) = body.position() + (body_to_world * foot_body);
  }
}

double HexapodModel::StabilityMargin(const Eigen::Vector3d& com_world, const FootArray& feet_world,
                                     std::span<const std::size_t> supporting) noexcept {
  // No polygon at all is not a small margin, it is the absence of one. Saying
  // so with -infinity keeps every "margin > threshold" test honest.
  constexpr double kNoSupport = -std::numeric_limits<double>::infinity();
  constexpr std::size_t kMinimumSupport = 3;

  if (supporting.size() < kMinimumSupport) {
    return kNoSupport;
  }

  std::vector<GroundPoint> points;
  points.reserve(supporting.size());
  for (const std::size_t leg : supporting) {
    if (leg >= kLegCount) {
      // An out-of-range index is a caller error, not a geometry to guess at.
      return kNoSupport;
    }
    points.push_back(GroundPoint{feet_world.at(leg).x(), feet_world.at(leg).y()});
  }

  const std::vector<GroundPoint> hull = ConvexHull(std::move(points));
  if (hull.size() < kMinimumSupport) {
    // Collinear or coincident feet enclose no area.
    return kNoSupport;
  }

  // Distance to the nearest edge, signed positive towards the interior. The
  // hull is counter-clockwise, so the interior is to the left of every edge.
  const GroundPoint centre{com_world.x(), com_world.y()};
  double margin = std::numeric_limits<double>::infinity();

  for (std::size_t i = 0; i < hull.size(); ++i) {
    const GroundPoint& from = hull.at(i);
    const GroundPoint& to = hull.at((i + 1) % hull.size());

    const double edge_x = to.x - from.x;
    const double edge_y = to.y - from.y;
    const double length = std::hypot(edge_x, edge_y);
    if (length == 0.0) {
      continue;
    }

    const double signed_distance = (((centre.y - from.y) * edge_x) - ((centre.x - from.x) * edge_y)) / length;
    margin = std::min(margin, -signed_distance);
  }

  return margin;
}

}  // namespace hexapod::model
