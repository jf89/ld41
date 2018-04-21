#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_image.h>

#include "types.h"
#include "my_math.h"
#include "state.h"
#include "puzzle.h"
#include "game.h"

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


	state = STATE_GAME;
	struct game_state game_state;
	game_state.renderer   = renderer;
	game_state.sprite_tex = sprite_tex;
	game_state.puzzle     = generate_puzzle(10, 10, 10);
	while (1) {
		switch (state) {
			case STATE_QUIT:
				exit_success = EXIT_SUCCESS;
				goto finished_running;
			case STATE_GAME:
				run_game(&game_state);
				break;
		}
	}

	/* struct puzzle puzzle = generate_puzzle(10, 10, 10);
	struct map    map    = generate_map(&puzzle);
	while (1) {
		char command;
		print_puzzle(&puzzle);
		printf("command: ");
		scanf(" %c", &command);
		printf("received command: %c\n", command);
		switch (command) {
		case 'j':
			step_puzzle(&puzzle, PLAYER_MOVE_S);
			break;
		case 'k':
			step_puzzle(&puzzle, PLAYER_MOVE_N);
			break;
		case 'h':
			step_puzzle(&puzzle, PLAYER_MOVE_W);
			break;
		case 'l':
			step_puzzle(&puzzle, PLAYER_MOVE_E);
			break;
		case 's':
			step_puzzle(&puzzle, PLAYER_MOVE_PAUSE);
			break;
		case 'p': {
				u32 page;
				printf("input page number: ");
				scanf(" %u", &page);
				print_map_page(&map, page);
			} break;
		case 'g': {
				u32 x, y;
				printf("input x, y: ");
				scanf(" %u %u", &x, &y);
				get_furthest_point(&map, x, y);
			} break;
		case 'r':
			reset_map(&map);
			break;
		case 'q':
			goto finished_running;
		default:
			printf("unknown command: %c\n", command);
		}
	} */

finished_running:
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
