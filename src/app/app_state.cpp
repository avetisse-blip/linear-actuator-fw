#include "app/app_state.h"

#include <Arduino.h>
#include <math.h>

#include "motor/motor_control.h"
#include "motor_config.h"

namespace {

AppState state = AppState::BOOT;
float velocity_command_rad_s = motor_config::TEST_VELOCITY_RAD_S;

const char* stateName(AppState value) {
    switch (value) {
        case AppState::BOOT:
            return "BOOT";
        case AppState::WAIT_START:
            return "WAIT_START";
        case AppState::FOC_ALIGN:
            return "FOC_ALIGN";
        case AppState::RUN:
            return "RUN";
        case AppState::STOPPED:
            return "STOPPED";
        case AppState::FAULT:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

void transitionTo(AppState next_state) {
    state = next_state;
    Serial.print("STATE ");
    Serial.println(stateName(state));
}

void enterFault() {
    motorStop();
    transitionTo(AppState::FAULT);
    Serial.print("FAULT: ");
    Serial.println(motorGetFaultText());
}

void startMotor() {
    switch (state) {
        case AppState::WAIT_START:
            transitionTo(AppState::FOC_ALIGN);
            Serial.println("FOC alignment started");
            if (motorStart()) {
                motorSetVelocity(velocity_command_rad_s);
                Serial.println("FOC alignment result: OK");
                transitionTo(AppState::RUN);
            } else {
                Serial.println("FOC alignment result: FAILED");
                enterFault();
            }
            break;

        case AppState::STOPPED:
            if (motorStart()) {
                motorSetVelocity(velocity_command_rad_s);
                transitionTo(AppState::RUN);
            } else {
                enterFault();
            }
            break;

        case AppState::RUN:
            Serial.println("OK motor already running");
            break;

        case AppState::BOOT:
        case AppState::FOC_ALIGN:
            Serial.println("ERR motor is not ready to start");
            break;

        case AppState::FAULT:
            Serial.println("ERR motor is in FAULT; reset is required");
            break;
    }
}

void stopMotor() {
    if (state == AppState::RUN) {
        motorStop();
        transitionTo(AppState::STOPPED);
        Serial.println("Motor outputs: DISABLED");
        return;
    }

    Serial.println("OK motor already stopped");
}

}  // namespace

void appInit() {
    transitionTo(AppState::BOOT);

    if (!motorInit()) {
        enterFault();
        return;
    }

    transitionTo(AppState::WAIT_START);
    Serial.println("Press USER B1 to align and start.");
}

void appUpdate(bool start_stop_button_pressed) {
    if (state != AppState::FAULT && motorHasFault()) {
        enterFault();
        return;
    }

    if (!start_stop_button_pressed) {
        return;
    }

    if (state == AppState::RUN) {
        stopMotor();
    } else {
        startMotor();
    }
}

void appRequestStart() {
    startMotor();
}

void appRequestStop() {
    stopMotor();
}

bool appSetVelocityCommand(float velocity_rad_s) {
    if (!isfinite(velocity_rad_s) ||
        velocity_rad_s < -motor_config::VELOCITY_LIMIT_RAD_S ||
        velocity_rad_s > motor_config::VELOCITY_LIMIT_RAD_S) {
        return false;
    }

    velocity_command_rad_s = velocity_rad_s;
    if (state == AppState::RUN) {
        motorSetVelocity(velocity_command_rad_s);
    }
    return true;
}

AppState appGetState() {
    return state;
}

const char* appGetStateName() {
    return stateName(state);
}

float appGetVelocityCommand() {
    return velocity_command_rad_s;
}
