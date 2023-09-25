#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state {
	game* game_inst;
	b8 is_running;
	b8 is_suspended;
	platform_state platform;
	i16 width;
	i16 height;
	clock clock;
	f64 last_time;
} application_state;

// safety check, the application initialize times
static b8 initialized = FALSE; 
// Static
static application_state app_state;

// event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);

b8 application_create(game* game_inst) {
	if (initialized) {
		KERROR("application_create called more than once");
		return FALSE;
	}

	app_state.game_inst = game_inst;

	// Initialize Subsystems.
	initialize_logging();
	input_initialize();

	// TODO: Remove this.
	KFATAL("a test message: %f", 3.14f);
	KERROR("a test message: %f", 3.14f);
	KWARN("a test message: %f", 3.14f);
	KINFO("a test message: %f", 3.14f);
	KDEBUG("a test message: %f", 3.14f);
	KTRACE("a test message: %f", 3.14f);

	app_state.is_running = TRUE;
	app_state.is_suspended = FALSE;

	if (!event_initialize()) {
		KERROR("Event system failed initialization!");
		return FALSE;
	}

	event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
	event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

	if (!platform_startup(&app_state.platform, game_inst->app_config.name,
	game_inst->app_config.start_pos_x, game_inst->app_config.start_pos_y,
	game_inst->app_config.start_width, game_inst->app_config.start_height)) {
		return FALSE;
	}

	// Renderer startup
	if (!renderer_initialize(game_inst->app_config.name, &app_state.platform)) {
		KFATAL("Failed to initialize renderer. Aborting application");
		return FALSE;
	}

	// Initialize the game.
	if (!app_state.game_inst->initialize(app_state.game_inst)) {
		KFATAL("Game failed to initialize");
		return FALSE;
	}

	app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);

	initialized = TRUE;

	return TRUE;
}

b8 application_run() {
	clock_start(&app_state.clock);
	clock_update(&app_state.clock);
	app_state.last_time = app_state.clock.elapsed;
	f64 running_time = 0;
	u8 frame_count = 0;
	f64 target_frame_seconds = 1.0f / 30;

	KINFO(get_memory_usage_str());

	while (app_state.is_running)
	{
		if (!platform_pump_messages(&app_state.platform)) {
			app_state.is_running = FALSE;
		}

		if (!app_state.is_suspended) {
			// update clock and get delta time
			clock_update(&app_state.clock);
			f64 current_time = app_state.clock.elapsed;
			f64 delta = (current_time - app_state.last_time);
			f64 frame_start_time = platform_get_absolute_time();

			if (!app_state.game_inst->update(app_state.game_inst, (f32)delta)) {
				KFATAL("Game update failed, shutting down");
				app_state.is_running = FALSE;
				break;
			}

			// Call the game's render routine
			if (!app_state.game_inst->render(app_state.game_inst, (f32)delta)) {
				KFATAL("Game render failed, shutting down");
				app_state.is_running = FALSE;
				break;
			}

			// TODO: refactor packet creation
			render_packet packet;
			packet.delta_time = delta;
			renderer_draw_frame(&packet);

			// figure out how long the frame took
			f64 frame_end_time = platform_get_absolute_time();
			f64 frame_elapsed_time = frame_end_time - frame_start_time;
			running_time += frame_elapsed_time;
			f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

			if (remaining_seconds > 0) {
				u64 remaining_ms = (remaining_seconds * 1000);

				// if there is time left, give it back to the os
				b8 limit_frames = TRUE;
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
			app_state.last_time = current_time;
		}
	}

	app_state.is_running = FALSE;

	// shutdown event system
	event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
	event_unregister(EVENT_CODE_RESIZED, 0, application_on_resized);
	event_shutdown();
	input_shutdown();

	renderer_shutdown();

	platform_shutdown(&app_state.platform);

	return TRUE;
}

void application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state.width;
    *height = app_state.height;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
	switch (code) {
		case EVENT_CODE_APPLICATION_QUIT : {
			KINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down. \n");
			app_state.is_running = FALSE;
			return TRUE;
		}
	}

	return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
	if (code == EVENT_CODE_KEY_PRESSED) {
		u16 key_code = context.data.u16[0];
		if (key_code == KEY_ESCAPE) {
			event_context data = {};
			event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

			// blocking anything else from processing this.
			return TRUE;
		} else if (key_code == KEY_A) {
			KDEBUG("Explicit - A key pressed!");
		} else {
			KDEBUG("'%c' key pressed in window", key_code);
		}
	} else if (code == EVENT_CODE_KEY_RELEASED) {
		u16 key_code = context.data.u16[0];
		if (key_code == KEY_B) {
			KDEBUG("Explicit - B key released!");
		} else {
			KDEBUG("'%c' key released in window", key_code);
		}
	}
	return FALSE;
}

b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != app_state.width || height != app_state.height) {
            app_state.width = width;
            app_state.height = height;

            KDEBUG("Window resize: %i, %i", width, height);

            // Handle minimization
            if (width == 0 || height == 0) {
                KINFO("Window minimized, suspending application.");
                app_state.is_suspended = TRUE;
                return TRUE;
            } else {
                if (app_state.is_suspended) {
                    KINFO("Window restored, resuming application.");
                    app_state.is_suspended = FALSE;
                }
                app_state.game_inst->on_resize(app_state.game_inst, width, height);
		renderer_on_resize(width, height);
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return FALSE;
}