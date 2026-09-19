#pragma once

namespace motor_config {

// Seller drawing: 12N14P means 14 rotor poles, hence 7 pole pairs.
inline constexpr int MOTOR_POLE_PAIRS = 7;

inline constexpr float POWER_SUPPLY_VOLTAGE_V = 12.0F;
inline constexpr float DRIVER_VOLTAGE_LIMIT_V = 1.0F;
inline constexpr float MOTOR_VOLTAGE_LIMIT_V = 1.0F;
inline constexpr float SENSOR_ALIGN_VOLTAGE_V = 1.0F;
inline constexpr float VELOCITY_LIMIT_RAD_S = 5.0F;
inline constexpr float TEST_VELOCITY_RAD_S = 1.0F;

// SimpleFOC low-pass filter for the velocity calculated from AS5600 angle
// samples. The default is deliberately increased for clean low-speed feedback.
inline constexpr float VELOCITY_FILTER_TIME_CONSTANT_S = 0.05F;

// Reference data from the seller drawing. These values are intentionally not
// fed into the first-run controller; they are kept here for later modelling and
// tuning after the hardware has been verified.
inline constexpr float PHASE_RESISTANCE_OHM = 2.55F;
inline constexpr float PHASE_INDUCTANCE_H = 0.00086F;
inline constexpr float MOTOR_KV_RPM_PER_V = 220.0F;

}  // namespace motor_config
