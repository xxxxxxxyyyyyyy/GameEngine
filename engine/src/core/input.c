#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

// internal input state
static b8 is_initialized = false;
static input_state state = {};

void input_initialize() {
    kzero_memory(&state, sizeof(input_state));
    is_initialized = true;
    KINFO("input subsystem initialized");
}

void input_shutdown() {
    // TODO: add shutdown routines when needed
    is_initialized = false;
}

void input_update(f64 delta_time) {
    if (!is_initialized) {
        return;
    }

    // copy current states to previous one
    kcopy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    kcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, b8 pressed) {
    if (key == KEY_LALT) {
        KINFO("Left alt pressed.");
    } else if (key == KEY_RALT) {
        KINFO("Right alt pressed.");
    }

    if (key == KEY_LCONTROL) {
        KINFO("Left ctrl pressed.");
    } else if (key == KEY_RCONTROL) {
        KINFO("Right ctrl pressed.");
    }

    if (key == KEY_LSHIFT) {
        KINFO("Left shift pressed.");
    } else if (key == KEY_RSHIFT) {
        KINFO("Right shift pressed.");
    }

    // Only handle this if the state actually changed.
    if (state.keyboard_current.keys[key] != pressed) {
        // update internal state
        state.keyboard_current.keys[key] = pressed;

        // fire off an event for immediate processing
        event_context context;
        context.data.u16[0] = key;
        event_execute(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(buttons button, b8 pressed) {
    // if the state changed, fire an event
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;

        // fire event
        event_context context;
        context.data.u16[0] = button;
        event_execute(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    // only process if actually different
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        // NOTE: enable this if debugging
        // KDEBUG("Mouse pos: %i, %i.", x, y);

        // update internal state
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        // fire event
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_execute(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    // fire envet
    event_context context;
    context.data.u8[0] = z_delta;
    event_execute(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 input_is_key_down(keys key) {
    if (!is_initialized) {
        return false;
    }
    return state.keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    if (!is_initialized) {
        return true;
    }
    return state.keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    if (!is_initialized) {
        return false;
    }
    return state.keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    if (!is_initialized) {
        return true;
    }
    return state.keyboard_previous.keys[key] == false;
}

// mouse input
b8 input_is_button_down(buttons button) {
    if (!is_initialized) {
        return false;
    }
    return state.mouse_current.buttons[button] == true;
}

b8 input_is_button_up(buttons button) {
    if (!is_initialized) {
        return true;
    }
    return state.mouse_current.buttons[button] == false;
}

b8 input_was_button_down(buttons button) {
    if (!is_initialized) {
        return false;
    }
    return state.mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button) {
    if (!is_initialized) {
        return true;
    }
    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!is_initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!is_initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}