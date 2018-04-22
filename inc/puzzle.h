#ifndef __PUZZLE_H__
#define __PUZZLE_H__

#include "types.h"
#include "anim.h"

#define MAX(x, y) (x > y ? x : y)
#define MAX_EMITTERS 32
#define MAX_WIDTH    32
#define MAX_HEIGHT   32
#define MAX_SIZE     (MAX_WIDTH * MAX_HEIGHT)
#define MAX_BULLETS  (MAX_EMITTERS * 8 * MAX(MAX_WIDTH, MAX_HEIGHT))
#define MAX_SOLUTION_LENGTH 1024

#define NUM_DIRS 8

enum player_move {
	PLAYER_MOVE_N,
	PLAYER_MOVE_E,
	PLAYER_MOVE_S,
	PLAYER_MOVE_W,
	PLAYER_MOVE_PAUSE,
};

enum move_response {
	MOVE_RESPONSE_NONE,
	MOVE_RESPONSE_DEATH,
	MOVE_RESPONSE_VICTORY,
};

struct puzzle {
	u32 width, height;
	struct {
		u32 x, y;
	} player;
	u32 num_emitters;
	struct emitter {
		enum {
			EMITTER_FIXED,
			EMITTER_CLOCKWISE,
			EMITTER_COUNTER_CLOCKWISE,
		} type;
		u32 x, y;
		u32 dir_mask;
		u32 step, num_steps, fire_mask;
	} emitters[MAX_EMITTERS];
	u32 num_bullets;
	struct bullet {
		u32 x, y;
		enum direction dir;
	} bullets[MAX_BULLETS];
	enum tile {
		TILE_EMPTY,
		TILE_EMITTER,
		TILE_WALL,
		TILE_GOAL,
	} tiles[MAX_SIZE];
};

void print_puzzle(struct puzzle *puzzle);
struct puzzle generate_puzzle(u32 width, u32 height, u32 num_emitters);
enum move_response step_puzzle(struct puzzle *puzzle,
                               enum player_move player_move,
                               struct anim_queue *anim_queue);

// XXX

struct map {
	u32 width, height, period;
	u32 *data;
};
struct map generate_map(struct puzzle *puzzle);
void reset_map(struct map *map);
void print_map_page(struct map *map, u32 page);
struct goal {
	u32 x, y, p, cost, others;
};
struct goal get_furthest_point(struct map *map, u32 x, u32 y);

struct solution {
	u32 len;
	enum player_move moves[MAX_SOLUTION_LENGTH];
};

void solve_puzzle(struct solution *solution, struct puzzle *puzzle);

#endif
