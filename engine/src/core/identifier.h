#pragma once

#include "defines.h"

/**
 * A Globally/Universally Unique identifier in 64-bit unsigned integer format.
 * To be used primarily as an identifier for resources. (De)serialization friendly.
 */
typedef struct identifier {
    // The actual internal identifier.
    u64 uniqueid;
} identifier;

/**
 * @brief Generates a new unique identifier.
 */
API identifier identifier_create(void);

/**
 * @brief Creates an identifier from a known value. Useful for deserialization.
 */
API identifier identifier_from_u64(u64 uniqueid);