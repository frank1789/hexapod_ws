/**
 * @file main.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Entry point of the ZeroMQ bridge node.
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

#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <stdexcept>
#include <zmq.hpp>

#include "hexapod_bridge/bridge_node.h"

/**
 * @brief Start the bridge node.
 *
 * Every failure is reported and returns non-zero, so a supervisor — systemd or
 * the restart policy of the compose service — sees the node fail rather than
 * sit there having quietly given up.
 *
 * @return 0 on a clean shutdown, 1 if the node could not be brought up
 */
int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  const auto logger = rclcpp::get_logger("hexapod_bridge");

  try {
    rclcpp::spin(std::make_shared<hexapod::bridge::BridgeNode>());
  } catch (const zmq::error_t& error) {
    RCLCPP_FATAL(logger, "ZeroMQ failure: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (const std::invalid_argument& error) {
    RCLCPP_FATAL(logger, "invalid configuration: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (const std::runtime_error& error) {
    RCLCPP_FATAL(logger, "runtime error: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (const std::exception& error) {
    RCLCPP_FATAL(logger, "error occurred: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (...) {
    RCLCPP_FATAL(logger, "unknown failure occurred");
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
