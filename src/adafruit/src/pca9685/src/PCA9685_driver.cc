#include "servo-driver/PCA9685_driver.h"

#include <chrono>
#include <cmath>
#include <thread>
#include <iostream>

#include "servo-driver/PCA9685_register.h"

namespace adafruit {

constexpr std::chrono::microseconds delay{5000};

PCA9685::PCA9685(const std::string& t_device, const uint8_t t_address) { Initialize(t_device, t_address); }

PCA9685::~PCA9685() = default;

void PCA9685::Initialize(const std::string& t_device, const uint8_t t_address) {
  m_i2c_device = std::make_unique<i2cPeripheral>(t_device, t_address);

  //SetAllPWM(0, 0);
  m_i2c_device->WriteRegisterByte(PCA9685_MODE2, PCA9685_OUTDRV);
  m_i2c_device->WriteRegisterByte(PCA9685_MODE1, PCA9685_ALLCALL);
  std::this_thread::sleep_for(delay);
  auto mode1_val = m_i2c_device->ReadRegisterByte(PCA9685_MODE1);
  mode1_val &= ~PCA9685_SLEEP;
  m_i2c_device->WriteRegisterByte(PCA9685_MODE1, mode1_val);
  std::this_thread::sleep_for(delay);
  std::cerr << "PCA9685::Initialize() done" << std::endl;
}

void PCA9685::SetPWMFrequency(double t_freq) {
  if (t_freq != m_frequency) {
    m_frequency = t_freq;
  }

  auto prescaleval = PCA9685_CLOCK_FREQ;  //  # 25MHz
  prescaleval /= 4096.0;                  //  # 12-bit
  prescaleval /= m_frequency;
  prescaleval -= 1.0;

  auto prescale = static_cast<int>(std::round(prescaleval));

  const auto oldmode = m_i2c_device->ReadRegisterByte(PCA9685_MODE1);

  auto newmode = (oldmode & 0x7F) | PCA9685_SLEEP;

  m_i2c_device->WriteRegisterByte(PCA9685_MODE1, newmode);
  m_i2c_device->WriteRegisterByte(PCA9685_PRE_SCALE, prescale);
  m_i2c_device->WriteRegisterByte(PCA9685_MODE1, oldmode);
  std::this_thread::sleep_for(delay);
  m_i2c_device->WriteRegisterByte(PCA9685_MODE1, oldmode | PCA9685_RESTART);
  std::cerr << "PCA9685::SetPWMFrequency() Done" << std::endl;
}

void PCA9685::SetAllPWM(const uint16_t t_on, const uint16_t t_off) {
  m_i2c_device->WriteRegisterByte(ALL_LED_ON_L, t_on & 0xFF);
  m_i2c_device->WriteRegisterByte(ALL_LED_ON_H, t_on >> 8);
  m_i2c_device->WriteRegisterByte(ALL_LED_OFF_L, t_off & 0xFF);
  m_i2c_device->WriteRegisterByte(ALL_LED_OFF_H, t_off >> 8);
}

void PCA9685::SetSinglePWM(const uint16_t t_channel, const uint16_t t_on, const uint16_t t_off) {
  const auto channel_offset = 4 * t_channel;
  m_i2c_device->WriteRegisterByte(LED0_ON_L + channel_offset, t_on & 0xFF);
  m_i2c_device->WriteRegisterByte(LED0_ON_H + channel_offset, t_on >> 8);
  m_i2c_device->WriteRegisterByte(LED0_OFF_L + channel_offset, t_off & 0xFF);
  m_i2c_device->WriteRegisterByte(LED0_OFF_H + channel_offset, t_off >> 8);
}

void PCA9685::SetPWMms(const int t_channel, const double t_ms) {
  auto period_ms = 1000.0 / m_frequency;
  auto bits_per_ms = 4096 / period_ms;
  auto bits = t_ms * bits_per_ms;
  SetSinglePWM(t_channel, 0, bits);
}

}  // namespace adafruit
