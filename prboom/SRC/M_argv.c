// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: M_argv.c,v 1.1 2000/04/09 18:14:39 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: M_argv.c,v 1.1 2000/04/09 18:14:39 proff_fs Exp $";

#include <string.h>

int    myargc;
char **myargv;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

#ifdef _MSC_VER // proff
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif //_MSC_VER

int M_CheckParm(const char *check)
{
  int i;
  for (i=1; i<myargc; i++)
    if (!strcasecmp(check, myargv[i]))
      return i;
  return 0;
}

//----------------------------------------------------------------------------
//
// $Log: M_argv.c,v $
// Revision 1.1  2000/04/09 18:14:39  proff_fs
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
