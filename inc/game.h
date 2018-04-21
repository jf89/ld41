#ifndef __GAME_H__
#define __GAME_H__

#include <SDL.h>

#include "anim.h"
#include "puzzle.h"

struct game_state {
	struct puzzle puzzle;
	struct anim_queue anim_queue;
	SDL_Renderer *renderer;
	SDL_Texture  *sprite_tex;
	u32 animating;
	u32 last_tick, anim_tick;
};

void init_game_state(struct game_state *game_state);
void run_game(struct game_state *game_state);

#endif
