#pragma once

#include <Arduino.h>

namespace app_config {

inline constexpr char FIRMWARE_NAME[] = "linear-actuator-first-bringup";
inline constexpr char FIRMWARE_VERSION[] = "0.2.0";

inline constexpr uint32_t SERIAL_BAUD = 115200U;
inline constexpr uint32_t SERIAL_STARTUP_WAIT_MS = 1000U;
inline constexpr uint32_t BUTTON_DEBOUNCE_MS = 40U;
inline constexpr uint32_t TELEMETRY_PERIOD_MS = 200U;  // 5 Hz
inline constexpr size_t SERIAL_COMMAND_BUFFER_SIZE = 64U;

}  // namespace app_config
