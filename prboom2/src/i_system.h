/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_system.h,v 1.1 2000/05/04 08:02:58 proff_fs Exp $
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

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#ifdef __GNUG__
#pragma interface
#endif

int I_GetTime_RealTime(void);     /* killough */

unsigned long I_GetRandomTimeSeed(void); /* cphipps */

void I_uSleep(unsigned long usecs);

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer 
 */
const char* I_GetVersionString(char* buf, size_t sz);

/* cphipps - I_SigString
 * Returns a string describing a signal number
 */
const char* I_SigString(char* buf, size_t sz, int signum);

#endif

/*----------------------------------------------------------------------------
 * $Log: i_system.h,v $
 * Revision 1.1  2000/05/04 08:02:58  proff_fs
 * Initial revision
 *
 * Revision 1.10  1999/10/31 16:30:31  cphipps
 * Moved many prototypes to i_main.h
 * Cleaned some dead wood
 * Added a couple of prototypes for new stuff in l_system.c
 *
 *----------------------------------------------------------------------------*/
