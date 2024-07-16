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
/**
 * @brief Attempts to peek an element (writing out a copy of the
 * element data on success) from the stack. If the stack is empty,
 * nothing is done and false is returned. The stack memory is not modified.
 *
 * @param s A pointer to the stack to peek from.
 * @param element_data A pointer to write the element data to. Required.
 * @return True on succcess; otherwise false.
 */
API b8 stack_peek(const stack* s, void* out_element_data);
API b8 stack_pop(stack* s, void* out_element_data);