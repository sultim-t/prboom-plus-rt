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
//      Sky rendering.
//
//-----------------------------------------------------------------------------

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"
#include "doomtype.h"

// SKY, store the number for name.
#define SKYFLATNAME  "F_SKY1"
// haleyjd: hexen-style skies
#define SKY2FLATNAME "F_SKY2"

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

// haleyjd: hashed sky texture information

#define NUMSKYCHAINS 13

typedef struct skytexture_s
{
   int     texturenum;        // hash key
   int     height;            // true height of texture
   fixed_t texturemid;        // vertical offset
   struct skytexture_s *next; // next skytexture in hash chain
} skytexture_t;

// the sky texture hashtable
extern skytexture_t *skytextures[NUMSKYCHAINS];

extern int skytexture;
extern int sky2texture; // haleyjd
extern int stretchsky;

// init sky at start of level
void R_StartSky();

// sky texture info hashing functions
skytexture_t *R_GetSkyTexture(int);
void R_ClearSkyTextures(void);

#endif

//----------------------------------------------------------------------------
//
// $Log: r_sky.h,v $
// Revision 1.4  1998/05/03  22:56:25  killough
// Add m_fixed.h #include
//
// Revision 1.3  1998/05/01  14:15:29  killough
// beautification
//
// Revision 1.2  1998/01/26  19:27:46  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
