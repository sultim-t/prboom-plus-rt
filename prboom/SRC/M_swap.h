// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: M_swap.h,v 1.1 2000/04/09 18:13:07 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP__
#define __M_SWAP__

// Endianess handling.
// WAD files are stored little endian.
//
// killough 5/1/98:
// Replaced old code with inlined code which works regardless of endianess.
//

// Swap 16bit, that is, MSB and LSB byte.

// proff 07/04/98: Changed from _MSC_VER to _WIN32 for CYGWIN32 compatibility
#ifdef _WIN32 // proff: This is a hack, I have to look into this
#define SHORT(x) (x)
#define LONG(x) (x)
#else // _WIN32

#ifdef __GNUC__
__inline__
#endif
static short SHORT(short x)
{
  return (((unsigned char *) &x)[1]<< 8) +
          ((unsigned char *) &x)[0];
}

// Swapping 32bit.

#ifdef __GNUC__
__inline__
#endif
static long LONG(long x)
{
  return (((unsigned char *) &x)[3]<<24) +
         (((unsigned char *) &x)[2]<<16) +
         (((unsigned char *) &x)[1]<< 8) +
          ((unsigned char *) &x)[0];
} 

#endif // _MSC_VER

#endif

//----------------------------------------------------------------------------
//
// $Log: M_swap.h,v $
// Revision 1.1  2000/04/09 18:13:07  proff_fs
// Initial revision
//
// Revision 1.3  1998/05/03  23:14:03  killough
// Make endian independent, beautify
//
// Revision 1.2  1998/01/26  19:27:15  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
