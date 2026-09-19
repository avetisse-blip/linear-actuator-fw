#pragma once

enum class AppState {
    BOOT,
    WAIT_START,
    FOC_ALIGN,
    RUN,
    STOPPED,
    FAULT,
};

void appInit();
void appUpdate(bool start_stop_button_pressed);
void appRequestStart();
void appRequestStop();
bool appSetVelocityCommand(float velocity_rad_s);

AppState appGetState();
const char* appGetStateName();
float appGetVelocityCommand();
