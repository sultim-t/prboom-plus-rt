/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_setup.h,v 1.1 2000/05/04 08:13:41 proff_fs Exp $
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
 *   Setup a game, startup stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __P_SETUP__
#define __P_SETUP__

#include "p_mobj.h"

#ifdef __GNUG__
#pragma interface
#endif

void P_SetupLevel(int episode, int map, int playermask, skill_t skill);
void P_Init(void);               /* Called by startup code. */

extern const byte *rejectmatrix;   /* for fast sight rejection -  cph - const* */

/* killough 3/1/98: change blockmap from "short" to "long" offsets: */
extern long     *blockmaplump;   /* offsets in blockmap are from here */
extern long     *blockmap;
extern int      bmapwidth;
extern int      bmapheight;      /* in mapblocks */
extern fixed_t  bmaporgx;
extern fixed_t  bmaporgy;        /* origin of block map */
extern mobj_t   **blocklinks;    /* for thing chains */

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: p_setup.h,v $
 * Revision 1.1  2000/05/04 08:13:41  proff_fs
 * Initial revision
 *
 * Revision 1.3  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.2  1998/12/31 20:53:15  cphipps
 * Made reject const
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/03  23:03:31  killough
 * beautification, add external declarations for blockmap
 *
 * Revision 1.2  1998/01/26  19:27:28  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:08  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/
