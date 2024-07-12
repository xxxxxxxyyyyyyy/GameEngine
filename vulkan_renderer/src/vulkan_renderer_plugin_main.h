#pragma once

#include <renderer/renderer_types.inl>

/**
 * @brief Creates a new renderer plugin of the given type.
 * 
 * @param out_renderer_backend A pointer to hold the newly-created renderer plugin.
 * @return True if successful; otherwise false.
 */
API b8 plugin_create(renderer_plugin* out_plugin);

/**
 * @brief Destroys the given renderer backend.
 * 
 * @param renderer_backend A pointer to the plugin to be destroyed.
 */
API void plugin_destroy(renderer_plugin* plugin);