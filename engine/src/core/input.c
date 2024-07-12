#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "core/keymap.h"
#include "containers/stack.h"

typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    b8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
    stack keymap_stack;
    // keymap active_keymap;
} input_state;

// Internal input state pointer
static input_state* state_ptr;
b8 check_modifiers(keymap_modifier modifiers);

void input_system_initialize(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(input_state);
    if (state == 0) {
        return;
    }
    kzero_memory(state, sizeof(input_state));
    state_ptr = state;

    // Create the keymap stack and an active keymap to apply to.
    stack_create(&state_ptr->keymap_stack, sizeof(keymap));
    // state_ptr->active_keymap = keymap_create();

    DINFO("Input subsystem initialized.");
}

void input_system_shutdown(void* state) {
    // TODO: Add shutdown routines when needed.
    state_ptr = 0;
}

void input_update(f64 delta_time) {
    if (!state_ptr) {
        return;
    }

    // Handle hold bindings.
    for (u32 k = 0; k < KEYS_MAX_KEYS; ++k) {
        keys key = (keys)k;
        if (input_is_key_down(key) && input_was_key_down(key)) {
            u32 map_count = state_ptr->keymap_stack.element_count;
            keymap* maps = (keymap*)state_ptr->keymap_stack.memory;
            for (i32 m = map_count - 1; m >= 0; --m) {
                keymap* map = &maps[m];
                keymap_binding* binding = &map->entries[key].bindings[0];
                b8 unset = false;
                while (binding) {
                    // If an unset is detected, stop processing.
                    if (binding->type == KEYMAP_BIND_TYPE_UNSET) {
                        unset = true;
                        break;
                    } else if (binding->type == KEYMAP_BIND_TYPE_HOLD) {
                        if (binding->callback && check_modifiers(binding->modifiers)) {
                            binding->callback(key, binding->type, binding->modifiers, binding->user_data);
                        }
                    }

                    binding = binding->next;
                }
                // If an unset is detected or the map is marked to override all, stop processing.
                if (unset || map->overrides_all) {
                    break;
                }
            }
        }
    }

    // copy current states to previous one
    kcopy_memory(&state_ptr->keyboard_previous, &state_ptr->keyboard_current, sizeof(keyboard_state));
    kcopy_memory(&state_ptr->mouse_previous, &state_ptr->mouse_current, sizeof(mouse_state));
}

b8 check_modifiers(keymap_modifier modifiers) {
    if (modifiers & KEYMAP_MODIFIER_SHIFT_BIT) {
        if (!input_is_key_down(KEY_SHIFT) && !input_is_key_down(KEY_LSHIFT) && !input_is_key_down(KEY_RSHIFT)) {
            return false;
        }
    }
    if (modifiers & KEYMAP_MODIFIER_CONTROL_BIT) {
        if (!input_is_key_down(KEY_CONTROL) && !input_is_key_down(KEY_LCONTROL) && !input_is_key_down(KEY_RCONTROL)) {
            return false;
        }
    }
    if (modifiers & KEYMAP_MODIFIER_ALT_BIT) {
        if (!input_is_key_down(KEY_LALT) && !input_is_key_down(KEY_RALT)) {
            return false;
        }
    }

    return true;
}

void input_process_key(keys key, b8 pressed) {
    // keymap_entry* map_entry = &state_ptr->active_keymap.entries[key];
    // Only handle this if the state actually changed.
    if (state_ptr && state_ptr->keyboard_current.keys[key] != pressed) {
        // update internal state
        state_ptr->keyboard_current.keys[key] = pressed;

        if (key == KEY_LALT) {
            DINFO("Left alt %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RALT) {
            DINFO("Right alt %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LCONTROL) {
            DINFO("Left ctrl %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RCONTROL) {
            DINFO("Right ctrl %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LSHIFT) {
            DINFO("Left shift %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RSHIFT) {
            DINFO("Right shift %s.", pressed ? "pressed" : "released");
        }

        // Check for key bindings
        // Iterate keymaps top-down on the stack.
        u32 map_count = state_ptr->keymap_stack.element_count;
        keymap* maps = (keymap*)state_ptr->keymap_stack.memory;
        for (i32 m = map_count - 1; m >= 0; --m) {
            keymap* map = &maps[m];
            keymap_binding* binding = &map->entries[key].bindings[0];
            b8 unset = false;
            while (binding) {
                // If an unset is detected, stop processing.
                if (binding->type == KEYMAP_BIND_TYPE_UNSET) {
                    unset = true;
                    break;
                } else if (pressed && binding->type == KEYMAP_BIND_TYPE_PRESS) {
                    if (binding->callback && check_modifiers(binding->modifiers)) {
                        binding->callback(key, binding->type, binding->modifiers, binding->user_data);
                    }
                } else if (!pressed && binding->type == KEYMAP_BIND_TYPE_RELEASE) {
                    if (binding->callback && check_modifiers(binding->modifiers)) {
                        binding->callback(key, binding->type, binding->modifiers, binding->user_data);
                    }
                }

                binding = binding->next;
            }
            // If an unset is detected or the map is marked to override all, stop processing.
            if (unset || map->overrides_all) {
                break;
            }
        }

        // fire off an event for immediate processing
        event_context context;
        context.data.u16[0] = key;
        event_execute(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(buttons button, b8 pressed) {
    // if the state changed, fire an event
    if (state_ptr->mouse_current.buttons[button] != pressed) {
        state_ptr->mouse_current.buttons[button] = pressed;

        // fire event
        event_context context;
        context.data.u16[0] = button;
        event_execute(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    // only process if actually different
    if (state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y) {
        // NOTE: enable this if debugging
        // DDEBUG("Mouse pos: %i, %i.", x, y);

        // update internal state
        state_ptr->mouse_current.x = x;
        state_ptr->mouse_current.y = y;

        // fire event
        event_context context;
        context.data.i16[0] = x;
        context.data.i16[1] = y;
        event_execute(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_keymap_push(const keymap* map) {
    if (state_ptr && map) {
        // Add the keymap to the stack, then apply it.
        if (!stack_push(&state_ptr->keymap_stack, (void*)map)) {
            DERROR("Failed to push keymap!");
            return;
        }
    }
}

void input_keymap_pop() {
    if (state_ptr) {
        // Pop the keymap from the stack, then re-apply the stack.
        keymap popped;
        if (!stack_pop(&state_ptr->keymap_stack, &popped)) {
            DERROR("Failed to pop keymap!");
            return;
        }
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    // fire envet
    event_context context;
    context.data.i8[0] = z_delta;
    event_execute(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 input_is_key_down(keys key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->keyboard_previous.keys[key] == false;
}

// mouse input
b8 input_is_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_current.buttons[button] == true;
}

b8 input_is_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_current.buttons[button] == false;
}

b8 input_was_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}