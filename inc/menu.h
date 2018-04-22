#ifndef __MENU_H__
#define __MENU_H__

#include <SDL.h>

#include "types.h"
#include "game.h"

struct menu_state {
	SDL_Renderer *renderer;
	SDL_Texture  *font_tex;
	u32 menu_item;
	struct game_state *game_state;
};

void run_menu(struct menu_state *menu_state);

#endif
