#include "serial/serial_commands.h"

#include <Arduino.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"

namespace {

char receive_buffer[app_config::SERIAL_COMMAND_BUFFER_SIZE] = {};
size_t receive_length = 0U;
bool receive_overflow = false;

char* trimWhitespace(char* text) {
    while (*text != '\0' && isspace(static_cast<unsigned char>(*text))) {
        ++text;
    }

    char* end = text + strlen(text);
    while (end > text && isspace(static_cast<unsigned char>(end[-1]))) {
        --end;
    }
    *end = '\0';
    return text;
}

void makeUppercase(char* text) {
    while (*text != '\0') {
        *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
        ++text;
    }
}

bool parseSpeed(char* line, SerialCommand& command) {
    if (strncmp(line, "SPEED", 5U) != 0 ||
        (line[5] != ' ' && line[5] != '\t')) {
        return false;
    }

    char* number = trimWhitespace(line + 5);
    char* number_end = nullptr;
    const float value = strtof(number, &number_end);
    if (number_end == number || !isfinite(value)) {
        return false;
    }

    number_end = trimWhitespace(number_end);
    if (*number_end != '\0') {
        return false;
    }

    command.type = SerialCommandType::SPEED;
    command.value = value;
    return true;
}

bool parseLine(char* line, SerialCommand& command) {
    line = trimWhitespace(line);
    makeUppercase(line);

    if (strcmp(line, "HELP") == 0) {
        command.type = SerialCommandType::HELP;
        return true;
    }
    if (strcmp(line, "STATUS") == 0) {
        command.type = SerialCommandType::STATUS;
        return true;
    }
    if (strcmp(line, "START") == 0) {
        command.type = SerialCommandType::START;
        return true;
    }
    if (strcmp(line, "STOP") == 0) {
        command.type = SerialCommandType::STOP;
        return true;
    }
    if (parseSpeed(line, command)) {
        return true;
    }

    Serial.println("ERR unknown command; enter HELP");
    return false;
}

}  // namespace

bool serialCommandPoll(SerialCommand& command) {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            if (receive_overflow) {
                Serial.println("ERR command is too long");
                receive_length = 0U;
                receive_overflow = false;
                return false;
            }

            receive_buffer[receive_length] = '\0';
            receive_length = 0U;
            if (receive_buffer[0] == '\0') {
                continue;
            }
            return parseLine(receive_buffer, command);
        }

        if (receive_overflow) {
            continue;
        }

        if (receive_length + 1U >= sizeof(receive_buffer)) {
            receive_overflow = true;
            continue;
        }

        receive_buffer[receive_length++] = received;
    }

    return false;
}
