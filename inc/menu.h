#ifndef __MENU_H__
#define __MENU_H__

#include <SDL.h>

#include "types.h"
#include "game.h"
#include "anim.h"

struct menu_state {
	SDL_Renderer *renderer;
	SDL_Texture  *font_tex;
	SDL_Texture  *sprite_tex;
	SDL_Texture  *target_tex;
	u32 menu_item;
	u32 last_frame_time, anim_ticks;
	struct game_state *game_state;
	struct puzzle puzzle;
	struct anim_queue anim_queue;
	struct explosion_queue explosion_queue;
};

void run_menu(struct menu_state *menu_state);
void init_menu_state(struct menu_state *menu_state);

#endif
