//-----------------------------------------------------------------------------
//
// $Id: Version.h,v 1.1 2000/04/09 18:00:15 proff_fs Exp $
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
//-----------------------------------------------------------------------------


#ifndef __DOOMVERSION__
#define __DOOMVERSION__

#include "z_zone.h"  /* memory allocation wrappers -- killough */

// DOOM version
enum { VERSION =  250 };

extern const char version_date[];

#endif

//----------------------------------------------------------------------------
//
// $Log: Version.h,v $
// Revision 1.1  2000/04/09 18:00:15  proff_fs
// Initial revision
//
// Revision 1.5  1998/11/20  23:16:19  phares
// New Fireline fix
//
// Revision 1.4  1998/06/21  09:11:05  jim
// updating version number
//
// Revision 1.3  1998/04/20  13:29:58  jim
// Update BOOM version, BOOM.TXT
//
// Revision 1.2  1998/02/02  17:36:25  killough
// fix comments
//
//
// Revision 1.1  1998/02/02  13:22:00  killough
// version information files
//
//----------------------------------------------------------------------------
