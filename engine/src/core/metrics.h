#pragma once

#include "defines.h"

/**
 * @brief Initializes the metrics system.
 */
API void metrics_initialize();

/**
 * @brief Updates metrics; should be called once per frame.
 *
 * @param frame_elapsed_time The amount of time elapsed on the previous frame.
 */
API void metrics_update(f64 frame_elapsed_time);

/**
 * @brief Returns the running average frames per second (fps).
 */
API f64 metrics_fps();

/**
 * @brief Returns the running average frametime in milliseconds.
 */
API f64 metrics_frame_time();

/**
 * @brief Gets both the running average frames per second (fps) and frametime in milliseconds.
 *
 * @param out_fps A pointer to hold the running average frames per second (fps).
 * @param out_frame_ms A pointer to hold the running average frametime in milliseconds.
 */
API void metrics_frame(f64* out_fps, f64* out_frame_ms);