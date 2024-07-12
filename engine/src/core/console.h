#pragma once

#include "defines.h"
#include "logger.h"

typedef b8 (*PFN_console_consumer_write)(void* inst, log_level level, const char* message);

typedef struct console_command_argument {
    const char* value;
} console_command_argument;

typedef struct console_command_context {
    u8 argument_count;
    console_command_argument* arguments;
} console_command_context;

typedef void (*PFN_console_command)(console_command_context context);

b8 console_initialize(u64* memory_requirement, void* memory, void* config);
void console_shutdown(void* state);

API void console_register_consumer(void* inst, PFN_console_consumer_write callback, u8* out_consumer_id);

/**
 * @brief Updates the instance and callback for the consumer with the given identifier.
 *
 * @param consumer_id The identifier of the consumer to update.
 * @param inst The consumer instance.
 * @param callback The new callback function.
 */
API void console_update_consumer(u8 consumer_id, void* inst, PFN_console_consumer_write callback);

void console_write_line(log_level level, const char* message);

API b8 console_register_command(const char* command, u8 arg_count, PFN_console_command func);

/**
 * @brief Unregisters the given command.
 *
 * @param command The name of the command to be unregistered.
 * @return True on success; otherwise false.
 */
API b8 console_unregister_command(const char* command);

API b8 console_execute_command(const char* command);