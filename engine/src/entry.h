#pragma once

#include "core/engine.h"
#include "core/logger.h"
#include "application_types.h"

// Externally-defined function to create a game.
// extern means define outside this dll/lib or something.
extern b8 create_application(application* out_app);

extern b8 initialize_application(application* app);

// The main entry point of the application.
int main(void) {
    // Request the game instance from the application.
    application app_inst = {0};
    if (!create_application(&app_inst)) {
        DFATAL("Could not create game!");
        return -1;
    }

    // Ensure the function pointers exist.
    if (!app_inst.render || !app_inst.update || !app_inst.initialize || !app_inst.on_resize) {
        DFATAL("The game's function pointers must be assigned");
        return -2;
    }

    // Initialization
    if (!engine_create(&app_inst)) {
        DINFO("Engine failed to create!");
        return 1;
    }

    if (!initialize_application(&app_inst)) {
        DFATAL("Could not initialize application!");
        return -1;
    }

    // Begin the game loop
    if (!engine_run(&app_inst)) {
        DINFO("Application did not shutdown gracefully");
        return 2;
    }

    return 0;
}