/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_tick.c,v 1.4 2000/05/09 21:45:39 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
 *      Thinker, Ticker.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_tick.c,v 1.4 2000/05/09 21:45:39 proff_fs Exp $";

#include "doomstat.h"
#include "p_user.h"
#include "p_spec.h"
#include "p_tick.h"
#include "lprintf.h"

int leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

// Both the head and tail of the thinker list.
thinker_t thinkercap;

// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.

static thinker_t *currentthinker;

//
// P_InitThinkers
//

void P_InitThinkers(void)
{
  thinkercap.prev = thinkercap.next  = &thinkercap;
}

//
// P_AddThinker
// Adds a new thinker at the end of the list.
//

void P_AddThinker(thinker_t* thinker)
{
  thinkercap.prev->next = thinker;
  thinker->next = &thinkercap;
  thinker->prev = thinkercap.prev;
  thinkercap.prev = thinker;
}

//
// CPhipps - reference counting for mobjs from MBF
//

void P_SetTarget(mobj_t **mop, mobj_t *targ) 
{
  if (*mop) {            // If there was a target already, decrease its refcount
#ifdef SIMPLECHECKS
    if (!((*mop)->references--)) 
      lprintf(LO_ERROR,"P_SetTarget: Bad reference count decrement\n");
#else
    (*mop)->references--;
#endif
  }
  if ((*mop = targ))    // Set new target and if non-NULL, increase its counter
    targ->references++;
}

//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceeding it, so
// that the next step in P_RunThinkers() will get its successor.
//

// cph - separate function for mobj deletion, to worry about references

static void P_RemoveThinkerDelayed(thinker_t *thinker)
{
  thinker_t *next = thinker->next;
  (next->prev = currentthinker = thinker->prev)->next = next;
  Z_Free(thinker);
}

static void P_RemoveMobjDelayed(mobj_t *mobj)
{
  if (!mobj->references)
    P_RemoveThinkerDelayed((thinker_t *)mobj);
}

//
// P_RemoveThinker
//
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// promoted to P_RemoveThinker() automatically as part of the thinker process.
//

void P_RemoveThinker(thinker_t *thinker)
{
  /* cph - Different removal function if it's an mobj
   * since for an mobj we have to check references first */
  thinker->function = 
    (thinker->function == P_MobjThinker) 
    ? P_RemoveMobjDelayed : P_RemoveThinkerDelayed;
}

//
// P_RunThinkers
//
// killough 4/25/98:
//
// Fix deallocator to stop using "next" pointer after node has been freed
// (a Doom bug).
//
// Process each thinker. For thinkers which are marked deleted, we must
// load the "next" pointer prior to freeing the node. In Doom, the "next"
// pointer was loaded AFTER the thinker was freed, which could have caused
// crashes.
//
// But if we are not deleting the thinker, we should reload the "next"
// pointer after calling the function, in case additional thinkers are
// added at the end of the list.
//
// killough 11/98:
//
// Rewritten to delete nodes implicitly, by making currentthinker
// external and using P_RemoveThinkerDelayed() implicitly.
//

static void P_RunThinkers (void)
{
  for (currentthinker = thinkercap.next;
       currentthinker != &thinkercap;
       currentthinker = currentthinker->next)
    if (currentthinker->function)
      currentthinker->function(currentthinker);
}

//
// P_Ticker
//

void P_Ticker (void)
{
  int i;

  // pause if in menu and at least one tic has been run
  if (paused || (!netgame && menuactive && !demoplayback &&
                  players[consoleplayer].viewz != 1))
    return;

  for (i=0; i<MAXPLAYERS; i++)
    if (playeringame[i])
      P_PlayerThink(&players[i]);

  P_RunThinkers();
  P_UpdateSpecials();
  P_RespawnSpecials();
  leveltime++;                       // for par times
}
