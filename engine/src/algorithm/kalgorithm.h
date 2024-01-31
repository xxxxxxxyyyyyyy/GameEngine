#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "core/kmemory.h"

KAPI void swap(void* a, void* b, u32 size);

KAPI void _quick_sort(i32 q[], i32 l, i32 r);

KAPI void _merge_sort(i32 q[], i32 tmp[], i32 l, i32 r);