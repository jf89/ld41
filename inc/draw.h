#ifndef __DRAW_H__
#define __DRAW_H__

#include <SDL.h>

#include "types.h"
#include "puzzle.h"

void draw_puzzle(SDL_Renderer *renderer, SDL_Texture *sprite_tex, struct puzzle *puzzle,
                 struct anim_queue *anim_queue, struct explosion_queue *explosion_queue,
                 u32 animating, f32 anim_fac, u32 frame_time);
void draw_string(SDL_Renderer *renderer, SDL_Texture *font_tex,
                 const char *string, u32 x, u32 y, u32 scale, u8 r, u8 g, u8 b);

#endif
