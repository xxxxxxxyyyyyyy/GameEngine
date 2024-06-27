#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

b8 vulkan_renderer_backend_initialize(struct renderer_backend* backend, const char* application_name);

void vulkan_renderer_backend_shutdown(struct renderer_backend* backend);

void vulkan_renderer_backend_on_resized(struct renderer_backend* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(struct renderer_backend* backend, f32 delta_time);
void vulkan_renderer_update_global_state(matrix4 projection, matrix4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
b8 vulkan_renderer_backend_end_frame(struct renderer_backend* backend, f32 delta_time);

void vulkan_backend_update_object(matrix4 model);

void vulkan_renderer_create_texture(const char* name, b8 auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels, b8 has_transparency, texture* out_texture);
void vulkan_renderer_destroy_texture(texture* texture);