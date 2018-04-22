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

#endif
