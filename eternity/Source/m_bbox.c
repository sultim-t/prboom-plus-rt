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
//      Main loop menu stuff.
//      Random number LUT.
//      Default Config File.
//      PCX Screenshots.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: m_bbox.c,v 1.4 1998/05/05 19:55:56 phares Exp $";

#include "m_bbox.h"

void M_ClearBox (fixed_t *box)
{
  box[BOXTOP] = box[BOXRIGHT] = D_MININT;
  box[BOXBOTTOM] = box[BOXLEFT] = D_MAXINT;
}

void M_AddToBox(fixed_t* box,fixed_t x,fixed_t y)
{
  if (x<box[BOXLEFT])
    box[BOXLEFT] = x;
  else
    if (x>box[BOXRIGHT])
      box[BOXRIGHT] = x;

  if (y<box[BOXBOTTOM])
    box[BOXBOTTOM] = y;
  else
    if (y>box[BOXTOP])
      box[BOXTOP] = y;
}

//----------------------------------------------------------------------------
//
// $Log: m_bbox.c,v $
// Revision 1.4  1998/05/05  19:55:56  phares
// Formatting and Doc changes
//
// Revision 1.3  1998/05/03  22:52:12  killough
// beautification
//
// Revision 1.2  1998/01/26  19:23:42  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
