#pragma once

#include "defines.h"

struct audio_plugin;

// Plugin entry point.
API b8 plugin_create(struct audio_plugin* out_plugin);