#pragma once

#include "math/math_types.h"
#include "renderer/renderer_types.h"

typedef struct viewport {
    /** @brief the dimensions of this viewport, x/y position, z/w are width/height.*/
    rect_2d rect;
    f32 fov;
    f32 near_clip;
    f32 far_clip;
    renderer_projection_matrix_type projection_matrix_type;
    matrix4 projection;
} viewport;

API b8 viewport_create(vec4 rect, f32 fov, f32 near_clip, f32 far_clip, renderer_projection_matrix_type projection_matrix_type, viewport* out_viewport);
API void viewport_destroy(viewport* v);

API void viewport_resize(viewport* v, vec4 rect);