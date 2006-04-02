// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//      Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_mobj.c,v 1.26 1998/05/16 00:24:12 phares Exp $";

#include "doomdef.h"
#include "doomstat.h"
#include "m_random.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_tick.h"
#include "sounds.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "info.h"
#include "g_game.h"
#include "p_chase.h"
#include "p_inter.h"
#include "p_spec.h" // haleyjd 04/05/99: TerrainTypes
#include "p_partcl.h"
#include "in_lude.h"
#include "d_gi.h"
#include "p_user.h" 
#include "g_dmflag.h"
#include "e_states.h"
#include "e_things.h"
#include "e_ttypes.h"
#include "e_exdata.h"
#include "a_small.h"
#include "d_dehtbl.h"
#include "p_info.h"

void P_FallingDamage(player_t *);

fixed_t FloatBobOffsets[64] = // haleyjd 04/30/99: FloatBob
{
   0, 51389, 102283, 152192,
   200636, 247147, 291278, 332604,
   370727, 405280, 435929, 462380,
   484378, 501712, 514213, 521763,
   524287, 521763, 514213, 501712,
   484378, 462380, 435929, 405280,
   370727, 332604, 291278, 247147,
   200636, 152192, 102283, 51389,
   -1, -51390, -102284, -152193,
   -200637, -247148, -291279, -332605,
   -370728, -405281, -435930, -462381,
   -484380, -501713, -514215, -521764,
   -524288, -521764, -514214, -501713,
   -484379, -462381, -435930, -405280,
   -370728, -332605, -291279, -247148,
   -200637, -152193, -102284, -51389
};

//
// P_SetMobjState
// Returns true if the mobj is still present.
//

boolean P_SetMobjState(mobj_t* mobj,statenum_t state)
{
   state_t*  st;

   // haleyjd 07/05/03: adjusted for EDF
   // killough 4/9/98: remember states seen, to detect cycles:
   //static statenum_t seenstate_tab[NUMSTATES]; // fast transition table      
   //statenum_t *seenstate = seenstate_tab;      // pointer to table

   static statenum_t *seenstate_tab;           // fast transition table
   statenum_t *seenstate;                      // pointer to table

   static int recursion;                       // detects recursion
   statenum_t i = state;                       // initial state
   boolean ret = true;                         // return value

   //statenum_t tempstate[NUMSTATES];            // for use with recursion

   // EDF FIXME: may be too slow, is there another solution?
   // for use with recursion
   statenum_t *tempstate = malloc(sizeof(statenum_t)*NUMSTATES);

   // EDF FIXME: might should to move to an initialization function
   if(!seenstate_tab)
   {
      seenstate_tab = Z_Malloc(sizeof(statenum_t)*NUMSTATES,PU_STATIC,NULL);
      memset(seenstate_tab, 0, sizeof(statenum_t)*NUMSTATES);
   }
   seenstate = seenstate_tab;

   // haleyjd: this doesn't work with dynamically allocated tempstate
   //if(recursion++)                             // if recursion detected,
   //   memset(seenstate=tempstate,0,sizeof tempstate); // clear state table

   // if recursion detected, clear state table
   if(recursion++)
      memset(seenstate=tempstate, 0, sizeof(statenum_t)*NUMSTATES);
   
   do
   {
      if(state == NullStateNum)
      {
         mobj->state = NULL;
         P_RemoveMobj(mobj);
         ret = false;
         break;                 // killough 4/9/98
      }
      
      st = states + state;
      mobj->state = st;
      mobj->tics = st->tics;
      
      // sf: skins
      mobj->sprite = (mobj->skin ? mobj->skin->sprite : st->sprite);
      
      mobj->frame = st->frame;
      
      // Modified handling.
      // Call action functions when the state is set
      
      if(st->action)
         st->action(mobj);
      
      // haleyjd 05/20/02: run particle events
      if(drawparticles && st->particle_evt)
         P_RunEvent(mobj);
      
      seenstate[state] = 1 + st->nextstate;   // killough 4/9/98
      
      state = st->nextstate;
   } 
   while(!mobj->tics && !seenstate[state]);   // killough 4/9/98
   
   if(ret && !mobj->tics)  // killough 4/9/98: detect state cycles
      doom_printf(FC_ERROR"Warning: State Cycle Detected");
   
   if(!--recursion)
   {
      for(;(state=seenstate[i]);i=state-1)
         seenstate[i] = 0;  // killough 4/9/98: erase memory of states
   }

   // haleyjd: free temporary state table (see notes above)
   free(tempstate);
   
   return ret;
}

//
// P_ExplodeMissile
//

void P_ExplodeMissile(mobj_t *mo)
{
   // haleyjd 08/02/04: EXPLOCOUNT flag
   if(mo->flags3 & MF3_EXPLOCOUNT)
   {
      if(++mo->special2 < mo->special3)
         return;
   }

   mo->momx = mo->momy = mo->momz = 0;
   
   // haleyjd: an attempt at fixing explosions on skies (works!)
   if(demo_version >= 329)
   {
      if((mo->subsector->sector->ceilingpic == skyflatnum ||
         mo->subsector->sector->ceilingpic == sky2flatnum) &&
         mo->z >= mo->subsector->sector->ceilingheight-P_ThingInfoHeight(mo->info))
      {
         P_RemoveMobj(mo); // don't explode on the actual sky itself
         return;
      }
   }
   
   P_SetMobjState(mo, mobjinfo[mo->type].deathstate);
   
   if(gameModeInfo->type == Game_DOOM)
   {
      mo->tics -= P_Random(pr_explode) & 3;
   
      if(mo->tics < 1)
         mo->tics = 1;
   }
   
   mo->flags &= ~MF_MISSILE;
   
   S_StartSound(mo, mo->info->deathsound);
   
   // haleyjd: disable any particle effects
   mo->effects = 0;
}

void P_ThrustMobj(mobj_t *mo, angle_t angle, fixed_t move)
{
   angle >>= ANGLETOFINESHIFT;
   mo->momx += FixedMul(move, finecosine[angle]);
   mo->momy += FixedMul(move, finesine[angle]);
}

//
// P_XYMovement
//
// Attempts to move something if it has momentum.
//
// killough 11/98: minor restructuring

void P_XYMovement (mobj_t* mo)
{
   player_t *player;
   fixed_t xmove, ymove;

   if(!(mo->momx | mo->momy)) // Any momentum?
   {
      if(mo->flags & MF_SKULLFLY)
      {
         // the skull slammed into something
         
         mo->flags &= ~MF_SKULLFLY;
         mo->momz = 0;
         
         P_SetMobjState(mo, mo->info->spawnstate);
      }

      // haleyjd 08/16/04: crashstate needs to be entered here
      // as well
      if(mo->info->crashstate && (mo->flags & MF_CORPSE) &&
         !(mo->intflags & MIF_CRASHED) &&
         mo->z <= mo->floorz)
      {
         mo->intflags |= MIF_CRASHED;
         P_SetMobjState(mo, mo->info->crashstate);
      }

      return;
   }

   // haleyjd 03/12/03: Heretic Wind transfer specials
   if(demo_version >= 331 && (mo->flags3 & MF3_WINDTHRUST) &&
      !(mo->flags & MF_NOCLIP))
   {
      sector_t *sec = mo->subsector->sector;

      if(sec->hticPushType >= 40 && sec->hticPushType <= 51)
         P_ThrustMobj(mo, sec->hticPushAngle, sec->hticPushForce);
   }

   player = mo->player;
   
   if(mo->momx > MAXMOVE)
      mo->momx = MAXMOVE;
   else if(mo->momx < -MAXMOVE)
      mo->momx = -MAXMOVE;
   
   if(mo->momy > MAXMOVE)
      mo->momy = MAXMOVE;
   else if(mo->momy < -MAXMOVE)
      mo->momy = -MAXMOVE;
   
   xmove = mo->momx;
   ymove = mo->momy;

   do
   {
      fixed_t ptryx, ptryy;

      // killough 8/9/98: fix bug in original Doom source:
      // Large negative displacements were never considered.
      // This explains the tendency for Mancubus fireballs
      // to pass through walls.

      if(xmove > MAXMOVE/2 || ymove > MAXMOVE/2 ||  // killough 8/9/98:
         ((xmove < -MAXMOVE/2 || ymove < -MAXMOVE/2) && demo_version >= 203))
      {
         ptryx = mo->x + xmove/2;
         ptryy = mo->y + ymove/2;
         xmove >>= 1;
         ymove >>= 1;
      }
      else
      {
         ptryx = mo->x + xmove;
         ptryy = mo->y + ymove;
         xmove = ymove = 0;
      }

      // killough 3/15/98: Allow objects to drop off
      
      if (!P_TryMove(mo, ptryx, ptryy, true))
      {
         // blocked move
         
         // killough 8/11/98: bouncing off walls
         // killough 10/98:
         // Add ability for objects other than players to bounce on ice
          
         if(!(mo->flags & MF_MISSILE) && demo_version >= 203 &&
            (mo->flags & MF_BOUNCES || 
             (!player && blockline &&
              variable_friction && mo->z <= mo->floorz &&
              P_GetFriction(mo, NULL) > ORIG_FRICTION)))
         {
            if (blockline)
            {
               fixed_t r = ((blockline->dx >> FRACBITS) * mo->momx +
                            (blockline->dy >> FRACBITS) * mo->momy) /
                    ((blockline->dx >> FRACBITS)*(blockline->dx >> FRACBITS)+
                     (blockline->dy >> FRACBITS)*(blockline->dy >> FRACBITS));
               fixed_t x = FixedMul(r, blockline->dx);
               fixed_t y = FixedMul(r, blockline->dy);

               // reflect momentum away from wall
               
               mo->momx = x*2 - mo->momx;
               mo->momy = y*2 - mo->momy;

               // if under gravity, slow down in
               // direction perpendicular to wall.
               
               if (!(mo->flags & MF_NOGRAVITY))
               {
                  mo->momx = (mo->momx + x)/2;
                  mo->momy = (mo->momy + y)/2;
               }
            }
            else
            {
               mo->momx = mo->momy = 0;
            }
         }
         else if(mo->flags3 & MF3_SLIDE) // haleyjd: SLIDE flag
         {
            P_SlideMove(mo); // try to slide along it
         }
         else if(mo->flags & MF_MISSILE)
         {
            // haleyjd 1/17/00: feel the might of reflection!
            if(demo_version >= 329 && BlockingMobj && 
               (BlockingMobj->flags2 & MF2_REFLECTIVE))
            {
               angle_t refangle = 
                  R_PointToAngle2(BlockingMobj->x, BlockingMobj->y, 
                                  mo->x, mo->y);

               // Change angle for reflection
               if(BlockingMobj->flags2 & MF2_DEFLECTIVE)
               {
                  // deflect it fully
                  if(P_Random(pr_reflect) < 128)
                     refangle += ANG45;
                  else
                     refangle -= ANG45;
               }
               else
                  refangle += ANGLE_1 * ((P_Random(pr_reflect)%16)-8);
                      
               // Reflect the missile along angle
               mo->angle = refangle;
               refangle >>= ANGLETOFINESHIFT;
               mo->momx = FixedMul(mo->info->speed>>1, finecosine[refangle]);
               mo->momy = FixedMul(mo->info->speed>>1, finesine[refangle]);

               if(mo->flags2 & MF2_SEEKERMISSILE)
               {
                  // send it back after the SOB who fired it
                  if(mo->tracer)
                     P_SetTarget(&mo->tracer, mo->target);
               }
               
               P_SetTarget(&mo->target, BlockingMobj);
               return;
            }
            // explode a missile
            
            if(ceilingline && ceilingline->backsector &&
               (ceilingline->backsector->ceilingpic == skyflatnum ||
                ceilingline->backsector->ceilingpic == sky2flatnum))
            {
               if (demo_compatibility ||  // killough
                  mo->z > ceilingline->backsector->ceilingheight)
               {
                  // Hack to prevent missiles exploding
                  // against the sky.
                  // Does not handle sky floors.
                  
                  // haleyjd: in fact, doesn't handle sky
                  // ceilings either -- this fix is for "sky
                  // hack walls" only apparently -- see 
                  // P_ExplodeMissile for my real sky fix
                  
                  P_RemoveMobj(mo);
                  return;
               }
            }
            
            P_ExplodeMissile(mo);
         }
         else // whatever else it is, it is now standing still in (x,y)
         {
            mo->momx = mo->momy = 0;
         }
      }
   }
   while (xmove | ymove);

   // slow down

#if 0  // killough 10/98: this is unused code (except maybe in .deh files?)
   if(player && player->mo == mo && player->cheats & CF_NOMOMENTUM)
   {
      // debug option for no sliding at all
      mo->momx = mo->momy = 0;
      player->momx = player->momy = 0;         // killough 10/98
      return;
   }
#endif

   // no friction for missiles or skulls ever, no friction when airborne
   if(mo->flags & (MF_MISSILE | MF_SKULLFLY) || mo->z > mo->floorz)
      return;

   // killough 8/11/98: add bouncers
   // killough 9/15/98: add objects falling off ledges
   // killough 11/98: only include bouncers hanging off ledges
   if(((mo->flags & MF_BOUNCES && mo->z > mo->dropoffz) ||
       mo->flags & MF_CORPSE || mo->intflags & MIF_FALLING) &&
      (mo->momx > FRACUNIT/4 || mo->momx < -FRACUNIT/4 ||
       mo->momy > FRACUNIT/4 || mo->momy < -FRACUNIT/4) &&
      mo->floorz != mo->subsector->sector->floorheight)
      return;  // do not stop sliding if halfway off a step with some momentum

   // killough 11/98:
   // Stop voodoo dolls that have come to rest, despite any
   // moving corresponding player, except in old demos:

   if(mo->momx > -STOPSPEED && mo->momx < STOPSPEED &&
      mo->momy > -STOPSPEED && mo->momy < STOPSPEED &&
      (!player || !(player->cmd.forwardmove | player->cmd.sidemove) ||
      (player->mo != mo && demo_version >= 203)))
   {
      // if in a walking frame, stop moving

      // haleyjd 07/25/03: FIXME: this is ugly.      
      // killough 10/98:
      // Don't affect main player when voodoo dolls stop, except in old demos:
      
      if(player && (unsigned)(player->mo->state - states - E_SafeState(S_PLAY_RUN1)) < 4 
         && (player->mo == mo || demo_version < 203))
         P_SetMobjState(player->mo, E_SafeState(S_PLAY));

      mo->momx = mo->momy = 0;
      
      // killough 10/98: kill any bobbing momentum too (except in voodoo dolls)
      if (player && player->mo == mo)
         player->momx = player->momy = 0;
   }
   else
   {
      // phares 3/17/98
      //
      // Friction will have been adjusted by friction thinkers for
      // icy or muddy floors. Otherwise it was never touched and
      // remained set at ORIG_FRICTION
      //
      // killough 8/28/98: removed inefficient thinker algorithm,
      // instead using touching_sectorlist in P_GetFriction() to
      // determine friction (and thus only when it is needed).
      //
      // killough 10/98: changed to work with new bobbing method.
      // Reducing player momentum is no longer needed to reduce
      // bobbing, so ice works much better now.

      fixed_t friction = P_GetFriction(mo, NULL);

      mo->momx = FixedMul(mo->momx, friction);
      mo->momy = FixedMul(mo->momy, friction);

      // killough 10/98: Always decrease player bobbing by ORIG_FRICTION.
      // This prevents problems with bobbing on ice, where it was not being
      // reduced fast enough, leading to all sorts of kludges being developed.

      if(player && player->mo == mo)     //  Not voodoo dolls
      {
         player->momx = FixedMul(player->momx, ORIG_FRICTION);
         player->momy = FixedMul(player->momy, ORIG_FRICTION);
      }
   }
}

//
// P_ZMovement
//
// Attempt vertical movement.

static void P_ZMovement(mobj_t* mo)
{
   // haleyjd: part of lost soul fix, moved up here for maximum
   //          scope
   boolean correct_lost_soul_bounce;
   boolean moving_down;

   if(demo_compatibility) // v1.9 demos
      correct_lost_soul_bounce = (gamemission != doom2);
   else if(demo_version < 331) // BOOM - EE v3.29
      correct_lost_soul_bounce = true;
   else // from now on...
      correct_lost_soul_bounce = !comp[comp_soul];

#ifdef OVER_UNDER
   // haleyjd: 04/01/03: Saving SoM's ass :)
   if(demo_version >= 331 && !comp[comp_overunder])
   {
      fixed_t oldz;

      oldz = mo->z;
      mo->z += mo->momz;
      P_CheckPositionMobjOnly(mo, mo->x, mo->y);
      if(tmfloorz > mo->floorz) // must update if new values
         mo->floorz = tmfloorz;
      if(tmceilingz < mo->ceilingz)
         mo->ceilingz = tmceilingz;
      mo->z = oldz;
   }
#endif   

   // killough 7/11/98:
   // BFG fireballs bounced on floors and ceilings in Pre-Beta Doom
   // killough 8/9/98: added support for non-missile objects bouncing
   // (e.g. grenade, mine, pipebomb)

   if(mo->flags & MF_BOUNCES && mo->momz)
   {
      mo->z += mo->momz;
            
      if (mo->z <= mo->floorz)                  // bounce off floors
      {
         mo->z = mo->floorz;
         E_HitFloor(mo); // haleyjd
         if (mo->momz < 0)
         {
            mo->momz = -mo->momz;
            if (!(mo->flags & MF_NOGRAVITY))  // bounce back with decay
            {
               mo->momz = mo->flags & MF_FLOAT ?   // floaters fall slowly
                  mo->flags & MF_DROPOFF ?          // DROPOFF indicates rate
                     FixedMul(mo->momz, (fixed_t)(FRACUNIT*.85)) :
                     FixedMul(mo->momz, (fixed_t)(FRACUNIT*.70)) :
                     FixedMul(mo->momz, (fixed_t)(FRACUNIT*.45)) ;
                  
               // Bring it to rest below a certain speed
               if(D_abs(mo->momz) <= mo->info->mass*(LevelInfo.gravity*4/256))
                  mo->momz = 0;
            }

            // killough 11/98: touchy objects explode on impact
            if (mo->flags & MF_TOUCHY && mo->intflags & MIF_ARMED &&
               mo->health > 0)
               P_DamageMobj(mo, NULL, NULL, mo->health,MOD_UNKNOWN);
            else if (mo->flags & MF_FLOAT && sentient(mo))
               goto floater;
            return;
         }
      }
      else if(mo->z >= mo->ceilingz - mo->height) // bounce off ceilings
      {
         mo->z = mo->ceilingz - mo->height;
         if(mo->momz > 0)
         {
            if(mo->subsector->sector->ceilingpic != skyflatnum &&
               mo->subsector->sector->ceilingpic != sky2flatnum)
               mo->momz = -mo->momz;    // always bounce off non-sky ceiling
            else
            {
               if(mo->flags & MF_MISSILE)
               {
                  P_RemoveMobj(mo);      // missiles don't bounce off skies
                  if(demo_version >= 331)
                     return; // haleyjd: return here for below fix
               }
               else if(mo->flags & MF_NOGRAVITY)
                  mo->momz = -mo->momz; // bounce unless under gravity
            }

            if (mo->flags & MF_FLOAT && sentient(mo))
               goto floater;

            // haleyjd 05/22/03: kludge for bouncers sticking to sky --
            // if momz is still positive, the thing is still trying
            // to go up, which it should not be doing, so apply some
            // gravity to get it out of this code segment gradually
            if(demo_version >= 331 && mo->momz > 0)
            {
               mo->momz -= mo->info->mass*(LevelInfo.gravity/256);
            }
            
            return;
         }
      }
      else
      {
         if(!(mo->flags & MF_NOGRAVITY))      // free-fall under gravity
            mo->momz -= mo->info->mass*(LevelInfo.gravity/256);
         if(mo->flags & MF_FLOAT && sentient(mo))
            goto floater;
         return;
      }

      // came to a stop
      mo->momz = 0;
      
      if (mo->flags & MF_MISSILE)
      {
         if(ceilingline &&
            ceilingline->backsector &&
            (mo->z > ceilingline->backsector->ceilingheight) &&
            (ceilingline->backsector->ceilingpic == skyflatnum ||
             ceilingline->backsector->ceilingpic == sky2flatnum))
         {
            P_RemoveMobj(mo);  // don't explode on skies
         }
         else
         {
            P_ExplodeMissile(mo);
         }
      }
      
      if (mo->flags & MF_FLOAT && sentient(mo))
         goto floater;
      return;
   }

   // killough 8/9/98: end bouncing object code
   
   // check for smooth step up

   if(mo->player &&
      mo->player->mo == mo &&  // killough 5/12/98: exclude voodoo dolls
      mo->z < mo->floorz)
   {
      mo->player->viewheight -= mo->floorz-mo->z;
      mo->player->deltaviewheight = 
         (VIEWHEIGHT - mo->player->viewheight)>>3;
   }

   // adjust altitude
   mo->z += mo->momz;
   
floater:

   // float down towards target if too close
   
   if(!((mo->flags ^ MF_FLOAT) & (MF_FLOAT | MF_SKULLFLY | MF_INFLOAT)) &&
      mo->target)     // killough 11/98: simplify
   {
      fixed_t delta;
      if(P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y) <
         D_abs(delta = mo->target->z + (mo->height>>1) - mo->z)*3)
         mo->z += delta < 0 ? -FLOATSPEED : FLOATSPEED;
   }

   // clip movement
   
   if (mo->z <= mo->floorz)
   {
      // hit the floor
      
      /*
         A Mystery Unravelled - Discovered by cph -- haleyjd
      
         The code below was once below the point where mo->momz
         was set to zero. Someone added a comment and moved this 
         code here.
      */
      if(correct_lost_soul_bounce && (mo->flags & MF_SKULLFLY))
         mo->momz = -mo->momz; // the skull slammed into something

      if((moving_down = mo->momz < 0))
      {
         // killough 11/98: touchy objects explode on impact
         if(mo->flags & MF_TOUCHY && mo->intflags & MIF_ARMED &&
            mo->health > 0)
            P_DamageMobj(mo, NULL, NULL, mo->health, MOD_UNKNOWN);
         else if (mo->player && // killough 5/12/98: exclude voodoo dolls
                  mo->player->mo == mo &&
                  mo->momz < -LevelInfo.gravity*8)
         {
            // Squat down.
            // Decrease viewheight for a moment
            // after hitting the ground (hard),
            // and utter appropriate sound.
            
            mo->player->deltaviewheight = mo->momz >> 3;
            
            // haleyjd 05/09/99 no oof when dead :)
            if(demo_version < 329 || mo->health > 0)
            {
               if(!comp[comp_fallingdmg] && demo_version >= 329)
               {
                  // haleyjd: new features -- feet sound for normal hits,
                  //          grunt for harder, falling damage for worse
                  
                  if(mo->momz < -23*FRACUNIT)
                  {
                     if(!mo->player->powers[pw_invulnerability] &&
                        !(players[consoleplayer].cheats & CF_GODMODE))
                        P_FallingDamage(mo->player);
                     else
                        S_StartSound(mo, sfx_oof);
                  }
                  else if(mo->momz < -12*FRACUNIT)
                     S_StartSound(mo, sfx_oof);
                  else if(!E_GetThingFloorType(mo)->liquid)
                     S_StartSound(mo, sfx_plfeet);
               }
               else if(!E_GetThingFloorType(mo)->liquid)
                  S_StartSound(mo, sfx_oof);    
            }
         }
         mo->momz = 0;
      }

      mo->z = mo->floorz;

      if(moving_down)
         E_HitFloor(mo);

      /* cph 2001/05/26 -
       * See lost soul bouncing comment above. We need this here for
       * bug compatibility with original Doom2 v1.9 - if a soul is
       * charging and is hit by a raising floor this incorrectly 
       * reverses its Z momentum.
       */
      if(!correct_lost_soul_bounce && (mo->flags & MF_SKULLFLY))
         mo->momz = -mo->momz;

      // haleyjd 08/07/04: crashstate
      if(mo->info->crashstate && (mo->flags & MF_CORPSE) &&
         !(mo->intflags & MIF_CRASHED))
      {
         mo->intflags |= MIF_CRASHED;
         P_SetMobjState(mo, mo->info->crashstate);
      }

      if(!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
      {
         if(!(mo->flags3 & MF3_FLOORMISSILE)) // haleyjd
            P_ExplodeMissile(mo);
         return;
      }
   }
   else if(mo->flags2 & MF2_LOGRAV) // haleyjd 04/09/99
   {
      if(!mo->momz)
         mo->momz = -(LevelInfo.gravity >> 3) * 2;
      else
         mo->momz -= LevelInfo.gravity >> 3;
   }
   else // still above the floor
   {
      if(!(mo->flags & MF_NOGRAVITY))
      {
         if(!mo->momz)
            mo->momz = -LevelInfo.gravity;
         mo->momz -= LevelInfo.gravity;
      }
   }

   // haleyjd 08/07/04: new footclip system   
   P_AdjustFloorClip(mo);

   if(mo->z + mo->height > mo->ceilingz)
   {
      // hit the ceiling

      if(mo->momz > 0)
         mo->momz = 0;
      
      mo->z = mo->ceilingz - mo->height;
      
      if(mo->flags & MF_SKULLFLY)
         mo->momz = -mo->momz; // the skull slammed into something
      
      if(!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
      {
         P_ExplodeMissile (mo);
         return;
      }
   }
}

//
// P_NightmareRespawn
//

void P_NightmareRespawn(mobj_t* mobj)
{
   fixed_t      x;
   fixed_t      y;
   fixed_t      z;
   subsector_t* ss;
   mobj_t*      mo;
   mapthing_t*  mthing;
   boolean      check; // haleyjd 11/11/04
   
   x = mobj->spawnpoint.x << FRACBITS;
   y = mobj->spawnpoint.y << FRACBITS;

   // haleyjd: stupid nightmare respawning bug fix
   //
   // 08/09/00: This fixes the notorious nightmare respawning bug that 
   // causes monsters that didn't spawn at level startup to respawn at 
   // the point (0,0) regardless of that point's nature.
  
   if(!comp[comp_respawnfix] && demo_version >= 329 && x == 0 && y == 0)
   {
      // spawnpoint was zeroed out, so use point of death instead
      x = mobj->x;
      y = mobj->y;
   }

   // something is occupying its position?

   // haleyjd 11/11/04: This has been broken since BOOM when Lee changed
   // PIT_CheckThing to allow non-solid things to move through solid
   // things.

   if(demo_version >= 331)
      mobj->flags |= MF_SOLID;
   
   check = P_CheckPosition(mobj, x, y);

   if(demo_version >= 331)
      mobj->flags &= ~MF_SOLID;

   if(!check)
      return; // no respawn

   // spawn a teleport fog at old spot
   // because of removal of the body?
   
   mo = P_SpawnMobj(mobj->x, mobj->y,
                    mobj->subsector->sector->floorheight +
                       gameModeInfo->teleFogHeight, 
                    gameModeInfo->teleFogType);

   // initiate teleport sound
   
   S_StartSound(mo, gameModeInfo->teleSound);
   
   // spawn a teleport fog at the new spot
   
   ss = R_PointInSubsector(x,y);
   
   mo = P_SpawnMobj(x, y, 
                    ss->sector->floorheight + gameModeInfo->teleFogHeight, 
                    gameModeInfo->teleFogType);
   
   S_StartSound(mo, gameModeInfo->teleSound);

   // spawn the new monster
   
   mthing = &mobj->spawnpoint;
   z = mobj->info->flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

   // inherit attributes from deceased one
   
   mo = P_SpawnMobj(x, y, z, mobj->type);
   mo->spawnpoint = mobj->spawnpoint;
   // sf: use R_WadToAngle
   mo->angle = R_WadToAngle(mthing->angle);

   if(mthing->options & MTF_AMBUSH)
      mo->flags |= MF_AMBUSH;

   // killough 11/98: transfer friendliness from deceased
   mo->flags = (mo->flags & ~MF_FRIEND) | (mobj->flags & MF_FRIEND);
   
   mo->reactiontime = 18;
   
   // remove the old monster,
   
   P_RemoveMobj(mobj);
}

//
// P_MobjThinker
//

void P_MobjThinker(mobj_t *mobj)
{
   int oldwaterstate, waterstate;

   // killough 11/98: 
   // removed old code which looked at target references
   // (we use pointer reference counting now)
   
   // haleyjd: sentient things do not think during cinemas
   if(cinema_pause && sentient(mobj))
      return;

   BlockingMobj = NULL; // haleyjd 1/17/00: global hit reference

   // haleyjd 08/07/04: handle deep water plane hits
   if(mobj->subsector->sector->heightsec != -1)
   {
      sector_t *hs = &sectors[mobj->subsector->sector->heightsec];

      waterstate = (mobj->z < hs->floorheight);
   }
   else
      waterstate = 0;
   
   // momentum movement
   if(mobj->momx | mobj->momy || mobj->flags & MF_SKULLFLY)
   {
      P_XYMovement(mobj);
      if(mobj->thinker.function == P_RemoveThinkerDelayed) // killough
         return;       // mobj was removed
   }

   if(mobj->flags2&MF2_FLOATBOB) // haleyjd
   {
      mobj->z = mobj->floorz + FloatBobOffsets[(mobj->floatbob++)&63];
   }
   else if(mobj->z != mobj->floorz || mobj->momz)
   {
      P_ZMovement(mobj);
      if (mobj->thinker.function == P_RemoveThinkerDelayed) // killough
         return;       // mobj was removed
   }
   else if(!(mobj->momx | mobj->momy) && !sentient(mobj))
   {                                  // non-sentient objects at rest
      mobj->intflags |= MIF_ARMED;    // arm a mine which has come to rest
      
      // killough 9/12/98: objects fall off ledges if they are hanging off
      // slightly push off of ledge if hanging more than halfway off
      
      if(mobj->z > mobj->dropoffz &&      // Only objects contacting dropoff
         !(mobj->flags & MF_NOGRAVITY) && // Only objects which fall
         !comp[comp_falloff] && demo_version >= 203) // Not in old demos
         P_ApplyTorque(mobj);               // Apply torque
      else
         mobj->intflags &= ~MIF_FALLING, mobj->gear = 0;  // Reset torque
   }

   // haleyjd 08/07/04: handle deep water plane hits
   oldwaterstate = waterstate;

   if(mobj->subsector->sector->heightsec != -1)
   {
      sector_t *hs = &sectors[mobj->subsector->sector->heightsec];

      waterstate = (mobj->z < hs->floorheight);
   }
   else
      waterstate = 0;

   if(mobj->flags & (MF_MISSILE|MF_BOUNCES))
   {
      // any time a missile or bouncer crosses, splash
      if(oldwaterstate != waterstate)
         E_HitWater(mobj, mobj->subsector->sector);
   }
   else if(oldwaterstate == 0 && waterstate != 0)
   {
      // normal things only splash going into the water
      E_HitWater(mobj, mobj->subsector->sector);
   }

   // cycle through states,
   // calling action functions at transitions
   // killough 11/98: simplify
   
   if (mobj->tics != -1)      // you can cycle through multiple states in a tic
   {
      if(!--mobj->tics)
         P_SetMobjState(mobj, mobj->state->nextstate);
   }
   else
   { 
      // haleyjd: minor restructuring, eternity features
      // A thing can respawn if:
      // 1) counts for kill AND
      // 2) respawn is on OR
      // 3) thing always respawns or removes itself after death.
      boolean can_respawn = 
         mobj->flags & MF_COUNTKILL &&                             
           (respawnmonsters ||                                     
            (mobj->flags2 & (MF2_ALWAYSRESPAWN | MF2_REMOVEDEAD)));

      // haleyjd 07/13/05: increment mobj->movecount earlier
      if(can_respawn || mobj->effects & FX_FLIESONDEATH)
         ++mobj->movecount;

      // don't respawn dormant things
      // don't respawn norespawn things
      if(mobj->flags2 & (MF2_DORMANT | MF2_NORESPAWN)) 
         return;
      
      if(can_respawn && mobj->movecount >= 12*35 && 
         !(leveltime & 31) && P_Random(pr_respawn) <= 4)
      { // check for nightmare respawn
         
         if(mobj->flags2 & MF2_REMOVEDEAD)
            P_RemoveMobj(mobj);
         else
            P_NightmareRespawn(mobj);
      }
   }
}


//
// P_SpawnMobj
//

mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
   mobj_t *mobj = Z_Malloc(sizeof *mobj, PU_LEVEL, NULL);
   mobjinfo_t *info = &mobjinfo[type];
   state_t    *st;

   memset(mobj, 0, sizeof *mobj);
   
   mobj->type = type;
   mobj->info = info;
   mobj->x = x;
   mobj->y = y;
   mobj->radius = info->radius;
   mobj->height = P_ThingInfoHeight(info); // phares
   mobj->flags  = info->flags;
   mobj->flags2 = info->flags2; // haleyjd
   mobj->flags3 = info->flags3; // haleyjd   
   mobj->effects = info->particlefx;  // haleyjd 07/13/03
   mobj->damage  = info->damage;      // haleyjd 08/02/04

   // haleyjd 09/26/04: rudimentary support for monster skins
   if(info->altsprite != NUMSPRITES)
      mobj->skin = P_GetMonsterSkin(info->altsprite);
   else
      mobj->skin = NULL;

   // haleyjd: zdoom-style translucency level 
   mobj->translucency = info->translucency;

   if(mobj->translucency != FRACUNIT)
   {
      // zdoom and BOOM style translucencies are mutually
      // exclusive, so unset the BOOM flag if the zdoom level is not 
      // FRACUNIT
      mobj->flags &= ~MF_TRANSLUCENT;
   }

   //sf: not friends in deathmatch!

   // killough 8/23/98: no friends, bouncers, or touchy things in old demos
   if(demo_version < 203)
   {
      mobj->flags &= ~(MF_BOUNCES | MF_FRIEND | MF_TOUCHY); 
   }
   else if(demo_version < 303 || !(GameType == gt_dm))
   {
      if(type == E_ThingNumForDEHNum(MT_PLAYER)) // Except in old demos, players
         mobj->flags |= MF_FRIEND;               // are always friends.
   }

   mobj->health = info->spawnhealth;
   
   if(gameskill != sk_nightmare)
      mobj->reactiontime = info->reactiontime;
   
   mobj->lastlook = P_Random(pr_lastlook) % MAXPLAYERS;

   // do not set the state with P_SetMobjState,
   // because action routines can not be called yet
   
   st = &states[info->spawnstate];
   
   mobj->state  = st;
   mobj->tics   = st->tics;
   mobj->sprite = (mobj->skin ? mobj->skin->sprite : st->sprite);
   mobj->frame  = st->frame;
   
   // NULL head of sector list // phares 3/13/98
   mobj->touching_sectorlist = NULL;

   // set subsector and/or block links
   
   P_SetThingPosition(mobj);
   
   mobj->dropoffz =           // killough 11/98: for tracking dropoffs
      mobj->floorz   = mobj->subsector->sector->floorheight;
   mobj->ceilingz = mobj->subsector->sector->ceilingheight;
  
   mobj->z = z == ONFLOORZ ? mobj->floorz : z == ONCEILINGZ ?
      mobj->ceilingz - mobj->height : z;

   // haleyjd 10/13/02: floatrand
   if(demo_version >= 331 && z == FLOATRANDZ)
   {
      int rnd, minz, maxz;
      
      // special thanks to fraggle for reminding me of how to do this
      rnd  = P_Random(pr_spawnfloat);
      rnd |= P_Random(pr_spawnfloat) << 8;

      minz = (mobj->floorz + MINFLTRNDZ) / FRACUNIT;
      maxz = (mobj->ceilingz - mobj->height) / FRACUNIT;

      if(minz >= maxz)
         mobj->z = mobj->floorz;
      else
         mobj->z = (minz + rnd%(maxz - minz + 1))*FRACUNIT;
   }

#ifdef OVER_UNDER
   // SoM 11/5/02: Set these at spawn.
   mobj->secfloorz = mobj->passfloorz = mobj->floorz;
   mobj->secceilz = mobj->passceilz = mobj->ceilingz;

   if(demo_version >= 331 && !comp[comp_overunder])
   {
      P_ThingMovez(mobj, 0);
   }
#endif

   // haleyjd 08/07/04: new floorclip system
   P_AdjustFloorClip(mobj);

   mobj->thinker.function = P_MobjThinker;

   P_AddThinker(&mobj->thinker);

   // haleyjd 01/12/04: support translation lumps
   if(mobj->info->colour)
      mobj->colour = mobj->info->colour;
   else
      mobj->colour = (info->flags & MF_TRANSLATION) >> MF_TRANSSHIFT;
   
   return mobj;
}

static mapthing_t itemrespawnque[ITEMQUESIZE];
static int itemrespawntime[ITEMQUESIZE];
int iquehead, iquetail;

//
// P_RemoveMobj
//

void P_RemoveMobj (mobj_t *mobj)
{
   // haleyjd 04/14/03: restructured
   boolean respawnitem = false;

   if((mobj->flags3 & MF3_SUPERITEM) && (dmflags & DM_RESPAWNSUPER))
   {
      respawnitem = true; // respawning super powerups
   }
   else if(mobj->type == E_ThingNumForDEHNum(MT_BARREL) &&
           (dmflags & DM_BARRELRESPAWN))
   {
      respawnitem = true; // respawning barrels
   }
   else
   {
      respawnitem = 
         !((mobj->flags ^ MF_SPECIAL) & (MF_SPECIAL | MF_DROPPED)) &&
         !(mobj->flags3 & MF3_NOITEMRESP);
   }

   if(respawnitem)
   {
      // haleyjd FIXME/TODO: spawnpoint is vulnerable to zeroing
      itemrespawnque[iquehead] = mobj->spawnpoint;
      itemrespawntime[iquehead++] = leveltime;
      if((iquehead &= ITEMQUESIZE-1) == iquetail)
         // lose one off the end?
         iquetail = (iquetail+1)&(ITEMQUESIZE-1);
   }

   // haleyjd 02/02/04: remove from tid hash
   P_RemoveThingTID(mobj);

   // unlink from sector and block lists
   
   P_UnsetThingPosition(mobj);
   
   // Delete all nodes on the current sector_list               phares 3/16/98
   if(sector_list)
   {
      P_DelSeclist(sector_list);
      sector_list = NULL;
   }

   // stop any playing sound
   
   S_StopSound(mobj);
   
   // killough 11/98:
   //
   // Remove any references to other mobjs.
   //
   // Older demos might depend on the fields being left alone, however,
   // if multiple thinkers reference each other indirectly before the
   // end of the current tic.

   if(demo_version >= 203)
   {
      P_SetTarget(&mobj->target,    NULL);
      P_SetTarget(&mobj->tracer,    NULL);
      P_SetTarget(&mobj->lastenemy, NULL);
   }
   
   // free block
   
   P_RemoveThinker(&mobj->thinker);
}

//
// P_FindDoomedNum
//
// Finds a mobj type with a matching doomednum
//
// killough 8/24/98: rewrote to use hashing
//

int P_FindDoomedNum(int type)
{
   static struct dnumhash_s { int first, next; } *hash;
   register int i;

   if(!hash)
   {
      hash = Z_Malloc(sizeof(*hash) * NUMMOBJTYPES, PU_CACHE, (void **)&hash);
      for(i = 0; i < NUMMOBJTYPES; ++i)
         hash[i].first = NUMMOBJTYPES;
      for(i = 0; i < NUMMOBJTYPES; ++i)
      {
         if(mobjinfo[i].doomednum != -1)
         {
            unsigned h = (unsigned) mobjinfo[i].doomednum % NUMMOBJTYPES;
            hash[i].next = hash[h].first;
            hash[h].first = i;
         }
      }
   }
  
   i = hash[type % NUMMOBJTYPES].first;
   while(i < NUMMOBJTYPES && mobjinfo[i].doomednum != type)
      i = hash[i].next;
   return i;
}

//
// P_RespawnSpecials
//

void P_RespawnSpecials (void)
{
   fixed_t x, y, z;
   subsector_t*  ss;
   mobj_t*       mo;
   mapthing_t*   mthing;
   int           i;

   if(!(dmflags & DM_ITEMRESPAWN) ||  // only respawn items in deathmatch
      iquehead == iquetail ||  // nothing left to respawn?
      leveltime - itemrespawntime[iquetail] < 30*35) // wait 30 seconds
      return;
  
   mthing = &itemrespawnque[iquetail];
   
   x = mthing->x << FRACBITS;
   y = mthing->y << FRACBITS;

   // spawn a teleport fog at the new spot

   // FIXME: Heretic support?
   
   ss = R_PointInSubsector (x,y);
   mo = P_SpawnMobj(x, y, ss->sector->floorheight , E_SafeThingType(MT_IFOG));
   S_StartSound(mo, sfx_itmbk);

   // find which type to spawn
   
   // killough 8/23/98: use table for faster lookup
   i = P_FindDoomedNum(mthing->type);
   
   // spawn it
   z = mobjinfo[i].flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

   mo = P_SpawnMobj(x,y,z, i);
   mo->spawnpoint = *mthing;
   // sf
   mo->angle = R_WadToAngle(mthing->angle);

   // pull it from the queue
   iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//

void P_SpawnPlayer(mapthing_t* mthing)
{
   player_t* p;
   fixed_t   x, y, z;
   mobj_t*   mobj;
   int       i;
   
   // not playing?
   
   if(!playeringame[mthing->type - 1])
      return;

   p = &players[mthing->type-1];
   
   if(p->playerstate == PST_REBORN)
      G_PlayerReborn(mthing->type - 1);
   
   x    = mthing->x << FRACBITS;
   y    = mthing->y << FRACBITS;
   z    = ONFLOORZ;
   mobj = P_SpawnMobj(x, y, z, E_SafeThingType(MT_PLAYER));

   // sf: set color translations for player sprites   
   mobj->colour = players[mthing->type - 1].colormap;
   
   mobj->angle      = R_WadToAngle(mthing->angle);
   mobj->player     = p;
   mobj->health     = p->health;

   // haleyjd: verify that the player skin is valid
   if(!p->skin)
      I_Error("P_SpawnPlayer: player skin undefined!\n");

   mobj->skin       = p->skin;
   mobj->sprite     = p->skin->sprite;
   
   p->mo            = mobj;
   p->playerstate   = PST_LIVE;
   p->refire        = 0;
   p->damagecount   = 0;
   p->bonuscount    = 0;
   p->extralight    = 0;
   p->fixedcolormap = 0;
   p->viewheight    = VIEWHEIGHT;
   p->viewz         = mobj->z + VIEWHEIGHT;
   
   p->momx = p->momy = 0;   // killough 10/98: initialize bobbing to 0.
   
   // setup gun psprite
   
   P_SetupPsprites(p);

   // give all cards in death match mode
   
   if(GameType == gt_dm)
   {
      for(i = 0 ; i < NUMCARDS ; i++)
         p->cards[i] = true;
   }

   if(mthing->type-1 == consoleplayer)
   {
      ST_Start(); // wake up the status bar
      HU_Start(); // wake up the heads up text
   }

   // sf: wake up chasecam
   if(mthing->type - 1 == displayplayer)
      P_ResetChasecam();
}


//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
// sf: made to return mobj_t* spawned

mobj_t *P_SpawnMapThing(mapthing_t *mthing)
{
   int    i;
   mobj_t *mobj;
   fixed_t x, y, z;

   switch(mthing->type)
   {
   case 0:             // killough 2/26/98: Ignore type-0 things as NOPs
   case DEN_PLAYER5:   // phares 5/14/98: Ignore Player 5-8 starts (for now)
   case DEN_PLAYER6:
   case DEN_PLAYER7:
   case DEN_PLAYER8:
      return NULL;
   case ED_CTRL_DOOMEDNUM:
      // haleyjd 10/08/03: ExtraData mapthing support
      return E_SpawnMapThingExt(mthing);
   }

   // killough 11/98: clear flags unused by Doom
   //
   // We clear the flags unused in Doom if we see flag mask 256 set, since
   // it is reserved to be 0 under the new scheme. A 1 in this reserved bit
   // indicates it's a Doom wad made by a Doom editor which puts 1's in
   // bits that weren't used in Doom (such as HellMaker wads). So we should
   // then simply ignore all upper bits.

   if(demo_compatibility || 
      (demo_version >= 203 && mthing->options & MTF_RESERVED))
      mthing->options &= MTF_EASY|MTF_NORMAL|MTF_HARD|MTF_AMBUSH|MTF_NOTSINGLE;

   // count deathmatch start positions
   
   if(mthing->type == 11)
   {
      // 1/11/98 killough -- new code removes limit on deathmatch starts:
      
      size_t offset = deathmatch_p - deathmatchstarts;
      
      if(offset >= num_deathmatchstarts)
      {
         num_deathmatchstarts = num_deathmatchstarts ?
            num_deathmatchstarts*2 : 16;
         deathmatchstarts = realloc(deathmatchstarts,
                                    num_deathmatchstarts *
                                    sizeof(*deathmatchstarts));
         deathmatch_p = deathmatchstarts + offset;
      }
      memcpy(deathmatch_p++, mthing, sizeof*mthing);
      return NULL; //sf
   }

   if(mthing->type == 5003 && demo_version < 331)
   {
      // haleyjd: now handled through IN_AddCameras
      // return NULL for old demos
      return NULL;
   }

   // check for players specially
   
   if (mthing->type <= 4 && mthing->type > 0) // killough 2/26/98 -- fix crashes
   {
      // killough 7/19/98: Marine's best friend :)
      if(GameType == gt_single && 
         mthing->type > 1 && mthing->type <= dogs+1 &&
         !players[mthing->type-1].secretcount)
      {  
         // use secretcount to avoid multiple dogs in case of multiple starts
         players[mthing->type-1].secretcount = 1;

         // killough 10/98: force it to be a friend
         mthing->options |= MTF_FRIEND;
         
         i = E_SafeThingType(MT_DOGS);
         
         // haleyjd 9/22/99: [HELPER] bex block type substitution
         if(HelperThing != -1)
         {
            // haleyjd 07/05/03: adjusted for EDF
            if(HelperThing != NUMMOBJTYPES)
               i = HelperThing;
            else
               doom_printf(FC_ERROR "Invalid value for helper, ignored.");
         }

         goto spawnit;
      }

      // save spots for respawning in network games
      playerstarts[mthing->type-1] = *mthing;
      if(GameType != gt_dm)
         P_SpawnPlayer(mthing);
      
      return NULL; //sf
   }

   // check for apropriate skill level
   
   //jff "not single" thing flag

   if(GameType == gt_single && (mthing->options & MTF_NOTSINGLE))
      return NULL; //sf

   //jff 3/30/98 implement "not deathmatch" thing flag
   
   if(GameType == gt_dm && (mthing->options & MTF_NOTDM))
      return NULL; //sf

   //jff 3/30/98 implement "not cooperative" thing flag
   
   if(GameType == gt_coop && (mthing->options & MTF_NOTCOOP))
      return NULL;  // sf

   // killough 11/98: simplify
   if(gameskill == sk_baby || gameskill == sk_easy ? 
      !(mthing->options & MTF_EASY) :
      gameskill == sk_hard || gameskill == sk_nightmare ?
      !(mthing->options & MTF_HARD) : !(mthing->options & MTF_NORMAL))
      return NULL;  // sf

   // find which type to spawn

   // haleyjd: special thing types that need to undergo the processing
   // below must be caught here
   if(mthing->type >= 9027 && mthing->type <= 9033)
   {
      // particle fountains
      i = E_SafeThingName("EEParticleFountain");
   }
   else
   {
      // killough 8/23/98: use table for faster lookup
      i = P_FindDoomedNum(mthing->type);
   }

   // phares 5/16/98:
   // Do not abort because of an unknown thing. Ignore it, but post a
   // warning message for the player.

   if(i == NUMMOBJTYPES)
   {
      doom_printf(FC_ERROR"Unknown Thing type %i at (%i, %i)",
                  mthing->type, mthing->x, mthing->y);
      return NULL;  // sf
   }

  // don't spawn keycards and players in deathmatch

   if(GameType == gt_dm && (mobjinfo[i].flags & MF_NOTDMATCH))
      return NULL;        // sf
   
   // don't spawn any monsters if -nomonsters

   if(nomonsters &&
      ((mobjinfo[i].flags3 & MF3_KILLABLE) || 
       (mobjinfo[i].flags & MF_COUNTKILL)))
      return NULL;        // sf

   // spawn it
spawnit:

   x = mthing->x << FRACBITS;
   y = mthing->y << FRACBITS;
   
   z = mobjinfo[i].flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

   // haleyjd 10/13/02: float rand z
   if(demo_version >= 331 && mobjinfo[i].flags2 & MF2_SPAWNFLOAT)
      z = FLOATRANDZ;

   mobj = P_SpawnMobj(x, y, z, i);
   
   mobj->spawnpoint = *mthing;

   if(mobj->tics > 0)
      mobj->tics = 1 + (P_Random (pr_spawnthing) % mobj->tics);

   if(!(mobj->flags & MF_FRIEND) &&
      mthing->options & MTF_FRIEND && 
      demo_version>=203)
   {
      mobj->flags |= MF_FRIEND;            // killough 10/98:
      P_UpdateThinker(&mobj->thinker);     // transfer friendliness flag
   }

   // killough 7/20/98: exclude friends
   if(!((mobj->flags ^ MF_COUNTKILL) & (MF_FRIEND | MF_COUNTKILL)))
      totalkills++;

   if(mobj->flags & MF_COUNTITEM)
      totalitems++;

   mobj->angle = R_WadToAngle(mthing->angle);
   if(mthing->options & MTF_AMBUSH)
      mobj->flags |= MF_AMBUSH;

   if(mobj->flags2 & MF2_FLOATBOB) // haleyjd: initialize floatbob seed
   {
      mobj->floatbob = P_Random(pr_floathealth);
   }

   // haleyjd: handling for dormant things
   if(mthing->options & MTF_DORMANT)
   {
      mobj->flags2 |= MF2_DORMANT;  // inert and invincible
      mobj->tics = -1; // don't go through state transitions
   }

   // haleyjd: set particle fountain color
   if(i == E_ThingNumForName("EEParticleFountain"))
   {
      mobj->effects |= (mthing->type - 9026) << FX_FOUNTAINSHIFT;
   }
   
   return mobj;
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//

extern fixed_t attackrange;

void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z, angle_t dir, 
                 int updown, boolean ptcl)
{
   mobj_t* th;

   // haleyjd 08/05/04: use new function
   z += P_SubRandom(pr_spawnpuff) << 10;

   th = P_SpawnMobj(x, y, z, E_SafeThingType(MT_PUFF));
   th->momz = FRACUNIT;
   th->tics -= P_Random(pr_spawnpuff) & 3;

   if(th->tics < 1)
      th->tics = 1;

   // don't make punches spark on the wall
   
   if(attackrange == MELEERANGE)
      P_SetMobjState(th, E_SafeState(S_PUFF3));

   // haleyjd: for demo sync etc we still need to do the above, so
   // here we'll make the puff invisible and draw particles instead
   if(ptcl && drawparticles && bulletpuff_particle)
   {
      if(bulletpuff_particle != 2)
         th->translucency = 0;
      if(attackrange != MELEERANGE)
         P_DrawSplash2(32, x, y, z, dir, updown, 1);
   }
}


//
// P_SpawnBlood
//
void P_SpawnBlood(fixed_t x,fixed_t y,fixed_t z, angle_t dir, int damage, mobj_t *target)
{
   // HTIC_TODO: Heretic support
   mobj_t* th;

   // haleyjd 08/05/04: use new function
   z += P_SubRandom(pr_spawnblood) << 10;

   th = P_SpawnMobj(x,y,z, E_SafeThingType(MT_BLOOD));
   th->momz = FRACUNIT*2;
   th->tics -= P_Random(pr_spawnblood)&3;
   
   if(th->tics < 1)
      th->tics = 1;
   
   if(damage <= 12 && damage >= 9)
   {
      P_SetMobjState(th, E_SafeState(S_BLOOD2));
   }
   else if (damage < 9)
   {
      P_SetMobjState(th, E_SafeState(S_BLOOD3));
   }

   // for demo sync, etc, we still need to do the above, so
   // we'll make the sprites above invisible and draw particles
   // instead
   if(drawparticles && bloodsplat_particle)
   {
      if(bloodsplat_particle != 2)
         th->translucency = 0;
      P_DrawSplash2(32, x, y, z, dir, 2, 
                    target->info->bloodcolor | MBC_BLOODMASK);
   }
}

// FIXME: These two functions are left over from an mobj-based
// particle system attempt in SMMU -- the particle line
// function could be useful for real particles maybe?

/*
void P_SpawnParticle(fixed_t x, fixed_t y, fixed_t z)
{
   P_SpawnMobj(x, y, z, MT_PARTICLE);
}


void P_ParticleLine(mobj_t *source, mobj_t *dest)
{
   fixed_t sourcex, sourcey, sourcez;
   fixed_t destx, desty, destz;
   int linedetail;
   int j;
   
   sourcex = source->x; sourcey = source->y;
   destx = dest->x; desty = dest->y;
   sourcez = source->z + (source->info->height/2);
   destz = dest->z + (dest->info->height/2);
   linedetail = P_AproxDistance(destx - sourcex, desty - sourcey)
      / FRACUNIT;
   
   // make the line
   for(j=0; j<linedetail; j++)
   {
      P_SpawnParticle(
         sourcex + ((destx - source->x)*j)/linedetail,
         sourcey + ((desty - source->y)*j)/linedetail,
         sourcez + ((destz - source->z)*j)/linedetail);
   }
}
*/

//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//

void P_CheckMissileSpawn (mobj_t* th)
{
   if(gameModeInfo->type == Game_DOOM)
   {
      th->tics -= P_Random(pr_missile)&3;
      if(th->tics < 1)
         th->tics = 1;
   }
   
   // move a little forward so an angle can
   // be computed if it immediately explodes
   
   th->x += th->momx>>1;
   th->y += th->momy>>1;
   th->z += th->momz>>1;
   
   // killough 8/12/98: for non-missile objects (e.g. grenades)
   if(!(th->flags & MF_MISSILE) && demo_version >= 203)
      return;
   
   // killough 3/15/98: no dropoff (really = don't care for missiles)
   if(!P_TryMove(th, th->x, th->y, false))
      P_ExplodeMissile (th);
}

//
// P_MissileMomz
//
// haleyjd 09/21/03: Missile z momentum calculation is now
// external to P_SpawnMissile, since it is also needed in a few
// other places.
//
fixed_t P_MissileMomz(fixed_t dx, fixed_t dy, fixed_t dz, int speed)
{
   int dist = P_AproxDistance(dx, dy);

   // haleyjd 11/17/03: a divide-by-zero error can occur here
   // if projectiles with zero speed are spawned -- use a suitable
   // default instead.
   if(speed == 0)
      speed = 10*FRACUNIT;

   dist = dist / speed;

   if(dist < 1)
      dist = 1;

   return dz / dist;
}

//
// P_SpawnMissile
//

mobj_t* P_SpawnMissile(mobj_t* source, mobj_t* dest, mobjtype_t type,
                       fixed_t z)
{
   angle_t an;
   mobj_t *th;  // haleyjd: restructured

   if(z != ONFLOORZ)
      z -= source->floorclip;

   th = P_SpawnMobj(source->x, source->y, z, type);

   S_StartSound(th, th->info->seesound);

   P_SetTarget(&th->target, source); // where it came from // killough 11/98
   an = R_PointToAngle2 (source->x, source->y, dest->x, dest->y);

   // fuzzy player --  haleyjd: add total invisibility, ghost
   if(dest->flags & MF_SHADOW || dest->flags2 & MF2_DONTDRAW ||
      dest->flags3 & MF3_GHOST)
   {  
      int shamt = (dest->flags3 & MF3_GHOST) ? 21 : 20; // haleyjd
      
      an += P_SubRandom(pr_shadow) << shamt;
   }

   th->angle = an;
   an >>= ANGLETOFINESHIFT;
   th->momx = FixedMul(th->info->speed, finecosine[an]);
   th->momy = FixedMul(th->info->speed, finesine[an]);
   th->momz = P_MissileMomz(dest->x - source->x, 
                            dest->y - source->y,
                            dest->z - source->z,
                            th->info->speed);

   P_CheckMissileSpawn(th);
   
   return th;
}

//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
extern fixed_t *yslope;

mobj_t *P_SpawnPlayerMissile(mobj_t* source, mobjtype_t type)
{
   mobj_t *th;
   fixed_t x, y, z, slope = 0;
   
   // see which target is to be aimed at
   
   angle_t an = source->angle;
   
   // killough 7/19/98: autoaiming was not in original beta
   // sf: made a multiplayer option
   if(autoaim)
   {
      // killough 8/2/98: prefer autoaiming at enemies
      int mask = demo_version < 203 ? 0 : MF_FRIEND;
      do
      {
         slope = P_AimLineAttack(source, an, 16*64*FRACUNIT, mask);
         if(!linetarget)
            slope = P_AimLineAttack(source, an += 1<<26, 16*64*FRACUNIT, mask);
         if(!linetarget)
            slope = P_AimLineAttack(source, an -= 2<<26, 16*64*FRACUNIT, mask);
         if(!linetarget)
         {
            an = source->angle;
            // haleyjd: use true slope angle
            slope = finetangent[(ANG90 - source->player->pitch)>>ANGLETOFINESHIFT];
         }
      }
      while(mask && (mask=0, !linetarget));  // killough 8/2/98
   }
   else
   {
      // haleyjd: use true slope angle
      slope = finetangent[(ANG90 - source->player->pitch)>>ANGLETOFINESHIFT];
   }

   x = source->x;
   y = source->y;
   z = source->z + 4*8*FRACUNIT - source->floorclip;
   
   th = P_SpawnMobj (x,y,z, type);
   
   S_StartSound(th, th->info->seesound);

   P_SetTarget(&th->target, source);   // killough 11/98
   th->angle = an;
   th->momx = FixedMul(th->info->speed,finecosine[an>>ANGLETOFINESHIFT]);
   th->momy = FixedMul(th->info->speed,finesine[an>>ANGLETOFINESHIFT]);
   th->momz = FixedMul(th->info->speed,slope);
   
   P_CheckMissileSpawn(th);
   
   return th;    //sf
}

//
// Start new Eternity mobj functions
//

boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state)
{
   state_t *st;
   
   if(state == NullStateNum)
   {
      // remove mobj
      mobj->state = NULL;
      P_RemoveMobj(mobj);
      return false;
   }
   
   st = &states[state];
   mobj->state = st;
   mobj->tics = st->tics;
   mobj->sprite = (mobj->skin ? mobj->skin->sprite : st->sprite);
   mobj->frame = st->frame;
   
   return true;
}

mobj_t *P_SpawnMissileAngle(mobj_t *source, mobjtype_t type, 
                            angle_t angle, fixed_t momz, fixed_t z)
{
   mobj_t *mo;

   if(z != ONFLOORZ)
      z -= source->floorclip;

   mo = P_SpawnMobj(source->x, source->y, z, type);
   
   S_StartSound(mo, mo->info->seesound);

   // haleyjd 09/21/03: don't set this directly!
   P_SetTarget(&mo->target, source);

   mo->angle = angle;
   angle >>= ANGLETOFINESHIFT;
   mo->momx = FixedMul(mo->info->speed, finecosine[angle]);
   mo->momy = FixedMul(mo->info->speed, finesine[angle]);
   mo->momz = momz;

   P_CheckMissileSpawn(mo);
   
   return mo;
}

void P_Massacre(int friends)
{
   mobj_t *mo;
   thinker_t *think;
   
   for(think = thinkercap.next; think != &thinkercap; think = think->next)
   {
      if(think->function != P_MobjThinker)
         continue;

      mo = (mobj_t *)think;
      
      if((mo->flags & MF_COUNTKILL || mo->flags3 & MF3_KILLABLE) && 
         mo->health > 0)
      {
         switch(friends)
         {
         case 1: // kill only friends
            if(!(mo->flags & MF_FRIEND))
               continue;
            break;
         case 2: // kill only enemies
            if(mo->flags & MF_FRIEND)
               continue;
            break;
         default: // Everybody Dies (TM)
            break;
         }
         mo->flags2 &= ~MF2_INVULNERABLE; // haleyjd 04/09/99: none of that!
         P_DamageMobj(mo, NULL, NULL, 10000, MOD_UNKNOWN);
      }
   }
}

// haleyjd: falling damage

void P_FallingDamage(player_t *player)
{
   int mom, damage, dist;
   
   mom = D_abs(player->mo->momz);
   dist = FixedMul(mom, 16*FRACUNIT/23);
   
   if(mom > 63*FRACUNIT)
      damage = 10000; // instant death
   else
      damage = ((FixedMul(dist, dist)/10)>>FRACBITS)-24;
        
   // no-death threshold
   //   don't kill the player unless he's fallen more than this, or only
   //   has 1 point of health left (in which case he deserves it ;)
   if(player->mo->momz > -39*FRACUNIT && damage > player->mo->health && 
      player->mo->health != 1)
   {
      damage = player->mo->health - 1;
   }
   
   if(damage >= player->mo->health)
      player->mo->intflags |= MIF_DIEDFALLING;
   
   P_DamageMobj(player->mo, NULL, NULL, damage, MOD_FALLING);
}

//
// P_AdjustFloorClip
//
// Adapted from zdoom source, see the zdoom license.
// Thanks to Randy Heit.
//
void P_AdjustFloorClip(mobj_t *thing)
{
   fixed_t oldclip = thing->floorclip;
   fixed_t shallowestclip = 0x7fffffff;
   const msecnode_t *m;

   // no floorclipping in old demos or if terrain types are off;
   // absorb test for FOOTCLIP flag here
   if(demo_version < 331 || comp[comp_terrain] ||
      !(thing->flags2 & MF2_FOOTCLIP))
   {
      thing->floorclip = 0;
      return;
   }

   // find shallowest touching floor, discarding any deep water
   // involved (it does its own clipping)
   for(m = thing->touching_sectorlist; m; m = m->m_tnext)
   {
      if(m->m_sector->heightsec == -1 &&
         thing->z == m->m_sector->floorheight)
      {
         fixed_t clip = E_SectorFloorClip(m->m_sector);

         if(clip < shallowestclip)
            shallowestclip = clip;
      }
   }

   if(shallowestclip == 0x7fffffff)
      thing->floorclip = 0;
   else
      thing->floorclip = shallowestclip;

   // adjust the player's viewheight
   if(thing->player && oldclip != thing->floorclip)
   {
      player_t *p = thing->player;

      p->viewheight -= oldclip - thing->floorclip;
      p->deltaviewheight = (VIEWHEIGHT - p->viewheight) >> 3;
   }
}

//
//
// P_ThingInfoHeight
//
// haleyjd 07/06/05: 
//
// Function to retrieve proper thing height information for a thing.
//
int P_ThingInfoHeight(mobjinfo_t *mi)
{
   return 
      ((demo_version >= 333 && !comp[comp_theights] &&
       mi->c3dheight) ?
       mi->c3dheight : mi->height);
}

//
// P_ChangeThingHeights
//
// Updates all things with appropriate height values depending on
// the conditions established in P_ThingInfoHeight above. Not safe
// if things are in over/under situations, but fine otherwise. This
// is used to keep the game consistent with the value of comp_theights.
//
void P_ChangeThingHeights(void)
{
   thinker_t *th;

   if(gamestate != GS_LEVEL)
      return;

   for(th = thinkercap.next; th != &thinkercap; th = th->next)
   {
      if(th->function == P_MobjThinker)
      {
         mobj_t *mo = (mobj_t *)th;

         mo->height = P_ThingInfoHeight(mo->info);
      }
   }
}

//
// haleyjd 02/02/04: Thing IDs (aka TIDs)
//

#define TIDCHAINS 131

// TID hash chains
static mobj_t *tidhash[TIDCHAINS];

//
// P_InitTIDHash
//
// Initializes the tid hash table.
//
void P_InitTIDHash(void)
{
   memset(tidhash, 0, TIDCHAINS*sizeof(mobj_t *));
}

//
// P_AddThingTID
//
// Adds a thing to the tid hash table
//
void P_AddThingTID(mobj_t *mo, int tid)
{
   // zero is no tid, and negative tids are reserved to
   // have special meanings
   if(tid <= 0)
   {
      mo->tid = 0;
      mo->tid_next = NULL;
      mo->tid_prevn = NULL;
   }
   else
   {
      int key = tid % TIDCHAINS;

      mo->tid = tid;

      // insert at head of chain
      mo->tid_next  = tidhash[key];
      mo->tid_prevn = &tidhash[key];
      tidhash[key]  = mo;

      // connect to any existing things in chain
      if(mo->tid_next)
         mo->tid_next->tid_prevn = &(mo->tid_next);
   }
}

//
// P_RemoveThingTID
//
// Removes the given thing from the tid hash table if it is
// in it already.
//
void P_RemoveThingTID(mobj_t *mo)
{
   if(mo->tid > 0 && mo->tid_prevn)
   {
      // set previous thing's next field to this thing's next thing
      *(mo->tid_prevn) = mo->tid_next;

      // set next thing's prev field to this thing's prev field
      if(mo->tid_next)
         mo->tid_next->tid_prevn = mo->tid_prevn;
   }
   
   // clear tid
   mo->tid = 0;
}

//
// P_FindMobjFromTID
//
// Like line and sector tag search functions, this function will
// keep returning the next object with the same tid when called
// repeatedly with the previous call's return value. Returns NULL
// once the end of the chain is hit. Calling it again at that point
// would restart the search from the base of the chain.
//
// The last parameter only applies when this is called from a
// Small native function, and can be left null otherwise.
//
mobj_t *P_FindMobjFromTID(int tid, mobj_t *rover, SmallContext_t *context)
{
   switch(tid)
   {
   // Reserved TIDs
   case 0:   // zero is "no tid"
      return NULL; 
   
   case -1:  // players are -1 through -4 
   case -2:
   case -3:
   case -4:
      {
         int pnum = abs(tid) - 1;

         return !rover && playeringame[pnum] ? players[pnum].mo : NULL;
      }
   
   case -10: // script trigger object (may be NULL, which is fine)
      return context ? context->invocationData.trigger : NULL;
   
   // Normal TIDs
   default:
      if(tid < 0)
         return NULL;

      rover = rover ? rover->tid_next : tidhash[tid % TIDCHAINS];

      while(rover && rover->tid != tid)
         rover = rover->tid_next;

      return rover;
   }
}

//
// Thing Collections
//
// haleyjd: This pseudo-class is the ultimate generalization of the
// boss spawner spots code, allowing arbitrary reallocating arrays
// of mobj_t pointers to be maintained and manipulated. This is currently
// used by boss spawn spots, D'Sparil spots, and intermission cameras.
// I wish it was used by deathmatch spots, but that would present a
// compatibility problem (spawning mobj_t's at their locations would
// most likely result in demo desyncs).
//

//
// P_InitMobjCollection
//
// Sets up an MobjCollection object. This is for objects kept on the 
// stack.
//
void P_InitMobjCollection(MobjCollection *mc, int type)
{
   memset(mc, 0, sizeof(MobjCollection));

   mc->type = type;
}

//
// P_ReInitMobjCollection
//
// Call this to set the number of mobjs in the collection to zero.
// Should be done for each global collection after a level ends and 
// before beginning a new one. Doesn't molest the array pointer or
// numalloc fields. Resets the wrap iterator by necessity.
//
void P_ReInitMobjCollection(MobjCollection *mc, int type)
{
   mc->num = mc->wrapiterator = 0;
   mc->type = type;   
}

//
// P_ClearMobjCollection
//
// Frees an mobj collection's pointer array and sets all the
// fields to zero.
//
void P_ClearMobjCollection(MobjCollection *mc)
{
   free(mc->ptrarray);

   memset(mc, 0, sizeof(MobjCollection));
}

//
// P_CollectThings
//
// Generalized from the boss brain spot code; builds a collection
// of mapthings of a certain type. The type must be set within the
// collection object before calling this.
//
void P_CollectThings(MobjCollection *mc)
{
   thinker_t *th;

   for(th = thinkercap.next; th != &thinkercap; th = th->next)
   {
      if(th->function == P_MobjThinker)
      {
         mobj_t *mo = (mobj_t *)th;

         if(mo->type == mc->type)
         {
            if(mc->num >= mc->numalloc)
            {
               mc->ptrarray = realloc(mc->ptrarray,
                  (mc->numalloc = mc->numalloc ?
                   mc->numalloc*2 : 32) * sizeof *mc->ptrarray);
            }
            (mc->ptrarray)[mc->num] = mo;
            mc->num++;
         }
      }
   }
}

//
// P_CollectionIsEmpty
//
// Returns true if there are no objects in the collection, and
// false otherwise.
//
boolean P_CollectionIsEmpty(MobjCollection *mc)
{
   return !mc->num;
}

//
// P_CollectionWrapIterator
//
// Returns each object in a collection in the order they were added
// at each consecutive call, wrapping to the beginning when the end
// is reached.
//
mobj_t *P_CollectionWrapIterator(MobjCollection *mc)
{
   mobj_t *ret = (mc->ptrarray)[mc->wrapiterator++];

   mc->wrapiterator %= mc->num;

   return ret;
}

//
// P_CollectionGetRandom
//
// Returns a random object from the collection using the specified
// random number generator for full compatibility.
//
mobj_t *P_CollectionGetRandom(MobjCollection *mc, pr_class_t rngnum)
{
   return (mc->ptrarray)[P_Random(rngnum) % mc->num];
}

//
// Small natives
//

//
// sm_thingspawn
// * Implements ThingSpawn(type, x, y, z, tid = 0, angle = 0)
//
static cell AMX_NATIVE_CALL sm_thingspawn(AMX *amx, cell *params)
{
   int type, tid, ang;
   fixed_t x, y, z;
   angle_t angle;
   mobj_t *mo;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   type  = params[1];
   x     = params[2];
   y     = params[3];
   z     = params[4];
   tid   = params[5];
   ang   = params[6];

   // Type resolving can be done in Small itself, so we treat
   // this as a raw type. Allows the most flexibility without
   // duplicate functions.

   if(type < 0 || type >= NUMMOBJTYPES)
   {
      amx_RaiseError(amx, AMX_ERR_BOUNDS);
      return -1;
   }

   if(ang >= 360)
   {
      while(ang >= 360)
         ang = ang - 360;
   }
   else if(ang < 0)
   {
      while(ang < 0)
         ang = ang + 360;
   }

   angle = (angle_t)(((ULong64)ang << 32) / 360);

   mo = P_SpawnMobj(x<<FRACBITS, y<<FRACBITS, z<<FRACBITS, type);

   P_AddThingTID(mo, tid);

   mo->angle = angle;

   return 0;
}

//
// sm_thingspawnspot
// * Implements ThingSpawnSpot(type, spottid, tid = 0, angle = 0)
//
static cell AMX_NATIVE_CALL sm_thingspawnspot(AMX *amx, cell *params)
{
   int type, spottid, tid, ang;
   angle_t angle;
   mobj_t *mo, *spawnspot = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   type    = params[1];
   spottid = params[2];
   tid     = params[3];
   ang     = params[4];

   // Type resolving can be done in Small itself, so we treat
   // this as a raw type. Allows the most flexibility without
   // duplicate functions.

   if(type < 0 || type >= NUMMOBJTYPES)
   {
      amx_RaiseError(amx, AMX_ERR_BOUNDS);
      return 0;
   }

   if(ang >= 360)
      ang = ang - 360;
   else if(ang < 0)
      ang = ang + 360;

   angle = (angle_t)(((ULong64)ang << 32) / 360);

   while((spawnspot = P_FindMobjFromTID(spottid, spawnspot, context)))
   {
      mo = P_SpawnMobj(spawnspot->x, spawnspot->y, spawnspot->z, type);
      
      P_AddThingTID(mo, tid);
      
      mo->angle = angle;
   }

   return 0;
}

//
// sm_thingsound
// * Implements ThingSound(name[], tid)
//
static cell AMX_NATIVE_CALL sm_thingsound(AMX *amx, cell *params)
{
   int err, tid;
   char *sndname;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   // get sound name
   if((err = A_GetSmallString(amx, &sndname, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   // get tid
   tid = (int)params[2];

   while((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      S_StartSoundName(mo, sndname);
   }

   Z_Free(sndname);

   return 0;
}

//
// sm_thingsoundnum
// * Implements ThingSoundNum(num, tid)
//
static cell AMX_NATIVE_CALL sm_thingsoundnum(AMX *amx, cell *params)
{
   int tid, sndnum;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   sndnum = (int)params[1];
   tid    = (int)params[2];

   while((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      S_StartSound(mo, sndnum);
   }

   return 0;
}

//
// sm_thinginfosound
// * Implements ThingInfoSound(type, tid)
//
// This function can play any of the internal thingtype sounds for
// an object. Will be very useful in custom behavior scripts.
//
static cell AMX_NATIVE_CALL sm_thinginfosound(AMX *amx, cell *params)
{
   int tid, typenum;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   typenum = (int)params[1];
   tid     = (int)params[2];

   while((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      int sndnum = 0;

      switch(typenum)
      {
      case 0:
         sndnum = mo->info->seesound; break;
      case 1:
         sndnum = mo->info->activesound; break;
      case 2:
         sndnum = mo->info->attacksound; break;
      case 3:
         sndnum = mo->info->painsound; break;
      case 4:
         sndnum = mo->info->deathsound; break;
      default:
         break;
      }

      S_StartSound(mo, sndnum);
   }

   return 0;
}

//
// sm_thingmassacre
// * Implements ThingMassacre(type)
//
static cell AMX_NATIVE_CALL sm_thingmassacre(AMX *amx, cell *params)
{
   int massacreType = (int)params[1];

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   P_Massacre(massacreType);

   return 0;
}

//
// Thing property IDs as defined in Small
//
enum
{
   TF_TYPE,
   TF_TICS,
   TF_HEALTH,
   TF_SPECIAL1,
   TF_SPECIAL2,
   TF_SPECIAL3,
   TF_EFFECTS,
   TF_TRANSLUCENCY
};

//
// sm_thinggetproperty
// * Implements ThingGetProperty(tid, field)
//
static cell AMX_NATIVE_CALL sm_thinggetproperty(AMX *amx, cell *params)
{
   int value = 0, field, tid;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }
   
   tid   = (int)params[1];
   field = (int)params[2];

   if((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      switch(field)
      {
      case TF_TYPE:     value = mo->type;     break;
      case TF_TICS:     value = mo->tics;     break;
      case TF_HEALTH:   value = mo->health;   break;
      case TF_SPECIAL1: value = mo->special1; break;
      case TF_SPECIAL2: value = mo->special2; break;
      case TF_SPECIAL3: value = mo->special3; break;
      case TF_EFFECTS:  value = mo->effects;  break;
      case TF_TRANSLUCENCY: value = mo->translucency; break;
      }
   }

   return value;
}

//
// sm_thingsetproperty
// * Implements ThingSetProperty(tid, field, value)
//
static cell AMX_NATIVE_CALL sm_thingsetproperty(AMX *amx, cell *params)
{
   int field, value, tid;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }
   
   tid        = (int)params[1];
   field      = (int)params[2];
   value      = (int)params[3];

   while((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      switch(field)
      {
      case TF_TICS:   mo->tics = value;   break;
      case TF_HEALTH: mo->health = value; break;
      case TF_SPECIAL1: mo->special1 = (short)value; break;
      case TF_SPECIAL2: mo->special2 = (short)value; break;
      case TF_SPECIAL3: mo->special3 = (short)value; break;
      case TF_EFFECTS:  mo->effects = value; break;
      case TF_TRANSLUCENCY: mo->translucency = value; break;
      }
   }

   return 0;
}

static cell AMX_NATIVE_CALL sm_thingflagsstr(AMX *amx, cell *params)
{
   int tid, op, err;
   char *flags;
   long *results;
   mobj_t *mo = NULL;
   SmallContext_t *context = A_GetContextForAMX(amx);

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   tid = (int)params[1];
   op  = (int)params[2];

   // get sound name
   if((err = A_GetSmallString(amx, &flags, params[3])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   results = deh_ParseFlagsCombined(flags);

   while((mo = P_FindMobjFromTID(tid, mo, context)))
   {
      switch(op)
      {
      case 0: // set flags
         mo->flags  = results[0];
         mo->flags2 = results[1];
         mo->flags3 = results[2];
         break;
      case 1: // add flags
         mo->flags  |= results[0];
         mo->flags2 |= results[1];
         mo->flags3 |= results[2];
         break;
      case 2: // remove flags
         mo->flags  &= ~(results[0]);
         mo->flags2 &= ~(results[1]);
         mo->flags3 &= ~(results[2]);
         break;
      }
   }

   free(flags);

   return 0;
}

AMX_NATIVE_INFO mobj_Natives[] =
{
   { "_ThingSpawn",        sm_thingspawn },
   { "_ThingSpawnSpot",    sm_thingspawnspot },
   { "_ThingSound",        sm_thingsound },
   { "_ThingSoundNum",     sm_thingsoundnum },
   { "_ThingInfoSound",    sm_thinginfosound },
   { "_ThingMassacre",     sm_thingmassacre },
   { "_ThingGetProperty",  sm_thinggetproperty },
   { "_ThingSetProperty",  sm_thingsetproperty },
   { "_ThingFlagsFromStr", sm_thingflagsstr },
   { NULL,                NULL }
};

//----------------------------------------------------------------------------
//
// $Log: p_mobj.c,v $
// Revision 1.26  1998/05/16  00:24:12  phares
// Unknown things now flash warning msg instead of causing abort
//
// Revision 1.25  1998/05/15  00:33:19  killough
// Change function used in thing deletion check
//
// Revision 1.24  1998/05/14  08:01:56  phares
// Added Player Starts 5-8 (4001-4004)
//
// Revision 1.23  1998/05/12  12:47:21  phares
// Removed OVER_UNDER code
//
// Revision 1.22  1998/05/12  06:09:32  killough
// Prevent voodoo dolls from causing player bopping
//
// Revision 1.21  1998/05/07  00:54:23  killough
// Remove dependence on evaluation order, fix (-1) ptr bug
//
// Revision 1.20  1998/05/05  15:35:16  phares
// Documentation and Reformatting changes
//
// Revision 1.19  1998/05/03  23:16:49  killough
// Remove unnecessary declarations, fix #includes
//
// Revision 1.18  1998/04/27  02:02:12  killough
// Fix crashes caused by mobjs targeting deleted thinkers
//
// Revision 1.17  1998/04/10  06:35:56  killough
// Fix mobj state machine cycle hangs
//
// Revision 1.16  1998/03/30  12:05:57  jim
// Added support for not-dm not-coop thing flags
//
// Revision 1.15  1998/03/28  18:00:58  killough
// Remove old dead code which is commented out
//
// Revision 1.14  1998/03/23  15:24:30  phares
// Changed pushers to linedef control
//
// Revision 1.13  1998/03/20  00:30:06  phares
// Changed friction to linedef control
//
// Revision 1.12  1998/03/16  12:43:41  killough
// Use new P_TryMove() allowing dropoffs in certain cases
//
// Revision 1.11  1998/03/12  14:28:46  phares
// friction and IDCLIP changes
//
// Revision 1.10  1998/03/11  17:48:28  phares
// New cheats, clean help code, friction fix
//
// Revision 1.9  1998/03/09  18:27:04  phares
// Fixed bug in neighboring variable friction sectors
//
// Revision 1.8  1998/03/04  07:40:04  killough
// comment out noclipping hack for now
//
// Revision 1.7  1998/02/26  21:15:30  killough
// Fix thing type 0 crashes, e.g. MAP25
//
// Revision 1.6  1998/02/24  09:20:11  phares
// Removed 'onground' local variable
//
// Revision 1.5  1998/02/24  08:46:21  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.4  1998/02/23  04:46:21  killough
// Preserve no-clipping cheat across idclev
//
// Revision 1.3  1998/02/17  05:47:11  killough
// Change RNG calls to use keys for each block
//
// Revision 1.2  1998/01/26  19:24:15  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
