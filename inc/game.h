#ifndef __GAME_H__
#define __GAME_H__

#include <SDL.h>

#include "anim.h"
#include "puzzle.h"

#define MAX_MOVES           4096

struct game_state {
	struct puzzle puzzle, reset;
	struct solution solution;
	struct anim_queue anim_queue;
	u32 num_moves;
	enum player_move moves[MAX_MOVES];
	SDL_Renderer *renderer;
	SDL_Texture  *sprite_tex;
	SDL_Texture  *target_tex;
	SDL_Texture  *font_tex;
	u32 animating;
	u32 last_tick, anim_tick;
	u32 menu_item_idx;
	struct explosion_queue explosion_queue;
	enum {
		GAME_STATE_ALIVE,
		GAME_STATE_OVER,
		GAME_STATE_VICTORY,
		GAME_STATE_SHOW_SOLUTION,
		GAME_STATE_PAUSED,
	} state, prev_state;
};

void init_game_state(struct game_state *game_state);
void run_game(struct game_state *game_state);

#endif
