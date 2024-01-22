#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <malloc.h>
#include <string.h>

KAPI void swap(void* a, void* b, u32 size);

KAPI void _quick_sort(i32 q[], i32 l, i32 r);