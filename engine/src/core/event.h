#pragma once

#include "defines.h"

// oberserver design pattern

typedef struct event_context {
    // 128 bytes
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
} event_context;

// should return true if handled
typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener_inst, event_context data);

void event_system_initialize(u64* memory_requirement, void* state);
void event_system_shutdown(void* state);

/**
 * Register to listen for when events are sent with the provided code. Events with duplicate
 * listener/callback combos will not be registered again and will cause this to return false;
 * @param code The event code to listen for
 * @param listener A pointer to a listener instance. Can be 0/NULL
 * @param on_event The callback function pointer to be invoked when the event code is fired
 * @returns true if the event is successfully registered. otherwise false
 */
API b8 event_register(u16 code, void* listener, PFN_on_event on_event);

/**
 * Unregister from listening for when events are sent with the provied code. if no matching
 * registration is found, this function returns false.
 * @param code The event code to stop listening for
 * @param listener A pointer to a listener instance. Can be 0/NULL
 * @param on_event The callback function pointer to be unregister
 * @returns true if the event is successfully unregistered. otherwise false
 */
API b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);

/**
 * Fires an event to listeners of the given code. If an event handler returns
 * true, the event is considered handled and is not passed on to any more listeners.
 * @param code The event code to fire
 * @param sender A pointer to the sender. Can be 0/NULL
 * @param data The event data
 * @returns true if handled. otherwise false
 */
API b8 event_execute(u16 code, void* sender, event_context context);

// System internal event codes. Application should use codes beyond 255.
typedef enum system_event_code {
    // shuts the application down on the next frame.
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed
    // context usage:
    // u16 key_code = data.data.u16[0];
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Keyboard key released
    // context usage:
    // u16 key_code = data.data.u16[0];
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Mouse button pressed.
    // context usage:
    // u16 button = data.data.u16[0];
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // Mouse button released.
    // context usage:
    // u16 button = data.data.u16[0];
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // Mouse moved.
    // context usage:
    // u16 x = data.data.u16[0];
    // u16 y = data.data.u16[1];
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // Mouse wheel.
    // context usage:
    // u16 z_delta = data.data.u8[0];
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Resized/resolution changed from the OS.
    // context usage:
    // u16 width = data.data.u16[0];
    // u16 height = data.data.u16[1];
    EVENT_CODE_RESIZED = 0x08,
    // Change the render mode for debugging purposes.
    /* Context usage:
     * i32 mode = context.data.i32[0];
     */
    EVENT_CODE_SET_RENDER_MODE = 0x09,
    EVENT_CODE_DEBUG0 = 0x0A,
    EVENT_CODE_DEBUG1 = 0x0B,
    EVENT_CODE_DEBUG2 = 0x0C,
    EVENT_CODE_DEBUG3 = 0x0D,
    EVENT_CODE_DEBUG4 = 0x0E,
    /** @brief The hovered-over object id, if there is one.
     * Context usage:
     * i32 id = context.data.u32[0]; - will be INVALID ID if nothing is hovered over.
     */
    EVENT_CODE_OBJECT_HOVER_ID_CHANGED = 0xF,

    /** 
     * @brief An event fired by the renderer backend to indicate when any render targets
     * associated with the default window resources need to be refreshed (i.e. a window resize)
     */
    EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED = 0x10,
    MAX_EVENT_CODE = 0xFF
} system_event_code;