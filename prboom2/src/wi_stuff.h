/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: wi_stuff.h,v 1.1 2000/05/04 08:19:18 proff_fs Exp $
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
 *  Intermission screens.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __WI_STUFF__
#define __WI_STUFF__

//#include "v_video.h"

#include "doomdef.h"

// States for the intermission

typedef enum
{
  NoState = -1,
  StatCount,
  ShowNextLoc

} stateenum_t;

// Called by main loop, animate the intermission.
void WI_Ticker (void);

// Called by main loop,
// draws the intermission directly into the screen buffer.
void WI_Drawer (void);

// Setup for an intermission screen.
void WI_Start(wbstartstruct_t*   wbstartstruct);

// Release intermission screen memory
void WI_End(void);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: wi_stuff.h,v $
 * Revision 1.1  2000/05/04 08:19:18  proff_fs
 * Initial revision
 *
 * Revision 1.3  1999/10/12 13:01:16  cphipps
 * Changed header to GPL
 *
 * Revision 1.2  1999/09/01 19:41:51  cphipps
 * Added WI_End prototype so g_game.c can release WI_* memory at an appropriate time
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/04  21:36:12  thldrmn
 * commenting and reformatting
 *
 * Revision 1.2  1998/01/26  19:28:03  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:05  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
