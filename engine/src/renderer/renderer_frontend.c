#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "math/kmath.h"

#include "core/logger.h"
#include "core/kmemory.h"

typedef struct renderer_system_state {
    renderer_backend backend;
} renderer_system_state;

static renderer_system_state* state_ptr;

b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name) {
    *memory_requirement = sizeof(renderer_system_state);
    if (state == 0) {
        return true;
    }
    state_ptr = state;

    // TODO: make this configurable.
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.frame_number = 0;

    if (!state_ptr->backend.initialize(&state_ptr->backend, application_name)) {
        KFATAL("Renderer backend failed to initialize. Shutting down.");
        return false;
    }

    return true;
}

void renderer_system_shutdown(void* state) {
    if (state_ptr) {
        state_ptr->backend.shutdown(&state_ptr->backend);
    }
    state_ptr = 0;
}

b8 renderer_begin_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
    state_ptr->backend.frame_number++;
    return result;
}

b8 renderer_draw_frame(render_packet* packet) {
    // if the begin frame returned successfully, mid-frame operations may continue.
    if (renderer_begin_frame(packet->delta_time)) {
        matrix4 projection = mat4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, 0.1f, 1000.0f);
        static f32 z = 0.0f;
        z += 0.01f;
        matrix4 view = mat4_translation((vec3){0, 0, z});  // -30.0f
        view = mat4_inverse(view);

        state_ptr->backend.update_global_state(projection, view, vec3_zero(), vec4_one(), 0);

        // mat4 model = mat4_translation((vec3){0, 0, 0});
        static f32 angle = 0.01f;
        angle += 0.01f;
        quaterion rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        matrix4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        state_ptr->backend.update_object(model);

        // End the frame.
        b8 result = renderer_end_frame(packet->delta_time);

        if (!result) {
            KERROR("renderer_end_frame failed. Application shutting down");
            return false;
        }
    }
    return true;
}

void renderer_on_resize(u16 width, u16 height) {
    if (state_ptr) {
        state_ptr->backend.resized(&state_ptr->backend, width, height);
    } else {
        KWARN("renderer backend does not exist to accept resize: %i %i", width, height);
    }
}
