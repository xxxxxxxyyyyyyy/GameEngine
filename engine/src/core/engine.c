#include "engine.h"
#include "application_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"
#include "core/kstring.h"
#include "core/uuid.h"
#include "core/metrics.h"
#include "containers/darray.h"

#include "memory/linear_allocator.h"

#include "renderer/renderer_frontend.h"

// systems
#include "core/console.h"
#include "core/kvar.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "systems/render_view_system.h"
#include "systems/job_system.h"
#include "systems/font_system.h"

typedef struct engine_state_t {
    application* game_inst;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;
    linear_allocator systems_allocator;

    u64 console_memory_requirement;
    void* console_state;

    u64 kvar_memory_requirement;
    void* kvar_state;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 job_system_memory_requirement;
    void* job_system_state;

    u64 logging_system_memory_requirement;
    void* logging_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;

    u64 platform_system_memory_requirement;
    void* platform_system_state;

    u64 resource_system_memory_requirement;
    void* resource_system_state;

    u64 shader_system_memory_requirement;
    void* shader_system_state;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state;

    u64 renderer_view_system_memory_requirement;
    void* renderer_view_system_state;

    u64 texture_system_memory_requirement;
    void* texture_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    u64 camera_system_memory_requirement;
    void* camera_system_state;

    u64 font_system_memory_requirement;
    void* font_system_state;
} engine_state_t;

// safety check, the application initialize times
static b8 initialized = false;
// Static
static engine_state_t* engine_state;

// event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);

b8 engine_create(application* game_inst) {
    if (game_inst->engine_state) {
        DERROR("engine_create called more than once");
        return false;
    }

    // Memory system must be the first thing to be stood up.
    memory_system_configuration memory_system_config = {};
    memory_system_config.total_alloc_size = GIBIBYTES(1);
    if (!memory_system_initialize(memory_system_config)) {
        DERROR("Failed to initialize memory system; shutting down.");
        return false;
    }

    // Seed the uuid generator.
    // TODO: A better seed here.
    uuid_seed(101);

    // Metrics
    metrics_initialize();

    // Allocate the game state.
    game_inst->state = kallocate(game_inst->state_memory_requirement, MEMORY_TAG_GAME);

    // Stand up the application state.
    game_inst->engine_state = kallocate(sizeof(engine_state_t), MEMORY_TAG_ENGINE);
    engine_state = game_inst->engine_state;
    engine_state->game_inst = game_inst;
    engine_state->is_running = false;
    engine_state->is_suspended = false;

    // Create a linear allocator for all systems (except memory) to use.
    u64 systems_allocator_total_size = 64 * 1024 * 1024;  // 64 mb
    linear_allocator_create(systems_allocator_total_size, 0, &engine_state->systems_allocator);

    // Initialize Other Subsystems.
    // Console
    console_initialize(&engine_state->console_memory_requirement, 0);
    engine_state->console_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->console_memory_requirement);
    console_initialize(&engine_state->console_memory_requirement, engine_state->console_state);

    // KVars
    kvar_initialize(&engine_state->kvar_memory_requirement, 0);
    engine_state->kvar_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->kvar_memory_requirement);
    kvar_initialize(&engine_state->kvar_memory_requirement, engine_state->kvar_state);

    // Events
    event_system_initialize(&engine_state->event_system_memory_requirement, 0);
    engine_state->event_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->event_system_memory_requirement);
    event_system_initialize(&engine_state->event_system_memory_requirement, engine_state->event_system_state);

    // Logging
    logging_system_initialize(&engine_state->logging_system_memory_requirement, 0);
    engine_state->logging_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->logging_system_memory_requirement);
    if (!logging_system_initialize(&engine_state->logging_system_memory_requirement, engine_state->logging_system_state)) {
        DERROR("Failed to initialize logging system; shutting down.");
        return false;
    }

    // Input
    input_system_initialize(&engine_state->input_system_memory_requirement, 0);
    engine_state->input_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->input_system_memory_requirement);
    input_system_initialize(&engine_state->input_system_memory_requirement, engine_state->input_system_state);

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

    // Platform
    platform_system_startup(&engine_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    engine_state->platform_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->platform_system_memory_requirement);
    if (!platform_system_startup(
            &engine_state->platform_system_memory_requirement,
            engine_state->platform_system_state,game_inst->app_config.name,
            game_inst->app_config.start_pos_x, game_inst->app_config.start_pos_y,
            game_inst->app_config.start_width, game_inst->app_config.start_height)) {
        return false;
    }

    // Resource system.
    resource_system_config resource_sys_config;
    resource_sys_config.asset_base_path = "../assets";
    resource_sys_config.max_loader_count = 32;
    resource_system_initialize(&engine_state->resource_system_memory_requirement, 0, resource_sys_config);
    engine_state->resource_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->resource_system_memory_requirement);
    if(!resource_system_initialize(&engine_state->resource_system_memory_requirement, engine_state->resource_system_state, resource_sys_config)) {
        DFATAL("Failed to initialize resource system. Aborting application.");
        return false;
    }
    
    // Shader system
    shader_system_config shader_sys_config;
    shader_sys_config.max_shader_count = 1024;
    shader_sys_config.max_uniform_count = 128;
    shader_sys_config.max_global_textures = 31;
    shader_sys_config.max_instance_textures = 31;
    shader_system_initialize(&engine_state->shader_system_memory_requirement, 0, shader_sys_config);
    engine_state->shader_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->shader_system_memory_requirement);
    if(!shader_system_initialize(&engine_state->shader_system_memory_requirement, engine_state->shader_system_state, shader_sys_config)) {
        DFATAL("Failed to initialize shader system. Aborting application.");
        return false;
    }

    // Renderer system
    renderer_system_initialize(&engine_state->renderer_system_memory_requirement, 0, 0);
    engine_state->renderer_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->renderer_system_memory_requirement);
    if (!renderer_system_initialize(&engine_state->renderer_system_memory_requirement, engine_state->renderer_system_state, game_inst->app_config.name)) {
        DFATAL("Failed to initialize renderer. Aborting application.");
        return false;
    }

    b8 renderer_multithreaded = renderer_is_multithreaded();

    // Perform the game's boot sequence.
    if (!game_inst->boot(game_inst)) {
        DFATAL("Game boot sequence failed; aborting application.");
        return false;
    }

    // Report engine version
    DINFO("Kohi Engine v. %f", DVERSION);

    // This is really a core count. Subtract 1 to account for the main thread already being in use.
    i32 thread_count = platform_get_processor_count() - 1;
    if (thread_count < 1) {
        DFATAL("Error: Platform reported processor count (minus one for main thread) as %i. Need at least one additional thread for the job system.", thread_count);
        return false;
    } else {
        DTRACE("Available threads: %i", thread_count);
    }

    // Cap the thread count.
    const i32 max_thread_count = 15;
    if (thread_count > max_thread_count) {
        DTRACE("Available threads on the system is %i, but will be capped at %i.", thread_count, max_thread_count);
        thread_count = max_thread_count;
    }

    // Initialize the job system.
    // Requires knowledge of renderer multithread support, so should be initialized here.
    u32 job_thread_types[15];
    for (u32 i = 0; i < 15; ++i) {
        job_thread_types[i] = JOB_TYPE_GENERAL;
    }

    if (max_thread_count == 1 || !renderer_multithreaded) {
        // Everything on one job thread.
        job_thread_types[0] |= (JOB_TYPE_GPU_RESOURCE | JOB_TYPE_RESOURCE_LOAD);
    } else if (max_thread_count == 2) {
        // Split things between the 2 threads
        job_thread_types[0] |= JOB_TYPE_GPU_RESOURCE;
        job_thread_types[1] |= JOB_TYPE_RESOURCE_LOAD;
    } else {
        // Dedicate the first 2 threads to these things, pass off general tasks to other threads.
        job_thread_types[0] = JOB_TYPE_GPU_RESOURCE;
        job_thread_types[1] = JOB_TYPE_RESOURCE_LOAD;
    }

    // Job System
    job_system_initialize(&engine_state->job_system_memory_requirement, 0, 0, 0);
    engine_state->job_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->job_system_memory_requirement);
    if (!job_system_initialize(&engine_state->job_system_memory_requirement, engine_state->job_system_state, thread_count, job_thread_types)) {
        DFATAL("Failed to initialize job system. Aborting application.");
        return false;
    }

    // Texture system.
    texture_system_config texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_initialize(&engine_state->texture_system_memory_requirement, 0, texture_sys_config);
    engine_state->texture_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->texture_system_memory_requirement);
    if (!texture_system_initialize(&engine_state->texture_system_memory_requirement, engine_state->texture_system_state, texture_sys_config)) {
        DFATAL("Failed to initialize texture system. Application cannot continue.");
        return false;
    }

    // Font system.
    font_system_initialize(&engine_state->font_system_memory_requirement, 0, &game_inst->app_config.font_config);
    engine_state->font_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->font_system_memory_requirement);
    if (!font_system_initialize(&engine_state->font_system_memory_requirement, engine_state->font_system_state, &game_inst->app_config.font_config)) {
        DFATAL("Failed to initialize font system. Application cannot continue.");
        return false;
    }

    // Camera
    camera_system_config camera_sys_config;
    camera_sys_config.max_camera_count = 61;
    camera_system_initialize(&engine_state->camera_system_memory_requirement, 0, camera_sys_config);
    engine_state->camera_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->camera_system_memory_requirement);
    if (!camera_system_initialize(&engine_state->camera_system_memory_requirement, engine_state->camera_system_state, camera_sys_config)) {
        DFATAL("Failed to initialize camera system. Application cannot continue.");
        return false;
    }

    render_view_system_config render_view_sys_config = {};
    render_view_sys_config.max_view_count = 251;
    render_view_system_initialize(&engine_state->renderer_view_system_memory_requirement, 0, render_view_sys_config);
    engine_state->renderer_view_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->renderer_view_system_memory_requirement);
    if (!render_view_system_initialize(&engine_state->renderer_view_system_memory_requirement, engine_state->renderer_view_system_state, render_view_sys_config)) {
        DFATAL("Failed to initialize render view system. Aborting application.");
        return false;
    }

    // Load render views from app config.
    u32 view_count = darray_length(game_inst->app_config.render_views);
    for (u32 v = 0; v < view_count; ++v) {
        render_view_config* view = &game_inst->app_config.render_views[v];
        if (!render_view_system_create(view)) {
            DFATAL("Failed to create view '%s'. Aborting application.", view->name);
            return false;
        }
    }

    // Material system.
    material_system_config material_sys_config;
    material_sys_config.max_material_count = 4096;
    material_system_initialize(&engine_state->material_system_memory_requirement, 0, material_sys_config);
    engine_state->material_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->material_system_memory_requirement);
    if (!material_system_initialize(&engine_state->material_system_memory_requirement, engine_state->material_system_state, material_sys_config)) {
        DFATAL("Failed to initialize material system. Application cannot continue.");
        return false;
    }

    // Geometry system.
    geometry_system_config geometry_sys_config;
    geometry_sys_config.max_geometry_count = 4096;
    geometry_system_initialize(&engine_state->geometry_system_memory_requirement, 0, geometry_sys_config);
    engine_state->geometry_system_state = linear_allocator_allocate(&engine_state->systems_allocator, engine_state->geometry_system_memory_requirement);
    if (!geometry_system_initialize(&engine_state->geometry_system_memory_requirement, engine_state->geometry_system_state, geometry_sys_config)) {
        DFATAL("Failed to initialize geometry system. Application cannot continue.");
        return false;
    }

    // Initialize the game.
    if (!engine_state->game_inst->initialize(engine_state->game_inst)) {
        DFATAL("Game failed to initialize");
        return false;
    }

    // Call resize once to ensure the proper size has been set.
    renderer_on_resize(engine_state->width, engine_state->height);
    engine_state->game_inst->on_resize(engine_state->game_inst, engine_state->width, engine_state->height);

    initialized = true;

    return true;
}

b8 engine_run() {
    engine_state->is_running = true;
    clock_start(&engine_state->clock);
    clock_update(&engine_state->clock);
    engine_state->last_time = engine_state->clock.elapsed;
    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 30;
    f64 frame_elapsed_time = 0;

    DINFO(get_memory_usage_str());

    while (engine_state->is_running) {
        if (!platform_pump_messages()) {
            engine_state->is_running = false;
        }

        if (!engine_state->is_suspended) {
            // update clock and get delta time
            clock_update(&engine_state->clock);
            f64 current_time = engine_state->clock.elapsed;
            f64 delta = (current_time - engine_state->last_time);
            f64 frame_start_time = platform_get_absolute_time();

            // Update the job system.
            job_system_update();

            // update metrics
            metrics_update(frame_elapsed_time);

            if (!engine_state->game_inst->update(engine_state->game_inst, (f32)delta)) {
                DFATAL("Game update failed, shutting down");
                engine_state->is_running = false;
                break;
            }

            // TODO: refactor packet creation
            render_packet packet = {};
            packet.delta_time = delta;

            // Call the game's render routine.
            if (!engine_state->game_inst->render(engine_state->game_inst, &packet, (f32)delta)) {
                DFATAL("Game render failed, shutting down.");
                engine_state->is_running = false;
                break;
            }
            renderer_draw_frame(&packet);

            // Clean-up
            for (u32 i = 0; i < packet.view_count; ++i) {
                packet.views[i].view->on_destroy_packet(packet.views[i].view, &packet.views[i]);
            }

            // figure out how long the frame took
            f64 frame_end_time = platform_get_absolute_time();
            frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

            if (remaining_seconds > 0) {
                u64 remaining_ms = (remaining_seconds * 1000);

                // if there is time left, give it back to the os
                b8 limit_frames = true;
                if (remaining_ms > 0 && limit_frames) {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            // NOTE: input update/state copying should always be handled
            // after any input should be recorded;
            // as a safety, input is the last thing to be updated before
            // this frame ends. update input used to next frame
            input_update(delta);

            // update last time
            engine_state->last_time = current_time;
        }
    }

    engine_state->is_running = false;

    // shutdown event system
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);

    input_system_shutdown(engine_state->input_system_state);

    font_system_shutdown(engine_state->font_system_state);
    render_view_system_shutdown(engine_state->renderer_view_system_state);
    geometry_system_shutdown(engine_state->geometry_system_state);
    material_system_shutdown(engine_state->material_system_state);
    texture_system_shutdown(engine_state->texture_system_state);
    shader_system_shutdown(engine_state->shader_system_state);
    renderer_system_shutdown(engine_state->renderer_system_state);

    resource_system_shutdown(engine_state->resource_system_state);

    job_system_shutdown(engine_state->job_system_state);

    platform_system_shutdown(engine_state->platform_system_state);

    event_system_shutdown(engine_state->event_system_state);

    console_shutdown(engine_state->console_state);
    //logging_system_shutdown(app_state->logging_system_state);    

    memory_system_shutdown();

    return true;
}

void application_get_framebuffer_size(u32* width, u32* height) {
    *width = engine_state->width;
    *height = engine_state->height;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            DINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down. \n");
            engine_state->is_running = false;
            return true;
        }
    }

    return false;
}

b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != engine_state->width || height != engine_state->height) {
            engine_state->width = width;
            engine_state->height = height;

            DDEBUG("Window resize: %i, %i", width, height);

            // Handle minimization
            if (width == 0 || height == 0) {
                DINFO("Window minimized, suspending application.");
                engine_state->is_suspended = true;
                return true;
            } else {
                if (engine_state->is_suspended) {
                    DINFO("Window restored, resuming application.");
                    engine_state->is_suspended = false;
                }
                engine_state->game_inst->on_resize(engine_state->game_inst, width, height);
                renderer_on_resize(width, height);
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}