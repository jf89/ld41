#include "menu.h"

#include <stdio.h>
#include <SDL.h>

#include "state.h"
#include "generator.h"
#include "menu_widget.h"

#define MENU_ITEM_GENERATE_1 0
#define MENU_ITEM_GENERATE_2 1
#define MENU_ITEM_GENERATE_3 2
#define MENU_ITEM_EXIT       3

static struct menu_widget menu_widget = {
	.title = "YATBBH",
	.cur_item = 0, .num_items = 4,
	.items = {
		"Generate easy level",
		"Generate medium level",
		"Generate hard level",
		"Exit",
	}
};

void run_menu(struct menu_state *menu_state) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		s32 response = menu_widget_handle_event(&menu_widget, &e);

		switch (response) {
		case MENU_WIDGET_QUIT:
			goto quit;
		case MENU_ITEM_GENERATE_1:
			generate_easy_puzzle(&menu_state->game_state->puzzle);
			init_game_state(menu_state->game_state);
			menu_state->menu_item = 0;
			state = STATE_GAME;
			return;
		case MENU_ITEM_GENERATE_2:
			generate_medium_puzzle(&menu_state->game_state->puzzle);
			init_game_state(menu_state->game_state);
			menu_state->menu_item = 0;
			state = STATE_GAME;
			return;
		case MENU_ITEM_GENERATE_3:
			generate_hard_puzzle(&menu_state->game_state->puzzle);
			init_game_state(menu_state->game_state);
			menu_state->menu_item = 0;
			state = STATE_GAME;
			return;
		case MENU_ITEM_EXIT:
			goto quit;
		}
	}

	SDL_Renderer *renderer = menu_state->renderer;
	SDL_SetRenderDrawColor(renderer, 32, 32, 48, 255);
	SDL_RenderClear(renderer);

	SDL_Texture *font_tex = menu_state->font_tex;

	draw_widget(&menu_widget, renderer, font_tex);

	SDL_RenderPresent(renderer);
	return;

quit:
	state = STATE_QUIT;
}
