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

API void memory_system_initialize(u64* memory_requirement, void* state);

API void memory_system_shutdown(void* state);

API void* kallocate(u64 size, memory_tag tag);

API void kfree(void* block, u64 size, memory_tag tag);

API void* kzero_memory(void* block, u64 size);

API void* kcopy_memory(void* dest, const void* source, u64 size);

API void* kset_memory(void* dest, i32 value, u64 size);

API char* get_memory_usage_str();

API u64 get_memory_alloc_count();