#pragma once

#include "defines.h"

#include "resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

/** @brief The name of the default UI material. */
#define DEFAULT_UI_MATERIAL_NAME "default_ui"

/** @brief The name of the default terrain material. */
#define DEFAULT_TERRAIN_MATERIAL_NAME "default_terrain"

typedef struct material_system_config {
    u32 max_material_count;
} material_system_config;

b8 material_system_initialize(u64* memory_requirement, void* state, void* config);
void material_system_shutdown(void* state);

API material* material_system_acquire(const char* name);
/**
 * @brief Attempts to acquire a terrain material with the given name. If it has not yet been
 * loaded, this triggers it to be loaded from using the provided standard material names. If
 * the material is not able to be loaded, a pointer to the default terrain material is returned.
 * If the material _is_ found and loaded, its reference counter is incremented.
 *
 * @param name The name of the terrain material to find.
 * @param material_count The number of standard source material names.
 * @param material_names The names of the source materials to be used.
 * @return A pointer to the loaded terrain material. Can be a pointer to the defualt terrain material if not found.
 */
API material* material_system_acquire_terrain_material(const char* material_name, u32 material_count, const char** material_names, b8 auto_release);
API material* material_system_acquire_from_config(material_config* config);
API void material_system_release(const char* name);

/**
 * @brief Gets a pointer to the default UI material. Does not reference count.
 */
API material* material_system_get_default_ui(void);

/**
 * @brief Gets a pointer to the default terrain material. Does not reference count.
 */
API material* material_system_get_default_terrain(void);

API material* material_system_get_default(void);

/**
 * @brief Applies global-level data for the material shader id.
 * 
 * @param shader_id The identifier of the shader to apply globals for.
 * @param projection A constant pointer to a projection matrix.
 * @param view A constant pointer to a view matrix.
 * @param ambient_colour The ambient colour of the scene.
 * @param view_position The camera position.
 * @param render_mode The render mode.
 * @return True on success; otherwise false.
 */
API b8 material_system_apply_global(u32 shader_id, u64 renderer_frame_number, const matrix4* projection, const matrix4* view, const vec4* ambient_colour, const vec3* view_position, u32 render_mode);

/**
 * @brief Applies instance-level material data for the given material.
 *
 * @param m A pointer to the material to be applied.
 * @return True on success; otherwise false.
 */
API b8 material_system_apply_instance(material* m, b8 needs_update);

/**
 * @brief Applies local-level material data (typically just model matrix).
 *
 * @param m A pointer to the material to be applied.
 * @param model A constant pointer to the model matrix to be applied.
 * @return True on success; otherwise false.
 */
API b8 material_system_apply_local(material* m, const matrix4* model);

/**
 * @brief Dumps all of the registered materials and their reference counts/handles.
 */
API void material_system_dump(void);