// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
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
// DESCRIPTION:
//      Player related stuff.
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

#ifndef __P_USER__
#define __P_USER__

#include "d_player.h"

// haleyjd 10/31/02: moved to header
// Index of the special effects (INVUL inverse) map.

#define INVERSECOLORMAP 32

void P_PlayerThink(player_t *player);
void P_CalcHeight(player_t *player);
void P_DeathThink(player_t *player);
void P_MovePlayer(player_t *player);
void P_Thrust(player_t *player, angle_t angle, fixed_t move);
boolean P_CheckClipView(mobj_t *mo);

#endif // __P_USER__

//----------------------------------------------------------------------------
//
// $Log: p_user.h,v $
// Revision 1.2  1998/05/10  23:38:38  killough
// Add more prototypes
//
// Revision 1.1  1998/05/03  23:19:24  killough
// Move from obsolete p_local.h
//
//----------------------------------------------------------------------------
