/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_video.h,v 1.1 2000/05/04 08:03:00 proff_fs Exp $
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
 *      System specific interface stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __I_VIDEO__
#define __I_VIDEO__

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

void I_PreInitGraphics(void); /* CPhipps - do stuff immediately on start */
void I_SetRes(unsigned int width, unsigned int height); /* CPhipps - set resolution */
void I_InitGraphics (void);
void I_ShutdownGraphics(void);

/* Takes full 8 bit values. */
void I_SetPalette(int pal); /* CPhipps - pass down palette number */

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

void I_ReadScreen (byte* scr);

/* I_StartTic
 * Called by D_DoomLoop,
 * called before processing each tic in a frame.
 * Quick syncronous operations are performed here.
 * Can call D_PostEvent.
 */
void I_StartTic (void);

/* I_StartFrame
 * Called by D_DoomLoop,
 * called before processing any tics in a frame
 * (just after displaying a frame).
 * Time consuming syncronous operations
 * are performed here (joystick reading).
 * Can call D_PostEvent.
 */

void I_StartFrame (void);

extern int use_vsync;  /* killough 2/8/98: controls whether vsync is called */

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: i_video.h,v $
 * Revision 1.1  2000/05/04 08:03:00  proff_fs
 * Initial revision
 *
 * Revision 1.10  2000/03/16 13:27:29  cph
 * Clean up uid stuff
 *
 * Revision 1.9  2000/01/25 21:33:22  cphipps
 * Fix security in case of being setuid
 *
 * Revision 1.8  1999/10/31 16:37:04  cphipps
 * Moved a couple of prototypes from i_system.h, because the functions are in
 * l_video_*.c
 *
 * Revision 1.7  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.6  1999/05/23 09:11:12  cphipps
 * Remove obsolete finctions
 *
 * Revision 1.5  1999/02/01 09:09:54  cphipps
 * Pass palette number to I_SetPalette
 *
 * Revision 1.4  1998/12/31 23:34:14  cphipps
 * Make I_SetPalette take a const*
 *
 * Revision 1.3  1998/12/16 22:35:18  cphipps
 * Added I_SetRes function for setting the resolution
 *
 * Revision 1.2  1998/10/13 11:51:58  cphipps
 * Added I_PreInitGraphics decl
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.4  1998/05/03  22:40:58  killough
 * beautification
 *
 * Revision 1.3  1998/02/09  03:01:51  killough
 * Add vsync for flicker-free blits
 *
 * Revision 1.2  1998/01/26  19:27:01  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:08  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/
