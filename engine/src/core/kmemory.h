#pragma once

#include "defines.h"

typedef enum memory_tag {
    // For temporary use. Should be assigned one of the below or have a new created
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_ALGORITHM,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,

    MEMORY_TAG_MAX_TAGS
} memory_tag;

/** @brief The configuration for the memory system. */
typedef struct memory_system_configuration {
    /** @brief The total memory size in bytes used by the internal allocator for this system. */
    u64 total_alloc_size;
} memory_system_configuration;

API b8 memory_system_initialize(memory_system_configuration config);

API void memory_system_shutdown();

API void* kallocate(u64 size, memory_tag tag);

API void kfree(void* block, u64 size, memory_tag tag);

API void* kzero_memory(void* block, u64 size);

API void* kcopy_memory(void* dest, const void* source, u64 size);

API void* kset_memory(void* dest, i32 value, u64 size);

API char* get_memory_usage_str();

API u64 get_memory_alloc_count();