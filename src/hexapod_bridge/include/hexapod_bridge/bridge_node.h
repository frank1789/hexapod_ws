/**
 * @file bridge_node.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief ROS 2 node receiving poses over ZeroMQ and arbitrating with the joypad.
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

#ifndef HEXAPOD_BRIDGE_BRIDGE_NODE_H_
#define HEXAPOD_BRIDGE_BRIDGE_NODE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <string>
#include <zmq.hpp>

#include "hexapod_bridge/override_policy.h"
#include "hexapod_msgs/msg/joypad_button.hpp"
#include "hexapod_msgs/msg/joypad_thumbstick.hpp"

namespace hexapod::bridge {

/**
 * @brief Brings animation poses into the ROS graph, with the joypad on top.
 *
 * ```
 *   Maya / Blender ──ZeroMQ──▶ BridgeNode ──sensor_msgs/JointState──▶ servos
 *                                  ▲
 *   joypad ──hexapod_msgs──────────┘   (pre-empts the stream)
 * ```
 *
 * The socket is polled from a ROS timer rather than a thread of its own, so
 * everything the node does happens on the executor and no locking is needed.
 *
 * The socket is opened with `ZMQ_CONFLATE`, which keeps only the most recent
 * message. That is not a tuning choice: a stalled link must not build a queue
 * of stale poses that the robot then replays at full rate. A late pose is
 * dropped in favour of the current one.
 *
 * Angles arrive in degrees, which is the unit the robot is configured in, and
 * are published in radians, which is what `sensor_msgs/msg/JointState` is
 * defined in. The conversion happens here, once.
 *
 * A malformed payload is logged and dropped, not thrown: this input crosses a
 * network, and a node that dies on a bad packet is a node an unrelated program
 * can switch the robot off with. Every rejection is counted and reported.
 */
class BridgeNode : public rclcpp::Node {
 public:
  /**
   * @brief Construct the node, open the socket and start polling.
   *
   * @throws std::invalid_argument if a parameter is out of range.
   * @throws zmq::error_t if the endpoint cannot be opened.
   */
  BridgeNode();

  /** @brief Close the socket and the context. */
  ~BridgeNode() override;

  BridgeNode(const BridgeNode&) = delete;
  BridgeNode& operator=(const BridgeNode&) = delete;
  BridgeNode(BridgeNode&&) = delete;
  BridgeNode& operator=(BridgeNode&&) = delete;

  /** @brief Number of payloads rejected by the format check so far. */
  [[nodiscard]] std::uint64_t RejectedCount() const noexcept;

  /** @brief Number of poses forwarded to the servos so far. */
  [[nodiscard]] std::uint64_t ForwardedCount() const noexcept;

 private:
  /** @brief Read and validate every node parameter. */
  void DeclareParameters();

  /** @brief Open the ZeroMQ socket and bind or connect it. */
  void OpenSocket();

  /** @brief Drain whatever the socket holds and publish the newest pose. */
  void PollSocket();

  /** @brief Decode one payload and publish it, or count it as rejected. */
  void HandlePayload(const std::string& t_payload);

  /** @brief Note deliberate joypad input and take the override. */
  void OnJoypadButton(const hexapod_msgs::msg::JoypadButton& t_message);

  /** @brief Note a thumbstick pushed past its dead zone. */
  void OnJoypadThumbstick(const hexapod_msgs::msg::JoypadThumbstick& t_message);

  /** @brief Report a change of owner exactly once per transition. */
  void ReportOwnership(bool t_stream_may_drive);

  zmq::context_t context_; /**< One I/O context for the node. */
  zmq::socket_t socket_;   /**< Subscriber carrying the pose stream. */

  std::optional<OverridePolicy> policy_; /**< Built once the timeout is known. */

  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_publisher_;
  rclcpp::Subscription<hexapod_msgs::msg::JoypadButton>::SharedPtr button_subscriber_;
  rclcpp::Subscription<hexapod_msgs::msg::JoypadThumbstick>::SharedPtr thumbstick_subscriber_;
  rclcpp::TimerBase::SharedPtr poll_timer_;

  std::string endpoint_;     /**< ZeroMQ endpoint, for instance tcp://0.0.0.0:5556. */
  std::string output_topic_; /**< Topic the decoded poses are published on. */

  std::uint64_t rejected_{0};      /**< Payloads refused by the format check. */
  std::uint64_t forwarded_{0};     /**< Poses passed on to the servos. */
  std::uint64_t last_sequence_{0}; /**< Last sender counter seen, for gap reporting. */

  double thumbstick_deadzone_{0.0}; /**< Magnitude below which a stick is ignored. */
  bool bind_{true};                 /**< Bind the endpoint rather than connect to it. */
  bool seen_sequence_{false};       /**< Whether last_sequence_ holds anything yet. */
  bool stream_was_driving_{true};   /**< Previous owner, so handovers log once. */
};

}  // namespace hexapod::bridge

#endif  // HEXAPOD_BRIDGE_BRIDGE_NODE_H_
