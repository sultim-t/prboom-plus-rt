// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Heads-up Graphics Support for FraggleScript
//
// By SoM, Revised by James Haley
//
//----------------------------------------------------------------------------

#ifndef __HU_FSPIC_H__
#define __HU_FSPIC_H__

typedef struct fspic_s
{
   struct fspic_s *next; // pointer to next pic in list

   int lumpnum;          // number of lump to draw
   int x;                // x coordinate
   int y;                // y coordinate
   boolean draw;         // to draw or not to draw
   boolean translucent;  // false = draw solid, true = translucent
   fixed_t priority;     // relative priority value
   int handle;           // unique id number

   patch_t *data;        // pointer to loaded patch data

} fspic_t;

int  HU_CreateFSPic(int, int, int, boolean, boolean, fixed_t);
void HU_ReInitFSPicList(void);           // called from P_SetupLevel
void HU_DrawFSPics(void);                // called from HU_Drawer
void HU_ModifyFSPic(int, int, int, int);
void HU_ToggleFSPicTL(int, boolean);
void HU_ToggleFSPicVisible(int, boolean);
int  HU_GetFSPicAttribute(int, const char *);
int  HU_GetFSPicHandle(int, int, int);
fixed_t HU_GetLowestPriority(void);
fixed_t HU_GetHighestPriority(void);
fixed_t HU_GetFSPicPriority(int);
void HU_SetFSPicPriority(int, fixed_t);

void HU_FSPicErase(void);

extern fspic_t fspiclist;
extern int lasthandle;

#endif

// EOF
