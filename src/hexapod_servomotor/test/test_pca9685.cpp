/**
 * @file test_pca9685.cpp
 * @brief Unit tests for the PCA9685 driver that need no board on the bus.
 *
 * Two things are covered: the frequency arithmetic taken from the data sheet,
 * and the argument validation, which runs before any transfer is attempted.
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include "PCA9685_driver.h"
#include "PCA9685_register.h"

namespace pca = adafruit::pca9685;

// --- frequency arithmetic ---------------------------------------------------

TEST(Prescale, MatchesTheDataSheetExample) {
  // "for an output default frequency of 200 Hz with an oscillator clock
  //  frequency of 25 MHz: prescale value = 30 (0x1Eh)"
  EXPECT_EQ(pca::PrescaleFromFrequency(200.0), 30);
}

TEST(Prescale, ServoRefreshRate) { EXPECT_EQ(pca::PrescaleFromFrequency(50.0), 121); }

TEST(Prescale, ReachesTheHardwareLimits) {
  EXPECT_NEAR(pca::FrequencyFromPrescale(pca::kPrescaleMin), pca::kFrequencyMaxHz, 1.0);
  EXPECT_NEAR(pca::FrequencyFromPrescale(pca::kPrescaleMax), pca::kFrequencyMinHz, 1.0);
}

TEST(Prescale, RoundTripIsLossy) {
  // The prescaler is an integer, so a request does not come back unchanged.
  // Pulse widths must therefore be computed from the frequency the board really
  // produces, not from the one that was asked for.
  const auto prescale = pca::PrescaleFromFrequency(50.0);
  const auto actual = pca::FrequencyFromPrescale(prescale);
  EXPECT_GT(actual, 50.0);
  EXPECT_LT(actual, 50.1);
}

// --- validation, with no device attached ------------------------------------

TEST(Channel, AcceptsEveryChannelOfTheDevice) {
  EXPECT_NO_THROW(adafruit::PCA9685::EnsureValidChannel(0));
  EXPECT_NO_THROW(adafruit::PCA9685::EnsureValidChannel(pca::kChannelCount - 1));
}

TEST(Channel, RejectsIndexesOutsideTheDevice) {
  // Without this check the register arithmetic 0x06 + 4 * channel would write
  // into the registers of another channel, or into PRE_SCALE.
  EXPECT_THROW(adafruit::PCA9685::EnsureValidChannel(-1), std::invalid_argument);
  EXPECT_THROW(adafruit::PCA9685::EnsureValidChannel(pca::kChannelCount), std::invalid_argument);
}

TEST(Counter, AcceptsTheWholeTwelveBitRange) {
  EXPECT_NO_THROW(adafruit::PCA9685::EnsureValidCounter(0, "ON"));
  EXPECT_NO_THROW(adafruit::PCA9685::EnsureValidCounter(pca::kCounterMax, "OFF"));
}

TEST(Counter, RejectsTheFullOnBit) {
  // 4096 is not a duty cycle, it is the full-ON or full-OFF flag.
  EXPECT_THROW(adafruit::PCA9685::EnsureValidCounter(pca::kFullOnOffBit, "ON"), std::invalid_argument);
}

TEST(Driver, StartsUninitialised) {
  const adafruit::PCA9685 driver;
  EXPECT_FALSE(driver.IsInitialised());
}

TEST(Driver, RefusesToWorkBeforeInitialize) {
  adafruit::PCA9685 driver;
  EXPECT_THROW(driver.SetPWMFrequency(50.0), std::logic_error);
  EXPECT_THROW(driver.SetAllPWM(0, 0), std::logic_error);
  EXPECT_THROW(driver.SetSinglePWM(0, 0, 0), std::logic_error);
  EXPECT_THROW(driver.AllOutputsOff(), std::logic_error);
}

TEST(Driver, RejectsAddressesTheDeviceCannotHave) {
  // The address is checked before the bus is opened, so this needs no hardware
  // and never touches /dev.
  adafruit::PCA9685 driver;
  EXPECT_THROW(driver.Initialize("/dev/i2c-1", pca::kAddressMin - 1), std::invalid_argument);
  EXPECT_THROW(driver.Initialize("/dev/i2c-1", 0x00), std::invalid_argument);
  EXPECT_FALSE(driver.IsInitialised());
}
