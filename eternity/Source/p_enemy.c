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

static const char
rcsid[] = "$Id: p_enemy.c,v 1.22 1998/05/12 12:47:10 phares Exp $";

#include "doomstat.h"
#include "m_random.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_inter.h"
#include "g_game.h"
#include "p_enemy.h"
#include "p_tick.h"
#include "m_bbox.h"
#include "p_anim.h" // haleyjd
#include "c_io.h"
#include "c_runcmd.h"
#include "w_wad.h"
#include "p_partcl.h"
#include "p_info.h"
#include "a_small.h"
#include "e_edf.h"

extern fixed_t FloatBobOffsets[64]; // haleyjd: Float Bobbing

static mobj_t *current_actor;

typedef enum {
   DI_EAST,
   DI_NORTHEAST,
   DI_NORTH,
   DI_NORTHWEST,
   DI_WEST,
   DI_SOUTHWEST,
   DI_SOUTH,
   DI_SOUTHEAST,
   DI_NODIR,
   NUMDIRS
} dirtype_t;

void A_Fall(mobj_t *actor);
void A_FaceTarget(mobj_t *actor);
static void P_NewChaseDir(mobj_t *actor);

//
// ENEMY THINKING
// Enemies are allways spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//

//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//
// killough 5/5/98: reformatted, cleaned up

static void P_RecursiveSound(sector_t *sec, int soundblocks,
                             mobj_t *soundtarget)
{
   int i;
   
   // wake up all monsters in this sector
   if(sec->validcount == validcount &&
      sec->soundtraversed <= soundblocks+1)
      return;             // already flooded

   sec->validcount = validcount;
   sec->soundtraversed = soundblocks+1;
   P_SetTarget(&sec->soundtarget, soundtarget);    // killough 11/98

   for(i=0; i<sec->linecount; i++)
   {
      sector_t *other;
      line_t *check = sec->lines[i];
      
      if(!(check->flags & ML_TWOSIDED))
         continue;

      P_LineOpening(check, NULL);
      
      if(openrange <= 0)
         continue;       // closed door

      other=sides[check->sidenum[sides[check->sidenum[0]].sector==sec]].sector;
      
      if(!(check->flags & ML_SOUNDBLOCK))
         P_RecursiveSound(other, soundblocks, soundtarget);
      else if(!soundblocks)
         P_RecursiveSound(other, 1, soundtarget);
   }
}

//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//

void P_NoiseAlert(mobj_t *target, mobj_t *emitter)
{
   validcount++;
   P_RecursiveSound(emitter->subsector->sector, 0, target);
}

//
// P_CheckMeleeRange
//

boolean P_CheckMeleeRange(mobj_t *actor)
{
   mobj_t *pl = actor->target;
   
   // haleyjd 02/15/02: revision of joel's fix for z height check
   if(pl && demo_version >= 329 && !comp[comp_overunder])
   {
      if(pl->z > (actor->z + actor->height) || // pl is too far above
         actor->z > (pl->z + pl->height))      // pl is too far below
         return false;
   }

   return  // killough 7/18/98: friendly monsters don't attack other friends
      pl && !(actor->flags & pl->flags & MF_FRIEND) &&
      (P_AproxDistance(pl->x-actor->x, pl->y-actor->y) <
       MELEERANGE - 20*FRACUNIT + pl->info->radius) &&
      P_CheckSight(actor, actor->target);
}

//
// P_HitFriend()
//
// killough 12/98
// This function tries to prevent shooting at friends
//
// haleyjd 09/22/02: reformatted
//
static boolean P_HitFriend(mobj_t *actor)
{
   return actor->target &&
      (P_AimLineAttack(actor,
                       R_PointToAngle2(actor->x, actor->y,
                          actor->target->x, actor->target->y),
                       P_AproxDistance(actor->x-actor->target->x, 
                          actor->y-actor->target->y),
                       0),
       linetarget) &&
      linetarget != actor->target &&
      !((linetarget->flags ^ actor->flags) & MF_FRIEND);
}

//
// P_CheckMissileRange
//
static boolean P_CheckMissileRange(mobj_t *actor)
{
   fixed_t dist;
   
   if(!P_CheckSight(actor, actor->target))
      return false;
   
   if (actor->flags & MF_JUSTHIT)
   {      // the target just hit the enemy, so fight back!
      actor->flags &= ~MF_JUSTHIT;

      // killough 7/18/98: no friendly fire at corpses
      // killough 11/98: prevent too much infighting among friends

      return 
         !(actor->flags & MF_FRIEND) || 
         (actor->target->health > 0 &&
          (!(actor->target->flags & MF_FRIEND) ||
          (actor->target->player ? 
           monster_infighting || P_Random(pr_defect) >128 :
           !(actor->target->flags & MF_JUSTHIT) && P_Random(pr_defect) >128)));
   }

   // killough 7/18/98: friendly monsters don't attack other friendly
   // monsters or players (except when attacked, and then only once)
   if(actor->flags & actor->target->flags & MF_FRIEND)
      return false;
   
   if(actor->reactiontime)
      return false;       // do not attack yet

   // OPTIMIZE: get this from a global checksight
   dist = P_AproxDistance(actor->x-actor->target->x,
                          actor->y-actor->target->y) - 64*FRACUNIT;

   if(!actor->info->meleestate)
      dist -= 128*FRACUNIT;       // no melee attack, so fire more

   dist >>= FRACBITS;

   // haleyjd 01/09/02: various changes made below to move
   // thing-type-specific differences in AI into flags

   if(actor->flags2 & MF2_SHORTMRANGE)
   {
      if(dist > 14*64)
         return false;     // too far away
   }

   if(actor->flags2 & MF2_LONGMELEE)
   {
      if (dist < 196)
         return false;   // close for fist attack
      // dist >>= 1;  -- this is now handled below
   }

   if(actor->flags2 & MF2_RANGEHALF)
      dist >>= 1;

   if(dist > 200)
      dist = 200;

   if((actor->flags2 & MF2_HIGHERMPROB) && dist > 160)
      dist = 160;

   if(P_Random(pr_missrange) < dist)
      return false;
  
   if((actor->flags & MF_FRIEND) && P_HitFriend(actor))
      return false;

   return true;
}

//
// P_IsOnLift
//
// killough 9/9/98:
//
// Returns true if the object is on a lift. Used for AI,
// since it may indicate the need for crowded conditions,
// or that a monster should stay on the lift for a while
// while it goes up or down.
//

static boolean P_IsOnLift(const mobj_t *actor)
{
   const sector_t *sec = actor->subsector->sector;
   line_t line;
   int l;

   // Short-circuit: it's on a lift which is active.
   if(sec->floordata &&
      ((thinker_t *) sec->floordata)->function==T_PlatRaise)
      return true;

   // Check to see if it's in a sector which can be 
   // activated as a lift.
   if((line.tag = sec->tag))
   {
      for(l = -1; (l = P_FindLineFromLineTag(&line, l)) >= 0;)
      {
         switch (lines[l].special)
         {
         case  10: case  14: case  15: case  20: case  21: case  22:
         case  47: case  53: case  62: case  66: case  67: case  68:
         case  87: case  88: case  95: case 120: case 121: case 122:
         case 123: case 143: case 162: case 163: case 181: case 182:
         case 144: case 148: case 149: case 211: case 227: case 228:
         case 231: case 232: case 235: case 236:
            return true;
         }
      }
   }
   
   return false;
}

//
// P_IsUnderDamage
//
// killough 9/9/98:
//
// Returns nonzero if the object is under damage based on
// their current position. Returns 1 if the damage is moderate,
// -1 if it is serious. Used for AI.
//

static int P_IsUnderDamage(mobj_t *actor)
{ 
   const struct msecnode_s *seclist;
   const ceiling_t *cl;             // Crushing ceiling
   int dir = 0;

   for(seclist=actor->touching_sectorlist; seclist; seclist=seclist->m_tnext)
   {
      if((cl = seclist->m_sector->ceilingdata) && 
         cl->thinker.function == T_MoveCeiling)
         dir |= cl->direction;
   }

   return dir;
}

//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//

static fixed_t xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
static fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};

// 1/11/98 killough: Limit removed on special lines crossed
extern  line_t **spechit;          // New code -- killough
extern  int    numspechit;

static boolean P_Move(mobj_t *actor, boolean dropoff) // killough 9/12/98
{
   fixed_t tryx, tryy, deltax, deltay;
   boolean try_ok;
   int movefactor = ORIG_FRICTION_FACTOR;    // killough 10/98
   int friction = ORIG_FRICTION;
   int speed;

   if(actor->movedir == DI_NODIR)
      return false;

#ifdef RANGECHECK
   if((unsigned)actor->movedir >= 8)
      I_Error ("Weird actor->movedir!");
#endif
   
   // killough 10/98: make monsters get affected by ice and sludge too:

   if(monster_friction)
      movefactor = P_GetMoveFactor(actor, &friction);

   speed = actor->info->speed;

   if(friction < ORIG_FRICTION &&     // sludge
      !(speed = ((ORIG_FRICTION_FACTOR - (ORIG_FRICTION_FACTOR-movefactor)/2)
        * speed) / ORIG_FRICTION_FACTOR))
   {
      speed = 1;      // always give the monster a little bit of speed
   }

   tryx = actor->x + (deltax = speed * xspeed[actor->movedir]);
   tryy = actor->y + (deltay = speed * yspeed[actor->movedir]);

   // killough 12/98: rearrange, fix potential for stickiness on ice

   if(friction <= ORIG_FRICTION)
      try_ok = P_TryMove(actor, tryx, tryy, dropoff);
   else
   {
      fixed_t x = actor->x;
      fixed_t y = actor->y;
      fixed_t floorz = actor->floorz;
      fixed_t ceilingz = actor->ceilingz;
      fixed_t dropoffz = actor->dropoffz;
      int     floorsec = actor->floorsec; // haleyjd
      
      try_ok = P_TryMove(actor, tryx, tryy, dropoff);
      
      // killough 10/98:
      // Let normal momentum carry them, instead of steptoeing them across ice.

      if(try_ok)
      {
         P_UnsetThingPosition(actor);
         actor->x = x;
         actor->y = y;
         actor->floorz = floorz;
         actor->ceilingz = ceilingz;
         actor->dropoffz = dropoffz;
         actor->floorsec = floorsec; // haleyjd
         P_SetThingPosition(actor);
         movefactor *= FRACUNIT / ORIG_FRICTION_FACTOR / 4;
         actor->momx += FixedMul(deltax, movefactor);
         actor->momy += FixedMul(deltay, movefactor);
      }
   }
   
   if(!try_ok)
   {      // open any specials
      int good;
      
      if(actor->flags & MF_FLOAT && floatok)
      {
         if(actor->z < tmfloorz)          // must adjust height
            actor->z += FLOATSPEED;
         else
            actor->z -= FLOATSPEED;
         
         actor->flags |= MF_INFLOAT;
         
         return true;
      }

      if(!numspechit)
         return false;

      actor->movedir = DI_NODIR;
      
      // if the special is not a door that can be opened, return false
      //
      // killough 8/9/98: this is what caused monsters to get stuck in
      // doortracks, because it thought that the monster freed itself
      // by opening a door, even if it was moving towards the doortrack,
      // and not the door itself.
      //
      // killough 9/9/98: If a line blocking the monster is activated,
      // return true 90% of the time. If a line blocking the monster is
      // not activated, but some other line is, return false 90% of the
      // time. A bit of randomness is needed to ensure it's free from
      // lockups, but for most cases, it returns the correct result.
      //
      // Do NOT simply return false 1/4th of the time (causes monsters to
      // back out when they shouldn't, and creates secondary stickiness).

      for (good = false; numspechit--; )
      {
         if(P_UseSpecialLine(actor, spechit[numspechit], 0))
            good |= (spechit[numspechit] == blockline ? 1 : 2);
      }

      return good && (demo_version < 203 || comp[comp_doorstuck] ||
                      (P_Random(pr_opendoor) >= 230) ^ (good & 1));
   }
   else
   {
      actor->flags &= ~MF_INFLOAT;
   }

   // killough 11/98: fall more slowly, under gravity, if felldown==true
   if(!(actor->flags & MF_FLOAT) && (!felldown || demo_version < 203))
   {
      if(demo_version >= 329 && !comp[comp_terrain] &&
         actor->z > actor->floorz)
      {
         P_HitFloor(actor);
      }
      actor->z = actor->floorz;
   }
   return true;
}

//
// P_SmartMove
//
// killough 9/12/98: Same as P_Move, except smarter
//

static boolean P_SmartMove(mobj_t *actor)
{
   mobj_t *target = actor->target;
   int on_lift, dropoff = false, under_damage;

   // killough 9/12/98: Stay on a lift if target is on one
   on_lift = !comp[comp_staylift] && target && target->health > 0 &&
      target->subsector->sector->tag==actor->subsector->sector->tag &&
      P_IsOnLift(actor);

   under_damage = monster_avoid_hazards && P_IsUnderDamage(actor);
   
   // killough 10/98: allow dogs to drop off of taller ledges sometimes.
   // dropoff==1 means always allow it, dropoff==2 means only up to 128 high,
   // and only if the target is immediately on the other side of the line.

   // haleyjd: allow things of HelperType to jump down,
   // as well as any thing that has the MF2_JUMPDOWN flag
   // (includes DOGS)

   if((actor->flags2 & MF2_JUMPDOWN || 
       (actor->type == HelperThing - 1)) &&
      target && dog_jumping &&
      !((target->flags ^ actor->flags) & MF_FRIEND) &&
      P_AproxDistance(actor->x - target->x,
                      actor->y - target->y) < FRACUNIT*144 &&
      P_Random(pr_dropoff) < 235)
   {
      dropoff = 2;
   }

   if(!P_Move(actor, dropoff))
      return false;

   // killough 9/9/98: avoid crushing ceilings or other damaging areas
   if(
      (on_lift && P_Random(pr_stayonlift) < 230 &&      // Stay on lift
       !P_IsOnLift(actor))
      ||
      (monster_avoid_hazards && !under_damage &&  // Get away from damage
       (under_damage = P_IsUnderDamage(actor)) &&
       (under_damage < 0 || P_Random(pr_avoidcrush) < 200))
     )
   {
      actor->movedir = DI_NODIR;    // avoid the area (most of the time anyway)
   }
   
   return true;
}

//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//

static boolean P_TryWalk(mobj_t *actor)
{
   if(!P_SmartMove(actor))
      return false;
   actor->movecount = P_Random(pr_trywalk)&15;
   return true;
}

//
// P_DoNewChaseDir
//
// killough 9/8/98:
//
// Most of P_NewChaseDir(), except for what
// determines the new direction to take
//

static void P_DoNewChaseDir(mobj_t *actor, fixed_t deltax, fixed_t deltay)
{
   dirtype_t xdir, ydir, tdir;
   dirtype_t olddir = actor->movedir;
   dirtype_t turnaround = olddir;
   
   if(turnaround != DI_NODIR)         // find reverse direction
   {
      turnaround ^= 4;
   }

   {
      xdir = 
         (deltax >  10*FRACUNIT ? DI_EAST :
          deltax < -10*FRACUNIT ? DI_WEST : DI_NODIR);
   
      ydir =
         (deltay < -10*FRACUNIT ? DI_SOUTH :
          deltay >  10*FRACUNIT ? DI_NORTH : DI_NODIR);
   }

   // try direct route
   if(xdir != DI_NODIR && ydir != DI_NODIR && turnaround != 
      (actor->movedir = deltay < 0 ? deltax > 0 ? DI_SOUTHEAST : DI_SOUTHWEST :
       deltax > 0 ? DI_NORTHEAST : DI_NORTHWEST) && P_TryWalk(actor))
   {
      return;
   }

   // try other directions
   if(P_Random(pr_newchase) > 200 || D_abs(deltay)>abs(deltax))
      tdir = xdir, xdir = ydir, ydir = tdir;

   if((xdir == turnaround ? xdir = DI_NODIR : xdir) != DI_NODIR &&
      (actor->movedir = xdir, P_TryWalk(actor)))
      return;         // either moved forward or attacked

   if((ydir == turnaround ? ydir = DI_NODIR : ydir) != DI_NODIR &&
      (actor->movedir = ydir, P_TryWalk(actor)))
      return;

   // there is no direct path to the player, so pick another direction.
   if(olddir != DI_NODIR && (actor->movedir = olddir, P_TryWalk(actor)))
      return;

   // randomly determine direction of search
   if(P_Random(pr_newchasedir) & 1)
   {
      for(tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
      {
         if(tdir != turnaround &&
            (actor->movedir = tdir, P_TryWalk(actor)))
            return;
      }
   }
   else
   {
      for (tdir = DI_SOUTHEAST; tdir != DI_EAST-1; tdir--)
      {
         if(tdir != turnaround &&
            (actor->movedir = tdir, P_TryWalk(actor)))
            return;
      }
   }
  
   if((actor->movedir = turnaround) != DI_NODIR && !P_TryWalk(actor))
      actor->movedir = DI_NODIR;
}

//
// killough 11/98:
//
// Monsters try to move away from tall dropoffs.
//
// In Doom, they were never allowed to hang over dropoffs,
// and would remain stuck if involuntarily forced over one.
// This logic, combined with p_map.c (P_TryMove), allows
// monsters to free themselves without making them tend to
// hang over dropoffs.

static fixed_t dropoff_deltax, dropoff_deltay, floorz;

static boolean PIT_AvoidDropoff(line_t *line)
{
   if(line->backsector                          && // Ignore one-sided linedefs
      tmbbox[BOXRIGHT]  > line->bbox[BOXLEFT]   &&
      tmbbox[BOXLEFT]   < line->bbox[BOXRIGHT]  &&
      tmbbox[BOXTOP]    > line->bbox[BOXBOTTOM] && // Linedef must be contacted
      tmbbox[BOXBOTTOM] < line->bbox[BOXTOP]    &&
      P_BoxOnLineSide(tmbbox, line) == -1)
   {
      fixed_t front = line->frontsector->floorheight;
      fixed_t back  = line->backsector->floorheight;
      angle_t angle;

      // The monster must contact one of the two floors,
      // and the other must be a tall dropoff (more than 24).

      if(back == floorz && front < floorz - FRACUNIT*24)
      {
         // front side dropoff
         angle = R_PointToAngle2(0,0,line->dx,line->dy);
      }
      else
      {
         // back side dropoff
         if(front == floorz && back < floorz - FRACUNIT*24)
            angle = R_PointToAngle2(line->dx,line->dy,0,0);
         else
            return true;
      }

      // Move away from dropoff at a standard speed.
      // Multiple contacted linedefs are cumulative (e.g. hanging over corner)
      dropoff_deltax -= finesine[angle >> ANGLETOFINESHIFT]*32;
      dropoff_deltay += finecosine[angle >> ANGLETOFINESHIFT]*32;
   }

   return true;
}

//
// Driver for above
//

static fixed_t P_AvoidDropoff(mobj_t *actor)
{
   int yh=((tmbbox[BOXTOP]   = actor->y+actor->radius)-bmaporgy)>>MAPBLOCKSHIFT;
   int yl=((tmbbox[BOXBOTTOM]= actor->y-actor->radius)-bmaporgy)>>MAPBLOCKSHIFT;
   int xh=((tmbbox[BOXRIGHT] = actor->x+actor->radius)-bmaporgx)>>MAPBLOCKSHIFT;
   int xl=((tmbbox[BOXLEFT]  = actor->x-actor->radius)-bmaporgx)>>MAPBLOCKSHIFT;
   int bx, by;

   floorz = actor->z;            // remember floor height

   dropoff_deltax = dropoff_deltay = 0;

   // check lines

   validcount++;
   for(bx=xl ; bx<=xh ; bx++)
   {
      // all contacted lines
      for(by=yl ; by<=yh ; by++)
         P_BlockLinesIterator(bx, by, PIT_AvoidDropoff);
   }
   
   // Non-zero if movement prescribed
   return dropoff_deltax | dropoff_deltay;
}

//
// P_NewChaseDir
//
// killough 9/8/98: Split into two functions
//

static void P_NewChaseDir(mobj_t *actor)
{
   mobj_t *target = actor->target;
   fixed_t deltax = target->x - actor->x;
   fixed_t deltay = target->y - actor->y;

   // killough 8/8/98: sometimes move away from target, keeping distance
   //
   // 1) Stay a certain distance away from a friend, to avoid being in their way
   // 2) Take advantage over an enemy without missiles, by keeping distance

   actor->strafecount = 0;

   if(demo_version >= 203)
   {
      if(actor->floorz - actor->dropoffz > FRACUNIT*24 &&
         actor->z <= actor->floorz &&
         !(actor->flags & (MF_DROPOFF|MF_FLOAT)) &&
         !comp[comp_dropoff] && P_AvoidDropoff(actor)) // Move away from dropoff
      {
         P_DoNewChaseDir(actor, dropoff_deltax, dropoff_deltay);
         
         // If moving away from dropoff, set movecount to 1 so that 
         // small steps are taken to get monster away from dropoff.
         
         actor->movecount = 1;
         return;
      }
      else
      {
         fixed_t dist = P_AproxDistance(deltax, deltay);
         
         // Move away from friends when too close, except
         // in certain situations (e.g. a crowded lift)
         
         if(actor->flags & target->flags & MF_FRIEND &&
            distfriend << FRACBITS > dist && 
            !P_IsOnLift(target) && !P_IsUnderDamage(actor))
         {
            deltax = -deltax, deltay = -deltay;
         }
         else
         {
            if(target->health > 0 &&
               (actor->flags ^ target->flags) & MF_FRIEND)
            {   // Live enemy target
               if(monster_backing &&
                  actor->info->missilestate && 
                  !(actor->flags2 & MF2_NOSTRAFE) &&
                  ((!target->info->missilestate && dist < MELEERANGE*2) ||
                   (target->player && dist < MELEERANGE*3 &&
                    (target->player->readyweapon == wp_fist ||
                     target->player->readyweapon == wp_chainsaw))))
               {       // Back away from melee attacker
                  actor->strafecount = P_Random(pr_enemystrafe) & 15;
                  deltax = -deltax, deltay = -deltay;
               }
            }
         }
      }
   }

   P_DoNewChaseDir(actor, deltax, deltay);

   // If strafing, set movecount to strafecount so that old Doom
   // logic still works the same, except in the strafing part
   
   if(actor->strafecount)
      actor->movecount = actor->strafecount;
}

//
// P_IsVisible
//
// killough 9/9/98: whether a target is visible to a monster
//

static boolean P_IsVisible(mobj_t *actor, mobj_t *mo, boolean allaround)
{
   if(mo->flags2 & MF2_DONTDRAW)
      return false;  // haleyjd: total invisibility!

   // haleyjd 11/14/02: Heretic ghost effects
   if(mo->flags3 & MF3_GHOST)
   {
      if(P_AproxDistance(mo->x - actor->x, mo->y - actor->y) > 2*MELEERANGE 
         && P_AproxDistance(mo->momx, mo->momy) < 5*FRACUNIT)
      {
         // when distant and moving slow, target is considered
         // to be "sneaking"
         return false;
      }
      if(P_Random(pr_ghostsneak) < 225)
         return false;
   }

   if(!allaround)
   {
      angle_t an = R_PointToAngle2(actor->x, actor->y, 
                                   mo->x, mo->y) - actor->angle;
      if(an > ANG90 && an < ANG270 &&
         P_AproxDistance(mo->x-actor->x, mo->y-actor->y) > MELEERANGE)
         return false;
   }

   return P_CheckSight(actor, mo);
}

//
// PIT_FindTarget
//
// killough 9/5/98
//
// Finds monster targets for other monsters
//

static int current_allaround;

static boolean PIT_FindTarget(mobj_t *mo)
{
   mobj_t *actor = current_actor;

   if(!((mo->flags ^ actor->flags) & MF_FRIEND && // Invalid target
      mo->health > 0 &&
      (mo->flags & MF_COUNTKILL || mo->flags3 & MF3_KILLABLE)))
      return true;

   // If the monster is already engaged in a one-on-one attack
   // with a healthy friend, don't attack around 60% the time
   {
      const mobj_t *targ = mo->target;
      if(targ && targ->target == mo &&
         P_Random(pr_skiptarget) > 100 &&
         (targ->flags ^ mo->flags) & MF_FRIEND &&
         targ->health*2 >= targ->info->spawnhealth)
         return true;
   }

   if(!P_IsVisible(actor, mo, current_allaround))
      return true;

   P_SetTarget(&actor->lastenemy, actor->target);  // Remember previous target
   P_SetTarget(&actor->target, mo);                // Found target

   // Move the selected monster to the end of its associated
   // list, so that it gets searched last next time.

   {
      thinker_t *cap = &thinkerclasscap[mo->flags & MF_FRIEND ?
                                        th_friends : th_enemies];
      (mo->thinker.cprev->cnext = mo->thinker.cnext)->cprev =
         mo->thinker.cprev;
      (mo->thinker.cprev = cap->cprev)->cnext = &mo->thinker;
      (mo->thinker.cnext = cap)->cprev = &mo->thinker;
   }
   
   return false;
}

//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//

static boolean P_LookForPlayers(mobj_t *actor, boolean allaround)
{
   player_t *player;
   int stop, stopc, c;
   
   if(actor->flags & MF_FRIEND)
   {  // killough 9/9/98: friendly monsters go about players differently
      int anyone;

#if 0
      if(!allaround) // If you want friendly monsters not to awaken unprovoked
         return false;
#endif

      // Go back to a player, no matter whether it's visible or not
      for(anyone=0; anyone<=1; anyone++)
      {
         for(c=0; c<MAXPLAYERS; c++)
         {
            if(playeringame[c] && players[c].playerstate==PST_LIVE &&
               (anyone || P_IsVisible(actor, players[c].mo, allaround)))
            {
               P_SetTarget(&actor->target, players[c].mo);

               // killough 12/98:
               // get out of refiring loop, to avoid hitting player accidentally

               if(actor->info->missilestate)
               {
                  P_SetMobjState(actor, actor->info->seestate);
                  actor->flags &= ~MF_JUSTHIT;
               }
               
               return true;
            }
         }
      }
      
      return false;
   }

   // Change mask of 3 to (MAXPLAYERS-1) -- killough 2/15/98:
   stop = (actor->lastlook-1)&(MAXPLAYERS-1);

   c = 0;

   stopc = demo_version < 203 && !demo_compatibility && monsters_remember ?
           MAXPLAYERS : 2;       // killough 9/9/98

   for(;; actor->lastlook = (actor->lastlook+1)&(MAXPLAYERS-1))
   {
      if(!playeringame[actor->lastlook])
         continue;

      // killough 2/15/98, 9/9/98:
      if(c++ == stopc || actor->lastlook == stop)  // done looking
         return false;
      
      player = &players[actor->lastlook];
      
      if(player->health <= 0)
         continue;               // dead
      
      if(!P_IsVisible(actor, player->mo, allaround))
         continue;
      
      P_SetTarget(&actor->target, player->mo);

      // killough 9/9/98: give monsters a threshold towards getting players
      // (we don't want it to be too easy for a player with dogs :)
      if(demo_version >= 203 && !comp[comp_pursuit])
         actor->threshold = 60;
      
      return true;
   }
}

// 
// Friendly monsters, by Lee Killough 7/18/98
//
// Friendly monsters go after other monsters first, but 
// also return to owner if they cannot find any targets.
// A marine's best friend :)  killough 7/18/98, 9/98
//

static boolean P_LookForMonsters(mobj_t *actor, boolean allaround)
{
   thinker_t *cap, *th;
   
   if(demo_compatibility)
      return false;

   if(actor->lastenemy && actor->lastenemy->health > 0 && monsters_remember &&
      !(actor->lastenemy->flags & actor->flags & MF_FRIEND)) // not friends
   {
      P_SetTarget(&actor->target, actor->lastenemy);
      P_SetTarget(&actor->lastenemy, NULL);
      return true;
   }

   if(demo_version < 203)  // Old demos do not support monster-seeking bots
      return false;

   // Search the threaded list corresponding to this object's potential targets
   cap = &thinkerclasscap[actor->flags & MF_FRIEND ? th_enemies : th_friends];

   // Search for new enemy

   if(cap->cnext != cap)        // Empty list? bail out early
   {
      int x = (actor->x - bmaporgx)>>MAPBLOCKSHIFT;
      int y = (actor->y - bmaporgy)>>MAPBLOCKSHIFT;
      int d;

      current_actor = actor;
      current_allaround = allaround;
      
      // Search first in the immediate vicinity.
      
      if(!P_BlockThingsIterator(x, y, PIT_FindTarget))
         return true;

      for (d=1; d<5; d++)
      {
         int i = 1 - d;
         do
         {
            if(!P_BlockThingsIterator(x+i, y-d, PIT_FindTarget) ||
               !P_BlockThingsIterator(x+i, y+d, PIT_FindTarget))
               return true;
         }
         while (++i < d);
         do
         {
            if(!P_BlockThingsIterator(x-d, y+i, PIT_FindTarget) ||
               !P_BlockThingsIterator(x+d, y+i, PIT_FindTarget))
               return true;
         }
         while (--i + d >= 0);
      }

      {   // Random number of monsters, to prevent patterns from forming
         int n = (P_Random(pr_friends) & 31) + 15;

         for (th = cap->cnext; th != cap; th = th->cnext)
         {
            if (--n < 0)
            { 
               // Only a subset of the monsters were searched. Move all of
               // the ones which were searched so far, to the end of the list.
               
               (cap->cnext->cprev = cap->cprev)->cnext = cap->cnext;
               (cap->cprev = th->cprev)->cnext = cap;
               (th->cprev = cap)->cnext = th;
               break;
            }
            else if (!PIT_FindTarget((mobj_t *) th))
               // If target sighted
               return true;
         }
      }
   }
   
   return false;  // No monster found
}

//
// P_LookForTargets
//
// killough 9/5/98: look for targets to go after, depending on kind of monster
//

static boolean P_LookForTargets(mobj_t *actor, int allaround)
{
   return actor->flags & MF_FRIEND ?
      P_LookForMonsters(actor, allaround) || P_LookForPlayers (actor, allaround):
      P_LookForPlayers (actor, allaround) || P_LookForMonsters(actor, allaround);
}

//
// P_HelpFriend
//
// killough 9/8/98: Help friends in danger of dying
//

static boolean P_HelpFriend(mobj_t *actor)
{
   thinker_t *cap, *th;

   // If less than 33% health, self-preservation rules
   if(actor->health*3 < actor->info->spawnhealth)
      return false;

   current_actor = actor;
   current_allaround = true;

   // Possibly help a friend under 50% health
   cap = &thinkerclasscap[actor->flags & MF_FRIEND ? th_friends : th_enemies];

   for (th = cap->cnext; th != cap; th = th->cnext)
   {
      if(((mobj_t *) th)->health*2 >=
         ((mobj_t *) th)->info->spawnhealth)
      {
         if(P_Random(pr_helpfriend) < 180)
            break;
      }
      else
         if(((mobj_t *) th)->flags & MF_JUSTHIT &&
            ((mobj_t *) th)->target && 
            ((mobj_t *) th)->target != actor->target &&
            !PIT_FindTarget(((mobj_t *) th)->target))
         {
            // Ignore any attacking monsters, while searching for 
            // friend
            actor->threshold = BASETHRESHOLD;
            return true;
         }
   }
   
   return false;
}

//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//

void A_Look(mobj_t *actor)
{
   mobj_t *targ;
   
   // haleyjd 1/25/00:  isolated assignment of targ to test earlier
   //  for "deaf" enemies seeing totally invisible players after the
   //  sector soundtarget is activated, which looks crazy and should
   //  NOT happen.  The if below is simply deplorable to try to read!
   
   targ = actor->subsector->sector->soundtarget;

   if(targ && 
      (targ->flags2 & MF2_DONTDRAW) && 
      (actor->flags & MF_AMBUSH))
      return;

   // killough 7/18/98:
   // Friendly monsters go after other monsters first, but 
   // also return to player, without attacking them, if they
   // cannot find any targets. A marine's best friend :)
   
   actor->threshold = actor->pursuecount = 0;
   if(!(actor->flags & MF_FRIEND && P_LookForTargets(actor, false)) &&
      !((targ) &&  // haleyjd: removed assignment here
        targ->flags & MF_SHOOTABLE &&
        (P_SetTarget(&actor->target, targ),
         !(actor->flags & MF_AMBUSH) || P_CheckSight(actor, targ))) &&
      (actor->flags & MF_FRIEND || !P_LookForTargets(actor, false)))
         return;

   // go into chase state
   
   if(actor->info->seesound)
   {
      int sound;
      switch (actor->info->seesound)
      {
      case sfx_posit1:
      case sfx_posit2:
      case sfx_posit3:
         sound = sfx_posit1+P_Random(pr_see)%3;
         break;
         
      case sfx_bgsit1:
      case sfx_bgsit2:
         sound = sfx_bgsit1+P_Random(pr_see)%2;
         break;
         
      case sfx_dfsit1:  // haleyjd: dwarf screams :)
      case sfx_dfsit2:
         sound = sfx_dfsit1+P_Random(pr_see)%2;
         break;
         
      case sfx_clsit1:  // haleyjd: new cleric quips - only if uses clsit1
         sound = sfx_clsit1+P_Random(pr_see)%4;
         break;
         
      default:
         sound = actor->info->seesound;
         break;
      }
      // haleyjd: generalize to all bosses
      if(actor->flags2&MF2_BOSS)
         S_StartSound(NULL, sound);          // full volume
      else
         S_StartSound(actor, sound);
   }
   
   P_SetMobjState(actor, actor->info->seestate);
}

//
// A_KeepChasing
//
// killough 10/98:
// Allows monsters to continue movement while attacking
//

void A_KeepChasing(mobj_t *actor)
{
   /*
   if(actor->movecount)
   {
      actor->movecount--;
      if(actor->strafecount)
         actor->strafecount--;
      P_SmartMove(actor);
   }
   */
   P_SmartMove(actor);
}

//
// P_SuperFriend
//
// haleyjd 01/11/04: returns true if this thing is a "super friend"
// and is going to attack a friend
//
static boolean P_SuperFriend(mobj_t *actor)
{
   mobj_t *target = actor->target;

   return ((actor->flags3 & MF3_SUPERFRIEND) && // thing is a super friend,
           target &&                            // target is valid, and
           (actor->flags & target->flags & MF_FRIEND)); // both are friends
}

//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//

void A_Chase(mobj_t *actor)
{
   boolean superfriend = false;

   if(actor->reactiontime)
      actor->reactiontime--;

   // modify target threshold
   if(actor->threshold)
   {
      if(!actor->target || actor->target->health <= 0)
         actor->threshold = 0;
      else
         actor->threshold--;
   }

   // turn towards movement direction if not there yet
   // killough 9/7/98: keep facing towards target if strafing or backing out

   if(actor->strafecount)
      A_FaceTarget(actor);
   else
   {
      if(actor->movedir < 8)
      {
         int delta = (actor->angle &= (7<<29)) - (actor->movedir << 29);

         if(delta > 0)
            actor->angle -= ANG90/2;
         else if (delta < 0)
            actor->angle += ANG90/2;
      }
   }

   if(!actor->target || !(actor->target->flags&MF_SHOOTABLE))
   {    
      if(!P_LookForTargets(actor,true)) // look for a new target
         P_SetMobjState(actor, actor->info->spawnstate); // no new target
      return;
   }

   // do not attack twice in a row
   if(actor->flags & MF_JUSTATTACKED)
   {
      actor->flags &= ~MF_JUSTATTACKED;
      if(gameskill != sk_nightmare && !fastparm)
         P_NewChaseDir(actor);
      return;
   }

   // haleyjd 01/11/04: check superfriend status
   if(demo_version >= 331)
      superfriend = P_SuperFriend(actor);

  // check for melee attack
   if(actor->info->meleestate && P_CheckMeleeRange(actor) && !superfriend)
   {
      S_StartSound(actor, actor->info->attacksound);
      
      P_SetMobjState(actor, actor->info->meleestate);
      
      if(!actor->info->missilestate)
         actor->flags |= MF_JUSTHIT; // killough 8/98: remember an attack
      return;
   }

   // check for missile attack
   if(actor->info->missilestate && !superfriend)
   {
      if(!actor->movecount || gameskill >= sk_nightmare || fastparm)
      {
         if(P_CheckMissileRange(actor))
         {
            P_SetMobjState(actor, actor->info->missilestate);
            actor->flags |= MF_JUSTATTACKED;
            return;
         }
      }
   }

   if (!actor->threshold)
   {
      if(demo_version < 203)
      {
         // killough 9/9/98: for backward demo compatibility
         if(netgame && !P_CheckSight(actor, actor->target) &&
            P_LookForPlayers(actor, true))
            return;  
      }
      else  // killough 7/18/98, 9/9/98: new monster AI
      {
         if(help_friends && P_HelpFriend(actor))
         {
            return;      // killough 9/8/98: Help friends in need
         }
         else  // Look for new targets if current one is bad or is out of view
         {
            if (actor->pursuecount)
               actor->pursuecount--;
            else
            {
               actor->pursuecount = BASETHRESHOLD;
               
               // If current target is bad and a new one is found, return:

               if(!(actor->target && actor->target->health > 0 &&
                   ((comp[comp_pursuit] && !netgame) || 
                    (((actor->target->flags ^ actor->flags) & MF_FRIEND ||
                      (!(actor->flags & MF_FRIEND) && monster_infighting)) &&
                    P_CheckSight(actor, actor->target)))) &&
                    P_LookForTargets(actor, true))
               {
                  return;
               }
              
               // (Current target was good, or no new target was found)
               //
               // If monster is a missile-less friend, give up pursuit 
               // and return to player, if no attacks have occurred 
               // recently.
               
               if(!actor->info->missilestate && 
                  actor->flags & MF_FRIEND)
               {
                  if(actor->flags & MF_JUSTHIT)        // if recent action,
                     actor->flags &= ~MF_JUSTHIT;      // keep fighting
                  else
                     if(P_LookForPlayers(actor, true)) // else return to player
                        return;
               }
            }
         }
      }
   }
   
   if(actor->strafecount)
      actor->strafecount--;
   
   // chase towards player
   if(--actor->movecount<0 || !P_SmartMove(actor))
      P_NewChaseDir(actor);
   
   // make active sound
   if(actor->info->activesound && P_Random(pr_see) < 3)
   {
      int sound = actor->info->activesound;
      mobj_t *sndsource = actor;

      // haleyjd: some heretic enemies use their seesound on
      // occasion, so I've made this a general feature
      if(demo_version >= 331 && actor->flags3 & MF3_ACTSEESOUND)
      {
         if(P_Random(pr_lookact) < 128 && actor->info->seesound)
            sound = actor->info->seesound;
      }

      // haleyjd: some heretic enemies snort at full volume :)
      if(actor->flags3 & MF3_LOUDACTIVE)
         sndsource = NULL;

      S_StartSound(sndsource, sound);
   }
}

//
// A_FaceTarget
//
void A_FaceTarget(mobj_t *actor)
{
   if(!actor->target)
      return;

   // haleyjd: special feature for player thunking
   if(actor->intflags & MIF_NOFACE)
      return;

   actor->flags &= ~MF_AMBUSH;
   actor->angle = R_PointToAngle2(actor->x, actor->y,
                                  actor->target->x, actor->target->y);
   if(actor->target->flags & MF_SHADOW ||
      actor->target->flags2 & MF2_DONTDRAW || // haleyjd
      actor->target->flags3 & MF3_GHOST)      // haleyjd
   { // killough 5/5/98: remove dependence on order of evaluation:
      int t = P_Random(pr_facetarget);
      actor->angle += (t-P_Random(pr_facetarget))<<21;
   }
}

//
// A_PosAttack
//

void A_PosAttack(mobj_t *actor)
{
   int angle, damage, slope, t;
   
   if(!actor->target)
      return;

   A_FaceTarget(actor);
   angle = actor->angle;
   slope = P_AimLineAttack(actor, angle, MISSILERANGE, 0); // killough 8/2/98
   S_StartSound(actor, sfx_pistol);
   
   // killough 5/5/98: remove dependence on order of evaluation:
   t = P_Random(pr_posattack);
   angle += (t - P_Random(pr_posattack))<<20;
   damage = (P_Random(pr_posattack)%5 + 1)*3;
   P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack(mobj_t* actor)
{
   int i, bangle, slope;
   
   if (!actor->target)
      return;
   S_StartSound(actor, sfx_shotgn);
   A_FaceTarget(actor);
   bangle = actor->angle;
   slope = P_AimLineAttack(actor, bangle, MISSILERANGE, 0); // killough 8/2/98
   for (i=0; i<3; i++)
   {  // killough 5/5/98: remove dependence on order of evaluation:
      int t = P_Random(pr_sposattack);
      int angle = bangle + ((t - P_Random(pr_sposattack))<<20);
      int damage = ((P_Random(pr_sposattack)%5)+1)*3;
      P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
   }
}

void A_CPosAttack(mobj_t *actor)
{
   int angle, bangle, damage, slope, t;
   
   if (!actor->target)
      return;
   S_StartSound(actor, sfx_chgun); //shotgn
   A_FaceTarget(actor);
   bangle = actor->angle;
   slope = P_AimLineAttack(actor, bangle, MISSILERANGE, 0); // killough 8/2/98
   
   // killough 5/5/98: remove dependence on order of evaluation:
   t = P_Random(pr_cposattack);
   angle = bangle + ((t - P_Random(pr_cposattack))<<20);
   damage = ((P_Random(pr_cposattack)%5)+1)*3;
   P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire(mobj_t *actor)
{
   // keep firing unless target got out of sight
   A_FaceTarget(actor);
   
   // killough 12/98: Stop firing if a friend has gotten in the way
   if(actor->flags & MF_FRIEND && P_HitFriend(actor))
      goto stop;
   
   // killough 11/98: prevent refiring on friends continuously
   if (P_Random(pr_cposrefire) < 40)
   {
      if(actor->target && 
         actor->flags & actor->target->flags & MF_FRIEND)
         goto stop;
      else
         return;
   }

   if(!actor->target || actor->target->health <= 0 ||
      !P_CheckSight(actor, actor->target))
stop: P_SetMobjState(actor, actor->info->seestate);
}

void A_SpidRefire(mobj_t* actor)
{
   // keep firing unless target got out of sight
   A_FaceTarget(actor);
   
   // killough 12/98: Stop firing if a friend has gotten in the way
   if(actor->flags & MF_FRIEND && P_HitFriend(actor))
      goto stop;
   
   if(P_Random(pr_spidrefire) < 10)
      return;

   // killough 11/98: prevent refiring on friends continuously
   if(!actor->target || actor->target->health <= 0    ||
      actor->flags & actor->target->flags & MF_FRIEND ||
      !P_CheckSight(actor, actor->target))
stop: P_SetMobjState(actor, actor->info->seestate);
}

void A_BspiAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   // launch a missile
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_ARACHPLAZ), 
                  actor->z + DEFAULTMISSILEZ);
}

//
// A_TroopAttack
//

void A_TroopAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   if(P_CheckMeleeRange(actor))
   {
      int damage;
      S_StartSound(actor, sfx_claw);
      damage = (P_Random(pr_troopattack)%8+1)*3;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
      return;
   }
   // launch a missile
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_TROOPSHOT),
                  actor->z + DEFAULTMISSILEZ);
}

void A_SargAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   if(P_CheckMeleeRange(actor))
   {
      int damage = ((P_Random(pr_sargattack)%10)+1)*4;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
   }
}

void A_HeadAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget (actor);
   if(P_CheckMeleeRange(actor))
   {
      int damage = (P_Random(pr_headattack)%6+1)*10;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
      return;
   }
   // launch a missile
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_HEADSHOT),
                  actor->z + DEFAULTMISSILEZ);
}

void A_CyberAttack(mobj_t *actor)
{
   mobj_t *mo;

   if(!actor->target)
      return;
   A_FaceTarget(actor);
   mo = P_SpawnMissile(actor, actor->target, 
                       E_SafeThingType(MT_ROCKET),
                       actor->z + DEFAULTMISSILEZ);   
}

void A_BruisAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   if(P_CheckMeleeRange(actor))
   {
      int damage;
      S_StartSound(actor, sfx_claw);
      damage = (P_Random(pr_bruisattack)%8+1)*10;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
      return;
   }
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_BRUISERSHOT),
                  actor->z + DEFAULTMISSILEZ);  // launch a missile
}

//
// A_SkelMissile
//

void A_SkelMissile(mobj_t *actor)
{
   mobj_t *mo;
   
   if(!actor->target)
      return;
   
   A_FaceTarget (actor);
   actor->z += 16*FRACUNIT;      // so missile spawns higher
   mo = P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_TRACER),
                       actor->z + DEFAULTMISSILEZ);
   actor->z -= 16*FRACUNIT;      // back to normal
   
   mo->x += mo->momx;
   mo->y += mo->momy;
   P_SetTarget(&mo->tracer, actor->target);  // killough 11/98
}

#define TRACEANGLE 0xc000000   /* killough 9/9/98: change to #define */

void A_Tracer(mobj_t *actor)
{
   angle_t       exact;
   fixed_t       dist;
   fixed_t       slope;
   mobj_t        *dest;
   mobj_t        *th;

   // killough 1/18/98: this is why some missiles do not have smoke
   // and some do. Also, internal demos start at random gametics, 
   // thus the bug in which revenants cause internal demos to go out 
   // of sync.
   //
   // killough 3/6/98: fix revenant internal demo bug by subtracting
   // levelstarttic from gametic.
   //
   // killough 9/29/98: use new "basetic" so that demos stay in sync
   // during pauses and menu activations, while retaining old demo 
   // sync.
   //
   // leveltime would have been better to use to start with in Doom, 
   // but since old demos were recorded using gametic, we must stick 
   // with it, and improvise around it (using leveltime causes desync 
   // across levels).

   if((gametic-basetic) & 3)
      return;

   // spawn a puff of smoke behind the rocket
   P_SpawnPuff(actor->x, actor->y, actor->z, 0, 3, false);
   
   th = P_SpawnMobj(actor->x-actor->momx,
                    actor->y-actor->momy,
                    actor->z, E_SafeThingType(MT_SMOKE));
  
   th->momz = FRACUNIT;
   th->tics -= P_Random(pr_tracer) & 3;
   if(th->tics < 1)
      th->tics = 1;
  
   // adjust direction
   dest = actor->tracer;
   
   if(!dest || dest->health <= 0)
      return;

   // change angle
   exact = R_PointToAngle2(actor->x, actor->y, dest->x, dest->y);

   if(exact != actor->angle)
   {
      if(exact - actor->angle > 0x80000000)
      {
         actor->angle -= TRACEANGLE;
         if(exact - actor->angle < 0x80000000)
            actor->angle = exact;
      }
      else
      {
         actor->angle += TRACEANGLE;
         if(exact - actor->angle > 0x80000000)
            actor->angle = exact;
      }
   }

   exact = actor->angle>>ANGLETOFINESHIFT;
   actor->momx = FixedMul(actor->info->speed, finecosine[exact]);
   actor->momy = FixedMul(actor->info->speed, finesine[exact]);

   // change slope
   dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
   
   dist = dist / actor->info->speed;

   if(dist < 1)
      dist = 1;

   slope = (dest->z+40*FRACUNIT - actor->z) / dist;
   
   if(slope < actor->momz)
      actor->momz -= FRACUNIT/8;
   else
      actor->momz += FRACUNIT/8;
}

void A_SkelWhoosh(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   S_StartSound(actor,sfx_skeswg);
}

void A_SkelFist(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   if(P_CheckMeleeRange(actor))
   {
      int damage = ((P_Random(pr_skelfist)%10)+1)*6;
      S_StartSound(actor, sfx_skepch);
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
   }
}

//
// PIT_VileCheck
// Detect a corpse that could be raised.
//

mobj_t* corpsehit;
mobj_t* vileobj;
fixed_t viletryx;
fixed_t viletryy;

boolean PIT_VileCheck(mobj_t *thing)
{
   int maxdist;
   boolean check;
   static int vileType = -1;
   
   if(vileType == -1)
      vileType = E_SafeThingType(MT_VILE);
   
   if(!(thing->flags & MF_CORPSE))
      return true;        // not a monster
   
   if(thing->tics != -1)
      return true;        // not lying still yet
   
   if(thing->info->raisestate == E_NullState())
      return true;        // monster doesn't have a raise state
   
   maxdist = thing->info->radius + mobjinfo[vileType].radius;
   
   if(D_abs(thing->x-viletryx) > maxdist ||
      D_abs(thing->y-viletryy) > maxdist)
      return true;                // not actually touching

// Check to see if the radius and height are zero. If they are      // phares
// then this is a crushed monster that has been turned into a       //   |
// gib. One of the options may be to ignore this guy.               //   V

// Option 1: the original, buggy method, -> ghost (compatibility)
// Option 2: ressurect the monster, but not as a ghost
// Option 3: ignore the gib

//    if (Option3)                                                  //   ^
//        if ((thing->height == 0) && (thing->radius == 0))         //   |
//            return true;                                          // phares

   corpsehit = thing;
   corpsehit->momx = corpsehit->momy = 0;
   if(comp[comp_vile])
   {                                                              // phares
      corpsehit->height <<= 2;                                    //   V
      check = P_CheckPosition(corpsehit,corpsehit->x,corpsehit->y);
      corpsehit->height >>= 2;
   }
   else
   {
      int height,radius;
      
      height = corpsehit->height; // save temporarily
      radius = corpsehit->radius; // save temporarily
      corpsehit->height = corpsehit->info->height;
      corpsehit->radius = corpsehit->info->radius;
      corpsehit->flags |= MF_SOLID;
      check = P_CheckPosition(corpsehit,corpsehit->x,corpsehit->y);
      corpsehit->height = height; // restore
      corpsehit->radius = radius; // restore                      //   ^
      corpsehit->flags &= ~MF_SOLID;
   }                                                              //   |
                                                                  // phares
   if(!check)
      return true;              // doesn't fit here
   return false;               // got one, so stop checking
}

//
// A_VileChase
// Check for ressurecting a body
//

void A_VileChase(mobj_t* actor)
{
   int xl, xh;
   int yl, yh;
   int bx, by;

   if(actor->movedir != DI_NODIR)
   {
      // check for corpses to raise
      viletryx =
         actor->x + actor->info->speed*xspeed[actor->movedir];
      viletryy =
         actor->y + actor->info->speed*yspeed[actor->movedir];
      
      xl = (viletryx - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
      xh = (viletryx - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
      yl = (viletryy - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
      yh = (viletryy - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

      vileobj = actor;
      for(bx=xl ; bx<=xh ; bx++)
      {
         for(by=yl ; by<=yh ; by++)
         {
            // Call PIT_VileCheck to check
            // whether object is a corpse
            // that can be raised.
            if(!P_BlockThingsIterator(bx,by,PIT_VileCheck))
            {
               mobjinfo_t *info;
               
               // got one!
               mobj_t *temp = actor->target;
               actor->target = corpsehit;
               A_FaceTarget(actor);
               actor->target = temp;

               P_SetMobjState(actor, E_SafeState(S_VILE_HEAL1));
               S_StartSound(corpsehit, sfx_slop);
               info = corpsehit->info;
               
               P_SetMobjState(corpsehit,info->raisestate);
               
               if (comp[comp_vile])
                  corpsehit->height <<= 2;                        // phares
               else                                               //   V
               {
                  corpsehit->height = info->height; // fix Ghost bug
                  corpsehit->radius = info->radius; // fix Ghost bug
               }                                                  // phares
               
               // killough 7/18/98: 
               // friendliness is transferred from AV to raised corpse
               corpsehit->flags = 
                  (info->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);
               
               corpsehit->health = info->spawnhealth;
               P_SetTarget(&corpsehit->target, NULL);  // killough 11/98

               if(demo_version >= 203)
               {         // kilough 9/9/98
                  P_SetTarget(&corpsehit->lastenemy, NULL);
                  corpsehit->flags &= ~MF_JUSTHIT;
               }
               
               // killough 8/29/98: add to appropriate thread
               P_UpdateThinker(&corpsehit->thinker);
               
               return;
            }
         }
      }
   }
   A_Chase(actor);  // Return to normal attack.
}

//
// A_VileStart
//

void A_VileStart(mobj_t *actor)
{
   S_StartSound(actor, sfx_vilatk);
}

//
// A_Fire
// Keep fire in front of player unless out of sight
//

void A_Fire(mobj_t *actor);

void A_StartFire(mobj_t *actor)
{
   S_StartSound(actor,sfx_flamst);
   A_Fire(actor);
}

void A_FireCrackle(mobj_t* actor)
{
   S_StartSound(actor,sfx_flame);
   A_Fire(actor);
}

void A_Fire(mobj_t *actor)
{
   unsigned an;
   mobj_t *dest = actor->tracer;
   
   if(!dest)
      return;
   
   // don't move it if the vile lost sight
   if(!P_CheckSight(actor->target, dest) )
      return;
   
   an = dest->angle >> ANGLETOFINESHIFT;
   
   P_UnsetThingPosition(actor);
   actor->x = dest->x + FixedMul(24*FRACUNIT, finecosine[an]);
   actor->y = dest->y + FixedMul(24*FRACUNIT, finesine[an]);
   actor->z = dest->z;
   P_SetThingPosition(actor);
}

//
// A_VileTarget
// Spawn the hellfire
//

void A_VileTarget(mobj_t *actor)
{
   mobj_t *fog;
   
   if(!actor->target)
      return;

   A_FaceTarget(actor);
   
   // killough 12/98: fix Vile fog coordinates
   fog = P_SpawnMobj(actor->target->x,
                     demo_version < 203 ? actor->target->x : actor->target->y,
                     actor->target->z,E_SafeThingType(MT_FIRE));
   
   P_SetTarget(&actor->tracer, fog);   // killough 11/98
   P_SetTarget(&fog->target, actor);
   P_SetTarget(&fog->tracer, actor->target);
   A_Fire(fog);
}

//
// A_VileAttack
//

void A_VileAttack(mobj_t *actor)
{
   mobj_t *fire;
   int    an;
   
   if(!actor->target)
      return;

   A_FaceTarget(actor);
   
   if(!P_CheckSight(actor, actor->target))
      return;

   S_StartSound(actor, sfx_barexp);
   P_DamageMobj(actor->target, actor, actor, 20, MOD_UNKNOWN);
   actor->target->momz = 1000*FRACUNIT/actor->target->info->mass;

   an = actor->angle >> ANGLETOFINESHIFT;
   
   fire = actor->tracer;
   
   if(!fire)
      return;

   // move the fire between the vile and the player
   fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
   fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
   P_RadiusAttack(fire, actor, 70, MOD_UNKNOWN);
}

//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.              haleyjd: weird BOOM
//                                             comment #345932

#define FATSPREAD       (ANG90/8)

void A_FatRaise(mobj_t *actor)
{
   A_FaceTarget(actor);
   S_StartSound(actor, sfx_manatk);
}

static int FatShotType = -1;

void A_FatAttack1(mobj_t *actor)
{
   mobj_t *mo;
   int    an;
   fixed_t z = actor->z + DEFAULTMISSILEZ;
   
   if(FatShotType == -1)
      FatShotType = E_SafeThingType(MT_FATSHOT);
   
   A_FaceTarget(actor);

   // Change direction  to ...
   actor->angle += FATSPREAD;
   
   P_SpawnMissile(actor, actor->target, FatShotType, z);
   
   mo = P_SpawnMissile(actor, actor->target, FatShotType, z);
   mo->angle += FATSPREAD;
   an = mo->angle >> ANGLETOFINESHIFT;
   mo->momx = FixedMul(mo->info->speed, finecosine[an]);
   mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

void A_FatAttack2(mobj_t *actor)
{
   mobj_t *mo;
   int    an;
   fixed_t z = actor->z + DEFAULTMISSILEZ;
   
   if(FatShotType == -1)
      FatShotType = E_SafeThingType(MT_FATSHOT);

   A_FaceTarget(actor);
   // Now here choose opposite deviation.
   actor->angle -= FATSPREAD;
   P_SpawnMissile(actor, actor->target, FatShotType, z);
   
   mo = P_SpawnMissile(actor, actor->target, FatShotType, z);
   mo->angle -= FATSPREAD*2;
   an = mo->angle >> ANGLETOFINESHIFT;
   mo->momx = FixedMul(mo->info->speed, finecosine[an]);
   mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

void A_FatAttack3(mobj_t *actor)
{
   mobj_t *mo;
   int    an;
   fixed_t z = actor->z + DEFAULTMISSILEZ;
   
   if(FatShotType == -1)
      FatShotType = E_SafeThingType(MT_FATSHOT);
   
   A_FaceTarget(actor);
   
   mo = P_SpawnMissile(actor, actor->target, FatShotType, z);
   mo->angle -= FATSPREAD/2;
   an = mo->angle >> ANGLETOFINESHIFT;
   mo->momx = FixedMul(mo->info->speed, finecosine[an]);
   mo->momy = FixedMul(mo->info->speed, finesine[an]);
   
   mo = P_SpawnMissile(actor, actor->target, FatShotType, z);
   mo->angle += FATSPREAD/2;
   an = mo->angle >> ANGLETOFINESHIFT;
   mo->momx = FixedMul(mo->info->speed, finecosine[an]);
   mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED              (20*FRACUNIT)

void A_SkullAttack(mobj_t *actor)
{
   mobj_t  *dest;
   angle_t an;
   int     dist;
   
   if(!actor->target)
      return;

   dest = actor->target;
   actor->flags |= MF_SKULLFLY;
   
   S_StartSound(actor, actor->info->attacksound);

   A_FaceTarget(actor);
   an = actor->angle >> ANGLETOFINESHIFT;
   actor->momx = FixedMul(SKULLSPEED, finecosine[an]);
   actor->momy = FixedMul(SKULLSPEED, finesine[an]);
   dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
   dist = dist / SKULLSPEED;
   
   if(dist < 1)
      dist = 1;
   actor->momz = (dest->z+(dest->height>>1) - actor->z) / dist;
}

// sf: removed beta lost soul

void A_Stop(mobj_t *actor)
{
   actor->momx = actor->momy = actor->momz = 0;
}

//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//

void A_PainShootSkull(mobj_t *actor, angle_t angle)
{
   fixed_t       x,y,z;
   mobj_t        *newmobj;
   angle_t       an;
   int           prestep;
   static int    skullType = -1;
      
   if(skullType == -1)
      skullType = E_SafeThingType(MT_SKULL);

// The original code checked for 20 skulls on the level,    // phares
// and wouldn't spit another one if there were. If not in   // phares
// compatibility mode, we remove the limit.                 // phares

   if (comp[comp_pain])  // killough 10/98: compatibility-optioned
   {
      // count total number of skulls currently on the level
      int count = 20;
      thinker_t *currentthinker;
      for(currentthinker = thinkercap.next;
          currentthinker != &thinkercap;
          currentthinker = currentthinker->next)
      {
         if((currentthinker->function == P_MobjThinker) &&
            ((mobj_t *)currentthinker)->type == skullType)
         {
            if(--count < 0)         // killough 8/29/98: early exit
               return;
         }
      }
   }

   // okay, there's room for another one
   
   an = angle >> ANGLETOFINESHIFT;
   
   prestep = 4*FRACUNIT + 3*(actor->info->radius + mobjinfo[skullType].radius)/2;

   x = actor->x + FixedMul(prestep, finecosine[an]);
   y = actor->y + FixedMul(prestep, finesine[an]);
   z = actor->z + 8*FRACUNIT;
   
   if (comp[comp_skull])   // killough 10/98: compatibility-optioned
      newmobj = P_SpawnMobj(x, y, z, skullType);                    // phares
   else                                                             //   V
   {
      // Check whether the Lost Soul is being fired through a 1-sided
      // wall or an impassible line, or a "monsters can't cross" line.
      // If it is, then we don't allow the spawn. This is a bug fix, 
      // but it should be considered an enhancement, since it may 
      // disturb existing demos, so don't do it in compatibility mode.
      
      if (Check_Sides(actor,x,y))
         return;
      
      newmobj = P_SpawnMobj(x, y, z, skullType);
      
      // Check to see if the new Lost Soul's z value is above the
      // ceiling of its new sector, or below the floor. If so, kill it.
      
      if((newmobj->z >
         (newmobj->subsector->sector->ceilingheight - newmobj->height)) ||
         (newmobj->z < newmobj->subsector->sector->floorheight))
      {
         // kill it immediately
         P_DamageMobj(newmobj,actor,actor,10000,MOD_UNKNOWN);
         return;                                                    //   ^
      }                                                             //   |
   }                                                                // phares

   // killough 7/20/98: PEs shoot lost souls with the same friendliness
   newmobj->flags = 
      (newmobj->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

   // killough 8/29/98: add to appropriate thread
   P_UpdateThinker(&newmobj->thinker);

   // Check for movements.
   // killough 3/15/98: don't jump over dropoffs:

   if(!P_TryMove(newmobj, newmobj->x, newmobj->y, false))
   {
      // kill it immediately
      P_DamageMobj(newmobj, actor, actor, 10000,MOD_UNKNOWN);
      return;
   }
   
   P_SetTarget(&newmobj->target, actor->target);
   A_SkullAttack(newmobj);
}

//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//

void A_PainAttack(mobj_t *actor)
{
   if(!actor->target)
      return;
   A_FaceTarget(actor);
   A_PainShootSkull(actor, actor->angle);
}

void A_PainDie(mobj_t *actor)
{
   A_Fall(actor);
   A_PainShootSkull(actor, actor->angle+ANG90);
   A_PainShootSkull(actor, actor->angle+ANG180);
   A_PainShootSkull(actor, actor->angle+ANG270);
}

void A_Scream(mobj_t *actor)
{
   int sound;
   
   switch (actor->info->deathsound)
   {
   case 0:
      return;

   case sfx_podth1:
   case sfx_podth2:
   case sfx_podth3:
      sound = sfx_podth1 + P_Random(pr_scream)%3;
      break;
      
   case sfx_bgdth1:
   case sfx_bgdth2:
      sound = sfx_bgdth1 + P_Random(pr_scream)%2;
      break;
      
   default:
      sound = actor->info->deathsound;
      break;
   }

   // Check for bosses.
   // haleyjd: generalize to all bosses
   if(actor->flags2&MF2_BOSS)
      S_StartSound(NULL, sound); // full volume
   else
      S_StartSound(actor, sound);
}

void A_XScream(mobj_t *actor)
{
   int sound = sfx_slop;
   
   // haleyjd: falling damage
   if(!comp[comp_fallingdmg] && demo_version >= 329)
   {
      if(actor->player && actor->intflags & MIF_DIEDFALLING)
         sound = sfx_fallht;
   }
   
   S_StartSound(actor, sound);
}

void A_Pain(mobj_t *actor)
{
   S_StartSound(actor, actor->info->painsound);
}

void A_Fall(mobj_t *actor)
{
   // actor is on ground, it can be walked over
   actor->flags &= ~MF_SOLID;
}

// killough 11/98: kill an object
void A_Die(mobj_t *actor)
{
   actor->flags2 &= ~MF2_INVULNERABLE;  // haleyjd: just in case
   P_DamageMobj(actor, NULL, NULL, actor->health,MOD_UNKNOWN);
}

//
// A_Explode
//
void A_Explode(mobj_t *thingy)
{
   P_RadiusAttack(thingy, thingy->target, 128, thingy->info->mod);
   
   if(!comp[comp_terrain] && 
      (thingy->z <= thingy->floorz + 128*FRACUNIT))
      P_HitFloor(thingy); // haleyjd: TerrainTypes
}

void A_Nailbomb(mobj_t *thing)
{
   int i;
   
   P_RadiusAttack(thing, thing->target, 128, thing->info->mod);

   // haleyjd: added here as of 3.31b3 -- was overlooked
   if(demo_version >= 331 && !comp[comp_terrain] &&
      (thing->z <= thing->floorz + 128*FRACUNIT))
      P_HitFloor(thing);

   for(i=0; i<30; i++)
      P_LineAttack(thing, i*(ANG180/15), MISSILERANGE, 0, 10);
}


//
// A_Detonate
// killough 8/9/98: same as A_Explode, except that the damage is variable
//

void A_Detonate(mobj_t *mo)
{
   P_RadiusAttack(mo, mo->target, mo->info->damage, mo->info->mod);

   // haleyjd: added here as of 3.31b3 -- was overlooked
   if(demo_version >= 331 && !comp[comp_terrain] &&
      (mo->z <= mo->floorz + mo->info->damage*FRACUNIT))
      P_HitFloor(mo);
}

//
// killough 9/98: a mushroom explosion effect, sorta :)
// Original idea: Linguica
//

void A_Mushroom(mobj_t *actor)
{
   int i, j, n = actor->info->damage;
   int arg0, ShotType;
   
   // Mushroom parameters are part of code pointer's state
   fixed_t misc1 = 
      actor->state->misc1 ? actor->state->misc1 : FRACUNIT*4;
   fixed_t misc2 = 
      actor->state->misc2 ? actor->state->misc2 : FRACUNIT/2;

   if(FatShotType == -1)
      FatShotType = E_SafeThingType(MT_FATSHOT);

   // haleyjd: extended parameter support requested by Mordeth:
   // allow specification of thing type in args[0]   
   if((arg0 = actor->state->args[0]))
      ShotType = E_SafeThingType(arg0);
   else
      ShotType = FatShotType;
   
   A_Explode(actor);               // make normal explosion

   for(i = -n; i <= n; i += 8)    // launch mushroom cloud
   {
      for(j = -n; j <= n; j += 8)
      {
         mobj_t target = *actor, *mo;
         target.x += i << FRACBITS;    // Aim in many directions from source
         target.y += j << FRACBITS;
         target.z += P_AproxDistance(i,j) * misc1;         // Aim fairly high
         mo = P_SpawnMissile(actor, &target, ShotType,
                             actor->z + DEFAULTMISSILEZ);  // Launch fireball
         mo->momx = FixedMul(mo->momx, misc2);
         mo->momy = FixedMul(mo->momy, misc2);             // Slow down a bit
         mo->momz = FixedMul(mo->momz, misc2);
         mo->flags &= ~MF_NOGRAVITY;   // Make debris fall under gravity
      }
   }
}

//
// A_BossDeath
//
// Possibly trigger special effects if on boss level
//
// haleyjd: enhanced to allow user specification of the thing types
// allowed to trigger each special effect
// TODO: allow the maps on which effects can take place to be set
// via MapInfo
//

void A_BossDeath(mobj_t *mo)
{
   thinker_t *th;
   line_t    junk;
   int       i;
   unsigned int flag = 0; // haleyjd
   
   if(gamemode == commercial)
   {
      if(gamemap != 7)
         return;

      if(!(mo->flags2 & MF2_MAP07BOSS1) &&
         !(mo->flags2 & MF2_MAP07BOSS2))
         return;
      
      if(mo->flags2 & MF2_MAP07BOSS1)
         flag = MF2_MAP07BOSS1;
      else
         flag = MF2_MAP07BOSS2;
   }
   else
   {
      switch(gameepisode)
      {
      case 1:
         if(gamemap != 8)
            return;

         if(!(mo->flags2 & MF2_E1M8BOSS))
            return;
         flag = MF2_E1M8BOSS;
         break;

      case 2:
         if (gamemap != 8)
            return;

         if(!(mo->flags2 & MF2_E2M8BOSS))
            return;
         flag = MF2_E2M8BOSS;
         break;

      case 3:
         if (gamemap != 8)
            return;

         if(!(mo->flags2 & MF2_E3M8BOSS))
            return;
         flag = MF2_E3M8BOSS;
         break;

      case 4:
         switch(gamemap)
         {
         case 6:
            if(!(mo->flags2 & MF2_E4M6BOSS))
               return;
            flag = MF2_E4M6BOSS;
            break;
            
         case 8:
            if(!(mo->flags2 & MF2_E4M8BOSS))
               return;
            flag = MF2_E4M8BOSS;
            break;
            
         default:
            return;
            break;
         }
         break;

      default:
        if (gamemap != 8)
           return;
        break;
      }
      
   }

   // make sure there is a player alive for victory
   for(i=0; i<MAXPLAYERS; i++)
   {
      if(playeringame[i] && players[i].health > 0)
         break;
   }

   if(i==MAXPLAYERS)
      return;     // no one left alive, so do not end game

   // scan the remaining thinkers to see
   // if all bosses are dead
   for(th = thinkercap.next ; th != &thinkercap ; th=th->next)
   {
      if(th->function == P_MobjThinker)
      {
         // haleyjd: check not only for all of the same type, but
         // also all of any other species marked as a boss of the
         // same activation class
         mobj_t *mo2 = (mobj_t *) th;
         if(mo2 != mo && 
            (mo2->type == mo->type || (mo2->flags2 & flag)) && 
            mo2->health > 0)
            return;         // other boss not dead
      }
   }

   // victory!
   if(gamemode == commercial)
   {
      if (gamemap == 7)
      {
         if(mo->flags2 & MF2_MAP07BOSS1)
         {
            junk.tag = 666;
            EV_DoFloor(&junk,lowerFloorToLowest);
            return;
         }
         
         if(mo->flags2 & MF2_MAP07BOSS2)
         {
            junk.tag = 667;
            EV_DoFloor(&junk,raiseToTexture);
            return;
         }
      }
   }
   else
   {
      switch(gameepisode)
      {
      case 1:
         junk.tag = 666;
         EV_DoFloor(&junk, lowerFloorToLowest);
         return;
         break;

      case 4:
         switch(gamemap)
         {
         case 6:
            junk.tag = 666;
            EV_DoDoor(&junk, blazeOpen);
            return;
            break;

         case 8:
            junk.tag = 666;
            EV_DoFloor(&junk, lowerFloorToLowest);
            return;
            break;
         }
      }
   }

   G_ExitLevel();
}

void A_Hoof (mobj_t* mo)
{
   S_StartSound(mo, sfx_hoof);
   A_Chase(mo);
}

void A_Metal(mobj_t *mo)
{
   S_StartSound(mo, sfx_metal);
   A_Chase(mo);
}

void A_BabyMetal(mobj_t *mo)
{
   S_StartSound(mo, sfx_bspwlk);
   A_Chase(mo);
}

void A_OpenShotgun2(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_dbopn);
}

void A_LoadShotgun2(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_dbload);
}

void A_ReFire(player_t *player, pspdef_t *psp);

void A_CloseShotgun2(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_dbcls);
   A_ReFire(player,psp);
}

// killough 2/7/98: Remove limit on icon landings:
mobj_t **braintargets;
int    numbraintargets_alloc;
int    numbraintargets;

struct brain_s brain;   // killough 3/26/98: global state of boss brain

// killough 3/26/98: initialize icon landings at level startup,
// rather than at boss wakeup, to prevent savegame-related crashes

void P_SpawnBrainTargets(void)  // killough 3/26/98: renamed old function
{
   int BrainSpotType = E_ThingNumForDEHNum(MT_BOSSTARGET);

   // find all the target spots
   numbraintargets = 0;
   brain.targeton = 0;
   brain.easy = 0;   // killough 3/26/98: always init easy to 0

   if(BrainSpotType == NUMMOBJTYPES)
      return;

   // haleyjd: use new generalized P_CollectThings
   P_CollectThings(BrainSpotType, &numbraintargets,
                   &numbraintargets_alloc, &braintargets);
}

//
// P_CollectThings
//
// haleyjd 11/19/02: Generalized this function out of the brain
// target spawning code above, so that it can be used in
// multiple places -- D'Sparil's teleport things, for instance.
// Notice this is the first time I've ever needed a 
// pointer-to-pointer-to-pointer.
//
// * Quasar` is scared
//
void P_CollectThings(mobjtype_t type, int *num, int *numalloc,
                     mobj_t ***ppArray)
{
   thinker_t *thinker;

   for(thinker = thinkercap.next; thinker != &thinkercap; thinker = thinker->next)
   {
      if(thinker->function == P_MobjThinker)
      {
         mobj_t *mo = (mobj_t *)thinker;

         if(mo->type == type)
         {
            if(*num >= *numalloc)
            {
               *ppArray = realloc(*ppArray,
                  (*numalloc = *numalloc ?
                   *numalloc*2 : 32) * sizeof **ppArray);
            }
            (*ppArray)[*num] = mo;
            *num = *num + 1;
         }
      }
   }
}

void A_BrainAwake(mobj_t *mo)
{
   S_StartSound(NULL,sfx_bossit); // killough 3/26/98: only generates sound now
}

void A_BrainPain(mobj_t *mo)
{
   S_StartSound(NULL,sfx_bospn);
}

static boolean peventflag = false;

void A_BrainScream(mobj_t *mo)
{
   int x;
   static int rocketType = -1;
   
   if(rocketType == -1)
      rocketType = E_SafeThingType(MT_ROCKET);

   // haleyjd 04/10/03: disable rocket explosion particle event
   // during boss brain explosion (looks retarded)
   if(particleEvents[P_EVENT_ROCKET_EXPLODE].enabled)
   {
      peventflag = true;
      particleEvents[P_EVENT_ROCKET_EXPLODE].enabled = 0;
   }

   for(x=mo->x - 196*FRACUNIT ; x< mo->x + 320*FRACUNIT ; x+= FRACUNIT*8)
   {
      int y = mo->y - 320*FRACUNIT;
      int z = 128 + P_Random(pr_brainscream)*2*FRACUNIT;
      mobj_t *th = P_SpawnMobj (x,y,z, rocketType);
      th->momz = P_Random(pr_brainscream)*512;
      P_SetMobjState(th, E_SafeState(S_BRAINEXPLODE1));
      th->tics -= P_Random(pr_brainscream)&7;
      if(th->tics < 1)
         th->tics = 1;
   }
   S_StartSound(NULL,sfx_bosdth);
}

void A_BrainExplode(mobj_t *mo)
{  // killough 5/5/98: remove dependence on order of evaluation:
   int t = P_Random(pr_brainexp);
   int x = mo->x + (t - P_Random(pr_brainexp))*2048;
   int y = mo->y;
   int z = 128 + P_Random(pr_brainexp)*2*FRACUNIT;
   mobj_t *th = P_SpawnMobj(x,y,z, E_SafeThingType(MT_ROCKET));
   th->momz = P_Random(pr_brainexp)*512;
   P_SetMobjState(th, E_SafeState(S_BRAINEXPLODE1));
   th->tics -= P_Random(pr_brainexp)&7;
   if(th->tics < 1)
      th->tics = 1;
}

void A_BrainDie(mobj_t *mo)
{
   // haleyjd: re-enable rocket explosions if they were disabled by
   // A_BrainScream
   if(peventflag)
   {
      peventflag = false;
      particleEvents[P_EVENT_ROCKET_EXPLODE].enabled = 1;
   }

   G_ExitLevel();
}

void A_BrainSpit(mobj_t *mo)
{
   mobj_t *targ, *newmobj;
   static int SpawnShotType = -1;
   
   if(SpawnShotType == -1)
      SpawnShotType = E_SafeThingType(MT_SPAWNSHOT);
   
   if(!numbraintargets)     // killough 4/1/98: ignore if no targets
      return;

   brain.easy ^= 1;          // killough 3/26/98: use brain struct
   if(gameskill <= sk_easy && !brain.easy)
      return;

   // shoot a cube at current target
   targ = braintargets[brain.targeton++]; // killough 3/26/98:
   brain.targeton %= numbraintargets;     // Use brain struct for targets

   // spawn brain missile
   newmobj = P_SpawnMissile(mo, targ, SpawnShotType, 
                            mo->z + DEFAULTMISSILEZ);
   P_SetTarget(&newmobj->target, targ);
   newmobj->reactiontime = (short)(((targ->y-mo->y)/newmobj->momy)/newmobj->state->tics);

   // killough 7/18/98: brain friendliness is transferred
   newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
   
   // killough 8/29/98: add to appropriate thread
   P_UpdateThinker(&newmobj->thinker);
   
   S_StartSound(NULL, sfx_bospit);
}

void A_SpawnFly(mobj_t *mo);

// travelling cube sound
void A_SpawnSound(mobj_t *mo)
{
   S_StartSound(mo,sfx_boscub);
   A_SpawnFly(mo);
}

// haleyjd 07/13/03: editable boss brain spawn types
// schepe: removed 11-type limit
int NumBossTypes;
int *BossSpawnTypes;
int *BossSpawnProbs;

void A_SpawnFly(mobj_t *mo)
{
   int    i;         // schepe 
   mobj_t *newmobj;  // killough 8/9/98
   int    r;
   mobjtype_t type = 0;
   static int fireType = -1;
      
   mobj_t *fog;
   mobj_t *targ;

   if(fireType == -1)
      fireType = E_SafeThingType(MT_SPAWNFIRE);

   if(--mo->reactiontime)
      return;     // still flying

   targ = mo->target;
   
   // First spawn teleport fog.
   fog = P_SpawnMobj(targ->x, targ->y, targ->z, fireType);
   
   S_StartSound(fog, sfx_telept);
   
   // Randomly select monster to spawn.
   r = P_Random(pr_spawnfly);

   // Probability distribution (kind of :), decreasing likelihood.
   // schepe:
   for(i = 0; i < NumBossTypes; ++i)
   {
      if(r < BossSpawnProbs[i])
      {
         type = BossSpawnTypes[i];
         break;
      }
   }

   newmobj = P_SpawnMobj(targ->x, targ->y, targ->z, type);
   
   // killough 7/18/98: brain friendliness is transferred
   newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);

   // killough 8/29/98: add to appropriate thread
   P_UpdateThinker(&newmobj->thinker);
   
   if(P_LookForTargets(newmobj,true))      // killough 9/4/98
      P_SetMobjState(newmobj, newmobj->info->seestate);
   
   // telefrag anything in this spot
   P_TeleportMove(newmobj, newmobj->x, newmobj->y, true); // killough 8/9/98
   
   // remove self (i.e., cube).
   P_RemoveMobj(mo);
}

void A_PlayerScream(mobj_t *mo)
{
   int sound = sfx_pldeth;  // Default death sound.
   if(gamemode != shareware && mo->health < -50) // killough 12/98
      sound = sfx_pdiehi;   // IF THE PLAYER DIES LESS THAN -50% WITHOUT GIBBING
   
   // haleyjd: maybe we died from falling, if so, gross falling death sound
   if(!comp[comp_fallingdmg] && demo_version >= 329 && 
      mo->intflags & MIF_DIEDFALLING)
      sound = sfx_fallht;
   
   S_StartSound(mo, sound);
}

//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie(mobj_t* mo)
{
   thinker_t *th;
   line_t   junk;
   
   A_Fall(mo);

   // scan the remaining thinkers to see if all Keens are dead
   
   for(th = thinkercap.next ; th != &thinkercap ; th=th->next)
   {
      if(th->function == P_MobjThinker)
      {
         mobj_t *mo2 = (mobj_t *) th;
         if(mo2 != mo && mo2->type == mo->type && mo2->health > 0)
            return;                           // other Keen not dead
      }
   }

   junk.tag = 666;
   EV_DoDoor(&junk,doorOpen);
}

//
// killough 11/98
//
// The following were inspired by Len Pitre
//
// A small set of highly-sought-after code pointers
//

void A_Spawn(mobj_t *mo)
{
   if(mo->state->misc1)
   {
      mobj_t *newmobj;

      // haleyjd 03/06/03 -- added error check
      //         07/05/03 -- adjusted for EDF
      int thingtype = E_SafeThingType((int)(mo->state->misc1));
      
      newmobj = 
         P_SpawnMobj(mo->x, mo->y, 
                     (mo->state->misc2 << FRACBITS) + mo->z,
                     thingtype);
      if(newmobj)
         newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
   }
}

void A_Turn(mobj_t *mo)
{
   mo->angle += (angle_t)(((ULong64) mo->state->misc1 << 32) / 360);
}

void A_Face(mobj_t *mo)
{
   mo->angle = (angle_t)(((ULong64) mo->state->misc1 << 32) / 360);
}

void A_Scratch(mobj_t *mo)
{
   // haleyjd: demystified
   if(!mo->target)
      return;

   A_FaceTarget(mo);

   if(P_CheckMeleeRange(mo))
   {
      if(mo->state->misc2)
         S_StartSound(mo, mo->state->misc2);

      P_DamageMobj(mo->target, mo, mo, mo->state->misc1, MOD_HIT);
   }   
}

void A_PlaySound(mobj_t *mo)
{
   S_StartSound(mo->state->misc2 ? NULL : mo, mo->state->misc1);
}

void A_RandomJump(mobj_t *mo)
{
   // haleyjd 03/06/03: rewrote to be failsafe
   //         07/05/03: adjusted for EDF
   int statenum = mo->state->misc1;

   statenum = E_StateNumForDEHNum(statenum);
   if(statenum == NUMSTATES)
      return;

   if(P_Random(pr_randomjump) < mo->state->misc2)
      P_SetMobjState(mo, statenum);
}

//
// This allows linedef effects to be activated inside deh frames.
//

void A_LineEffect(mobj_t *mo)
{
   if(!(mo->intflags & MIF_LINEDONE))                // Unless already used up
   {
      line_t junk = *lines;                          // Fake linedef set to 1st
      if((junk.special = (short)mo->state->misc1))   // Linedef type
      {
         player_t player, *oldplayer = mo->player;   // Remember player status
         mo->player = &player;                       // Fake player
         player.health = 100;                        // Alive player
         junk.tag = (short)mo->state->misc2;         // Sector tag for linedef
         if(!P_UseSpecialLine(mo, &junk, 0))         // Try using it
            P_CrossSpecialLine(&junk, 0, mo);        // Try crossing it
         if(!junk.special)                           // If type cleared,
            mo->intflags |= MIF_LINEDONE;            // no more for this thing
         mo->player = oldplayer;                     // Restore player status
      }
   }
}

//
// haleyjd: Start Eternity Engine action functions
//

//
// A_SetFlags
//
// A parameterized codepointer that turns on thing flags
//
// args[0] == 1, 2, 3 -- flags field to affect
// args[1] == flags value to OR with thing flags
//
void A_SetFlags(mobj_t *actor)
{
   int flagfield = (int)(actor->state->args[0]);
   unsigned long flags = (unsigned long)(actor->state->args[1]);

   switch(flagfield)
   {
   case 1:
      actor->flags |= flags;
      break;
   case 2:
      actor->flags2 |= flags;
      break;
   case 3:
      actor->flags3 |= flags;
      break;
   }
}

//
// A_UnSetFlags
//
// A parameterized codepointer that turns off thing flags
//
// args[0] == 1, 2, 3 -- flags field to affect
// args[1] == flags value to inverse AND with thing flags
//
void A_UnSetFlags(mobj_t *actor)
{
   int flagfield = (int)(actor->state->args[0]);
   unsigned long flags = (unsigned long)(actor->state->args[1]);

   switch(flagfield)
   {
   case 1:
      actor->flags &= ~flags;
      break;
   case 2:
      actor->flags2 &= ~flags;
      break;
   case 3:
      actor->flags3 &= ~flags;
      break;
   }
}

// haleyjd: add back for mbf dehacked patch compatibility
//          might be a useful function to someone, anyway

//
// A_BetaSkullAttack()
// killough 10/98: this emulates the beta version's lost soul attacks
//

void A_BetaSkullAttack(mobj_t *actor)
{
   int damage;
   
   // haleyjd: changed to check if objects are the SAME type, not
   // for hard-coded lost soul
   if(!actor->target || actor->target->type == actor->type)
      return;
   
   S_StartSound(actor, actor->info->attacksound);
   
   A_FaceTarget(actor);
   damage = (P_Random(pr_skullfly)%8+1)*actor->info->damage;
   P_DamageMobj(actor->target, actor, actor, damage, MOD_UNKNOWN);
}

//
// A_StartScript
//
// Parameterized codepointer for starting Small scripts
//
// args[0] - script number to start
// args[1] - select vm (0 == gamescript, 1 == levelscript)
// args[2-4] - parameters to script (must accept 3 params)
//
void A_StartScript(mobj_t *actor)
{
   AMX *vm;
   int scriptnum = (int)(actor->state->args[0]);
   int selectvm  = (int)(actor->state->args[1]);

   cell params[3] =
   {
      (cell)(actor->state->args[2]),
      (cell)(actor->state->args[3]),
      (cell)(actor->state->args[4]),
   };

   switch(selectvm)
   {
   default:
   case 0: // game script
      if(!gameScriptLoaded)
         return;
      vm = &gamescript;
      break;
   case 1: // level script
      if(!levelScriptLoaded)
         return;
      vm = &levelscript;
      break;
   }

   sc_invocation.invokeType = SC_INVOKE_THING;
   sc_invocation.trigger = actor;

   A_ExecScriptByNum(vm, scriptnum, 3, params);

   A_ClearInvocation();
}

//
// A_PlayerStartScript
//
// Parameterized player gun frame codepointer for starting
// Small scripts
//
// args[0] - script number to start
// args[1] - select vm (0 == gamescript, 1 == levelscript)
// args[2-4] - parameters to script (must accept 3 params)
//
void A_PlayerStartScript(player_t *player, pspdef_t *psp)
{
   AMX *vm;
   int scriptnum = (int)(psp->state->args[0]);
   int selectvm  = (int)(psp->state->args[1]);

   cell params[3] =
   {
      (cell)(psp->state->args[2]),
      (cell)(psp->state->args[3]),
      (cell)(psp->state->args[4]),
   };

   switch(selectvm)
   {
   default:
   case 0: // game script
      if(!gameScriptLoaded)
         return;
      vm = &gamescript;
      break;
   case 1: // level script
      if(!levelScriptLoaded)
         return;
      vm = &levelscript;
      break;
   }

   sc_invocation.invokeType = SC_INVOKE_PLAYER;
   sc_invocation.playernum = (int)(player - players);
   sc_invocation.trigger = player->mo;

   A_ExecScriptByNum(vm, scriptnum, 3, params);

   A_ClearInvocation();
}

//
// haleyjd: Start Eternity TC Action Functions
// TODO: possibly eliminate some of these
//

//
// A_MinotaurAtk1
//
void A_MinotaurAtk1(mobj_t *actor)
{
   player_t *player;

   if(!actor->target)
   {
      return;
   }
   S_StartSound(actor, sfx_stfpow);
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, ((1+(P_Random(pr_minatk1)&7))*5), MOD_HIT);
      if((player = actor->target->player) != NULL)
      { // squish player
         player->deltaviewheight = -16*FRACUNIT;
      }
   }
}

//
// A_MinotaurDecide
//
#define MNTR_CHARGE_SPEED (13*FRACUNIT)

void A_MinotaurDecide(mobj_t *actor)
{
   angle_t angle;
   mobj_t *target;
   int dist;

   target = actor->target;
   if(!target)
   {
      return;
   }
   S_StartSound(actor, sfx_minsit);
   dist = P_AproxDistance(actor->x - target->x, actor->y - target->y);
   if(target->z + target->height > actor->z
        && target->z + target->height < actor->z + actor->height
        && dist < 8*64*FRACUNIT
        && dist > 1*64*FRACUNIT
        && P_Random(pr_mindist) < 150)
   { // charge attack
      P_SetMobjStateNF(actor, E_SafeState(S_MNTR_ATK4_1));
      actor->flags |= MF_SKULLFLY;
      A_FaceTarget(actor);
      angle = actor->angle>>ANGLETOFINESHIFT;
      actor->momx = FixedMul(MNTR_CHARGE_SPEED, finecosine[angle]);
      actor->momy = FixedMul(MNTR_CHARGE_SPEED, finesine[angle]);
      // haleyjd 1/22/98: heretic special fields added 
      actor->special1 = 35/2;
   }
   else if(target->z == target->floorz
           && dist < 9*64*FRACUNIT
           && P_Random(pr_mindist) < 220)
   { // floor fire attack
      P_SetMobjState(actor, E_SafeState(S_MNTR_ATK3_1));
      actor->special2 = 0;
   }
   else
   { // swing attack
      A_FaceTarget(actor);
      // don't need to call P_SetMobjState, falls through to swing attack
   }
}

//
// A_MinotaurCharge
//
void A_MinotaurCharge(mobj_t *actor)
{
   mobj_t *puff;
   static int phoenixType = -1;
   
   if(phoenixType == -1)
      phoenixType = E_SafeThingType(MT_PHOENIXPUFF);

   if(actor->special1)
   {
      puff = P_SpawnMobj(actor->x, actor->y, actor->z, phoenixType);
      if(puff)
        puff->momz = 2*FRACUNIT;
      actor->special1--;
   }
   else
   {
      actor->flags &= ~MF_SKULLFLY;
      P_SetMobjState(actor, actor->info->seestate);
   }
}

static int mntrfxType = -1;

//
// A_MinotaurAtk2
//
void A_MinotaurAtk2(mobj_t *actor)
{
   mobj_t *mo;
   angle_t angle;
   fixed_t momz;
   fixed_t z = actor->z + 40*FRACUNIT;
   
   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX1);

   if(!actor->target)
   {
      return;
   }
   S_StartSound(actor, sfx_minat2);

   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, ((1+(P_Random(pr_minatk2)&7))*5), MOD_HIT);
      return;
   }
   mo = P_SpawnMissile(actor, actor->target, mntrfxType, z);

   if(mo)
   {
      S_StartSound(mo, sfx_minat2);
      momz = mo->momz;
      angle = mo->angle;
      P_SpawnMissileAngle(actor, mntrfxType, angle-(ANG45/8), momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle+(ANG45/8), momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle-(ANG45/16), momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle+(ANG45/16), momz, z);
   }
}

//
// A_MinotaurAtk3
//
void A_MinotaurAtk3(mobj_t *actor)
{
   mobj_t *mo;
   player_t *player;

   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX1);

   if(!actor->target)
   {
      return;
   }
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, ((1+(P_Random(pr_minatk3)&7))*5), MOD_HIT);
      if((player = actor->target->player) != NULL)
      {  // squish player
         player->deltaviewheight = -16*FRACUNIT;
      }
   }
   else
   {
      mo = P_SpawnMissile(actor, actor->target, mntrfxType, ONFLOORZ);
      if(mo)
      {
         S_StartSound(mo, sfx_minat1);
      }
   }
   if(P_Random(pr_minatk3) < 192 && actor->special2 == 0)
   {
      P_SetMobjState(actor, E_SafeState(S_MNTR_ATK3_4));
      actor->special2 = 1;
   }
}

//
// A_MntrFloorFire
//
void A_MntrFloorFire(mobj_t *actor)
{
   mobj_t *mo;

   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX1);

   actor->z = actor->floorz;
   mo = P_SpawnMobj(actor->x + ((P_Random(pr_mffire) - P_Random(pr_mffire))<<10),
                    actor->y + ((P_Random(pr_mffire) - P_Random(pr_mffire))<<10),
                    ONFLOORZ,
                    mntrfxType);
   if(mo)
   {
     mo->target = actor->target;
     mo->momx = 1;
     P_CheckMissileSpawn(mo);
   }
}
   
//
//  A_ETCBossDeath -- new death function for ETC bosses
//
void A_ETCBossDeath(mobj_t *actor)
{
}

//
// A_ClericAtk()
//
void A_ClericAtk(mobj_t *actor)
{
   mobj_t* mo;
   fixed_t momz;
   angle_t angle;
   fixed_t z = actor->z + DEFAULTMISSILEZ;

   if(!actor->target)
   {
      return;
   }

   A_FaceTarget(actor);

   if(P_CheckMeleeRange(actor))
   {
      int damage;
      S_StartSound(actor, sfx_clratk);
      damage = ((P_Random(pr_clrattack)%8)+1)*4;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
      return;
   }
   else if(actor->health*3 < actor->info->spawnhealth)
   {  // Limit break if under 1/3 life
      int clrballType = E_SafeThingType(MT_CLRBALL);

      mo = P_SpawnMissile(actor, actor->target, clrballType, z);

      if(mo)
      {
         momz = mo->momz;
         angle = mo->angle;
         P_SpawnMissileAngle(actor, clrballType, angle-(ANG45/8), momz, z);
         P_SpawnMissileAngle(actor, clrballType, angle+(ANG45/8), momz, z);
         P_SpawnMissileAngle(actor, clrballType, angle-(ANG45/16), momz, z);
         P_SpawnMissileAngle(actor, clrballType, angle+(ANG45/16), momz, z);
      }
      return;

   }
   else
   {
      P_SpawnMissile(actor, actor->target, 
                     E_SafeThingType(MT_CLRBALL), z);
   }
}

//
// A_FogSpawn
//
// The slightly-more-complicated-than-you-thought Hexen fog cloud generator
// Modified to use random initializer values as opposed to actor->args[]
// set in mapthing data fields
//
#define FOGSPEED 2
#define FOGFREQUENCY 8

void A_FogSpawn(mobj_t *actor)
{
   mobj_t *mo = NULL;
   angle_t delta;

   if(actor->special1-- > 0)
     return;

   actor->special1 = FOGFREQUENCY; // reset frequency

   switch(P_Random(pr_cloudpick)%3)
   {
   case 0:
      mo = P_SpawnMobj(actor->x, actor->y, actor->z, 
         E_SafeThingType(MT_FOGPATCHS));
      break;
   case 1:
      mo = P_SpawnMobj(actor->x, actor->y, actor->z, 
         E_SafeThingType(MT_FOGPATCHM));
      break;
   case 2:
      mo = P_SpawnMobj(actor->x, actor->y, actor->z, 
         E_SafeThingType(MT_FOGPATCHL));
      break;
   }
   if(mo)
   {
      delta = (P_Random(pr_fogangle)&0x7f)+1;
      mo->angle = actor->angle + (((P_Random(pr_fogangle)%delta)-(delta>>1))<<24);
      mo->target = actor;
      mo->args[0] = (P_Random(pr_fogangle)%FOGSPEED)+1; // haleyjd: narrowed range
      mo->args[3] = (P_Random(pr_fogcount)&0x7f)+1;
      mo->args[4] = 1;
      mo->special2 = P_Random(pr_fogfloat)&63;
   }
}

//
// A_FogMove
//

void A_FogMove(mobj_t *actor)
{
   int speed = actor->args[0]<<FRACBITS;
   angle_t angle;
   int weaveindex;

   if(!(actor->args[4]))
     return;

   if(actor->args[3]-- <= 0)
   {
      P_SetMobjStateNF(actor, actor->info->deathstate);
      return;
   }

   if((actor->args[3]%4) == 0)
   {
      weaveindex = actor->special2;
      actor->z += FloatBobOffsets[weaveindex]>>1;
      actor->special2 = (weaveindex+1)&63;
   }
   angle = actor->angle>>ANGLETOFINESHIFT;
   actor->momx = FixedMul(speed, finecosine[angle]);
   actor->momy = FixedMul(speed, finesine[angle]);
}


//=====================================================
// Leader Cleric Boss
//
// special1 --------- counter to teleport time
// special2 --------- count-down to end of defense
// special3 --------- boolean, currently defending
// 
// args[0]  --------- time to run to evade missile
//
//=====================================================

#define CLERICSTRAFETIME 3

void P_ClericSparkle(mobj_t *actor);

// Attempted missile avoidance with approx. 1/3 probability

void A_Cleric2Chase(mobj_t *actor)
{
   mobj_t *target = actor->target;

   // decrement teleport and defend countdown timers
   // while walking
   if(actor->special1 && !actor->args[0]) // not when strafing
     actor->special1--;
   if(actor->special2)
     actor->special2--;

   // sparkle if invulnerable
   if(actor->flags2&MF2_INVULNERABLE)
     P_ClericSparkle(actor);

   // Try to evade missile attack!
   if(actor->args[0])
   {
      actor->args[0]--;  // count-down to end of current evasion
   }
   else if(target && (P_Random(pr_clericevade) < 85) &&
           P_CheckSight(actor, target) &&
           !(target->flags & MF_CORPSE) &&
           !(actor->flags & target->flags & MF_FRIEND) &&
           !(actor->flags2 & MF2_INVULNERABLE))
   {
      P_AimLineAttack(target, target->angle, 16*64*FRACUNIT, 0);

      if(!linetarget)
      {
         A_Chase(actor);
         return;
      }

      // if aiming at cleric, or at something nearby...
      if(linetarget == actor ||
         linetarget->subsector == actor->subsector)
      {
         angle_t ang;
         
         ang = R_PointToAngle2(actor->x, actor->y,
                               target->x, target->y);
         if(P_Random(pr_clericevade)<128)
            ang += ANG90;
         else
            ang -= ANG90;
         
         ang >>= ANGLETOFINESHIFT;
         actor->momx = FixedMul(8*FRACUNIT, finecosine[ang]);
         actor->momy = FixedMul(8*FRACUNIT, finesine[ang]);
         actor->args[0] = CLERICSTRAFETIME;            // strafe time
      }
   }

   if(!actor->args[0])
     A_Chase(actor);
}

void P_ClericSparkle(mobj_t *actor)
{
   fixed_t x,y,z;
   int i,t;

   for(i=0; i<3; i++)
   {
      t = P_Random(pr_clrspark);

      x = actor->x + ((P_Random(pr_clrspark)&63)-16)*FRACUNIT;
      y = actor->y + ((P_Random(pr_clrspark)&63)-16)*FRACUNIT;
      z = actor->z + actor->info->height/2 + ((t - P_Random(pr_clrspark))<<15);
      if(z > (actor->z + actor->info->height)) // haleyjd 7/31/00: bug fix
        z = actor->z + actor->info->height;
      if(z < actor->floorz)
        z = actor->floorz;      
      P_SpawnMobj(x,y,z,E_SafeThingType(MT_SPARKLE));
   }
}

boolean P_ClericDefense(mobj_t *actor)
{
   if(actor->special2 == 0 && actor->special3 == 0 &&
      (P_Random(pr_clr2attack) < 128))
   {
      if(!actor->health)
       return false;

      P_SetMobjStateNF(actor, E_SafeState(S_LCLER_SPEC));

      if(info_lightning) // call down lightning from heaven!
        P_ForceLightning();

      S_StartSound(actor, sfx_clrdef);  // haleyjd 07/13/99: new sound

      switch(P_Random(pr_clr2attack)%3)
      {
         case 0:  // invisibility spell 1
            actor->flags3 |= MF3_GHOST;
            break;
         case 1:  // invisibility spell 2
            actor->flags |= MF_SHADOW;
            break;
         case 2:  // invulnerability spell
            actor->flags2 |= MF2_INVULNERABLE;
            actor->flags2 |= MF2_REFLECTIVE;
         default:   // just in case
            break;
      }
      actor->special2 = (P_Random(pr_clr2attack)&0x7f)+32;
      actor->special3 = 1;
      return true;  // defended this turn
   }
   else if(actor->special2 == 0 && actor->special3 == 1)
   {
      actor->flags3 &= ~MF3_GHOST;
      actor->flags &= ~MF_SHADOW;
      if(actor->flags2 & MF2_INVULNERABLE)
      {
        actor->flags2 &= ~MF2_INVULNERABLE;
        actor->flags2 &= ~MF2_REFLECTIVE;
      }
      actor->special2 = (P_Random(pr_clr2attack)&0x7f)+1; // reset count
      actor->special3 = 0;                                // not defending
   }
   return false; // no defense this turn
}

void A_Cleric2Attack(mobj_t *actor)
{
   fixed_t momz;
   angle_t angle;
   mobj_t *mo;
   fixed_t z = actor->z + DEFAULTMISSILEZ;

   if(!actor->target)
     return;

   if(actor->flags2&MF2_INVULNERABLE)
     P_ClericSparkle(actor);

   A_FaceTarget(actor);

   if(P_CheckMeleeRange(actor))
   {
      int damage;
      S_StartSound(actor, sfx_clratk);
      damage = ((P_Random(pr_clrattack)%8)+1)*4;
      P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
      return;
   }

   // defend, time and luck permitting
   if(P_ClericDefense(actor))
     return;

   // offensive spells
   if((actor->health*3 < actor->info->spawnhealth) &&
      (P_Random(pr_clr2attack) < 160))                  // limit break
   {
      P_SetMobjState(actor, E_SafeState(S_LCLER_BREAK1));
      return;
   }

   if(P_Random(pr_clr2attack) < 128)  // Triple Mace
   {
      int clrball = E_SafeThingType(MT_CLRBALL);

      mo = P_SpawnMissile(actor, actor->target, clrball, z);

      if(mo)
      {
         momz = mo->momz;
         angle = mo->angle;
         P_SpawnMissileAngle(actor, clrball, angle-(ANG45/8), momz, z);
         P_SpawnMissileAngle(actor, clrball, angle+(ANG45/8), momz, z);
      }
   }
   else                         // Fire Attack
   {
     // TODO: fire attack
   }
}

void P_ClericTeleport(mobj_t *actor)
{
   bossteleport_t bt;

   if(actor->flags2&MF2_INVULNERABLE)
     P_ClericSparkle(actor);

   bt.spotCount = numbraintargets;          // use boss brain targets
   bt.spots     = braintargets;
   bt.rngNum    = pr_clr2choose;            // use this rng
   bt.boss      = actor;                    // teleport leader cleric
   bt.state     = E_SafeState(S_LCLER_TELE1); // put cleric in this state
   bt.fxtype    = E_SafeThingType(MT_IFOG); // spawn item fog for fx
   bt.zpamt     = 24*FRACUNIT;              // add 24 units to fog z 
   bt.hereThere = BOSSTELE_BOTH;            // spawn fx @ origin & dest.
   bt.soundNum  = sfx_itmbk;                // use item respawn sound

   P_BossTeleport(&bt);
}

void A_Cleric2Decide(mobj_t *actor)
{
   if(actor->flags2&MF2_INVULNERABLE)
     P_ClericSparkle(actor);

   if(actor->special1 == 0 && numbraintargets != 0)
   {
      P_ClericTeleport(actor);
      actor->special1 = (P_Random(pr_clericteleport)&0x7f)+1;
   }
   // fall through to attack state
}

void A_ClericBreak(mobj_t *actor)
{
   angle_t ang, an1;
   mobj_t *target = actor->target;

   if(!target)
     return;

   if(actor->flags2&MF2_INVULNERABLE)
     P_ClericSparkle(actor);

   A_FaceTarget(actor);

   // Limit break         
   ang = R_PointToAngle2(actor->x, actor->y, target->x, target->y);
   an1 = ((P_Random(pr_clr2attack)&127) - 64) * (ANG90/768) + ang;

   P_SpawnMissileAngle(actor, E_SafeThingType(MT_CLRBALL), an1, 0, 
                       actor->z + DEFAULTMISSILEZ);
}

//==================================================
//
// Halif Swordsmythe -- Helper Dwarf Extraordinaire
//
//==================================================

#define DWARFALTEGOTIME 128
#define GOLEMTIME 256
#define FROZENFRICTION 160

void P_SpawnDwarfAlterego(mobj_t* actor, mobjtype_t type);
boolean P_CheckAlterEgo(mobjtype_t type);

// A_DwarfLDOCMagic
//  
//  Casts Life, Death, Order, and Chaos spells
//  Sets actor->special2 to 1 if FWAE magic
//  should not be cast afterward

void A_DwarfLDOCMagic(mobj_t *actor)
{
   mobj_t *mo;
   boolean can_see_enemy = false;
   int selections = deathmatch_p - deathmatchstarts;
   static int alterEgoType = -1;
   
   if(alterEgoType == -1)
      alterEgoType = E_SafeThingType(MT_ALTERDWARF);
   
   if(actor->target)
    can_see_enemy = P_CheckSight(actor, actor->target);

   if(actor->target && (actor->health*4 < actor->info->spawnhealth) &&
      (P_Random(pr_dwarfatk) < 24) &&
      !(actor->flags & actor->target->flags & MF_FRIEND))
   {  // Death

      mo = actor->target;

      if(!mo->player && !(mo->flags2&MF2_BOSS) && can_see_enemy)
      {         
         A_FaceTarget(actor);
         P_SetMobjState(actor, E_SafeState(S_DWARFREDAURA1));
         S_StartSound(NULL, sfx_clrdef);  // scary sound
         if(mo->info->spawnhealth <= 300)  // look of death for minor enemies
         {  // kill it            
            A_Die(mo);
         }
         else
         {  // critical damage
            P_DamageMobj(mo, actor, actor, ((mo->health*2)/3), MOD_UNKNOWN);
         }
        // death is an offensive spell, so will not be followed with a
        // second attack
        actor->special2 = 1;
        return;
      }
   }

   if((actor->health*3 < actor->info->spawnhealth) &&
      (P_Random(pr_dwarfatk) < 32))
   {  // Life -- self-healing
      P_SetMobjState(actor, E_SafeState(S_DWARFBLUEAURA1));
      S_StartSound(NULL, sfx_heal);
      actor->health += (P_Random(pr_dwarfatk)%251)+100; // haleyjd 1/14/00
      if(actor->health > actor->info->spawnhealth)      // made more useful
        actor->health = actor->info->spawnhealth;       // and limited to
      return;                                           // spawnhealth
   }

   if(actor->target && !(actor->target->flags2&MF2_BOSS) &&
      !actor->target->player && selections && P_CheckMeleeRange(actor) &&
      P_Random(pr_dwarfatk) < 128 )
   {  // Chaos

      // warp enemy to random deathmatch spot
      // added P_CheckMeleeRange to requirements to make teleportation
      // more tactically advantageous (dwarves do poorly up close)

      fixed_t prevX, prevY, prevZ;

      int i = P_Random(pr_dwarfatk)%selections;
      fixed_t destX = deathmatchstarts[i].x*FRACUNIT;
      fixed_t destY = deathmatchstarts[i].y*FRACUNIT;

      P_SetMobjState(actor, E_SafeState(S_DWARFPURPLEAURA1));
      A_FaceTarget(actor);
      S_StartSound(NULL, sfx_harp);

      mo = actor->target;

      prevX = mo->x;
      prevY = mo->y;
      prevZ = mo->z;

      if(P_TeleportMove(mo, destX, destY, false))
      {
         mobj_t *th1;
         int fogType = E_SafeThingType(MT_IFOG);

         th1 = P_SpawnMobj(prevX, prevY, prevZ+24*FRACUNIT, fogType);
         S_StartSound(th1, sfx_itmbk);
         P_SetMobjState(mo, mo->info->seestate);
         P_SpawnMobj(mo->x, mo->y, mo->z+24*FRACUNIT, fogType);
         S_StartSound(mo, sfx_itmbk);
         mo->z = mo->floorz;
         mo->momx = mo->momy = mo->momz = 0;
      }
      return;
   }

   if(actor->target && !actor->target->player && can_see_enemy &&
      P_CheckAlterEgo(alterEgoType) && P_Random(pr_dwarfatk) < 96)
   {  // Order
      // project alter-ego

      P_SetMobjState(actor, E_SafeState(S_DWARFPURPLEAURA1));
      A_FaceTarget(actor);
      S_StartSound(NULL, sfx_wofp);

      //spawn alterego
      P_SpawnDwarfAlterego(actor, alterEgoType);
   }
}

void A_DwarfFWAEMagic(mobj_t *actor)
{
  int decision, waterchance;
  mobj_t *mo;

  if(actor->special2)
  {
    actor->special2 = 0;  // LDOC spell does not need to be followed up
    return;
  }

  if(!actor->target)
    return;

  if(!P_CheckSight(actor, actor->target))
    return;

  // Friendly mobj logic is causing Halif to automatically attack the
  // player if he is in sight after killing an enemy with an LDOC spell
  // (target field is, sadly, used to "chase" the player). Halif will no
  // longer use elemental spells in retaliation, unless he happens to be
  // an evil Halif :)

  if((actor->flags & MF_FRIEND) && actor->target->player)
  {
    A_FaceTarget(actor); // grimace at the player intentionally ;)
    return;
  }

  P_SetMobjState(actor, E_SafeState(S_DWARFGREENAURA1));

  // haleyjd: restructured, rebalanced probabilites

  decision = P_Random(pr_dwarffwae);

  if(decision < 128)
  {
     int pick = P_Random(pr_dwarffwae)%2;
     switch(pick)
     {
        case 0:
                // fire -- Phoenix Fire
                A_FaceTarget(actor);
                mo = P_SpawnMissile(actor, actor->target, 
                                    E_SafeThingType(MT_PHOENIXSHOT),
                                    actor->z + 20*FRACUNIT);
                if(!mo) return;
                mo->x += mo->momx;
                mo->y += mo->momy;
                P_SetTarget(&mo->tracer, actor->target);  // killough 11/98
                break;
        case 1:
                // water
                waterchance = P_Random(pr_dwarffwae);

                // very rare move -- Permafrost

                if(waterchance < 16 &&
                   (actor->z == actor->floorz) &&  // on floor
                   (actor->subsector->sector->heightsec == -1) &&  // no deep water
                   (P_GetThingFloorType(actor) == FLOOR_WATER) &&  // water only
                   ((W_CheckNumForName)("NFICE01", ns_flats) != -1)) // ice flat found
                {
                   int fr = (0x1EB8*FROZENFRICTION)/0x80 + 0xD000;
                   int mvf = ((0x10092 - fr)*(0x70))/0x158;
                   sector_t *sec = actor->subsector->sector;
                   mobj_t *sndorg = (mobj_t *)&sec->soundorg;

                   sec->floorpic = R_FlatNumForName("NFICE01");
                   sec->friction = fr;
                   sec->movefactor = mvf;
                   sec->special |= FRICTION_MASK;

                   A_FaceTarget(actor);
                   S_StartSound(NULL, sfx_cone3);
                   S_StartSound(sndorg, sfx_icedth);
                }
                else
                {
                   // TODO: main water attack
                }
                break;
     }
  }
  else if(decision < 192)
  {
        // air -- Poison Gas
        // TODO
  }
  else
  {
        int mummyType = E_SafeThingType(MT_HALIFMUMMY);
        int skullType = E_SafeThingType(MT_MUMMYFX1);
        // earth -- Summon Golem -or- Stone Skull
        A_FaceTarget(actor);
        if(P_CheckAlterEgo(mummyType))
        {
           S_StartSound(NULL, sfx_wofp);
           P_SpawnDwarfAlterego(actor, mummyType);
        }
        else 
        {
           // cast Stone Skull if max golems
           P_SpawnMissile(actor, actor->target, skullType,
                          actor->z + 20*FRACUNIT);
        }
  
  }
}

void A_DwarfDie(mobj_t *actor)
{
   static int alterDwarfType = -1;

   // haleyjd: only look this up once
   if(alterDwarfType == -1)
      alterDwarfType = E_ThingNumForDEHNum(MT_ALTERDWARF);

   if(!actor->special1)  // first time catch on fire
   {
      P_SpawnMobj(actor->x, actor->y, actor->z, E_SafeThingType(MT_FIRE));
      actor->special1++;      
   }
   else  // second time explode!  >:)
   {
      mobj_t *explosion;
      explosion = P_SpawnMobj(actor->x, actor->y, (actor->z+24*FRACUNIT),
                              E_SafeThingType(MT_ROCKET));
      if(explosion)
      {
        P_SetMobjState(explosion, E_SafeState(S_EXPLODE1));
        S_StartSound(NULL, sfx_barexp);
        if(actor->type == alterDwarfType)
          P_RemoveMobj(actor);
      }
   }
   // P_NightmareRespawn will bring him back to life
   // in same manner as nightmare respawning
}

// Dwarf Alterego Routines -- Halif Daemonica / Summoned Golem

// 3/23/00: extended to spawn golem as well

void P_SpawnDwarfAlterego(mobj_t *actor, mobjtype_t type)
{
   fixed_t       x,y,z;
   mobj_t        *newmobj;
   angle_t       an;
   int           prestep;
   static int    halifMummyType = -1;

   if(halifMummyType == -1)
      halifMummyType = E_ThingNumForDEHNum(MT_HALIFMUMMY);
   
   // good old-fashioned pain elemental style spawning
   
   an = actor->angle >> ANGLETOFINESHIFT;
   
   prestep = 4*FRACUNIT + 3*(actor->info->radius + mobjinfo[type].radius)/2;
   
   x = actor->x + FixedMul(prestep, finecosine[an]);
   y = actor->y + FixedMul(prestep, finesine[an]);
   z = actor->z;

   // Check whether the Dwarf is being spawned through a 1-sided
   // wall or an impassible line, or a "monsters can't cross" line.
   // If it is, then we don't allow the spawn.
   
   if(Check_Sides(actor,x,y))
      return;

   newmobj = P_SpawnMobj(x, y, z, type);
   
   // Check to see if the new Dwarf's z value is above the
   // ceiling of its new sector, or below the floor. If so, kill it.

   if((newmobj->z >
      (newmobj->subsector->sector->ceilingheight - newmobj->height)) ||
      (newmobj->z < newmobj->subsector->sector->floorheight))
   {
      // kill it immediately
      P_DamageMobj(newmobj,actor,actor,10000,MOD_UNKNOWN);
      return;                                                 
   }                                                         
   
   // spawn dwarf with same friendliness
   newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

   // killough 8/29/98: add to appropriate thread
   P_UpdateThinker(&newmobj->thinker);
   
   // Check for movements.
   // killough 3/15/98: don't jump over dropoffs:

   if(!P_TryMove(newmobj, newmobj->x, newmobj->y, false))
   {
      // kill it immediately
      P_DamageMobj(newmobj, actor, actor, 10000, MOD_UNKNOWN);
      return;
   }

   P_SetTarget(&newmobj->target, actor->target);
   if(type == halifMummyType)
      newmobj->special1 = GOLEMTIME;
   else
      newmobj->special1 = DWARFALTEGOTIME; // blows up when this runs out
}

boolean P_CheckAlterEgo(mobjtype_t type)
{
   // This works just like the old limit on lost souls.
   // Note count must be init.'ed to zero for only one dwarf to be active
   // at a time -- this is contrary to the comments above
   // ((count = 20) != 20 lost souls) -- haleyjd
   
   // 8/3/00: consolidated with P_CheckGolems()

   int count = 0;
   thinker_t *currentthinker;
   static int halifMummyType = -1;

   if(halifMummyType == -1)
      halifMummyType = E_ThingNumForDEHNum(MT_HALIFMUMMY);

   for (currentthinker = thinkercap.next;
        currentthinker != &thinkercap;
        currentthinker = currentthinker->next)
   {
      mobj_t *mo = NULL;
      if(currentthinker->function == P_MobjThinker)
         mo = (mobj_t *)currentthinker;
      
      if(mo && (mo->type == type) && 
         !(type == halifMummyType && 
         mo->health <= 0))
      {
         if (--count < 0)         // killough 8/29/98: early exit
            return false;
      }
   }
   return true;
}

void A_DwarfAlterEgoChase(mobj_t *actor)
{
   if(actor->special1)
    actor->special1--;
   else
   {
     A_Die(actor);
     return;
   }
   A_Chase(actor);
}

void A_DwarfAlterEgoAttack(mobj_t *actor)
{
  mobj_t *mo;
  fixed_t z = actor->z + 20*FRACUNIT;
  static int phoenixType = -1; 
  
  if(phoenixType == -1)
     phoenixType = E_SafeThingType(MT_PHOENIXSHOT);

  // perform Halif's fire spell with boosted power -- 2x projectile
  if(!actor->target)
    return;
  A_FaceTarget(actor);
  mo = P_SpawnMissile(actor, actor->target, phoenixType, z);
  if(!mo) return;
  mo->x += mo->momx;
  mo->y += mo->momy;
  P_SetTarget(&mo->tracer, actor->target);  // killough 11/98
  mo = P_SpawnMissile(actor, actor->target, phoenixType, z);
  if(!mo) return;
  mo->x += mo->momx;
  mo->y += mo->momy;
  P_SetTarget(&mo->tracer, actor->target);  // killough 11/98
}

// used by dwarf fire spell

void A_PhoenixTracer(mobj_t *actor)
{
  angle_t       exact;
  fixed_t       dist;
  fixed_t       slope;
  mobj_t        *dest;
  mobj_t        *puff;
  static int    puffType = -1;
  
  if(puffType == -1)
     puffType = E_SafeThingType(MT_PHOENIXPUFF);

  // spawn a puff of smoke behind the phoenix fire
  puff = P_SpawnMobj(actor->x, actor->y, actor->z, puffType); 
  if(puff)
    puff->momz = 2*FRACUNIT;
  
  // adjust direction
  dest = actor->tracer;

  if (!dest || dest->health <= 0)
    return;

  // change angle
  exact = R_PointToAngle2(actor->x, actor->y, dest->x, dest->y);

  if (exact != actor->angle)
  {
    if (exact - actor->angle > 0x80000000)
      {
        actor->angle -= TRACEANGLE;
        if (exact - actor->angle < 0x80000000)
          actor->angle = exact;
      }
    else
      {
        actor->angle += TRACEANGLE;
        if (exact - actor->angle > 0x80000000)
          actor->angle = exact;
      }
  }
  exact = actor->angle>>ANGLETOFINESHIFT;
  actor->momx = FixedMul(actor->info->speed, finecosine[exact]);
  actor->momy = FixedMul(actor->info->speed, finesine[exact]);

  // change slope
  dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);

  dist = dist / actor->info->speed;

  if (dist < 1)
    dist = 1;

  slope = (dest->z+20*FRACUNIT - actor->z) / dist;

  if (slope < actor->momz)
    actor->momz -= FRACUNIT/8;
  else
    actor->momz += FRACUNIT/8;
}

//==============================
//
// Cyberdemon Guard functions
//
//==============================
void A_CyberGuardSigh(mobj_t *actor)
{
   // make a little sound occasionally for ambience
   // only used by inert cyberguard

   if(P_Random(pr_see)<2)   // 10/17/99: make less frequent than activesound
     S_StartSound(actor, sfx_dmact);
}

void A_CyberGuardWake(mobj_t *actor)
{
   mobj_t *mo;
   angle_t angle;

   // disable invulnerability set from spawning
   actor->flags2 &= ~MF2_INVULNERABLE;

   // set actor information to that for the Cyberdemon
   //  prevents restoration of battle axe after waking the first time

   // haleyjd 6/24/00: not save-game safe, need to find a better method :(
   // haleyjd 8/13/00: i don't care - weird things happen if this isn't
   //                  done :)
   // haleyjd 11/13/00: it works now :P

   actor->info = &mobjinfo[E_SafeThingType(MT_CYBORG)];

   // spawn broken staff and axe objects
   mo = P_SpawnMobj(actor->x, actor->y, actor->z+65*FRACUNIT, E_SafeThingType(MT_BROKENAXE));
   if(mo)
   {
      angle = actor->angle + ANG90;
      mo->momz = 8*FRACUNIT + (P_Random(pr_cybdrop)<<10);
      mo->momx = FixedMul(((P_Random(pr_cybdrop)-128)<<11) + FRACUNIT,
                          finecosine[angle>>ANGLETOFINESHIFT]);
      mo->momy = FixedMul(((P_Random(pr_cybdrop)-128)<<11) + FRACUNIT, 
                          finesine[angle>>ANGLETOFINESHIFT]);
      P_SetTarget(&mo->target, actor);
   }
   mo = P_SpawnMobj(actor->x, actor->y, actor->z+45*FRACUNIT, E_SafeThingType(MT_BROKENSTAFF));
   if(mo)
   {
      angle = actor->angle - ANG90;
      mo->momz = 8*FRACUNIT + (P_Random(pr_cybdrop)<<10);
      mo->momx = FixedMul(((P_Random(pr_cybdrop)-128)<<11) + FRACUNIT,
                          finecosine[angle>>ANGLETOFINESHIFT]);
      mo->momy = FixedMul(((P_Random(pr_cybdrop)-128)<<11) + FRACUNIT, 
                          finesine[angle>>ANGLETOFINESHIFT]);
      P_SetTarget(&mo->target, actor);
   }
   // fall through to cyberdemon walking frames
}

void A_AxePieceFall(mobj_t *actor)
{
   static int axeType = -1, staffType = -1;

   if(axeType == -1)
   {
      axeType   = E_ThingNumForDEHNum(MT_BROKENAXE);
      staffType = E_ThingNumForDEHNum(MT_BROKENSTAFF);
   }

   if(actor->z <= actor->floorz && actor->momz == 0)
   {
      if(actor->type == axeType)
      {
         actor->momx = actor->momy = 0;     // embeds into floor :)
         P_SetMobjState(actor, E_SafeState(S_AXEFALL4));
      }
      else if(actor->type == staffType)
      {
         P_SetMobjState(actor, E_SafeState(S_STAFF_FALL3));
      }
   }
}

void A_Sparkle(mobj_t *actor)
{
   if(!actor->special2)
   {
      actor->special1 = 5;
      actor->special2 = 1;
      return;
   }
   actor->special1--;     // sparkle duration count-down
   if(!actor->special1)
   {
      P_SetMobjState(actor, E_NullState());
   }
}

//==============================
// Console Commands for Things
//==============================

CONSOLE_COMMAND(spawn, cf_notnet|cf_level|cf_hidden)
{
   fixed_t       x,y,z;
   mobj_t        *newmobj;
   angle_t       an;
   int           type, prestep;
   static int    teleManType = -1;
   static int    fountainType = -1;
   static int    playerType = -1;

   player_t *plyr = &players[consoleplayer];

   if(teleManType == -1)
   {
      teleManType  = E_ThingNumForDEHNum(MT_TELEPORTMAN);
      fountainType = E_ThingNumForDEHNum(MT_FOUNTAIN);
      playerType   = E_ThingNumForDEHNum(MT_PLAYER);
   }

   if(!c_argc)
   {
      return;
   }
   if(c_argc >= 1)
   {
      type = atoi(c_argv[0]);
      if(type < 0 || type >= NUMMOBJTYPES)
      {
         C_Printf(FC_ERROR "Invalid mobj type\n");
         return;
      }
      // weed out always-disallowed types
      if(type == teleManType)
      {
         C_Printf(FC_ERROR "Disallowed spawn\n");
         return;
      }
            
      if(plyr->health <= 0)
        return;

      // if its a missile, shoot it
      if(mobjinfo[type].flags & MF_MISSILE)
      {
         P_SpawnPlayerMissile(plyr->mo, type);
         return;
      }

      an = plyr->mo->angle >> ANGLETOFINESHIFT;
      prestep = 4*FRACUNIT + 3*(plyr->mo->info->radius + mobjinfo[type].radius)/2;

      x = plyr->mo->x + FixedMul(prestep, finecosine[an]);
      y = plyr->mo->y + FixedMul(prestep, finesine[an]);

      z = (mobjinfo[type].flags & MF_SPAWNCEILING) ? ONCEILINGZ : ONFLOORZ;

      if(Check_Sides(plyr->mo, x, y))
        return;

      newmobj = P_SpawnMobj(x, y, z, type);

      if(!newmobj)
        return;

      if(c_argc >= 2)
      {
         int arg2 = atoi(c_argv[1]);
         
         if(type == fountainType)
         {
            if(arg2 < 9027) arg2 = 9027;
            if(arg2 > 9033) arg2 = 9033;

            newmobj->effects |= (arg2 - 9026) << FX_FOUNTAINSHIFT;
         }
         else
         {
            if(arg2)
               newmobj->flags |= MF_FRIEND;
         }
      }

      // killough 8/29/98: add to appropriate thread
      P_UpdateThinker(&newmobj->thinker);

      // not such a great idea, makes it unkillable by cheat code
      // if(newmobj->flags & MF_COUNTKILL)
      //  newmobj->flags &= ~MF_COUNTKILL;
      
      // this, however, is fine
      if(newmobj->flags & MF_COUNTITEM)
        newmobj->flags &= ~MF_COUNTITEM;

      if(newmobj->type == playerType)
      {
         // 06/09/02: set player field for voodoo dolls
         newmobj->player = plyr;
      }
   }
}

// haleyjd 07/05/03: new console commands that can use
// EDF thing type names instead of internal type numbers

CONSOLE_COMMAND(summon, cf_notnet|cf_level|cf_hidden)
{
   int num;
   char buffer[64];

   if(!c_argc)
      return;

   num = E_ThingNumForName(c_argv[0]);

   if(num == NUMMOBJTYPES)
   {
      C_Printf("unknown thing type\n");
      return;
   }

   switch(c_argc)
   {
   default:
   case 1:
      psnprintf(buffer, sizeof(buffer), "spawn %d", num);
      break;
   case 2:
      psnprintf(buffer, sizeof(buffer), "spawn %d %s", num, c_argv[1]);
      break;
   }
   C_RunTextCmd(buffer);
}

CONSOLE_COMMAND(give, cf_notnet|cf_level)
{
   int i;
   int itemnum;
   int thingnum;
   player_t *plyr = &players[consoleplayer];

   if(!c_argc)
      return;
   
   thingnum = E_ThingNumForName(c_argv[0]);
   if(thingnum == NUMMOBJTYPES)
   {
      C_Printf("unknown thing type\n");
      return;
   }
   if(!(mobjinfo[thingnum].flags & MF_SPECIAL))
   {
      C_Printf("thing type is not a special\n");
      return;
   }
   itemnum = (c_argc >= 2) ? atoi(c_argv[1]) : 1;

   for(i = 0; i < itemnum; i++)
   {
      mobj_t *mo = P_SpawnMobj(plyr->mo->x, plyr->mo->y, plyr->mo->z,
                               thingnum);
      mo->flags &= ~(MF_SPECIAL|MF_COUNTITEM);

      P_TouchSpecialThing(mo, plyr->mo);

      // if it wasn't picked up, remove it
      if(mo->thinker.function == P_MobjThinker)
         P_RemoveMobj(mo);
   }
}

//
// whistle
//
// This command lets the player "whistle" to teleport a friend
// of the given type to his location.
//
CONSOLE_COMMAND(whistle, cf_notnet|cf_level)
{
   int thingnum;
   player_t *plyr = &players[consoleplayer];

   if(!c_argc)
      return;
   
   thingnum = E_ThingNumForName(c_argv[0]);
   if(thingnum == NUMMOBJTYPES)
   {
      C_Printf("unknown thing type\n");
      return;
   }

   P_Whistle(plyr->mo, thingnum);
}

void PE_AddCommands(void)
{
   C_AddCommand(spawn);
   C_AddCommand(summon);
   C_AddCommand(give);
   C_AddCommand(whistle);
}

//----------------------------------------------------------------------------
//
// $Log: p_enemy.c,v $
// Revision 1.22  1998/05/12  12:47:10  phares
// Removed OVER_UNDER code
//
// Revision 1.21  1998/05/07  00:50:55  killough
// beautification, remove dependence on evaluation order
//
// Revision 1.20  1998/05/03  22:28:02  killough
// beautification, move declarations and includes around
//
// Revision 1.19  1998/04/01  12:58:44  killough
// Disable boss brain if no targets
//
// Revision 1.18  1998/03/28  17:57:05  killough
// Fix boss spawn savegame bug
//
// Revision 1.17  1998/03/23  15:18:03  phares
// Repaired AV ghosts stuck together bug
//
// Revision 1.16  1998/03/16  12:33:12  killough
// Use new P_TryMove()
//
// Revision 1.15  1998/03/09  07:17:58  killough
// Fix revenant tracer bug
//
// Revision 1.14  1998/03/02  11:40:52  killough
// Use separate monsters_remember flag instead of bitmask
//
// Revision 1.13  1998/02/24  08:46:12  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.12  1998/02/23  04:43:44  killough
// Add revenant p_atracer, optioned monster ai_vengence
//
// Revision 1.11  1998/02/17  06:04:55  killough
// Change RNG calling sequences
// Fix minor icon landing bug
// Use lastenemy to make monsters remember former targets, and fix player look
//
// Revision 1.10  1998/02/09  03:05:22  killough
// Remove icon landing limit
//
// Revision 1.9  1998/02/05  12:15:39  phares
// tighten lost soul wall fix to compatibility
//
// Revision 1.8  1998/02/02  13:42:54  killough
// Relax lost soul wall fix to demo_compatibility
//
// Revision 1.7  1998/01/28  13:21:01  phares
// corrected Option3 in AV bug
//
// Revision 1.6  1998/01/28  12:22:17  phares
// AV bug fix and Lost Soul trajectory bug fix
//
// Revision 1.5  1998/01/26  19:24:00  phares
// First rev with no ^Ms
//
// Revision 1.4  1998/01/23  14:51:51  phares
// No content change. Put ^Ms back.
//
// Revision 1.3  1998/01/23  14:42:14  phares
// No content change. Removed ^Ms for experimental checkin.
//
// Revision 1.2  1998/01/19  14:45:01  rand
// Temporary line for checking checkins
//
// Revision 1.1.1.1  1998/01/19  14:02:59  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
