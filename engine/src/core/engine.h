#pragma once

#include "defines.h"
#include "renderer/renderer_types.inl"
#include "systems/font_system.h"

struct application;
struct frame_data;

// Application configuration
typedef struct application_config {
    // Window starting position x axis, if applicable.
    i16 start_pos_x;

    // Window starting position y axis, if applicable.
    i16 start_pos_y;

    // Window starting width
    i16 start_width;

    // Window starting height
    i16 start_height;

    // The application name used in windowing
    char* name;

    /** @brief Configuration for the font system. */
    font_system_config font_config;

    /** @brief A darray of render views. */
    render_view* views;
    
    renderer_plugin renderer_plugin;

    /** @brief The size of the frame allocator. */
    u64 frame_allocator_size;

    /** @brief The size of the application-specific frame data. Set to 0 if not used. */
    u64 app_frame_data_size;
} application_config;

API b8 engine_create(struct application* game_inst);

API b8 engine_run(struct application* game_inst);

void engine_on_event_system_initialized(void);

API const struct frame_data* engine_frame_data_get(struct application* game_inst);