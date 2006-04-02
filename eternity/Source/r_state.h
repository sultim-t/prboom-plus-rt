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
//      Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------

#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "p_chase.h"
#include "r_data.h"

//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t *textureheight;

// needed for pre rendering (fracs)
extern fixed_t *spritewidth;
extern fixed_t *spriteoffset;
extern fixed_t *spritetopoffset;

extern lighttable_t **colormaps;          // killough 3/20/98, 4/4/98
extern lighttable_t *fullcolormap;        // killough 3/20/98

extern int viewwidth;
extern int scaledviewwidth;
extern int viewheight;
extern int scaledviewheight;              // killough 11/98

extern int firstflat;

// for global animation
extern int *flattranslation;    
extern int *texturetranslation; 

// Sprite....
extern int firstspritelump;
extern int lastspritelump;
extern int numspritelumps;

//
// Lookup tables for map data.
//
extern int              numsprites;
extern spritedef_t      *sprites;

extern int              numvertexes;
extern vertex_t         *vertexes;

extern int              numsegs;
extern seg_t            *segs;

extern int              numsectors;
extern sector_t         *sectors;

extern int              numsubsectors;
extern subsector_t      *subsectors;

extern int              numnodes;
extern node_t           *nodes;

extern int              numlines;
extern line_t           *lines;

extern int              numsides;
extern side_t           *sides;

        // sf: for scripting
extern int              numthings;

//
// POV data.
//
extern fixed_t          viewx;
extern fixed_t          viewy;
extern fixed_t          viewz;
extern angle_t          viewangle;
extern player_t         *viewplayer;
extern camera_t         *viewcamera;
extern angle_t          clipangle;
extern int              viewangletox[FINEANGLES/2];
extern angle_t          xtoviewangle[MAX_SCREENWIDTH+1];  // killough 2/8/98
extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;

// angle to line origin
extern int              rw_angle1;

// Segs count?
extern int              sscount;

extern visplane_t       *floorplane;
extern visplane_t       *ceilingplane;

#endif

//----------------------------------------------------------------------------
//
// $Log: r_state.h,v $
// Revision 1.6  1998/05/01  14:49:12  killough
// beautification
//
// Revision 1.5  1998/04/06  04:40:54  killough
// Make colormaps completely dynamic
//
// Revision 1.4  1998/03/23  03:39:48  killough
// Add support for arbitrary number of colormaps
//
// Revision 1.3  1998/02/09  03:23:56  killough
// Change array decl to use MAX screen width/height
//
// Revision 1.2  1998/01/26  19:27:47  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
