#pragma once

#include "resource_types.h"

// API b8 mesh_load_from_resource(const char* resource_name, mesh* out_mesh);

API b8 mesh_create(mesh_config config, mesh* out_mesh);

API b8 mesh_initialize(mesh* m);

API b8 mesh_load(mesh* m);

API b8 mesh_unload(mesh* m);

API b8 mesh_destroy(mesh* m);