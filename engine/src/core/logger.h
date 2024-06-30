#pragma once

#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// disable debug and trace logging for release builds
#if RELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,  // crash
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,  // debug build only
    LOG_LEVEL_TRACE = 5,  // debug build only
} log_level;

/**
 * @brief Initializes logging system. Call twice; once with state = 0 to get required memory size,
 * then a second time passing allocated memory to state.
 * 
 * @param memory_requirement A pointer to hold the required memory size of internal state.
 * @param state 0 if just requesting memory requirement, otherwise allocated block of memory.
 * @return b8 True on success; otherwise false.
 */
b8 logging_system_initialize(u64* memory_requirement, void* state);
void logging_system_shutdown(void* state);

API void log_output(log_level level, const char* message, ...);

// Logs a fatal level message
#define FATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef ERROR
// Logs a error level message
#define ERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif  // !ERROR

#if LOG_WARN_ENABLED == 1
// Logs a warning level message
#define WARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define WARN(message, ...)  // allows to compile but compile to nothing`
#endif

#if LOG_INFO_ENABLED == 1
// Logs a info level message
#define INFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define INFO(message, ...)  // allows to compile but compile to nothing`
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug level message
#define DEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define DEBUG(message, ...)  // allows to compile but compile to nothing`
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a debug level message
#define TRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#define TRACE(message, ...)  // allows to compile but compile to nothing`
#endif