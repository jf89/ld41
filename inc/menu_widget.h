#ifndef __MENU_WIDGET_H__
#define __MENU_WIDGET_H__

#include <SDL.h>

#include "types.h"
#include "game.h"

#define MENU_WIDGET_NO_SELECTION -1
#define MENU_WIDGET_QUIT         -2

struct menu_widget {
	char *title;
	u32 cur_item, num_items;
	char *items[];
};

void draw_widget(struct menu_widget *widget, SDL_Renderer *renderer, SDL_Texture *font_tex);
s32 menu_widget_handle_event(struct menu_widget *menu_widget, SDL_Event *event);

#endif
