#include "generator.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "puzzle.h"

typedef s32 (*goal_compare)(struct goal *g1, struct goal *g2);

static void generate(struct puzzle *puzzle,
                     goal_compare is_better,
                     u32 w, u32 h, u32 num_emitters, u32 puzzles_to_try) {
	u32 best_x, best_y;
	struct puzzle best_puzzle;
	struct goal   best_puzzle_goal = {
		.x = 0, .y = 0, .p = 0, .cost = 0, .others = 1000,
	};
	for (u32 i = 0; i < puzzles_to_try; ++i) {
		u32 this_x, this_y;
		struct goal best_goal = {
			.x = 0, .y = 0, .p = 0, .cost = 0, .others = 1000,
		};
		// TODO -- change generator?
		struct puzzle this_puzzle = generate_puzzle(w, h, num_emitters);
		struct map map = generate_map(&this_puzzle);
		for (u32 x = 0; x < w; ++x) {
			for (u32 y = 0; y < h; ++y) {
				reset_map(&map);
				struct goal this_goal = get_furthest_point(&map, x, y);
				if (is_better(&this_goal, &best_goal)) {
					best_goal = this_goal;
					this_x = x; this_y = y;
				}
			}
		}
		if (is_better(&best_goal, &best_puzzle_goal)) {
			best_puzzle      = this_puzzle;
			best_puzzle_goal = best_goal;
			best_x = this_x; best_y = this_y;
		}
		free(map.data);
	}
	best_puzzle.player.x = w  + 1;
	best_puzzle.player.y = h + 1;
	for (u32 i = 0; i < best_puzzle_goal.p; ++i) {
		step_puzzle(&best_puzzle, PLAYER_MOVE_PAUSE, NULL);
	}
	best_puzzle.player.x = best_puzzle_goal.x - 1;
	best_puzzle.player.y = best_puzzle_goal.y - 1;
	best_puzzle.tiles[(best_y - 1) * w + (best_x - 1)] = TILE_GOAL;
	memcpy(puzzle, &best_puzzle, sizeof(*puzzle));
}

static s32 longer_goal(struct goal *g1, struct goal *g2) {
	return g1->cost > g2->cost;
}

void generate_easy_puzzle(struct puzzle *puzzle) {
	generate(puzzle, longer_goal, 5, 5, 3, 100);
}

void generate_medium_puzzle(struct puzzle *puzzle) {
	generate(puzzle, longer_goal, 8, 8, 6, 100);
}

void generate_hard_puzzle(struct puzzle *puzzle) {
	generate(puzzle, longer_goal, 20, 10, 10, 100);
}
