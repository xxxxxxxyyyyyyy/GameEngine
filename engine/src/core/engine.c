#include "engine.h"

#include "application_types.h"
#include "containers/darray.h"
#include "core/clock.h"
#include "core/event.h"
#include "core/frame_data.h"
#include "core/input.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "core/logger.h"
#include "core/metrics.h"
#include "core/uuid.h"
#include "memory/linear_allocator.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"

#include "containers/darray.h"

#include "memory/linear_allocator.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"

// systems
#include "core/systems_manager.h"

typedef struct engine_state_t {
    application* game_inst;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;
    systems_manager_state sys_manager_state;
    // An allocator used for per-frame allocations, that is reset every frame.
    linear_allocator frame_allocator;

    frame_data p_frame_data;
} engine_state_t;

// safety check, the application initialize times
static b8 initialized = false;
// Static
static engine_state_t* engine_state;

// event handlers
static b8 engine_on_event(u16 code, void* sender, void* listener_inst, event_context context);
static b8 engine_on_resized(u16 code, void* sender, void* listener_inst, event_context context);

b8 engine_create(application* game_inst) {
    if (game_inst->engine_state) {
        DERROR("engine_create called more than once");
        return false;
    }

    // Memory system must be the first thing to be stood up.
    memory_system_configuration memory_system_config = {};
    memory_system_config.total_alloc_size = GIBIBYTES(2);
    if (!memory_system_initialize(memory_system_config)) {
        DERROR("Failed to initialize memory system; shutting down.");
        return false;
    }

    // Seed the uuid generator.
    // TODO: A better seed here.
    uuid_seed(101);

    // Metrics
    metrics_initialize();

    // Stand up the application state.
    game_inst->engine_state = kallocate(sizeof(engine_state_t), MEMORY_TAG_ENGINE);
    engine_state = game_inst->engine_state;
    engine_state->game_inst = game_inst;
    engine_state->is_running = false;
    engine_state->is_suspended = false;
    game_inst->app_config.renderer_plugin = game_inst->render_plugin;

    if (!systems_manager_initialize(&engine_state->sys_manager_state, &game_inst->app_config)) {
        DFATAL("Systems manager failed to initialize. Aborting process.");
        return false;
    }

    // Perform the game's boot sequence.
    game_inst->stage = APPLICATION_STAGE_BOOTING;
    if (!game_inst->boot(game_inst)) {
        DFATAL("Game boot sequence failed; aborting application.");
        return false;
    }

    // Setup the frame allocator.
    linear_allocator_create(game_inst->app_config.frame_allocator_size, 0, &engine_state->frame_allocator);
    engine_state->p_frame_data.frame_allocator = &engine_state->frame_allocator;
    if (game_inst->app_config.app_frame_data_size > 0) {
        engine_state->p_frame_data.application_frame_data = kallocate(game_inst->app_config.app_frame_data_size, MEMORY_TAG_GAME);
    } else {
        engine_state->p_frame_data.application_frame_data = 0;
    }

    game_inst->stage = APPLICATION_STAGE_BOOT_COMPLETE;

    // Report engine version
    DINFO("DOD Engine v. %f", DVERSION);

    if (!systems_manager_post_boot_initialize(&engine_state->sys_manager_state, &game_inst->app_config)) {
    DFATAL("Post-boot system manager initialization failed!");
    return false;
    }

    // Initialize the game.
    game_inst->stage = APPLICATION_STAGE_INITIALIZING;
    if (!engine_state->game_inst->initialize(engine_state->game_inst)) {
        DFATAL("Game failed to initialize");
        return false;
    }
    game_inst->stage = APPLICATION_STAGE_INITIALIZED;

    // Call resize once to ensure the proper size has been set.
    renderer_on_resize(engine_state->width, engine_state->height);
    engine_state->game_inst->on_resize(engine_state->game_inst, engine_state->width, engine_state->height);

    initialized = true;

    return true;
}

b8 engine_run(application* game_inst) {
    game_inst->stage = APPLICATION_STAGE_RUNNING;

    engine_state->is_running = true;
    clock_start(&engine_state->clock);
    clock_update(&engine_state->clock);
    engine_state->last_time = engine_state->clock.elapsed;
    f64 running_time = 0;
    // u8 frame_count = 0;
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

            engine_state->p_frame_data.total_time = current_time;
            engine_state->p_frame_data.delta_time = (f32)delta;

            // Reset the frame allocator
            linear_allocator_free_all(&engine_state->frame_allocator);

            // Update systems.
            systems_manager_update(&engine_state->sys_manager_state, &engine_state->p_frame_data);

            // update metrics
            metrics_update(frame_elapsed_time);

            if (!engine_state->game_inst->update(engine_state->game_inst, &engine_state->p_frame_data)) {
                DFATAL("Game update failed, shutting down");
                engine_state->is_running = false;
                break;
            }

            // this frames render packet
            render_packet packet = {};

            // Have the application generate the render packet.
            b8 prepare_result = engine_state->game_inst->prepare_render_packet(engine_state->game_inst, &packet, &engine_state->p_frame_data);
            if (!prepare_result) {
                DERROR("Application failed to prepare the render packet. Skipping this frame.");
                continue;
            }

            // Call the game's render routine.
            if (!engine_state->game_inst->render(engine_state->game_inst, &packet, &engine_state->p_frame_data)) {
                DFATAL("Game render failed, shutting down.");
                engine_state->is_running = false;
                break;
            }

            // Clean-up
            for (u32 i = 0; i < packet.view_count; ++i) {
                packet.views[i].view->on_packet_destroy(packet.views[i].view, &packet.views[i]);
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

                // frame_count++;
            }

            // NOTE: input update/state copying should always be handled
            // after any input should be recorded;
            // as a safety, input is the last thing to be updated before
            // this frame ends. update input used to next frame
            input_update(&engine_state->p_frame_data);

            // update last time
            engine_state->last_time = current_time;
        }
    }

    engine_state->is_running = false;
    game_inst->stage = APPLICATION_STAGE_SHUTTING_DOWN;

    // shutdown event system
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, engine_on_event);

    // Shut down all systems.
    systems_manager_shutdown(&engine_state->sys_manager_state);

    game_inst->stage = APPLICATION_STAGE_UNINITIALIZED;

    return true;
}

// void application_get_framebuffer_size(u32* width, u32* height) {
//     *width = engine_state->width;
//     *height = engine_state->height;
// }

static void engine_on_event_system_initialized(void) {
    // Register for engine-level events.
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, engine_on_event);
    event_register(EVENT_CODE_RESIZED, 0, engine_on_resized);
}

const struct frame_data* engine_frame_data_get(struct application* game_inst) {
    return &((engine_state_t*)game_inst->engine_state)->p_frame_data;
}

static b8 engine_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            DINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down. \n");
            engine_state->is_running = false;
            return true;
        }
    }

    return false;
}

b8 engine_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
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