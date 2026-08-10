/**
 * @file wire_format.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Implementation of the ZeroMQ pose message format.
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

#include "hexapod_bridge/wire_format.h"

#include <fmt/format.h>

#include <cmath>
#include <nlohmann/json.hpp>
#include <numbers>
#include <stdexcept>

namespace hexapod::bridge {

namespace {

/** @brief Unit string the sender must declare, so a radian sender is refused. */
constexpr std::string_view kExpectedUnits{"deg"};

/** @brief Degrees in a half turn, the bridge between degrees and radians. */
constexpr double kHalfTurnDegrees{180.0};

constexpr std::string_view kFieldSchema{"schema"};
constexpr std::string_view kFieldSequence{"seq"};
constexpr std::string_view kFieldUnits{"units"};
constexpr std::string_view kFieldJoints{"joints"};

}  // namespace

JointPose WireFormat::Decode(const std::string_view t_payload) {
  if (t_payload.empty()) {
    throw std::invalid_argument("payload is empty");
  }

  const auto document = nlohmann::json::parse(t_payload, nullptr, false);
  if (document.is_discarded()) {
    throw std::invalid_argument("payload is not valid JSON");
  }
  if (!document.is_object()) {
    throw std::invalid_argument("payload is not a JSON object");
  }

  JointPose pose{};

  // The schema version is checked first: every other field only means anything
  // once both ends agree on what the fields are.
  const auto schema = document.find(kFieldSchema);
  if (schema == document.end() || !schema->is_number_unsigned()) {
    throw std::invalid_argument(R"(field "schema" is missing or is not an unsigned number)");
  }
  pose.schema = schema->get<std::uint32_t>();
  if (pose.schema != kSchemaVersion) {
    throw std::invalid_argument(
        fmt::format("schema version {} is not supported, this build speaks version {}", pose.schema, kSchemaVersion));
  }

  const auto units = document.find(kFieldUnits);
  if (units == document.end() || !units->is_string()) {
    throw std::invalid_argument(R"(field "units" is missing or is not a string)");
  }
  if (units->get<std::string>() != kExpectedUnits) {
    throw std::invalid_argument(
        fmt::format(R"(units "{}" are not supported, expected "{}")", units->get<std::string>(), kExpectedUnits));
  }

  // A missing sequence number is tolerated: it only serves to report gaps.
  if (const auto sequence = document.find(kFieldSequence);
      sequence != document.end() && sequence->is_number_unsigned()) {
    pose.sequence = sequence->get<std::uint64_t>();
  }

  const auto joints = document.find(kFieldJoints);
  if (joints == document.end() || !joints->is_object()) {
    throw std::invalid_argument(R"(field "joints" is missing or is not an object)");
  }
  if (joints->empty()) {
    throw std::invalid_argument(R"(field "joints" is empty, there is nothing to command)");
  }

  pose.names.reserve(joints->size());
  pose.degrees.reserve(joints->size());

  for (const auto& [name, value] : joints->items()) {
    if (name.empty()) {
      throw std::invalid_argument("a joint was given an empty name");
    }
    if (!value.is_number()) {
      throw std::invalid_argument(fmt::format("joint \"{}\" does not carry a number", name));
    }

    const auto angle = value.get<double>();
    if (!std::isfinite(angle)) {
      throw std::invalid_argument(fmt::format("joint \"{}\" has a non-finite angle", name));
    }
    if (angle < kMinAngleDegree || angle > kMaxAngleDegree) {
      throw std::invalid_argument(fmt::format("joint \"{}\" is at {} degrees, outside [{}, {}]", name, angle,
                                              kMinAngleDegree, kMaxAngleDegree));
    }

    pose.names.push_back(name);
    pose.degrees.push_back(angle);
  }

  return pose;
}

std::string WireFormat::Encode(const JointPose& t_pose) {
  if (t_pose.names.size() != t_pose.degrees.size()) {
    throw std::invalid_argument(fmt::format("{} names against {} angles", t_pose.names.size(), t_pose.degrees.size()));
  }

  nlohmann::json joints = nlohmann::json::object();
  for (std::size_t index = 0; index < t_pose.names.size(); ++index) {
    joints[t_pose.names[index]] = t_pose.degrees[index];
  }

  const nlohmann::json document{{std::string{kFieldSchema}, kSchemaVersion},
                                {std::string{kFieldSequence}, t_pose.sequence},
                                {std::string{kFieldUnits}, std::string{kExpectedUnits}},
                                {std::string{kFieldJoints}, joints}};

  return document.dump();
}

double WireFormat::DegreesToRadians(const double t_degrees) noexcept {
  return t_degrees * std::numbers::pi / kHalfTurnDegrees;
}

double WireFormat::RadiansToDegrees(const double t_radians) noexcept {
  return t_radians * kHalfTurnDegrees / std::numbers::pi;
}

}  // namespace hexapod::bridge
