/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_map.c,v 1.2 2000/05/09 21:45:39 proff_fs Exp $
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
 *  Movement, collision handling.
 *  Shooting and aiming.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_map.c,v 1.2 2000/05/09 21:45:39 proff_fs Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "p_mobj.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_inter.h"
#include "m_random.h"
#include "m_bbox.h"
#include "lprintf.h"

static mobj_t    *tmthing;
static int       tmflags;
static fixed_t   tmx;
static fixed_t   tmy;
static int pe_x; // Pain Elemental position for Lost Soul checks // phares
static int pe_y; // Pain Elemental position for Lost Soul checks // phares
static int ls_x; // Lost Soul position for Lost Soul checks      // phares
static int ls_y; // Lost Soul position for Lost Soul checks      // phares

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".

boolean   floatok;

// The tm* items are used to hold information globally, usually for
// line or object intersection checking

fixed_t   tmbbox[4];  // bounding box for line intersection checks
fixed_t   tmfloorz;   // floor you'd hit if free to fall
fixed_t   tmceilingz; // ceiling of sector you're in
fixed_t   tmdropoffz; // dropoff on other side of line you're crossing

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls

line_t    *ceilingline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid

// 1/11/98 killough: removed limit on special lines crossed
line_t **spechit;                // new code -- killough
static int spechit_max;          // killough

int numspechit;

// Temporary holder for thing_sectorlist threads
msecnode_t* sector_list = NULL;                             // phares 3/16/98

extern boolean onground; // whether you're on the ground, for ice purposes

//
// TELEPORT MOVE
//

//
// PIT_StompThing
//
boolean PIT_StompThing (mobj_t* thing)
  {
  fixed_t blockdist;

  // phares 9/10/98: moved this self-check to start of routine

  // don't clip against self

  if (thing == tmthing)
    return true;

  if (!(thing->flags & MF_SHOOTABLE)) // Can't shoot it? Can't stomp it!
    return true;

  blockdist = thing->radius + tmthing->radius;

  if (D_abs(thing->x - tmx) >= blockdist || D_abs(thing->y - tmy) >= blockdist)
    return true; // didn't hit it

  // monsters don't stomp things except on boss level

  if (!tmthing->player && gamemap != 30)
    return false;

  P_DamageMobj (thing, tmthing, tmthing, 10000); // Stomp!

  return true;
  }


// P_GetMoveFactor() returns the value by which the x,y     // phares 3/19/98
// movements are multiplied to add to player movement.      //     |
                                                            //     V
int P_GetMoveFactor(mobj_t* mo)
  {
  int movefactor = ORIG_FRICTION_FACTOR;

  // If the floor is icy or muddy, it's harder to get moving. This is where
  // the different friction factors are applied to 'trying to move'. In
  // p_mobj.c, the friction factors are applied as you coast and slow down.

  int momentum,friction;

  if (!compatibility && variable_friction &&
      !(mo->flags & (MF_NOGRAVITY | MF_NOCLIP)))
    {
    friction = mo->friction;
    if (friction == ORIG_FRICTION)            // normal floor
      ;
    else if (friction > ORIG_FRICTION)        // ice
      {
      movefactor = mo->movefactor;
      mo->movefactor = ORIG_FRICTION_FACTOR;  // reset
      }
    else                                      // sludge
      {

      // phares 3/11/98: you start off slowly, then increase as
      // you get better footing

      momentum = (P_AproxDistance(mo->momx,mo->momy));
      movefactor = mo->movefactor;
      if (momentum > MORE_FRICTION_MOMENTUM<<2)
        movefactor <<= 3;

      else if (momentum > MORE_FRICTION_MOMENTUM<<1)
        movefactor <<= 2;

      else if (momentum > MORE_FRICTION_MOMENTUM)
        movefactor <<= 1;

      mo->movefactor = ORIG_FRICTION_FACTOR;  // reset
      }
    }                                                       //     ^
  return(movefactor);                                       //     |
  }                                                         // phares 3/19/98

//
// P_TeleportMove
//

boolean P_TeleportMove (mobj_t* thing,fixed_t x,fixed_t y)
  {
  int     xl;
  int     xh;
  int     yl;
  int     yh;
  int     bx;
  int     by;

  subsector_t*  newsubsec;

  // kill anything occupying the position

  tmthing = thing;
  tmflags = thing->flags;

  tmx = x;
  tmy = y;

  tmbbox[BOXTOP] = y + tmthing->radius;
  tmbbox[BOXBOTTOM] = y - tmthing->radius;
  tmbbox[BOXRIGHT] = x + tmthing->radius;
  tmbbox[BOXLEFT] = x - tmthing->radius;

  newsubsec = R_PointInSubsector (x,y);
  ceilingline = NULL;

  // The base floor/ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.

  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;

  validcount++;
  numspechit = 0;

  // stomp on any things contacted

  xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
        return false;

  // the move is ok,
  // so unlink from the old position & link into the new position

  P_UnsetThingPosition (thing);

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;
  thing->x = x;
  thing->y = y;

  P_SetThingPosition (thing);

  return true;
  }


//
// MOVEMENT ITERATOR FUNCTIONS
//

//                                                                  // phares
// PIT_CrossLine                                                    //   |
// Checks to see if a PE->LS trajectory line crosses a blocking     //   V
// line. Returns false if it does.
//
// tmbbox holds the bounding box of the trajectory. If that box
// does not touch the bounding box of the line in question,
// then the trajectory is not blocked. If the PE is on one side
// of the line and the LS is on the other side, then the
// trajectory is blocked.
//
// Currently this assumes an infinite line, which is not quite
// correct. A more correct solution would be to check for an
// intersection of the trajectory and the line, but that takes
// longer and probably really isn't worth the effort.
//

static // killough 3/26/98: make static
boolean PIT_CrossLine (line_t* ld)
  {
  if (!(ld->flags & ML_TWOSIDED) ||
      (ld->flags & (ML_BLOCKING|ML_BLOCKMONSTERS)))
    if (!(tmbbox[BOXLEFT]   > ld->bbox[BOXRIGHT]  ||
          tmbbox[BOXRIGHT]  < ld->bbox[BOXLEFT]   ||
          tmbbox[BOXTOP]    < ld->bbox[BOXBOTTOM] ||
          tmbbox[BOXBOTTOM] > ld->bbox[BOXTOP]))
      if (P_PointOnLineSide(pe_x,pe_y,ld) != P_PointOnLineSide(ls_x,ls_y,ld))
        return(false);  // line blocks trajectory                   //   ^
  return(true); // line doesn't block trajectory                    //   |
  }                                                                 // phares


//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//

static // killough 3/26/98: make static
boolean PIT_CheckLine (line_t* ld)
  {
  if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
   || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
   || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
   || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
    return true; // didn't hit it

  if (P_BoxOnLineSide(tmbbox, ld) != -1)
    return true; // didn't hit it

  // A line has been hit

  // The moving thing's destination position will cross the given line.
  // If this should not be allowed, return false.
  // If the line is special, keep track of it
  // to process later if the move is proven ok.
  // NOTE: specials are NOT sorted by order,
  // so two special lines that are only 8 pixels apart
  // could be crossed in either order.

  if (!ld->backsector) // one-sided line
    return false; // 9/8/98 remove 1S wall escape - causes problems w
                 // motion by monsters or caused by monster attacks

  if (!(tmthing->flags & MF_MISSILE) )
    {
    if ( ld->flags & ML_BLOCKING )
      return false; // explicitly blocking everything

    if ( !tmthing->player && ld->flags & ML_BLOCKMONSTERS )
      return false; // block monsters only
    }

  // set openrange, opentop, openbottom
  // these define a 'window' from one sector to another across this line

  P_LineOpening (ld);

  // adjust floor & ceiling heights

  if (opentop < tmceilingz)
    {
    tmceilingz = opentop;
    ceilingline = ld;
    }

  if (openbottom > tmfloorz)
    tmfloorz = openbottom;

  if (lowfloor < tmdropoffz)
    tmdropoffz = lowfloor;

  // if contacted a special line, add it to the list

  if (ld->special)
    {
    // 1/11/98 killough: remove limit on lines hit, by array doubling
    if (numspechit >= spechit_max)
      {
      spechit_max = spechit_max ? spechit_max*2 : 8;
      spechit = realloc(spechit,sizeof(*spechit)*spechit_max); // killough
      }

    spechit[numspechit] = ld;
    numspechit++;
    }

  return true;
  }

//
// PIT_CheckThing
//

static // killough 3/26/98: make static
boolean PIT_CheckThing (mobj_t* thing)
  {
  fixed_t blockdist;
  boolean solid;
  int     damage;

  // phares 9/10/98: moved this self-check to start of routine

  // don't clip against self

  if (thing == tmthing)
    return true;

  if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
    return true;

  blockdist = thing->radius + tmthing->radius;

  if ( D_abs(thing->x - tmx) >= blockdist || D_abs(thing->y - tmy) >= blockdist)
    return true; // didn't hit it

  // don't clip against self

  if (thing == tmthing)
    return true;

  // check for skulls slamming into things

  if (tmthing->flags & MF_SKULLFLY)
    {

    // A flying skull is smacking something.
    // Determine damage amount, and the skull comes to a dead stop.

    damage = ((P_Random(pr_skullfly)%8)+1)*tmthing->info->damage;

    P_DamageMobj (thing, tmthing, tmthing, damage);

    tmthing->flags &= ~MF_SKULLFLY;
    tmthing->momx = tmthing->momy = tmthing->momz = 0;

    P_SetMobjState (tmthing, tmthing->info->spawnstate);

    return false;   // stop moving
    }


  // missiles can hit other things

  if (tmthing->flags & MF_MISSILE)
    {

    // see if it went over / under

    if (tmthing->z > thing->z + thing->height)
      return true;    // overhead

    if (tmthing->z+tmthing->height < thing->z)
      return true;    // underneath

    if (tmthing->target &&
        (tmthing->target->type == thing->type ||
        (tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
        (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
      {

      // Don't hit same species as originator.

      if (thing == tmthing->target)
        return true;

      if (thing->type != MT_PLAYER)
        {

        // Explode, but do no damage.
        // Let players missile other players.

        return false;
        }
      }

    if (! (thing->flags & MF_SHOOTABLE) )
      return !(thing->flags & MF_SOLID); // didn't do any damage

    // damage / explode

    damage = ((P_Random(pr_damage)%8)+1)*tmthing->info->damage;
    P_DamageMobj (thing, tmthing, tmthing->target, damage);

    // don't traverse any more
    return false;
    }

  // check for special pickup

  if (thing->flags & MF_SPECIAL)
    {
    solid = thing->flags&MF_SOLID;
    if (tmflags&MF_PICKUP)
      P_TouchSpecialThing (thing, tmthing); // can remove thing
    return !solid;
    }

  // killough 3/16/98: Allow non-solid moving objects to move through solid
  // ones, by allowing the moving thing (tmthing) to move if it's non-solid,
  // despite another solid thing being in the way.
  // killough 4/11/98: Treat no-clipping things as not blocking

  return !((thing->flags & MF_SOLID && !(thing->flags & MF_NOCLIP))
           && (tmthing->flags & MF_SOLID || demo_compatibility));

  // return !(thing->flags & MF_SOLID);   // old code -- killough
  }


// This routine checks for Lost Souls trying to be spawned      // phares
// across 1-sided lines, impassible lines, or "monsters can't   //   |
// cross" lines. Draw an imaginary line between the PE          //   V
// and the new Lost Soul spawn spot. If that line crosses
// a 'blocking' line, then disallow the spawn. Only search
// lines in the blocks of the blockmap where the bounding box
// of the trajectory line resides. Then check bounding box
// of the trajectory vs. the bounding box of each blocking
// line to see if the trajectory and the blocking line cross.
// Then check the PE and LS to see if they're on different
// sides of the blocking line. If so, return true, otherwise
// false.

boolean Check_Sides(mobj_t* actor, int x, int y)
  {
  int bx,by,xl,xh,yl,yh;

  pe_x = actor->x;
  pe_y = actor->y;
  ls_x = x;
  ls_y = y;

  // Here is the bounding box of the trajectory

  tmbbox[BOXLEFT]   = pe_x < x ? pe_x : x;
  tmbbox[BOXRIGHT]  = pe_x > x ? pe_x : x;
  tmbbox[BOXTOP]    = pe_y > y ? pe_y : y;
  tmbbox[BOXBOTTOM] = pe_y < y ? pe_y : y;

  // Determine which blocks to look in for blocking lines

  xl = (tmbbox[BOXLEFT]   - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT]  - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP]    - bmaporgy)>>MAPBLOCKSHIFT;

  // xl->xh, yl->yh determine the mapblock set to search

  validcount++; // prevents checking same line twice
  for (bx = xl ; bx <= xh ; bx++)
    for (by = yl ; by <= yh ; by++)
      if (!P_BlockLinesIterator(bx,by,PIT_CrossLine))
        return true;                                                //   ^
  return(false);                                                    //   |
  }                                                                 // phares

//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//

boolean P_CheckPosition (mobj_t* thing,fixed_t x,fixed_t y)
  {
  int     xl;
  int     xh;
  int     yl;
  int     yh;
  int     bx;
  int     by;
  subsector_t*  newsubsec;

  tmthing = thing;
  tmflags = thing->flags;

  tmx = x;
  tmy = y;

  tmbbox[BOXTOP] = y + tmthing->radius;
  tmbbox[BOXBOTTOM] = y - tmthing->radius;
  tmbbox[BOXRIGHT] = x + tmthing->radius;
  tmbbox[BOXLEFT] = x - tmthing->radius;

  newsubsec = R_PointInSubsector (x,y);
  ceilingline = NULL;

  // The base floor / ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.

  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;
  validcount++;
  numspechit = 0;

  if ( tmflags & MF_NOCLIP )
    return true;

  // Check things first, possibly picking things up.
  // The bounding box is extended by MAXRADIUS
  // because mobj_ts are grouped into mapblocks
  // based on their origin point, and can overlap
  // into adjacent blocks by up to MAXRADIUS units.

  xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;


  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
        return false;

  // check lines

  xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
        return false; // doesn't fit

  return true;
  }


//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
boolean P_TryMove(mobj_t* thing,fixed_t x,fixed_t y,
                  boolean dropoff) // killough 3/15/98: allow dropoff as option
  {
  fixed_t oldx;
  fixed_t oldy;
  int     side;
  int     oldside;
  line_t* ld;

  floatok = false;
  if (!P_CheckPosition (thing, x, y))
    return false;   // solid wall or thing

  if ( !(thing->flags & MF_NOCLIP) )
    {
#ifdef CEILCARRY
    // killough 4/11/98: if the thing hangs from
    // a ceiling, don't worry about it fitting

    if (!(thing->flags & MF_SPAWNCEILING) || demo_compatibility)
#endif

      if (tmceilingz - tmfloorz < thing->height)
        return false; // doesn't fit

    floatok = true;

    if (!(thing->flags&MF_TELEPORT) && tmceilingz - thing->z < thing->height)
      return false; // mobj must lower itself to fit

    if (!(thing->flags&MF_TELEPORT) && tmfloorz - thing->z > 24*FRACUNIT)
      return false; // too big a step up

    // killough 3/15/98: Allow certain objects to drop off

    if (compatibility || !dropoff)
      if ( !(thing->flags&(MF_DROPOFF|MF_FLOAT))
          && tmfloorz - tmdropoffz > 24*FRACUNIT )
        return false; // don't stand over a dropoff
    }

  // the move is ok,
  // so unlink from the old position and link into the new position

  P_UnsetThingPosition (thing);

  oldx = thing->x;
  oldy = thing->y;
  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;
  thing->x = x;
  thing->y = y;

  P_SetThingPosition (thing);

  // if any special lines were hit, do the effect

  if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
    while (numspechit--)
      {

      // see if the line was crossed

      ld = spechit[numspechit];
      side = P_PointOnLineSide (thing->x, thing->y, ld);
      oldside = P_PointOnLineSide (oldx, oldy, ld);
      if (side != oldside)
        if (ld->special)
          P_CrossSpecialLine (ld, oldside, thing); // CPhipps - pass line pointer
      }

  return true;
  }

//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//

boolean P_ThingHeightClip (mobj_t* thing)
  {
  boolean   onfloor;
  int                 oldz; // phares 3/10/98

  onfloor = (thing->z == thing->floorz);

  P_CheckPosition (thing, thing->x, thing->y);

  // what about stranding a monster partially off an edge?

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;

  // Capture the old z value and compare
  // it to the new z value below. If they're the same, do nothing. If they're
  // different, the player was affected by the moving sector, and he should
  // get the friction value of the sector below his (x,y).

  oldz = thing->z;                                          // phares 3/10/98
  if (onfloor)
    {

    // walking monsters rise and fall with the floor

    thing->z = thing->floorz;
    }
  else
    {

  // don't adjust a floating monster unless forced to

    if (thing->z+thing->height > thing->ceilingz)
      thing->z = thing->ceilingz - thing->height;
    }

  if (thing->ceilingz - thing->floorz < thing->height)
    return false;

  return true;
  }


//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//

fixed_t   bestslidefrac;
fixed_t   secondslidefrac;

line_t*   bestslideline;
line_t*   secondslideline;

mobj_t*   slidemo;

fixed_t   tmxmove;
fixed_t   tmymove;


//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
// If the floor is icy, then you can bounce off a wall.             // phares
//

void P_HitSlideLine (line_t* ld)
  {
  int     side;
  angle_t lineangle;
  angle_t moveangle;
  angle_t deltaangle;
  fixed_t movelen;
  fixed_t newlen;
  boolean icyfloor;  // is floor icy?                               // phares
                                                                    //   |
  // Under icy conditions, if the angle of approach to the wall     //   V
  // is more than 45 degrees, then you'll bounce and lose half
  // your momentum. If less than 45 degrees, you'll slide along
  // the wall. 45 is arbitrary and is believable.

  // Check for the special cases of horz or vert walls.

  if (!compatibility && variable_friction && slidemo->player)
    // player must be on ground
    icyfloor = (onground && (slidemo->friction > ORIG_FRICTION));
  else
    icyfloor = false;

  if (ld->slopetype == ST_HORIZONTAL)
    {
    if (icyfloor && (D_abs(tmymove) > D_abs(tmxmove)))
      {
      tmxmove /= 2; // absorb half the momentum
      tmymove = -tmymove/2;
      S_StartSound(slidemo,sfx_oof); // oooff!
      }
    else
      tmymove = 0; // no more movement in the Y direction
    return;
    }

  if (ld->slopetype == ST_VERTICAL)
    {
    if (icyfloor && (D_abs(tmxmove) > D_abs(tmymove)))
      {
      tmxmove = -tmxmove/2; // absorb half the momentum
      tmymove /= 2;
      S_StartSound(slidemo,sfx_oof); // oooff!                      //   ^
      }                                                             //   |
    else                                                            // phares
      tmxmove = 0; // no more movement in the X direction
    return;
    }

  // The wall is angled. Bounce if the angle of approach is         // phares
  // less than 45 degrees.                                          // phares

  side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);

  lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);
  if (side == 1)
    lineangle += ANG180;
  moveangle = R_PointToAngle2 (0,0, tmxmove, tmymove);

  // killough 3/2/98:
  // The moveangle+=10 breaks v1.9 demo compatibility in
  // some demos, so it needs demo_compatibility switch.

  if (!demo_compatibility)
    moveangle += 10; // prevents sudden path reversal due to        // phares
                     // rounding error                              //   |
  deltaangle = moveangle-lineangle;                                 //   V
  movelen = P_AproxDistance (tmxmove, tmymove);
  if (icyfloor && (deltaangle > ANG45) && (deltaangle < ANG90+ANG45))
    {
    moveangle = lineangle - deltaangle;
    movelen /= 2; // absorb
    S_StartSound(slidemo,sfx_oof); // oooff!
    moveangle >>= ANGLETOFINESHIFT;
    tmxmove = FixedMul (movelen, finecosine[moveangle]);
    tmymove = FixedMul (movelen, finesine[moveangle]);
    }                                                               //   ^
  else                                                              //   |
    {                                                               // phares
    if (deltaangle > ANG180)
      deltaangle += ANG180;

    //  I_Error ("SlideLine: ang>ANG180");

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;
    newlen = FixedMul (movelen, finecosine[deltaangle]);
    tmxmove = FixedMul (newlen, finecosine[lineangle]);
    tmymove = FixedMul (newlen, finesine[lineangle]);
    }                                                               // phares
  }


//
// PTR_SlideTraverse
//

boolean PTR_SlideTraverse (intercept_t* in)
  {
  line_t* li;

  if (!in->isaline)
    I_Error ("PTR_SlideTraverse: not a line?");

  li = in->d.line;

  if ( ! (li->flags & ML_TWOSIDED) )
    {
    if (P_PointOnLineSide (slidemo->x, slidemo->y, li))
      return true; // don't hit the back side
    goto isblocking;
    }

  // set openrange, opentop, openbottom.
  // These define a 'window' from one sector to another across a line

  P_LineOpening (li);

  if (openrange < slidemo->height)
    goto isblocking;  // doesn't fit

  if (opentop - slidemo->z < slidemo->height)
    goto isblocking;  // mobj is too high

  if (openbottom - slidemo->z > 24*FRACUNIT )
    goto isblocking;  // too big a step up

  // this line doesn't block movement

  return true;

  // the line does block movement,
  // see if it is closer than best so far

isblocking:

  if (in->frac < bestslidefrac)
    {
    secondslidefrac = bestslidefrac;
    secondslideline = bestslideline;
    bestslidefrac = in->frac;
    bestslideline = li;
    }

  return false; // stop
  }


//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//

void P_SlideMove (mobj_t* mo)
  {
  fixed_t leadx;
  fixed_t leady;
  fixed_t trailx;
  fixed_t traily;
  fixed_t newx;
  fixed_t newy;
  int     hitcount;

  slidemo = mo; // the object that's sliding
  hitcount = 0;

retry:

  if (++hitcount == 3)
    goto stairstep;   // don't loop forever

  // trace along the three leading corners

  if (mo->momx > 0)
    {
    leadx = mo->x + mo->radius;
    trailx = mo->x - mo->radius;
    }
  else
    {
    leadx = mo->x - mo->radius;
    trailx = mo->x + mo->radius;
    }

  if (mo->momy > 0)
    {
    leady = mo->y + mo->radius;
    traily = mo->y - mo->radius;
    }
  else
    {
    leady = mo->y - mo->radius;
    traily = mo->y + mo->radius;
    }

  bestslidefrac = FRACUNIT+1;

  P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
     PT_ADDLINES, PTR_SlideTraverse );
  P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
     PT_ADDLINES, PTR_SlideTraverse );
  P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
     PT_ADDLINES, PTR_SlideTraverse );

  // move up to the wall

  if (bestslidefrac == FRACUNIT+1)
    {

    // the move must have hit the middle, so stairstep

  stairstep:

    // killough 3/15/98: Allow objects to drop off ledges

    if (!P_TryMove (mo, mo->x, mo->y + mo->momy, true))
      P_TryMove (mo, mo->x + mo->momx, mo->y, true);
    return;
    }

  // fudge a bit to make sure it doesn't hit

  bestslidefrac -= 0x800;
  if (bestslidefrac > 0)
    {
    newx = FixedMul (mo->momx, bestslidefrac);
    newy = FixedMul (mo->momy, bestslidefrac);

    // killough 3/15/98: Allow objects to drop off ledges

    if (!P_TryMove (mo, mo->x+newx, mo->y+newy, true))
      goto stairstep;
    }

  // Now continue along the wall.
  // First calculate remainder.

  bestslidefrac = FRACUNIT-(bestslidefrac+0x800);

  if (bestslidefrac > FRACUNIT)
    bestslidefrac = FRACUNIT;

  if (bestslidefrac <= 0)
    return;

  tmxmove = FixedMul (mo->momx, bestslidefrac);
  tmymove = FixedMul (mo->momy, bestslidefrac);

  P_HitSlideLine (bestslideline); // clip the moves

  mo->momx = tmxmove;
  mo->momy = tmymove;

  // killough 3/15/98: Allow objects to drop off ledges

  if (!P_TryMove (mo, mo->x+tmxmove, mo->y+tmymove, true))
    goto retry;
  }


//
// P_LineAttack
//
mobj_t*   linetarget; // who got hit (or NULL)
mobj_t*   shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t   shootz;

int       la_damage;
fixed_t   attackrange;

fixed_t   aimslope;

// slopes to top and bottom of target
// killough 4/20/98: make static instead of using ones in p_sight.c

static fixed_t  topslope;
static fixed_t  bottomslope;


//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
boolean PTR_AimTraverse (intercept_t* in)
  {
  line_t* li;
  mobj_t* th;
  fixed_t slope;
  fixed_t thingtopslope;
  fixed_t thingbottomslope;
  fixed_t dist;

  if (in->isaline)
    {
    li = in->d.line;

    if ( !(li->flags & ML_TWOSIDED) )
      return false;   // stop

    // Crosses a two sided line.
    // A two sided line will restrict
    // the possible target ranges.

    P_LineOpening (li);

    if (openbottom >= opentop)
      return false;   // stop

    dist = FixedMul (attackrange, in->frac);

    if (li->frontsector->floorheight != li->backsector->floorheight)
      {
      slope = FixedDiv (openbottom - shootz , dist);
      if (slope > bottomslope)
        bottomslope = slope;
      }

    if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
      {
      slope = FixedDiv (opentop - shootz , dist);
      if (slope < topslope)
        topslope = slope;
      }

    if (topslope <= bottomslope)
      return false;   // stop

    return true;    // shot continues
    }

  // shoot a thing

  th = in->d.thing;
  if (th == shootthing)
    return true;    // can't shoot self

  if (!(th->flags&MF_SHOOTABLE))
    return true;    // corpse or something

  // check angles to see if the thing can be aimed at

  dist = FixedMul (attackrange, in->frac);
  thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

  if (thingtopslope < bottomslope)
    return true;    // shot over the thing

  thingbottomslope = FixedDiv (th->z - shootz, dist);

  if (thingbottomslope > topslope)
    return true;    // shot under the thing

  // this thing can be hit!

  if (thingtopslope > topslope)
    thingtopslope = topslope;

  if (thingbottomslope < bottomslope)
    thingbottomslope = bottomslope;

  aimslope = (thingtopslope+thingbottomslope)/2;
  linetarget = th;

  return false;   // don't go any farther
  }


//
// PTR_ShootTraverse
//
boolean PTR_ShootTraverse (intercept_t* in)
  {
  fixed_t x;
  fixed_t y;
  fixed_t z;
  fixed_t frac;

  line_t* li;

  mobj_t* th;

  fixed_t slope;
  fixed_t dist;
  fixed_t thingtopslope;
  fixed_t thingbottomslope;

  if (in->isaline)
    {
    li = in->d.line;

    if (li->special)
      P_ShootSpecialLine (shootthing, li);

    if ( !(li->flags & ML_TWOSIDED) )
      goto hitline;

    // crosses a two sided line

    P_LineOpening (li);

    dist = FixedMul (attackrange, in->frac);

    if (li->frontsector->floorheight != li->backsector->floorheight)
      {
      slope = FixedDiv (openbottom - shootz , dist);
      if (slope > aimslope)
        goto hitline;
      }

    if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
      {
      slope = FixedDiv (opentop - shootz , dist);
      if (slope < aimslope)
        goto hitline;
      }

    // shot continues

    return true;

    // hit line

hitline:

    // position a bit closer

    frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
    x = trace.x + FixedMul (trace.dx, frac);
    y = trace.y + FixedMul (trace.dy, frac);
    z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

    if (li->frontsector->ceilingpic == skyflatnum)
      {
      // don't shoot the sky!

      if (z > li->frontsector->ceilingheight)
        return false;

      // it's a sky hack wall

      if  (li->backsector && li->backsector->ceilingpic == skyflatnum)

        // fix bullet-eaters -- killough:
        // WARNING: Almost all demos will lose sync without this
        // demo_compatibility flag check!!! killough 1/18/98
      if (demo_compatibility || li->backsector->ceilingheight < z)
        return false;
      }

    // Spawn bullet puffs.

    P_SpawnPuff (x,y,z);

    // don't go any farther

    return false;
    }

  // shoot a thing

  th = in->d.thing;
  if (th == shootthing)
    return true;  // can't shoot self

  if (!(th->flags&MF_SHOOTABLE))
    return true;  // corpse or something

  // check angles to see if the thing can be aimed at

  dist = FixedMul (attackrange, in->frac);
  thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

  if (thingtopslope < aimslope)
    return true;  // shot over the thing

  thingbottomslope = FixedDiv (th->z - shootz, dist);

  if (thingbottomslope > aimslope)
    return true;  // shot under the thing

  // hit thing
  // position a bit closer

  frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

  x = trace.x + FixedMul (trace.dx, frac);
  y = trace.y + FixedMul (trace.dy, frac);
  z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

  // Spawn bullet puffs or blod spots,
  // depending on target type.
  if (in->d.thing->flags & MF_NOBLOOD)
    P_SpawnPuff (x,y,z);
  else
    P_SpawnBlood (x,y,z, la_damage);

  if (la_damage)
    P_DamageMobj (th, shootthing, shootthing, la_damage);

  // don't go any farther
  return false;
  }


//
// P_AimLineAttack
//
fixed_t P_AimLineAttack(mobj_t* t1,angle_t angle,fixed_t distance)
  {
  fixed_t x2;
  fixed_t y2;

  angle >>= ANGLETOFINESHIFT;
  shootthing = t1;

  x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
  y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
  shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

  // can't shoot outside view angles

  topslope = 100*FRACUNIT/160;
  bottomslope = -100*FRACUNIT/160;

  attackrange = distance;
  linetarget = NULL;

  P_PathTraverse(t1->x,t1->y,x2,y2,PT_ADDLINES|PT_ADDTHINGS,PTR_AimTraverse);

  if (linetarget)
    return aimslope;

  return 0;
  }


//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//

void P_LineAttack
(mobj_t* t1,
 angle_t angle,
 fixed_t distance,
 fixed_t slope,
 int     damage)
  {
  fixed_t x2;
  fixed_t y2;

  angle >>= ANGLETOFINESHIFT;
  shootthing = t1;
  la_damage = damage;
  x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
  y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
  shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
  attackrange = distance;
  aimslope = slope;

  P_PathTraverse(t1->x,t1->y,x2,y2,PT_ADDLINES|PT_ADDTHINGS,PTR_ShootTraverse);
  }


//
// USE LINES
//

mobj_t*   usething;

boolean PTR_UseTraverse (intercept_t* in)
  {
  int side;

  if (!in->d.line->special)
    {
    P_LineOpening (in->d.line);
    if (openrange <= 0)
      {
      S_StartSound (usething, sfx_noway);

      // can't use through a wall
      return false;
      }

    // not a special line, but keep checking

    return true;
    }

  side = 0;
  if (P_PointOnLineSide (usething->x, usething->y, in->d.line) == 1)
    side = 1;

  //  return false;   // don't use back side

  P_UseSpecialLine (usething, in->d.line, side);

  //WAS can't use for than one special line in a row
  //jff 3/21/98 NOW multiple use allowed with enabling line flag

  return (!demo_compatibility && (in->d.line->flags&ML_PASSUSE))?
          true : false;
}

// Returns false if a "oof" sound should be made because of a blocking
// linedef. Makes 2s middles which are impassable, as well as 2s uppers
// and lowers which block the player, cause the sound effect when the
// player tries to activate them. Specials are excluded, although it is
// assumed that all special linedefs within reach have been considered
// and rejected already (see P_UseLines).
//
// by Lee Killough
//

boolean PTR_NoWayTraverse(intercept_t* in)
  {
  line_t *ld = in->d.line;
                                           // This linedef
  return ld->special || !(                 // Ignore specials
   ld->flags & ML_BLOCKING || (            // Always blocking
   P_LineOpening(ld),                      // Find openings
   openrange <= 0 ||                       // No opening
   openbottom > usething->z+24*FRACUNIT || // Too high it blocks
   opentop < usething->z+usething->height  // Too low it blocks
  )
  );
  }

//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines (player_t*  player)
  {
  int     angle;
  fixed_t x1;
  fixed_t y1;
  fixed_t x2;
  fixed_t y2;

  usething = player->mo;

  angle = player->mo->angle >> ANGLETOFINESHIFT;

  x1 = player->mo->x;
  y1 = player->mo->y;
  x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
  y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];

  // old code:
  //
  // P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
  //
  // This added test makes the "oof" sound work on 2s lines -- killough:

  if (P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse ))
    if (!P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse ))
      S_StartSound (usething, sfx_noway);
  }


//
// RADIUS ATTACK
//

mobj_t*   bombsource;
mobj_t*   bombspot;
int   bombdamage;


//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//

boolean PIT_RadiusAttack (mobj_t* thing)
  {
  fixed_t dx;
  fixed_t dy;
  fixed_t dist;

  if (!(thing->flags & MF_SHOOTABLE) )
    return true;

  // Boss spider and cyborg
  // take no damage from concussion.

  if (thing->type == MT_CYBORG || thing->type == MT_SPIDER)
    return true;

  dx = D_abs(thing->x - bombspot->x);
  dy = D_abs(thing->y - bombspot->y);

  dist = dx>dy ? dx : dy;
  dist = (dist - thing->radius) >> FRACBITS;

  if (dist < 0)
  dist = 0;

  if (dist >= bombdamage)
    return true;  // out of range

  if ( P_CheckSight (thing, bombspot) )
    {
    // must be in direct path
    P_DamageMobj (thing, bombspot, bombsource, bombdamage - dist);
    }

  return true;
  }


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack(mobj_t* spot,mobj_t* source,int damage)
  {
  int x;
  int y;

  int xl;
  int xh;
  int yl;
  int yh;

  fixed_t dist;

  dist = (damage+MAXRADIUS)<<FRACBITS;
  yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
  yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
  xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
  xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
  bombspot = spot;
  bombsource = source;
  bombdamage = damage;

  for (y=yl ; y<=yh ; y++)
    for (x=xl ; x<=xh ; x++)
      P_BlockThingsIterator (x, y, PIT_RadiusAttack );
  }



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//

boolean crushchange;
boolean nofit;


//
// PIT_ChangeSector
//

boolean PIT_ChangeSector (mobj_t* thing)
  {
  mobj_t* mo;

  if (P_ThingHeightClip (thing))
    return true; // keep checking

  // crunch bodies to giblets

  if (thing->health <= 0)
    {
    P_SetMobjState (thing, S_GIBS);

    thing->flags &= ~MF_SOLID;
    thing->height = 0;
    thing->radius = 0;
    return true; // keep checking
    }

  // crunch dropped items

  if (thing->flags & MF_DROPPED)
    {
    P_RemoveMobj (thing);

    // keep checking
    return true;
    }

  if (! (thing->flags & MF_SHOOTABLE) )
    {
    // assume it is bloody gibs or something
    return true;
    }

  nofit = true;

  if (crushchange && !(leveltime&3) )
    {
    P_DamageMobj(thing,NULL,NULL,10);

    // spray blood in a random direction
    mo = P_SpawnMobj (thing->x,
                      thing->y,
                      thing->z + thing->height/2, MT_BLOOD);

    mo->momx = (P_Random(pr_crush) - P_Random (pr_crush))<<12;
    mo->momy = (P_Random(pr_crush) - P_Random (pr_crush))<<12;
    }

  // keep checking (crush other things)
  return true;
  }


//
// P_ChangeSector
//
boolean P_ChangeSector(sector_t* sector,boolean crunch)
  {
  int   x;
  int   y;

  nofit = false;
  crushchange = crunch;

  // ARRGGHHH!!!!
  // This is horrendously slow!!!
  // killough 3/14/98

  // re-check heights for all things near the moving sector

  for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
    for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
      P_BlockThingsIterator (x, y, PIT_ChangeSector);

  return nofit;
  }

//
// P_CheckSector
// jff 3/19/98 added to just check monsters on the periphery
// of a moving sector instead of all in bounding box of the
// sector. Both more accurate and faster.
//

boolean P_CheckSector(sector_t* sector,boolean crunch)
  {
  msecnode_t *n;

  if (demo_compatibility) // use the old routine for old demos though
    return P_ChangeSector(sector,crunch);

  nofit = false;
  crushchange = crunch;

  // killough 4/4/98: scan list front-to-back until empty or exhausted,
  // restarting from beginning after each thing is processed. Avoids
  // crashes, and is sure to examine all things in the sector, and only
  // the things which are in the sector, until a steady-state is reached.
  // Things can arbitrarily be inserted and removed and it won't mess up.
  //
  // killough 4/7/98: simplified to avoid using complicated counter

  // Mark all things invalid

  for (n=sector->touching_thinglist; n; n=n->m_snext)
    n->visited = false;

  do
    for (n=sector->touching_thinglist; n; n=n->m_snext)  // go through list
      if (!n->visited)               // unprocessed thing found
        {
        n->visited  = true;          // mark thing as processed
        if (!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
          PIT_ChangeSector(n->m_thing);    // process it
        break;                 // exit and start over
        }
  while (n);  // repeat from scratch until all things left are marked valid

  return nofit;
  }


// CPhipps - 
// Use block memory allocator here

#include "z_bmalloc.h"

IMPLEMENT_BLOCK_MEMORY_ALLOC_ZONE(secnodezone, sizeof(msecnode_t), PU_LEVEL, 32, "SecNodes");

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static msecnode_t* P_GetSecnode(void)
{
  return (msecnode_t*)Z_BMalloc(&secnodezone);
}

// P_PutSecnode() returns a node to the freelist.

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static void P_PutSecnode(msecnode_t* node)
{
  Z_BFree(&secnodezone, node);
}

// phares 3/16/98
//
// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.

msecnode_t* P_AddSecnode(sector_t* s, mobj_t* thing, msecnode_t* nextnode)
  {
  msecnode_t* node;

  node = nextnode;
  while (node)
    {
    if (node->m_sector == s)   // Already have a node for this sector?
      {
      node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
      return(nextnode);
      }
    node = node->m_tnext;
    }

  // Couldn't find an existing node for this sector. Add one at the head
  // of the list.

  node = P_GetSecnode();

  // killough 4/4/98, 4/7/98: mark new nodes unvisited.
  node->visited = 0;

  node->m_sector = s;       // sector
  node->m_thing  = thing;     // mobj
  node->m_tprev  = NULL;    // prev node on Thing thread
  node->m_tnext  = nextnode;  // next node on Thing thread
  if (nextnode)
    nextnode->m_tprev = node; // set back link on Thing

  // Add new node at head of sector thread starting at s->touching_thinglist

  node->m_sprev  = NULL;    // prev node on sector thread
  node->m_snext  = s->touching_thinglist; // next node on sector thread
  if (s->touching_thinglist)
    node->m_snext->m_sprev = node;
  s->touching_thinglist = node;
  return(node);
  }


// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.

msecnode_t* P_DelSecnode(msecnode_t* node)
  {
  msecnode_t* tp;  // prev node on thing thread
  msecnode_t* tn;  // next node on thing thread
  msecnode_t* sp;  // prev node on sector thread
  msecnode_t* sn;  // next node on sector thread

  if (node)
    {

    // Unlink from the Thing thread. The Thing thread begins at
    // sector_list and not from mobj_t->touching_sectorlist.

    tp = node->m_tprev;
    tn = node->m_tnext;
    if (tp)
      tp->m_tnext = tn;
    if (tn)
      tn->m_tprev = tp;

    // Unlink from the sector thread. This thread begins at
    // sector_t->touching_thinglist.

    sp = node->m_sprev;
    sn = node->m_snext;
    if (sp)
      sp->m_snext = sn;
    else
      node->m_sector->touching_thinglist = sn;
    if (sn)
      sn->m_sprev = sp;

    // Return this node to the freelist

    P_PutSecnode(node);
    return(tn);
    }
  return(NULL);
  }                             // phares 3/13/98

// Delete an entire sector list

void P_DelSeclist(msecnode_t* node)

  {
  while (node)
    node = P_DelSecnode(node);
  }


// phares 3/14/98
//
// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

boolean PIT_GetSectors(line_t* ld)
  {
  if (tmbbox[BOXRIGHT]  <= ld->bbox[BOXLEFT]   ||
      tmbbox[BOXLEFT]   >= ld->bbox[BOXRIGHT]  ||
      tmbbox[BOXTOP]    <= ld->bbox[BOXBOTTOM] ||
      tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    return true;

  if (P_BoxOnLineSide(tmbbox, ld) != -1)
    return true;

  // This line crosses through the object.

  // Collect the sector(s) from the line and add to the
  // sector_list you're examining. If the Thing ends up being
  // allowed to move to this position, then the sector_list
  // will be attached to the Thing's mobj_t at touching_sectorlist.

  sector_list = P_AddSecnode(ld->frontsector,tmthing,sector_list);

  // Don't assume all lines are 2-sided, since some Things
  // like MT_TFOG are allowed regardless of whether their radius takes
  // them beyond an impassable linedef.

  // killough 3/27/98, 4/4/98:
  // Use sidedefs instead of 2s flag to determine two-sidedness.

  if (ld->backsector)
    sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

  return true;
  }


// phares 3/14/98
//
// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.

void P_CreateSecNodeList(mobj_t* thing,fixed_t x,fixed_t y)
  {
  int xl;
  int xh;
  int yl;
  int yh;
  int bx;
  int by;
  msecnode_t* node;

  // First, clear out the existing m_thing fields. As each node is
  // added or verified as needed, m_thing will be set properly. When
  // finished, delete all nodes where m_thing is still NULL. These
  // represent the sectors the Thing has vacated.

  node = sector_list;
  while (node)
    {
    node->m_thing = NULL;
    node = node->m_tnext;
    }

  tmthing = thing;
  tmflags = thing->flags;

  tmx = x;
  tmy = y;

  tmbbox[BOXTOP]  = y + tmthing->radius;
  tmbbox[BOXBOTTOM] = y - tmthing->radius;
  tmbbox[BOXRIGHT]  = x + tmthing->radius;
  tmbbox[BOXLEFT]   = x - tmthing->radius;

  validcount++; // used to make sure we only process a line once

  xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      P_BlockLinesIterator(bx,by,PIT_GetSectors);

  // Add the sector of the (x,y) point to sector_list.

  sector_list = P_AddSecnode(thing->subsector->sector,thing,sector_list);

  // Now delete any nodes that won't be used. These are the ones where
  // m_thing is still NULL.

  node = sector_list;
  while (node)
    {
    if (node->m_thing == NULL)
      {
      if (node == sector_list)
        sector_list = node->m_tnext;
      node = P_DelSecnode(node);
      }
    else
      node = node->m_tnext;
    }
  }
