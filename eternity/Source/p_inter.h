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
//   Constants and functions for object interaction.
//   External variables editable by DeHackEd patch.
//
//-----------------------------------------------------------------------------

#ifndef __P_INTER__
#define __P_INTER__

#include "d_player.h"
#include "p_mobj.h"

// Ty 03/09/98 Moved to an int in p_inter.c for deh and externalization 
#define MAXHEALTH maxhealth

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD   (100)

boolean P_GivePower(player_t *, int);
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher);
void P_DamageMobj(mobj_t *target,mobj_t *inflictor,mobj_t *source,int damage,int mod);
void P_KillMobj(mobj_t *target, mobj_t *source);

void P_Whistle(mobj_t *actor, int mobjtype);

// killough 5/2/98: moved from d_deh.c, g_game.c, m_misc.c, others:

extern int god_health;   // Ty 03/09/98 - deh support, see also p_inter.c
extern int idfa_armor;
extern int idfa_armor_class;
extern int idkfa_armor;
extern int idkfa_armor_class;  // Ty - end
// Ty 03/13/98 - externalized initial settings for respawned player
extern int initial_health;
extern int initial_bullets;
extern int maxhealth;
extern int max_armor;
extern int green_armor_class;
extern int blue_armor_class;
extern int max_soul;
extern int soul_health;
extern int mega_health;
extern int god_health;
extern int idfa_armor;
extern int idfa_armor_class;
extern int idkfa_armor;
extern int idkfa_armor_class;
extern int bfgcells;
extern int maxammo[], clipammo[];

// haleyjd 08/01/04: special inflictor types
typedef enum
{
   INFLICTOR_NONE,
   INFLICTOR_MINOTAUR,  // minotaur charge
   INFLICTOR_WHIRLWIND, // whirlwinds
   INFLICTOR_NUMTYPES
} inflictor_type_e;

#endif

//----------------------------------------------------------------------------
//
// $Log: p_inter.h,v $
// Revision 1.3  1998/05/03  23:08:57  killough
// beautification, add of the DEH parameter declarations
//
// Revision 1.2  1998/01/26  19:27:19  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
