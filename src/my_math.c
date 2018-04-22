#include "my_math.h"

#include <stdio.h>

#include "types.h"

u32 hcf_u32(u32 a, u32 b) {
	if (b > a) {
		u32 tmp = b; b = a; a = tmp;
	}
	while (b) {
		a %= b;
		u32 tmp = b; b = a; a = tmp;
	}
	return a;
}

u32 lcm_u32(u32 a, u32 b) {
	return a * b / hcf_u32(a, b);
}
