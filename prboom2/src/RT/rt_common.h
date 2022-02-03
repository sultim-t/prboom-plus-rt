#pragma once

#include "RTGL1/RTGL1.h"
#include <assert.h>

#define RG_MAX_TEXTURE_COUNT 4096

#define assert_always(msg) assert(0 && msg)

#define RG_CHECK(x) assert((x) == RG_SUCCESS)
#define RG_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) 
#define RG_TRANSFORM_IDENTITY { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 }
#define RG_COLOR_WHITE { 1, 1, 1, 1 }
