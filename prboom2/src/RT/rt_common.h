/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2022 by
 *  Sultim Tsyrendashiev
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#pragma once

#include "RTGL1/RTGL1.h"
#include <assert.h>
#include <lprintf.h>

#define RG_RESOURCES_FOLDER "rt/"

#define RG_MAX_TEXTURE_COUNT 4096

#define assert_always(msg) assert(0 && msg)

#define RG_CHECK(x) do{ \
if (!((x) == RG_RESULT_SUCCESS || (x) == RG_RESULT_SUCCESS_FOUND_MESH || (x) == RG_RESULT_SUCCESS_FOUND_TEXTURE)) \
{ \
  I_Error("RT: %d", (x)); \
}}while (0)


#define RG_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) 

#define RG_VEC3_SET(vec, x, y, z) (vec)[0]=(x);(vec)[1]=(y);(vec)[2]=(z)
#define RG_VEC3_SCALE(vec, f) (vec)[0]*=(f);(vec)[1]*=(f);(vec)[2]*=(f)
#define RG_VEC3_MULTIPLY(vec, x, y, z) (vec)[0]*=(x);(vec)[1]*=(y);(vec)[2]*=(z)
#define RG_VEC3_MULTIPLY_V(vec1, vec2) (vec1)[0]*=(vec2)[0];(vec1)[1]*=(vec2)[1];(vec1)[2]*=(vec2)[2]

#define RG_TRANSFORM_IDENTITY { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 }
#define RG_COLOR_WHITE { 1, 1, 1, 1 }
#define RG_PACKED_COLOR_WHITE 0xFFFFFFFF

#define RG_STR_FLASHLIGHT_HINT "Press \'%s\' to turn flashlight on"

#define RG_WORLD_METALLICITY  0.05f
#define RG_WORLD_ROUGHNESS    0.7f
#define RG_SPRITE_METALLICITY 0.05f
#define RG_SPRITE_ROUGHNESS   0.7f

#define RG_LIGHT_INTENSITY_MULT 0.2f

#define RT_SEPARATE_HUD_SCALE 0
