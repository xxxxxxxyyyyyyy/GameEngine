#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"

// TODO: temporary
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

b8 initialize_logging(){
    //TODO: create log file
    return TRUE;
}

void shutdown_logging(){
    //TODO: cleanup logging/write queued entries
}

void log_output(log_level level, const char* message, ...){
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < LOG_LEVEL_WARN;

    // Technically imposes a 32k char limit on a single log entry, but...
    // Dont do that!
    const i32 msg_length = 32000;
    char output_message[msg_length];
    // dynamic array is slow, avoid dynamic allocation, this is the way for now.
    memset(output_message, 0, sizeof(output_message));

    // Format orignial message.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(output_message, msg_length, message, arg_ptr);
    va_end(arg_ptr);

    char output_message2[msg_length];
    sprintf(output_message2, "%s%s\n", level_strings[level], output_message);

    // platform-specific output
    if (is_error) {
        platform_console_write_error(output_message2, level);
    } else {
        platform_console_write(output_message2, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line){
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
