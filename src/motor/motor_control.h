#pragma once

enum class MotorFault {
    NONE,
    ENCODER_NOT_FOUND,
    ENCODER_READ_FAILED,
    DRIVER_FAULT,
    DRIVER_INIT_FAILED,
    MOTOR_INIT_FAILED,
    FOC_ALIGNMENT_FAILED,
    POLE_PAIR_CHECK_FAILED,
};

// Hardware-independent API used by the application layer. Arduino, Wire and
// SimpleFOC types deliberately do not appear in this header.
bool motorInit();
bool motorStart();
void motorStop();
void motorSetVelocity(float velocity_rad_s);

// Optional DRV8313 control pins on SimpleFOC Mini v1.0. These APIs never
// start the motor. Reset and sleep first disable all motor outputs.
bool driverHasFault();
bool driverResetFault();
void driverSleep();
bool driverWake();

float motorGetVelocity();
float motorGetAngle();
float motorGetTargetVelocity();

bool motorIsRunning();
bool motorIsAligned();
bool motorHasFault();
MotorFault motorGetFault();
const char* motorGetFaultText();

// Call as frequently as possible while motorIsRunning() is true.
void motorControlLoop();
