/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_bbox.h,v 1.2 2000/05/06 08:49:55 cph Exp $
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
 *    Simple bounding box datatype and functions.
 *    
 *-----------------------------------------------------------------------------*/


#ifndef __M_BBOX__
#define __M_BBOX__

#include <limits.h>
#include "m_fixed.h"

/* Bounding box coordinate storage. */
enum
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT
};  /* bbox coordinates */

/* Bounding box functions. */

void M_ClearBox(fixed_t* box);

void M_AddToBox(fixed_t* box,fixed_t x,fixed_t y);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: m_bbox.h,v $
 * Revision 1.2  2000/05/06 08:49:55  cph
 * Minor header file fixing
 *
 * Revision 1.1.1.1  2000/05/04 08:08:47  proff_fs
 * initial login on sourceforge as prboom2
 *
 * Revision 1.3  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.2  1999/01/27 16:03:48  cphipps
 * Use newer limits.h instead of depreciated values.h
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/05  19:55:58  phares
 * Formatting and Doc changes
 *
 * Revision 1.2  1998/01/26  19:27:06  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:58  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
