#pragma once

#include "RTGL1/RTGL1.h"
#include <assert.h>

#define RG_MAX_TEXTURE_COUNT 4096

#define assert_always(msg) assert(0 && msg)

#define RG_CHECK(x) {if ((x) != RG_SUCCESS){I_Error("RT: %d", (x));}}
#define RG_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) 

#define RG_VEC3_SET(vec, x, y, z) (vec)[0]=(x);(vec)[1]=(y);(vec)[2]=(z)
#define RG_VEC3_MULTIPLY(vec, x, y, z) (vec)[0]*=(x);(vec)[1]*=(y);(vec)[2]*=(z)
#define RG_VEC3_MULTIPLY_V(vec1, vec2) (vec1)[0]*=(vec2)[0];(vec1)[1]*=(vec2)[1];(vec1)[2]*=(vec2)[2]

#define RG_TRANSFORM_IDENTITY { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 }
#define RG_COLOR_WHITE { 1, 1, 1, 1 }


#define RG_SKY_REUSE_BUG_HACK 1
