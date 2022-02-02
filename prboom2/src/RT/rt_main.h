#pragma once

#include <assert.h>

#include "RTGL1/RTGL1.h"


#define RG_CHECK(x) assert((x) == RG_SUCCESS)


typedef struct
{
	RgInstance instance;
} rtmain_t;

extern rtmain_t rtmain;
