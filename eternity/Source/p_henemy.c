// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
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
#include "e_edf.h"
#include "e_sound.h"

void A_FaceTarget(mobj_t *actor);
void A_Chase(mobj_t *actor);
void A_Fall(mobj_t *actor);
void A_Pain(mobj_t *actor);

//
// A_SpawnGlitter
//
// Parameterized code pointer to spawn inert objects with some
// positive z momentum
//
// Parameters:
// args[0] - object type (use DeHackEd number)
// args[1] - momentum (scaled by FRACUNIT)
//
void A_SpawnGlitter(mobj_t *actor)
{
   mobj_t *glitter;
   int glitterType;
   fixed_t initMomentum;
   fixed_t x, y;

   glitterType  = (int)(actor->state->args[0]);
   initMomentum = (fixed_t)(actor->state->args[1]);

   if(!initMomentum)
      initMomentum = FRACUNIT >> 2;

   // haleyjd 07/05/03: adjusted for EDF
   glitterType = E_SafeThingType(glitterType);

   // randomize spawning coordinates within a 32-unit square
   x = actor->x + ((P_Random(pr_tglit) & 31) - 16) * FRACUNIT;
   y = actor->y + ((P_Random(pr_tglit) & 31) - 16) * FRACUNIT;

   glitter = P_SpawnMobj(x, y, actor->floorz, glitterType);

   // give it some upward momentum
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

void P_HticDrop(mobj_t *actor, mobjtype_t type)
{
   int t;
   mobj_t *item;

   item = P_SpawnMobj(actor->x, actor->y, 
                      actor->z + (actor->height>>1), 
                      type);
   
   t = P_Random(pr_hdropmom);
   item->momx = (t - P_Random(pr_hdropmom))<<8;
   t = P_Random(pr_hdropmom);
   item->momy = (t - P_Random(pr_hdropmom))<<8;
   item->momz = (P_Random(pr_hdropmom)<<10) + 5*FRACUNIT;

   item->flags |= MF_DROPPED;
}

//
// A_HticDrop
//
// Parameterized code pointer, drops one or two items at random
//
// args[0] -- thing type 1 (0 == none)
// args[1] -- thing type 1 drop chance
// args[2] -- thing type 2 (0 == none)
// args[3] -- thing type 2 drop chance
//
void A_HticDrop(mobj_t *actor)
{
   int thingtype1, thingtype2, chance1, chance2;

   thingtype1 = (int)(actor->state->args[0]);
   chance1    = (int)(actor->state->args[1]);
   thingtype2 = (int)(actor->state->args[2]);
   chance2    = (int)(actor->state->args[3]);

   A_Fall(actor);

   // haleyjd 07/05/03: adjusted for EDF
   if(thingtype1)
   {
      thingtype1 = E_SafeThingType(thingtype1);
      
      if(P_Random(pr_hdrop1) <= chance1)
         P_HticDrop(actor, thingtype1);
   }

   if(thingtype2)
   {
      thingtype2 = E_SafeThingType(thingtype2);

      if(P_Random(pr_hdrop2) <= chance2)
         P_HticDrop(actor, thingtype2);
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
// args[0]: threshold in angles
// args[1]: maxturn in angles
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


// haleyjd 11/19/02:
// Teleport spots for D'Sparil -- these are more or less identical
// to boss spots used for DOOM II MAP30, and I have generalized
// the code that looks for them so it can be used here (and elsewhere
// as needed). This automatically removes the Heretic boss spot
// limit, of course.

int numsorcspots;
int numsorcspots_alloc;
mobj_t **sorcspots;

void P_SpawnSorcSpots(void)
{
   static int spotType = -1;
   
   if(spotType == -1)
      spotType = E_ThingNumForDEHNum(MT_DSPARILSPOT);

   numsorcspots = 0;

   if(spotType == NUMMOBJTYPES)
      return;

   P_CollectThings(spotType, &numsorcspots, 
                   &numsorcspots_alloc, &sorcspots);
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
   int spotnum;
   mobj_t *boss, *mo, *targ;
   fixed_t prevx, prevy, prevz;

   if(!bt->spotCount)
      return;

   spotnum = P_Random(bt->rngNum) % bt->spotCount;
   targ = (bt->spots)[spotnum];

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
      int chance = (int)(256.0f * (1.0f - pct) / 2.0f);

      return chance >= 16 ? chance : 16;
   }
   else if(pct > 0.125f)
   {
      return (int)(256.0f * (1.0f - pct) / 2.0f) + 40;
   }
   else
   {
      return 192;
   }
}

void A_Srcr2Decide(mobj_t *actor)
{
   if(!numsorcspots)
      return;

   if(P_Random(pr_sorctele1) < P_SorcTeleportProb(actor))
   {
      bossteleport_t bt;

      bt.spotCount = numsorcspots;    // number of spots
      bt.spots     = sorcspots;       // use sorcspots from above
      bt.rngNum    = pr_sorctele2;    // use this rng
      bt.boss      = actor;           // teleport D'Sparil
      bt.state     = E_SafeState(S_SOR2_TELE1);   
      bt.fxtype    = E_SafeThingType(MT_SOR2TELEFADE);
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
      mo->flags |= (actor->flags & MF_FRIEND);
      
      mo = P_SpawnMissileAngle(actor, sor2fx2Type,
                               actor->angle + ANG45, 
                               FRACUNIT >> 1, z);
      mo->flags |= (actor->flags & MF_FRIEND);
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
   
   for(i = 0; i < 2; i++)
   {
      // haleyjd: remove dependence on order of evaluation
      int rnd1;

      mo = P_SpawnMobj(actor->x, actor->y, actor->z, sparkType);
      
      rnd1 = P_Random(pr_bluespark);
      mo->momx = (rnd1 - P_Random(pr_bluespark)) << 9;
      
      rnd1 = P_Random(pr_bluespark);
      mo->momy = (rnd1 - P_Random(pr_bluespark)) << 9;
      
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
      // remove it immediately
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

   actor->momx = actor->momy = actor->momz = 0;
   P_SetMobjState(actor, mobjinfo[actor->type].deathstate);
   actor->flags &= ~MF_MISSILE;
   
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
   default:
      damage = 128;
      break;
   }

   P_RadiusAttack(actor, actor->target, damage, actor->info->mod);

   if(!comp[comp_terrain] && 
      (actor->z <= actor->floorz + damage*FRACUNIT))
      P_HitFloor(actor);
}

//
// A_HticBossDeath
//
// Heretic boss deaths
//
void A_HticBossDeath(mobj_t *actor)
{
   int flag = 0;
   int flagfield = 0;
   thinker_t *thinker;
   line_t junk;

   if(gamemap != 8)
      return;

   switch(gameepisode)
   {
   case 1:
      if(!(actor->flags2 & MF2_E1M8BOSS))
         return;
      flag = MF2_E1M8BOSS;
      flagfield = 2;
      break;
   case 2:
      if(!(actor->flags2 & MF2_E2M8BOSS))
         return;
      flag = MF2_E2M8BOSS;
      flagfield = 2;
      break;
   case 3:
      if(!(actor->flags2 & MF2_E3M8BOSS))
         return;
      flag = MF2_E3M8BOSS;
      flagfield = 2;
      break;
   case 4:
      if(!(actor->flags2 & MF2_E4M8BOSS))
         return;
      flag = MF2_E4M8BOSS;
      flagfield = 2;
      break;
   case 5:
      if(!(actor->flags3 & MF3_E5M8BOSS))
         return;
      flag = MF3_E5M8BOSS;
      flagfield = 3;
      break;
   }

   for(thinker = thinkercap.next; thinker != &thinkercap; thinker = thinker->next)
   {
      mobj_t *mo;

      if(thinker->function != P_MobjThinker)
         continue;

      mo = (mobj_t *)thinker;

      if(mo != actor && mo->health > 0)
      {
         if(flagfield == 2)
         {
            if(!(mo->flags2 & flag))
               continue;
         }
         else if(flagfield == 3)
         {
            if(!(mo->flags3 & flag))
               continue;
         }

         return;
      }
   }

   if(gameepisode > 1)
   {
      // if a friendly boss dies, kill only friends
      // if an enemy boss dies, kill only enemies
      P_Massacre((actor->flags & MF_FRIEND) ? 1 : 2);
   }

   junk.tag = 666;
   EV_DoFloor(&junk, lowerFloor);
}

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
      int rnd1;

      goo = P_SpawnMobj(actor->x, actor->y,
                        actor->z + 48*FRACUNIT, gooType);
      P_SetTarget(&goo->target, actor);
      
      rnd1 = P_Random(pr_podpain);
      goo->momx = (rnd1 - P_Random(pr_podpain)) << 9;
      
      rnd1 = P_Random(pr_podpain);
      goo->momy = (rnd1 - P_Random(pr_podpain)) << 9;

      goo->momz = (FRACUNIT>>1) + (P_Random(pr_podpain) << 9);
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

#define MAXGENPODS 16

void A_MakePod(mobj_t *actor)
{
   angle_t angle;
   fixed_t move;
   mobj_t *mo;
   fixed_t x;
   fixed_t y;
   fixed_t z;

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
   angle >>= ANGLETOFINESHIFT;
   move = 9*FRACUNIT/2;

   mo->momx += FixedMul(move, finecosine[angle]);
   mo->momy += FixedMul(move, finesine[angle]);
   
   // use tracer field to link pod to generator, and increment
   // generator's pod count
   P_SetTarget(&mo->tracer, actor);
   actor->special1++;
}

//
// A_KnightAttack
//
void A_KnightAttack(mobj_t *actor)
{
   static int ghostType = -1;
   static int axeType = -1;
   static int redAxeType = -1;

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
      return;
   }

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

void A_DripBlood(mobj_t *actor)
{
   mobj_t *mo;   
   fixed_t x, y;
   int rnd1;

   rnd1 = P_Random(pr_dripblood);
   x = actor->x + ((rnd1 - P_Random(pr_dripblood)) << 11);
   rnd1 = P_Random(pr_dripblood);
   y = actor->y + ((rnd1 - P_Random(pr_dripblood)) << 11);

   mo = P_SpawnMobj(x, y, actor->z, E_SafeThingType(MT_HTICBLOOD));
   
   rnd1 = P_Random(pr_dripblood);
   mo->momx = (rnd1 - P_Random(pr_dripblood)) << 10;
   rnd1 = P_Random(pr_dripblood);
   mo->momy = (rnd1 - P_Random(pr_dripblood)) << 10;

   mo->flags2 |= MF2_LOGRAV;
}

void A_BeastAttack(mobj_t *actor)
{
   if(!actor->target)
      return;

   S_StartSound(actor, actor->info->attacksound);
   
   if(P_CheckMeleeRange(actor))
   {
      int dmg = ((P_Random(pr_beastbite) & 7) + 1) * 3;
      P_DamageMobj(actor->target, actor, actor, dmg, MOD_HIT);
      return;
   }

   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_BEASTBALL),
                  actor->z + DEFAULTMISSILEZ);
}

void A_BeastPuff(mobj_t *actor)
{
   if(P_Random(pr_puffy) > 64)
   {
      int rnd1;
      fixed_t x, y, z;
      mobj_t *mo;

      rnd1 = P_Random(pr_puffy);
      x = actor->x + ((rnd1 - P_Random(pr_puffy)) << 10);
      rnd1 = P_Random(pr_puffy);
      y = actor->y + ((rnd1 - P_Random(pr_puffy)) << 10);
      rnd1 = P_Random(pr_puffy);
      z = actor->z + ((rnd1 - P_Random(pr_puffy)) << 10);

      mo = P_SpawnMobj(x, y, z, E_SafeThingType(MT_PUFFY));

      mo->momx = -(actor->momx / 16);
      mo->momy = -(actor->momy / 16);

      // pass on the beast so that it doesn't hurt itself
      P_SetTarget(&mo->target, actor->target);
   }
}

void A_SnakeAttack(mobj_t *actor)
{
   if(!actor->target)
   {
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
      P_SetMobjState(actor, actor->info->seestate);
      return;
   }

   S_StartSound(actor, actor->info->attacksound);
   A_FaceTarget(actor);
   P_SpawnMissile(actor, actor->target, E_SafeThingType(MT_SNAKEPRO_B),
                  actor->z + DEFAULTMISSILEZ);
}

//
// other parameterized pointers
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
            int t = P_Random(pr_monmisfire);
            int aimshift = ((accurate == 4) ? 20 : 18);
            angle += (t - P_Random(pr_monmisfire)) << aimshift;
         }

         P_LineAttack(actor, angle, MISSILERANGE, slope, dmg);
      }
      else if(accurate == 3) // ssg spread
      {
         int t = P_Random(pr_monmisfire);
         angle += (t - P_Random(pr_monmisfire))<<19;
         t = P_Random(pr_monmisfire);
         t -= P_Random(pr_monmisfire);

         P_LineAttack(actor, angle, MISSILERANGE, slope + (t<<5), dmg);
      }
   }
}

// Codepointer check types

enum
{
   CPC_LESS,
   CPC_LESSOREQUAL,
   CPC_GREATER,
   CPC_GREATEROREQUAL,
   CPC_EQUAL,
   CPC_NOTEQUAL,
   CPC_BITWISEAND,
};

//
// A_HealthJump
//
// Parameterized codepointer for branching based on comparisons
// against a thing's health.
//
// args[0] : state number
// args[1] : comparison type
// args[2] : health value
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

// EOF

