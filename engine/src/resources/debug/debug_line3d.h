#pragma once

#include "core/identifier.h"
#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

typedef struct debug_line3d {
    identifier id;
    char *name;
    vec3 point_0;
    vec3 point_1;
    vec4 colour;
    transform xform;

    u32 vertex_count;
    colour_vertex_3d *vertices;

    geometry geo;
} debug_line3d;

API b8 debug_line3d_create(vec3 point_0, vec3 point_1, transform *parent, debug_line3d *out_line);
API void debug_line3d_destroy(debug_line3d *line);

API void debug_line3d_parent_set(debug_line3d *line, transform *parent);
API void debug_line3d_colour_set(debug_line3d *line, vec4 colour);
API void debug_line3d_points_set(debug_line3d *line, vec3 point_0, vec3 point_1);

API b8 debug_line3d_initialize(debug_line3d *line);
API b8 debug_line3d_load(debug_line3d *line);
API b8 debug_line3d_unload(debug_line3d *line);

API b8 debug_line3d_update(debug_line3d *line);