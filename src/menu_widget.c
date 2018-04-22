#include "menu_widget.h"

#include <stdio.h>
#include <SDL.h>

#include "types.h"
#include "draw.h"

#define TITLE_SCALE      8
#define MENU_ITEM_SCALE  2
#define FONT_WIDTH       9
#define FONT_HEIGHT      16
#define MENU_ITEM_LEFT   (MENU_ITEM_SCALE * FONT_WIDTH * 4)

static u32 last_controller_ticks = 0;

s32 menu_widget_handle_event(struct menu_widget *menu_widget, SDL_Event *event) {
	switch (event->type) {
	case SDL_QUIT:
		goto quit;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
		case SDLK_UP:
			goto menu_up;
		case SDLK_DOWN:
			goto menu_down;
		case SDLK_RETURN:
		case SDLK_SPACE:
			goto menu_select;
		}
		break;
	case SDL_CONTROLLERBUTTONUP:
		switch (event->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			goto menu_up;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			goto menu_down;
		case SDL_CONTROLLER_BUTTON_A:
			goto menu_select;
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		switch (event->caxis.axis) {
		case SDL_CONTROLLER_AXIS_LEFTY:
		case SDL_CONTROLLER_AXIS_RIGHTY:
			if (event->caxis.value > 32000) {
				if (SDL_GetTicks() - last_controller_ticks > 1000) {
					last_controller_ticks = SDL_GetTicks();
					goto menu_down;
				}
			} else if (event->caxis.value < -32000) {
				if (SDL_GetTicks() - last_controller_ticks > 1000) {
					last_controller_ticks = SDL_GetTicks();
					goto menu_up;
				}
			} else {
				last_controller_ticks = 0;
			}
			break;
		}
		break;
	}
	return MENU_WIDGET_NO_SELECTION;

menu_up:
	menu_widget->cur_item += menu_widget->num_items - 1;
	menu_widget->cur_item %= menu_widget->num_items;
	return MENU_WIDGET_NO_SELECTION;
menu_down:
	++menu_widget->cur_item;
	menu_widget->cur_item %= menu_widget->num_items;
	return MENU_WIDGET_NO_SELECTION;
menu_select:
	return menu_widget->cur_item;
quit:
	return MENU_WIDGET_QUIT;
}

void draw_widget(struct menu_widget *menu_widget, SDL_Renderer *renderer, SDL_Texture *font_tex) {
	u32 title_len = strlen(menu_widget->title);
	draw_string(renderer, font_tex, menu_widget->title,
	            (1300 - (FONT_WIDTH * title_len * TITLE_SCALE)) / 2, 0,
	            TITLE_SCALE, 255, 255, 255);

	u32 menu_item_top = ((700 - ((menu_widget->num_items * FONT_HEIGHT * MENU_ITEM_SCALE) + \
                                      TITLE_SCALE * FONT_HEIGHT)) / 2 + TITLE_SCALE * FONT_HEIGHT);
	for (u32 i = 0; i < menu_widget->num_items; ++i) {
		u8 color = i == menu_widget->cur_item ? 255 : 128;
		draw_string(renderer, font_tex, menu_widget->items[i], MENU_ITEM_LEFT,
		            menu_item_top + i * MENU_ITEM_SCALE * FONT_HEIGHT,
		            MENU_ITEM_SCALE, color, color, color);
	}
}
