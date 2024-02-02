#pragma once

#include "renderer_types.inl"

b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name);

void renderer_system_shutdown(void* state);

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(render_packet* packet);

// HACK: this should not be exposed outside the engine.
KAPI void renderer_set_view(matrix4 view);