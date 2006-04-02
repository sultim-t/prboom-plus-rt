// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
//
// Parameterized and Heretic-inspired action functions
//
// DISCLAIMER: None of this code was taken from Heretic. Any
// resemblence is purely coincidental or is the result of work from
// a common base source.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "doomstat.h"
#include "info.h"
#include "p_mobj.h"
#include "m_random.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_inter.h"
#include "p_tick.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_enemy.h"
#include "p_map.h"
#include "p_spec.h"
#include "p_info.h"
#include "e_states.h"
#include "e_things.h"
#include "e_sound.h"
#include "e_string.h"
#include "e_ttypes.h"
#include "hu_stuff.h"
#include "c_io.h"

// external prototypes from p_enemy.c

void A_FaceTarget(mobj_t *actor);
void A_Chase(mobj_t *actor);
void A_Fall(mobj_t *actor);
void A_Pain(mobj_t *actor);
void A_Die(mobj_t *actor);

//
// A_SpawnGlitter
//
// Parameterized code pointer to spawn inert objects with some
// positive z momentum
//
// Parameters:
// args[0] - object type (use DeHackEd number)
// args[1] - z momentum (scaled by FRACUNIT)
// args[2] - mode select:
//  * 0 = spawn on floor, use z momentum
//  * 1 = spawn on floor, no z momentum
//  * 2 = random z coord, use z momentum
//  * 3 = random z coord, no z momentum
// args[3] - alternate radius for effect
//
void A_SpawnGlitter(mobj_t *actor)
{
   mobj_t *glitter;
   int glitterType, mode;
   fixed_t initMomentum;
   fixed_t x, y, z;
   int radius;

   glitterType  = (int)(actor->state->args[0]);
   initMomentum = (fixed_t)(actor->state->args[1]);
   mode         = (int)(actor->state->args[2]);
   radius       = (int)(actor->state->args[3]);

   // special defaults

   // momentum of zero == 1/2 unit per tic (use mode 1 to avoid)
   if(!initMomentum)
      initMomentum = FRACUNIT >> 2;

   // radius of zero == radius of 32 (zero is useless)
   if(!radius)
      radius = 32;

   // haleyjd 07/05/03: adjusted for EDF
   glitterType = E_SafeThingType(glitterType);

   // randomize spawning coordinates within a radius-unit square
   x = actor->x + ((P_Random(pr_tglit) % radius) - (radius/2)) * FRACUNIT;
   y = actor->y + ((P_Random(pr_tglit) % radius) - (radius/2)) * FRACUNIT;

   // haleyjd 07/14/04: use this to absorb P_ClericSparkle effect
   if(mode == 2 || mode == 3)
   {
      int h = P_ThingInfoHeight(actor->info);
      z = actor->z + h/2 + (P_SubRandom(pr_tglitz) << 15);
      
      // cap to within actor's height range
      if(z > actor->z + h)
        z = actor->z + h;
      else if(z < actor->floorz)
        z = actor->floorz;
   }
   else
      z = actor->floorz;

   glitter = P_SpawnMobj(x, y, z, glitterType);

   // give it some upward momentum
   if(mode == 0 || mode == 2)
      glitter->momz = initMomentum;
}

//
// A_AccelGlitter
//
// Increases object's z momentum by 50%
//
void A_AccelGlitter(mobj_t *actor)
{
   actor->momz += actor->momz / 2;
}

//
// A_SpawnAbove
//
// Parameterized pointer to spawn a solid object above the one
// calling the pointer. Used by the key gizmos.
//
// args[0] -- thing type (DeHackEd num)
// args[1] -- state number (< 0 == no state transition)
// args[2] -- amount to add to z coordinate
//
void A_SpawnAbove(mobj_t *actor)
{
   int thingtype;
   int statenum;
   fixed_t zamt;
   mobj_t *mo;

   thingtype = (int)(actor->state->args[0]);
   statenum  = (int)(actor->state->args[1]);
   zamt      = (actor->state->args[2]) * FRACUNIT;

   // haleyjd 07/05/03: adjusted for EDF
   thingtype = E_SafeThingType(thingtype);

   mo = P_SpawnMobj(actor->x, actor->y, actor->z + zamt, thingtype);

   if(statenum >= 0)
   {
      statenum = E_SafeState(statenum);
      P_SetMobjState(mo, statenum);
   }
}

//
// Heretic Mummy
//

void A_MummyAttack(mobj_t *actor)
{
   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_mumpunch)&7)+1)*2;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      S_StartSound(actor, sfx_mumat2);
      return;
   }

   S_StartSound(actor, sfx_mumat1);
}

void A_MummyAttack2(mobj_t *actor)
{
   mobj_t *mo;
   
   if(!actor->target)
      return;
   
   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_mumpunch2)&7)+1)*2;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      return;
   }
   
   mo = P_SpawnMissile(actor, actor->target, 
                       E_SafeThingType(MT_MUMMYFX1),
                       actor->z + DEFAULTMISSILEZ);

   P_SetTarget(&mo->tracer, actor->target);
}

void A_MummySoul(mobj_t *actor)
{
   mobj_t *mo;
   static int soulType = -1;
   
   if(soulType == -1)
      soulType = E_SafeThingType(MT_MUMMYSOUL);
   
   mo = P_SpawnMobj(actor->x, actor->y, actor->z+10*FRACUNIT, soulType);
   mo->momz = FRACUNIT;
}

void P_HticDrop(mobj_t *actor, int special, mobjtype_t type)
{
   mobj_t *item;

   item = P_SpawnMobj(actor->x, actor->y, 
                      actor->z + (actor->height >> 1),
                      type);
   
   item->momx = P_SubRandom(pr_hdropmom) << 8;
   item->momy = P_SubRandom(pr_hdropmom) << 8;
   item->momz = (P_Random(pr_hdropmom) << 10) + 5*FRACUNIT;

   if(special)
   {
      item->health = special;
   }

   item->flags |= MF_DROPPED;
}

//
// A_HticDrop
//
// HTIC_FIXME / HTIC_TODO: Just get rid of this and make item
// drops a sole property of thing types, where it belongs.
//
// Parameterized code pointer, drops one or two items at random
//
// args[0] -- thing type 1 (0 == none)
// args[1] -- thing type 1 drop chance
// args[2] -- thing type 2 (0 == none)
// args[3] -- thing type 2 drop chance
// args[4] -- modify ammo drop amounts for HTICAMMO flag things
//            (type2<<16|type1)
//
void A_HticDrop(mobj_t *actor)
{
   int thingtype1, thingtype2, chance1, chance2;
   long dt;
   int drop1 = 0, drop2 = 0;

   thingtype1 = (int)(actor->state->args[0]);
   chance1    = (int)(actor->state->args[1]);
   thingtype2 = (int)(actor->state->args[2]);
   chance2    = (int)(actor->state->args[3]);

   // this is hackish, but it'll work
   if((dt = actor->state->args[4]))
   {
      drop1 = (int)(dt & 0x00007fff);
      drop2 = (int)((dt & 0x7fff0000) >> 16);
   }

   A_Fall(actor);

   // haleyjd 07/05/03: adjusted for EDF
   if(thingtype1)
   {
      thingtype1 = E_SafeThingType(thingtype1);
      
      if(P_Random(pr_hdrop1) <= chance1)
         P_HticDrop(actor, drop1, thingtype1);
   }

   if(thingtype2)
   {
      thingtype2 = E_SafeThingType(thingtype2);

      if(P_Random(pr_hdrop2) <= chance2)
         P_HticDrop(actor, drop2, thingtype2);
   }
}

#define TRACEANGLE 0xc000000

//
// A_GenTracer
//
// Generic homing missile maintenance
//
void A_GenTracer(mobj_t *actor)
{
   angle_t       exact;
   fixed_t       dist;
   fixed_t       slope;
   mobj_t        *dest;
  
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

void P_HticTracer(mobj_t *actor, angle_t threshold, angle_t maxturn)
{
   angle_t exact, diff;
   fixed_t dist;
   mobj_t  *dest;
   boolean dir;
  
   // adjust direction
   dest = actor->tracer;
   
   if(!dest || dest->health <= 0)
      return;

   exact = R_PointToAngle2(actor->x, actor->y, dest->x, dest->y);

   if(exact > actor->angle)
   {
      diff = exact - actor->angle;
      dir = true;
   }
   else
   {
      diff = actor->angle - exact;
      dir = false;
   }

   // if > 180, invert angle and direction
   if(diff > 0x80000000)
   {
      diff = 0xFFFFFFFF - diff;
      dir = !dir;
   }

   // apply limiting parameters
   if(diff > threshold)
   {
      diff >>= 1;
      if(diff > maxturn)
         diff = maxturn;
   }

   // turn clockwise or counterclockwise
   if(dir)
      actor->angle += diff;
   else
      actor->angle -= diff;

   // directly from above
   diff = actor->angle>>ANGLETOFINESHIFT;
   actor->momx = FixedMul(actor->info->speed, finecosine[diff]);
   actor->momy = FixedMul(actor->info->speed, finesine[diff]);

   // adjust z only when significant height difference exists
   if(actor->z+actor->height < dest->z ||
      dest->z+dest->height < actor->z)
   {
      // directly from above
      dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
      
      dist = dist / actor->info->speed;
      
      if(dist < 1)
         dist = 1;

      // momentum is set to equal slope rather than having some
      // added to it
      actor->momz = (dest->z - actor->z) / dist;
   }
}

//
// A_HticTracer
//
// Parameterized pointer for Heretic-style tracers. I wanted
// to merge this with the function above, but the logic looks
// incompatible no matter how I rewrite it.
//
// args[0]: threshold in degrees
// args[1]: maxturn in degrees
//
void A_HticTracer(mobj_t *actor)
{
   angle_t threshold, maxturn;

   threshold = (angle_t)(actor->state->args[0]);
   maxturn   = (angle_t)(actor->state->args[1]);

   // convert from integer angle to angle_t
   threshold = (angle_t)(((ULong64)threshold << 32) / 360);
   maxturn   = (angle_t)(((ULong64)maxturn << 32) / 360);

   P_HticTracer(actor, threshold, maxturn);
}

//
// A_ClinkAttack
//
// Sabreclaw's melee attack
//
void A_ClinkAttack(mobj_t *actor)
{
   int dmg;

   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);

   if(P_CheckMeleeRange(actor))
   {
      dmg = (P_Random(pr_clinkatk) % 7) + 3;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
   }
}

//
// Disciple Actions
//

void A_WizardAtk1(mobj_t *actor)
{
   A_FaceTarget(actor);
   actor->flags3 &= ~MF3_GHOST;
}

void A_WizardAtk2(mobj_t *actor)
{
   A_FaceTarget(actor);
   actor->flags3 |= MF3_GHOST;
}

void A_WizardAtk3(mobj_t *actor)
{
   mobj_t *mo;
   angle_t angle;
   fixed_t momz;
   fixed_t z = actor->z + DEFAULTMISSILEZ;
   static int wizfxType = -1;
   
   if(wizfxType == -1)
      wizfxType = E_SafeThingType(MT_WIZFX1);
   
   actor->flags3 &= ~MF3_GHOST;
   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_wizatk) & 7) + 1) * 4;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      return;
   }

   mo = P_SpawnMissile(actor, actor->target, wizfxType, z);
   momz = mo->momz;
   angle = mo->angle;
   P_SpawnMissileAngle(actor, wizfxType, angle-(ANG45/8), momz, z);
   P_SpawnMissileAngle(actor, wizfxType, angle+(ANG45/8), momz, z);
}

//
// Serpent Rider D'Sparil Actions
//

void A_Sor1Chase(mobj_t *actor)
{
   if(actor->special1)
   {
      // decrement fast walk timer
      actor->special1--;
      actor->tics -= 3;
      // don't make tics less than 1
      if(actor->tics < 1)
         actor->tics = 1;
   }

   A_Chase(actor);
}

void A_Sor1Pain(mobj_t *actor)
{
   actor->special1 = 20; // Number of steps to walk fast
   A_Pain(actor);
}

void A_Srcr1Attack(mobj_t *actor)
{
   mobj_t *mo;
   fixed_t momz;
   angle_t angle;
   fixed_t mheight = actor->z + 48*FRACUNIT;
   static int srcrfxType = -1;
   
   if(srcrfxType == -1)
      srcrfxType = E_SafeThingType(MT_SRCRFX1);

   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   // bite attack
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, 
                   ((P_Random(pr_sorc1atk)&7)+1)*8, MOD_HIT);
      return;
   }

   if(actor->health > (actor->info->spawnhealth*2)/3)
   {
      // regular attack, one fire ball
      P_SpawnMissile(actor, actor->target, srcrfxType, mheight);
   }
   else
   {
      // "limit break": 3 fire balls
      mo = P_SpawnMissile(actor, actor->target, srcrfxType, mheight);
      momz = mo->momz;
      angle = mo->angle;
      P_SpawnMissileAngle(actor, srcrfxType, angle-ANGLE_1*3, 
                          momz, mheight);
      P_SpawnMissileAngle(actor, srcrfxType, angle+ANGLE_1*3,
                          momz, mheight);
      
      // desperation -- attack twice
      if(actor->health*3 < actor->info->spawnhealth)
      {
         // don't bite the fast walk counter here
         if(actor->special2)
         {
            actor->special2 = 0;
         }
         else
         { 
            actor->special2 = 1;
            P_SetMobjState(actor, E_SafeState(S_SRCR1_ATK4));
         }
      }
   }
}

//
// A_SorcererRise
//
// Spawns the normal D'Sparil after the Chaos Serpent dies.
//
void A_SorcererRise(mobj_t *actor)
{
   mobj_t *mo;
   static int sorc2Type = -1;
   
   if(sorc2Type == -1)
      sorc2Type = E_SafeThingType(MT_SORCERER2);
   
   actor->flags &= ~MF_SOLID;
   mo = P_SpawnMobj(actor->x, actor->y, actor->z, sorc2Type);
   mo->angle = actor->angle;

   // transfer friendliness
   mo->flags = (mo->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

   // add to appropriate thread
   P_UpdateThinker(&mo->thinker);
   
   if(actor->target && !(mo->flags & MF_FRIEND))
      P_SetTarget(&mo->target, actor->target);
   
   P_SetMobjState(mo, E_SafeState(S_SOR2_RISE1));
}

//
// Normal D'Sparil Actions
//

// haleyjd 11/19/02:
// Teleport spots for D'Sparil -- these are more or less identical
// to boss spots used for DOOM II MAP30, and I have generalized
// the code that looks for them so it can be used here (and elsewhere
// as needed). This automatically removes the Heretic boss spot
// limit, of course.

MobjCollection sorcspots;

void P_SpawnSorcSpots(void)
{
   static int spotType = -1;
   
   if(spotType == -1)
      spotType = E_ThingNumForDEHNum(MT_DSPARILSPOT);

   P_ReInitMobjCollection(&sorcspots, spotType);

   if(spotType == NUMMOBJTYPES)
      return;

   P_CollectThings(&sorcspots);
}

//
// P_BossTeleport
//
// haleyjd 11/19/02
//
// Generalized function for teleporting a boss (or any other thing)
// around at random between a provided set of map spots, along
// with special effects on demand.
// Currently used by D'Sparil and Eternity Leader Cleric.
//
void P_BossTeleport(bossteleport_t *bt)
{
   mobj_t *boss, *mo, *targ;
   fixed_t prevx, prevy, prevz;

   if(P_CollectionIsEmpty(bt->mc))
      return;

   targ = P_CollectionGetRandom(bt->mc, bt->rngNum);

   boss = bt->boss;

   prevx = boss->x;
   prevy = boss->y;
   prevz = boss->z;

   if(P_TeleportMove(boss, targ->x, targ->y, false))
   {
      if(bt->hereThere <= BOSSTELE_BOTH &&
         bt->hereThere != BOSSTELE_NONE)
      {
         mo = P_SpawnMobj(prevx, prevy, prevz + bt->zpamt, bt->fxtype);
         S_StartSound(mo, bt->soundNum);
      }

      if(bt->state >= 0)
         P_SetMobjState(boss, bt->state);
      S_StartSound(boss, bt->soundNum);

      if(bt->hereThere >= BOSSTELE_BOTH &&
         bt->hereThere != BOSSTELE_NONE)
         P_SpawnMobj(boss->x, boss->y, boss->z + bt->zpamt, bt->fxtype);

      boss->z = boss->floorz;
      boss->angle = targ->angle;
      boss->momx = boss->momy = boss->momz = 0;
   }
}

//
// P_SorcTeleportProb
//
// haleyjd 11/19/02: a function to calculate the probability
// ramp for D'Sparil's teleportation. Raven used a simple eight-
// level lookup array. Of course, I cannot use their code, so
// I've come up with an interesting formula that rather closely
// matches their output, and is actually superior in terms of
// smoothness of the probability curve.
//
static int P_SorcTeleportProb(mobj_t *actor)
{
   float pct;

   if(actor->info->spawnhealth == 0)
      return 0;

   pct = (float)(actor->health) / (float)(actor->info->spawnhealth);

   if(pct == 1.0f)
   {
      return 0;
   }
   else if(pct > 0.499f)
   {
      int chance = (int)(128.0f * (1.0f - pct));

      return chance >= 16 ? chance : 16;
   }
   else if(pct > 0.125f)
   {
      return (int)(128.0f * (1.0f - pct)) + 40;
   }
   else
   {
      return 192;
   }
}

void A_Srcr2Decide(mobj_t *actor)
{
   if(P_CollectionIsEmpty(&sorcspots))
      return;

   if(P_Random(pr_sorctele1) < P_SorcTeleportProb(actor))
   {
      bossteleport_t bt;

      bt.mc        = &sorcspots;      // use sorcspots collection
      bt.rngNum    = pr_sorctele2;    // use this rng
      bt.boss      = actor;           // teleport D'Sparil
      bt.state     = E_SafeState(S_SOR2_TELE1); // set him to this state
      bt.fxtype    = E_SafeThingType(MT_SOR2TELEFADE); // spawn a DSparil TeleFade
      bt.zpamt     = 0;               // add 0 to fx z coordinate
      bt.hereThere = BOSSTELE_ORIG;   // spawn fx only at origin
      bt.soundNum  = sfx_htelept;     // use heretic teleport sound

      P_BossTeleport(&bt);
   }
}

void A_Srcr2Attack(mobj_t *actor)
{
   int chance;
   fixed_t z = actor->z + DEFAULTMISSILEZ;
   static int sor2fx1Type = -1;
   static int sor2fx2Type = -1;

   if(sor2fx1Type == -1)
   {
      sor2fx1Type = E_SafeThingType(MT_SOR2FX1);
      sor2fx2Type = E_SafeThingType(MT_SOR2FX2);
   }
   
   if(!actor->target)
      return;
   
   S_StartSound(NULL, actor->info->attacksound);

   if(P_CheckMeleeRange(actor))
   {
      // ouch!
      int dmg = ((P_Random(pr_soratk1) & 7) + 1) * 20;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      return;
   }

   chance = (actor->health * 2 < actor->info->spawnhealth) ? 96 : 48;

   if(P_Random(pr_soratk2) < chance)
   {
      mobj_t *mo;

      // spawn wizards -- transfer friendliness
      mo = P_SpawnMissileAngle(actor, sor2fx2Type, 
                               actor->angle - ANG45, 
                               FRACUNIT >> 1, z);
      mo->flags = (mo->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);
      
      mo = P_SpawnMissileAngle(actor, sor2fx2Type,
                               actor->angle + ANG45, 
                               FRACUNIT >> 1, z);
      mo->flags = (mo->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);
   }
   else
   {
      // shoot blue bolt
      P_SpawnMissile(actor, actor->target, sor2fx1Type, z);
   }
}

void A_BlueSpark(mobj_t *actor)
{
   int i;
   mobj_t *mo;
   static int sparkType = -1;
   
   if(sparkType == -1)
      sparkType = E_SafeThingType(MT_SOR2FXSPARK);
   
   for(i = 0; i < 2; ++i)
   {
      mo = P_SpawnMobj(actor->x, actor->y, actor->z, sparkType);
      
      mo->momx = P_SubRandom(pr_bluespark) << 9;
      mo->momy = P_SubRandom(pr_bluespark) << 9;      
      mo->momz = FRACUNIT + (P_Random(pr_bluespark) << 8);
   }
}

void A_GenWizard(mobj_t *actor)
{
   mobj_t *mo;
   mobj_t *fog;
   static int wizType = -1;
   static int fogType = -1;

   if(wizType == -1)
   {
      wizType = E_SafeThingType(MT_WIZARD);
      fogType = E_SafeThingType(MT_HTFOG);
   }
   
   mo = P_SpawnMobj(actor->x, actor->y, 
                    actor->z-mobjinfo[wizType].height/2, 
                    wizType);

   if(!P_CheckPosition(mo, mo->x, mo->y) ||
      (mo->z >
      (mo->subsector->sector->ceilingheight - mo->height)) ||
      (mo->z < mo->subsector->sector->floorheight))
   {
      // doesn't fit, so remove it immediately
      P_RemoveMobj(mo);
      return;
   }

   mo->flags = (mo->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

   // add to appropriate thread
   P_UpdateThinker(&mo->thinker);

   // Check for movements.
   if(!P_TryMove(mo, mo->x, mo->y, false))
   {
      P_RemoveMobj(mo);
      return;
   }

   // set this missile object to die
   actor->momx = actor->momy = actor->momz = 0;
   P_SetMobjState(actor, mobjinfo[actor->type].deathstate);
   actor->flags &= ~MF_MISSILE;
   
   // spawn a telefog
   fog = P_SpawnMobj(actor->x, actor->y, actor->z, fogType);
   S_StartSound(fog, sfx_htelept);
}

void A_Sor2DthInit(mobj_t *actor)
{
   actor->special1 = 7; // Animation loop counter

   // kill monsters early
   // kill only friends or enemies depending on friendliness
   P_Massacre((actor->flags & MF_FRIEND) ? 1 : 2);
}

void A_Sor2DthLoop(mobj_t *actor)
{
   if(--actor->special1)
   { 
      // Need to loop
      P_SetMobjState(actor, E_SafeState(S_SOR2_DIE4));
   }
}

//
// A_HticExplode
//
// Parameterized pointer, enables several different Heretic
// explosion actions
//
void A_HticExplode(mobj_t *actor)
{
   int damage;

   int action = (int)(actor->state->args[0]);

   switch(action)
   {
   case 1: // 1 -- D'Sparil FX1 explosion, random damage
      damage = 80 + (P_Random(pr_sorfx1xpl) & 31);
      break;
   case 2: // 2 -- Maulotaur floor fire, constant 24 damage
      damage = 24;
      break;
   case 3: // 3 -- Time Bomb of the Ancients, special effects
      actor->z += 32*FRACUNIT;
      actor->translucency = FRACUNIT;
      // fall through
   default:
      damage = 128;
      break;
   }

   P_RadiusAttack(actor, actor->target, damage, actor->info->mod);

   if(actor->z <= actor->secfloorz + damage*FRACUNIT)
      E_HitWater(actor, actor->subsector->sector);
}

typedef struct
{
   unsigned long thing_flag;
   unsigned long level_flag;
   int flagfield;
} boss_spec_htic_t;

#define NUM_HBOSS_SPECS 5

static boss_spec_htic_t hboss_specs[NUM_HBOSS_SPECS] =
{
   { MF2_E1M8BOSS,   BSPEC_E1M8, 2 },
   { MF2_E2M8BOSS,   BSPEC_E2M8, 2 },
   { MF2_E3M8BOSS,   BSPEC_E3M8, 2 },
   { MF2_E4M8BOSS,   BSPEC_E4M8, 2 },
   { MF3_E5M8BOSS,   BSPEC_E5M8, 3 },
};

//
// A_HticBossDeath
//
// Heretic boss deaths
//
void A_HticBossDeath(mobj_t *actor)
{
   thinker_t *th;
   line_t    junk;
   int       i;

   for(i = 0; i < NUM_HBOSS_SPECS; ++i)
   {
      unsigned long flags = 
         hboss_specs[i].flagfield == 2 ? actor->flags2 : actor->flags3;
      
      // to activate a special, the thing must be a boss that triggers
      // it, and the map must have the special enabled.
      if((flags & hboss_specs[i].thing_flag) &&
         (LevelInfo.bossSpecs & hboss_specs[i].level_flag))
      {
         for(th = thinkercap.next; th != &thinkercap; th = th->next)
         {
            if(th->function == P_MobjThinker)
            {
               mobj_t *mo = (mobj_t *)th;
               unsigned long moflags =
                  hboss_specs[i].flagfield == 2 ? mo->flags2 : mo->flags3;
               if(mo != actor && 
                  (moflags & hboss_specs[i].thing_flag) && 
                  mo->health > 0)
                  return;         // other boss not dead
            }
         }

         // victory!
         switch(hboss_specs[i].level_flag)
         {
         default:
         case BSPEC_E2M8:
         case BSPEC_E3M8:
         case BSPEC_E4M8:
         case BSPEC_E5M8:
            // if a friendly boss dies, kill only friends
            // if an enemy boss dies, kill only enemies
            P_Massacre((actor->flags & MF_FRIEND) ? 1 : 2);
            
            // fall through
         case BSPEC_E1M8:
            junk.tag = 666;
            EV_DoFloor(&junk, lowerFloor);
            break;
         } // end switch
      } // end if
   } // end for
}

//
// Pods and Pod Generators
//

void A_PodPain(mobj_t *actor)
{
   int i;
   int count;
   int chance;
   mobj_t *goo;
   static int gooType = -1;
   
   if(gooType == -1)
      gooType = E_SafeThingType(MT_PODGOO);
   
   chance = P_Random(pr_podpain);

   if(chance < 128)
      return;
   
   count = (chance > 240) ? 2 : 1;
   
   for(i = 0; i < count; i++)
   {
      goo = P_SpawnMobj(actor->x, actor->y,
                        actor->z + 48*FRACUNIT, gooType);
      P_SetTarget(&goo->target, actor);
      
      goo->momx = P_SubRandom(pr_podpain) << 9;
      goo->momy = P_SubRandom(pr_podpain) << 9;
      goo->momz = (FRACUNIT >> 1) + (P_Random(pr_podpain) << 9);
   }
}

void A_RemovePod(mobj_t *actor)
{
   // actor->tracer points to the generator that made this pod --
   // this method is save game safe and doesn't require any new
   // fields

   if(actor->tracer)
   {
      if(actor->tracer->special1 > 0)
      {
         actor->tracer->special1--;
      }
   }
}

// Note on MAXGENPODS: unlike the limit on PE lost souls, this
// limit makes inarguable sense. If you remove it, areas with
// pod generators will become so crowded with pods that they'll
// begin flying around the map like mad. So, this limit isn't a
// good candidate for removal; it's a necessity.

#define MAXGENPODS 16

void A_MakePod(mobj_t *actor)
{
   angle_t angle;
   fixed_t move;
   mobj_t *mo;
   fixed_t x, y, z;

   // limit pods per generator to avoid crowding, slow-down
   if(actor->special1 >= MAXGENPODS)
      return;

   x = actor->x;
   y = actor->y;
   z = actor->z;
   mo = P_SpawnMobj(x, y, ONFLOORZ, E_SafeThingType(MT_POD));
   if(!P_CheckPosition(mo, x, y))
   {
      P_RemoveMobj(mo);
      return;
   }
   
   P_SetMobjState(mo, E_SafeState(S_POD_GROW1));
   S_StartSound(mo, sfx_newpod);
   
   // give the pod some random momentum
   angle = P_Random(pr_makepod) << 24;
   move  = 9*FRACUNIT >> 1;

   P_ThrustMobj(mo, angle, move);
   
   // use tracer field to link pod to generator, and increment
   // generator's pod count
   P_SetTarget(&mo->tracer, actor);
   actor->special1++;
}

//
// Volcano Actions
//

//
// A_SetTics
//
// Parameterized codepointer to set a thing's tics value.
// * args[0] : base amount
// * args[1] : randomizer modulus value (0 == not randomized)
// * args[2] : counter toggle
//
void A_SetTics(mobj_t *actor)
{
   int baseamt = (int)(actor->state->args[0]);
   int rnd     = (int)(actor->state->args[1]);

   // if counter toggle is set, args[0] is a counter number
   if(actor->state->args[2])
   {
      switch(baseamt)
      {
      case 0:
         baseamt = actor->special1; break;
      case 1:
         baseamt = actor->special2; break;
      case 2:
         baseamt = actor->special3; break;
      default: // invalid
         return;
      }
   }

   actor->tics = baseamt + (rnd ? P_Random(pr_settics) % rnd : 0);
}

//
// A_VolcanoBlast
//
// Called when a volcano is ready to erupt.
//
void A_VolcanoBlast(mobj_t *actor)
{
   static int ballType = -1;
   int i, numvolcballs;
   mobj_t *volcball;
   angle_t angle;

   if(ballType == -1)
      ballType = E_SafeThingType(MT_VOLCANOBLAST);
   
   // spawn 1 to 3 volcano balls
   numvolcballs = (P_Random(pr_volcano) % 3) + 1;
   
   for(i = 0; i < numvolcballs; ++i)
   {
      volcball = P_SpawnMobj(actor->x, actor->y, actor->z + 44*FRACUNIT, 
                             ballType);
      P_SetTarget(&volcball->target, actor);
      S_StartSound(volcball, sfx_bstatk);

      // shoot at a random angle
      volcball->angle = P_Random(pr_volcano) << 24;      
      angle = volcball->angle >> ANGLETOFINESHIFT;
      
      // give it some momentum
      volcball->momx = finecosine[angle];
      volcball->momy = finesine[angle];
      volcball->momz = (5 * FRACUNIT >> 1) + (P_Random(pr_volcano) << 10);

      // check if it hit something immediately
      P_CheckMissileSpawn(volcball);
   }
}

//
// A_VolcBallImpact
//
// Called when a volcano ball hits something.
//
void A_VolcBallImpact(mobj_t *actor)
{
   static int sballType = -1;
   int i;
   mobj_t *svolcball;
   angle_t angle;

   if(sballType == -1)
      sballType = E_SafeThingType(MT_VOLCANOTBLAST);
  
   // if the thing hit the floor, move it up so that the little
   // volcano balls don't hit the floor immediately
   if(actor->z <= actor->floorz)
   {
      actor->flags |= MF_NOGRAVITY;
      actor->flags2 &= ~MF2_LOGRAV;
      actor->z += 28*FRACUNIT;
   }

   // do some radius damage
   P_RadiusAttack(actor, actor->target, 25, actor->info->mod);

   // spawn 4 little volcano balls
   for(i = 0; i < 4; ++i)
   {
      svolcball = P_SpawnMobj(actor->x, actor->y, actor->z, sballType);

      // pass on whatever shot the original volcano ball
      P_SetTarget(&svolcball->target, actor->target);
      
      svolcball->angle = i * ANG90;
      angle = svolcball->angle >> ANGLETOFINESHIFT;
      
      // give them some momentum
      svolcball->momx = FixedMul(7*FRACUNIT/10, finecosine[angle]);
      svolcball->momy = FixedMul(7*FRACUNIT/10, finesine[angle]);
      svolcball->momz = FRACUNIT + (P_Random(pr_svolcano) << 9);
      
      // check if it hit something immediately
      P_CheckMissileSpawn(svolcball);
   }
}

//
// Knight Actions
//

//
// A_KnightAttack
//
// Shoots one of two missiles, depending on whether a Knight
// Ghost or some other object uses it.
//
void A_KnightAttack(mobj_t *actor)
{
   static int ghostType = -1, axeType = -1, redAxeType = -1;

   // resolve thing types only once for max speed
   if(ghostType == -1)
   {
      ghostType  = E_ThingNumForDEHNum(MT_KNIGHTGHOST);
      axeType    = E_SafeThingType(MT_KNIGHTAXE);
      redAxeType = E_SafeThingType(MT_REDAXE);
   }

   if(!actor->target)
      return;

   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_knightat1) & 7) + 1) * 3;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      S_StartSound(actor, sfx_kgtat2);
   }
   else
   {
      S_StartSound(actor, actor->info->attacksound);
      
      if(actor->type == ghostType || P_Random(pr_knightat2) < 40)
      {
         P_SpawnMissile(actor, actor->target, redAxeType, 
                        actor->z + 36*FRACUNIT);
      }
      else
      {
         P_SpawnMissile(actor, actor->target, axeType,
                        actor->z + 36*FRACUNIT);
      }
   }
}

//
// A_DripBlood
//
// Throws some Heretic blood objects out from the source thing.
//
void A_DripBlood(mobj_t *actor)
{
   mobj_t *mo;
   fixed_t x, y;

   x = actor->x + (P_SubRandom(pr_dripblood) << 11);
   y = actor->y + (P_SubRandom(pr_dripblood) << 11);

   mo = P_SpawnMobj(x, y, actor->z, E_SafeThingType(MT_HTICBLOOD));
   
   mo->momx = P_SubRandom(pr_dripblood) << 10;
   mo->momy = P_SubRandom(pr_dripblood) << 10;

   mo->flags2 |= MF2_LOGRAV;
}

//
// Beast Actions
//

void A_BeastAttack(mobj_t *actor)
{
   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_beastbite) & 7) + 1) * 3;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
   }
   else
      P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_BEASTBALL),
                     actor->z + DEFAULTMISSILEZ);
}

void A_BeastPuff(mobj_t *actor)
{
   // 07/29/04: allow momentum to be disabled
   int momentumToggle = (int)(actor->state->args[0]);

   if(P_Random(pr_puffy) > 64)
   {
      fixed_t x, y, z;
      mobj_t *mo;

      // Note: this actually didn't work as intended in Heretic
      // because there, they gave it no momenta. A missile has
      // to be moving to inflict any damage. Doing this makes the
      // smoke a little dangerous. It also requires the missile's
      // target to be passed on, however, since otherwise the 
      // Weredragon that shot this missile can get hurt by it.

      x = actor->x + (P_SubRandom(pr_puffy) << 10);      
      y = actor->y + (P_SubRandom(pr_puffy) << 10);
      z = actor->z + (P_SubRandom(pr_puffy) << 10);

      mo = P_SpawnMobj(x, y, z, E_SafeThingType(MT_PUFFY));

      if(!momentumToggle)
      {
         mo->momx = -(actor->momx / 16);
         mo->momy = -(actor->momy / 16);
      }

      // pass on the beast so that it doesn't hurt itself
      P_SetTarget(&mo->target, actor->target);
   }
}

//
// Ophidian Actions
//

void A_SnakeAttack(mobj_t *actor)
{
   if(!actor->target)
   {
      // avoid going through other attack frames if target is gone
      P_SetMobjState(actor, actor->info->seestate);
      return;
   }

   S_StartSound(actor, actor->info->attacksound);
   A_FaceTarget(actor);
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_SNAKEPRO_A),
                  actor->z + DEFAULTMISSILEZ);
}

void A_SnakeAttack2(mobj_t *actor)
{
   if(!actor->target)
   {
      // avoid going through other attack frames if target is gone
      P_SetMobjState(actor, actor->info->seestate);
      return;
   }

   S_StartSound(actor, actor->info->attacksound);
   A_FaceTarget(actor);
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_SNAKEPRO_B),
                  actor->z + DEFAULTMISSILEZ);
}

//
// Maulotaur Actions
//

//
// A_MinotaurAtk1
//
// Maulotaur melee attack. Big hammer, squishes player.
//
void A_MinotaurAtk1(mobj_t *actor)
{
   player_t *player;

   if(!actor->target)
      return;

   S_StartSound(actor, sfx_stfpow);
   
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, 
                   ((P_Random(pr_minatk1) & 7) + 1) * 4, MOD_HIT);
   
      // if target is player, make the viewheight go down
      if((player = actor->target->player) != NULL)
         player->deltaviewheight = -16*FRACUNIT;
   }
}

//
// P_CheckMntrCharge
//
// Returns true if the Maulotaur should do a charge attack.
//
d_inline static
boolean P_CheckMntrCharge(fixed_t dist, mobj_t *actor, mobj_t *target)
{
   return (target->z + target->height > actor->z &&      // check heights
           target->z + target->height < actor->z + actor->height &&
           dist > 64*FRACUNIT && dist < 512*FRACUNIT &&  // check distance
           P_Random(pr_mindist) < 150);                  // random factor
}

//
// P_CheckFloorFire
//
// Returns true if the Maulotaur should use floor fire.
//
d_inline static boolean P_CheckFloorFire(fixed_t dist, mobj_t *target)
{
   return (target->z == target->floorz && // target on floor?
           dist < 576*FRACUNIT &&         // target in range?
           P_Random(pr_mindist) < 220);   // random factor
}

//
// A_MinotaurDecide
//
// Picks a Maulotaur attack.
//
void A_MinotaurDecide(mobj_t *actor)
{
   angle_t angle;
   mobj_t *target;
   int dist;

   if(!(target = actor->target))
      return;

   S_StartSound(actor, sfx_minsit);
   
   dist = P_AproxDistance(actor->x - target->x, actor->y - target->y);
   
   // charge attack
   if(P_CheckMntrCharge(dist, actor, target))
   {
      // set to charge state and start skull-flying
      P_SetMobjStateNF(actor, E_SafeState(S_MNTR_ATK4_1));
      actor->flags |= MF_SKULLFLY;
      A_FaceTarget(actor);
      
      // give him momentum
      angle = actor->angle >> ANGLETOFINESHIFT;
      actor->momx = FixedMul(13*FRACUNIT, finecosine[angle]);
      actor->momy = FixedMul(13*FRACUNIT, finesine[angle]);
      
      // set a timer
      actor->special1 = TICRATE >> 1;
   }
   else if(P_CheckFloorFire(dist, target))
   { 
      // floor fire
      P_SetMobjState(actor, E_SafeState(S_MNTR_ATK3_1));
      actor->special2 = 0;
   }
   else
      A_FaceTarget(actor);
      
   // Fall through to swing attack
}

//
// A_MinotaurCharge
//
// Called while the Maulotaur is charging.
//
void A_MinotaurCharge(mobj_t *actor)
{
   static int puffType = -1;
   mobj_t *puff;
   
   if(puffType == -1)
      puffType = E_SafeThingType(MT_PHOENIXPUFF);

   if(actor->special1) // test charge timer
   {
      // spawn some smoke and count down the charge
      puff = P_SpawnMobj(actor->x, actor->y, actor->z, puffType);
      puff->momz = FRACUNIT << 1;
      --actor->special1;
   }
   else
   {
      // end of the charge
      actor->flags &= ~MF_SKULLFLY;
      P_SetMobjState(actor, actor->info->seestate);
   }
}

//
// A_MinotaurAtk2
//
// Fireball attack for Maulotaur
//
void A_MinotaurAtk2(mobj_t *actor)
{
   static int mntrfxType = -1;
   mobj_t *mo;
   angle_t angle;
   fixed_t momz;
   
   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX1);

   if(!actor->target)
      return;

   S_StartSound(actor, sfx_minat2);

   if(P_CheckMeleeRange(actor)) // hit directly
   {
      P_DamageMobj(actor->target, actor, actor, 
                   ((P_Random(pr_minatk2) & 7) + 1) * 5, MOD_HIT);
   }
   else // missile spread attack
   {
      fixed_t z = actor->z + 40*FRACUNIT;
      
      // shoot a missile straight
      mo = P_SpawnMissile(actor, actor->target, mntrfxType, z);
      S_StartSound(mo, sfx_minat2);

      // shoot 4 more missiles in a spread
      momz = mo->momz;
      angle = mo->angle;
      P_SpawnMissileAngle(actor, mntrfxType, angle - (ANG45/8),  momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle + (ANG45/8),  momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle - (ANG45/16), momz, z);
      P_SpawnMissileAngle(actor, mntrfxType, angle + (ANG45/16), momz, z);
   }
}

//
// A_MinotaurAtk3
//
// Performs floor fire attack, or melee if in range.
//
void A_MinotaurAtk3(mobj_t *actor)
{
   static int mntrfxType = -1;
   mobj_t *mo;
   player_t *player;

   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX2);

   if(!actor->target)
      return;

   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, 
                   ((P_Random(pr_minatk3) & 7) + 1) * 5, MOD_HIT);

      // if target is player, decrease viewheight
      if((player = actor->target->player) != NULL)
         player->deltaviewheight = -16*FRACUNIT;
   }
   else
   {
      // floor fire attack
      mo = P_SpawnMissile(actor, actor->target, mntrfxType, ONFLOORZ);
      S_StartSound(mo, sfx_minat1);
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
// Called by floor fire missile as it moves.
// Spawns small burning flames.
//
void A_MntrFloorFire(mobj_t *actor)
{
   static int mntrfxType = -1;
   mobj_t *mo;
   fixed_t x, y;

   if(mntrfxType == -1)
      mntrfxType = E_SafeThingType(MT_MNTRFX3);

   // set actor to floor
   actor->z = actor->floorz;
   
   // determine spawn coordinates for small flame
   x = actor->x + (P_SubRandom(pr_mffire) << 10);
   y = actor->y + (P_SubRandom(pr_mffire) << 10);
   
   // spawn the flame
   mo = P_SpawnMobj(x, y, ONFLOORZ, mntrfxType);

   // pass on the Maulotaur as the source of damage
   P_SetTarget(&mo->target, actor->target);

   // give it a bit of momentum and then check to see if it hit something
   mo->momx = 1;
   P_CheckMissileSpawn(mo);
}

//
// Iron Lich Actions
//

//
// A_LichFire
//
// Spawns a column of expanding fireballs. Called by A_LichAttack,
// but also available separately.
//
void A_LichFire(mobj_t *actor)
{
   static int headfxType = -1, frameNum = -1;
   mobj_t *target, *baseFire, *fire;
   int i;

   if(headfxType == -1)
   {
      headfxType = E_SafeThingType(MT_LICHFX3);
      frameNum   = E_SafeState(S_LICHFX3_4);
   }

   if(!(target = actor->target))
      return;

   // spawn the parent fireball
   baseFire = P_SpawnMissile(actor, target, headfxType, 
                             actor->z + DEFAULTMISSILEZ);
   
   // set it to S_HEADFX3_4 so that it doesn't grow
   P_SetMobjState(baseFire, frameNum);

   S_StartSound(actor, sfx_hedat1);

   for(i = 0; i < 5; ++i)
   {
      fire = P_SpawnMobj(baseFire->x, baseFire->y, baseFire->z, headfxType);
    
      // pass on the lich as the originator
      P_SetTarget(&fire->target, baseFire->target);
      
      // inherit the motion properties of the parent fireball
      fire->angle = baseFire->angle;
      fire->momx  = baseFire->momx;
      fire->momy  = baseFire->momy;
      fire->momz  = baseFire->momz;
      
      // start out with zero damage
      fire->damage = 0;

      // set a counter for growth
      fire->special1 = (i + 1) << 1;
      
      P_CheckMissileSpawn(fire);
   }
}

//
// A_LichWhirlwind
//
// Spawns a heat-seeking tornado. Called by A_LichAttack, but also
// available separately.
//
void A_LichWhirlwind(mobj_t *actor)
{
   static int wwType = -1;
   mobj_t *mo, *target;

   if(!(target = actor->target))
      return;

   if(wwType == -1)
      wwType = E_SafeThingType(MT_WHIRLWIND);

   mo = P_SpawnMissile(actor, target, wwType, actor->z);
   
   // use mo->tracer to track target
   P_SetTarget(&mo->tracer, target);
   
   mo->special1 = 20*TICRATE; // duration
   mo->special2 = 50;         // timer for active sound
   mo->special3 = 60;         // explocount limit

   S_StartSound(actor, sfx_hedat3);
}

//
// A_LichAttack
//
// Main Iron Lich attack logic.
//
void A_LichAttack(mobj_t *actor)
{
   static int fxType = -1;
   mobj_t *target;
   int randAttack, dist;

   // Distance threshold = 512 units
   // Probabilities:
   // Attack       Close   Far
   // -----------------------------
   // Ice ball       20%   60%
   // Fire column    40%   20%
   // Whirlwind      40% : 20%

   if(fxType == -1)
      fxType = E_SafeThingType(MT_LICHFX1);
   
   if(!(target = actor->target))
      return;

   A_FaceTarget(actor);
   
   // hit directly when in melee range
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(target, actor, actor, 
                   ((P_Random(pr_lichmelee) & 7) + 1) * 6, 
                   MOD_HIT);
      return;
   }
   
   // determine distance and use it to alter attack probabilities
   dist = P_AproxDistance(actor->x-target->x, actor->y-target->y) > 512*FRACUNIT;
   
   randAttack = P_Random(pr_lichattack);
   
   if(randAttack < (dist ? 150 : 50))
   {
      // ice attack
      P_SpawnMissile(actor, target, fxType, actor->z + DEFAULTMISSILEZ);
      S_StartSound(actor, sfx_hedat2);	
   }
   else if(randAttack < (dist ? 200 : 150))
      A_LichFire(actor);
   else
      A_LichWhirlwind(actor);
}

//
// A_WhirlwindSeek
//
// Special homing maintenance pointer for whirlwinds.
//
void A_WhirlwindSeek(mobj_t *actor)
{
   // decrement duration counter
   if((actor->special1 -= 3) < 0)
   {
      actor->momx = actor->momy = actor->momz = 0;
      P_SetMobjState(actor, actor->info->deathstate);
      actor->flags &= ~MF_MISSILE;
      return;
   }
   
   // decrement active sound counter
   if((actor->special2 -= 3) < 0)
   {
      actor->special2 = 58 + (P_Random(pr_whirlseek) & 31);
      S_StartSound(actor, sfx_hedat3);
   }
   
   // test if tracer has become an invalid target
   if(actor->tracer && 
      (actor->tracer->flags3 & MF3_GHOST ||
       actor->tracer->health < 0))
   {
      mobj_t *originator = actor->target;
      mobj_t *origtarget = originator ? originator->target : NULL;

      // See if the Lich has a new target; if so, maybe chase it now.
      // This keeps the tornado from sitting around uselessly.
      if(originator && origtarget && actor->tracer != origtarget &&
         origtarget->health > 0 &&
         !(origtarget->flags3 & MF3_GHOST) &&
         !(originator->flags & origtarget->flags & MF_FRIEND))
         P_SetTarget(&actor->tracer, origtarget);
      else
         return;
   }

   // follow the target
   P_HticTracer(actor, ANGLE_1 * 10, ANGLE_1 * 30);
}

//
// A_LichIceImpact
//
// Called when a Lich ice ball hits something. Shatters into
// shards that fly in all directions.
//
void A_LichIceImpact(mobj_t *actor)
{
   static int fxType = -1;
   int i;
   angle_t angle;
   mobj_t *shard;

   if(fxType == -1)
      fxType = E_SafeThingType(MT_LICHFX2);
   
   for(i = 0; i < 8; ++i)
   {
      shard = P_SpawnMobj(actor->x, actor->y, actor->z, fxType);      
      P_SetTarget(&shard->target, actor->target);

      // send shards out every 45 degrees
      shard->angle = i * ANG45;

      // set momenta
      angle = shard->angle >> ANGLETOFINESHIFT;
      shard->momx = FixedMul(shard->info->speed, finecosine[angle]);
      shard->momy = FixedMul(shard->info->speed, finesine[angle]);
      shard->momz = -3 * FRACUNIT / 5;
      
      // check the spawn to see if it hit immediately
      P_CheckMissileSpawn(shard);
   }
}

//
// A_LichFireGrow
//
// Called by Lich fire pillar fireballs so that they can expand.
//
void A_LichFireGrow(mobj_t *actor)
{
   static int frameNum = -1;

   if(frameNum == -1)
      frameNum = E_SafeState(S_LICHFX3_4);

   actor->z += 9*FRACUNIT;
   
   if(--actor->special1 == 0) // count down growth timer
   {
      actor->damage = actor->info->damage; // restore normal damage
      P_SetMobjState(actor, frameNum);  // don't grow any more
   }
}

//
// Imp Actions
//

//
// A_ImpChargeAtk
//
// Almost identical to the Lost Soul's attack, but adds a frequent
// failure to attack so that the imps do not constantly charge.
//
void A_ImpChargeAtk(mobj_t *actor)
{   
   if(!actor->target || P_Random(pr_impcharge) > 64)
   {
      P_SetMobjState(actor, actor->info->seestate);
      return;
   }
   
   S_StartSound(actor, actor->info->attacksound);
   
   P_SkullFly(actor, 12*FRACUNIT);
}

//
// A_ImpMeleeAtk
//
void A_ImpMeleeAtk(mobj_t *actor)
{
   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, 
                   5 + (P_Random(pr_impmelee) & 7), MOD_HIT);
   }
}

//
// A_ImpMissileAtk
//
// Leader Imp's missile/melee attack
//
void A_ImpMissileAtk(mobj_t *actor)
{
   static int fxType = -1;

   if(!actor->target)
      return;

   if(fxType == -1)
      fxType = E_SafeThingType(MT_IMPBALL);

   S_StartSound(actor, actor->info->attacksound);

   if(P_CheckMeleeRange(actor))
   {
      P_DamageMobj(actor->target, actor, actor, 
                   5 + (P_Random(pr_impmelee2) & 7), MOD_HIT);
   }
   else
      P_SpawnMissile(actor, actor->target, fxType, actor->z + DEFAULTMISSILEZ);
}

//
// A_ImpDeath
//
// Called when the imp dies normally.
//
void A_ImpDeath(mobj_t *actor)
{
   actor->flags &= ~MF_SOLID;
   actor->flags2 |= MF2_FOOTCLIP;
   
   if(actor->z <= actor->floorz && actor->info->crashstate)
   {
      actor->intflags |= MIF_CRASHED;
      P_SetMobjState(actor, actor->info->crashstate);
   }
}

//
// A_ImpXDeath1
//
// Called on imp extreme death. First half of action
//
void A_ImpXDeath1(mobj_t *actor)
{
   actor->flags &= ~MF_SOLID;
   actor->flags |= MF_NOGRAVITY;
   actor->flags2 |= MF2_FOOTCLIP;

   // set special1 so the crashstate goes to the
   // extreme crash death
   actor->special1 = 666;
}

//
// A_ImpXDeath2
//
// Called on imp extreme death. Second half of action.
//
void A_ImpXDeath2(mobj_t *actor)
{
   actor->flags &= ~MF_NOGRAVITY;

   if(actor->z <= actor->floorz && actor->info->crashstate)
   {
      actor->intflags |= MIF_CRASHED;
      P_SetMobjState(actor, actor->info->crashstate);
   }
}

//
// A_ImpExplode
//
// Called from imp crashstate.
//
void A_ImpExplode(mobj_t *actor)
{
   static int fxType1 = -1, fxType2 = -1, stateNum = -1;
   mobj_t *mo;

   // haleyjd 09/13/04: it's possible for an imp to enter its
   // crash state between calls to ImpXDeath1 and ImpXDeath2 --
   // if this happens, the NOGRAVITY flag must be cleared here,
   // or else it will remain indefinitely.

   actor->flags &= ~MF_NOGRAVITY;

   if(fxType1 == -1)
   {
      fxType1 = E_SafeThingType(MT_IMPCHUNK1);
      fxType2 = E_SafeThingType(MT_IMPCHUNK2);
      stateNum = E_SafeState(S_IMP_XCRASH1);
   }
   
   mo = P_SpawnMobj(actor->x, actor->y, actor->z, fxType1);
   mo->momx = P_SubRandom(pr_impcrash) << 10;
   mo->momy = P_SubRandom(pr_impcrash) << 10;
   mo->momz = 9*FRACUNIT;
   
   mo = P_SpawnMobj(actor->x, actor->y, actor->z, fxType2);
   mo->momx = P_SubRandom(pr_impcrash) << 10;
   mo->momy = P_SubRandom(pr_impcrash) << 10;
   mo->momz = 9*FRACUNIT;

   // extreme death crash
   if(actor->special1 == 666)
      P_SetMobjState(actor, stateNum);
}

//
// Other Parameterized Codepointer Functions
//

//
// A_MissileAttack
//
// Parameterized missile firing for enemies.
// Arguments:
// * args[0] = type to fire
// * args[1] = whether or not to home on target
// * args[2] = amount to add to standard missile z firing height
// * args[3] = amount to add to actor angle
// * args[4] = optional state to enter for melee attack
//
void A_MissileAttack(mobj_t *actor)
{
   int type, a;
   fixed_t z, momz;
   boolean homing;
   angle_t ang;
   mobj_t *mo;
   int sdehnum, statenum;

   if(!actor->target)
      return;
   
   type    = E_SafeThingType((int)(actor->state->args[0]));
   homing  = !!(actor->state->args[1]);
   z       = (fixed_t)(actor->state->args[2] * FRACUNIT);
   a       = (int)(actor->state->args[3]);
   sdehnum = (int)(actor->state->args[4]);

   A_FaceTarget(actor);

   if(sdehnum >= 0)
   {
      statenum = E_SafeState(sdehnum);
      if(P_CheckMeleeRange(actor))
      {
         P_SetMobjState(actor, statenum);
         return;
      }
   }

   // adjust angle -> BAM (must adjust negative angles too)
   if(a >= 360)
      a = a - 360;
   else if(a < 0)
      a = 360 + a;

   ang = (angle_t)(((ULong64)a << 32) / 360);

   // adjust z coordinate
   z = actor->z + DEFAULTMISSILEZ + z;

   if(!a)
      mo = P_SpawnMissile(actor, actor->target, type, z);
   else
   {
      // calculate z momentum
      mobj_t *target = actor->target;

      momz = P_MissileMomz(target->x - actor->x,
                           target->y - actor->y,
                           target->z - actor->z,
                           mobjinfo[type].speed);

      mo = P_SpawnMissileAngle(actor, type, actor->angle + ang, momz, z);
   }

   if(homing)
      P_SetTarget(&mo->tracer, actor->target);
}

//
// A_MissileSpread
//
// Fires an angular spread of missiles.
// Arguments:
// * args[0] = type to fire
// * args[1] = number of missiles to fire
// * args[2] = amount to add to standard missile z firing height
// * args[3] = total angular sweep
// * args[4] = optional state to enter for melee attack
//
void A_MissileSpread(mobj_t *actor)
{
   int type, num, a, i;
   fixed_t z, momz;
   angle_t angsweep, ang, astep;
   int sdehnum, statenum;

   if(!actor->target)
      return;
   
   type    = E_SafeThingType((int)(actor->state->args[0]));
   num     = (int)(actor->state->args[1]);
   z       = (fixed_t)(actor->state->args[2] * FRACUNIT);
   a       = (int)(actor->state->args[3]);
   sdehnum = (int)(actor->state->args[4]);

   if(num < 2)
      return;

   A_FaceTarget(actor);

   if(sdehnum >= 0)
   {
      statenum = E_SafeState(sdehnum);
      if(P_CheckMeleeRange(actor))
      {
         P_SetMobjState(actor, statenum);
         return;
      }
   }

   // adjust angle -> BAM (must adjust negative angles too)
   if(a >= 360)
      a = a - 360;
   else if(a < 0)
      a = 360 + a;

   angsweep = (angle_t)(((ULong64)a << 32) / 360);

   // adjust z coordinate
   z = actor->z + DEFAULTMISSILEZ + z;

   ang = actor->angle - angsweep / 2;
   astep = angsweep / (num - 1);

   for(i = 0; i < num; ++i)
   {
      // calculate z momentum
      mobj_t *target = actor->target;

      momz = P_MissileMomz(target->x - actor->x,
                           target->y - actor->y,
                           target->z - actor->z,
                           mobjinfo[type].speed);

      P_SpawnMissileAngle(actor, type, ang, momz, z);

      ang += astep;
   }
}

//
// A_BulletAttack
//
// A parameterized monster bullet code pointer
// Parameters:
// args[0] : sound (dehacked num)
// args[1] : accuracy (always, never, ssg, monster)
// args[2] : number of bullets to fire
// args[3] : damage factor of bullets
// args[4] : damage modulus of bullets
//
void A_BulletAttack(mobj_t *actor)
{
   int i, sound, accurate, numbullets, damage, dmgmod, slope;
   sfxinfo_t *sfx;

   if(!actor->target)
      return;

   sound      = (int)(actor->state->args[0]);
   accurate   = (int)(actor->state->args[1]);
   numbullets = (int)(actor->state->args[2]);
   damage     = (int)(actor->state->args[3]);
   dmgmod     = (int)(actor->state->args[4]);

   if(!accurate)
      accurate = 1;

   if(dmgmod < 1)
      dmgmod = 1;
   else if(dmgmod > 256)
      dmgmod = 256;

   sfx = E_SoundForDEHNum(sound);

   A_FaceTarget(actor);
   S_StartSfxInfo(actor, sfx);

   slope = P_AimLineAttack(actor, actor->angle, MISSILERANGE, 0);

   // loop on numbullets
   for(i = 0; i < numbullets; i++)
   {
      int dmg = damage * (P_Random(pr_monbullets)%dmgmod + 1);
      angle_t angle = actor->angle;
      
      if(accurate <= 2 || accurate == 4)
      {
         // if never accurate or monster accurate,
         // add some to the angle
         if(accurate == 2 || accurate == 4)
         {
            int aimshift = ((accurate == 4) ? 20 : 18);
            angle += P_SubRandom(pr_monmisfire) << aimshift;
         }

         P_LineAttack(actor, angle, MISSILERANGE, slope, dmg);
      }
      else if(accurate == 3) // ssg spread
      {
         angle += P_SubRandom(pr_monmisfire) << 19;         
         slope += P_SubRandom(pr_monmisfire) << 5;

         P_LineAttack(actor, angle, MISSILERANGE, slope, dmg);
      }
   }
}

void A_ThingSummon(mobj_t *actor)
{
   fixed_t x, y, z;
   mobj_t  *newmobj;
   angle_t an;
   int     type, prestep, deltaz, kill_or_remove, make_child;

   type    = E_SafeThingType((int)(actor->state->args[0]));
   prestep = (int)(actor->state->args[1]) << FRACBITS;
   deltaz  = (int)(actor->state->args[2]) << FRACBITS;

   kill_or_remove = !!(actor->state->args[3]);
   make_child     = !!(actor->state->args[4]);
   
   // good old-fashioned pain elemental style spawning
   
   an = actor->angle >> ANGLETOFINESHIFT;
   
   prestep = prestep + 3*(actor->info->radius + mobjinfo[type].radius)/2;
   
   x = actor->x + FixedMul(prestep, finecosine[an]);
   y = actor->y + FixedMul(prestep, finesine[an]);
   z = actor->z + deltaz;

   // Check whether the thing is being spawned through a 1-sided
   // wall or an impassible line, or a "monsters can't cross" line.
   // If it is, then we don't allow the spawn.
   
   if(Check_Sides(actor, x, y))
      return;

   newmobj = P_SpawnMobj(x, y, z, type);
   
   // Check to see if the new thing's z value is above the
   // ceiling of its new sector, or below the floor. If so, kill it.

   if((newmobj->z >
      (newmobj->subsector->sector->ceilingheight - newmobj->height)) ||
      (newmobj->z < newmobj->subsector->sector->floorheight))
   {
      // kill it immediately
      switch(kill_or_remove)
      {
      case 0:
         A_Die(newmobj);
         break;
      case 1:
         P_RemoveMobj(newmobj);
         break;
      }
      return;
   }                                                         
   
   // spawn thing with same friendliness
   newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

   // killough 8/29/98: add to appropriate thread
   P_UpdateThinker(&newmobj->thinker);
   
   // Check for movements.
   // killough 3/15/98: don't jump over dropoffs:

   if(!P_TryMove(newmobj, newmobj->x, newmobj->y, false))
   {
      // kill it immediately
      switch(kill_or_remove)
      {
      case 0:
         A_Die(newmobj);
         break;
      case 1:
         P_RemoveMobj(newmobj);
         break;
      }
      return;
   }

   // give same target
   P_SetTarget(&newmobj->target, actor->target);

   // set child properties
   if(make_child)
   {
      P_SetTarget(&newmobj->tracer, actor);
      newmobj->intflags |= MIF_ISCHILD;
   }
}

void A_KillChildren(mobj_t *actor)
{
   thinker_t *th;
   int kill_or_remove = !!(actor->state->args[0]);

   for(th = thinkercap.next; th != &thinkercap; th = th->next)
   {
      mobj_t *mo;

      if(th->function != P_MobjThinker)
         continue;

      mo = (mobj_t *)th;

      if(mo->intflags & MIF_ISCHILD && mo->tracer == actor)
      {
         switch(kill_or_remove)
         {
         case 0:
            A_Die(mo);
            break;
         case 1:
            P_RemoveMobj(mo);
            break;
         }
      }
   }
}

//
// A_AproxDistance
//
// Parameterized pointer, returns the approximate distance between
// a thing and its target in the indicated counter.
// * args[0] == destination counter
//
void A_AproxDistance(mobj_t *actor)
{
   mobj_t *target;
   short *dest = NULL;
   fixed_t dist;

   switch(actor->state->args[0])
   {
   case 0:
      dest = &actor->special1; break;
   case 1:
      dest = &actor->special2; break;
   case 2:
      dest = &actor->special3; break;
   default:
      return; // invalid
   }

   if(!(target = actor->target))
   {
      *dest = -1;
      return;
   }
   
   dist = P_AproxDistance(actor->x - target->x, actor->y - target->y);

   *dest = dist >> FRACBITS;
}

//
// A_ShowMessage
//
// A codepointer that can display EDF strings to the players as
// messages.
// Arguments:
// args[0] = EDF message number
// args[1] = message type
//
void A_ShowMessage(mobj_t *actor)
{
   int msgnum = (int)(actor->state->args[0]);
   int type   = (int)(actor->state->args[1]);
   int player = (int)(actor->state->args[2]);
   edf_string_t *msg;

   // find the message
   if(!(msg = E_StringForNum(msgnum)))
      return;

   switch(type)
   {
   case 0:
      C_Printf(msg->string);
      break;
   case 1:
      doom_printf(msg->string);
      break;
   case 2:
      HU_CenterMessage(msg->string);
      break;
   }
}

//
// Frame Scripting Codepointers
//
// These implement what is more or less a full frame assembly
// language, whereby you cannot only loop, but branch on hard
// criteria, and write to memory. Not strictly necessary, but will
// make EDF more flexible and reduce the need for action scripting
// for mundane logical control of state transitions (and the
// potential of confusing recursive script executions).
//

// Codepointer comparison types

enum
{
   CPC_LESS,
   CPC_LESSOREQUAL,
   CPC_GREATER,
   CPC_GREATEROREQUAL,
   CPC_EQUAL,
   CPC_NOTEQUAL,
   CPC_BITWISEAND,
   
   CPC_CNTR_LESS,           // alternate counter versions
   CPC_CNTR_LESSOREQUAL,
   CPC_CNTR_GREATER,
   CPC_CNTR_GREATEROREQUAL,
   CPC_CNTR_EQUAL,
   CPC_CNTR_NOTEQUAL,
   CPC_CNTR_BITWISEAND,

   CPC_NUMIMMEDIATE = CPC_BITWISEAND + 1
};

// Codepointer operation types

enum
{
   CPOP_ASSIGN,
   CPOP_ADD,
   CPOP_SUB,
   CPOP_MUL,
   CPOP_DIV,
   CPOP_MOD,
   CPOP_AND,
   CPOP_ANDNOT,
   CPOP_OR,
   CPOP_XOR,
   CPOP_RND,
   CPOP_RNDMOD,
   CPOP_DAMAGE,
   CPOP_SHIFTLEFT,
   CPOP_SHIFTRIGHT,

   // unary operators
   CPOP_ABS,
   CPOP_NEGATE,
   CPOP_NOT,
   CPOP_INVERT,
};

//
// A_HealthJump
//
// Parameterized codepointer for branching based on comparisons
// against a thing's health.
//
// args[0] : state number
// args[1] : comparison type
// args[2] : health value OR counter number
//
void A_HealthJump(mobj_t *mo)
{
   boolean branch  = false;
   int statenum    = mo->state->args[0];
   int checktype   = mo->state->args[1];
   int checkhealth = mo->state->args[2];
   
   // validate state number
   statenum = E_StateNumForDEHNum(statenum);
   if(statenum == NUMSTATES)
      return;

   // 08/02/04:
   // support getting check value from a counter
   // if checktype is greater than the last immediate operator,
   // then the checkhealth value is actually a counter number

   if(checktype >= CPC_NUMIMMEDIATE)
   {
      // turn it into the corresponding immediate operation
      checktype -= CPC_NUMIMMEDIATE;

      switch(checkhealth)
      {
      case 0:
         checkhealth = mo->special1;
         break;
      case 1:
         checkhealth = mo->special2;
         break;
      case 2:
         checkhealth = mo->special3;
         break;
      default:
         return; // invalid counter number
      }
   }

   switch(checktype)
   {
   case CPC_LESS:
      branch = (mo->health < checkhealth); break;
   case CPC_LESSOREQUAL:
      branch = (mo->health <= checkhealth); break;
   case CPC_GREATER:
      branch = (mo->health > checkhealth); break;
   case CPC_GREATEROREQUAL:
      branch = (mo->health >= checkhealth); break;
   case CPC_EQUAL:
      branch = (mo->health == checkhealth); break;
   case CPC_NOTEQUAL:
      branch = (mo->health != checkhealth); break;
   case CPC_BITWISEAND:
      branch = (mo->health & checkhealth); break;
   default:
      break;
   }

   if(branch)
      P_SetMobjState(mo, statenum);
}

//
// A_CounterJump
//
// Parameterized codepointer for branching based on comparisons
// against a thing's counter values.
//
// args[0] : state number
// args[1] : comparison type
// args[2] : immediate value OR counter number
// args[3] : counter # to use
//
void A_CounterJump(mobj_t *mo)
{
   boolean branch = false;
   int statenum   = mo->state->args[0];
   int checktype  = mo->state->args[1];
   short value    = (short)(mo->state->args[2]);
   int cnum       = mo->state->args[3];
   short *counter;
   
   // validate state number
   statenum = E_StateNumForDEHNum(statenum);
   if(statenum == NUMSTATES)
      return;

   switch(cnum)
   {
   case 0:
      counter = &mo->special1; break;
   case 1:
      counter = &mo->special2; break;
   case 2:
      counter = &mo->special3; break;
   default:
      return;
   }

   // 08/02/04:
   // support getting check value from a counter
   // if checktype is greater than the last immediate operator,
   // then the comparison value is actually a counter number

   if(checktype >= CPC_NUMIMMEDIATE)
   {
      // turn it into the corresponding immediate operation
      checktype -= CPC_NUMIMMEDIATE;

      switch(value)
      {
      case 0:
         value = mo->special1;
         break;
      case 1:
         value = mo->special2;
         break;
      case 2:
         value = mo->special3;
         break;
      default:
         return; // invalid counter number
      }
   }

   switch(checktype)
   {
   case CPC_LESS:
      branch = (*counter < value); break;
   case CPC_LESSOREQUAL:
      branch = (*counter <= value); break;
   case CPC_GREATER:
      branch = (*counter > value); break;
   case CPC_GREATEROREQUAL:
      branch = (*counter >= value); break;
   case CPC_EQUAL:
      branch = (*counter == value); break;
   case CPC_NOTEQUAL:
      branch = (*counter != value); break;
   case CPC_BITWISEAND:
      branch = (*counter & value); break;
   default:
      break;
   }

   if(branch)
      P_SetMobjState(mo, statenum);
}

//
// A_CounterSwitch
//
// This powerful codepointer can branch to one of N states
// depending on the value of the indicated counter, and it
// remains totally safe at all times. If the entire indicated
// frame set is not valid, no actions will be taken.
//
// args[0] : counter # to use
// args[1] : DeHackEd number of first frame in consecutive set
// args[2] : number of frames in consecutive set
//
void A_CounterSwitch(mobj_t *mo)
{
   int cnum = mo->state->args[0];
   int startstate = mo->state->args[1];
   int numstates  = mo->state->args[2] - 1;
   short *counter;

   // get counter
   switch(cnum)
   {
   case 0:
      counter = &mo->special1; break;
   case 1:
      counter = &mo->special2; break;
   case 2:
      counter = &mo->special3; break;
   default:
      return;
   }

   // verify startstate
   startstate = E_StateNumForDEHNum(startstate);
   if(startstate == NUMSTATES)
      return;

   // verify last state is < NUMSTATES
   if(startstate + numstates >= NUMSTATES)
      return;

   // verify counter is in range
   if(*counter < 0 || *counter > numstates)
      return;

   // jump!
   P_SetMobjState(mo, startstate + *counter);
}

//
// A_SetCounter
//
// Sets the value of the indicated counter variable for the thing.
// Can perform numerous operations -- this is more like a virtual
// machine than a codepointer ;)
//
// args[0] : counter # to set
// args[1] : value to utilize
// args[2] : operation to perform
//
void A_SetCounter(mobj_t *mo)
{
   int cnum = mo->state->args[0];
   short value = (short)(mo->state->args[1]);
   int specialop = mo->state->args[2];
   short *counter;

   switch(cnum)
   {
   case 0:
      counter = &mo->special1; break;
   case 1:
      counter = &mo->special2; break;
   case 2:
      counter = &mo->special3; break;
   default:
      return;
   }

   switch(specialop)
   {
   case CPOP_ASSIGN:
      *counter = value; break;
   case CPOP_ADD:
      *counter += value; break;
   case CPOP_SUB:
      *counter -= value; break;
   case CPOP_MUL:
      *counter *= value; break;
   case CPOP_DIV:
      if(value) // don't divide by zero
         *counter /= value;
      break;
   case CPOP_MOD:
      if(value > 0) // only allow modulus by positive values
         *counter %= value;
      break;
   case CPOP_AND:
      *counter &= value; break;
   case CPOP_ANDNOT:
      *counter &= ~value; break; // compound and-not operation
   case CPOP_OR:
      *counter |= value; break;
   case CPOP_XOR:
      *counter ^= value; break;
   case CPOP_RND:
      *counter = P_Random(pr_setcounter); break;
   case CPOP_RNDMOD:
      if(value > 0)
         *counter = P_Random(pr_setcounter) % value; break;
   case CPOP_SHIFTLEFT:
      *counter <<= value; break;
   case CPOP_SHIFTRIGHT:
      *counter >>= value; break;
   default:
      break;
   }
}

//
// A_CounterOp
//
// Sets the value of the indicated counter variable for the thing
// using two (possibly the same) counters as operands.
//
// args[0] : counter operand #1
// args[1] : counter operand #2
// args[2] : counter destination
// args[3] : operation to perform
//
void A_CounterOp(mobj_t *mo)
{
   int c_oper1_num = mo->state->args[0];
   int c_oper2_num = mo->state->args[1];
   int c_dest_num  = mo->state->args[2];   
   int specialop = mo->state->args[2];
   
   short *c_oper1, *c_oper2, *c_dest;

   switch(c_oper1_num)
   {
   case 0:
      c_oper1 = &mo->special1; break;
   case 1:
      c_oper1 = &mo->special2; break;
   case 2:
      c_oper1 = &mo->special3; break;
   default:
      return;
   }

   switch(c_oper2_num)
   {
   case 0:
      c_oper2 = &mo->special1; break;
   case 1:
      c_oper2 = &mo->special2; break;
   case 2:
      c_oper2 = &mo->special3; break;
   default:
      return;
   }

   switch(c_dest_num)
   {
   case 0:
      c_dest = &mo->special1; break;
   case 1:
      c_dest = &mo->special2; break;
   case 2:
      c_dest = &mo->special3; break;
   default:
      return;
   }

   switch(specialop)
   {
   case CPOP_ADD:
      *c_dest = *c_oper1 + *c_oper2; break;
   case CPOP_SUB:
      *c_dest = *c_oper1 - *c_oper2; break;
   case CPOP_MUL:
      *c_dest = *c_oper1 * *c_oper2; break;
   case CPOP_DIV:
      if(c_oper2) // don't divide by zero
         *c_dest = *c_oper1 / *c_oper2;
      break;
   case CPOP_MOD:
      if(*c_oper2 > 0) // only allow modulus by positive values
         *c_dest = *c_oper1 % *c_oper2;
      break;
   case CPOP_AND:
      *c_dest = *c_oper1 & *c_oper2; break;
   case CPOP_OR:
      *c_dest = *c_oper1 | *c_oper2; break;
   case CPOP_XOR:
      *c_dest = *c_oper1 ^ *c_oper2; break;
   case CPOP_DAMAGE:
      // do a HITDICE-style calculation
      if(*c_oper2 > 0) // the modulus must be positive
         *c_dest = *c_oper1 * ((P_Random(pr_setcounter) % *c_oper2) + 1);
      break;
   case CPOP_SHIFTLEFT:
      *c_dest = *c_oper1 << *c_oper2; break;
   case CPOP_SHIFTRIGHT:
      *c_dest = *c_oper1 >> *c_oper2; break;

      // unary operations (c_oper2 is unused for these)
   case CPOP_ABS:
      *c_dest = (short)(abs(*c_oper1)); break;
   case CPOP_NEGATE:
      *c_dest = -(*c_oper1); break;
   case CPOP_NOT:
      *c_dest = !(*c_oper1); break;
   case CPOP_INVERT:
      *c_dest = ~(*c_oper1); break;
   default:
      break;
   }
}

//
// A_CopyCounter
//
// Copies the value of one counter into another.
//
// args[0] : source counter #
// args[1] : destination counter #
//
void A_CopyCounter(mobj_t *mo)
{
   int cnum1 = mo->state->args[0];
   int cnum2 = mo->state->args[1];
   short *src, *dest;

   switch(cnum1)
   {
   case 0:
      src = &mo->special1; break;
   case 1:
      src = &mo->special2; break;
   case 2:
      src = &mo->special3; break;
   default:
      return;
   }

   switch(cnum2)
   {
   case 0:
      dest = &mo->special1; break;
   case 1:
      dest = &mo->special2; break;
   case 2:
      dest = &mo->special3; break;
   default:
      return;
   }

   *dest = *src;
}

//
// A_TargetJump
//
// Parameterized codepointer for branching based on whether a
// thing's target is valid and alive.
//
// args[0] : state number
//
void A_TargetJump(mobj_t *mo)
{
   int statenum   = mo->state->args[0];
   
   // validate state number
   if((statenum = E_StateNumForDEHNum(statenum)) == NUMSTATES)
      return;

   // 1) must be valid
   // 2) must be alive
   // 3) if a super friend, target cannot be a friend
   if(mo->target && mo->target->health > 0 &&
      !((mo->flags & mo->target->flags & MF_FRIEND) && 
        mo->flags3 & MF3_SUPERFRIEND))
      P_SetMobjState(mo, statenum);
}

// EOF

