/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: doomdef.c,v 1.1 2000/05/04 08:01:00 proff_fs Exp $
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
 *  DoomDef - basic defines for DOOM, e.g. Version, game mode
 *   and skill level, and display parameters.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: doomdef.c,v 1.1 2000/05/04 08:01:00 proff_fs Exp $";

#ifdef __GNUG__
#pragma implementation "doomdef.h"
#endif

#include "doomdef.h"

// Location for any defines turned variables.
// None.

#ifdef HIGHRES
// proff 08/17/98: Changed for high-res
int SCREENWIDTH=320;
int SCREENHEIGHT=200;
#endif

//----------------------------------------------------------------------------
//
// $Log: doomdef.c,v $
// Revision 1.1  2000/05/04 08:01:00  proff_fs
// Initial revision
//
// Revision 1.3  1999/10/12 13:01:09  cphipps
// Changed header to GPL
//
// Revision 1.2  1998/11/16 18:54:39  cphipps
// Added hi-res vars
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.3  1998/05/03  22:40:02  killough
// beautification
//
// Revision 1.2  1998/01/26  19:23:09  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------

