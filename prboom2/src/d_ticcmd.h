/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: d_ticcmd.h,v 1.1 2000/05/04 08:00:56 proff_fs Exp $
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
 *	System specific interface stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

/* The data sampled per tick (single player)
 * and transmitted to other peers (multiplayer).
 * Mainly movements/button commands per game tick,
 * plus a checksum for internal state consistency.
 * CPhipps - explicitely signed the elements, since they have to be signed to work right
 */
typedef struct
{
  signed char	forwardmove;	/* *2048 for move       */
  signed char	sidemove;	/* *2048 for move       */
  signed short  angleturn;	/* <<16 for angle delta */
  short	consistancy;	        /* checks for net game  */ 
  byte	chatchar;
  byte	buttons;
} ticcmd_t;

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: d_ticcmd.h,v $
 * Revision 1.1  2000/05/04 08:00:56  proff_fs
 * Initial revision
 *
 * Revision 1.3  1999/10/12 13:00:56  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.2  1999/01/13 19:06:55  cphipps
 * Make explicit the signedness of structure members
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.2  1998/01/26  19:26:36  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:08  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
