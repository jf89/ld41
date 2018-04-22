#include "draw.h"

#include <SDL.h>

#include "types.h"
#include "anim.h"
#include "puzzle.h"

#define FONT_WIDTH  9
#define FONT_HEIGHT 16

#define TW 64
#define TH 64

#define TIMER_W 7
#define TIMER_H 7

#define EXPLOSION_LEN 500

#define PARTICLE_W 4
#define PARTICLE_H 4

void draw_string(SDL_Renderer *renderer, SDL_Texture *font_tex,
                 const char *string, u32 x, u32 y, u32 scale, u8 r, u8 g, u8 b) {
	SDL_SetTextureColorMod(font_tex, r, g, b);
	SDL_Rect src = { 0, 0, FONT_WIDTH, FONT_HEIGHT };
	SDL_Rect dst = { x, y, FONT_WIDTH * scale, FONT_HEIGHT * scale };
	for (const char *p = string; *p; ++p, dst.x += scale * FONT_WIDTH) {
		src.x = (((u8)*p) & 0x1F) * FONT_WIDTH;
		src.y = (((u8)*p) >> 5) * FONT_HEIGHT;
		SDL_RenderCopy(renderer, font_tex, &src, &dst);
	}
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

static s32 lerp(s32 x1, s32 x2, f32 v) {
	f32 fx1 = (float)x1, fx2 = (float)x2;
	return (s32)(fx1 + (fx2 - fx1) * v);
}

#define PI 3.1415926535f
#define MIN_SPEED 30.0f
#define MAX_SPEED 60.0f
static void add_explosion(struct explosion_queue *explosion_queue, s32 x, s32 y) {
	// TODO -- check we're not exceeding the max explosions?
	struct explosion *exp = &explosion_queue->explosions[explosion_queue->num_explosions++];
	exp->x = x; exp->y = y; exp->ticks = EXPLOSION_LEN;
	struct particle *part = exp->particles;
	for (u32 i = 0; i < EXPLOSION_PARTICLES; ++i, ++part) {
		f32 theta = ((f32)rand() / (f32)RAND_MAX) * 2.0f * PI;
		part->dx    = cos(theta);
		part->dy    = sin(theta);
		part->speed = MIN_SPEED + (((f32)rand() / (f32)RAND_MAX) * MAX_SPEED - MIN_SPEED);
	}
}

void draw_puzzle(SDL_Renderer *renderer, SDL_Texture *sprite_tex, struct puzzle *puzzle,
                 struct anim_queue *anim_queue, struct explosion_queue *explosion_queue,
                 u32 animating, f32 anim_fac, u32 frame_time) {
	// draw background
	{
		u32 w = puzzle->width, h = puzzle->height;
		enum tile *p = puzzle->tiles;
		SDL_Rect src = { 0, 0, TW, TH }, dst = { 0, 0, TW, TH };
		for (u32 j = 0; j < h; ++j) {
			for (u32 i = 0; i < w; ++i, ++p) {
				dst.x = i * TW; dst.y = j * TH;
				u8 k = (i + j) % 2 ? 16 : 0;
				switch (*p) {
				case TILE_GOAL:
					src.x = 2 * TW; src.y = 2 * TH;
					SDL_SetTextureColorMod(sprite_tex, 64 + k, 64 + k, 64 + k);
					SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
					break;
				case TILE_EMPTY:
					src.x = 0; src.y = 2 * TH;
					SDL_SetTextureColorMod(sprite_tex, 48 + k, 48 + k, 48 + k);
					SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
					break;
				case TILE_EMITTER:
					src.x = TW; src.y = 2 * TH;
					SDL_SetTextureColorMod(sprite_tex, 48 + k, 48 + k, 48 + k);
					SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
					break;
				case TILE_WALL:
					SDL_SetRenderDrawColor(renderer, 64 + k, 32 + k, 32 + k, 255);
					SDL_RenderFillRect(renderer, &dst);
					break;
				}
			}
		}
	}
	SDL_SetTextureColorMod(sprite_tex, 255, 255, 255);

	// draw emitter timers
	{
		u32 num_emitters  = puzzle->num_emitters;
		struct emitter *p = puzzle->emitters;
		for (u32 i = 0; i < num_emitters; ++i, ++p) {
			u32 len = p->num_steps;
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_Rect r = { p->x * TW + (TW - (len * TIMER_W)) / 2,
				p->y * TH + (TH - 2*TIMER_H), len * TIMER_W, TIMER_H };
			SDL_RenderFillRect(renderer, &r);
			++r.x; ++r.y; r.w = TIMER_W - 2; r.h = TIMER_H - 2;
			for (u32 j = 0; j < len; ++j) {
				u8 color = (p->step == j + 1) ? 255 : 128;

				if (!animating) {
					color = (p->step % p->num_steps == j) ? 255 : 128;
				}

				if ((1 << j) & p->fire_mask) {
					SDL_SetRenderDrawColor(renderer, 0, color, color, 255);
				} else {
					SDL_SetRenderDrawColor(renderer, color, 0, color, 255);
				}
				SDL_RenderFillRect(renderer, &r);
				r.x += TIMER_W;
			}
		}
	}

	// draw player
	{
		SDL_Rect src = { 2*TW, 0, TW, TH }, dst = { 0, 0, TW, TH };
		if (animating) {
			u32 len = anim_queue->len;
			struct anim *p = anim_queue->queue;
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
		dst.x = puzzle->player.x * TW; dst.y = puzzle->player.y * TH;
		SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
	}
drawn_player:

	// draw bullets
	{
		SDL_Rect src = { 0, 0, TW, TH }, dst = { 0, 0, TW, TH };
		SDL_Point center = { TW / 2, TH / 2 };
		if (animating) {
			u32 len = anim_queue->len;
			struct anim *p = anim_queue->queue;
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
							add_explosion(explosion_queue, x, y);
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
							add_explosion(explosion_queue,
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
			u32 num_bullets  = puzzle->num_bullets;
			struct bullet *b = puzzle->bullets;
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
		u32 num_explosions = explosion_queue->num_explosions;
		u32 i = 0;
		while (i < num_explosions) {
			struct explosion *exp = &explosion_queue->explosions[i];
			if (frame_time > exp->ticks) {
				*exp = explosion_queue->explosions[--num_explosions];
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
		explosion_queue->num_explosions = num_explosions;
	}

	// draw emitter bases
	{
		u32 num_emitters = puzzle->num_emitters;
		struct emitter *p = puzzle->emitters;
		SDL_Rect src = { 0, TH, TW, TH }, dst = { 0, 0, TW, TH };
		for (u32 i = 0; i < num_emitters; ++i, ++p) {
			dst.x = p->x * TW; dst.y = p->y * TH;
			switch (p->type) {
			case EMITTER_FIXED:
				src.x = 0;
				break;
			case EMITTER_CLOCKWISE:
				src.x = 2 * TW;
				break;
			case EMITTER_COUNTER_CLOCKWISE:
				src.x = TW;
				break;
			}
			SDL_RenderCopy(renderer, sprite_tex, &src, &dst);
		}
	}

	// draw emitter cannons
	{
		u32 num_emitters = puzzle->num_emitters;
		struct emitter *p = puzzle->emitters;
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
}
