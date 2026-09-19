#include "motor/motor_control.h"

#include <Arduino.h>
#include <SimpleFOC.h>
#include <Wire.h>
#include <math.h>

#include "board_config.h"
#include "motor_config.h"

namespace {

MagneticSensorI2C encoder(AS5600_I2C);
BLDCDriver3PWM driver(board_config::DRIVER_IN1_PIN,
                      board_config::DRIVER_IN2_PIN,
                      board_config::DRIVER_IN3_PIN,
                      board_config::DRIVER_ENABLE_PIN);
BLDCMotor motor(motor_config::MOTOR_POLE_PAIRS);

bool driver_initialized = false;
bool motor_initialized = false;
bool foc_aligned = false;
bool running = false;
float target_velocity_rad_s = 0.0F;
MotorFault fault = MotorFault::NONE;

float clampVelocity(float velocity_rad_s) {
    if (velocity_rad_s > motor_config::VELOCITY_LIMIT_RAD_S) {
        return motor_config::VELOCITY_LIMIT_RAD_S;
    }
    if (velocity_rad_s < -motor_config::VELOCITY_LIMIT_RAD_S) {
        return -motor_config::VELOCITY_LIMIT_RAD_S;
    }
    return velocity_rad_s;
}

void disableOutputs() {
    target_velocity_rad_s = 0.0F;
    running = false;

    if (motor_initialized) {
        // Send a zero demand before electrically releasing the phases.
        motor.move(0.0F);
        motor.disable();
    } else if (driver_initialized) {
        driver.disable();
    }
}

void setFault(MotorFault new_fault) {
    fault = new_fault;
    disableOutputs();
}

bool probeAs5600() {
    Wire.beginTransmission(board_config::AS5600_I2C_ADDRESS);
    return Wire.endTransmission() == 0U;
}

bool readEncoderOnce(float& angle_rad) {
    encoder.update();
    angle_rad = encoder.getAngle();
    return encoder.currWireError == 0U && isfinite(angle_rad);
}

const char* directionText(Direction direction) {
    switch (direction) {
        case Direction::CW:
            return "CW";
        case Direction::CCW:
            return "CCW";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

bool motorInit() {
    fault = MotorFault::NONE;
    foc_aligned = false;
    running = false;
    target_velocity_rad_s = 0.0F;

    Wire.setSDA(board_config::ENCODER_SDA_PIN);
    Wire.setSCL(board_config::ENCODER_SCL_PIN);
    Wire.begin();
    Wire.setClock(board_config::I2C_CLOCK_HZ);

    if (!probeAs5600()) {
        Serial.println("AS5600 I2C probe: FAILED (address 0x36)");
        setFault(MotorFault::ENCODER_NOT_FOUND);
        return false;
    }
    Serial.println("AS5600 I2C probe: OK");

    encoder.init(&Wire);
    // MagneticSensorI2C::init() calls Wire.begin() internally. Re-apply the
    // chosen bus rate afterwards so it cannot be reset to the core default.
    Wire.setClock(board_config::I2C_CLOCK_HZ);
    float initial_angle_rad = 0.0F;
    if (!readEncoderOnce(initial_angle_rad)) {
        Serial.println("AS5600 initial read: FAILED");
        setFault(MotorFault::ENCODER_READ_FAILED);
        return false;
    }
    Serial.print("AS5600 initial angle [rad]: ");
    Serial.println(initial_angle_rad, 4);

    driver.voltage_power_supply = motor_config::POWER_SUPPLY_VOLTAGE_V;
    driver.voltage_limit = motor_config::DRIVER_VOLTAGE_LIMIT_V;
    if (driver.init() == 0) {
        Serial.println("3PWM driver init: FAILED");
        setFault(MotorFault::DRIVER_INIT_FAILED);
        return false;
    }
    driver_initialized = true;
    driver.disable();
    Serial.println("3PWM driver init: OK");

    motor.linkSensor(&encoder);
    motor.linkDriver(&driver);
    motor.torque_controller = TorqueControlType::voltage;
    motor.controller = MotionControlType::velocity;
    motor.voltage_limit = motor_config::MOTOR_VOLTAGE_LIMIT_V;
    motor.velocity_limit = motor_config::VELOCITY_LIMIT_RAD_S;
    motor.voltage_sensor_align = motor_config::SENSOR_ALIGN_VOLTAGE_V;
    motor.LPF_velocity.Tf =
        motor_config::VELOCITY_FILTER_TIME_CONSTANT_S;

    if (motor.init() == 0) {
        Serial.println("SimpleFOC motor init: FAILED");
        setFault(MotorFault::MOTOR_INIT_FAILED);
        return false;
    }
    motor_initialized = true;

    // BLDCMotor::init() enables the driver internally. Disable it immediately;
    // alignment and any rotor movement are allowed only after a button press.
    motor.disable();
    Serial.println("SimpleFOC motor init: OK");
    Serial.println("Motor outputs: DISABLED");
    return true;
}

bool motorStart() {
    if (!motor_initialized || fault != MotorFault::NONE) {
        return false;
    }

    motor.enable();

    if (!foc_aligned) {
        if (motor.initFOC() == 0) {
            setFault(MotorFault::FOC_ALIGNMENT_FAILED);
            return false;
        }

        if (encoder.currWireError != 0U) {
            setFault(MotorFault::ENCODER_READ_FAILED);
            return false;
        }

        if (!motor.pp_check_result) {
            setFault(MotorFault::POLE_PAIR_CHECK_FAILED);
            return false;
        }

        foc_aligned = true;
        Serial.print("Sensor direction: ");
        Serial.println(directionText(motor.sensor_direction));
        Serial.print("Electrical zero [rad]: ");
        Serial.println(motor.zero_electric_angle, 4);
        Serial.println("Pole-pair movement check: OK");
    }

    motorSetVelocity(motor_config::TEST_VELOCITY_RAD_S);
    running = true;
    return true;
}

void motorStop() {
    disableOutputs();
}

void motorSetVelocity(float velocity_rad_s) {
    target_velocity_rad_s = clampVelocity(velocity_rad_s);
}

float motorGetVelocity() {
    return motor.shaft_velocity;
}

float motorGetAngle() {
    return motor.shaft_angle;
}

float motorGetTargetVelocity() {
    return target_velocity_rad_s;
}

bool motorIsRunning() {
    return running;
}

bool motorIsAligned() {
    return foc_aligned;
}

bool motorHasFault() {
    return fault != MotorFault::NONE;
}

MotorFault motorGetFault() {
    return fault;
}

const char* motorGetFaultText() {
    switch (fault) {
        case MotorFault::NONE:
            return "none";
        case MotorFault::ENCODER_NOT_FOUND:
            return "AS5600 not found at I2C address 0x36";
        case MotorFault::ENCODER_READ_FAILED:
            return "AS5600 read failed";
        case MotorFault::DRIVER_INIT_FAILED:
            return "3PWM driver initialization failed";
        case MotorFault::MOTOR_INIT_FAILED:
            return "SimpleFOC motor initialization failed";
        case MotorFault::FOC_ALIGNMENT_FAILED:
            return "FOC sensor alignment failed";
        case MotorFault::POLE_PAIR_CHECK_FAILED:
            return "pole-pair movement check failed";
        default:
            return "unknown motor fault";
    }
}

void motorControlLoop() {
    if (!running || fault != MotorFault::NONE) {
        return;
    }

    motor.loopFOC();
    if (encoder.currWireError != 0U) {
        setFault(MotorFault::ENCODER_READ_FAILED);
        return;
    }

    motor.move(target_velocity_rad_s);
}
