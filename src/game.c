#include "game.h"

#include <stdio.h>
#include <SDL.h>

#include "state.h"
#include "puzzle.h"
#include "anim.h"

#define TW 64
#define TH 64
#define ANIM_LEN 500

#define TIMER_W 7
#define TIMER_H 7

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
}

void run_game(struct game_state *game_state) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			goto quit;
		}
	}

	u32 this_tick = SDL_GetTicks();
	u32 frame_time = this_tick - game_state->last_tick;
	game_state->last_tick = this_tick;
	if (game_state->animating && (frame_time > game_state->anim_tick)) {
		game_state->anim_tick = ANIM_LEN;
		// game_state->animating = 0;
		game_state->anim_queue.len = 0;
		step_puzzle(&game_state->puzzle, PLAYER_MOVE_PAUSE, &game_state->anim_queue);
	} else if (game_state->animating) {
		game_state->anim_tick -= frame_time;
	}

	SDL_Renderer *renderer = game_state->renderer;
	SDL_SetRenderDrawColor(renderer, 32, 32, 48, 255);
	SDL_RenderClear(renderer);

	SDL_Texture *sprite_tex = game_state->sprite_tex;

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

	// draw emitter cannons
	{
		f32 anim_fac = 0.0f;
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

	SDL_RenderPresent(renderer);

	return;
quit:
	state = STATE_QUIT;
	return;
}
