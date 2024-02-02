#pragma once

#include "defines.h"
#include "math/math_types.h"

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

// need to be 256 bytes in size
typedef struct global_uniform_object {
    matrix4 projection;   // 64 bytes
    matrix4 view;         // 64 bytes
    matrix4 m_reserved0;  // 64 bytes, reserved for future use
    matrix4 m_reserved1;  // 64 bytes, reserved for future use
} global_uniform_object;

typedef struct renderer_backend {
    u64 frame_number;

    b8 (*initialize)(struct renderer_backend* backend, const char* application_name);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);
    void (*update_global_state)(matrix4 projection, matrix4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*update_object)(matrix4 model);
} renderer_backend;

typedef struct render_packet {
    f32 delta_time;
} render_packet;