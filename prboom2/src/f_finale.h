/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: f_finale.h,v 1.1 2000/05/04 08:01:31 proff_fs Exp $
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
 *   Related to f_finale.c, which is called at the end of a level
 *    
 *-----------------------------------------------------------------------------*/


#ifndef __F_FINALE__
#define __F_FINALE__

#include "doomtype.h"
#include "d_event.h"

/*
 * FINALE
 */

/* Called by main loop. */
boolean F_Responder (event_t* ev);

/* Called by main loop. */
void F_Ticker (void);

/* Called by main loop. */
void F_Drawer (void);

void F_StartFinale (void);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: f_finale.h,v $
 * Revision 1.1  2000/05/04 08:01:31  proff_fs
 * Initial revision
 *
 * Revision 1.2  1999/10/12 13:00:56  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/04  21:58:52  thldrmn
 * commenting and reformatting
 *
 * Revision 1.2  1998/01/26  19:26:47  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:54  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
