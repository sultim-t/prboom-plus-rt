/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_argv.c,v 1.1 2000/05/04 08:08:44 proff_fs Exp $
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
 *  Some argument handling.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_argv.c,v 1.1 2000/05/04 08:08:44 proff_fs Exp $";

#include <string.h>
// CPhipps - include the correct header
#include "doomtype.h"
#include "m_argv.h"

int    myargc;
const char * const * myargv; // CPhipps - not sure if ANSI C allows you to 
// modify contents of argv, but I can't imagine it does.

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

int M_CheckParm(const char *check)
{
  signed int i = myargc;
  while (--i>0)
    if (!strcasecmp(check, myargv[i]))
      return i;
  return 0;
}

//----------------------------------------------------------------------------
//
// $Log: m_argv.c,v $
// Revision 1.1  2000/05/04 08:08:44  proff_fs
// Initial revision
//
// Revision 1.5  2000/05/01 17:50:35  Proff
// made changes to compile with VisualC and SDL
//
// Revision 1.4  1999/10/12 13:01:12  cphipps
// Changed header to GPL
//
// Revision 1.3  1999/02/23 09:53:44  cphipps
// Scan arguments in reverse, so later arguments override earlier
//
// Revision 1.2  1998/10/16 22:11:10  cphipps
// Made myargv a const char* const * as argv is
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.5  1998/05/03  22:51:40  killough
// beautification
//
// Revision 1.4  1998/05/01  14:26:14  killough
// beautification
//
// Revision 1.3  1998/05/01  14:23:29  killough
// beautification
//
// Revision 1.2  1998/01/26  19:23:40  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
