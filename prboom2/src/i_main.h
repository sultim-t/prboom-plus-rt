/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_main.h,v 1.1 2000/05/04 08:02:55 proff_fs Exp $
 *
 *  Parts of the Boom i_system.h and original linuxdoom i_system.h
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
 *      General system functions. Signal related stuff, exit function
 *      prototypes, and programmable Doom clock.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __I_MAIN__
#define __I_MAIN__

void I_Init(void);
void I_SafeExit(int rc);

extern int broken_pipe;
extern int (*I_GetTime)(void);

#ifdef SECURE_UID
extern uid_t stored_euid; /* UID that the SVGALib I_InitGraphics switches to before vga_init() */
#endif

#endif

/*-----------------------------------------------------------------------------
 * $Log: i_main.h,v $
 * Revision 1.1  2000/05/04 08:02:55  proff_fs
 * Initial revision
 *
 * Revision 1.4  2000/05/01 17:50:34  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.3  2000/03/16 13:27:29  cph
 * Clean up uid stuff
 *
 * Revision 1.2  1999/11/01 17:11:43  cphipps
 * Added I_Init for d_main.c
 *
 * Revision 1.1  1999/11/01 07:27:20  cphipps
 * Added new header
 *
 *-----------------------------------------------------------------------------*/
