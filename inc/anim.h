#ifndef __ANIM_H__
#define __ANIM_H__

#include "types.h"

#define MAX_ANIMS 1024

struct anim_queue {
	u32 len;
	struct anim {
		enum {
			ANIMATION_BULLET_MOVE,
			ANIMATION_BULLET_EXPLODE_EDGE,
			ANIMATION_BULLET_EXPLODE_MID,
			ANIMATION_PLAYER_MOVE,
		} type;
		union {
			struct {
				s32 sx, sy, ex, ey;
				enum direction dir;
			} bullet_move;
			struct {
				s32 sx, sy, ex, ey;
				enum direction dir;
				s32 added_explosion;
			} bullet_explode;
			struct {
				u32 sx, sy, ex, ey;
			} player_move;
		};
	} queue[MAX_ANIMS];
};

#define MAX_EXPLOSIONS      1024
#define EXPLOSION_PARTICLES 32

struct explosion_queue {
	u32 num_explosions;
	struct explosion {
		u32 x, y, ticks;
		struct particle {
			f32 dx, dy, speed;
		} particles[EXPLOSION_PARTICLES];
	} explosions[MAX_EXPLOSIONS];
};

#endif
