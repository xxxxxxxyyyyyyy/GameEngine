#pragma once

#include "../standard_ui_system.h"

typedef struct sui_panel_internal_data {
    vec4 rect;
    vec4 colour;
    geometry* g;
    u32 instance_id;
    u64 frame_number;
    u8 draw_index;
} sui_panel_internal_data;

API b8 sui_panel_control_create(const char* name, vec2 size, vec4 colour, struct sui_control* out_control);
API void sui_panel_control_destroy(struct sui_control* self);

API b8 sui_panel_control_load(struct sui_control* self);
API void sui_panel_control_unload(struct sui_control* self);

API b8 sui_panel_control_update(struct sui_control* self, struct frame_data* p_frame_data);
API b8 sui_panel_control_render(struct sui_control* self, struct frame_data* p_frame_data, standard_ui_render_data* render_data);

API vec2 sui_panel_size(struct sui_control* self);
API b8 sui_panel_control_resize(struct sui_control* self, vec2 new_size);