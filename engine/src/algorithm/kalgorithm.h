#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "core/kmemory.h"

API void swap(void* a, void* b, u32 size);

API void _quick_sort(i32 q[], i32 l, i32 r);

API void _merge_sort(i32 q[], i32 tmp[], i32 l, i32 r);