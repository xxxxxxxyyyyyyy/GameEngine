#pragma once

#include "defines.h"

typedef struct clock {
    f64 start_time;
    f64 elapsed;
} clock;

// Updates the provided clock , should be called just before checking elapsed time.
// has no effect on no-started clocks.
API void clock_update(clock* clock);

// starts the provided clock, resets elapsed time
API void clock_start(clock* clock);

// stops the provided clock. does not rest elapsed time
API void clock_stop(clock* clock);