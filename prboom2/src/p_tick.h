/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_tick.h,v 1.1 2000/05/04 08:15:01 proff_fs Exp $
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
 *  Core thinker processing prototypes.
 *-----------------------------------------------------------------------------*/

#ifndef __P_TICK__
#define __P_TICK__

#include "d_think.h"

#ifdef __GNUG__
#pragma interface
#endif

/* Called by C_Ticker, can call G_PlayerExited.
 * Carries out all thinking of monsters and players. */

void P_Ticker(void);

extern thinker_t thinkercap;  /* Both the head and tail of the thinker list */

void P_InitThinkers(void);
void P_AddThinker(thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);
void P_SetTarget(mobj_t **mop, mobj_t *targ);   /* killough 11/98 */

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: p_tick.h,v $
 * Revision 1.1  2000/05/04 08:15:01  proff_fs
 * Initial revision
 *
 * Revision 1.4  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.3  1999/06/20 20:00:45  cphipps
 * Remove P_RemoveThinkerDelayed prototype (it's use in p_tick.c was changed)
 *
 * Revision 1.2  1999/02/04 15:16:57  cphipps
 * Add P_SetTarget prototype for MBF-like reference counting
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.5  1998/05/15  00:36:22  killough
 * Remove unnecessary crash hack
 *
 * Revision 1.4  1998/05/13  22:58:01  killough
 * Restore Doom bug compatibility for demos
 *
 * Revision 1.3  1998/05/03  22:49:29  killough
 * Add external declarations, formerly in p_local.h
 *
 * Revision 1.2  1998/01/26  19:27:31  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:08  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/
