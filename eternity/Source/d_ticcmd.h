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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "doomtype.h"

// NETCODE_FIXME: ticcmd_t lives here. It's the structure used to hold
// all player input that can currently be transmitted across the network.
// It is NOT sufficient for the following reasons:
// 1) It cannot efficiently transmit console cmd/cvar changes
// 2) It cannot efficiently transmit player chat (and never has)
// 3) Weapon changes and special events are packed into buttons using
//    special bit codes. This is hackish and artificially limiting, and
//    will create severe problems for the future generalized weapon
//    system.
// 4) There is no packing currently, so the entire ticcmd_t structure
//    is sent every tic, sometimes multiple times. This is horribly
//    inefficient and is hampering the addition of more inputs such as
//    flying, swimming, jumping, inventory use, etc.
//
// DEMO_FIXME: Warning -- changes to ticcmd_t must be reflected in the
// code that reads and writes demos as well.
// ticcmds are built in G_BuildTiccmd or by G_ReadDemoTiccmd
// ticcmds are transmitted over the network by code in d_net.c
//

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.
typedef struct
{
   char  forwardmove; // *2048 for move
   char  sidemove;    // *2048 for move
   short look;        // haleyjd: <<16 for look delta
   short angleturn;   // <<16 for angle delta
   short consistancy; // checks for net game
   byte  chatchar;
   byte  buttons;
} ticcmd_t;

#endif

//----------------------------------------------------------------------------
//
// $Log: d_ticcmd.h,v $
// Revision 1.2  1998/01/26  19:26:36  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
