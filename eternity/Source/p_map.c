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
//  Movement, collision handling.
//  Shooting and aiming.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_map.c,v 1.35 1998/05/12 12:47:16 phares Exp $";

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
#include "r_segs.h"
#include "m_bbox.h"
#include "p_partcl.h"
#include "p_tick.h"
#include "p_user.h"
#include "d_gi.h"
#include "e_edf.h"

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

// killough 11/98: if "felldown" true, object was pushed down ledge
boolean   felldown;

// The tm* items are used to hold information globally, usually for
// line or object intersection checking

fixed_t   tmbbox[4];  // bounding box for line intersection checks
fixed_t   tmfloorz;   // floor you'd hit if free to fall
fixed_t   tmceilingz; // ceiling of sector you're in
fixed_t   tmdropoffz; // dropoff on other side of line you're crossing

#ifdef OVER_UNDER
// SoM : the floorz and ceilingz of the sectors.
fixed_t   tmsecfloorz;
fixed_t   tmsecceilz;

// SoM: Stepup floorz from a thing... Not sure if it should be used?
// set this instead of tmfloorz
fixed_t   tmstepupfloorz;

// SoM 11/6/02: UGHAH
fixed_t   tmpassfloorz;
fixed_t   tmpassceilz;
#endif

// haleyjd
int       tmfloorpic;
sector_t *tmfloorsec = NULL; // 10/16/02: floorsec

// SoM 09/07/02: Solution to problem of monsters walking on 3dsides
boolean   tmtouch3dside = false;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls

line_t        *ceilingline;
line_t        *blockline;    // killough 8/11/98: blocking linedef
line_t        *floorline;    // killough 8/1/98: Highest touched floor
static int    tmunstuck;     // killough 8/1/98: whether to allow unsticking

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid

// 1/11/98 killough: removed limit on special lines crossed
line_t **spechit;                // new code -- killough
static int spechit_max;          // killough

mobj_t *hitthing;       //sf

int numspechit;

// Temporary holder for thing_sectorlist threads
msecnode_t *sector_list = NULL;                             // phares 3/16/98

mobj_t *BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference

extern boolean reset_viewz;

//
// TELEPORT MOVE
//

//
// PIT_StompThing
//

static boolean telefrag;   // killough 8/9/98: whether to telefrag at exit

static boolean PIT_StompThing (mobj_t *thing)
{
   fixed_t blockdist;
   
   if(!(thing->flags & MF_SHOOTABLE)) // Can't shoot it? Can't stomp it!
      return true;
   
   blockdist = thing->radius + tmthing->radius;
   
   if(D_abs(thing->x - tmx) >= blockdist ||
      D_abs(thing->y - tmy) >= blockdist)
      return true; // didn't hit it
   
   // don't clip against self
   if(thing == tmthing)
      return true;
   
   // monsters don't stomp things except on boss level
   // killough 8/9/98: make consistent across all levels
   if(!telefrag)
      return false;
   
   P_DamageMobj(thing, tmthing, tmthing, 10000, MOD_TELEFRAG); // Stomp!
   
   return true;
}

//
// killough 8/28/98:
//
// P_GetFriction()
//
// Returns the friction associated with a particular mobj.

int P_GetFriction(const mobj_t *mo, int *frictionfactor)
{
   int friction = ORIG_FRICTION;
   int movefactor = ORIG_FRICTION_FACTOR;
   const msecnode_t *m;
   const sector_t *sec;

   // Assign the friction value to objects on the floor, non-floating,
   // and clipped. Normally the object's friction value is kept at
   // ORIG_FRICTION and this thinker changes it for icy or muddy floors.
   //
   // When the object is straddling sectors with the same
   // floorheight that have different frictions, use the lowest
   // friction value (muddy has precedence over icy).

   if(!(mo->flags & (MF_NOCLIP|MF_NOGRAVITY)) 
      && (demo_version >= 203 || (mo->player && !compatibility)) &&
      variable_friction)
   {
      for (m = mo->touching_sectorlist; m; m = m->m_tnext)
      {
         if((sec = m->m_sector)->special & FRICTION_MASK &&
            (sec->friction < friction || friction == ORIG_FRICTION) &&
            (mo->z <= sec->floorheight ||
             (sec->heightsec != -1 &&
              mo->z <= sectors[sec->heightsec].floorheight &&
              demo_version >= 203)))
         {
            friction = sec->friction, movefactor = sec->movefactor;
         }
      }
   }
   
   if(frictionfactor)
      *frictionfactor = movefactor;
   
   return friction;
}

// phares 3/19/98
// P_GetMoveFactor() returns the value by which the x,y
// movements are multiplied to add to player movement.
//
// killough 8/28/98: rewritten

int P_GetMoveFactor(const mobj_t *mo, int *frictionp)
{
   int movefactor, friction;
   
   // If the floor is icy or muddy, it's harder to get moving. This is where
   // the different friction factors are applied to 'trying to move'. In
   // p_mobj.c, the friction factors are applied as you coast and slow down.

   if((friction = P_GetFriction(mo, &movefactor)) < ORIG_FRICTION)
   {
      // phares 3/11/98: you start off slowly, then increase as
      // you get better footing
      
      int momentum = P_AproxDistance(mo->momx,mo->momy);
      
      if(momentum > MORE_FRICTION_MOMENTUM<<2)
         movefactor <<= 3;
      else if(momentum > MORE_FRICTION_MOMENTUM<<1)
         movefactor <<= 2;
      else if(momentum > MORE_FRICTION_MOMENTUM)
         movefactor <<= 1;
   }

   if(frictionp)
      *frictionp = friction;
   
   return movefactor;
}

//
// P_TeleportMove
//

// killough 8/9/98
boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, boolean boss)
{
   int xl, xh, yl, yh, bx, by;
   subsector_t *newsubsec;

   // killough 8/9/98: make telefragging more consistent, preserve compatibility
   // haleyjd 03/25/03: TELESTOMP flag handling moved here (was thing->player)
   telefrag = (thing->flags3 & MF3_TELESTOMP) || 
      (comp[comp_telefrag] || demo_version < 203 ? gamemap==30 : boss);

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
   #ifdef OVER_UNDER
   tmsecfloorz = tmstepupfloorz = tmpassfloorz = tmfloorz;
   tmsecceilz = tmpassceilz = tmceilingz;
   #endif

   // haleyjd
   tmfloorpic = newsubsec->sector->floorpic;

   // haleyjd 10/16/02: set tmfloorsec
   tmfloorsec = newsubsec->sector;
   
   // SoM 09/07/02: 3dsides monster fix
   tmtouch3dside = false;
   
   validcount++;
   numspechit = 0;
   
   // stomp on any things contacted
   
   xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
   xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
   yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
   yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

   for(bx=xl ; bx<=xh ; bx++)
   {
      for(by=yl ; by<=yh ; by++)
      {
         if(!P_BlockThingsIterator(bx,by,PIT_StompThing))
            return false;
      }
   }


   // the move is ok,
   // so unlink from the old position & link into the new position
   
   P_UnsetThingPosition(thing);
   
   thing->floorz = tmfloorz;
   thing->ceilingz = tmceilingz;
   thing->dropoffz = tmdropoffz;        // killough 11/98

   #ifdef OVER_UNDER
   thing->passfloorz = tmpassfloorz;
   thing->passceilz = tmpassceilz;
   thing->secfloorz = tmsecfloorz;
   thing->secceilz = tmsecceilz;
   #endif

   // haleyjd 10/16/02: set floorsec
   thing->floorsec = (int)(tmfloorsec - sectors);
   
   thing->x = x;
   thing->y = y;
   
   P_SetThingPosition(thing);
   
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
// killough 11/98: reformatted

static boolean PIT_CrossLine(line_t *ld)
{
   // SoM 9/7/02: wow a killoughism... * SoM is scared
   int flags = ML_TWOSIDED | ML_BLOCKING | ML_BLOCKMONSTERS;

   if(ld->flags & ML_3DMIDTEX)
      flags &= ~ML_BLOCKMONSTERS;

   return 
      !((ld->flags ^ ML_TWOSIDED) & flags)
      || tmbbox[BOXLEFT]   > ld->bbox[BOXRIGHT]
      || tmbbox[BOXRIGHT]  < ld->bbox[BOXLEFT]   
      || tmbbox[BOXTOP]    < ld->bbox[BOXBOTTOM]
      || tmbbox[BOXBOTTOM] > ld->bbox[BOXTOP]
      || P_PointOnLineSide(pe_x,pe_y,ld) == P_PointOnLineSide(ls_x,ls_y,ld);
}

// killough 8/1/98: used to test intersection between thing and line
// assuming NO movement occurs -- used to avoid sticky situations.

static int untouched(line_t *ld)
{
   fixed_t x, y, tmbbox[4];
   return 
     (tmbbox[BOXRIGHT] = (x=tmthing->x)+tmthing->radius) <= ld->bbox[BOXLEFT] ||
     (tmbbox[BOXLEFT] = x-tmthing->radius) >= ld->bbox[BOXRIGHT] ||
     (tmbbox[BOXTOP] = (y=tmthing->y)+tmthing->radius) <= ld->bbox[BOXBOTTOM] ||
     (tmbbox[BOXBOTTOM] = y-tmthing->radius) >= ld->bbox[BOXTOP] ||
     P_BoxOnLineSide(tmbbox, ld) != -1;
}

//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//

static boolean PIT_CheckLine(line_t *ld) // killough 3/26/98: make static
{
   if(tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
      || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
      || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
      || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
      return true; // didn't hit it

   if(P_BoxOnLineSide(tmbbox, ld) != -1)
      return true; // didn't hit it

   // A line has been hit
   
   // The moving thing's destination position will cross the given line.
   // If this should not be allowed, return false.
   // If the line is special, keep track of it
   // to process later if the move is proven ok.
   // NOTE: specials are NOT sorted by order,
   // so two special lines that are only 8 pixels apart
   // could be crossed in either order.

   // killough 7/24/98: allow player to move out of 1s wall, to prevent sticking
   if(!ld->backsector) // one sided line
   {
      blockline = ld;
      return tmunstuck && !untouched(ld) &&
         FixedMul(tmx-tmthing->x,ld->dy) > FixedMul(tmy-tmthing->y,ld->dx);
   }

   // killough 8/10/98: allow bouncing objects to pass through as missiles
   if(!(tmthing->flags & (MF_MISSILE | MF_BOUNCES)))
   {
      if(ld->flags & ML_BLOCKING)           // explicitly blocking everything
         return tmunstuck && !untouched(ld);  // killough 8/1/98: allow escape

      // killough 8/9/98: monster-blockers don't affect friends
      // SoM 9/7/02: block monsters standing on 3dmidtex only
      if(!(tmthing->flags & MF_FRIEND || tmthing->player)
         && ld->flags & ML_BLOCKMONSTERS && 
         !(ld->flags & ML_3DMIDTEX))
         return false; // block monsters only
   }

   // set openrange, opentop, openbottom
   // these define a 'window' from one sector to another across this line
   
   P_LineOpening(ld, tmthing);

   // adjust floor & ceiling heights
   
   if(opentop < tmceilingz)
   {
      tmceilingz = opentop;
      ceilingline = ld;
      blockline = ld;
   }

   if(openbottom > tmfloorz)
   {
      tmfloorz = openbottom;
      floorline = ld;          // killough 8/1/98: remember floor linedef
      blockline = ld;

      // haleyjd
      tmfloorsec = openfloorsec;
   }

   if(lowfloor < tmdropoffz)
      tmdropoffz = lowfloor;

#ifdef OVER_UNDER
   if(opensecfloor > tmsecfloorz)
      tmsecfloorz = opensecfloor;
   if(opensecceil < tmsecceilz)
      tmsecceilz = opensecceil;

   // SoM 11/6/02: AGHAH
   if(tmfloorz > tmpassfloorz)
      tmpassfloorz = tmfloorz;
   if(tmceilingz < tmpassceilz)
      tmpassceilz = tmceilingz;
#endif

   // if contacted a special line, add it to the list
   
   if(ld->special)
   {
      // 1/11/98 killough: remove limit on lines hit, by array doubling
      if(numspechit >= spechit_max)
      {
         spechit_max = spechit_max ? spechit_max*2 : 8;
         spechit = realloc(spechit,sizeof *spechit*spechit_max); // killough
      }
      spechit[numspechit++] = ld;
   }
   
   return true;
}

// haleyjd 04/01/03: test routines for new experimental
// over/under kernel -- these routines carefully select which
// things are allowed to initiate and respond to over/under
// movement -- a player must now be involved in order for the
// movement to be allowed. This cleans up a lot of problems.

#ifdef OVER_UNDER
// returns true if moving thing can move over/under other things
static boolean P_Is3DMover(mobj_t *mover, mobj_t *blocker)
{
   // player is always 3D mover unless in noclipping or a corpse
   if(mover->player && !(mover->flags & (MF_NOCLIP|MF_CORPSE)))
      return true;

   // only solid things are 3D movers
   if(mover->flags & MF_SOLID)
   {
      // things which are not 3D movers, even if solid:
      // Missiles
      // Specials
      // Corpses
      // NoClipping things
      // Touchy things
      // Bouncers
      // Flying skulls
      if(!(mover->flags & (MF_MISSILE|MF_BOUNCES|MF_SPECIAL|MF_CORPSE|MF_NOCLIP|MF_TOUCHY|MF_SKULLFLY)))
      {
         // only a 3D mover if being blocked by player
         if(blocker->player)
            return true;
      }
   }

   return false;
}

// returns true if the blocking thing can be moved over/under
static boolean P_Is3DBlocker(mobj_t *blocker)
{
   // player is always 3D blocker unless in noclipping or a corpse
   if(blocker->player && !(blocker->flags & (MF_NOCLIP|MF_CORPSE)))
      return true;

   // if blocker is not solid, it is normal
   if(!(blocker->flags & MF_SOLID))
      return false;

   // things which are restricted to normal clipping:
   // Missiles
   // Bouncers
   // Specials
   // Corpses
   // NoClipping things
   // Touchy things
   // Flying skulls
   if(blocker->flags & (MF_MISSILE|MF_BOUNCES|MF_SPECIAL|MF_CORPSE|MF_NOCLIP|MF_TOUCHY|MF_SKULLFLY))
      return false;

   return true;
}

// returns true if the mover can step up on the blocker
static boolean P_CanStepUpOn(mobj_t *mover, mobj_t *blocker)
{
   if(!mover->player)
      return false; // only players can step up

   // things which cannot be stepped up on by players
   // Missiles
   // Bouncers
   // Specials
   // Corpses
   // NoClipping things
   // Touchy things
   // Flying skulls
   if(blocker->flags & (MF_MISSILE|MF_BOUNCES|MF_SPECIAL|MF_CORPSE|MF_NOCLIP|MF_TOUCHY|MF_SKULLFLY))
      return false;

   return true;
}
#endif

//
// PIT_CheckThing
// 
static boolean PIT_CheckThing(mobj_t *thing) // killough 3/26/98: make static
{
   fixed_t blockdist;
   int damage;

   // EDF FIXME: haleyjd 07/13/03: these may be temporary fixes
   static int painType    = -1;
   static int skullType   = -1;
   static int bruiserType = -1;
   static int knightType  = -1;

   if(painType == -1)
   {
      painType    = E_ThingNumForDEHNum(MT_PAIN);
      skullType   = E_ThingNumForDEHNum(MT_SKULL);
      bruiserType = E_ThingNumForDEHNum(MT_BRUISER);
      knightType  = E_ThingNumForDEHNum(MT_KNIGHT);
   }

   hitthing = tmthing;
   
   // killough 11/98: add touchy things
   if(!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE|MF_TOUCHY)))
      return true;

   blockdist = thing->radius + tmthing->radius;
   
   if(D_abs(thing->x - tmx) >= blockdist ||
      D_abs(thing->y - tmy) >= blockdist)
      return true; // didn't hit it

   // killough 11/98:
   //
   // This test has less information content (it's almost always false), so it
   // should not be moved up to first, as it adds more overhead than it removes.
   
   // don't clip against self
   
   if(thing == tmthing)
      return true;
/*
   #ifdef OVER_UNDER
   // SoM: 10/21/02
   // SoM  11/4/02: So yeah, MF_SPECIAL checks...

   if(demo_version >= 331 && !comp[comp_overunder] &&
      (thing->flags & MF_SOLID) && (tmthing->flags & MF_SOLID) &&
      !(thing->flags & (MF_MISSILE|MF_SPECIAL)) && 
      !(tmthing->flags & (MF_MISSILE|MF_SPECIAL)))
   {
      if(tmthing->z >= thing->z + thing->height)
      {
         if(thing->z + thing->height > tmfloorz)
            tmfloorz = thing->z + thing->height;
         
         return true;
      }
      else if(thing->z + thing->height - tmthing->z <= (24 << FRACBITS) &&
         !(thing->flags & (MF_CORPSE|MF_SKULLFLY)) &&
         !(tmthing->flags & (MF_SKULLFLY|MF_FLOAT)))
      {
         // SoM: Set tmstepupfloorz instead
         if(thing->z + thing->height > tmfloorz)
            tmstepupfloorz = thing->z + thing->height;

         return true;
      }
      else if(tmthing->z + tmthing->height <= thing->z)
      {
         if(thing->z < tmceilingz)
            tmceilingz = thing->z;
         
         return true;
      }
   }
   #endif
*/
#ifdef OVER_UNDER
   // haleyjd 04/01/03: new experimental over/under kernel
   if(demo_version >= 331 && !comp[comp_overunder] &&
      P_Is3DMover(tmthing, thing) && P_Is3DBlocker(thing))
   {
      fixed_t blockerhz = thing->z + thing->height;

      if(tmthing->z >= blockerhz)
      {
         // mover is over blocker
         if(blockerhz > tmfloorz)
            tmfloorz = blockerhz;
         
         return true;
      }
      else if(blockerhz - tmthing->z <= 24*FRACUNIT &&
         P_CanStepUpOn(tmthing, thing))
      {
         // mover can step up on blocker
         if(blockerhz > tmfloorz)
            tmstepupfloorz = blockerhz;
         
         return true;
      }
      else if(tmthing->z + tmthing->height <= thing->z)
      {
         // mover is under blocker
         if(thing->z < tmceilingz)
            tmceilingz = thing->z;

         return true;
      }
   }
#endif

   // haleyjd 1/17/00: set global hit reference
   BlockingMobj = thing;

   // killough 11/98:
   //
   // TOUCHY flag, for mines or other objects which die on contact with solids.
   // If a solid object of a different type comes in contact with a touchy
   // thing, and the touchy thing is not the sole one moving relative to fixed
   // surroundings such as walls, then the touchy thing dies immediately.

   if(thing->flags & MF_TOUCHY &&                  // touchy object
      tmthing->flags & MF_SOLID &&                 // solid object touches it
      thing->health > 0 &&                         // touchy object is alive
      (thing->intflags & MIF_ARMED ||              // Thing is an armed mine
       sentient(thing)) &&                         // ... or a sentient thing
      (thing->type != tmthing->type ||             // only different species
       thing->player) &&                           // ... or different players
      thing->z + thing->height >= tmthing->z &&    // touches vertically
      tmthing->z + tmthing->height >= thing->z &&
      (thing->type ^ painType) |                   // PEs and lost souls
      (tmthing->type ^ skullType) &&               // are considered same
      (thing->type ^ skullType) |                  // (but Barons & Knights
      (tmthing->type ^ painType))                  // are intentionally not)
   {
      P_DamageMobj(thing, NULL, NULL, thing->health, MOD_UNKNOWN); // kill object
      return true;
   }

   // check for skulls slamming into things
   
   if(tmthing->flags & MF_SKULLFLY)
   {
      // A flying skull is smacking something.
      // Determine damage amount, and the skull comes to a dead stop.
      int damage;
      
      damage = ((P_Random(pr_skullfly)%8)+1)*tmthing->info->damage;
      
      P_DamageMobj(thing, tmthing, tmthing, damage, MOD_UNKNOWN);
      
      tmthing->flags &= ~MF_SKULLFLY;
      tmthing->momx = tmthing->momy = tmthing->momz = 0;
      
      P_SetMobjState (tmthing, tmthing->info->spawnstate);
      
      return false;   // stop moving
   }

   // missiles can hit other things
   // killough 8/10/98: bouncing non-solid things can hit other things too
   
   if(tmthing->flags & MF_MISSILE || (tmthing->flags & MF_BOUNCES &&
                                      !(tmthing->flags & MF_SOLID)))
   {
      // haleyjd: some missiles can go through ghosts
      if(thing->flags3 & MF3_GHOST &&
         tmthing->flags3 & MF3_THRUGHOST)
         return true;

      // see if it went over / under
      
      if(tmthing->z > thing->z + thing->height)
         return true;    // overhead
      
      if(tmthing->z+tmthing->height < thing->z)
         return true;    // underneath

      if(tmthing->target &&
         (tmthing->target->type == thing->type ||
          (tmthing->target->type == knightType && thing->type == bruiserType)||
          (tmthing->target->type == bruiserType && thing->type == knightType)))
      {
         if(thing == tmthing->target)
            return true;                // Don't hit same species as originator.
         else if(!(thing->player))      // Explode, but do no damage.
            return false;               // Let players missile other players.
      }
      
      // killough 8/10/98: if moving thing is not a missile, no damage
      // is inflicted, and momentum is reduced if object hit is solid.

      if(!(tmthing->flags & MF_MISSILE))
      {
         if(!(thing->flags & MF_SOLID))
            return true;
         else
         {
            tmthing->momx = -tmthing->momx;
            tmthing->momy = -tmthing->momy;
            if(!(tmthing->flags & MF_NOGRAVITY))
            {
               tmthing->momx >>= 2;
               tmthing->momy >>= 2;
            }
            return false;
         }
      }

      if(!(thing->flags & MF_SHOOTABLE))
         return !(thing->flags & MF_SOLID); // didn't do any damage

      // damage / explode
      
      damage = ((P_Random(pr_damage)%8)+1)*tmthing->info->damage;
      P_DamageMobj(thing, tmthing, tmthing->target, damage,
                   tmthing->info->mod);

      // don't traverse any more
      return false;
   }

   // haleyjd 1/16/00: Pushable objects -- at last!
   //   This is remarkably simpler than I had anticipated!
   
   if(demo_version >= 329 && thing->flags2 & MF2_PUSHABLE)
   {
      // transfer one-fourth momentum along the x and y axes
      thing->momx += tmthing->momx >> 2;
      thing->momy += tmthing->momy >> 2;
   }

   // check for special pickup
   
   if(thing->flags & MF_SPECIAL)
   {
      int solid = thing->flags & MF_SOLID;
      if(tmflags & MF_PICKUP)
         P_TouchSpecialThing(thing, tmthing); // can remove thing
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

boolean Check_Sides(mobj_t *actor, int x, int y)
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
   for(bx = xl ; bx <= xh ; bx++)
      for (by = yl ; by <= yh ; by++)
         if(!P_BlockLinesIterator(bx,by,PIT_CrossLine))
            return true;                                          //   ^
   return false;                                                  //   |
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

boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y) 
{
   int xl, xh, yl, yh, bx, by;
   subsector_t *newsubsec;
   
   tmthing = thing;
   tmflags = thing->flags;
   
   tmx = x;
   tmy = y;
   
   tmbbox[BOXTOP] = y + tmthing->radius;
   tmbbox[BOXBOTTOM] = y - tmthing->radius;
   tmbbox[BOXRIGHT] = x + tmthing->radius;
   tmbbox[BOXLEFT] = x - tmthing->radius;
   
   newsubsec = R_PointInSubsector(x,y);
   floorline = blockline = ceilingline = NULL; // killough 8/1/98

   // Whether object can get out of a sticky situation:
   tmunstuck = thing->player &&          // only players
      thing->player->mo == thing &&       // not voodoo dolls
      demo_version >= 203;                // not under old demos

   // The base floor / ceiling is from the subsector
   // that contains the point.
   // Any contacted lines the step closer together
   // will adjust them.

   tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
   tmceilingz = newsubsec->sector->ceilingheight;

   #ifdef OVER_UNDER
   tmsecfloorz = tmpassfloorz = tmstepupfloorz = tmfloorz;
   tmsecceilz = tmpassceilz = tmceilingz;
   #endif

   // haleyjd
   tmfloorpic = newsubsec->sector->floorpic;
   // haleyjd 10/16/02: tmfloorsec
   tmfloorsec = newsubsec->sector;
   // SoM: 09/07/02: 3dsides monster fix
   tmtouch3dside = false;
   validcount++;
   numspechit = 0;

   if(tmflags & MF_NOCLIP)
   {
      return true;
   }

   // Check things first, possibly picking things up.
   // The bounding box is extended by MAXRADIUS
   // because mobj_ts are grouped into mapblocks
   // based on their origin point, and can overlap
   // into adjacent blocks by up to MAXRADIUS units.

   xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
   xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
   yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
   yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference

   for(bx=xl ; bx<=xh ; bx++)
   {
      for(by=yl ; by<=yh ; by++)
      {
         if(!P_BlockThingsIterator(bx,by,PIT_CheckThing))
            return false;
      }
   }

   // check lines
   
   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference
   
   xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
   xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
   yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
   yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

   for(bx=xl ; bx<=xh ; bx++)
   {
      for(by=yl ; by<=yh ; by++)
      {
         if(!P_BlockLinesIterator(bx,by,PIT_CheckLine))
            return false; // doesn't fit
      }
   }

#ifdef OVER_UNDER
   if(demo_version >= 331 && !comp[comp_overunder])
   {
      // Check the stepupfloorz

      if(tmstepupfloorz > tmfloorz)
      { 
         if(tmceilingz - tmstepupfloorz >= tmthing->height)
            tmfloorz = tmstepupfloorz;
         else
            return false;
      }
   }
#endif


   return true;
}

#ifdef OVER_UNDER

//
// P_CheckPositionMobjOnly
//
// Does a clipping check for an object, comparing it only against
// other objects and not against lines or sector floor/ceiling
// heights.  This is needed for the over/under coordinate update
// at the start of P_ZMovement.
//
boolean P_CheckPositionMobjOnly(mobj_t *thing, fixed_t x, fixed_t y) 
{
   int xl, xh, yl, yh, bx, by;
   
   tmthing = thing;
   tmflags = thing->flags;
   
   tmx = x;
   tmy = y;
   
   tmbbox[BOXTOP] = y + tmthing->radius;
   tmbbox[BOXBOTTOM] = y - tmthing->radius;
   tmbbox[BOXRIGHT] = x + tmthing->radius;
   tmbbox[BOXLEFT] = x - tmthing->radius;
   
   // Whether object can get out of a sticky situation:
   tmunstuck = thing->player &&          // only players
      thing->player->mo == thing &&       // not voodoo dolls
      demo_version >= 203;                // not under old demos

   // The base floor / ceiling is from the subsector
   // that contains the point.
   // Any contacted lines the step closer together
   // will adjust them.

   tmfloorz = tmdropoffz = thing->floorz;
   tmceilingz = thing->ceilingz;

   tmsecfloorz = tmpassfloorz = tmstepupfloorz = tmfloorz;
   tmsecceilz = tmpassceilz = tmceilingz;

   validcount++;

   if(tmflags & MF_NOCLIP)
   {
      return true;
   }

   // Check things first, possibly picking things up.
   // The bounding box is extended by MAXRADIUS
   // because mobj_ts are grouped into mapblocks
   // based on their origin point, and can overlap
   // into adjacent blocks by up to MAXRADIUS units.

   xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
   xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
   yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
   yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference

   for(bx=xl ; bx<=xh ; bx++)
   {
      for(by=yl ; by<=yh ; by++)
      {
         if(!P_BlockThingsIterator(bx,by,PIT_CheckThing))
            return false;
      }
   }

   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference

   if(demo_version >= 331 && !comp[comp_overunder])
   {
      // Check the stepupfloorz

      if(tmstepupfloorz > tmfloorz)
      { 
         if(tmceilingz - tmstepupfloorz >= tmthing->height)
            tmfloorz = tmstepupfloorz;
         else
            return false;
      }
   }

   return true;
}

static boolean P_ThingMovexy(mobj_t *thing, fixed_t oldx, 
                             fixed_t oldy);
#endif 

//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
// killough 3/15/98: allow dropoff as option

boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean dropoff)
{
   fixed_t oldx, oldy;
   
   felldown = floatok = false;               // killough 11/98
   
   if(!P_CheckPosition(thing, x, y))
      return false;   // solid wall or thing

   if(!(thing->flags & MF_NOCLIP))
   {
      // killough 7/26/98: reformatted slightly
      // killough 8/1/98: Possibly allow escape if otherwise stuck

      if(tmceilingz - tmfloorz < thing->height ||     // doesn't fit
         // mobj must lower to fit
         (floatok = true, !(thing->flags & MF_TELEPORT) &&
          tmceilingz - thing->z < thing->height) ||
         // too big a step up
         (!(thing->flags & MF_TELEPORT) &&
          !(thing->flags3 & MF3_FLOORMISSILE) &&  // haleyjd 03/27/99
          tmfloorz - thing->z > 24*FRACUNIT))
      {
         return tmunstuck 
            && !(ceilingline && untouched(ceilingline))
            && !(  floorline && untouched(  floorline));
      }
      
      // killough 3/15/98: Allow certain objects to drop off
      // killough 7/24/98, 8/1/98: 
      // Prevent monsters from getting stuck hanging off ledges
      // killough 10/98: Allow dropoffs in controlled circumstances
      // killough 11/98: Improve symmetry of clipping on stairs

      if(!(thing->flags & (MF_DROPOFF|MF_FLOAT)) &&
         (demo_version < 331 || !tmtouch3dside)) // SoM
      {
         if(demo_version >= 331 && tmfloorz < thing->z && 
            (thing->z - tmfloorz) > 24*FRACUNIT)
         {
            // SoM 09/07/02: 3d lines monster fix
            if(thing->z == thing->floorz)
               return false;
         }
         else if(comp[comp_dropoff])
         {
            if (tmfloorz - tmdropoffz > 24*FRACUNIT)
               return false;                      // don't stand over a dropoff
         }
         else
         {
            if(!dropoff || 
               (dropoff==2 &&  // large jump down (e.g. dogs)
                (tmfloorz-tmdropoffz > 128*FRACUNIT || 
                 !thing->target || thing->target->z >tmdropoffz)))
            {
               fixed_t zdist;

               if(!monkeys || demo_version < 203)
                  zdist = tmfloorz - tmdropoffz;
               else
                  zdist = thing->floorz - tmfloorz;
               
               if(zdist > 24*FRACUNIT ||
                  thing->dropoffz - tmdropoffz > 24*FRACUNIT)                    
                  return false;
            }
            else  // dropoff allowed -- check for whether it fell more than 24
            {
               felldown = !(thing->flags & MF_NOGRAVITY) &&
                  thing->z - tmfloorz > 24*FRACUNIT;
            }
         }
      }

      if(thing->flags & MF_BOUNCES &&    // killough 8/13/98
         !(thing->flags & (MF_MISSILE|MF_NOGRAVITY)) &&
         !sentient(thing) && tmfloorz - thing->z > 16*FRACUNIT)
         return false; // too big a step up for bouncers under gravity

      // killough 11/98: prevent falling objects from going up too many steps
      if(thing->intflags & MIF_FALLING && tmfloorz - thing->z >
         FixedMul(thing->momx,thing->momx)+FixedMul(thing->momy,thing->momy))
         return false;

      // haleyjd: CANTLEAVEFLOORPIC flag
      if((thing->flags2 & MF2_CANTLEAVEFLOORPIC) &&
         (tmfloorpic != thing->subsector->sector->floorpic ||
          tmfloorz - thing->z != 0))
         // thing must stay within its current floor type
         return false;
   }

   // the move is ok,
   // so unlink from the old position and link into the new position

   P_UnsetThingPosition (thing);
   
   oldx = thing->x;
   oldy = thing->y;
   thing->floorz = tmfloorz;
   thing->ceilingz = tmceilingz;
   thing->dropoffz = tmdropoffz;      // killough 11/98: keep track of dropoffs
   #ifdef OVER_UNDER
   thing->passfloorz = tmpassfloorz;
   thing->passceilz = tmpassceilz;
   thing->secfloorz = tmsecfloorz;
   thing->secceilz = tmsecceilz;
   #endif

   thing->x = x;
   thing->y = y;
   // haleyjd
   thing->floorsec = tmfloorsec ? (int)(tmfloorsec - sectors) : -1;
   
   P_SetThingPosition(thing);

#ifdef OVER_UNDER
   // SoM 11/4/02: Adjust floorz/ceilingz of things where this obj
   // used to stand.
   if(demo_version >= 331 && !comp[comp_overunder])
      P_ThingMovexy(thing, oldx, oldy);
#endif

   // haleyjd 10/12/02: set footclip flag
   if(demo_version >= 331 && !comp[comp_terrain] &&
      (thing->flags2 & MF2_FOOTCLIP) &&
      P_TerrainFloorClip(P_GetSecTerrainType(thing->subsector->sector,0)))
   {
      thing->intflags |= MIF_FOOTCLIP;
   }
   else
      thing->intflags &= ~MIF_FOOTCLIP;

   // if any special lines were hit, do the effect
   // killough 11/98: simplified

   if(!(thing->flags & (MF_TELEPORT | MF_NOCLIP)))
   {
      while(numspechit--)
      {
         if(spechit[numspechit]->special)  // see if the line was crossed
         {
            int oldside;
            if((oldside = P_PointOnLineSide(oldx, oldy, spechit[numspechit])) !=
               P_PointOnLineSide(thing->x, thing->y, spechit[numspechit]))
               P_CrossSpecialLine(spechit[numspechit], oldside, thing);
         }
      }
   }
   
   return true;
}

//
// killough 9/12/98:
//
// Apply "torque" to objects hanging off of ledges, so that they
// fall off. It's not really torque, since Doom has no concept of
// rotation, but it's a convincing effect which avoids anomalies
// such as lifeless objects hanging more than halfway off of ledges,
// and allows objects to roll off of the edges of moving lifts, or
// to slide up and then back down stairs, or to fall into a ditch.
// If more than one linedef is contacted, the effects are cumulative,
// so balancing is possible.
//

static boolean PIT_ApplyTorque(line_t *ld)
{
   if(ld->backsector &&       // If thing touches two-sided pivot linedef
      tmbbox[BOXRIGHT]  > ld->bbox[BOXLEFT]  &&
      tmbbox[BOXLEFT]   < ld->bbox[BOXRIGHT] &&
      tmbbox[BOXTOP]    > ld->bbox[BOXBOTTOM] &&
      tmbbox[BOXBOTTOM] < ld->bbox[BOXTOP] &&
      P_BoxOnLineSide(tmbbox, ld) == -1)
   {
      mobj_t *mo = tmthing;

      fixed_t dist =                               // lever arm
         + (ld->dx >> FRACBITS) * (mo->y >> FRACBITS)
         - (ld->dy >> FRACBITS) * (mo->x >> FRACBITS) 
         - (ld->dx >> FRACBITS) * (ld->v1->y >> FRACBITS)
         + (ld->dy >> FRACBITS) * (ld->v1->x >> FRACBITS);

      if(dist < 0 ?                               // dropoff direction
         ld->frontsector->floorheight < mo->z &&
         ld->backsector->floorheight >= mo->z :
         ld->backsector->floorheight < mo->z &&
         ld->frontsector->floorheight >= mo->z)
      {
         // At this point, we know that the object straddles a two-sided
         // linedef, and that the object's center of mass is above-ground.

         fixed_t x = D_abs(ld->dx), y = D_abs(ld->dy);

         if(y > x)
         {
            fixed_t t = x;
            x = y;
            y = t;
         }

         y = finesine[(tantoangle[FixedDiv(y,x)>>DBITS] +
                      ANG90) >> ANGLETOFINESHIFT];

         // Momentum is proportional to distance between the
         // object's center of mass and the pivot linedef.
         //
         // It is scaled by 2^(OVERDRIVE - gear). When gear is
         // increased, the momentum gradually decreases to 0 for
         // the same amount of pseudotorque, so that oscillations
         // are prevented, yet it has a chance to reach equilibrium.

         dist = FixedDiv(FixedMul(dist, (mo->gear < OVERDRIVE) ?
                 y << -(mo->gear - OVERDRIVE) :
                 y >> +(mo->gear - OVERDRIVE)), x);

         // Apply momentum away from the pivot linedef.
                 
         x = FixedMul(ld->dy, dist);
         y = FixedMul(ld->dx, dist);

         // Avoid moving too fast all of a sudden (step into "overdrive")

         dist = FixedMul(x,x) + FixedMul(y,y);

         while(dist > FRACUNIT*4 && mo->gear < MAXGEAR)
            ++mo->gear, x >>= 1, y >>= 1, dist >>= 1;
         
         mo->momx -= x;
         mo->momy += y;
      }
   }
   return true;
}

//
// killough 9/12/98
//
// Applies "torque" to objects, based on all contacted linedefs
//

void P_ApplyTorque(mobj_t *mo)
{
   int xl = ((tmbbox[BOXLEFT] = 
              mo->x - mo->radius) - bmaporgx) >> MAPBLOCKSHIFT;
   int xh = ((tmbbox[BOXRIGHT] = 
              mo->x + mo->radius) - bmaporgx) >> MAPBLOCKSHIFT;
   int yl = ((tmbbox[BOXBOTTOM] =
              mo->y - mo->radius) - bmaporgy) >> MAPBLOCKSHIFT;
   int yh = ((tmbbox[BOXTOP] = 
      mo->y + mo->radius) - bmaporgy) >> MAPBLOCKSHIFT;
   int bx,by,flags = mo->intflags; //Remember the current state, for gear-change

   tmthing = mo;
   validcount++; // prevents checking same line twice
      
   for(bx = xl ; bx <= xh ; bx++)
      for(by = yl ; by <= yh ; by++)
         P_BlockLinesIterator(bx, by, PIT_ApplyTorque);
      
   // If any momentum, mark object as 'falling' using engine-internal flags
   if (mo->momx | mo->momy)
      mo->intflags |= MIF_FALLING;
   else  // Clear the engine-internal flag indicating falling object.
      mo->intflags &= ~MIF_FALLING;

   // If the object has been moving, step up the gear.
   // This helps reach equilibrium and avoid oscillations.
   //
   // Doom has no concept of potential energy, much less
   // of rotation, so we have to creatively simulate these 
   // systems somehow :)

   if(!((mo->intflags | flags) & MIF_FALLING))  // If not falling for a while,
      mo->gear = 0;                             // Reset it to full strength
   else if(mo->gear < MAXGEAR)                  // Else if not at max gear,
      mo->gear++;                               // move up a gear
}




#ifdef OVER_UNDER

// SoM 10/28/02: Moved this for new 3d object clipping code
static boolean crushchange, nofit;

boolean P_ThingMovez(mobj_t *thing, fixed_t zmove)
{
   mobj_t *mobj;
   fixed_t box1[4], blockdist, thingzl, thingzh;
   int xl, yl, xh, yh, x, y;

   box1[BOXTOP]    = thing->y + thing->radius;
   box1[BOXBOTTOM] = thing->y - thing->radius;
   box1[BOXRIGHT]  = thing->x + thing->radius;
   box1[BOXLEFT]   = thing->x - thing->radius;

   xl = (box1[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
   xh = (box1[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
   yl = (box1[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
   yh = (box1[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

   thing->z += zmove;

   // SoM 11/4/02: ok! First off, if a mobj is special it should NEVER
   // be clipped against other mobjs. For some reason checking the
   // solid flag was not enough. Secondly, a special thing should 
   // never be crushed unless it was dropped.
   if(thing->flags & MF_SPECIAL && !(thing->flags & MF_DROPPED))
      return true;

   thingzl = thing->z;
   thingzh = thingzl + thing->height;

   if(thingzl < thing->floorz || thingzh > thing->ceilingz)
      return false;

   if(thing->flags & (MF_NOCLIP | MF_DROPPED))
      return true;

   // Bad formatting but I don't want this to get indented off the screen :/
   for(y = yl; y <= yh; y++) for(x = xl; x <= xh; x++)
   {
      if(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
         continue;

      for(mobj = blocklinks[y*bmapwidth+x]; mobj; mobj = mobj->bnext)
      {
         // SoM 11/4/02: uh ehehe, DON'T clip against MF_SPECIALs DUH
         if(!(mobj->flags & MF_SOLID))
            continue;

         if(mobj->flags & (MF_SPECIAL | MF_CORPSE | MF_NOCLIP))
            continue;

         blockdist = thing->radius + mobj->radius;
   
         if(D_abs(mobj->x - thing->x) >= blockdist ||
            D_abs(mobj->y - thing->y) >= blockdist)
            continue; // didn't hit it

         if(mobj == thing)
            continue;

         if(!zmove || (thing->flags & MF_CORPSE))
            goto moveok;

         // There is an intersection
         if(zmove > 0)
         {
            // Moving up. Check things above because they may need
            // to be moved up as well...
            if(mobj->z + mobj->height <= thingzl || mobj->z >= thingzh)
               goto moveok;

            if(mobj->z > thingzl && mobj->z + zmove == thingzh)
            {
               if(!P_ThingMovez(mobj, zmove))
                  return false;
               goto moveok;
            }

            if(mobj->z + zmove >= thingzh)
               goto moveok;

            return false;
         }
         else
         {
            // Moving down. Things standing on top will need to be lowered
            // and things below may need to be pushed down.
            if(mobj->z + mobj->height <= thingzl)
               goto moveok;

            if(mobj->z + mobj->height > thingzl && mobj->z + 
               mobj->height + zmove <= thingzl)
            {
               if(!P_ThingMovez(mobj, thingzl - (mobj->z + mobj->height)))
                  return false;
               goto moveok;
            }

            if((mobj->intflags & MIF_ONFLOOR) && 
               mobj->z + zmove == thingzh)
            {
               P_ThingMovez(mobj, zmove);
               goto moveok;
            }

            if(mobj->z >= thingzh)
               goto moveok;

            return false;
         }

         moveok:

         if(thingzl >= mobj->z + mobj->height)
         {
            if(mobj->z + mobj->height > thing->floorz)
               thing->floorz = mobj->z + mobj->height;
         }
         else if(thingzh <= mobj->z)
         {
            // SoM 11/29/02: For corpses never set a ceiling from
            // another thing.
            if(!(thing->flags & MF_CORPSE) && mobj->z < thing->ceilingz)
               thing->ceilingz = mobj->z;
         }
      }
   }

   // Test once again with new floor and ceilingz
   if(thingzl < thing->floorz || thingzh > thing->ceilingz)
      return false;

   thing->intflags &= ~(MIF_ONSECFLOOR|MIF_ONFLOOR);
   if(thing->z == thing->secfloorz)
      thing->intflags |= MIF_ONSECFLOOR;
   else if(thing->z == thing->floorz)
      thing->intflags |= MIF_ONFLOOR;

   return true;
}

static boolean P_ThingMovexy(mobj_t *thing, fixed_t oldx, fixed_t oldy)
{
   mobj_t *mobj;
   fixed_t box1[4], blockdist;
   int xl, yl, xh, yh, x, y;
   
   
   box1[BOXTOP]    = oldy + thing->radius;
   box1[BOXBOTTOM] = oldy - thing->radius;
   box1[BOXRIGHT]  = oldx + thing->radius;
   box1[BOXLEFT]   = oldx - thing->radius;
   
   xl = (box1[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
   xh = (box1[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
   yl = (box1[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
   yh = (box1[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
   
   // Bad formatting but I don't want this to get indented off the screen :/
   for(y = yl; y <= yh; y++) for(x = xl; x <= xh; x++)
   {
      if(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
         continue;
      
      for(mobj = blocklinks[y*bmapwidth+x]; mobj; mobj = mobj->bnext)
      {
         if(!(mobj->flags & MF_SOLID))
         {
            // Non-solid things fall to the floor.
            mobj->floorz = mobj->passfloorz;
            mobj->ceilingz = mobj->passceilz;

            continue;
         }
                  
         blockdist = thing->radius + mobj->radius;
         
         if(D_abs(mobj->x - oldx) >= blockdist ||
            D_abs(mobj->y - oldy) >= blockdist)
            continue; // didn't hit it

         if(D_abs(mobj->x - thing->x) < blockdist &&
            D_abs(mobj->y - thing->y) < blockdist)
            continue; // currently still hitting it

         if(mobj == thing)
            continue;
         
         mobj->floorz = mobj->passfloorz;
         mobj->ceilingz = mobj->passceilz;
         
         P_ThingMovez(mobj, 0);
      }
   }
      
   return true;
}


// SoM: UGH WHY?
static boolean PIT_ChangeSector(mobj_t *thing);

static void P_AdjustThing(mobj_t *thing)
{
   boolean tnofit = false;

   if(thing->floorz > thing->z || (thing->intflags & MIF_ONFLOOR))
   {
      P_ThingMovez(thing, 0);
      
      // SoM 11/29/02: Don't movez 0 twice!
      if(thing->floorz != thing->z && 
         !P_ThingMovez(thing, thing->floorz - thing->z))
         tnofit = true;
   }
   else if(thing->intflags & MIF_ONSECFLOOR)
   {
      P_ThingMovez(thing, 0);

      // SoM 11/29/02: Don't movez 0 twice!
      if(thing->secfloorz != thing->z && 
         !P_ThingMovez(thing, thing->secfloorz - thing->z))
         tnofit = true;
   }
   else if(thing->z + thing->height > thing->ceilingz)
   {
      if(!P_ThingMovez(thing, thing->ceilingz - (thing->z + thing->height)))
         tnofit = true;
   }
   else
   {
      if(!P_ThingMovez(thing, 0))
         tnofit = true;
   }

   if(tnofit)
      PIT_ChangeSector(thing);
}



// SoM 10/23/02: Function that only runs through the lines.
// P_CheckLines
//
static void P_CheckLines(mobj_t *thing)
{
   int xl, xh, yl, yh, bx, by;
   subsector_t *newsubsec;
   
   tmthing = thing;
   tmflags = thing->flags;
   
   tmx = thing->x;
   tmy = thing->y;
   
   tmbbox[BOXTOP] = tmy + tmthing->radius;
   tmbbox[BOXBOTTOM] = tmy - tmthing->radius;
   tmbbox[BOXRIGHT] = tmx + tmthing->radius;
   tmbbox[BOXLEFT] = tmx - tmthing->radius;
   
   newsubsec = R_PointInSubsector(tmx,tmy);
   floorline = blockline = ceilingline = NULL; // killough 8/1/98

   // Whether object can get out of a sticky situation:
   tmunstuck = thing->player &&          // only players
      thing->player->mo == thing &&       // not voodoo dolls
      demo_version >= 203;                // not under old demos

   // The base floor / ceiling is from the subsector
   // that contains the point.
   // Any contacted lines the step closer together
   // will adjust them.

   tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
   tmceilingz = newsubsec->sector->ceilingheight;
   tmsecfloorz = tmpassfloorz = tmstepupfloorz = tmfloorz;
   tmsecceilz = tmpassceilz = tmceilingz;

   // haleyjd
   tmfloorpic = newsubsec->sector->floorpic;
   // haleyjd 10/16/02: tmfloorsec
   tmfloorsec = newsubsec->sector;
   // SoM: 09/07/02: 3dsides monster fix
   tmtouch3dside = false;
   validcount++;
   numspechit = 0;

   if(tmflags & MF_NOCLIP)
   {
      // SoM 11/4/02: Yeah... stuff...
      thing->floorz = tmfloorz;
      thing->ceilingz = tmceilingz;
      thing->dropoffz = tmdropoffz;
      thing->secfloorz = tmsecfloorz;
      thing->secceilz = tmsecceilz;
      thing->passfloorz = tmpassfloorz;
      thing->passceilz = tmpassceilz;
      return;
   }

   // check lines
   
   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference
   
   xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
   xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
   yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
   yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

   for(bx=xl ; bx<=xh ; bx++)
   {
      for(by=yl ; by<=yh ; by++)
      {
         // SoM 12/2/02: Checkline shouldn't be used to determine
         // success or failure.
         P_BlockLinesIterator(bx,by,PIT_CheckLine);
      }
   }

   thing->intflags &= ~(MIF_ONFLOOR|MIF_ONSECFLOOR);

   if(thing->z == thing->secfloorz)
   {
      thing->intflags |= MIF_ONSECFLOOR;
   }
   else if(thing->z == thing->floorz)
   {
      thing->intflags |= MIF_ONFLOOR;
   }

   thing->floorz = tmfloorz;
   thing->ceilingz = tmceilingz;
   thing->secfloorz = tmsecfloorz;
   thing->secceilz = tmsecceilz;
   thing->dropoffz = tmdropoffz;
   thing->passfloorz = tmpassfloorz;
   thing->passceilz = tmpassceilz;

   if(thing->ceilingz - thing->floorz < thing->height)
   {
      if(!PIT_ChangeSector(thing))
         return; // doesn't fit
   }
}

#endif



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

static boolean P_ThingHeightClip(mobj_t *thing)
{
   boolean onfloor = thing->z == thing->floorz;

   P_CheckPosition(thing, thing->x, thing->y);

  
   // what about stranding a monster partially off an edge?
   // killough 11/98: Answer: see below (upset balance if hanging off ledge)
   
   thing->floorz = tmfloorz;
   thing->ceilingz = tmceilingz;
   thing->dropoffz = tmdropoffz;         // killough 11/98: remember dropoffs

   #ifdef OVER_UNDER
   thing->passfloorz = tmpassfloorz;
   thing->passceilz = tmpassceilz;
   thing->secfloorz = tmsecfloorz;
   thing->secceilz = tmsecceilz;
   #endif

   // haleyjd
   thing->floorsec = tmfloorsec ? (int)(tmfloorsec - sectors) : -1;

   if(onfloor)  // walking monsters rise and fall with the floor
   {
      thing->z = thing->floorz;
      
      // killough 11/98: Possibly upset balance of objects hanging off ledges
      if(thing->intflags & MIF_FALLING && thing->gear >= MAXGEAR)
         thing->gear = 0;
   }
   else          // don't adjust a floating monster unless forced to
      if(thing->z + thing->height > thing->ceilingz)
         thing->z = thing->ceilingz - thing->height;

   return thing->ceilingz - thing->floorz >= thing->height;
}

//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//

// killough 8/2/98: make variables static
static fixed_t   bestslidefrac;
static fixed_t   secondslidefrac;
static line_t    *bestslideline;
static line_t    *secondslideline;
static mobj_t    *slidemo;
static fixed_t   tmxmove;
static fixed_t   tmymove;

//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
// If the floor is icy, then you can bounce off a wall.             // phares
//

static void P_HitSlideLine(line_t *ld)
{
   int     side;
   angle_t lineangle;
   angle_t moveangle;
   angle_t deltaangle;
   fixed_t movelen;
   fixed_t newlen;
   boolean icyfloor;  // is floor icy?

   // phares:
   // Under icy conditions, if the angle of approach to the wall
   // is more than 45 degrees, then you'll bounce and lose half
   // your momentum. If less than 45 degrees, you'll slide along
   // the wall. 45 is arbitrary and is believable.
   //
   // Check for the special cases of horz or vert walls.

   // killough 10/98: only bounce if hit hard (prevents wobbling)
   icyfloor = 
      (demo_version >= 203 ? 
       P_AproxDistance(tmxmove, tmymove) > 4*FRACUNIT : !compatibility) &&
      variable_friction &&  // killough 8/28/98: calc friction on demand
      slidemo->z <= slidemo->floorz &&
      P_GetFriction(slidemo, NULL) > ORIG_FRICTION;

   if(ld->slopetype == ST_HORIZONTAL)
   {
      if(icyfloor && D_abs(tmymove) > D_abs(tmxmove))
      {
         // haleyjd: only the player should oof
         if(slidemo->player)
            S_StartSound(slidemo, sfx_oof); // oooff!
         tmxmove /= 2; // absorb half the momentum
         tmymove = -tmymove/2;
      }
      else
         tmymove = 0; // no more movement in the Y direction
      return;
   }

   if(ld->slopetype == ST_VERTICAL)
   {
      if(icyfloor && D_abs(tmxmove) > D_abs(tmymove))
      {
         // haleyjd: only the player should oof
         if(slidemo->player)
            S_StartSound(slidemo, sfx_oof); // oooff!
         tmxmove = -tmxmove/2; // absorb half the momentum
         tmymove /= 2;
      }
      else // phares
         tmxmove = 0; // no more movement in the X direction
      return;
   }

   // The wall is angled. Bounce if the angle of approach is  // phares
   // less than 45 degrees.

   side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);
   
   lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);
   if(side == 1)
      lineangle += ANG180;
   moveangle = R_PointToAngle2 (0,0, tmxmove, tmymove);

   // killough 3/2/98:
   // The moveangle+=10 breaks v1.9 demo compatibility in
   // some demos, so it needs demo_compatibility switch.
   
   if(!demo_compatibility)
      moveangle += 10;
   // ^ prevents sudden path reversal due to rounding error // phares

   deltaangle = moveangle-lineangle;
   movelen = P_AproxDistance (tmxmove, tmymove);

   if(icyfloor && deltaangle > ANG45 && deltaangle < ANG90+ANG45)
   {
      // haleyjd: only the player should oof
      if(slidemo->player)
         S_StartSound(slidemo,sfx_oof); // oooff!
      moveangle = lineangle - deltaangle;
      movelen /= 2; // absorb
      moveangle >>= ANGLETOFINESHIFT;
      tmxmove = FixedMul (movelen, finecosine[moveangle]);
      tmymove = FixedMul (movelen, finesine[moveangle]);
   }
   else
   {
      if(deltaangle > ANG180)
         deltaangle += ANG180;
      
      //  I_Error ("SlideLine: ang>ANG180");
      
      lineangle >>= ANGLETOFINESHIFT;
      deltaangle >>= ANGLETOFINESHIFT;
      newlen = FixedMul (movelen, finecosine[deltaangle]);
      tmxmove = FixedMul (newlen, finecosine[lineangle]);
      tmymove = FixedMul (newlen, finesine[lineangle]);
   }
}


//
// PTR_SlideTraverse
//

static boolean PTR_SlideTraverse(intercept_t *in)
{
   line_t *li;
   
#ifdef RANGECHECK
   if(!in->isaline)
      I_Error ("PTR_SlideTraverse: not a line?");
#endif

   li = in->d.line;
   
   if(!(li->flags & ML_TWOSIDED))
   {
      if(P_PointOnLineSide (slidemo->x, slidemo->y, li))
         return true; // don't hit the back side
      goto isblocking;
   }

   // set openrange, opentop, openbottom.
   // These define a 'window' from one sector to another across a line
   
   P_LineOpening(li, slidemo);
   
   if(openrange < slidemo->height)
      goto isblocking;  // doesn't fit
   
   if(opentop - slidemo->z < slidemo->height)
      goto isblocking;  // mobj is too high
   
   if(openbottom - slidemo->z > 24*FRACUNIT )
      goto isblocking;  // too big a step up

   // this line doesn't block movement
   
   return true;
   
   // the line does block movement,
   // see if it is closer than best so far

isblocking:
   
   if(in->frac < bestslidefrac)
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
// killough 11/98: reformatted

void P_SlideMove(mobj_t *mo)
{
   int hitcount = 3;
   
   slidemo = mo; // the object that's sliding
   
   do
   {
      fixed_t leadx, leady, trailx, traily;

      if(!--hitcount)
         goto stairstep;   // don't loop forever

      // trace along the three leading corners
      
      if(mo->momx > 0)
         leadx = mo->x + mo->radius, trailx = mo->x - mo->radius;
      else
         leadx = mo->x - mo->radius, trailx = mo->x + mo->radius;

      if(mo->momy > 0)
         leady = mo->y + mo->radius, traily = mo->y - mo->radius;
      else
         leady = mo->y - mo->radius, traily = mo->y + mo->radius;

      bestslidefrac = FRACUNIT+1;
      
      P_PathTraverse(leadx, leady, leadx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse);
      P_PathTraverse(trailx, leady, trailx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse);
      P_PathTraverse(leadx, traily, leadx+mo->momx, traily+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse);

      // move up to the wall

      if(bestslidefrac == FRACUNIT+1)
      {
         // the move must have hit the middle, so stairstep
         
         stairstep:
      
         // killough 3/15/98: Allow objects to drop off ledges
         //
         // phares 5/4/98: kill momentum if you can't move at all
         // This eliminates player bobbing if pressed against a wall
         // while on ice.
         //
         // killough 10/98: keep buggy code around for old Boom demos
         
         // haleyjd: yet another compatibility fix by cph -- the
         // fix is only necessary for boom v2.01

         if(!P_TryMove(mo, mo->x, mo->y + mo->momy, true))
            if(!P_TryMove(mo, mo->x + mo->momx, mo->y, true))	      
               if(demo_version == 201)
                  mo->momx = mo->momy = 0;
               
         break;
      }

      // fudge a bit to make sure it doesn't hit
      
      if ((bestslidefrac -= 0x800) > 0)
      {
         fixed_t newx = FixedMul(mo->momx, bestslidefrac);
         fixed_t newy = FixedMul(mo->momy, bestslidefrac);
         
         // killough 3/15/98: Allow objects to drop off ledges
         
         if(!P_TryMove(mo, mo->x+newx, mo->y+newy, true))
            goto stairstep;
      }

      // Now continue along the wall.
      // First calculate remainder.
      
      bestslidefrac = FRACUNIT-(bestslidefrac+0x800);
      
      if(bestslidefrac > FRACUNIT)
         bestslidefrac = FRACUNIT;
      
      if(bestslidefrac <= 0)
         break;

      tmxmove = FixedMul(mo->momx, bestslidefrac);
      tmymove = FixedMul(mo->momy, bestslidefrac);
      
      P_HitSlideLine(bestslideline); // clip the moves
      
      mo->momx = tmxmove;
      mo->momy = tmymove;
      
      // killough 10/98: affect the bobbing the same way (but not voodoo dolls)
      if(mo->player && mo->player->mo == mo)
      {
         if(D_abs(mo->player->momx) > D_abs(tmxmove))
            mo->player->momx = tmxmove;
         if(D_abs(mo->player->momy) > D_abs(tmymove))
            mo->player->momy = tmymove;
      }
   }  // killough 3/15/98: Allow objects to drop off ledges:
   while(!P_TryMove(mo, mo->x+tmxmove, mo->y+tmymove, true));
}

//
// P_LineAttack
//
mobj_t *linetarget; // who got hit (or NULL)
static mobj_t *shootthing;

static int aim_flags_mask; // killough 8/2/98: for more intelligent autoaiming

fixed_t shootz;  // Height if not aiming up or down
fixed_t startz;  // SoM: Height of shot origin
static int la_damage;
fixed_t attackrange;

fixed_t   aimslope;

// slopes to top and bottom of target
// killough 4/20/98: make static instead of using ones in p_sight.c

static fixed_t  topslope;
static fixed_t  bottomslope;

//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
static boolean PTR_AimTraverse (intercept_t *in)
{
   fixed_t slope, thingtopslope, thingbottomslope, dist;
   line_t *li;
   mobj_t *th;
   
   if(in->isaline)
   {
      li = in->d.line;
      
      if(!(li->flags & ML_TWOSIDED))
         return false;   // stop

      // Crosses a two sided line.
      // A two sided line will restrict
      // the possible target ranges.
      
      P_LineOpening (li, NULL);
      
      if(openbottom >= opentop)
         return false;   // stop

      dist = FixedMul (attackrange, in->frac);
      
      if(li->frontsector->floorheight != li->backsector->floorheight)
      {
         slope = FixedDiv (openbottom - shootz , dist);
         if(slope > bottomslope)
            bottomslope = slope;
      }

      if(li->frontsector->ceilingheight != li->backsector->ceilingheight)
      {
         slope = FixedDiv (opentop - shootz , dist);
         if(slope < topslope)
            topslope = slope;
      }

      if(topslope <= bottomslope)
         return false;   // stop
      
      return true;    // shot continues
   }

   // shoot a thing
   
   th = in->d.thing;
   if(th == shootthing)
      return true;    // can't shoot self

   if(!(th->flags&MF_SHOOTABLE))
      return true;    // corpse or something

   // killough 7/19/98, 8/2/98:
   // friends don't aim at friends (except players), at least not first
   if(th->flags & shootthing->flags & aim_flags_mask && !th->player)
      return true;

   // check angles to see if the thing can be aimed at
   
   dist = FixedMul(attackrange, in->frac);
   thingtopslope = FixedDiv(th->z+th->height - shootz , dist);

   if(thingtopslope < bottomslope)
      return true;    // shot over the thing
   
   thingbottomslope = FixedDiv (th->z - shootz, dist);
   
   if(thingbottomslope > topslope)
      return true;    // shot under the thing

   // this thing can be hit!
   
   if(thingtopslope > topslope)
      thingtopslope = topslope;
   
   if(thingbottomslope < bottomslope)
      thingbottomslope = bottomslope;
   
   aimslope = (thingtopslope+thingbottomslope)/2;
   linetarget = th;
   
   return false;   // don't go any farther
}

//
// PTR_ShootTraverse
//
// haleyjd 11/21/01: fixed by SoM to allow bullets to puff on the
// floors and ceilings rather than along the line which they actually
// intersected far below or above the ceiling.
//
static boolean PTR_ShootTraverse(intercept_t *in)
{
   fixed_t slope, dist, thingtopslope, thingbottomslope, x, y, z, frac;
   mobj_t *th;
   boolean hitplane = false; // SoM: Remember if the bullet hit a plane.
   int updown = 2; // haleyjd 05/02: particle puff z dist correction
   
   if(in->isaline)
   {
      line_t *li = in->d.line;
      
      // SoM: Shouldn't be called until A: we know the bullet passed or
      // B: We know it didn't hit a plane first
      if(li->special && demo_version < 329)
         P_ShootSpecialLine (shootthing, li);
      
      if(li->flags & ML_TWOSIDED)
      {  // crosses a two sided (really 2s) line
         P_LineOpening (li, NULL);
         dist = FixedMul(attackrange, in->frac);
         
         // killough 11/98: simplify
         
         if((li->frontsector->floorheight==li->backsector->floorheight ||
            (slope = FixedDiv(openbottom - shootz , dist)) <= aimslope) &&
            (li->frontsector->ceilingheight==li->backsector->ceilingheight ||
            (slope = FixedDiv (opentop - shootz , dist)) >= aimslope))
         {
            if(li->special && demo_version >= 329)
               P_ShootSpecialLine (shootthing, li);
            
            return true;      // shot continues
         }
      }
      
      // hit line
      // position a bit closer

      frac = in->frac - FixedDiv(4*FRACUNIT,attackrange);
      x = trace.x + FixedMul(trace.dx, frac);
      y = trace.y + FixedMul(trace.dy, frac);
      z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

      if(demo_version >= 329)
      {
         // SoM: Check for colision with a plane.
         int        lineside = P_PointOnLineSide(shootthing->x, shootthing->y, li);
         sector_t*  sidesector = lineside ? li->backsector : li->frontsector;
         fixed_t    zdiff;

         // SoM: If we are in no-clip and are shooting on the backside of a
         // 1s line, don't crash!
         if(sidesector)
         {
            if(z < sidesector->floorheight)
            {
               if(sidesector->floorpic == skyflatnum ||
                  sidesector->floorpic == sky2flatnum
#ifdef R_PORTALS
                  || sidesector->f_portal
#endif
                  )
                  return false;
               zdiff = FixedDiv(D_abs(z - sidesector->floorheight),
                                D_abs(z - startz));
               x += FixedMul(trace.x - x, zdiff);
               y += FixedMul(trace.y - y, zdiff);
               z = sidesector->floorheight;
               hitplane = true;
               updown = 0; // haleyjd
            }
            else if(z > sidesector->ceilingheight)
            {
               if(sidesector->ceilingpic == skyflatnum ||
                  sidesector->ceilingpic == sky2flatnum
#ifdef R_PORTALS
                  || sidesector->c_portal
#endif
                  )
                  return false;
               zdiff = FixedDiv(D_abs(z - sidesector->ceilingheight),
                                D_abs(z - startz));
               x += FixedMul(trace.x - x, zdiff);
               y += FixedMul(trace.y - y, zdiff);
               z = sidesector->ceilingheight;
               hitplane = true;
               updown = 1; // haleyjd
            }
         }

         if(!hitplane && li->special)
            P_ShootSpecialLine (shootthing, li);
      }

      if(li->frontsector->ceilingpic == skyflatnum ||
         li->frontsector->ceilingpic == sky2flatnum
#ifdef R_PORTALS
         || li->frontsector->c_portal
#endif
         )
      {
         // don't shoot the sky!
         
         if(z > li->frontsector->ceilingheight)
            return false;
         
         // it's a sky hack wall
         // fix bullet-eaters -- killough:
         if(li->backsector && 
            (li->backsector->ceilingpic == skyflatnum ||
            li->backsector->ceilingpic == sky2flatnum))
         {
            if(demo_compatibility ||
               li->backsector->ceilingheight < z)
               return false;
         }
      }
      
#ifdef R_PORTALS
      if(!hitplane && li->portal)
         return false;
#endif
      
      // Spawn bullet puffs.
      
      P_SpawnPuff(x, y, z, 
                  R_PointToAngle2(0, 0, li->dx, li->dy) - ANG90,
                  updown, true);
      
      // don't go any farther
      
      return false;
   }
   
   // shoot a thing
   
   th = in->d.thing;
   if(th == shootthing)
      return true;  // can't shoot self
   
   if(!(th->flags&MF_SHOOTABLE))
      return true;  // corpse or something

   // haleyjd: don't let players use melee attacks on ghosts
   if((th->flags3 & MF3_GHOST) && shootthing->player &&
      (shootthing->player->readyweapon == wp_fist ||
       shootthing->player->readyweapon == wp_chainsaw))
   {
      return true;
   }
   
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
   
   // Spawn bullet puffs or blood spots,
   // depending on target type. -- haleyjd: and status flags!
   if(th->flags & MF_NOBLOOD || 
      th->flags2 & (MF2_INVULNERABLE | MF2_DORMANT))
   {
      P_SpawnPuff(x, y, z, 
                  R_PointToAngle2(0, 0, trace.dx, trace.dy) - ANG180,
                  2, true);
   }
   else
   {
      P_SpawnBlood(x, y, z,
                   R_PointToAngle2(0, 0, trace.dx, trace.dy) - ANG180,
                   la_damage, th);
   }
   
   if(la_damage)
      P_DamageMobj(th, shootthing, shootthing, la_damage,
                   shootthing->info->mod);
   
   // don't go any farther
   return false;
}

//
// P_AimLineAttack
//
// killough 8/2/98: add mask parameter, which, if set to MF_FRIEND,
// makes autoaiming skip past friends.

fixed_t P_AimLineAttack(mobj_t *t1,angle_t angle,fixed_t distance,int mask)
{
   fixed_t x2, y2;
   
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

   // killough 8/2/98: prevent friends from aiming at friends
   aim_flags_mask = mask;
   
   P_PathTraverse(t1->x,t1->y,x2,y2,PT_ADDLINES|PT_ADDTHINGS,PTR_AimTraverse);
   
   if(linetarget)
      return aimslope;
   
   if(t1->player)
      return t1->player->updownangle * LOOKSLOPE;
   
   return 0;
}

//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//

void P_LineAttack(mobj_t *t1, angle_t angle, fixed_t distance,
                  fixed_t slope, int damage)
{
   fixed_t x2, y2;
   
   angle >>= ANGLETOFINESHIFT;
   shootthing = t1;
   la_damage = damage;
   x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
   y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
   
   shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;   
   if(demo_version >= 331 && !comp[comp_terrain] &&
      t1->intflags & MIF_FOOTCLIP &&
      P_CheckClipView(t1))
   {
      shootz -= FOOTCLIPHEIGHT;
   }
   startz = shootz; // SoM: record this

   attackrange = distance;
   aimslope = slope;
   P_PathTraverse(t1->x,t1->y,x2,y2,PT_ADDLINES|PT_ADDTHINGS,
                  PTR_ShootTraverse);
}

//
// USE LINES
//

static mobj_t *usething;

// killough 11/98: reformatted
// haleyjd  09/02: reformatted again.

static boolean PTR_UseTraverse(intercept_t *in)
{
   if(in->d.line->special)
   {
      P_UseSpecialLine(usething, in->d.line,
         P_PointOnLineSide(usething->x, usething->y,in->d.line)==1);

      //WAS can't use for than one special line in a row
      //jff 3/21/98 NOW multiple use allowed with enabling line flag
      return (!demo_compatibility && in->d.line->flags & ML_PASSUSE);
   }
   else
   {
      P_LineOpening(in->d.line, NULL);
      if(openrange <= 0)
      {
         // can't use through a wall
         S_StartSound(usething, sfx_noway);
         return false;
      }
      // not a special line, but keep checking
      return true;
   }

/*
  return in->d.line->special ?
    P_UseSpecialLine(usething, in->d.line, 
		     P_PointOnLineSide(usething->x,usething->y,in->d.line)==1),

    //WAS can't use for than one special line in a row
    //jff 3/21/98 NOW multiple use allowed with enabling line flag
    
    !demo_compatibility && in->d.line->flags & ML_PASSUSE :

    (P_LineOpening(in->d.line, NULL), openrange <= 0) ?

    // can't use through a wall / not a special line, but keep checking

    S_StartSound (usething, sfx_noway), false : true;
*/
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

static boolean PTR_NoWayTraverse(intercept_t *in)
{
   line_t *ld = in->d.line;                       // This linedef

   return ld->special ||                          // Ignore specials
     !(ld->flags & ML_BLOCKING ||                 // Always blocking
       (P_LineOpening(ld, NULL),                  // Find openings
        openrange <= 0 ||                         // No opening
        openbottom > usething->z+24*FRACUNIT ||   // Too high it blocks
        opentop < usething->z+usething->height)); // Too low it blocks
}

//
// P_UseLines
// Looks for special lines in front of the player to activate.
//

void P_UseLines(player_t *player)
{
   fixed_t x1, y1, x2, y2;
   int angle;
   
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
   
   if(P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse))
      if(!P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse))
         S_StartSound (usething, sfx_noway);
}

//
// RADIUS ATTACK
//

static mobj_t *bombsource, *bombspot;
static int bombdamage;
static int bombmod; // haleyjd 07/13/03

//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//

static boolean PIT_RadiusAttack(mobj_t *thing)
{
   fixed_t dx, dy, dist;

   // EDF FIXME: temporary solution
   static int cyberType = -1;
   
   if(cyberType == -1)
      cyberType = E_ThingNumForDEHNum(MT_CYBORG);
   
   // killough 8/20/98: allow bouncers to take damage 
   // (missile bouncers are already excluded with MF_NOBLOCKMAP)
   
   if(!(thing->flags & (MF_SHOOTABLE | MF_BOUNCES)))
      return true;
   
   // Boss spider and cyborg
   // take no damage from concussion.
   
   // killough 8/10/98: allow grenades to hurt anyone, unless
   // fired by Cyberdemons, in which case it won't hurt Cybers.
   // haleyjd 05/22/99: exclude all bosses

   if(bombspot->flags & MF_BOUNCES ?
      thing->type == cyberType && bombsource->type == cyberType :
      thing->flags2 & MF2_BOSS)
      return true;

   dx = D_abs(thing->x - bombspot->x);
   dy = D_abs(thing->y - bombspot->y);
   dist = dx>dy ? dx : dy;
   dist = (dist - thing->radius) >> FRACBITS;

   if(dist < 0)
      dist = 0;

   if(dist >= bombdamage)
      return true;  // out of range

   if(P_CheckSight(thing, bombspot))      // must be in direct path
      P_DamageMobj(thing, bombspot, bombsource, bombdamage - dist, bombmod);
   
   return true;
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//   haleyjd 07/13/03: added method of death flag
//

void P_RadiusAttack(mobj_t *spot, mobj_t *source, int damage, int mod)
{
   fixed_t dist = (damage+MAXRADIUS)<<FRACBITS;
   int yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
   int yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
   int xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
   int xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
   int x, y;

   bombspot = spot;
   bombsource = source;
   bombdamage = damage;
   bombmod = mod;       // haleyjd
   
   for(y=yl ; y<=yh ; y++)
      for(x=xl ; x<=xh ; x++)
         P_BlockThingsIterator(x, y, PIT_RadiusAttack);
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

#ifdef OVER_UNDER
// SoM 10/28/02: Moved this for new 3d object clipping code
#else
static boolean crushchange, nofit;
#endif
//
// PIT_ChangeSector
//

static boolean PIT_ChangeSector(mobj_t *thing)
{
   mobj_t *mo;
   

#ifdef OVER_UNDER
   // SoM: with the new over_under scheme, this will only
   // be called when a thing does not fit.
   if(demo_version < 331 || comp[comp_overunder])
#endif
      if(P_ThingHeightClip(thing))
         return true; // keep checking
   
   // crunch bodies to giblets
   
   if(thing->health <= 0)
   {
      // sf: clear the skin which will mess things up
      // haleyjd 03/11/03: not in heretic
      if(!(gameModeInfo->flags & GIF_HERETIC))
      {
         thing->skin = NULL;
         P_SetMobjState(thing, E_SafeState(S_GIBS));
      }
      thing->flags &= ~MF_SOLID;
      thing->height = thing->radius = 0;
      return true;      // keep checking
   }

   // crunch dropped items
   
   if(thing->flags & MF_DROPPED)
   {
      P_RemoveMobj(thing);
      return true;      // keep checking
   }

   // killough 11/98: kill touchy things immediately
   if(thing->flags & MF_TOUCHY &&
      (thing->intflags & MIF_ARMED || sentient(thing)))
   {
      // kill object
      P_DamageMobj(thing, NULL, NULL, thing->health, MOD_CRUSH);
      return true;   // keep checking
   }

#ifdef OVER_UNDER
   if(demo_version >= 331 && !comp[comp_overunder])
   {
      if(!((thing->flags|tmthing->flags) & MF_SOLID) || 
         (thing->flags|tmthing->flags) & MF_SPECIAL)
         return true;
   }
   else
#endif
   if(!(thing->flags & MF_SHOOTABLE))
      return true;        // assume it is bloody gibs or something

   nofit = true;
   
   // haleyjd 6/19/00: fix for invulnerable things -- no crusher effects

   if (crushchange && !(leveltime&3))
   {
      int t;         // killough 8/10/98
      
      if(thing->flags2 & MF2_INVULNERABLE ||
         thing->flags2 & MF2_DORMANT)
         return true;

      P_DamageMobj(thing,NULL,NULL,10,MOD_CRUSH);
      
      // spray blood in a random direction
      mo = P_SpawnMobj (thing->x,
                        thing->y,
                        thing->z + thing->height/2, 
                        E_SafeThingType(MT_BLOOD));

      // killough 8/10/98: remove dependence on order of evaluation
      t = P_Random(pr_crush);
      mo->momx = (t - P_Random (pr_crush))<<12;
      t = P_Random(pr_crush);
      mo->momy = (t - P_Random (pr_crush))<<12;

      if(drawparticles && bloodsplat_particle)
      {
         angle_t an;
         an = (M_Random () - 128) << 24;
         if(bloodsplat_particle != 2)
            mo->flags2 |= MF2_DONTDRAW;
         P_DrawSplash2(32, thing->x, thing->y, 
                       thing->z + thing->height/2, an, 2, 
                       thing->info->bloodcolor | MBC_BLOODMASK); 
      }
   }

   // keep checking (crush other things)
   return true;
}

//
// P_ChangeSector
//
// haleyjd: removed static; needed in p_floor.c
//
boolean P_ChangeSector(sector_t *sector,boolean crunch)
{
   int x, y;
   
   nofit = false;
   crushchange = crunch;
   
   // ARRGGHHH!!!!
   // This is horrendously slow!!!
   // killough 3/14/98
   
   // re-check heights for all things near the moving sector

   for(x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
      for(y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
         P_BlockThingsIterator(x, y, PIT_ChangeSector);
      
   return nofit;
}

//
// P_CheckSector
// jff 3/19/98 added to just check monsters on the periphery
// of a moving sector instead of all in bounding box of the
// sector. Both more accurate and faster.
//

boolean P_CheckSector(sector_t *sector,boolean crunch)
{
   msecnode_t *n;
   
   // killough 10/98: sometimes use Doom's method
   if(comp[comp_floors] && (demo_version >= 203 || demo_compatibility))
      return P_ChangeSector(sector,crunch);
   
   nofit = false;
   crushchange = crunch;

   #ifdef OVER_UNDER
   if(!comp[comp_overunder] && demo_version >= 331)
   {
      // SoM: Multi-pass method:
      // Start by accumulating information about the mobj's
      // new surroundings (i.e. floors/ceilings)
      // then check things after every thing has it's set floorz and
      // ceilingz.
      // Mark all things invalid
   
      for(n=sector->touching_thinglist; n; n=n->m_snext)
         n->visited = false;

      do
      {
         for(n=sector->touching_thinglist; n; n=n->m_snext)
         {
            if(!n->visited)               // unprocessed thing found
            {
               n->visited  = true;          // mark thing as processed
               if(!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
                  P_CheckLines(n->m_thing);    // process it
               break;                 // exit and start over
            }
         }
      }
      while (n);  // repeat from scratch until all things left are marked valid


      for(n=sector->touching_thinglist; n; n=n->m_snext)
         n->visited = false;

      do
      {
         for(n=sector->touching_thinglist; n; n=n->m_snext)
         {
            if(!n->visited)               // unprocessed thing found
            {
               n->visited  = true;          // mark thing as processed
               if(!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
               {
                  P_AdjustThing(n->m_thing);    // process it
               }
               break;                 // exit and start over
            }
         }
      }
      while (n);  // repeat from scratch until all things left are marked valid
   }
   else
   #endif
   {
   // killough 4/4/98: scan list front-to-back until empty or exhausted,
   // restarting from beginning after each thing is processed. Avoids
   // crashes, and is sure to examine all things in the sector, and only
   // the things which are in the sector, until a steady-state is reached.
   // Things can arbitrarily be inserted and removed and it won't mess up.
   //
   // killough 4/7/98: simplified to avoid using complicated counter

   // Mark all things invalid
   
   for(n=sector->touching_thinglist; n; n=n->m_snext)
      n->visited = false;

   do
   {
      for(n=sector->touching_thinglist; n; n=n->m_snext)  // go through list
      {
         if(!n->visited)               // unprocessed thing found
         {
            n->visited  = true;          // mark thing as processed
            if(!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
               PIT_ChangeSector(n->m_thing);    // process it
            break;                 // exit and start over
         }
      }
   }
   while (n);  // repeat from scratch until all things left are marked valid
   }
   
   return nofit;
}

// phares 3/21/98
//
// Maintain a freelist of msecnode_t's to reduce memory allocs and frees.

msecnode_t *headsecnode = NULL;

// sf: fix annoying crash on restarting levels
//
//      This crash occurred because the msecnode_t's are allocated as
//      PU_LEVEL. This meant that these were freed whenever a new level
//      was loaded or the level restarted. However, the actual list was
//      not emptied. As a result, msecnode_t's were being used that had
//      been freed back to the zone memory. I really do not understand
//      why boom or MBF never got this bug- or am I missing something?
//
//      additional comment 5/7/99
//      There _is_ code to free the list in g_game.c. But this is called
//      too late: some msecnode_t's are used during the loading of the
//      level. 

void P_FreeSecNodeList()
{
   headsecnode = NULL; // this is all thats needed to fix the bug
}

// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
//
// killough 11/98: reformatted

static msecnode_t *P_GetSecnode(void)
{
   msecnode_t *node;
   
   return headsecnode ?
      node = headsecnode, headsecnode = node->m_snext, node :
      Z_Malloc(sizeof *node, PU_LEVEL, NULL); 
}

// P_PutSecnode() returns a node to the freelist.

static void P_PutSecnode(msecnode_t *node)
{
   node->m_snext = headsecnode;
   headsecnode = node;
}

// phares 3/16/98
//
// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
//
// killough 11/98: reformatted

static msecnode_t *P_AddSecnode(sector_t *s, mobj_t *thing, 
                                msecnode_t *nextnode)
{
   msecnode_t *node;
   
   for (node = nextnode; node; node = node->m_tnext)
   {
      if (node->m_sector == s)   // Already have a node for this sector?
      {
         node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
         return nextnode;
      }
   }

   // Couldn't find an existing node for this sector. Add one at the head
   // of the list.
   
   node = P_GetSecnode();
   
   node->visited = 0;  // killough 4/4/98, 4/7/98: mark new nodes unvisited.

   node->m_sector = s;         // sector
   node->m_thing  = thing;     // mobj
   node->m_tprev  = NULL;      // prev node on Thing thread
   node->m_tnext  = nextnode;  // next node on Thing thread
   
   if(nextnode)
      nextnode->m_tprev = node; // set back link on Thing

   // Add new node at head of sector thread starting at s->touching_thinglist
   
   node->m_sprev  = NULL;    // prev node on sector thread
   node->m_snext  = s->touching_thinglist; // next node on sector thread
   if(s->touching_thinglist)
      node->m_snext->m_sprev = node;
   return s->touching_thinglist = node;
}

// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
//
// killough 11/98: reformatted

static msecnode_t *P_DelSecnode(msecnode_t *node)
{
   if(node)
   {
      msecnode_t *tp = node->m_tprev;  // prev node on thing thread
      msecnode_t *tn = node->m_tnext;  // next node on thing thread
      msecnode_t *sp;  // prev node on sector thread
      msecnode_t *sn;  // next node on sector thread

      // Unlink from the Thing thread. The Thing thread begins at
      // sector_list and not from mobj_t->touching_sectorlist.

      if(tp)
         tp->m_tnext = tn;
      
      if(tn)
         tn->m_tprev = tp;

      // Unlink from the sector thread. This thread begins at
      // sector_t->touching_thinglist.
      
      sp = node->m_sprev;
      sn = node->m_snext;
      
      if(sp)
         sp->m_snext = sn;
      else
         node->m_sector->touching_thinglist = sn;
      
      if(sn)
         sn->m_sprev = sp;

      // Return this node to the freelist
      
      P_PutSecnode(node);
      
      node = tn;
   }
   return node;
}

// Delete an entire sector list

void P_DelSeclist(msecnode_t *node)
{
   while(node)
      node = P_DelSecnode(node);
}

// phares 3/14/98
//
// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

static boolean PIT_GetSectors(line_t *ld)
{
   if(tmbbox[BOXRIGHT]  <= ld->bbox[BOXLEFT]   ||
      tmbbox[BOXLEFT]   >= ld->bbox[BOXRIGHT]  ||
      tmbbox[BOXTOP]    <= ld->bbox[BOXBOTTOM] ||
      tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
      return true;

   if(P_BoxOnLineSide(tmbbox, ld) != -1)
      return true;

   // This line crosses through the object.
   
   // Collect the sector(s) from the line and add to the
   // sector_list you're examining. If the Thing ends up being
   // allowed to move to this position, then the sector_list
   // will be attached to the Thing's mobj_t at touching_sectorlist.

   sector_list = P_AddSecnode(ld->frontsector,tmthing,sector_list);

   // Don't assume all lines are 2-sided, since some Things
   // like teleport fog are allowed regardless of whether their 
   // radius takes them beyond an impassable linedef.
   
   // killough 3/27/98, 4/4/98:
   // Use sidedefs instead of 2s flag to determine two-sidedness.
   // killough 8/1/98: avoid duplicate if same sector on both sides

   if(ld->backsector && ld->backsector != ld->frontsector)
      sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);
   
   return true;
}

// phares 3/14/98
//
// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.
//
// killough 11/98: reformatted
//
// haleyjd 01/02/00: added cph's fix to stop clobbering the tmthing
//  global variable (whose stupid idea was it to use that?)

void P_CreateSecNodeList(mobj_t *thing,fixed_t x,fixed_t y)
{
   int xl, xh, yl, yh, bx, by;
   msecnode_t *node;
   mobj_t *saved_tmthing = tmthing; // haleyjd
   fixed_t saved_tmx = tmx, saved_tmy = tmy;
   int saved_tmflags = tmflags;

   fixed_t saved_bbox[4]; // haleyjd

   // SoM says tmbbox needs to be saved too -- don't bother if
   // not needed for compatibility though
   if(demo_version < 200 || demo_version >= 331)
      memcpy(saved_bbox, tmbbox, 4*sizeof(fixed_t));

   // First, clear out the existing m_thing fields. As each node is
   // added or verified as needed, m_thing will be set properly. When
   // finished, delete all nodes where m_thing is still NULL. These
   // represent the sectors the Thing has vacated.
   
   for(node = sector_list; node; node = node->m_tnext)
      node->m_thing = NULL;

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

   for(bx=xl ; bx<=xh ; bx++)
      for(by=yl ; by<=yh ; by++)
         P_BlockLinesIterator(bx,by,PIT_GetSectors);

   // Add the sector of the (x,y) point to sector_list.

   sector_list = 
      P_AddSecnode(thing->subsector->sector,thing,sector_list);

   // Now delete any nodes that won't be used. These are the ones where
   // m_thing is still NULL.
   
   for(node = sector_list; node;)
   {
      if(node->m_thing == NULL)
      {
         if(node == sector_list)
            sector_list = node->m_tnext;
         node = P_DelSecnode(node);
      }
      else
         node = node->m_tnext;
   }

  /* cph -
   * This is the strife we get into for using global variables. 
   *  tmthing is being used by several different functions calling
   *  P_BlockThingIterator, including functions that can be called 
   *  *from* P_BlockThingIterator. Using a global tmthing is not 
   *  reentrant. OTOH for Boom/MBF demos we have to preserve the buggy 
   *  behaviour. Fun. We restore its previous value unless we're in a 
   *  Boom/MBF demo. -- haleyjd: add SMMU too :)
   */

   if(demo_version < 200 || demo_version >= 329)
      tmthing = saved_tmthing;

   // haleyjd 11/11/02: cph forgot these in the original fix
   if(demo_version < 200 || demo_version >= 331)
   {
      tmx = saved_tmx;
      tmy = saved_tmy;
      tmflags = saved_tmflags;
      memcpy(tmbbox, saved_bbox, 4*sizeof(fixed_t)); // haleyjd
   }
}

//----------------------------------------------------------------------------
//
// $Log: p_map.c,v $
// Revision 1.35  1998/05/12  12:47:16  phares
// Removed OVER_UNDER code
//
// Revision 1.34  1998/05/07  00:52:38  killough
// beautification
//
// Revision 1.33  1998/05/05  15:35:10  phares
// Documentation and Reformatting changes
//
// Revision 1.32  1998/05/04  12:29:27  phares
// Eliminate player bobbing when stuck against wall
//
// Revision 1.31  1998/05/03  23:22:19  killough
// Fix #includes and remove unnecessary decls at the top, make some vars static
//
// Revision 1.30  1998/04/20  11:12:59  killough
// Make topslope, bottomslope local
//
// Revision 1.29  1998/04/12  01:56:51  killough
// Prevent no-clipping objects from blocking things
//
// Revision 1.28  1998/04/07  11:39:21  jim
// Skip MF_NOBLOCK things in P_CheckSector to get puffs back
//
// Revision 1.27  1998/04/07  06:52:36  killough
// Simplify sector_thinglist traversal to use simpler markers
//
// Revision 1.26  1998/04/06  11:05:11  jim
// Remove LEESFIXES, AMAP bdg->247
//
// Revision 1.25  1998/04/06  04:46:13  killough
// Fix CheckSector problems
//
// Revision 1.24  1998/04/05  10:08:51  jim
// changed crusher check back to old code
//
// Revision 1.23  1998/04/03  14:44:14  jim
// Fixed P_CheckSector problem
//
// Revision 1.21  1998/04/01  14:46:48  jim
// Prevent P_CheckSector from passing NULL things
//
// Revision 1.20  1998/03/29  20:14:35  jim
// Fixed use of deleted link in P_CheckSector
//
// Revision 1.19  1998/03/28  18:00:14  killough
// Fix telefrag/spawnfrag bug, and use sidedefs rather than 2s flag
//
// Revision 1.18  1998/03/23  15:24:25  phares
// Changed pushers to linedef control
//
// Revision 1.17  1998/03/23  06:43:14  jim
// linedefs reference initial version
//
// Revision 1.16  1998/03/20  02:10:43  jim
// Improved crusher code with new mobj data structures
//
// Revision 1.15  1998/03/20  00:29:57  phares
// Changed friction to linedef control
//
// Revision 1.14  1998/03/16  12:25:17  killough
// Allow conveyors to push things off ledges
//
// Revision 1.13  1998/03/12  14:28:42  phares
// friction and IDCLIP changes
//
// Revision 1.12  1998/03/11  17:48:24  phares
// New cheats, clean help code, friction fix
//
// Revision 1.11  1998/03/09  22:27:23  phares
// Fixed friction problem when teleporting
//
// Revision 1.10  1998/03/09  18:27:00  phares
// Fixed bug in neighboring variable friction sectors
//
// Revision 1.9  1998/03/02  12:05:56  killough
// Add demo_compatibility switch around moveangle+=10
//
// Revision 1.8  1998/02/24  08:46:17  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.7  1998/02/17  06:01:51  killough
// Use new RNG calling sequence
//
// Revision 1.6  1998/02/05  12:15:03  phares
// cleaned up comments
//
// Revision 1.5  1998/01/28  23:42:02  phares
// Bug fix to PE->LS code; better line checking
//
// Revision 1.4  1998/01/28  17:36:06  phares
// Expanded comments on Pit_CrossLine
//
// Revision 1.3  1998/01/28  12:22:21  phares
// AV bug fix and Lost Soul trajectory bug fix
//
// Revision 1.2  1998/01/26  19:24:09  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:59  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
