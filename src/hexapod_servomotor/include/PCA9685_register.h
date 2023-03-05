/**
 * @file PCA9685_register.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef PCA9685_REGISTER_H_
#define PCA9685_REGISTER_H_

#include <cstdint>

// clang-format off
#ifdef __cplusplus
extern "C" {
#endif

/* Constant definition */
#define PCA9685_MAX_SERVOS      16
#define PCA9685_CLOCK_FREQ      25000000.0  /* 25MHz default oscillator clock */
#define PCA9685_PRE_SCALE       0xFE        /* pre-scaler for output frequency */
#define PCA9685_BUFFER_SIZE     0x08        /* 1 byte buffer */

/* Mode definition */
#define PCA9685_MODE1           0x00        /* Mode  register  1 */
#define PCA9685_MODE2           0x01        /* Mode  register  2 */
#define PCA9685_SUBADR1         0x02
#define PCA9685_SUBADR2         0x03
#define PCA9685_SUBADR3         0x04

/*  Bits */
#define PCA9685_RESTART         0x80
#define PCA9685_SLEEP           0x10
#define PCA9685_ALLCALL         0x01
#define PCA9685_INVRT           0x10
#define PCA9685_OUTDRV          0x04

#ifdef __cplusplus
}
#endif
// clang-format on

constexpr uint8_t LED0_ON_L = 0x06;
constexpr uint8_t LED0_ON_H = 0x07;
constexpr uint8_t LED0_OFF_L = 0x08;
constexpr uint8_t LED0_OFF_H = 0x09;
constexpr uint8_t ALL_LED_ON_L = 0xFA;
constexpr uint8_t ALL_LED_ON_H = 0xFB;
constexpr uint8_t ALL_LED_OFF_L = 0xFC;
constexpr uint8_t ALL_LED_OFF_H = 0xFD;

#endif  // PCA9685_REGISTER_H_
