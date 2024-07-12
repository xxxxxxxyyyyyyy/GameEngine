#pragma once

#include "core/application.h"
#include "memory/linear_allocator.h"

struct render_packet;

typedef struct game_frame_data {
    // A darray of world geometries to be rendered this frame.
    geometry_render_data* world_geometries;
} game_frame_data;

/**
 * Represents the basic game state in a game.
 * Called for creation by the application.
 */
typedef struct game {
    // the application configuration
    application_config app_config;

    /**
     * @brief Function pointer to the game's boot sequence. This should 
     * fill out the application config with the game's specific requirements.
     * @param game_inst A pointer to the game instance.
     * @returns True on success; otherwise false.
     */
    b8 (*boot)(struct game* game_inst);

    // Function pointer to game's initialize function
    b8 (*initialize)(struct game* game_inst);

    // Function pointer to game's update function
    b8 (*update)(struct game* game_inst, f32 delta_time);

    // Function pointer to game's render function
    b8 (*render)(struct game* game_inst, struct render_packet* packet, f32 delta_time);

    // Function pointer to handle resizes, if applicable
    void (*on_resize)(struct game* game_inst, u32 width, u32 height);

    /**
     * @brief Shuts down the game, prompting release of resources.
     * @param game_inst A pointer to the game instance.
     */
    void (*shutdown)(struct game* game_inst);

    /** @brief The required size for the game state. */
    u64 state_memory_requirement;

    // Game-specific game state. Created and managed by the game.
    void* state;

    // Application state.
    void* application_state;

    /** 
     * @brief An allocator used for allocations needing to be made every frame. Contents are wiped
     * at the beginning of the frame.
     */
    linear_allocator frame_allocator;

    /** @brief Data which is built up, used and discarded every frame. */
    game_frame_data frame_data;
} game;