/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_tick.c,v 1.2 2000/05/04 11:23:01 proff_fs Exp $
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
 *      Thinker, Ticker.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_tick.c,v 1.2 2000/05/04 11:23:01 proff_fs Exp $";

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
  thinker->function.acv = 
    (thinker->function.acp1 == (actionf_p1) P_MobjThinker) 
    ? (actionf_v)P_RemoveMobjDelayed : (actionf_v)P_RemoveThinkerDelayed;
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
    if (currentthinker->function.acp1)
      currentthinker->function.acp1(currentthinker);
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

//----------------------------------------------------------------------------
//
// $Log: p_tick.c,v $
// Revision 1.2  2000/05/04 11:23:01  proff_fs
// added an textwindow for Win32 and
// changed some printfs to lprintfs
//
// Revision 1.1.1.1  2000/05/04 08:15:01  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.5  1999/10/12 13:01:13  cphipps
// Changed header to GPL
//
// Revision 1.4  1999/10/02 12:00:41  cphipps
// Disable mobj reference count checking if SIMPLECHECKS not set
//
// Revision 1.3  1999/06/20 20:04:07  cphipps
// Fix bug where mobj references weren't preventing them being freed (d'oh)
// Made the P_Remove* functions static
//
// Revision 1.2  1999/02/04 15:21:59  cphipps
// New thinker deletion algorithm from MBF
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.7  1998/05/15  00:37:56  killough
// Remove unnecessary crash hack, fix demo sync
//
// Revision 1.6  1998/05/13  22:57:59  killough
// Restore Doom bug compatibility for demos
//
// Revision 1.5  1998/05/03  22:49:01  killough
// Get minimal includes at top
//
// Revision 1.4  1998/04/29  16:19:16  killough
// Fix typo causing game to not pause correctly
//
// Revision 1.3  1998/04/27  01:59:58  killough
// Fix crashes caused by thinkers being used after freed
//
// Revision 1.2  1998/01/26  19:24:32  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:01  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
