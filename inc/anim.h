#ifndef __ANIM_H__
#define __ANIM_H__

#include "types.h"

#define MAX_ANIMS 1024

struct anim_queue {
	u32 len;
	struct anim {
		enum {
			ANIMATION_BULLET_MOVE,
			ANIMATION_BULLET_EXPLODE,
			ANIMATION_PLAYER_MOVE,
			ANIMATION_EMITTER_FIRE,
		} type;
		union {
			struct {
				u32 sx, sy, ex, ey;
				enum direction dir;
			} bullet_move;
		};
	} queue[MAX_ANIMS];
};

#endif
