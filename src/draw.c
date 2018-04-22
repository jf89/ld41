#include "draw.h"

#define FONT_WIDTH  9
#define FONT_HEIGHT 16

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
