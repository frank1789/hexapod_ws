/**
 * @file bridge_node.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Implementation of the ZeroMQ to ROS 2 pose bridge.
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

#include "hexapod_bridge/bridge_node.h"

#include <fmt/format.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

#include "hexapod_bridge/wire_format.h"

namespace hexapod::bridge {

namespace {

const std::string kDefaultEndpoint{"tcp://0.0.0.0:5556"};
const std::string kDefaultOutputTopic{"joint_command"};
const std::string kTopicJoypadButton{"joypad/button"};
const std::string kTopicJoypadThumbstick{"joypad/thumbstick"};

constexpr int kDefaultPollPeriodMs{5};
constexpr int kDefaultOverrideTimeoutMs{1500};
constexpr double kDefaultDeadzone{0.25};

/** @brief Depth of one: the newest pose is the only one worth keeping. */
constexpr int kQueueDepth{1};

/** @brief How often a repeated rejection may be logged, in milliseconds. */
constexpr int kLogThrottleMs{2000};

/** @brief Joypad input is sparse, but a press must not be dropped under load. */
constexpr int kJoypadQueueDepth{10};

}  // namespace

BridgeNode::BridgeNode() : Node("hexapod_bridge"), context_{1} {
  DeclareParameters();
  OpenSocket();

  joint_publisher_ = create_publisher<sensor_msgs::msg::JointState>(output_topic_, rclcpp::QoS(kQueueDepth));

  button_subscriber_ = create_subscription<hexapod_msgs::msg::JoypadButton>(
      kTopicJoypadButton, rclcpp::QoS(kJoypadQueueDepth),
      [this](const hexapod_msgs::msg::JoypadButton& message) { OnJoypadButton(message); });

  thumbstick_subscriber_ = create_subscription<hexapod_msgs::msg::JoypadThumbstick>(
      kTopicJoypadThumbstick, rclcpp::QoS(kJoypadQueueDepth),
      [this](const hexapod_msgs::msg::JoypadThumbstick& message) { OnJoypadThumbstick(message); });

  const auto poll_period = std::chrono::milliseconds{get_parameter("poll_period_ms").as_int()};
  poll_timer_ = create_wall_timer(poll_period, [this]() { PollSocket(); });

  RCLCPP_INFO(get_logger(), "listening on %s, publishing %s", endpoint_.c_str(), output_topic_.c_str());
  RCLCPP_INFO(get_logger(), "joypad overrides the stream and freezes the joints, releasing after %ld ms",
              static_cast<long>(policy_->ReleaseAfter().count()));
}

BridgeNode::~BridgeNode() {
  // Closing the socket before the context is destroyed keeps the destructor
  // from blocking on a peer that is still connected.
  try {
    socket_.close();
  } catch (const zmq::error_t& error) {
    RCLCPP_ERROR(get_logger(), "closing the socket failed: %s", error.what());
  }
}

void BridgeNode::DeclareParameters() {
  endpoint_ = declare_parameter<std::string>("endpoint", kDefaultEndpoint);
  if (endpoint_.empty()) {
    throw std::invalid_argument("endpoint must not be empty");
  }

  output_topic_ = declare_parameter<std::string>("output_topic", kDefaultOutputTopic);
  if (output_topic_.empty()) {
    throw std::invalid_argument("output_topic must not be empty");
  }

  bind_ = declare_parameter<bool>("bind", true);

  const auto poll_period_ms = declare_parameter<int>("poll_period_ms", kDefaultPollPeriodMs);
  if (poll_period_ms <= 0) {
    throw std::invalid_argument("poll_period_ms must be greater than zero");
  }

  const auto override_timeout_ms = declare_parameter<int>("override_timeout_ms", kDefaultOverrideTimeoutMs);
  if (override_timeout_ms <= 0) {
    throw std::invalid_argument("override_timeout_ms must be greater than zero");
  }
  policy_.emplace(std::chrono::milliseconds{override_timeout_ms});

  thumbstick_deadzone_ = declare_parameter<double>("thumbstick_deadzone", kDefaultDeadzone);
  if (thumbstick_deadzone_ < 0.0 || thumbstick_deadzone_ > 1.0) {
    throw std::invalid_argument("thumbstick_deadzone must be between 0 and 1");
  }
}

void BridgeNode::OpenSocket() {
  socket_ = zmq::socket_t{context_, zmq::socket_type::sub};

  // CONFLATE keeps only the most recent message, and must be set before the
  // endpoint is opened. It is what stops a stalled link from queueing poses
  // the robot would later replay in a burst. With a subscriber it only works
  // against an empty subscription, so no topic filter is offered.
  socket_.set(zmq::sockopt::conflate, 1);
  socket_.set(zmq::sockopt::subscribe, "");

  if (bind_) {
    socket_.bind(endpoint_);
  } else {
    socket_.connect(endpoint_);
  }
}

void BridgeNode::PollSocket() {
  zmq::message_t message;

  // CONFLATE leaves at most one message waiting, but the loop costs nothing and
  // keeps the node correct if the option is ever turned off.
  while (true) {
    zmq::recv_result_t received{};
    try {
      received = socket_.recv(message, zmq::recv_flags::dontwait);
    } catch (const zmq::error_t& error) {
      RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs, "receive failed: %s", error.what());
      return;
    }

    if (!received.has_value()) {
      return;
    }

    HandlePayload(message.to_string_view());
  }
}

void BridgeNode::HandlePayload(const std::string_view t_payload) {
  JointPose pose{};
  try {
    pose = WireFormat::Decode(t_payload);
  } catch (const std::exception& error) {
    ++rejected_;
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs, "rejected a payload (%lu so far): %s",
                          static_cast<unsigned long>(rejected_), error.what());
    return;
  }

  if (seen_sequence_ && pose.sequence > last_sequence_ + 1) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs,
                         "sequence jumped from %lu to %lu, %lu pose(s) were dropped in flight",
                         static_cast<unsigned long>(last_sequence_), static_cast<unsigned long>(pose.sequence),
                         static_cast<unsigned long>(pose.sequence - last_sequence_ - 1));
  }
  last_sequence_ = pose.sequence;
  seen_sequence_ = true;

  const auto now = OverridePolicy::Clock::now();
  const auto stream_may_drive = policy_->StreamMayDrive(now);
  ReportOwnership(stream_may_drive);

  if (!stream_may_drive) {
    // The joypad has the robot. Dropping the pose is the whole of the freeze:
    // the servo node keeps the last angles it was given.
    return;
  }

  sensor_msgs::msg::JointState command;
  command.header.stamp = this->now();
  command.name = std::move(pose.names);
  command.position.reserve(pose.degrees.size());
  for (const auto degrees : pose.degrees) {
    command.position.push_back(WireFormat::DegreesToRadians(degrees));
  }

  joint_publisher_->publish(command);
  ++forwarded_;
}

void BridgeNode::OnJoypadButton(const hexapod_msgs::msg::JoypadButton& t_message) {
  // Any button going down is deliberate. A release is ignored, so letting go
  // starts the release timeout rather than handing the robot back at once.
  if (t_message.value != 0) {
    policy_->NoteJoypadActivity(OverridePolicy::Clock::now());
  }
}

void BridgeNode::OnJoypadThumbstick(const hexapod_msgs::msg::JoypadThumbstick& t_message) {
  // A worn stick never rests exactly at zero. Only a deliberate push counts,
  // or the robot would be frozen by noise and never driven again.
  if (std::isfinite(t_message.vector_magnitute) && t_message.vector_magnitute > thumbstick_deadzone_) {
    policy_->NoteJoypadActivity(OverridePolicy::Clock::now());
  }
}

void BridgeNode::ReportOwnership(const bool t_stream_may_drive) {
  if (t_stream_may_drive == stream_was_driving_) {
    return;
  }
  stream_was_driving_ = t_stream_may_drive;

  if (t_stream_may_drive) {
    RCLCPP_INFO(get_logger(), "joypad released, the streamed poses drive the joints again");
  } else {
    RCLCPP_WARN(get_logger(), "joypad took over, the joints are frozen at their last angles");
  }
}

std::uint64_t BridgeNode::RejectedCount() const noexcept { return rejected_; }

std::uint64_t BridgeNode::ForwardedCount() const noexcept { return forwarded_; }

}  // namespace hexapod::bridge
