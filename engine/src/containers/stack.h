#pragma once

#include "defines.h"

typedef struct stack {
    u32 element_size;
    u32 element_count;
    u32 allocated;
    void* memory;
} stack;

API b8 stack_create(stack* out_stack, u32 element_size);
API void stack_destroy(stack* s);

API b8 stack_push(stack* s, void* element_data);
API b8 stack_pop(stack* s, void* out_element_data);