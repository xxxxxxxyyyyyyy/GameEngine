#pragma once
#include <defines.h>

struct application;
struct render_packet;

API u64 application_state_size();

API b8 application_boot(struct application* game_inst);

API b8 application_initialize(struct application* game_inst);

API b8 application_update(struct application* game_inst, f32 delta_time);

API b8 application_render(struct application* game_inst, struct render_packet* packet, f32 delta_time);

API void application_on_resize(struct application* game_inst, u32 width, u32 height);

API void application_shutdown(struct application* game_inst);