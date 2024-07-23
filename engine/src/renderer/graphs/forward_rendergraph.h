#pragma once

#include "renderer/rendergraph.h"

struct frame_data;
struct simple_scene;
struct viewport;

typedef struct forward_rendergraph {
    rendergraph internal_graph;

    u16 shadowmap_resolution;

    rendergraph_pass skybox_pass;
    rendergraph_pass shadowmap_pass;
    rendergraph_pass scene_pass;

} forward_rendergraph;

typedef struct forward_rendergraph_config {
    u16 shadowmap_resolution;
} forward_rendergraph_config;

API b8 forward_rendergraph_create(const forward_rendergraph_config* config, forward_rendergraph* out_graph);
API void forward_rendergraph_destroy(forward_rendergraph* graph);

API b8 forward_rendergraph_initialize(forward_rendergraph* graph);
API b8 forward_rendergraph_update(forward_rendergraph* graph, struct frame_data* p_frame_data);
API b8 forward_rendergraph_frame_prepare(forward_rendergraph* graph, struct frame_data* p_frame_data, struct camera* current_camera, struct viewport* current_viewport, struct simple_scene* scene, u32 render_mode);
API b8 forward_rendergraph_execute(forward_rendergraph* graph, struct frame_data* p_frame_data);
API b8 forward_rendergraph_on_resize(forward_rendergraph* graph, u32 width, u32 height);