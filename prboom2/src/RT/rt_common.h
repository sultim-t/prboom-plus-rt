#pragma once

#include "RTGL1/RTGL1.h"
#include <assert.h>
#include <lprintf.h>

#define RG_RESOURCES_FOLDER "ovrd/"

#define RG_MAX_TEXTURE_COUNT 4096

#define assert_always(msg) assert(0 && msg)

#define RG_CHECK(x) do{if ((x) != RG_SUCCESS){I_Error("RT: %d", (x));}}while(0)
#define RG_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) 

#define RG_VEC3_SET(vec, x, y, z) (vec)[0]=(x);(vec)[1]=(y);(vec)[2]=(z)
#define RG_VEC3_SCALE(vec, f) (vec)[0]*=(f);(vec)[1]*=(f);(vec)[2]*=(f)
#define RG_VEC3_MULTIPLY(vec, x, y, z) (vec)[0]*=(x);(vec)[1]*=(y);(vec)[2]*=(z)
#define RG_VEC3_MULTIPLY_V(vec1, vec2) (vec1)[0]*=(vec2)[0];(vec1)[1]*=(vec2)[1];(vec1)[2]*=(vec2)[2]

#define RG_TRANSFORM_IDENTITY { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 }
#define RG_COLOR_WHITE { 1, 1, 1, 1 }

#define RG_STR_FLASHLIGHT_HINT "Press \'%s\' to turn flashlight on"

#define RG_WORLD_METALLICITY  0.05f
#define RG_WORLD_ROUGHNESS    0.7f
#define RG_SPRITE_METALLICITY 0.05f
#define RG_SPRITE_ROUGHNESS   0.7f


#define RG_SKY_REUSE_BUG_HACK 1
