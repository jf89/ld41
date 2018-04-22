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
#define ANIM_LEN 250

#define EXPLOSION_LEN 500

#define PARTICLE_W 4
#define PARTICLE_H 4

#define TIMER_W 7
#define TIMER_H 7

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
#define GAME_OVER_WIDGET_CONTINUE      3
#define GAME_OVER_WIDGET_MAIN_MENU     4
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

s32 lerp(s32 x1, s32 x2, f32 v) {
	f32 fx1 = (float)x1, fx2 = (float)x2;
	return (s32)(fx1 + (fx2 - fx1) * v);
}

static f64 dir_to_angle(enum direction dir) {
	switch (dir) {
	case DIR_N:
		return 0.0;
	case DIR_NE:
		return 45.0;
	case DIR_E:
		return 90.0;
	case DIR_SE:
		return 135.0;
	case DIR_S:
		return 180.0;
	case DIR_SW:
		return 225.0;
	case DIR_W:
		return 270.0;
	case DIR_NW:
		return 315.0;
	}
	printf("Invalid argument to dir_to_angle.\n");
	return 0.0;
}

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
		break;
	case MOVE_RESPONSE_VICTORY:
		game_state->state = GAME_STATE_VICTORY;
		break;
	}
	game_state->anim_tick = ANIM_LEN;
	game_state->animating = 1;
}

#define PI 3.1415926535f
#define MIN_SPEED 30.0f
#define MAX_SPEED 60.0f
static void add_explosion(struct game_state *game_state, s32 x, s32 y) {
	// TODO -- check we're not exceeding the max explosions?
	struct explosion *exp = &game_state->explosions[game_state->num_explosions++];
	exp->x = x; exp->y = y; exp->ticks = EXPLOSION_LEN;
	struct particle *part = exp->particles;
	for (u32 i = 0; i < EXPLOSION_PARTICLES; ++i, ++part) {
		f32 theta = ((f32)rand() / (f32)RAND_MAX) * 2.0f * PI;
		part->dx    = cos(theta);
		part->dy    = sin(theta);
		part->speed = MIN_SPEED + (((f32)rand() / (f32)RAND_MAX) * MAX_SPEED - MIN_SPEED);
	}
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
	SDL_SetRenderDrawColor(renderer, 32, 32, 48, 255);
	SDL_RenderClear(renderer);

	SDL_SetRenderTarget(renderer, game_state->target_tex);
	SDL_RenderClear(renderer);

	SDL_Texture *sprite_tex = game_state->sprite_tex;
	SDL_Texture *font_tex   = game_state->font_tex;

	// draw background
	{
		u32 w = game_state->puzzle.width, h = game_state->puzzle.height;
		enum tile *p = game_state->puzzle.tiles;
		SDL_Rect r = { 0, 0, TW, TH };
		for (u32 j = 0; j < h; ++j) {
			for (u32 i = 0; i < w; ++i, ++p) {
				r.x = i * TW; r.y = j * TH;
				u8 k = (i + j) % 2 ? 32 : 0;
				switch (*p) {
				case TILE_GOAL:
					SDL_SetRenderDrawColor(renderer, 0, 168, 0, 255);
					SDL_RenderFillRect(renderer, &r);
					break;
				case TILE_EMPTY:
					SDL_SetRenderDrawColor(renderer, 64 + k, 64 + k, 96 + k, 255);
					SDL_RenderFillRect(renderer, &r);
					break;
				case TILE_EMITTER:
					SDL_SetRenderDrawColor(renderer, 16 + k, 16 + k, 16 + k, 255);
					SDL_RenderFillRect(renderer, &r);
					break;
				case TILE_WALL:
					SDL_SetRenderDrawColor(renderer, 64 + k, 32 + k, 32 + k, 255);
					SDL_RenderFillRect(renderer, &r);
					break;
				}
			}
		}
	}

	// draw emitter timers
	{
		u32 num_emitters  = game_state->puzzle.num_emitters;
		struct emitter *p = game_state->puzzle.emitters;
		for (u32 i = 0; i < num_emitters; ++i, ++p) {
			u32 len = p->num_steps;
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_Rect r = { p->x * TW + (TW - (len * TIMER_W)) / 2,
				p->y * TH + (TH - 2*TIMER_H), len * TIMER_W, TIMER_H };
			SDL_RenderFillRect(renderer, &r);
			++r.x; ++r.y; r.w = TIMER_W - 2; r.h = TIMER_H - 2;
			for (u32 j = 0; j < len; ++j) {
				u8 color = (p->step == j + 1) ? 255 : 128;
				if ((1 << j) & p->fire_mask) {
					SDL_SetRenderDrawColor(renderer, 0, color, 0, 255);
				} else {
					SDL_SetRenderDrawColor(renderer, color, 0, 0, 255);
				}
				SDL_RenderFillRect(renderer, &r);
				r.x += TIMER_W;
			}
		}
	}

	// draw player
	{
		SDL_Rect src = { 2*TW, 0, TW, TH }, dst = { 0, 0, TW, TH };
		if (game_state->animating) {
			f32 anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
			u32 len = game_state->anim_queue.len;
			struct anim *p = game_state->anim_queue.queue;
			for (u32 i = 0; i < len; ++i, ++p) {
				if (p->type == ANIMATION_PLAYER_MOVE) {
					dst.x = lerp(p->player_move.sx * TW,
					             p->player_move.ex * TW, anim_fac);
					dst.y = lerp(p->player_move.sy * TW,
					             p->player_move.ey * TW, anim_fac);
					SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
					goto drawn_player;
				}
			}
		}
		dst.x = game_state->puzzle.player.x * TW;
		dst.y = game_state->puzzle.player.y * TH;
		SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
	}
drawn_player:

	// draw bullets
	{
		SDL_Rect src = { 0, 0, TW, TH }, dst = { 0, 0, TW, TH };
		SDL_Point center = { TW / 2, TH / 2 };
		if (game_state->animating) {
			f32 anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
			u32 len = game_state->anim_queue.len;
			struct anim *p = game_state->anim_queue.queue;
			for (u32 i = 0; i < len; ++i, ++p) {
				switch (p->type) {
				case ANIMATION_BULLET_MOVE: {
						f64 angle = dir_to_angle(p->bullet_move.dir);
						dst.x = lerp(p->bullet_move.sx * TW,
						             p->bullet_move.ex * TW, anim_fac);
						dst.y = lerp(p->bullet_move.sy * TH,
						             p->bullet_move.ey * TH, anim_fac);
						SDL_RenderCopyEx(renderer, sprite_tex, &src, &dst,
							angle, &center, SDL_FLIP_NONE);
					} break;
				case ANIMATION_BULLET_EXPLODE_EDGE:
					if (anim_fac > 0.5f) {
						if (!p->bullet_explode.added_explosion) {
							s32 x = (p->bullet_explode.sx +
								 p->bullet_explode.ex + 1) * TW / 2;
							s32 y = (p->bullet_explode.sy +
								 p->bullet_explode.ey + 1) * TH / 2;
							p->bullet_explode.added_explosion = 1;
							add_explosion(game_state, x, y);
						}
					} else {
						f64 angle = dir_to_angle(p->bullet_explode.dir);
						dst.x = lerp(p->bullet_explode.sx * TW,
						             p->bullet_explode.ex * TW, anim_fac);
						dst.y = lerp(p->bullet_explode.sy * TH,
						             p->bullet_explode.ey * TH, anim_fac);
						SDL_RenderCopyEx(renderer, sprite_tex, &src, &dst,
							angle, &center, SDL_FLIP_NONE);
					}
					break;
				case ANIMATION_BULLET_EXPLODE_MID:
					if (anim_fac > 0.9f) {
						if (!p->bullet_explode.added_explosion) {
							p->bullet_explode.added_explosion = 1;
							add_explosion(game_state,
								      p->bullet_explode.ex * TW + TW/2,
								      p->bullet_explode.ey * TH + TH/2);
						}
					} else {
						f64 angle = dir_to_angle(p->bullet_explode.dir);
						dst.x = lerp(p->bullet_explode.sx * TW,
						             p->bullet_explode.ex * TW, anim_fac);
						dst.y = lerp(p->bullet_explode.sy * TH,
						             p->bullet_explode.ey * TH, anim_fac);
						SDL_RenderCopyEx(renderer, sprite_tex, &src, &dst,
							angle, &center, SDL_FLIP_NONE);
					}
					break;
				case ANIMATION_PLAYER_MOVE:
					break;
				}
			}
		} else {
			u32 num_bullets  = game_state->puzzle.num_bullets;
			struct bullet *b = game_state->puzzle.bullets;
			for (u32 i = 0; i < num_bullets; ++i, ++b) {
				f64 angle = dir_to_angle(b->dir);
				dst.x = b->x * TW; dst.y = b->y * TH;
				SDL_RenderCopyEx(renderer, sprite_tex, &src, &dst, angle,
					&center, SDL_FLIP_NONE);
			}
		}
	}

	// draw explosions
	{
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect r = { 0, 0, PARTICLE_W, PARTICLE_H };
		u32 num_explosions = game_state->num_explosions;
		u32 i = 0;
		while (i < num_explosions) {
			struct explosion *exp = &game_state->explosions[i];
			if (frame_time > exp->ticks) {
				*exp = game_state->explosions[--num_explosions];
				continue;
			}
			++i;
			exp->ticks -= frame_time;
			f32 exp_time = 1.0f - ((f32)exp->ticks / (f32)EXPLOSION_LEN);
			u32 x = exp->x, y = exp->y;
			struct particle *p = exp->particles;
			for (u32 j = 0; j < EXPLOSION_PARTICLES; ++j, ++p) {
				r.x = x + (s32)(p->dx * p->speed * exp_time);
				r.y = y + (s32)(p->dy * p->speed * exp_time);
				SDL_SetRenderDrawColor(renderer, 255, 255, 255,
				                       (s8)(192.0f * (1.0f - exp_time)));
				SDL_RenderFillRect(renderer, &r);
			}
		}
		game_state->num_explosions = num_explosions;
	}

	// draw emitter cannons
	{
		f32 anim_fac = 1.0f;
		if (game_state->animating) {
			anim_fac = 1.0f - ((float)game_state->anim_tick / (float)ANIM_LEN);
		}
		u32 num_emitters = game_state->puzzle.num_emitters;
		struct emitter *p = game_state->puzzle.emitters;
		SDL_Rect src = { TW, 0, TW, TH }, dst = { 0, 0, TW, TH };
		SDL_Point center = { TW / 2, TH / 2 };
		for (u32 i = 0; i < num_emitters; ++i, ++p) {
			dst.x = p->x * TW; dst.y = p->y * TH;
			f32 offset;
			switch (p->type) {
			case EMITTER_FIXED:
				offset = 0.0f;
				break;
			case EMITTER_CLOCKWISE:
				if (anim_fac < 0.5f) {
					offset = 0.0f;
				} else {
					offset = 2.0f * (anim_fac - 0.5f);
				}
				break;
			case EMITTER_COUNTER_CLOCKWISE:
				if (anim_fac < 0.5f) {
					offset = 0.0f;
				} else {
					offset = -2.0f * (anim_fac - 0.5f);
				}
				break;
			}
			for (u32 d = 0; d < NUM_DIRS; ++d) {
				if (p->dir_mask & (1 << d)) {
					SDL_RenderCopyEx(renderer, sprite_tex, &src, &dst,
						45.0 * ((f64)d + offset), &center, SDL_FLIP_NONE);
				}
			}
		}
	}

	SDL_SetRenderTarget(renderer, NULL);
	{
		s32 w = TW * game_state->puzzle.width, h = TH * game_state->puzzle.height;
		SDL_Rect src = { 0, 0, w, h }, dst = { (1300 - w) / 2, (700 - h) / 2, w, h };
		SDL_RenderCopy(renderer, game_state->target_tex, &src, &dst);
	}

	if (game_state->state == GAME_STATE_OVER) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_Rect r = { 0, 0, 1300, 700 };
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
		SDL_Rect r = { 0, 0, 1300, 700 };
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
		SDL_Rect r = { 0, 0, 1300, 700 };
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
