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
//      Weapon sprite animation, weapon objects.
//      Action functions for weapons.
//
// NETCODE_FIXME -- DEMO_FIXME -- WEAPON_FIXME: Weapon changing, prefs,
// etc. need overhaul for all of these. See comments in other modules
// for why and how it needs to be changed. zdoom uses separate events
// outside of ticcmd_t to indicate weapon changes now, and so will not
// face the issue of being limited to 16 weapons.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_pspr.c,v 1.13 1998/05/07 00:53:36 killough Exp $";

#include "doomstat.h"
#include "d_event.h"
#include "g_game.h"
#include "m_random.h"
#include "p_enemy.h"
#include "p_inter.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_pspr.h"
#include "p_tick.h"
#include "r_main.h"
#include "r_segs.h"
#include "r_things.h"
#include "s_sound.h"
#include "sounds.h"
#include "e_states.h"
#include "e_things.h"
#include "e_sound.h"
#include "d_dehtbl.h"
#include "p_info.h"

int weapon_speed = 6;
int default_weapon_speed = 6;

#define LOWERSPEED   (FRACUNIT*weapon_speed)
#define RAISESPEED   (FRACUNIT*weapon_speed)
#define WEAPONBOTTOM (FRACUNIT*128)
#define WEAPONTOP    (FRACUNIT*32)

#define BFGCELLS bfgcells        /* Ty 03/09/98 externalized in p_inter.c */

// haleyjd 08/10/02
#define SUBTRACTAMMO() \
        player->ammo[weaponinfo[player->readyweapon].ammo] -= \
        weaponinfo[player->readyweapon].ammopershot

extern void P_Thrust(player_t *, angle_t, fixed_t);
int weapon_recoil;      // weapon recoil

// The following array holds the recoil values         // phares

static const int recoil_values[] = {    // phares
   10,  // wp_fist
   10,  // wp_pistol
   30,  // wp_shotgun
   10,  // wp_chaingun
   100, // wp_missile
   20,  // wp_plasma
   100, // wp_bfg
   0,   // wp_chainsaw
   80,  // wp_supershotgun
};

//
// P_SetPsprite
//

static void P_SetPsprite(player_t *player, int position, statenum_t stnum)
{
   pspdef_t *psp = &player->psprites[position];
   
   do
   {
      state_t *state;
      
      if(!stnum)
      {
         // object removed itself
         psp->state = NULL;
         break;
      }

      // killough 7/19/98: Pre-Beta BFG
      if(stnum == E_StateNumForDEHNum(S_BFG1) && bfgtype == bfg_classic)
         stnum = E_SafeState(S_OLDBFG1); // Skip to alternative weapon frame

      state = &states[stnum];
      psp->state = state;
      psp->tics = state->tics;        // could be 0

      if(state->misc1)
      {
         // coordinate set
         psp->sx = state->misc1 << FRACBITS;
         psp->sy = state->misc2 << FRACBITS;
      }

      // Call action routine.
      // Modified handling.
      if(state->action)
      {
         state->action(player, psp);
         if(!psp->state)
            break;
      }
      stnum = psp->state->nextstate;
   }
   while(!psp->tics);   // an initial state of 0 could cycle through
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//

static void P_BringUpWeapon(player_t *player)
{
   statenum_t newstate;
   
   if(player->pendingweapon == wp_nochange)
      player->pendingweapon = player->readyweapon;
   
   if(player->pendingweapon == wp_chainsaw)
      S_StartSound(player->mo, sfx_sawup);
   
   newstate = weaponinfo[player->pendingweapon].upstate;
   
   player->pendingweapon = wp_nochange;
   
   // killough 12/98: prevent pistol from starting visibly at bottom of screen:
   player->psprites[ps_weapon].sy = demo_version >= 203 ? 
      WEAPONBOTTOM+FRACUNIT*2 : WEAPONBOTTOM;
   
   P_SetPsprite(player, ps_weapon, newstate);
}

// The first set is where the weapon preferences from             // killough,
// default.cfg are stored. These values represent the keys used   // phares
// in DOOM2 to bring up the weapon, i.e. 6 = plasma gun. These    //    |
// are NOT the wp_* constants.                                    //    V

int weapon_preferences[2][NUMWEAPONS+1] =
{
   {6, 9, 4, 3, 2, 8, 5, 7, 1, 0},  // !compatibility preferences
   {6, 9, 4, 3, 2, 8, 5, 7, 1, 0},  //  compatibility preferences
};

// P_SwitchWeapon checks current ammo levels and gives you the
// most preferred weapon with ammo. It will not pick the currently
// raised weapon. When called from P_CheckAmmo this won't matter,
// because the raised weapon has no ammo anyway. When called from
// G_BuildTiccmd you want to toggle to a different weapon regardless.

int P_SwitchWeapon(player_t *player)
{
   int *prefer = weapon_preferences[demo_compatibility!=0]; // killough 3/22/98
   int currentweapon = player->readyweapon;
   int newweapon = currentweapon;
   int i = NUMWEAPONS+1;   // killough 5/2/98

  // killough 2/8/98: follow preferences and fix BFG/SSG bugs

   // haleyjd FIXME: makes assumptions about ammo per shot
   do
   {
      switch (*prefer++)
      {
      case 1:
         if(!player->powers[pw_strength])  // allow chainsaw override
            break;
      case 0:
         newweapon = wp_fist;
         break;
      case 2:
         if(player->ammo[am_clip])
            newweapon = wp_pistol;
         break;
      case 3:
         if(player->weaponowned[wp_shotgun] && player->ammo[am_shell])
            newweapon = wp_shotgun;
         break;
      case 4:
         if(player->weaponowned[wp_chaingun] && player->ammo[am_clip])
            newweapon = wp_chaingun;
         break;
      case 5:
         if(player->weaponowned[wp_missile] && player->ammo[am_misl])
            newweapon = wp_missile;
         break;
      case 6:
         if(player->weaponowned[wp_plasma] && player->ammo[am_cell] &&
            gamemode != shareware)
            newweapon = wp_plasma;
         break;
      case 7:
         if(player->weaponowned[wp_bfg] && gamemode != shareware &&
            player->ammo[am_cell] >= (demo_compatibility ? 41 : 40))
            newweapon = wp_bfg;
         break;
      case 8:
         if(player->weaponowned[wp_chainsaw])
            newweapon = wp_chainsaw;
         break;
      case 9:
         if(player->weaponowned[wp_supershotgun] && 
            gamemode == commercial &&
            player->ammo[am_shell] >= (demo_compatibility ? 3 : 2))
            newweapon = wp_supershotgun;
         break;
      }
   }
   while (newweapon==currentweapon && --i);        // killough 5/2/98
   return newweapon;
}

// killough 5/2/98: whether consoleplayer prefers weapon w1 over weapon w2.
int P_WeaponPreferred(int w1, int w2)
{
  return
    (weapon_preferences[0][0] != ++w2 && (weapon_preferences[0][0] == ++w1 ||
    (weapon_preferences[0][1] !=   w2 && (weapon_preferences[0][1] ==   w1 ||
    (weapon_preferences[0][2] !=   w2 && (weapon_preferences[0][2] ==   w1 ||
    (weapon_preferences[0][3] !=   w2 && (weapon_preferences[0][3] ==   w1 ||
    (weapon_preferences[0][4] !=   w2 && (weapon_preferences[0][4] ==   w1 ||
    (weapon_preferences[0][5] !=   w2 && (weapon_preferences[0][5] ==   w1 ||
    (weapon_preferences[0][6] !=   w2 && (weapon_preferences[0][6] ==   w1 ||
    (weapon_preferences[0][7] !=   w2 && (weapon_preferences[0][7] ==   w1
   ))))))))))))))));
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
// (only in demo_compatibility mode -- killough 3/22/98)
//

boolean P_CheckAmmo(player_t *player)
{
   ammotype_t ammo = weaponinfo[player->readyweapon].ammo;

   // haleyjd 08/10/02: get count from weaponinfo_t
   // (BFGCELLS value is now written into struct by DeHackEd code)
   int count = weaponinfo[player->readyweapon].ammopershot;

#if 0  
   int count = 1;  // Regular
   
   if(player->readyweapon == wp_bfg) // Minimal amount for one shot varies.
      count = BFGCELLS;
   else if(player->readyweapon == wp_supershotgun) // Double barrel.
      count = 2;
#endif
      
   // Some do not need ammunition anyway.
   // Return if current ammunition sufficient.
   
   if(ammo == am_noammo || player->ammo[ammo] >= count)
      return true;
   
   // Out of ammo, pick a weapon to change to.
   //
   // killough 3/22/98: for old demos we do the switch here and now;
   // for Boom games we cannot do this, and have different player
   // preferences across demos or networks, so we have to use the
   // G_BuildTiccmd() interface instead of making the switch here.
   
   if(demo_compatibility)
   {
      player->pendingweapon = P_SwitchWeapon(player);      // phares
      // Now set appropriate weapon overlay.
      P_SetPsprite(player,ps_weapon,weaponinfo[player->readyweapon].downstate);
   }
   
#if 0 /* PROBABLY UNSAFE */
   else
      if (demo_version >= 203)  // killough 9/5/98: force switch if out of ammo
         P_SetPsprite(player,ps_weapon,weaponinfo[player->readyweapon].downstate);
#endif
      
   return false;
}

//
// P_FireWeapon.
//

int lastshottic; // killough 3/22/98

static void P_FireWeapon(player_t *player)
{
   statenum_t newstate;
   
   if(!P_CheckAmmo(player))
      return;
   
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK1));
   newstate = weaponinfo[player->readyweapon].atkstate;
   P_SetPsprite(player, ps_weapon, newstate);
   if(!player->powers[pw_silencer]) // haleyjd 04/06/03
      P_NoiseAlert(player->mo, player->mo);
   lastshottic = gametic;                       // killough 3/22/98
}

//
// P_DropWeapon
// Player died, so put the weapon away.
//

void P_DropWeapon(player_t *player)
{
   P_SetPsprite(player, ps_weapon,
                weaponinfo[player->readyweapon].downstate);
}

//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//

void A_WeaponReady(player_t *player, pspdef_t *psp)
{
   // get out of attack state
   if(player->mo->state == &states[E_SafeState(S_PLAY_ATK1)]
      || player->mo->state == &states[E_SafeState(S_PLAY_ATK2)])
      P_SetMobjState(player->mo, E_SafeState(S_PLAY));

   if(player->readyweapon == wp_chainsaw && 
      psp->state == &states[E_SafeState(S_SAW)])
      S_StartSound(player->mo, sfx_sawidl);

   // check for change
   //  if player is dead, put the weapon away
   
   if(player->pendingweapon != wp_nochange || !player->health)
   {
      // change weapon (pending weapon should already be validated)
      statenum_t newstate = weaponinfo[player->readyweapon].downstate;
      P_SetPsprite(player, ps_weapon, newstate);
      return;
   }

   // check for fire
   //  the missile launcher and bfg do not auto fire
   
   if(player->cmd.buttons & BT_ATTACK)
   {
      if(!player->attackdown || 
         (player->readyweapon != wp_missile && 
          player->readyweapon != wp_bfg))
      {
         player->attackdown = true;
         P_FireWeapon(player);
         return;
      }
   }
   else
      player->attackdown = false;

   // bob the weapon based on movement speed
   {
      int angle = (128*leveltime) & FINEMASK;
      psp->sx = FRACUNIT + FixedMul(player->bob, finecosine[angle]);
      angle &= FINEANGLES/2-1;
      psp->sy = WEAPONTOP + FixedMul(player->bob, finesine[angle]);
   }
}

//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//

void A_ReFire(player_t *player, pspdef_t *psp)
{
   // check for fire
   //  (if a weaponchange is pending, let it go through instead)
   
   if((player->cmd.buttons & BT_ATTACK)
      && player->pendingweapon == wp_nochange && player->health)
   {
      player->refire++;
      P_FireWeapon(player);
   }
   else
   {
      player->refire = 0;
      P_CheckAmmo(player);
   }
}

/* cph 2002/08/08 - In old Doom, P_CheckAmmo would start the weapon 
 * lowering immediately. This was lost in Boom when the weapon 
 * switching logic was rewritten. But we must tell Doom that we 
 * don't need to complete the reload frames for the weapon here. 
 * G_BuildTiccmd will set ->pendingweapon
 * for us later on. */

void A_CheckReload(player_t *player, pspdef_t *psp)
{
   if(!P_CheckAmmo(player) && demo_version >= 331)
   {
      P_SetPsprite(player,ps_weapon,
                   weaponinfo[player->readyweapon].downstate);
   }
}

//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//

void A_Lower(player_t *player, pspdef_t *psp)
{
   psp->sy += LOWERSPEED;
   
   // Is already down.
   if(psp->sy < WEAPONBOTTOM)
      return;

   // Player is dead.
   if(player->playerstate == PST_DEAD)
   {
      psp->sy = WEAPONBOTTOM;
      return;      // don't bring weapon back up
   }

   // The old weapon has been lowered off the screen,
   // so change the weapon and start raising it
   
   if(!player->health)
   {      // Player is dead, so keep the weapon off screen.
      P_SetPsprite(player,  ps_weapon, NullStateNum);
      return;
   }
   
   player->readyweapon = player->pendingweapon;
   
   P_BringUpWeapon(player);
}

//
// A_Raise
//

void A_Raise(player_t *player, pspdef_t *psp)
{
   statenum_t newstate;
   
   psp->sy -= RAISESPEED;
   
   if(psp->sy > WEAPONTOP)
      return;
   
   psp->sy = WEAPONTOP;
   
   // The weapon has been raised all the way,
   //  so change to the ready state.
   
   newstate = weaponinfo[player->readyweapon].readystate;
   
   P_SetPsprite(player, ps_weapon, newstate);
}

// Weapons now recoil, amount depending on the weapon.              // phares
//                                                                  //   |
// The P_SetPsprite call in each of the weapon firing routines      //   V
// was moved here so the recoil could be synched with the
// muzzle flash, rather than the pressing of the trigger.
// The BFG delay caused this to be necessary.

static void A_FireSomething(player_t* player,int adder)
{
   P_SetPsprite(player, ps_flash,
      weaponinfo[player->readyweapon].flashstate+adder);
   
   // killough 3/27/98: prevent recoil in no-clipping mode
   if(!(player->mo->flags & MF_NOCLIP))
      if(weapon_recoil && (demo_version >= 203 || !compatibility))
         P_Thrust(player, ANG180 + player->mo->angle,
                  2048*recoil_values[player->readyweapon]); // phares
}

//
// A_GunFlash
//

void A_GunFlash(player_t *player, pspdef_t *psp)
{
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   A_FireSomething(player,0);                               // phares
}

//
// WEAPON ATTACKS
//

// haleyjd 09/24/00: Infamous DeHackEd Problem Fix
//   Its been known for years that setting weapons to use am_noammo
//   that normally use non-infinite ammo types causes your max shells
//   to reduce, now I know why:
//
//   player->ammo[weaponinfo[player->readyweapon].ammo]--;
//
//   If weaponinfo[].ammo evaluates to am_noammo, then it is equal
//   to NUMAMMO+1. In the player struct we find:
//
//   int ammo[NUMAMMO];
//   int maxammo[NUMAMMO];
//
//   It is indexing 2 past the end of ammo into maxammo, and the
//   second ammo type just happens to be shells. How funny, eh?
//   All the functions below are fixed to check for this, optioned 
//   with demo compatibility.

//
// A_Punch
//

void A_Punch(player_t *player, pspdef_t *psp)
{
   angle_t angle;
   int slope, damage = (P_Random(pr_punch) % 10 + 1) << 1;
   
   if(player->powers[pw_strength])
      damage *= 10;
   
   angle = player->mo->angle;

   // haleyjd 08/05/04: use new function
   angle += P_SubRandom(pr_punchangle) << 18;

  // killough 8/2/98: make autoaiming prefer enemies
   if(demo_version<203 ||
      (slope = P_AimLineAttack(player->mo, angle, MELEERANGE, MF_FRIEND),
       !linetarget))
      slope = P_AimLineAttack(player->mo, angle, MELEERANGE, 0);

   P_LineAttack(player->mo, angle, MELEERANGE, slope, damage);

   if(!linetarget)
      return;
   
   S_StartSound(player->mo, sfx_punch);

   // turn to face target
   
   player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y,
                                       linetarget->x, linetarget->y);
}

//
// A_Saw
//

void A_Saw(player_t *player, pspdef_t *psp)
{
   int slope, damage = 2*(P_Random(pr_saw)%10+1);
   angle_t angle = player->mo->angle;
   
   // haleyjd 08/05/04: use new function
   angle += P_SubRandom(pr_saw) << 18;

   // Use meleerange + 1 so that the puff doesn't skip the flash
   
   // killough 8/2/98: make autoaiming prefer enemies
   if(demo_version<203 ||
      (slope = P_AimLineAttack(player->mo, angle, MELEERANGE+1, MF_FRIEND),
       !linetarget))
      slope = P_AimLineAttack(player->mo, angle, MELEERANGE+1, 0);

   P_LineAttack(player->mo, angle, MELEERANGE+1, slope, damage);
   
   if(!linetarget)
   {
      S_StartSound(player->mo, sfx_sawful);
      return;
   }

   S_StartSound(player->mo, sfx_sawhit);
   
   // turn to face target
   angle = R_PointToAngle2(player->mo->x, player->mo->y,
                           linetarget->x, linetarget->y);

   if(angle - player->mo->angle > ANG180)
   {
      if(angle - player->mo->angle < -ANG90/20)
         player->mo->angle = angle + ANG90/21;
      else
         player->mo->angle -= ANG90/20;
   }
   else
   {
      if(angle - player->mo->angle > ANG90/20)
         player->mo->angle = angle - ANG90/21;
      else
         player->mo->angle += ANG90/20;
   }

   player->mo->flags |= MF_JUSTATTACKED;
}

//
// A_FireMissile
//

void A_FireMissile(player_t *player, pspdef_t *psp)
{
   if((demo_version < 329 || 
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo]--;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   
   P_SpawnPlayerMissile(player->mo, E_SafeThingType(MT_ROCKET));
}

//
// A_FireBFG
//

#define BFGBOUNCE 16

void A_FireBFG(player_t *player, pspdef_t *psp)
{
   mobj_t *mo;
   
   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo] -= BFGCELLS;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   mo = P_SpawnPlayerMissile(player->mo, E_SafeThingType(MT_BFG));
   mo->extradata.bfgcount = BFGBOUNCE;   // for bouncing bfg - redundant
}

//
// A_FireOldBFG
//
// This function emulates Doom's Pre-Beta BFG
// By Lee Killough 6/6/98, 7/11/98, 7/19/98, 8/20/98
//
// This code may not be used in other mods without appropriate credit given.
// Code leeches will be telefragged.

void A_FireOldBFG(player_t *player, pspdef_t *psp)
{
   static int type1  = -1;
   static int type2 = -1;
   int type;

   if(type1 == -1)
   {
      type1 = E_SafeThingType(MT_PLASMA1);
      type2 = E_SafeThingType(MT_PLASMA2);
   }

   type = type1;
   
   // sf: make sure the player is in firing frame, or it looks silly
   if(demo_version > 300)
      P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   if(weapon_recoil && !(player->mo->flags & MF_NOCLIP))
      P_Thrust(player, ANG180 + player->mo->angle,
               512*recoil_values[wp_plasma]);

   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      player->ammo[weaponinfo[player->readyweapon].ammo]--;
   }

   if(LevelInfo.useFullBright) // haleyjd
      player->extralight = 2;

   do
   {
      mobj_t *th, *mo = player->mo;
      angle_t an = mo->angle;
      angle_t an1 = ((P_Random(pr_bfg)&127) - 64) * (ANG90/768) + an;
      angle_t an2 = ((P_Random(pr_bfg)&127) - 64) * (ANG90/640) + ANG90;
      extern int autoaim;
      fixed_t slope;

      if(autoaim)
      {
         // killough 8/2/98: make autoaiming prefer enemies
         int mask = MF_FRIEND;
         do
         {
            slope = P_AimLineAttack(mo, an, 16*64*FRACUNIT, mask);
            if(!linetarget)
               slope = P_AimLineAttack(mo, an += 1<<26, 16*64*FRACUNIT, mask);
            if(!linetarget)
               slope = P_AimLineAttack(mo, an -= 2<<26, 16*64*FRACUNIT, mask);
            if(!linetarget) // sf: looking up/down
            {
               slope = finetangent[(ANG90-player->pitch)>>ANGLETOFINESHIFT];
               an = mo->angle;
            }
         }
         while(mask && (mask=0, !linetarget));     // killough 8/2/98
         an1 += an - mo->angle;
         // sf: despite killough's infinite wisdom.. even
         // he is prone to mistakes. seems negative numbers
         // won't survive a bitshift!
         if(slope < 0 && demo_version >= 303)
            an2 -= tantoangle[-slope >> DBITS];
         else
            an2 += tantoangle[slope >> DBITS];
      }
      else
      {
         slope = finetangent[(ANG90-player->pitch)>>ANGLETOFINESHIFT];
         if(slope < 0 && demo_version >= 303)
            an2 -= tantoangle[-slope >> DBITS];
         else
            an2 += tantoangle[slope >> DBITS];
      }

      th = P_SpawnMobj(mo->x, mo->y,
                       mo->z + 62*FRACUNIT - player->psprites[ps_weapon].sy,
                       type);

      P_SetTarget(&th->target, mo);
      th->angle = an1;
      th->momx = finecosine[an1>>ANGLETOFINESHIFT] * 25;
      th->momy = finesine[an1>>ANGLETOFINESHIFT] * 25;
      th->momz = finetangent[an2>>ANGLETOFINESHIFT] * 25;
      P_CheckMissileSpawn(th);
   }
   while((type != type2) && (type = type2)); //killough: obfuscated!
}

//
// A_FirePlasma
//

void A_FirePlasma(player_t *player, pspdef_t *psp)
{
   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo]--;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   
   A_FireSomething(player, P_Random(pr_plasma) & 1);
   
   // sf: removed beta
   P_SpawnPlayerMissile(player->mo, E_SafeThingType(MT_PLASMA));
}

//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//

static fixed_t bulletslope;

static void P_BulletSlope(mobj_t *mo)
{
   angle_t an = mo->angle;    // see which target is to be aimed at
   
   // killough 8/2/98: make autoaiming prefer enemies
   int mask = demo_version < 203 ? 0 : MF_FRIEND;
   
   do
   {
      bulletslope = P_AimLineAttack(mo, an, 16*64*FRACUNIT, mask);
      if(!linetarget)
         bulletslope = P_AimLineAttack(mo, an += 1<<26, 16*64*FRACUNIT, mask);
      if(!linetarget)
         bulletslope = P_AimLineAttack(mo, an -= 2<<26, 16*64*FRACUNIT, mask);
   }
   while (mask && (mask=0, !linetarget));  // killough 8/2/98
}

//
// P_GunShot
//

void P_GunShot(mobj_t *mo, boolean accurate)
{
   int damage = 5 * (P_Random(pr_gunshot) % 3 + 1);
   angle_t angle = mo->angle;
   
   if(!accurate)
      angle += P_SubRandom(pr_misfire) << 18;
   
   P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage);
}

//
// A_FirePistol
//

void A_FirePistol(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_pistol);
   
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   if((demo_version < 329 || 
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO))
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo]--;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   
   A_FireSomething(player, 0); // phares
   P_BulletSlope(player->mo);
   P_GunShot(player->mo, !player->refire);
}

//
// A_FireShotgun
//

void A_FireShotgun(player_t *player, pspdef_t *psp)
{
   int i;
   
   S_StartSound(player->mo, sfx_shotgn);
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo]--;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   
   A_FireSomething(player,0);                                      // phares
   
   P_BulletSlope(player->mo);
   
   for(i=0; i<7; i++)
      P_GunShot(player->mo, false);
}

//
// A_FireShotgun2
//

void A_FireShotgun2(player_t *player, pspdef_t *psp)
{
   int i;
   
   S_StartSound(player->mo, sfx_dshtgn);
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo] -= 2;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }
   
   A_FireSomething(player,0);                                      // phares
   
   P_BulletSlope(player->mo);
   
   for(i = 0; i < 20; ++i)
   {
      int damage = 5 * (P_Random(pr_shotgun) % 3 + 1);
      angle_t angle = player->mo->angle;

      angle += P_SubRandom(pr_shotgun) << 19;
      
      P_LineAttack(player->mo, angle, MISSILERANGE, bulletslope +
                   (P_SubRandom(pr_shotgun) << 5), damage);
   }
}

//
// A_FireCGun
//

void A_FireCGun(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_chgun);

   if(!player->ammo[weaponinfo[player->readyweapon].ammo])
      return;
   
   // sf: removed beta
   
   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));
   
   if((demo_version < 329 ||
       weaponinfo[player->readyweapon].ammo < NUMAMMO) &&
      !(player->cheats & CF_INFAMMO)) // haleyjd
   {
      if(!weaponinfo[player->readyweapon].enableaps)
      {
         player->ammo[weaponinfo[player->readyweapon].ammo]--;
      }
      else
      {
         SUBTRACTAMMO();
      }
   }

   // haleyjd 08/28/03: this is not safe for DeHackEd/EDF, so it
   // needs some modification to be safer
   // haleyjd FIXME: hackish and dangerous for EDF, needs fix.
   if(demo_version < 331 || 
      ((psp->state - states) >= E_StateNumForDEHNum(S_CHAIN1) &&
       (psp->state - states) < E_StateNumForDEHNum(S_CHAIN3)))
   {      
      // phares
      A_FireSomething(player,psp->state - &states[E_SafeState(S_CHAIN1)]);
   }
   else
      A_FireSomething(player, 0); // new default behavior
   
   P_BulletSlope(player->mo);
   
   P_GunShot(player->mo, !player->refire);
}

void A_Light0(player_t *player, pspdef_t *psp)
{
   player->extralight = 0;
}

void A_Light1 (player_t *player, pspdef_t *psp)
{
   if(LevelInfo.useFullBright) // haleyjd
      player->extralight = 1;
}

void A_Light2 (player_t *player, pspdef_t *psp)
{
   if(LevelInfo.useFullBright) // haleyjd
      player->extralight = 2;
}

void A_BouncingBFG(mobj_t *mo);
void A_BFG11KHit(mobj_t *mo);
void A_BFGBurst(mobj_t *mo); // haleyjd

//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//

void A_BFGSpray(mobj_t *mo)
{
   int i;

   switch(bfgtype)
   {
   case bfg_11k:
      A_BFG11KHit(mo);
      return;
   case bfg_bouncing:
      A_BouncingBFG(mo);
      return;
   case bfg_burst:
      A_BFGBurst(mo);
      return;
   default:
      break;
   }
   
   for(i = 0; i < 40; i++)  // offset angles from its attack angle
   {
      int j, damage;
      angle_t an = mo->angle - ANG90/2 + ANG90/40*i;
      
      // mo->target is the originator (player) of the missile
      
      // killough 8/2/98: make autoaiming prefer enemies
      if(demo_version < 203 || 
         (P_AimLineAttack(mo->target, an, 16*64*FRACUNIT, MF_FRIEND), 
         !linetarget))
         P_AimLineAttack(mo->target, an, 16*64*FRACUNIT, 0);
      
      if(!linetarget)
         continue;
      
      P_SpawnMobj(linetarget->x, linetarget->y,
                  linetarget->z + (linetarget->height>>2), 
                  E_SafeThingType(MT_EXTRABFG));
      
      for(damage = j = 0; j < 15; j++)
         damage += (P_Random(pr_bfg)&7) + 1;
      
      P_DamageMobj(linetarget, mo->target, mo->target, damage,
                   MOD_BFG_SPLASH);
   }
}

        /********* Bouncing BFG Code ********/
void A_BouncingBFG(mobj_t *mo)
{
   int i;
   mobj_t *newmo;
   
   if(!mo->extradata.bfgcount)
      return;
   
   for(i = 0 ; i < 40 ; i++)  // offset angles from its attack angle
   {
      angle_t an2, an = (ANG360/40)*i;
      int dist;
      
      P_AimLineAttack(mo, an, 16*64*FRACUNIT,0);
      
      // haleyjd: track last target with mo->tracer, don't fire
      // at same target more than one time in a row
      if(!linetarget || (mo->tracer && mo->tracer == linetarget))
         continue;
      if(an/6 == mo->angle/6) continue;
      
      // don't aim for shooter, or for friends of shooter
      if(linetarget == mo->target ||
         (linetarget->flags & mo->target->flags & MF_FRIEND))
         continue; 
      
      P_SpawnMobj(linetarget->x, linetarget->y,
                  linetarget->z + (linetarget->height>>2),
                  E_SafeThingType(MT_EXTRABFG));

      // spawn new bfg      
      // haleyjd: can't use P_SpawnMissile here
      newmo = P_SpawnMobj(mo->x, mo->y, mo->z, E_SafeThingType(MT_BFG));
      S_StartSound(newmo, newmo->info->seesound);
      P_SetTarget(&newmo->target, mo->target); // pass on the player
      an2 = R_PointToAngle2(newmo->x, newmo->y, linetarget->x, linetarget->y);
      newmo->angle = an2;
      
      an2 >>= ANGLETOFINESHIFT;
      newmo->momx = FixedMul(newmo->info->speed, finecosine[an2]);
      newmo->momy = FixedMul(newmo->info->speed, finesine[an2]);

      dist = P_AproxDistance(linetarget->x - newmo->x, 
                             linetarget->y - newmo->y);
      dist = dist / newmo->info->speed;
      
      if(dist < 1)
         dist = 1;
      
      newmo->momz = 
         (linetarget->z + (linetarget->height>>1) - newmo->z) / dist;

      newmo->extradata.bfgcount = mo->extradata.bfgcount - 1; // count down
      P_SetTarget(&newmo->tracer, linetarget); // haleyjd: track target

      P_CheckMissileSpawn(newmo);

      P_RemoveMobj(mo); // remove the old one

      break; //only spawn 1
   }
}

// when the BFG 11K hits the wall or whatever
void A_BFG11KHit(mobj_t *mo)
{
   int i = 0;
   int j, damage;
   long origdist;
   
   // check the originator and hurt them if too close
   
   if((origdist = R_PointToDist2(mo->target->x, mo->target->y, mo->x, mo->y))
      < 96*FRACUNIT)
   {
      // decide on damage
      // damage decreases with distance
      for(damage = j = 0; j < 48 - (origdist/(FRACUNIT*2)); j++)
         damage += (P_Random(pr_bfg)&7) + 1;
      
      // flash
      P_SpawnMobj(mo->target->x, mo->target->y,
                  mo->target->z + (mo->target->height>>2), 
                  E_SafeThingType(MT_EXTRABFG));
      
      P_DamageMobj(mo->target, mo, mo->target, damage, 
                   MOD_BFG11K_SPLASH);
   }
   
   // now check everyone else
   
   for(i = 0 ; i < 40 ; i++)  // offset angles from its attack angle
   {
      angle_t an = (ANG360/40)*i;
      
      // mo->target is the originator (player) of the missile
      
      P_AimLineAttack(mo, an, 16*64*FRACUNIT,0);
      
      if(!linetarget) continue;
      if(linetarget == mo->target)
         continue;
      
      // decide on damage
      for(damage = j = 0; j < 20; j++)
         damage += (P_Random(pr_bfg)&7) + 1;
      
      // dumbass flash
      P_SpawnMobj(linetarget->x, linetarget->y,
                  linetarget->z + (linetarget->height>>2), 
                  E_SafeThingType(MT_EXTRABFG));
      
      P_DamageMobj(linetarget, mo->target, mo->target, damage,
                   MOD_BFG_SPLASH);
   }
}

//
// A_BFGBurst
//
// This implements the projectile burst that was featured
// in Cephaler's unreleased MagDoom port. Much thanks to my
// long lost friend for giving me the source to his port back
// when he stopped working on it. This is a tribute to his bold
// spirit ^_^
//
void A_BFGBurst(mobj_t *mo)
{
   int a;
   angle_t an = 0;
   mobj_t *th;
   static int plasmaType = -1;
   
   if(plasmaType == -1)
      plasmaType = E_SafeThingType(MT_PLASMA3);

   for(a = 0; a < 40; a++)
   {
      an += ANG90 / 10;

      th = P_SpawnMobj(mo->x, mo->y, mo->z, plasmaType);
      P_SetTarget(&th->target, mo->target);

      th->angle = an;
      th->momx = finecosine[an >> ANGLETOFINESHIFT] << 4;
      th->momy = finesine[an >> ANGLETOFINESHIFT] << 4;
      th->momz = FRACUNIT * ((16 - P_Random(pr_bfg)) >> 5);

      P_CheckMissileSpawn(th);      
   }
}

//
// A_BFGsound
//

void A_BFGsound(player_t *player, pspdef_t *psp)
{
   S_StartSound(player->mo, sfx_bfg);
}

//
// P_SetupPsprites
// Called at start of level for each player.
//

void P_SetupPsprites(player_t *player)
{
   int i;
   
   // remove all psprites
   for(i=0; i<NUMPSPRITES; i++)
      player->psprites[i].state = NULL;
   
   // spawn the gun
   player->pendingweapon = player->readyweapon;
   P_BringUpWeapon(player);
}

//
// P_MovePsprites
// Called every tic by player thinking routine.
//

void P_MovePsprites(player_t *player)
{
   pspdef_t *psp = player->psprites;
   int i;

   // a null state means not active
   // drop tic count and possibly change state
   // a -1 tic count never changes
   
   for(i=0; i<NUMPSPRITES; i++, psp++)
   {
      if(psp->state && psp->tics != -1 && !--psp->tics)
         P_SetPsprite(player, i, psp->state->nextstate);
   }
   
   player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
   player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}

//===============================
//
// New Eternity Weapon Functions
//
//===============================

void A_FireGrenade(player_t *player, pspdef_t *psp)
{
   if(weaponinfo[player->readyweapon].ammo < NUMAMMO &&
      !(player->cheats & CF_INFAMMO))
   {
      SUBTRACTAMMO();
   }

   P_SpawnPlayerMissile(player->mo, E_SafeThingType(MT_GRENADE));
}

//
// A_FireCustomBullets
//
// A parameterized player bullet weapon code pointer
// Parameters:
// args[0] : sound (dehacked num)
// args[1] : accuracy (always, first fire only, never, ssg, monster)
// args[2] : number of bullets to fire
// args[3] : damage factor of bullets
// args[4] : damage modulus of bullets
//
void A_FireCustomBullets(player_t *player, pspdef_t *psp)
{
   int i, sound, accurate, numbullets, damage, dmgmod;
   sfxinfo_t *sfx;

   sound      = (int)(psp->state->args[0]);
   accurate   = (int)(psp->state->args[1]);
   numbullets = (int)(psp->state->args[2]);
   damage     = (int)(psp->state->args[3]);
   dmgmod     = (int)(psp->state->args[4]);

   if(!accurate)
      accurate = 1;

   if(dmgmod < 1)
      dmgmod = 1;
   else if(dmgmod > 256)
      dmgmod = 256;

   // haleyjd 12/08/03: changed to use sound dehacked num
   sfx = E_SoundForDEHNum(sound);

   S_StartSfxInfo(player->mo, sfx);

   P_SetMobjState(player->mo, E_SafeState(S_PLAY_ATK2));

   // subtract ammo amount
   if(weaponinfo[player->readyweapon].ammo < NUMAMMO &&
      !(player->cheats & CF_INFAMMO))
   {
      // now settable in weapon, not needed as a parameter here
      SUBTRACTAMMO();
   }

   A_FireSomething(player, 0);
   P_BulletSlope(player->mo);

   // loop on numbullets
   for(i = 0; i < numbullets; ++i)
   {
      int dmg = damage * (P_Random(pr_custombullets)%dmgmod + 1);
      angle_t angle = player->mo->angle;
      
      if(accurate <= 3 || accurate == 5)
      {
         // if never accurate, monster accurate, or accurate only on 
         // refire and player is refiring, add some to the angle
         if(accurate == 3 || accurate == 5 || 
            (accurate == 2 && player->refire))
         {
            int aimshift = ((accurate == 5) ? 20 : 18);
            angle += P_SubRandom(pr_custommisfire) << aimshift;
         }

         P_LineAttack(player->mo, angle, MISSILERANGE, bulletslope, 
                      dmg);
      }
      else if(accurate == 4) // ssg spread
      {
         angle += P_SubRandom(pr_custommisfire) << 19;
         bulletslope += P_SubRandom(pr_custommisfire) << 5;

         P_LineAttack(player->mo, angle, MISSILERANGE, 
                      bulletslope, dmg);
      }
   }
}

//
// A_FirePlayerMissile
//
// A parameterized code pointer function for custom missiles
// Parameters:
// args[0] : thing type to shoot
// args[1] : whether or not to home at current autoaim target
//           (missile requires homing maintenance pointers, however)
//
void A_FirePlayerMissile(player_t *player, pspdef_t *psp)
{
   int thingnum;
   mobj_t *mo;
   boolean seek;

   //. haleyjd 07/05/03: adjusted for EDF
   thingnum = (int)(psp->state->args[0]); 
   seek = !!((int)(psp->state->args[1]));

   thingnum = E_ThingNumForDEHNum(thingnum);
   if(thingnum == NUMMOBJTYPES)
      return;

   // decrement ammo if appropriate
   if(weaponinfo[player->readyweapon].ammo < NUMAMMO &&
      !(player->cheats & CF_INFAMMO))
   {
      SUBTRACTAMMO();
   }

   mo = P_SpawnPlayerMissile(player->mo, thingnum);

   if(mo && seek)
   {
      P_BulletSlope(player->mo);

      if(linetarget)
         P_SetTarget(&mo->tracer, linetarget);
   }
}

//
// A_CustomPlayerMelee
//
// A parameterized melee attack codepointer function
// Parameters:
// args[0] : damage factor
// args[1] : damage modulus
// args[2] : berzerk multiplier
// args[3] : angle deflection type (none, punch, chainsaw)
// args[4] : sound to make (dehacked number)
//
void A_CustomPlayerMelee(player_t *player, pspdef_t *psp)
{
   angle_t angle;
   int slope, damage, dmgfactor, dmgmod, berzerkmul, deftype, sound;
   sfxinfo_t *sfx;

   dmgfactor  = (int)(psp->state->args[0]);
   dmgmod     = (int)(psp->state->args[1]);
   berzerkmul = (int)(psp->state->args[2]);
   deftype    = (int)(psp->state->args[3]);
   sound      = (int)(psp->state->args[4]);

   // adjust parameters

   if(dmgmod < 1)
      dmgmod = 1;
   else if(dmgmod > 256)
      dmgmod = 256;

   // haleyjd 12/08/03: changed sound parameter to DeHackEd number
   sfx = E_SoundForDEHNum(sound);

   // calculate damage
   damage = dmgfactor * ((P_Random(pr_custompunch)%dmgmod) + 1);

   // apply berzerk multiplier
   if(player->powers[pw_strength])
      damage *= berzerkmul;

   // decrement ammo if appropriate
   if(weaponinfo[player->readyweapon].ammo < NUMAMMO &&
      !(player->cheats & CF_INFAMMO))
   {
      SUBTRACTAMMO();
   }
   
   angle = player->mo->angle;
   
   if(deftype == 2 || deftype == 3)
      angle += P_SubRandom(pr_custompunch) << 18;
   
   if((slope = P_AimLineAttack(player->mo, angle, MELEERANGE, MF_FRIEND),
      !linetarget))
      slope = P_AimLineAttack(player->mo, angle, MELEERANGE, 0);
   
   P_LineAttack(player->mo, angle, MELEERANGE, slope, damage);
   
   if(!linetarget)
   {
      // assume they want sawful on miss if sawhit specified
      if(sound == sfx_sawhit)
         S_StartSound(player->mo, sfx_sawful);
      return;
   }

   // start sound
   S_StartSfxInfo(player->mo, sfx);
   
   // turn to face target   
   player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y,
      linetarget->x, linetarget->y);

   // apply chainsaw deflection if selected
   if(deftype == 3)
   {
      if(angle - player->mo->angle > ANG180)
      {
         if (angle - player->mo->angle < -ANG90/20)
            player->mo->angle = angle + ANG90/21;
         else
            player->mo->angle -= ANG90/20;
      }
      else
      {
         if(angle - player->mo->angle > ANG90/20)
            player->mo->angle = angle - ANG90/21;
         else
            player->mo->angle += ANG90/20;
      }
      
      player->mo->flags |= MF_JUSTATTACKED;
   }
}

//
// A_PlayerThunk
//
// Allows the player's weapons to use a certain class of monster
// codepointers that are deemed safe (these have the BPF_PTHUNK
// flag in the deh_bexptrs array).
// Parameters:
// args[0] : index of codepointer to call (is validated)
// args[1] : boolean, 0 disables the FaceTarget cp for the player
// args[2] : dehacked num of a state to put the player in temporarily
// args[3] : boolean, 1 == set player's target to autoaim target
// args[4] : boolean, 1 == use ammo on current weapon if attack succeeds
//
void A_PlayerThunk(player_t *player, pspdef_t *psp)
{
   boolean face;
   boolean settarget;
   boolean useammo;
   int cptrnum, statenum;
   state_t *oldstate = 0;
   mobj_t *oldtarget = NULL, *localtarget = NULL;
   mobj_t *plyr = player->mo;

   cptrnum   =    (int)(psp->state->args[0]);
   face      = !!((int)(psp->state->args[1]));
   statenum  =    (int)(psp->state->args[2]);
   settarget = !!((int)(psp->state->args[3]));
   useammo   = !!((int)(psp->state->args[4]));

   // validate codepointer index
   if(cptrnum < 0 || cptrnum >= num_bexptrs)
      return;

   // make sure codepointer is thunkable
   if(!(deh_bexptrs[cptrnum].flags & BPF_PTHUNK))
      return;

   // validate and resolve state
   if(statenum >= 0)
   {
      statenum = E_StateNumForDEHNum(statenum);
      if(statenum == NUMSTATES)
         return;
   }

   // set player's target to thing being autoaimed at if this
   // behavior is requested.
   if(settarget)
   {
      // record old target
      oldtarget = plyr->target;
      P_BulletSlope(plyr);
      if(linetarget)
      {
         P_SetTarget(&(plyr->target), linetarget);
         localtarget = linetarget;
      }
      else
         return;
   }

   // possibly disable the FaceTarget pointer using MIF_NOFACE
   if(!face)
      plyr->intflags |= MIF_NOFACE;

   // If a state has been provided, place the player into it. This
   // allows use of parameterized codepointers.
   if(statenum >= 0)
   {
      oldstate = plyr->state;
      plyr->state = &states[statenum];
   }

   // if ammo should be used, subtract it now
   if(useammo)
   {
      if(weaponinfo[player->readyweapon].ammo < NUMAMMO &&
         !(player->cheats & CF_INFAMMO))
      {
         SUBTRACTAMMO();
      }
   }

   // execute the codepointer
   deh_bexptrs[cptrnum].cptr(player->mo);

   // remove MIF_NOFACE
   plyr->intflags &= ~MIF_NOFACE;

   // restore player's old target if a new one was found & set
   if(settarget && localtarget)
   {
      P_SetTarget(&(plyr->target), oldtarget);
   }

   // put player back into his normal state
   if(statenum >= 0)
   {
      plyr->state = oldstate;
   }
}

//----------------------------------------------------------------------------
//
// $Log: p_pspr.c,v $
// Revision 1.13  1998/05/07  00:53:36  killough
// Remove dependence on order of evaluation
//
// Revision 1.12  1998/05/05  16:29:17  phares
// Removed RECOIL and OPT_BOBBING defines
//
// Revision 1.11  1998/05/03  22:35:21  killough
// Fix weapons switch bug again, beautification, headers
//
// Revision 1.10  1998/04/29  10:01:55  killough
// Fix buggy weapons switch code
//
// Revision 1.9  1998/03/28  18:01:38  killough
// Prevent weapon recoil in no-clipping mode
//
// Revision 1.8  1998/03/23  03:28:29  killough
// Move weapons changes to G_BuildTiccmd()
//
// Revision 1.7  1998/03/10  07:14:47  jim
// Initial DEH support added, minus text
//
// Revision 1.6  1998/02/24  08:46:27  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.5  1998/02/17  05:59:41  killough
// Use new RNG calling sequence
//
// Revision 1.4  1998/02/15  02:47:54  phares
// User-defined keys
//
// Revision 1.3  1998/02/09  03:06:15  killough
// Add player weapon preference options
//
// Revision 1.2  1998/01/26  19:24:18  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
