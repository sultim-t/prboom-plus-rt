// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
//      Refresh module, drawing LineSegs from BSP.
//
//-----------------------------------------------------------------------------

#ifndef __R_SEGS__
#define __R_SEGS__

void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2);
void R_StoreWallRange(int start, int stop);

#ifdef R_PORTALS
// SoM 3/14/2005: Seg rejection for portals.
boolean R_ClipSeg(int *start, int *stop);
#endif

fixed_t R_PointToDist2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);

#endif

//----------------------------------------------------------------------------
//
// $Log: r_segs.h,v $
// Revision 1.5  1998/05/03  23:02:40  killough
// beautification, add R_StoreWallRange() decl
//
// Revision 1.4  1998/04/27  02:01:28  killough
// Program beautification
//
// Revision 1.3  1998/03/02  11:53:29  killough
// add scrolling walls
//
// Revision 1.2  1998/01/26  19:27:44  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
