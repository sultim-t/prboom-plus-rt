// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------

#ifndef __M_FIXED__
#define __M_FIXED__

#include "d_keywds.h" // haleyjd 05/22/02
#include "i_system.h"

//
// Fixed point, 32bit as 16.16.
//

#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)

typedef int fixed_t;

//
// Absolute Value
//

// killough 5/10/98: In djgpp, use inlined assembly for performance
// killough 9/05/98: better code seems to be gotten from using inlined C

// haleyjd 04/24/02: renamed to D_abs, added Win32 version

#if defined(__GNUC__)
  #define D_abs(x) ({fixed_t _t = (x), _s = _t >> (8*sizeof _t-1); (_t^_s)-_s;})
#elif defined(_MSC_VER)
d_inline static int D_abs(int x)
{
   __asm
   {
      mov eax,x
      cdq
      xor eax,edx
      sub eax,edx
   }
}
#else
d_inline static int D_abs(int x)
{
   fixed_t _t = x, _s;
   _s = _t >> (8*sizeof _t-1);
   return (_t^_s) - _s;
}
#endif // DJGPP

// haleyjd 11/30/02: removed asm const modifiers due to continued
// problems

//
// Fixed Point Multiplication
//

#ifdef DJGPP

// killough 5/10/98: In djgpp, use inlined assembly for performance

d_inline static fixed_t FixedMul(fixed_t a, fixed_t b)
{
  fixed_t result;


  /* Julian 6/7/2001

        1) turned from asm to __asm__ as encouraged in docs for .h
        2) made asm block const since output depends only on values
        3) cleansed the constraints (now much simpler)
        
	haleyjd 02/16/02: 
	fix for Joel -- GCC 3.03 seems to not like the const
  */


  __asm__ (

  "  imull %2 \n"
  "  shrdl $16,%%edx,%0"
  : "=a" (result)           // eax is always the result
  : "0"  (a),               // eax is also first operand
    "rm" (b)                // second operand can be mem or reg
  : "%edx", "%cc"           // edx and condition codes clobbered
  );

  return result;
}

#else // DJGPP

d_inline static fixed_t FixedMul(fixed_t a, fixed_t b)
{
  return (fixed_t)((Long64) a*b >> FRACBITS);
}

#endif // DJGPP

//
// Fixed Point Division
//

#ifdef DJGPP

// killough 5/10/98: In djgpp, use inlined assembly for performance
// killough 9/5/98: optimized to reduce the number of branches

d_inline static fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  if (D_abs(a) >> 14 < D_abs(b))
    {
      fixed_t result;


      /* Julian 6/7/2001

            1) turned from asm to __asm__ as encouraged in docs for .h
            2) made asm block const since output depends only on values
            3) cleansed the constraints (now much simpler)
            4) removed edx from clobbered regs as it is used as an input
               (was useless on old gcc and error maker with modern gcc)
	    
	    haleyjd 02/16/02: 
	    fix for Joel -- GCC 3.03 doesn't like the const
      */


      __asm__ (

      " idivl %3"
      : "=a" (result)
      : "0" (a<<16),
        "d" (a>>16),
        "rm" (b)
      : "%cc"
      );

      return result;
    }
  return ((a^b)>>31) ^ D_MAXINT;
}

#else // DJGPP

d_inline static fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  return (D_abs(a)>>14) >= D_abs(b) ? ((a^b)>>31) ^ D_MAXINT :
    (fixed_t)(((Long64) a << FRACBITS) / b);
}

#endif // DJGPP

#endif

//----------------------------------------------------------------------------
//
// $Log: m_fixed.h,v $
// Revision 1.5  1998/05/10  23:42:22  killough
// Add inline assembly for djgpp (x86) target
//
// Revision 1.4  1998/04/27  01:53:37  killough
// Make gcc extensions #ifdef'ed
//
// Revision 1.3  1998/02/02  13:30:35  killough
// move fixed point arith funcs to m_fixed.h
//
// Revision 1.2  1998/01/26  19:27:09  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:53  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------

