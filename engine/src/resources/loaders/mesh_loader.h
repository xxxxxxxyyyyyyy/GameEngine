/**
 * @file mesh_loader.h
 * @author DOD
 * @brief A resource loader that handles mesh resources.
 * @version 1.0
 * 
 * @copyright DOD Game Engine is Copyright (c) DOD 2024
 * 
 */

#pragma once

#include "systems/resource_system.h"

/**
 * @brief Creates and returns a mesh resource loader.
 * 
 * @return The newly created resource loader.
 */
resource_loader mesh_resource_loader_create();