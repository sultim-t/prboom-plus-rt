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
//      Enemy thinking, AI.
//      Action Pointer Functions
//      that are associated with states/frames.
//
//-----------------------------------------------------------------------------

#ifndef __P_ENEMY__
#define __P_ENEMY__

#include "m_random.h"
#include "p_mobj.h"

void P_NoiseAlert (mobj_t *target, mobj_t *emmiter);
void P_SpawnBrainTargets(void); // killough 3/26/98: spawn icon landings
void P_SpawnSorcSpots(void);    // haleyjd 11/19/02: spawn dsparil spots

extern struct brain_s {         // killough 3/26/98: global state of boss brain
  int easy;
} brain;

boolean P_CheckMeleeRange(mobj_t *actor);

// haleyjd 07/13/03: editable boss brain spawn types
// schepe: removed 11-type limit
extern int NumBossTypes;
extern int *BossSpawnTypes;
extern int *BossSpawnProbs;

enum
{
   BOSSTELE_NONE,
   BOSSTELE_ORIG,
   BOSSTELE_BOTH,
   BOSSTELE_DEST
};

// haleyjd: bossteleport_t
//
// holds a ton of information to allow for the teleportation of
// a thing with various effects
//
typedef struct bossteleport_s
{
   MobjCollection *mc; // mobj collection to use
   pr_class_t rngNum;    // rng number to use for selecting spot

   mobj_t *boss;      // boss to teleport
   statenum_t state;  // number of state to put boss in (-1 to not)

   mobjtype_t fxtype; // type of effect object to spawn
   fixed_t zpamt;     // amount to add of z coordinate of effect
   int hereThere;     // locations to spawn effects at (0, 1, or 2)
   int soundNum;      // sound to play at locations
} bossteleport_t;

void P_BossTeleport(bossteleport_t *bt);

void P_SkullFly(mobj_t *actor, fixed_t speed);

#endif // __P_ENEMY__

//----------------------------------------------------------------------------
//
// $Log: p_enemy.h,v $
// Revision 1.1  1998/05/03  22:29:32  killough
// External declarations formerly in p_local.h
//
//
//----------------------------------------------------------------------------
