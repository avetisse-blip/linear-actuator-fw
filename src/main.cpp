#include <Arduino.h>

#include "app/app_state.h"
#include "app_config.h"
#include "board_config.h"
#include "motor/motor_control.h"
#include "motor_config.h"
#include "serial/serial_commands.h"

namespace {

struct DebouncedButton {
    bool raw_pressed = false;
    bool stable_pressed = false;
    uint32_t raw_changed_at_ms = 0U;
};

DebouncedButton user_button;
uint32_t last_telemetry_ms = 0U;
float last_telemetry_angle_rad = 0.0F;
bool telemetry_reference_valid = false;

bool isStartStopButtonPressed() {
    return digitalRead(board_config::USER_BUTTON_PIN) ==
           board_config::USER_BUTTON_PRESSED_LEVEL;
}

bool updateButton() {
    const uint32_t now_ms = millis();
    const bool raw_pressed = isStartStopButtonPressed();

    if (raw_pressed != user_button.raw_pressed) {
        user_button.raw_pressed = raw_pressed;
        user_button.raw_changed_at_ms = now_ms;
    }

    if ((now_ms - user_button.raw_changed_at_ms) >=
            app_config::BUTTON_DEBOUNCE_MS &&
        user_button.stable_pressed != user_button.raw_pressed) {
        user_button.stable_pressed = user_button.raw_pressed;
        Serial.print("USER B1: ");
        Serial.println(user_button.stable_pressed ? "PRESSED" : "RELEASED");
        return user_button.stable_pressed;  // one event on the pressed edge
    }

    return false;
}

void printBootBanner() {
    Serial.println();
    Serial.println("========================================");
    Serial.print("Firmware: ");
    Serial.print(app_config::FIRMWARE_NAME);
    Serial.print(" v");
    Serial.println(app_config::FIRMWARE_VERSION);
    Serial.println("Board: NUCLEO-L476RG");
    Serial.print("Motor pole pairs: ");
    Serial.print(motor_config::MOTOR_POLE_PAIRS);
    Serial.println(" (confirmed from seller marking 12N14P)");
    Serial.print("Voltage limit [V]: ");
    Serial.println(motor_config::MOTOR_VOLTAGE_LIMIT_V, 2);
    Serial.print("Test velocity [rad/s]: ");
    Serial.println(motor_config::TEST_VELOCITY_RAD_S, 2);
    Serial.print("Velocity filter Tf [s]: ");
    Serial.println(motor_config::VELOCITY_FILTER_TIME_CONSTANT_S, 3);
    Serial.println("No automatic start after reset.");
    Serial.println("Commands: HELP, STATUS, START, STOP, SPEED <rad/s>");
    Serial.println("========================================");
}

void printHelp() {
    Serial.println("Commands:");
    Serial.println("  HELP              - show this list");
    Serial.println("  STATUS            - print one status line");
    Serial.println("  START             - align if needed, then run");
    Serial.println("  STOP              - disable motor outputs");
    Serial.println("  SPEED <rad/s>     - set speed in range -5.0..5.0");
}

void printStatus() {
    Serial.print("STATUS state=");
    Serial.print(appGetStateName());
    Serial.print(" aligned=");
    Serial.print(motorIsAligned() ? "yes" : "no");
    Serial.print(" angle_rad=");
    Serial.print(motorGetAngle(), 4);
    Serial.print(" velocity_rad_s=");
    Serial.print(motorGetVelocity(), 3);
    Serial.print(" command_rad_s=");
    Serial.print(appGetVelocityCommand(), 3);
    Serial.print(" driver_fault=");
    Serial.println(driverHasFault() ? "yes" : "no");
}

void handleSerialCommand(const SerialCommand& command) {
    switch (command.type) {
        case SerialCommandType::HELP:
            printHelp();
            break;

        case SerialCommandType::STATUS:
            printStatus();
            break;

        case SerialCommandType::START:
            Serial.println("CMD START");
            appRequestStart();
            break;

        case SerialCommandType::STOP:
            Serial.println("CMD STOP");
            appRequestStop();
            break;

        case SerialCommandType::SPEED:
            if (appSetVelocityCommand(command.value)) {
                Serial.print("OK command_rad_s=");
                Serial.println(appGetVelocityCommand(), 3);
            } else {
                Serial.println("ERR speed must be in range -5.0..5.0 rad/s");
            }
            break;
    }
}

void updateTelemetry() {
    if (appGetState() != AppState::RUN) {
        telemetry_reference_valid = false;
        return;
    }

    const uint32_t now_ms = millis();
    const uint32_t elapsed_ms = now_ms - last_telemetry_ms;
    if (elapsed_ms < app_config::TELEMETRY_PERIOD_MS) {
        return;
    }
    last_telemetry_ms = now_ms;

    const float angle_rad = motorGetAngle();
    float average_velocity_rad_s = 0.0F;
    if (telemetry_reference_valid) {
        average_velocity_rad_s =
            (angle_rad - last_telemetry_angle_rad) *
            (1000.0F / static_cast<float>(elapsed_ms));
    }
    last_telemetry_angle_rad = angle_rad;
    telemetry_reference_valid = true;

    Serial.print("TEL angle_rad=");
    Serial.print(angle_rad, 4);
    Serial.print(" velocity_rad_s=");
    Serial.print(motorGetVelocity(), 3);
    Serial.print(" velocity_avg_rad_s=");
    Serial.print(average_velocity_rad_s, 3);
    Serial.print(" target_rad_s=");
    Serial.print(motorGetTargetVelocity(), 3);
    Serial.print(" state=");
    Serial.println(appGetStateName());
}

}  // namespace

void setup() {
    Serial.begin(app_config::SERIAL_BAUD);

    const uint32_t serial_wait_started_ms = millis();
    while (!Serial &&
           (millis() - serial_wait_started_ms) <
               app_config::SERIAL_STARTUP_WAIT_MS) {
        // Bounded boot-only wait so the first diagnostics are visible.
    }

    pinMode(board_config::USER_BUTTON_PIN, INPUT);
    user_button.raw_pressed = isStartStopButtonPressed();
    user_button.stable_pressed = user_button.raw_pressed;
    user_button.raw_changed_at_ms = millis();

    printBootBanner();
    Serial.print("USER B1 initial state: ");
    Serial.println(user_button.stable_pressed ? "PRESSED" : "RELEASED");
    appInit();
}

void loop() {
    const bool button_pressed_event = updateButton();

    if (motorIsRunning()) {
        motorControlLoop();
    }

    appUpdate(button_pressed_event);

    SerialCommand command;
    if (serialCommandPoll(command)) {
        handleSerialCommand(command);
    }

    updateTelemetry();
}
