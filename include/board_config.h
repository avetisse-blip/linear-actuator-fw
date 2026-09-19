#pragma once

#include <Arduino.h>

namespace board_config {

// SimpleFOC and the STM32duino GPIO API expect Arduino digital-pin numbers.
// The no-underscore macros below (PA8, PB5, ...) provide those numbers for
// this Nucleo variant. Do not replace them with PinName values such as PA_8.
inline constexpr uint32_t DRIVER_IN1_PIN = PA8;   // D7, TIM1_CH1
inline constexpr uint32_t DRIVER_IN2_PIN = PA9;   // D8, TIM1_CH2
inline constexpr uint32_t DRIVER_IN3_PIN = PA10;  // D2, TIM1_CH3
inline constexpr uint32_t DRIVER_ENABLE_PIN = PB5;  // D4

inline constexpr uint32_t ENCODER_SCL_PIN = PB8;  // D15, I2C1_SCL
inline constexpr uint32_t ENCODER_SDA_PIN = PB9;  // D14, I2C1_SDA
inline constexpr uint8_t AS5600_I2C_ADDRESS = 0x36U;
inline constexpr uint32_t I2C_CLOCK_HZ = 400000U;

inline constexpr uint32_t USER_BUTTON_PIN = PC13;

// On NUCLEO-L476RG the PC13 input is LOW while B1 is pressed. Keeping the
// polarity in this one constant makes it easy to change for another board.
inline constexpr int USER_BUTTON_PRESSED_LEVEL = LOW;

}  // namespace board_config
