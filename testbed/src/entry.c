#include "game.h"

#include <entry.h>

#include <core/kmemory.h>

#include "stdio.h"
#include <Algorithm/algorithm.h>

// Define the function to create a game
b8 create_game(game* out_game) {

	// algorithm test
	// const i32 N = 10;
	// u32 n;
	// i32 q[N];

	// scanf_s("%d", &n);
	// for (u32 i = 0; i < n; i++) 
	// 	scanf_s("%d", &q[i]);

	// _quick_sort(q, 0, n - 1);

	// for (u32 i = 0; i < n; i++)	
	// 	KDEBUG("%d", q[i]);

	// Application configuration.
	out_game->app_config.start_pos_x = 100;
	out_game->app_config.start_pos_y = 100;
	out_game->app_config.start_width = 1280;
	out_game->app_config.start_height = 720;
	out_game->app_config.name = "Engine Testbed";
	
	out_game->initialize = game_initialize;
	out_game->update = game_update;
	out_game->render = game_render;
	out_game->on_resize = game_on_resize;

	// create the game state
	out_game->state = kallocate(sizeof(game_state), MEMORY_TAG_GAME);

	return TRUE;
}