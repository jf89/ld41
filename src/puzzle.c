#include "puzzle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "my_math.h"
#include "anim.h"

static void step_coords(u32 *x, u32 *y, enum direction dir) {
	switch (dir) {
	case DIR_N:
		(*y)--;
		break;
	case DIR_NE:
		(*x)++; (*y)--;
		break;
	case DIR_E:
		(*x)++;
		break;
	case DIR_SE:
		(*x)++; (*y)++;
		break;
	case DIR_S:
		(*y)++;
		break;
	case DIR_SW:
		(*x)--; (*y)++;
		break;
	case DIR_W:
		(*x)--;
		break;
	case DIR_NW:
		(*x)--; (*y)--;
		break;
	}
}

void print_puzzle(struct puzzle *puzzle) {
	printf("num_bullets: %u\n", puzzle->num_bullets);
	u32 w = puzzle->width, h = puzzle->height;
	for (u32 j = 0; j < h; ++j) {
		char line[MAX_WIDTH + 1] = {};
		for (u32 i = 0; i < w; ++i) {
			if (i == puzzle->player.x && j == puzzle->player.y) {
				line[i] = '@';
				goto next_i;
			}
			struct bullet *bullets = puzzle->bullets;
			for (u32 k = 0; k < puzzle->num_bullets; ++k) {
				if (i == bullets[k].x && j == bullets[k].y) {
					switch (bullets[k].dir) {
					case DIR_N:
					case DIR_S:
						line[i] = '|';
						goto next_i;
					case DIR_E:
					case DIR_W:
						line[i] = '-';
						goto next_i;
					case DIR_NE:
					case DIR_SW:
						line[i] = '/';
						goto next_i;
					case DIR_NW:
					case DIR_SE:
						line[i] = '\\';
						goto next_i;
					}
				}
			}
			switch (puzzle->tiles[j*w + i]) {
			case TILE_EMPTY:
				line[i] = '.';
				goto next_i;
			case TILE_EMITTER:
				line[i] = 'E';
				goto next_i;
			case TILE_WALL:
				line[i] = '#';
				goto next_i;
			}
		next_i: ;
		}
		printf("%s\n", line);
	}
}

struct puzzle generate_puzzle(u32 width, u32 height, u32 num_emitters) {
	struct puzzle puzzle;
	puzzle.width  = width;
	puzzle.height = height;
	puzzle.num_emitters = num_emitters;
	puzzle.num_bullets  = 0;
	for (u32 j = 0; j < height; ++j) {
		for (u32 i = 0; i < width; ++i) {
			puzzle.tiles[j*width + i] = TILE_EMPTY;
		}
	}
	for (u32 i = 0; i < num_emitters; ++i) {
		struct emitter *e = &puzzle.emitters[i];
		u32 x, y;
		do {
			x = rand() % width; y = rand() % height;
		} while (puzzle.tiles[y*width + x] != TILE_EMPTY);
		puzzle.tiles[y*width + x] = TILE_EMITTER;
		e->x = x; e->y = y;
		e->type       = rand() % 3;
		e->dir_mask   = rand() & 0xFF;
		e->fire_mask  = rand() & 0xFF;
		e->num_steps  = (rand() % 8) + 1;
		e->step       = (rand() % e->num_steps) + 1;
	}
	puzzle.player.x = width + 1;
	u32 steps_to_init = MAX(width, height);
	for (u32 i = 0; i < steps_to_init; ++i) {
		step_puzzle(&puzzle, PLAYER_MOVE_PAUSE, NULL);
	}
	return puzzle;
}

void step_puzzle(struct puzzle *puzzle,
                 enum player_move player_move,
                 struct anim_queue *anim_queue) {
	enum tile *tiles = puzzle->tiles;
	u32 num_bullets = puzzle->num_bullets;
	struct bullet *bullets = puzzle->bullets;
	u32 w = puzzle->width, h = puzzle->height;
	{ // step player
		u32 x = puzzle->player.x, y = puzzle->player.y;
		switch (player_move) {
		case PLAYER_MOVE_N:
			step_coords(&x, &y, DIR_N);
			break;
		case PLAYER_MOVE_E:
			step_coords(&x, &y, DIR_E);
			break;
		case PLAYER_MOVE_S:
			step_coords(&x, &y, DIR_S);
			break;
		case PLAYER_MOVE_W:
			step_coords(&x, &y, DIR_W);
			break;
		case PLAYER_MOVE_PAUSE:
			break;
		}
		if (x < w && y < h && tiles[y*w + x] == TILE_EMPTY) {
			puzzle->player.x = x; puzzle->player.y = y;
		}
	}
	{ // step emitters
		u32 num_emitters = puzzle->num_emitters;
		for (u32 i = 0; i < num_emitters; ++i) {
			struct emitter *e = &puzzle->emitters[i];
			switch (e->type) {
			case EMITTER_FIXED:
				goto fire_emitter;
			case EMITTER_CLOCKWISE:
				e->dir_mask <<= 1;
				e->dir_mask |= e->dir_mask >> 8;
				e->dir_mask &= 0xFF;
				goto fire_emitter;
			case EMITTER_COUNTER_CLOCKWISE:
				e->dir_mask |= e->dir_mask << 8;
				e->dir_mask >>= 1;
				e->dir_mask &= 0xFF;
				goto fire_emitter;
			}
		fire_emitter:
			++e->step;
			if (e->step > e->num_steps) {
				e->step = 1;
			}
			if ((1 << (e->step - 1)) & e->fire_mask) {
				for (u32 d = 0; d < NUM_DIRS; ++d) {
					if (e->dir_mask & (1 << d)) {
						bullets[num_bullets++] = (struct bullet) {
							.x = e->x, .y = e->y,
							.dir = (enum direction)d,
						};
					}
				}
			}
			continue;
		}
	}
	{ // step bullets
		u32 i = 0;
		while (i < num_bullets) {
			struct bullet *b = &bullets[i];
			u32 sx = b->x, sy = b->y;
			step_coords(&b->x, &b->y, b->dir);
			if (b->x >= w || b->y >= h) {
				if (anim_queue != NULL) {
					anim_queue->queue[anim_queue->len++] = (struct anim) {
						.type = ANIMATION_BULLET_MOVE,
						.bullet_move = {
							.sx = sx,   .sy = sy,
							.ex = b->x, .ey = b->y,
							.dir = b->dir,
						},
					};
				}
				bullets[i] = bullets[--num_bullets];
				continue;
			}
			enum tile tile = tiles[b->y*w + b->x];
			if (tile == TILE_EMITTER || tile == TILE_WALL) {
				bullets[i] = bullets[--num_bullets];
				continue;
			}
			if (b->x == puzzle->player.x && b->y == puzzle->player.y) {
				bullets[i] = bullets[--num_bullets];
				// TODO -- player death
				continue;
			}
			if (anim_queue != NULL) {
				anim_queue->queue[anim_queue->len++] = (struct anim) {
					.type = ANIMATION_BULLET_MOVE,
					.bullet_move = {
						.sx = sx,   .sy = sy,
						.ex = b->x, .ey = b->y,
						.dir = b->dir,
					},
				};
			}
			++i;
		}
	}
	puzzle->num_bullets = num_bullets;
}

#define WALL                0x01000000
#define WALL_FROM_N         0x10000000
#define WALL_FROM_E         0x20000000
#define WALL_FROM_S         0x40000000
#define WALL_FROM_W         0x80000000
#define COST_MASK           0x00FFFFFF
#define ALWAYS_BLOCKED_MASK 0x0FFFFFFF

static u32 *map_xyz(struct map *map, u32 x, u32 y, u32 z) {
	return map->data + (z * map->width * map->height) + (y * map->width) + x;
}

struct map generate_map(struct puzzle *puzzle) {
	u32 period = 1;
	for (u32 i = 0; i < puzzle->num_emitters; ++i) {
		struct emitter *e = &puzzle->emitters[i];
		switch(e->type) {
			case EMITTER_FIXED:
				period = lcm_u32(period, e->num_steps);
				break;
			case EMITTER_COUNTER_CLOCKWISE:
			case EMITTER_CLOCKWISE:
				period = lcm_u32(period, e->num_steps);
				period = lcm_u32(period, 8);
				break;
		}
	}
	u32 w = puzzle->width + 2, h = puzzle->height + 2;
	struct map map;
	map.width = w; map.height = h; map.period = period;
	size_t map_size = w * h * period * sizeof(map.data);
	map.data = malloc(map_size);
	memset(map.data, 0, map_size);
	u32 *p = map.data;
	for (u32 k = 0; k < period; ++k) {
		for (u32 j = 0; j < h; ++j) {
			for (u32 i = 0; i < w; ++i, ++p) {
				if (i == 0 || j == 0 || i == w-1 || j == h-1) {
					*p = WALL;
					continue;
				}
				enum tile tile = puzzle->tiles[(j - 1)*(w - 2) + (i - 1)];
				switch (tile) {
				case TILE_EMPTY:
					break;
				case TILE_EMITTER:
					*p = WALL;
					break;
				case TILE_WALL:
					*p = WALL;
					break;
				}
			}
		}
		u32 num_bullets = puzzle->num_bullets;
		struct bullet *b = puzzle->bullets;
		for (u32 i = 0; i < num_bullets; ++i, ++b) {
			u32 x = b->x, y = b->y;
			enum direction dir = b->dir;
			*map_xyz(&map, x+1, y+1, k) |= WALL;
			switch (dir) {
			case DIR_N:
				step_coords(&x, &y, DIR_S);
				*map_xyz(&map, x+1, y+1, k) |= WALL_FROM_N;
				break;
			case DIR_E:
				step_coords(&x, &y, DIR_W);
				*map_xyz(&map, x+1, y+1, k) |= WALL_FROM_E;
				break;
			case DIR_S:
				step_coords(&x, &y, DIR_N);
				*map_xyz(&map, x+1, y+1, k) |= WALL_FROM_S;
				break;
			case DIR_W:
				step_coords(&x, &y, DIR_E);
				*map_xyz(&map, x+1, y+1, k) |= WALL_FROM_W;
				break;
			default:
				break;
			}
		}
		step_puzzle(puzzle, PLAYER_MOVE_PAUSE, NULL);
	}
	return map;
}

void print_map_page(struct map *map, u32 page) {
	u32 w = map->width, h = map->height;
	u32 *p = map->data + (w * h * page);
	for (u32 j = 0; j < h; ++j) {
		char line[MAX_WIDTH + 1] = {};
		for (u32 i = 0; i < w; ++i, ++p) {
			printf("%c:%08X    ", *p & WALL ? '#' : '.', *p);
		}
		printf("%s\n", line);
	}
}

void reset_map(struct map *map) {
	u32 map_size = map->width * map->height * map->period;
	u32 *p = map->data;
	for (u32 i = map_size; i; --i, ++p) {
		*p &= 0xFF000000;
	}
}

void get_furthest_point(struct map *map, u32 x, u32 y) {
	u32 w = map->width, h = map->height, period = map->period;
	struct to_explore {
		u32 x, y, period;
	} *to_explore = malloc(w * h * period * sizeof(*to_explore));
	u32 start = 0, end = 0;
	for (u32 i = 0; i < period; ++i) {
		u32 *p = map_xyz(map, x, y, i);
		if (!(*p & WALL)) {
			to_explore[end++] = (struct to_explore) {
				.x = x, .y = y, .period = i,
			};
			*p |= 1;
		}
	}
	u32 last_cost = 1, last_cost_start = 0;
	while (start < end) {
		struct to_explore this = to_explore[start++];
		u32 this_tile = *map_xyz(map, this.x, this.y, this.period);
		u32 cost = this_tile & COST_MASK;
		if (cost != last_cost) {
			last_cost = cost;
			last_cost_start = start - 1;
		}
		++cost;
		u32 n_x, n_y, n_p = (this.period + period - 1) % period;
		// PLAYER_MOVE_PAUSE
		n_x = this.x; n_y = this.y;
		u32 *p = map_xyz(map, n_x, n_y, n_p);
		if (!(*p & ALWAYS_BLOCKED_MASK)) {
			to_explore[end++] = (struct to_explore) {
				.x = n_x, .y = n_y, .period = n_p,
			};
			*p |= cost;
		}
		// PLAYER_MOVE_N
		if (!(this_tile & WALL_FROM_N)) {
			n_x = this.x; n_y = this.y + 1;
			p = map_xyz(map, n_x, n_y, n_p);
			if (!(*p & ALWAYS_BLOCKED_MASK)) {
				to_explore[end++] = (struct to_explore) {
					.x = n_x, .y = n_y, .period = n_p,
				};
				*p |= cost;
			}
		}
		// PLAYER_MOVE_E
		if (!(this_tile & WALL_FROM_E)) {
			n_x = this.x - 1; n_y = this.y;
			p = map_xyz(map, n_x, n_y, n_p);
			if (!(*p & ALWAYS_BLOCKED_MASK)) {
				to_explore[end++] = (struct to_explore) {
					.x = n_x, .y = n_y, .period = n_p,
				};
				*p |= cost;
			}
		}
		// PLAYER_MOVE_S
		if (!(this_tile & WALL_FROM_S)) {
			n_x = this.x; n_y = this.y - 1;
			p = map_xyz(map, n_x, n_y, n_p);
			if (!(*p & ALWAYS_BLOCKED_MASK)) {
				to_explore[end++] = (struct to_explore) {
					.x = n_x, .y = n_y, .period = n_p,
				};
				*p |= cost;
			}
		}
		// PLAYER_MOVE_W
		if (!(this_tile & WALL_FROM_W)) {
			n_x = this.x + 1; n_y = this.y;
			p = map_xyz(map, n_x, n_y, n_p);
			if (!(*p & ALWAYS_BLOCKED_MASK)) {
				to_explore[end++] = (struct to_explore) {
					.x = n_x, .y = n_y, .period = n_p,
				};
				*p |= cost;
			}
		}
	}
	printf("%u - %u\n", last_cost, end - last_cost_start);
	free(to_explore);
}
