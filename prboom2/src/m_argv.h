/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_argv.h,v 1.1 2000/05/04 08:08:47 proff_fs Exp $
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
 *  Argument handling.
 *    
 *-----------------------------------------------------------------------------*/


#ifndef __M_ARGV__
#define __M_ARGV__

/*
 * MISC
 */
extern int  myargc;
extern const char * const * myargv; /* CPhipps - const * const * */

/* Returns the position of the given parameter in the arg list (0 if not found). */
int M_CheckParm(const char *check);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: m_argv.h,v $
 * Revision 1.1  2000/05/04 08:08:47  proff_fs
 * Initial revision
 *
 * Revision 1.3  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.2  1998/10/16 22:10:36  cphipps
 * Make myargv a const char* const *, for compatibility with argv
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/01  14:26:18  killough
 * beautification
 *
 * Revision 1.2  1998/01/26  19:27:05  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:58  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
