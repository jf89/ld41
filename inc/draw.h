#ifndef __DRAW_H__
#define __DRAW_H__

#include <SDL.h>

#include "types.h"

void draw_string(SDL_Renderer *renderer, SDL_Texture *font_tex,
                 const char *string, u32 x, u32 y, u32 scale, u8 r, u8 g, u8 b);

#endif
