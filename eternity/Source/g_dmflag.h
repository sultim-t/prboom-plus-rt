// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Deathmatch Flags
//
// By James Haley
//
//---------------------------------------------------------------------------

#ifndef __G_DMFLAG_H__
#define __G_DMFLAG_H__

#include "doomtype.h"

enum
{
   DM_ITEMRESPAWN   = 0x00000001, // items respawn
   DM_WEAPONSTAY    = 0x00000002, // weapons stay
   DM_BARRELRESPAWN = 0x00000004, // barrels respawn (dm3)
   DM_PLAYERDROP    = 0x00000008, // players drop items (dm3)
   DM_RESPAWNSUPER  = 0x00000010, // respawning super powerups
};

// default dmflags for certain game modes

#define DMD_SINGLE      (0)
#define DMD_COOP        (DM_WEAPONSTAY)
#define DMD_DEATHMATCH  (DM_WEAPONSTAY)
#define DMD_DEATHMATCH2 (DM_ITEMRESPAWN)
#define DMD_DEATHMATCH3 (DM_ITEMRESPAWN|DM_WEAPONSTAY|DM_BARRELRESPAWN|DM_PLAYERDROP)

extern unsigned long dmflags;
extern unsigned long default_dmflags;

void G_SetDefaultDMFlags(int dmtype, boolean setdefault);

#endif

// EOF

