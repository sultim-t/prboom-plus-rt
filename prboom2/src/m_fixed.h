/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_fixed.h,v 1.1 2000/05/04 08:09:03 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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

#ifdef _MSC_VER
__inline static int D_abs(int x)
{
    if (x>0)
        return x;
    else
        return -x;
}
#endif

#ifdef I386
#define D_abs(x) ({fixed_t _t = (x), _s = _t >> (8*sizeof _t-1); (_t^_s)-_s;})
#endif /* I386 */

/*
 * Fixed Point Multiplication
 */

#ifdef I386

/* killough 5/10/98: In djgpp, use inlined assembly for performance
 * CPhipps - made __inline__ to inline, as specified in the gcc docs
 * Also made const */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const fixed_t FixedMul(fixed_t a, fixed_t b)
{
  fixed_t result;
  int dummy;

  asm("  imull %3 ;"
      "  shrdl $16,%1,%0 ;"
      : "=a" (result),          /* eax is always the result */
        "=d" (dummy)		/* cphipps - fix compile problem with gcc-2.95.1
				   edx is clobbered, but it might be an input */
      : "0" (a),                /* eax is also first operand */
        "r" (b)                 /* second operand could be mem or reg before,
				   but gcc compile problems mean i can only us reg */
      : "%cc"                   /* edx and condition codes clobbered */
      );

  return result;
}

#else /* I386 */

/* CPhipps - made __inline__ to inline, as specified in the gcc docs
 * Also made const */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const fixed_t FixedMul(fixed_t a, fixed_t b)
{
  return (fixed_t)((int_64_t) a*b >> FRACBITS);
}

#endif /* I386 */

/*
 * Fixed Point Division
 */

#ifdef I386

/* killough 5/10/98: In djgpp, use inlined assembly for performance
 * killough 9/5/98: optimized to reduce the number of branches
 * CPhipps - made __inline__ to inline, as specified in the gcc docs
 * Also made const */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  if (D_abs(a) >> 14 < D_abs(b))
    {
      fixed_t result;
      int dummy;
      asm(" idivl %4 ;"
	  : "=a" (result),
	    "=d" (dummy)  /* cphipps - fix compile problems with gcc 2.95.1
			     edx is clobbered, but also an input */
	  : "0" (a<<16),
	    "1" (a>>16),
	    "r" (b)
	  : "%cc"
	  );
      return result;
    }
  return ((a^b)>>31) ^ INT_MAX;
}

#else /* I386 */
/* CPhipps - made __inline__ to inline, as specified in the gcc docs
 * Also made const */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  return (D_abs(a)>>14) >= D_abs(b) ? ((a^b)>>31) ^ INT_MAX :
    (fixed_t)(((int_64_t) a << FRACBITS) / b);
}

#endif /* I386 */

/* CPhipps - 
 * FixedMod - returns a % b, guaranteeing 0<=a<b
 * (notice that the C standard for % does not guarantee this)
 */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const fixed_t FixedMod(fixed_t a, fixed_t b)
{
  if (b & (b-1)) {
    fixed_t r = a % b;
    return ((r<0) ? r+b : r);  
  } else
    return (a & (b-1));
}

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: m_fixed.h,v $
 * Revision 1.1  2000/05/04 08:09:03  proff_fs
 * Initial revision
 *
 * Revision 1.13  2000/05/01 17:50:35  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.12  2000/05/01 15:16:47  Proff
 * added __inline for VisualC
 *
 * Revision 1.11  2000/05/01 14:37:33  Proff
 * changed abs to D_abs
 *
 * Revision 1.10  1999/10/31 10:23:48  cphipps
 * Fixed #include to get just the data types
 *
 * Revision 1.9  1999/10/17 08:50:06  cphipps
 * Added FixedMod
 *
 * Revision 1.8  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.7  1999/10/09 16:20:18  cphipps
 * Fixed inline asm() to conform to gcc 2.95.1's stricter checking
 *
 * Revision 1.6  1999/06/08 17:27:48  cphipps
 * Change long long references to int_64_t's
 *
 * Revision 1.5  1999/01/25 15:47:05  cphipps
 * Use newer limit macros from limits.h instead of depreciated values.h macros
 *
 * Revision 1.4  1999/01/13 07:57:35  cphipps
 * Remove non-gnu compiler global subs
 *
 * Revision 1.3  1998/12/21 21:51:22  cphipps
 * Imported MBF m_fixed.h
 * Added const keywords
 * Changed __inline__'s to inline's, as guided by gcc's docs
 * Changed DJGPP defines to I386's
 *
 * Revision 1.5  1998/05/10  23:42:22  killough
 * Add inline assembly for djgpp (x86) target
 *
 * Revision 1.4  1998/04/27  01:53:37  killough
 * Make gcc extensions #ifdef'ed
 *
 * Revision 1.3  1998/02/02  13:30:35  killough
 * move fixed point arith funcs to m_fixed.h
 *
 * Revision 1.2  1998/01/26  19:27:09  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:53  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/

