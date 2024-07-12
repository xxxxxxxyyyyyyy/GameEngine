#pragma once

#include "renderer_types.inl"

struct win32_handle_info;

// b8 renderer_backend_create(renderer_backend_type type, renderer_plugin* out_renderer_backend);
void renderer_backend_destroy(renderer_plugin* renderer_backend);