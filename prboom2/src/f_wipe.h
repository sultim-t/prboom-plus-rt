/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: f_wipe.h,v 1.1 2000/05/04 08:01:33 proff_fs Exp $
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
 *      Mission start screen wipe/melt, special effects.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

/*
 * SCREEN WIPE PACKAGE
 */

int wipe_ScreenWipe (int x, int y, int width, int height, int ticks);
int wipe_StartScreen(int x, int y, int width, int height);
int wipe_EndScreen  (int x, int y, int width, int height);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: f_wipe.h,v $
 * Revision 1.1  2000/05/04 08:01:33  proff_fs
 * Initial revision
 *
 * Revision 1.3  1999/10/12 13:00:56  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.2  1999/08/26 21:36:36  cphipps
 * Removed old alternate screen wipes, unused
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/03  22:11:27  killough
 * beautification
 *
 * Revision 1.2  1998/01/26  19:26:49  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:54  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/
