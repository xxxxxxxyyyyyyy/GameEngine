#pragma once

#include "defines.h"
#include "systems/font_system.h"
#include "renderer/renderer_types.inl"

struct application;

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

    /** @brief A darray of render view configurations. */
    render_view_config* render_views;
    
    renderer_plugin renderer_plugin;
} application_config;

API b8 engine_create(struct application* game_inst);

API b8 engine_run(struct application* game_inst);

void engine_on_event_system_initialized();