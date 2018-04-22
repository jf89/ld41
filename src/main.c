#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>
#include <SDL_image.h>

#include "types.h"
#include "my_math.h"
#include "state.h"
#include "puzzle.h"
#include "generator.h"
#include "game.h"
#include "menu.h"

char *res_dir = NULL;

char *get_resource_filename(const char *filename) {
	size_t len = strlen(res_dir) + strlen(filename) + 1;
	char *result = malloc(len * sizeof(char));
	strcpy(result, res_dir);
	strcat(result, filename);
	return result;
}

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *filename) {
	char *full_filename = get_resource_filename(filename);
	SDL_Surface *surface = IMG_Load(full_filename);
	free(full_filename);
	if (surface == NULL) {
		printf("Unable to load image '%s': %s\n", filename, IMG_GetError());
		return NULL;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if (texture == NULL) {
		printf("Unable to create texture '%s': %s\n", filename, SDL_GetError());
		return NULL;
	}
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	return texture;
}

int main(s32 argc, char *argv[]) {
	s32 exit_success = EXIT_FAILURE;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS |
	             SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO)) {
		printf("Unable to initialise SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	SDL_Window *window = SDL_CreateWindow("yatbbh", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 1300, 700, 0);
	if (window == NULL) {
		printf("Unable to create window: %s\n", SDL_GetError());
		goto cleanup_sdl;
	}
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("Unable to create renderer: %s\n", SDL_GetError());
		goto cleanup_window;
	}
	{
		char *base_dir = SDL_GetBasePath();
		size_t len = strlen(base_dir);
		base_dir[len - 4] = 0;
		res_dir = malloc(len * sizeof(char));
		strcpy(res_dir, base_dir);
		SDL_free(base_dir);
		strcat(res_dir, "res/");
	}
	SDL_Texture *sprite_tex = load_texture(renderer, "sprites.png");
	if (sprite_tex == NULL) {
		goto cleanup_renderer;
	}
	SDL_Texture *font_tex = load_texture(renderer, "codepage437.png");
	if (font_tex == NULL) {
		goto cleanup_sprite_tex;
	}
	SDL_GameController *controller = NULL;
	{ // init game controller
		s32 num_controllers = SDL_NumJoysticks();
		for (s32 i = 0; i < num_controllers; ++i) {
			if (SDL_IsGameController(i)) {
				controller = SDL_GameControllerOpen(i);
				if (controller) {
					break;
				}
			}
		}
	}

	struct menu_state menu_state;
	struct game_state game_state;

	state = STATE_MENU;

	menu_state.renderer = renderer;
	menu_state.font_tex = font_tex;
	menu_state.game_state = &game_state;

	game_state.renderer   = renderer;
	game_state.sprite_tex = sprite_tex;
	game_state.font_tex   = font_tex;
	// generate_hard_puzzle_2(&game_state.puzzle);
	// init_game_state(&game_state);
	u32 seed = time(NULL);
	printf("Random seed: 0x%x\n", seed);
	srand(seed);
	while (1) {
		switch (state) {
			case STATE_QUIT:
				exit_success = EXIT_SUCCESS;
				goto finished_running;
			case STATE_GAME:
				run_game(&game_state);
				break;
			case STATE_MENU:
				run_menu(&menu_state);
				break;
		}
	}

finished_running:
	if (controller) {
		SDL_GameControllerClose(controller);
	}
	SDL_DestroyTexture(font_tex);
cleanup_sprite_tex:
	SDL_DestroyTexture(sprite_tex);
	free(res_dir);
cleanup_renderer:
	SDL_DestroyRenderer(renderer);
cleanup_window:
	SDL_DestroyWindow(window);
cleanup_sdl:
	SDL_Quit();
	return exit_success;
}
