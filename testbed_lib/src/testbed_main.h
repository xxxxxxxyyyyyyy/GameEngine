#pragma once
#include <defines.h>

struct application;
struct render_packet;
struct frame_data;

API u64 application_state_size();

API b8 application_boot(struct application* game_inst);

API b8 application_initialize(struct application* game_inst);

API b8 application_update(struct application* game_inst, const struct frame_data* p_frame_data);

API b8 application_render(struct application* game_inst, struct render_packet* packet, const struct frame_data* p_frame_data);

API void application_on_resize(struct application* game_inst, u32 width, u32 height);

API void application_shutdown(struct application* game_inst);

API void application_lib_on_unload(struct application* game_inst);

API void application_lib_on_load(struct application* game_inst);