#include "game.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL.h>

#include "state.h"
#include "puzzle.h"
#include "anim.h"
#include "draw.h"
#include "menu_widget.h"

#define TW 64
#define TH 64
#define ANIM_LEN 400

#define SW (TW * 20)
#define SH (TH * 10)

#define EXPLOSION_LEN 500

#define PARTICLE_W 4
#define PARTICLE_H 4

#define TIMER_W 7
#define TIMER_H 7

#define MIN(x, y) (x < y ? x : y)

#define PAUSE_WIDGET_UNDO          0
#define PAUSE_WIDGET_RESET         1
#define PAUSE_WIDGET_SHOW_SOLUTION 2
#define PAUSE_WIDGET_CONTINUE      3
#define PAUSE_WIDGET_MAIN_MENU     4
static struct menu_widget pause_widget = {
	.title = "PAUSE",
	.cur_item = 0, .num_items = 5,
	.items = {
		"(z) undo",
		"(r) reset",
		"(s) show solution",
		"    continue",
		"    main menu",
	},
};

#define GAME_OVER_WIDGET_UNDO          0
#define GAME_OVER_WIDGET_RESET         1
#define GAME_OVER_WIDGET_SHOW_SOLUTION 2
#define GAME_OVER_WIDGET_MAIN_MENU     3
static struct menu_widget game_over_widget = {
	.title = "GAME OVER",
	.cur_item = 0, .num_items = 4,
	.items = {
		"(z) undo",
		"(r) reset",
		"(s) show solution",
		"    main menu",
	},
};

#define VICTORY_WIDGET_RESET         0
#define VICTORY_WIDGET_MAIN_MENU     1
static struct menu_widget victory_widget = {
	.title = "VICTORY!",
	.cur_item = 0, .num_items = 2,
	.items = {
		"(r) reset",
		"    main menu",
	},
};

void init_game_state(struct game_state *game_state) {
	game_state->anim_tick = ANIM_LEN;
	game_state->animating = 1;
	game_state->num_moves = 0;
	game_state->state = GAME_STATE_ALIVE;
	memcpy(&game_state->reset, &game_state->puzzle, sizeof(game_state->puzzle));
	game_state->target_tex = SDL_CreateTexture(game_state->renderer, SDL_PIXELFORMAT_RGBA32,
	                                           SDL_TEXTUREACCESS_TARGET,
	                                           TW * game_state->puzzle.width,
	                                           TH * game_state->puzzle.height);
	solve_puzzle(&game_state->solution, &game_state->puzzle);
}

static void do_move(struct game_state *game_state, enum player_move move) {
	game_state->moves[game_state->num_moves++] = move;
	game_state->anim_queue.len = 0;
	enum move_response move_response = step_puzzle(&game_state->puzzle,
	                                               move, &game_state->anim_queue);
	switch (move_response) {
	case MOVE_RESPONSE_NONE:
		break;
	case MOVE_RESPONSE_DEATH:
		game_state->state = GAME_STATE_OVER;
		game_over_widget.cur_item = 0;
		break;
	case MOVE_RESPONSE_VICTORY:
		game_state->state = GAME_STATE_VICTORY;
		victory_widget.cur_item = 0;
		break;
	}
	game_state->anim_tick = ANIM_LEN;
	game_state->animating = 1;
}

void run_game(struct game_state *game_state) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (game_state->state) {
		case GAME_STATE_OVER:
			{
				s32 response = menu_widget_handle_event(&game_over_widget, &e);
				switch (response) {
				case MENU_WIDGET_QUIT:
					goto quit;
				case GAME_OVER_WIDGET_UNDO:
					goto undo_move;
				case GAME_OVER_WIDGET_RESET:
					goto reset;
				case GAME_OVER_WIDGET_SHOW_SOLUTION:
					goto show_solution;
				case GAME_OVER_WIDGET_MAIN_MENU:
					goto return_to_main_menu;
				}
			} // XXX - deliberate fallthrough
		case GAME_STATE_VICTORY:
			if (game_state->state == GAME_STATE_VICTORY) {
				s32 response = menu_widget_handle_event(&victory_widget, &e);
				switch (response) {
				case MENU_WIDGET_QUIT:
					goto quit;
				case VICTORY_WIDGET_RESET:
					goto reset;
				case VICTORY_WIDGET_MAIN_MENU:
					goto return_to_main_menu;
				}
			}
		case GAME_STATE_SHOW_SOLUTION:
		case GAME_STATE_ALIVE:
			switch (e.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_KEYUP:
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					goto move_north;
				case SDLK_RIGHT:
					goto move_east;
				case SDLK_DOWN:
					goto move_south;
				case SDLK_LEFT:
					goto move_west;
				case SDLK_SPACE:
					goto move_pause;
				case SDLK_ESCAPE:
					goto show_menu;
				case SDLK_z:
					goto undo_move;
				case SDLK_r:
					goto reset;
				case SDLK_s:
					goto show_solution;
				}
				break;
			case SDL_CONTROLLERBUTTONUP:
				switch (e.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_DPAD_UP:
					goto move_north;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					goto move_east;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					goto move_south;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					goto move_west;
				case SDL_CONTROLLER_BUTTON_A:
					goto move_pause;
				case SDL_CONTROLLER_BUTTON_B:
					goto undo_move;
				case SDL_CONTROLLER_BUTTON_Y:
					goto reset;
				case SDL_CONTROLLER_BUTTON_X:
					goto show_solution;
				case SDL_CONTROLLER_BUTTON_START:
				case SDL_CONTROLLER_BUTTON_BACK:
					goto show_menu;
				}
				break;
			case SDL_CONTROLLERAXISMOTION:
				switch (e.caxis.axis) {
				case SDL_CONTROLLER_AXIS_LEFTX:
				case SDL_CONTROLLER_AXIS_RIGHTX:
					if (e.caxis.value > 32000) {
						goto move_east;
					} else if (e.caxis.value < -32000) {
						goto move_west;
					}
					break;
				case SDL_CONTROLLER_AXIS_LEFTY:
				case SDL_CONTROLLER_AXIS_RIGHTY:
					if (e.caxis.value > 32000) {
						goto move_south;
					} else if (e.caxis.value < -32000) {
						goto move_north;
					}
					break;
				}
				break;
			}
			break;
		case GAME_STATE_PAUSED:
			{
				s32 response = menu_widget_handle_event(&pause_widget, &e);
				switch (response) {
				case MENU_WIDGET_QUIT:
					goto quit;
				case PAUSE_WIDGET_UNDO:
					game_state->state = game_state->prev_state;
					goto undo_move;
				case PAUSE_WIDGET_RESET:
					game_state->state = game_state->prev_state;
					goto reset;
				case PAUSE_WIDGET_SHOW_SOLUTION:
					game_state->state = game_state->prev_state;
					goto show_solution;
				case PAUSE_WIDGET_CONTINUE:
					goto leave_menu;
				case PAUSE_WIDGET_MAIN_MENU:
					goto return_to_main_menu;
				}
			}
			break;
		}
		continue;

	move_north:
		if (game_state->animating || game_state->state != GAME_STATE_ALIVE) {
			continue;
		}
		do_move(game_state, PLAYER_MOVE_N);
		continue;
	move_east:
		if (game_state->animating || game_state->state != GAME_STATE_ALIVE) {
			continue;
		}
		do_move(game_state, PLAYER_MOVE_E);
		continue;
	move_south:
		if (game_state->animating || game_state->state != GAME_STATE_ALIVE) {
			continue;
		}
		do_move(game_state, PLAYER_MOVE_S);
		continue;
	move_west:
		if (game_state->animating || game_state->state != GAME_STATE_ALIVE) {
			continue;
		}
		do_move(game_state, PLAYER_MOVE_W);
		continue;
	move_pause:
		if (game_state->animating || game_state->state != GAME_STATE_ALIVE) {
			continue;
		}
		do_move(game_state, PLAYER_MOVE_PAUSE);
		continue;
	reset:
		game_state->num_moves = 0;
		game_state->state = GAME_STATE_ALIVE;
		memcpy(&game_state->puzzle, &game_state->reset, sizeof(game_state->puzzle));
		continue;
	undo_move:
		if (game_state->num_moves) {
			game_state->state = GAME_STATE_ALIVE;
			memcpy(&game_state->puzzle, &game_state->reset, sizeof(game_state->puzzle));
			--game_state->num_moves;
			for (u32 i = 0; i < game_state->num_moves; ++i) {
				step_puzzle(&game_state->puzzle, game_state->moves[i], NULL);
			}
		}
		continue;
	show_solution:
		game_state->num_moves = 0;
		game_state->state = GAME_STATE_SHOW_SOLUTION;
		memcpy(&game_state->puzzle, &game_state->reset, sizeof(game_state->puzzle));
		do_move(game_state, game_state->solution.moves[game_state->num_moves]);
		continue;
	show_menu:
		if (game_state->state != GAME_STATE_OVER && game_state->state != GAME_STATE_VICTORY) {
			pause_widget.cur_item = 0;
			game_state->menu_item_idx = 0;
			game_state->prev_state = game_state->state;
			game_state->state = GAME_STATE_PAUSED;
		}
		continue;
	leave_menu:
		game_state->state = game_state->prev_state;
		continue;
	return_to_main_menu:
		state = STATE_MENU;
		SDL_DestroyTexture(game_state->target_tex);
		return;
	}

	u32 this_tick = SDL_GetTicks();
	u32 frame_time = this_tick - game_state->last_tick;
	game_state->last_tick = this_tick;
	if (game_state->animating && (frame_time > game_state->anim_tick)) {
		switch (game_state->state) {
		case GAME_STATE_OVER:
		case GAME_STATE_ALIVE:
		case GAME_STATE_VICTORY:
		case GAME_STATE_PAUSED:
			game_state->animating = 0;
			break;
		case GAME_STATE_SHOW_SOLUTION:
			do_move(game_state, game_state->solution.moves[game_state->num_moves]);
			break;
		}
	} else if (game_state->animating) {
		game_state->anim_tick -= frame_time;
	}

	SDL_Renderer *renderer = game_state->renderer;
	SDL_SetRenderDrawColor(renderer, 16, 0, 16, 255);
	SDL_RenderClear(renderer);

	SDL_SetRenderTarget(renderer, game_state->target_tex);
	SDL_RenderClear(renderer);

	SDL_Texture *sprite_tex = game_state->sprite_tex;
	SDL_Texture *font_tex   = game_state->font_tex;

	f32 anim_fac = 1.0f;
	if (game_state->animating) {
		anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
	}

	draw_puzzle(renderer, sprite_tex, &game_state->puzzle, &game_state->anim_queue,
	            &game_state->explosion_queue, game_state->animating, anim_fac, frame_time);

	SDL_SetRenderTarget(renderer, NULL);
	{
		s32 w = TW * game_state->puzzle.width, h = TH * game_state->puzzle.height;
		u32 sf = MIN(SW / w, SH / h);
		w *= sf; h *= sf;
		SDL_Rect src = { 0, 0, w, h }, dst = { (SW - w) / 2, (SH - h) / 2, w, h };
		SDL_RenderCopy(renderer, game_state->target_tex, &src, &dst);
	}

	if (game_state->state == GAME_STATE_OVER) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect r = { 0, 0, SW, SH };
		f32 anim_fac = 1.0f;
		if (game_state->animating) {
			anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, (u8)(anim_fac * 64.0f));
		SDL_RenderFillRect(renderer, &r);
		if (anim_fac == 1.0f) {
			draw_widget(&game_over_widget, renderer, font_tex);
		}
	}

	if (game_state->state == GAME_STATE_VICTORY) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect r = { 0, 0, SW, SH };
		f32 anim_fac = 1.0f;
		if (game_state->animating) {
			anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, (u8)(anim_fac * 64.0f));
		SDL_RenderFillRect(renderer, &r);
		if (anim_fac == 1.0f) {
			draw_widget(&victory_widget, renderer, font_tex);
		}
	}

	if (game_state->state == GAME_STATE_PAUSED) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect r = { 0, 0, SW, SH };
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
		SDL_RenderFillRect(renderer, &r);
		draw_widget(&pause_widget, renderer, font_tex);
	}

	SDL_RenderPresent(renderer);

	return;
quit:
	SDL_DestroyTexture(game_state->target_tex);
	state = STATE_QUIT;
	return;
}
