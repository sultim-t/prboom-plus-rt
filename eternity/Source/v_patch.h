// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley
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
//  Functions to draw patches (by post)
//
//-----------------------------------------------------------------------------

#ifndef V_PATCH_H
#define V_PATCH_H

#include "r_data.h"
#include "doomtype.h"

typedef struct PatchInfo_s
{
   patch_t *patch;
   int x, y;        // screen coordinates
   boolean flipped; // flipped?
   int drawstyle;   // drawing style (normal, tr, tl, trtl)
} PatchInfo;

typedef struct VBuffer_s
{
   byte *data;
   int width;
   int height;
   int pitch;

   void (*PatchWrapper)(PatchInfo *, struct VBuffer_s *);
   void (*BlockDrawer)(int, int, struct VBuffer_s *, int, int, byte *);
   void (*TileBlock64)(struct VBuffer_s *, byte *);

   int  *xlookup;
   int  *ylookup;
   fixed_t ixscale;
   fixed_t iyscale;
} VBuffer;

enum
{
   PSTYLE_NORMAL,
   PSTYLE_TLATED,
   PSTYLE_TRANSLUC,
   PSTYLE_TLTRANSLUC,
   PSTYLE_ADD,
   PSTYLE_TLADD,
};

void V_SetPatchColrng(byte *colrng);
void V_SetPatchTL(unsigned int *fg, unsigned int *bg);

enum
{
   DRAWTYPE_UNSCALED,
   DRAWTYPE_2XSCALED,
   DRAWTYPE_GENSCALED,
};

void V_SetupBufferFuncs(VBuffer *buffer, int drawtype);

#endif

// EOF
