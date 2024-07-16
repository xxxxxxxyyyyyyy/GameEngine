#pragma once

#include "audio/audio_types.h"
#include "systems/resource_system.h"

typedef struct audio_resource_loader_params {
    audio_file_type type;
    u64 chunk_size;
} audio_resource_loader_params;

/**
 * @brief Creates and returns a audio resource loader.
 *
 * @return The newly created resource loader.
 */
API resource_loader audio_resource_loader_create(void);