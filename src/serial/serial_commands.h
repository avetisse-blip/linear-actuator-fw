#pragma once

enum class SerialCommandType {
    HELP,
    STATUS,
    START,
    STOP,
    SPEED,
};

struct SerialCommand {
    SerialCommandType type = SerialCommandType::HELP;
    float value = 0.0F;
};

// Reads complete newline-terminated commands without blocking. Returns true
// only when a valid command has been parsed into command.
bool serialCommandPoll(SerialCommand& command);
