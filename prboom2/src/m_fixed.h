/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *      Fixed point arithemtics, implementation.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __M_FIXED__
#define __M_FIXED__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "doomtype.h"

/*
 * Fixed point, 32bit as 16.16.
 */

#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)

typedef int fixed_t;

/*
 * Absolute Value
 *
 * killough 5/10/98: In djgpp, use inlined assembly for performance
 * killough 9/05/98: better code seems to be gotten from using inlined C
 */

// e6y
// Microsoft and Intel compilers produce stupid and slow code for asm version of D_abs:
//
// mov    DWORD PTR $T49478[esp+12], eax
// mov    eax, DWORD PTR $T49478[esp+12]
//
// Plane abs() generates absolutely the same code as in asm version of D_abs(),
// so we do not need additional implementations for abs() at all.
//
// btw, GCC generates code without nonsenses

#if 0
inline static CONSTFUNC fixed_t D_abs(fixed_t x)
{
  fixed_t _t = (x),_s;
  _s = _t >> (8*sizeof _t-1);
  return (_t^_s)-_s;
}
#else
#define D_abs abs
#endif

/*
 * Fixed Point Multiplication
 */


/* CPhipps - made __inline__ to inline, as specified in the gcc docs
 * Also made const */

inline static CONSTFUNC fixed_t FixedMul(fixed_t a, fixed_t b)
{
  return (fixed_t)((int_64_t) a*b >> FRACBITS);
}

/*
 * Fixed Point Division
 */

static CONSTFUNC fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  return (D_abs(a)>>14) >= D_abs(b) ? ((a^b)>>31) ^ INT_MAX :
    (fixed_t)(((int_64_t) a << FRACBITS) / b);
}

/* CPhipps -
 * FixedMod - returns a % b, guaranteeing 0<=a<b
 * (notice that the C standard for % does not guarantee this)
 */

inline static CONSTFUNC fixed_t FixedMod(fixed_t a, fixed_t b)
{
  if (b & (b-1)) {
    fixed_t r = a % b;
    return ((r<0) ? r+b : r);
  } else
    return (a & (b-1));
}

static CONSTFUNC fixed_t Scale(fixed_t a, fixed_t b, fixed_t c)
{
	return (fixed_t)(((int_64_t)a*b)/c);
}

#endif
