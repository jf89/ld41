#include "menu.h"

#include <stdio.h>
#include <SDL.h>

#include "state.h"
#include "generator.h"
#include "menu_widget.h"
#include "puzzle.h"
#include "draw.h"

#define MENU_ITEM_GENERATE_1 0
#define MENU_ITEM_GENERATE_2 1
#define MENU_ITEM_GENERATE_3 2
#define MENU_ITEM_EXIT       3

#define BG_PUZZLE_W 20
#define BG_PUZZLE_H 10
#define BG_PUZZLE_E 32

#define TW 64
#define TH 64

#define TEX_W (BG_PUZZLE_W * TW)
#define TEX_H (BG_PUZZLE_H * TH)

#define SW (TW * 20)
#define SH (TH * 10)

#define ANIM_LEN 1000

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

void init_menu_state(struct menu_state *menu_state) {
	menu_state->last_frame_time = SDL_GetTicks();
	menu_state->anim_ticks = 0;
	menu_state->puzzle     = generate_puzzle(BG_PUZZLE_W, BG_PUZZLE_H, BG_PUZZLE_E);
	menu_state->target_tex = SDL_CreateTexture(menu_state->renderer, SDL_PIXELFORMAT_RGBA32,
                                                   SDL_TEXTUREACCESS_TARGET,
                                                   TW * BG_PUZZLE_W, TH * BG_PUZZLE_H);
}

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

	u32 this_tick = SDL_GetTicks();
	u32 frame_time = this_tick - menu_state->last_frame_time;
	if (frame_time > menu_state->anim_ticks) {
		menu_state->anim_ticks = ANIM_LEN;
		menu_state->anim_queue.len = 0;
		step_puzzle(&menu_state->puzzle, PLAYER_MOVE_PAUSE, &menu_state->anim_queue);
	} else {
		menu_state->anim_ticks -= frame_time;
	}
	menu_state->last_frame_time = this_tick;

	f32 anim_fac = 1.0f - ((f32)menu_state->anim_ticks / (f32)ANIM_LEN);

	SDL_SetRenderTarget(renderer, menu_state->target_tex);
	SDL_RenderClear(renderer);
	draw_puzzle(renderer, menu_state->sprite_tex, &menu_state->puzzle, &menu_state->anim_queue,
	            &menu_state->explosion_queue, 1, anim_fac, frame_time);

	SDL_SetRenderTarget(renderer, NULL);
	SDL_Rect src = { 0, 0, TEX_W, TEX_H };
	SDL_Rect dst = { (SW - TEX_W) / 2, (SH - TEX_H) / 2, TEX_W, TEX_H };
	SDL_SetTextureColorMod(menu_state->target_tex, 96, 64, 96);
	SDL_RenderCopy(renderer, menu_state->target_tex, &src, &dst);

	SDL_Texture *font_tex = menu_state->font_tex;

	draw_widget(&menu_widget, renderer, font_tex);

	SDL_RenderPresent(renderer);
	return;

quit:
	state = STATE_QUIT;
}
