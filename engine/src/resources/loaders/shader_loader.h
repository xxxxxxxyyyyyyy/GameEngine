/**
 * @file shader_loader.h
 * @author DOD
 * @brief A resource loader that handles shader config resources.
 * @version 1.0
 * 
 * @copyright DOD Game Engine is Copyright (c) DOD 2024
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a shader resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader shader_resource_loader_create(void);