/**
 * @file PCA9685_driver.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PCA9685_DRIVER_H_
#define PCA9685_DRIVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "i2c-peripheral.h"

namespace adafruit {

class PCA9685 {
 public:
  /**
   * @brief
   *
   */
  explicit PCA9685() noexcept = default;

  /**
   * @brief
   *
   * @param
   * @param t_address
   */
  explicit PCA9685(const std::string& t_device, const uint8_t t_address);

  /**
   * @brief Destroy the PCA9685 object
   *
   */
  ~PCA9685();

  void Initialize(const std::string& t_device, const uint8_t t_address);

  void SetPWMFrequency(double t_freq);

  void SetAllPWM(const uint16_t t_on, const uint16_t t_off);

  void SetSinglePWM(const int t_channel, const uint16_t t_on, const uint16_t t_off);

  void SetPWMms(const int channel, const double ms);

 private:
  std::unique_ptr<i2cPeripheral> m_i2c_device{nullptr};
  double m_frequency{50};
};

}  // namespace adafruit

#endif  // PCA9685_DRIVER_H_
