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
//      Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_inter.c,v 1.10 1998/05/03 23:09:29 killough Exp $";

#include "c_io.h"
#include "doomstat.h"
#include "dstrings.h"
#include "m_random.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_frags.h"
#include "am_map.h"
#include "r_main.h"
#include "r_segs.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_tick.h"
#include "d_deh.h"  // Ty 03/22/98 - externalized strings
#include "d_player.h"
#include "p_inter.h"
#include "d_gi.h"
#include "g_dmflag.h"
#include "e_edf.h"
#include "e_states.h"
#include "p_map.h"
#include "a_small.h"

#define BONUSADD        6

// Ty 03/07/98 - add deh externals
// Maximums and such were hardcoded values.  Need to externalize those for
// dehacked support (and future flexibility).  Most var names came from the key
// strings used in dehacked.

int initial_health = 100;
int initial_bullets = 50;
int maxhealth = 100; // was MAXHEALTH as a #define, used only in this module
int max_armor = 200;
int green_armor_class = 1;  // these are involved with armortype below
int blue_armor_class = 2;
int max_soul = 200;
int soul_health = 100;
int mega_health = 200;
int god_health = 100;   // these are used in cheats (see st_stuff.c)
int idfa_armor = 200;
int idfa_armor_class = 2;
// not actually used due to pairing of cheat_k and cheat_fa
int idkfa_armor = 200;
int idkfa_armor_class = 2;

int bfgcells = 40;      // used in p_pspr.c
// Ty 03/07/98 - end deh externals

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO]  = {200, 50, 300, 50};
int clipammo[NUMAMMO] = { 10,  4,  20,  1};

//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

boolean P_GiveAmmo(player_t *player, ammotype_t ammo, int num)
{
   int oldammo;
   
   if(ammo == am_noammo)
      return false;
   
   // haleyjd: Part of the campaign to eliminate unnecessary
   //   I_Error bombouts for things that can be ignored safely
   if((unsigned) ammo > NUMAMMO)
   {
      C_Printf(FC_ERROR"P_GiveAmmo: bad type %i\a\n", ammo);
      return false;
   }

   if(player->ammo[ammo] == player->maxammo[ammo])
      return false;

   if(num)
      num *= clipammo[ammo];
   else
      num = clipammo[ammo]/2;

   // give double ammo in trainer mode, you'll need in nightmare
   if(gameskill == sk_baby || gameskill == sk_nightmare)
      num <<= 1;

   oldammo = player->ammo[ammo];
   player->ammo[ammo] += num;
   
   if(player->ammo[ammo] > player->maxammo[ammo])
      player->ammo[ammo] = player->maxammo[ammo];
   
   // If non zero ammo, don't change up weapons, player was lower on purpose.
   if(oldammo)
      return true;

   // We were down to zero, so select a new weapon.
   // Preferences are not user selectable.
   
   switch (ammo)
   {
   case am_clip:
      if(player->readyweapon == wp_fist)
      {
         if(player->weaponowned[wp_chaingun])
            player->pendingweapon = wp_chaingun;
         else
            player->pendingweapon = wp_pistol;
      }
      break;

   case am_shell:
      if(player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
         if(player->weaponowned[wp_shotgun])
            player->pendingweapon = wp_shotgun;
      break;

   case am_cell:
      if(player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
         if(player->weaponowned[wp_plasma])
            player->pendingweapon = wp_plasma;
      break;

   case am_misl:
      if(player->readyweapon == wp_fist)
         if(player->weaponowned[wp_missile])
            player->pendingweapon = wp_missile;
      break;

   default:
      break;
   }
   return true;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//

boolean P_GiveWeapon(player_t *player, weapontype_t weapon, boolean dropped)
{
   boolean gaveammo;
   
   if((dmflags & DM_WEAPONSTAY) && !dropped)
   {
      // leave placed weapons forever on net games
      if(player->weaponowned[weapon])
         return false;

      player->bonuscount += BONUSADD;
      player->weaponowned[weapon] = true;
      
      P_GiveAmmo(player, weaponinfo[weapon].ammo, 
                 (GameType == gt_dm) ? 5 : 2);
      
      player->pendingweapon = weapon;
      S_StartSound(player->mo, sfx_wpnup); // killough 4/25/98, 12/98
      return false;
   }

   // give one clip with a dropped weapon, two clips with a found weapon
   gaveammo = weaponinfo[weapon].ammo != am_noammo &&
      P_GiveAmmo(player, weaponinfo[weapon].ammo, dropped ? 1 : 2);
   
   return !player->weaponowned[weapon] ?
      player->weaponowned[player->pendingweapon = weapon] = true : gaveammo;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//

boolean P_GiveBody(player_t *player, int num)
{
   if(player->health >= maxhealth)
      return false; // Ty 03/09/98 externalized MAXHEALTH to maxhealth
   player->health += num;
   if(player->health > maxhealth)
      player->health = maxhealth;
   player->mo->health = player->health;
   return true;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//

boolean P_GiveArmor(player_t *player, int armortype, boolean htic)
{
   int hits = armortype*100;

   if(player->armorpoints >= hits)
      return false;   // don't pick up

   player->armortype = armortype;
   player->armorpoints = hits;
   
   // haleyjd 10/10/02
   player->hereticarmor = player->hereticarmor || htic;
   
   return true;
}

//
// P_GiveCard
//

void P_GiveCard(player_t *player, card_t card)
{
   if(player->cards[card])
      return;
   player->bonuscount = BONUSADD;
   player->cards[card] = 1;
}

//
// P_GivePower
//
// Rewritten by Lee Killough
//

boolean P_GivePower(player_t *player, int power)
{
   static const int tics[NUMPOWERS] = 
   {
      INVULNTICS,
      1,          /* strength */
      INVISTICS,
      IRONTICS, 
      1,          /* allmap */
      INFRATICS,
      INVISTICS,  /* haleyjd: totalinvis */
      INVISTICS,  /* haleyjd: ghost */
      1,          /* haleyjd: silencer */
   };

   switch(power)
   {
   case pw_invisibility:
      player->mo->flags |= MF_SHADOW;
      break;
   case pw_allmap:
      if(player->powers[pw_allmap])
         return false;
      break;
   case pw_strength:
      P_GiveBody(player,100);
      break;
   case pw_totalinvis:   // haleyjd: total invisibility
      player->mo->flags2 |= MF2_DONTDRAW;
      break;
   case pw_ghost:        // haleyjd: heretic ghost
      player->mo->flags3 |= MF3_GHOST;
      break;
   case pw_silencer:
      if(player->powers[pw_silencer])
         return false;
      break;
   }

   // Unless player has infinite duration cheat, set duration (killough)
   
   if(player->powers[power] >= 0)
      player->powers[power] = tics[power];
   return true;
}

//
// P_TouchSpecialThing
//

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher)
{
   player_t *player;
   int      i;
   int      sound;
   char*    message = NULL;
   boolean  removeobj = true;
   boolean  pickup_fx = true; // haleyjd 04/14/03
   fixed_t  delta = special->z - toucher->z;

   if(delta > toucher->height || delta < -8*FRACUNIT)
      return;        // out of reach

   sound = sfx_itemup;

   // haleyjd: don't crash if a monster gets here.
   if(!(player = toucher->player))
      return;
   
   // Dead thing touching.
   // Can happen with a sliding player corpse.
   if(toucher->health <= 0)
      return;

   // haleyjd 05/11/03: EDF pickups modifications
   if(special->sprite >= NUMSPRITES)
      return;

   // Identify by sprite.
   switch(pickupfx[special->sprite])
   {
      // armor
   case PFX_GREENARMOR:
      if(!P_GiveArmor(player, green_armor_class, false))
         return;
      message = s_GOTARMOR; // Ty 03/22/98 - externalized
      break;

   case PFX_BLUEARMOR:
      if(!P_GiveArmor(player, blue_armor_class, false))
         return;
      message = s_GOTMEGA; // Ty 03/22/98 - externalized
      break;

      // bonus items
   case PFX_POTION:
      
      // sf: removed beta
      player->health++;               // can go over 100%
      if(player->health > (maxhealth * 2))
         player->health = (maxhealth * 2);
      player->mo->health = player->health;
      message = s_GOTHTHBONUS; // Ty 03/22/98 - externalized
      break;

   case PFX_ARMORBONUS:
      // sf: removed beta
      player->armorpoints++;          // can go over 100%
      if(player->armorpoints > max_armor)
         player->armorpoints = max_armor;
      if(!player->armortype)
         player->armortype = green_armor_class;
      message = s_GOTARMBONUS; // Ty 03/22/98 - externalized
      break;

      // sf: removed beta items
      
   case PFX_SOULSPHERE:
      player->health += soul_health;
      if(player->health > max_soul)
         player->health = max_soul;
      player->mo->health = player->health;
      message = s_GOTSUPER; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

   case PFX_MEGASPHERE:
      if(gamemode != commercial)
         return;
      player->health = mega_health;
      player->mo->health = player->health;
      P_GiveArmor(player,blue_armor_class, false);
      message = s_GOTMSPHERE; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

      // cards
      // leave cards for everyone
   case PFX_BLUEKEY:
      if(!player->cards[it_bluecard])
         message = s_GOTBLUECARD; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_bluecard);
      removeobj = pickup_fx = (GameType == gt_single);
      break;

   case PFX_YELLOWKEY:
      if(!player->cards[it_yellowcard])
         message = s_GOTYELWCARD; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_yellowcard);
      removeobj = pickup_fx = (GameType == gt_single);
      break;

   case PFX_REDKEY:
      if(!player->cards[it_redcard])
         message = s_GOTREDCARD; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_redcard);
      removeobj = pickup_fx = (GameType == gt_single);
      break;
      
   case PFX_BLUESKULL:
      if(!player->cards[it_blueskull])
         message = s_GOTBLUESKUL; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_blueskull);
      removeobj = pickup_fx = (GameType == gt_single);
      break;
      
   case PFX_YELLOWSKULL:
      if(!player->cards[it_yellowskull])
         message = s_GOTYELWSKUL; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_yellowskull);
      removeobj = pickup_fx = (GameType == gt_single);
      break;

   case PFX_REDSKULL:
      if(!player->cards[it_redskull])
         message = s_GOTREDSKULL; // Ty 03/22/98 - externalized
      P_GiveCard(player, it_redskull);
      removeobj = pickup_fx = (GameType == gt_single);
      break;

   // medikits, heals
   case PFX_STIMPACK:
      if(!P_GiveBody(player, 10))
         return;
      message = s_GOTSTIM; // Ty 03/22/98 - externalized
      break;
      
   case PFX_MEDIKIT:
      if(!P_GiveBody(player, 25))
         return;
      // sf: fix medineed (check for below 25, but medikit gives
      // 25, so always > 25)
      message = player->health < 50 ?     // was 25
          s_GOTMEDINEED : s_GOTMEDIKIT; // Ty 03/22/98 - externalized
      break;


      // power ups
   case PFX_INVULNSPHERE:
      if(!P_GivePower(player, pw_invulnerability))
         return;
      message = s_GOTINVUL; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

   case PFX_BERZERKBOX:
      if(!P_GivePower(player, pw_strength))
         return;
      message = s_GOTBERSERK; // Ty 03/22/98 - externalized
      if(player->readyweapon != wp_fist)
         // sf: removed beta
         player->pendingweapon = wp_fist;
      sound = sfx_getpow;
      break;

   case PFX_INVISISPHERE:
      if(!P_GivePower (player, pw_invisibility))
         return;
      message = s_GOTINVIS; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

   case PFX_RADSUIT:
      if(!P_GivePower(player, pw_ironfeet))
         return;
      // sf:removed beta
      
      message = s_GOTSUIT; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

   case PFX_ALLMAP:
      if(!P_GivePower(player, pw_allmap))
         return;
      message = s_GOTMAP; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

   case PFX_LIGHTAMP:
      
      if(!P_GivePower(player, pw_infrared))
         return;
      // sf:removed beta
      sound = sfx_getpow;
      message = s_GOTVISOR; // Ty 03/22/98 - externalized
      break;

      // ammo
   case PFX_CLIP:
      if(special->flags & MF_DROPPED)
      {
         if(!P_GiveAmmo(player,am_clip,0))
            return;
      }
      else
      {
         if(!P_GiveAmmo(player,am_clip,1))
            return;
      }
      message = s_GOTCLIP; // Ty 03/22/98 - externalized
      break;

   case PFX_CLIPBOX:
      if(!P_GiveAmmo(player, am_clip,5))
         return;
      message = s_GOTCLIPBOX; // Ty 03/22/98 - externalized
      break;

   case PFX_ROCKET:
      if(!P_GiveAmmo(player, am_misl,1))
         return;
      message = s_GOTROCKET; // Ty 03/22/98 - externalized
      break;

   case PFX_ROCKETBOX:
      if(!P_GiveAmmo(player, am_misl,5))
         return;
      message = s_GOTROCKBOX; // Ty 03/22/98 - externalized
      break;

   case PFX_CELL:
      if(!P_GiveAmmo(player, am_cell,1))
         return;
      message = s_GOTCELL; // Ty 03/22/98 - externalized
      break;

   case PFX_CELLPACK:
      if(!P_GiveAmmo(player, am_cell,5))
         return;
      message = s_GOTCELLBOX; // Ty 03/22/98 - externalized
      break;
      
   case PFX_SHELL:
      if(!P_GiveAmmo(player, am_shell,1))
         return;
      message = s_GOTSHELLS; // Ty 03/22/98 - externalized
      break;
      
   case PFX_SHELLBOX:
      if(!P_GiveAmmo(player, am_shell,5))
         return;
      message = s_GOTSHELLBOX; // Ty 03/22/98 - externalized
      break;

   case PFX_BACKPACK:
      if(!player->backpack)
      {
         for (i=0 ; i<NUMAMMO ; i++)
            player->maxammo[i] *= 2;
         player->backpack = true;
      }
      // EDF FIXME: needs a ton of work
#if 0
      if(special->flags & MF_DROPPED)
      {
         int i;
         for(i=0 ; i<NUMAMMO ; i++)
         {
            player->ammo[i] +=special->extradata.backpack->ammo[i];
            if(player->ammo[i]>player->maxammo[i])
               player->ammo[i]=player->maxammo[i];
         }
         P_GiveWeapon(player,special->extradata.backpack->weapon,true);
         Z_Free(special->extradata.backpack);
         message = "got player backpack";
      }
      else
#endif
      {
         for(i=0 ; i<NUMAMMO ; i++)
            P_GiveAmmo (player, i, 1);
         message = s_GOTBACKPACK; // Ty 03/22/98 - externalized
      }

      break;

      // weapons
   case PFX_BFG:
      if (!P_GiveWeapon (player, wp_bfg, false) )
         return;
      message = bfgtype==0 ? s_GOTBFG9000       // sf
                : bfgtype==1 ? "You got the BFG 2704!"
                : bfgtype==2 ? "You got the BFG 11K!"
                : bfgtype==3 ? "You got the Bouncing BFG!"
                : bfgtype==4 ? "You got the Plasma Burst BFG!"
                : "You got some kind of BFG";
      sound = sfx_wpnup;
      break;

   case PFX_CHAINGUN:
      if(!P_GiveWeapon(player, wp_chaingun, special->flags & MF_DROPPED))
         return;
      message = s_GOTCHAINGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

   case PFX_CHAINSAW:
      if(!P_GiveWeapon(player, wp_chainsaw, false))
         return;
      message = s_GOTCHAINSAW; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

   case PFX_LAUNCHER:
      if(!P_GiveWeapon(player, wp_missile, false) )
         return;
      message = s_GOTLAUNCHER; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

   case PFX_PLASMA:
      if(!P_GiveWeapon(player, wp_plasma, false))
         return;
      message = s_GOTPLASMA; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

   case PFX_SHOTGUN:
      if(!P_GiveWeapon(player, wp_shotgun, special->flags & MF_DROPPED))
         return;
      message = s_GOTSHOTGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

   case PFX_SSG:
      if(!P_GiveWeapon(player, wp_supershotgun, special->flags & MF_DROPPED))
         return;
      message = s_GOTSHOTGUN2; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

      // haleyjd 10/10/02: Heretic powerups

      // heretic keys: give both card and skull equivalent DOOM keys
   case PFX_HGREENKEY: // green key (red in doom)
      if(!player->cards[it_redcard])
         message = s_HGOTGREENKEY;
      P_GiveCard(player, it_redcard);
      P_GiveCard(player, it_redskull);
      removeobj = pickup_fx = (GameType == gt_single);
      sound = sfx_keyup;
      break;

   case PFX_HBLUEKEY: // blue key (blue in doom)
      if(!player->cards[it_bluecard])
         message = s_HGOTBLUEKEY;
      P_GiveCard(player, it_bluecard);
      P_GiveCard(player, it_blueskull);
      removeobj = pickup_fx = (GameType == gt_single);
      sound = sfx_keyup;
      break;

   case PFX_HYELLOWKEY: // yellow key (yellow in doom)
      if(!player->cards[it_yellowcard])
         message = s_HGOTYELLOWKEY;
      P_GiveCard(player, it_yellowcard);
      P_GiveCard(player, it_yellowskull);
      removeobj = pickup_fx = (GameType == gt_single);
      sound = sfx_keyup;
      break;

   case PFX_HPOTION: // heretic potion
      if(!P_GiveBody(player, 10))
         return;
      message = s_HITEMHEALTH;
      sound = sfx_hitemup;
      break;

   case PFX_SILVERSHIELD: // heretic shield 1
      if(!P_GiveArmor(player, 1, true))
         return;
      message = s_HITEMSHIELD1;
      sound = sfx_hitemup;
      break;

   case PFX_ENCHANTEDSHIELD: // heretic shield 2
      if(!P_GiveArmor(player, 2, true))
         return;
      message = s_HITEMSHIELD2;
      sound = sfx_hitemup;
      break;

   case PFX_BAGOFHOLDING: // bag of holding
      // HTIC_TODO: bag of holding effects
      message = s_HITEMBAGOFHOLDING;
      sound = sfx_hitemup;
      break;

   case PFX_HMAP: // map scroll
      if(!P_GivePower(player, pw_allmap))
         return;
      message = s_HITEMSUPERMAP;
      sound = sfx_hitemup;
      break;
   
      // Heretic Ammo items
   case PFX_GWNDWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOGOLDWAND1;
      sound = sfx_hitemup;
      break;
   
   case PFX_GWNDHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOGOLDWAND2;
      sound = sfx_hitemup;
      break;
   
   case PFX_MACEWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOMACE1;
      sound = sfx_hitemup;
      break;
   
   case PFX_MACEHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOMACE2;
      sound = sfx_hitemup;
      break;
   
   case PFX_CBOWWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOCROSSBOW1;
      sound = sfx_hitemup;
      break;
   
   case PFX_CBOWHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOCROSSBOW2;
      sound = sfx_hitemup;
      break;
   
   case PFX_BLSRWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOBLASTER1;
      sound = sfx_hitemup;
      break;
   
   case PFX_BLSRHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOBLASTER2;
      sound = sfx_hitemup;
      break;
   
   case PFX_PHRDWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOPHOENIXROD1;
      sound = sfx_hitemup;
      break;
   
   case PFX_PHRDHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOPHOENIXROD2;
      sound = sfx_hitemup;
      break;
   
   case PFX_SKRDWIMPY:
      // HTIC_TODO: give ammo
      message = s_HAMMOSKULLROD1;
      sound = sfx_hitemup;
      break;
   
   case PFX_SKRDHEFTY:
      // HTIC_TODO: give ammo
      message = s_HAMMOSKULLROD2;
      sound = sfx_hitemup;
      break;

      // start new Eternity power-ups
   case PFX_TOTALINVIS:
      if(!P_GivePower(player, pw_totalinvis))
         return;
      message = "Total Invisibility!";
      sound = sfx_getpow;
      break;

   default:
      // I_Error("P_SpecialThing: Unknown gettable thing");
      return;      // killough 12/98: suppress error message
   }

   // sf: display message using player_printf
   if(message)
      player_printf(player, "%s", message);

   // haleyjd 07/08/05: rearranged to avoid removing before
   // checking for COUNTITEM flag.
   if(special->flags & MF_COUNTITEM)
      player->itemcount++;

   if(removeobj)
      P_RemoveMobj(special);

   // haleyjd 07/08/05: inverted condition
   if(pickup_fx)
   {
      player->bonuscount += BONUSADD;
      S_StartSound(player->mo, sound);   // killough 4/25/98, 12/98
   }
}

//
// P_KillMobj
//
void P_KillMobj(mobj_t *source, mobj_t *target)
{
   mobjtype_t item;
   mobj_t     *mo;

   target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);
   target->flags2 &= ~MF2_INVULNERABLE; // haleyjd 04/09/99
   
   if(!(target->flags3 & MF3_DEADFLOAT))
      target->flags &= ~MF_NOGRAVITY;

   target->flags |= MF_CORPSE|MF_DROPOFF;
   target->height >>= 2;

   // killough 8/29/98: remove from threaded list
   P_UpdateThinker(&target->thinker);
   
   if(source && source->player)
   {
      // count for intermission
      // killough 7/20/98: don't count friends
      if(!(target->flags & MF_FRIEND))
      {
         if(target->flags & MF_COUNTKILL)
            source->player->killcount++;
      }
      if(target->player)
      {
         source->player->frags[target->player-players]++;
         HU_FragsUpdate();
      }
   }
   else if(GameType == gt_single && (target->flags & MF_COUNTKILL))
   {
      // count all monster deaths,
      // even those caused by other monsters
      // killough 7/20/98: don't count friends
      if(!(target->flags & MF_FRIEND))
         players->killcount++;
   }

   if(target->player)
   {
      // count environment kills against you
      if(!source)
      {
         target->player->frags[target->player-players]++;
         HU_FragsUpdate();
      }

      target->flags &= ~MF_SOLID;
      target->player->playerstate = PST_DEAD;
      P_DropWeapon (target->player);

      if(target->player == &players[consoleplayer] && automapactive)
      {
         if(!demoplayback) // killough 11/98: don't switch out in demos, though
            AM_Stop();    // don't die in auto map; switch view prior to dying
      }
   }

   if(target->health < -target->info->spawnhealth &&
      target->info->xdeathstate)
      P_SetMobjState (target, target->info->xdeathstate);
   else
      P_SetMobjState (target, target->info->deathstate);

   target->tics -= P_Random(pr_killtics)&3;
   
   if (target->tics < 1)
      target->tics = 1;

   // Drop stuff.
   // This determines the kind of object spawned
   // during the death frame of a thing.

   if(target->info->droptype != -1)
   {
      if(target->player)
      {
         // players only drop backpacks if so indicated
         if(!(dmflags & DM_PLAYERDROP))
            return;
      }

      item = target->info->droptype;
   }
   else
      return;

   mo = P_SpawnMobj(target->x, target->y, ONFLOORZ, item);
   mo->flags |= MF_DROPPED;    // special versions of items
   
   // EDF FIXME: problematic, needed work to begin with
#if 0
   if(mo->type == MT_MISC24) // put all the players stuff into the
   {                         // backpack
      int a;
      mo->extradata.backpack = Z_Malloc(sizeof(backpack_t), PU_LEVEL, NULL);
      for(a=0; a<NUMAMMO; a++)
         mo->extradata.backpack->ammo[a] = target->player->ammo[a];
      mo->extradata.backpack->weapon = target->player->readyweapon;
      // set the backpack moving slightly faster than the player
      
      // start it moving in a (fairly) random direction
      // i cant be bothered to create a new random number
      // class right now
      // haleyjd: not demo safe, fix later (TODO)
      /*
      mo->momx = target->momx * (gametic-basetic) % 5;
      mo->momy = target->momy * (gametic-basetic+30) % 5;
      */
   }
#endif
}

static int MeansOfDeath;

void P_DeathMessage(mobj_t *source, mobj_t *target, mobj_t *inflictor)
{
   boolean friendly = false;
   const char *message = NULL;

   if(!target->player || !obituaries)
      return;

   // voodoo doll death
   if(inflictor && inflictor != target &&
      inflictor->player == target->player)
      MeansOfDeath = MOD_UNKNOWN;

   if(GameType == gt_coop)
      friendly = true;

   // miscellaneous death types that cannot be determined
   // directly from the source or inflictor without difficulty

   switch(MeansOfDeath)
   {
   case MOD_SUICIDE: message = s_OB_SUICIDE; break;
   case MOD_FALLING: message = s_OB_FALLING; break;
   case MOD_CRUSH:   message = s_OB_CRUSH;   break;
   case MOD_SLIME:   message = s_OB_SLIME;   break;
   case MOD_LAVA:    message = s_OB_LAVA;    break;
   case MOD_BARREL:  message = s_OB_BARREL;  break;
   case MOD_SPLASH:  message = s_OB_SPLASH;  break;
   default: break;
   }

   if(source && !message)
   {
      if(source == target)
      {
         // killed self

         switch(MeansOfDeath)
         {
         case MOD_R_SPLASH:      message = s_OB_R_SPLASH_SELF; break;
         case MOD_ROCKET:        message = s_OB_ROCKET_SELF;   break;
         case MOD_BFG11K_SPLASH: message = s_OB_BFG11K_SELF; break;
         case MOD_GRENADE:       message = s_OB_GRENADE_SELF; break;
         default: break;
         }
      }
      else if(!source->player)
      {
         // monster kills

         if(MeansOfDeath == MOD_HIT)
         {
            // melee attack
            if(source->info->meleeobit)
               message = source->info->meleeobit;
         }            
         else
         {
            // other attack
            if(source->info->obituary)
               message = source->info->obituary;
         }
      }
   }

   // print message if any
   if(message)
   {
      doom_printf("%c%s %s",obcolour+128,target->player->name,message);
      return;
   }

   if(source && source->player)
   {
      if(MeansOfDeath == MOD_PLAYERMISC)
      {
         // look at source's readyweapon to determine cause
         switch(source->player->readyweapon)
         {
         case wp_fist:         MeansOfDeath = MOD_FIST; break;
         case wp_pistol:       MeansOfDeath = MOD_PISTOL; break;
         case wp_shotgun:      MeansOfDeath = MOD_SHOTGUN; break;
         case wp_chaingun:     MeansOfDeath = MOD_CHAINGUN; break;
         case wp_chainsaw:     MeansOfDeath = MOD_CHAINSAW; break;
         case wp_supershotgun: MeansOfDeath = MOD_SSHOTGUN; break;
         default: break;
         }
      }

      if(friendly)
      {
         // in coop mode, player kills are bad
         message = s_OB_COOP;
      }
      else
      {
         // deathmatch deaths

         switch(MeansOfDeath)
         {
         case MOD_FIST:       message = s_OB_FIST; break;
         case MOD_CHAINSAW:   message = s_OB_CHAINSAW; break;
         case MOD_PISTOL:     message = s_OB_PISTOL; break;
         case MOD_SHOTGUN:    message = s_OB_SHOTGUN; break;
         case MOD_SSHOTGUN:   message = s_OB_SSHOTGUN; break;
         case MOD_CHAINGUN:   message = s_OB_CHAINGUN; break;
         case MOD_ROCKET:     message = s_OB_ROCKET; break;
         case MOD_R_SPLASH:   message = s_OB_R_SPLASH; break;
         case MOD_PLASMA:     message = s_OB_PLASMA; break;
         case MOD_BFG:        message = s_OB_BFG; break;
         case MOD_BFG_SPLASH: message = s_OB_BFG_SPLASH; break;
         case MOD_BETABFG:    message = s_OB_BETABFG; break;
         case MOD_BFGBURST:   message = s_OB_BFGBURST; break;
         case MOD_GRENADE:    message = s_OB_GRENADE; break;
         default: break;
         }
      }
   }

   // use default message
   if(!message)
      message = s_OB_DEFAULT;

   // print message
   doom_printf("%c%s %s",obcolour+128,target->player->name,message);
}

// Special damage type code

typedef struct dmgspecdata_s
{
   mobj_t *source;
   mobj_t *target;
   int     damage;
} dmgspecdata_t;

//
// P_MinotaurChargeHit
//
// Special damage action for Maulotaurs slamming into things.
//
static boolean P_MinotaurChargeHit(dmgspecdata_t *dmgspec)
{
   mobj_t *source = dmgspec->source;
   mobj_t *target = dmgspec->target;

   // only when charging
   if(source->flags & MF_SKULLFLY)
   {
      angle_t angle;
      fixed_t thrust;
      
      angle = R_PointToAngle2(source->x, source->y, target->x, target->y);
      thrust = 16*FRACUNIT + (P_Random(pr_mincharge) << 10);

      P_ThrustMobj(target, angle, thrust);
      P_DamageMobj(target, NULL, NULL, 
                   ((P_Random(pr_mincharge) & 7) + 1) * 6, 
                   MOD_UNKNOWN);
      
      if(target->player)
         target->reactiontime = 14 + (P_Random(pr_mincharge) & 7);

      return true; // return early from P_DamageMobj
   }

   return false; // just normal damage
}

//
// P_TouchWhirlwind
//
// Called when an Iron Lich whirlwind hits something. Does damage
// and may toss the target around violently.
//
static boolean P_TouchWhirlwind(dmgspecdata_t *dmgspec)
{
   mobj_t *target = dmgspec->target;
   
   // toss the target around
   
   target->angle += P_SubRandom(pr_whirlwind) << 20;
   target->momx  += P_SubRandom(pr_whirlwind) << 10;   
   target->momy  += P_SubRandom(pr_whirlwind) << 10;

   // z momentum -- Bosses will not be tossed up.

   if((leveltime & 16) && !(target->flags2 & MF2_BOSS))
   {
      int randVal = P_Random(pr_whirlwind);

      if(randVal > 160)
         randVal = 160;
      
      target->momz += randVal << 10;
      
      if(target->momz > 12*FRACUNIT)
         target->momz = 12*FRACUNIT;
   }
   
   // do a small amount of damage (it adds up fast)
   if(!(leveltime & 7))
      P_DamageMobj(target, NULL, NULL, 3, MOD_UNKNOWN);

   return true; // always return from P_DamageMobj
}

typedef boolean (*dmgspecial_t)(dmgspecdata_t *);

static dmgspecial_t DamageSpecials[INFLICTOR_NUMTYPES] =
{
   NULL,                // none
   P_MinotaurChargeHit, // MinotaurCharge
   P_TouchWhirlwind,    // Whirlwind
};

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// haleyjd 07/13/03: added method of death flag
//
void P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, 
                  int damage, int mod)
{
   player_t *player;
   boolean justhit;          // killough 11/98
   boolean bossignore;       // haleyjd
   
   // killough 8/31/98: allow bouncers to take damage
   if(!(target->flags & (MF_SHOOTABLE | MF_BOUNCES)))
      return; // shouldn't happen...
   
   if(target->health <= 0)
      return;
   
   // haleyjd: 
   // Invulnerability -- telestomp can still kill to avoid getting stuck
   // Dormancy -- things are invulnerable until they are awakened
   // No Friend Damage -- some things aren't hurt by friends
   if(damage < 10000)
   {
      if(target->flags2 & (MF2_INVULNERABLE | MF2_DORMANT))
         return;

      if(target->flags3 & MF3_NOFRIENDDMG && source && 
         source->flags & MF_FRIEND)
         return;
   }

   // a dormant thing being destroyed gets restored to normal first
   if(target->flags2 & MF2_DORMANT)
   {
      target->flags2 &= ~MF2_DORMANT;
      target->tics = 1;
   }

   MeansOfDeath = mod;

   if(target->flags & MF_SKULLFLY)
   {
      // haleyjd 07/30/04: generalized
      if(target->flags3 & MF3_INVULNCHARGE)
         return;
      target->momx = target->momy = target->momz = 0;
   }

   player = target->player;
   if(player && gameskill == sk_baby)
      damage >>= 1;   // take half damage in trainer mode

   // haleyjd 08/01/04: dmgspecial -- special inflictor types
   if(inflictor && inflictor->info->dmgspecial)
   {
      dmgspecdata_t dmgspec;

      dmgspec.source = inflictor;
      dmgspec.target = target;
      dmgspec.damage = damage;

      // if the handler returns true, damage was taken care of by
      // the handler, otherwise, we go on as normal
      if(DamageSpecials[inflictor->info->dmgspecial](&dmgspec))
         return;

      // damage may be modified by the handler
      damage = dmgspec.damage;
   }

   // Some close combat weapons should not
   // inflict thrust and push the victim out of reach,
   // thus kick away unless using the chainsaw.

   // haleyjd FIXME: make lack of thrust a property of weapons

   if(inflictor && !(target->flags & MF_NOCLIP) &&
      (!source || !source->player ||
      source->player->readyweapon != wp_chainsaw) &&
      !(inflictor->flags3 & MF3_NODMGTHRUST)) // haleyjd 11/14/02
   {
      // haleyjd: thrust factor differs for Heretic
      short tf = gameModeInfo->thrustFactor;
      unsigned ang = R_PointToAngle2 (inflictor->x, inflictor->y,
                                      target->x, target->y);

      fixed_t thrust = damage*(FRACUNIT>>3)*tf/target->info->mass;

      // make fall forwards sometimes
      if(damage < 40 && damage > target->health
         && target->z - inflictor->z > 64*FRACUNIT
         && P_Random(pr_damagemobj) & 1)
      {
         ang += ANG180;
         thrust *= 4;
      }

      P_ThrustMobj(target, ang, thrust);
      
      // killough 11/98: thrust objects hanging off ledges
      if(target->intflags & MIF_FALLING && target->gear >= MAXGEAR)
         target->gear = 0;
   }

   // player specific
   if(player)
   {
      // end of game hell hack
      if(target->subsector->sector->special == 11 && damage >= target->health)
         damage = target->health - 1;

      // Below certain threshold,
      // ignore damage in GOD mode, or with INVUL power.
      // killough 3/26/98: make god mode 100% god mode in non-compat mode

      if((damage < 1000 || (!comp[comp_god] && player->cheats&CF_GODMODE)) &&
         (player->cheats&CF_GODMODE || player->powers[pw_invulnerability]))
         return;

      if(player->armortype)
      {
         int saved;
         
         // haleyjd 10/10/02: heretic armor -- its better! :P
         if(player->hereticarmor)
            saved = player->armortype == 1 ? damage/2 : (damage*3)/4;
         else
            saved = player->armortype == 1 ? damage/3 : damage/2;
         
         if(player->armorpoints <= saved)
         {
            // armor is used up
            saved = player->armorpoints;
            player->armortype = 0;
            // haleyjd 10/10/02
            player->hereticarmor = false;
         }
         player->armorpoints -= saved;
         damage -= saved;
      }

      player->health -= damage;       // mirror mobj health here for Dave
      if(player->health < 0)
         player->health = 0;
      
      player->attacker = source;
      player->damagecount += damage;  // add damage after armor / invuln
      
      if(player->damagecount > 100)
         player->damagecount = 100;  // teleport stomp does 10k points...

#if 0
      // killough 11/98: 
      // This is unused -- perhaps it was designed for
      // a hand-connected input device or VR helmet,
      // to pinch the player when they're hurt :)

      // haleyjd: this was for the "CyberMan" glove controller ^_^
      
      {
         int temp = damage < 100 ? damage : 100;
         
         if(player == &players[consoleplayer])
            I_Tactile(40,10,40+temp*2);
      }
#endif
      
   }

   // do the damage
   if((target->health -= damage) <= 0)
   {
      // death messages for players
      if(player)
         P_DeathMessage(source, target, inflictor);

      P_KillMobj(source, target);
      return;
   }

   // haleyjd: special falling death hack
   //          if we got here, we didn't really die
   if(target->intflags & MIF_DIEDFALLING)
      target->intflags &= ~MIF_DIEDFALLING;

   // killough 9/7/98: keep track of targets so that friends can help friends
   if(demo_version >= 203)
   {
      // If target is a player, set player's target to source,
      // so that a friend can tell who's hurting a player
      if(player)
         P_SetTarget(&target->target, source);
      
      // killough 9/8/98:
      // If target's health is less than 50%, move it to the front of its list.
      // This will slightly increase the chances that enemies will choose to
      // "finish it off", but its main purpose is to alert friends of danger.
      if(target->health * 2 < target->info->spawnhealth)
      {
         thinker_t *cap = 
            &thinkerclasscap[target->flags & MF_FRIEND ? 
                             th_friends : th_enemies];
         (target->thinker.cprev->cnext = target->thinker.cnext)->cprev =
            target->thinker.cprev;
         (target->thinker.cnext = cap->cnext)->cprev = &target->thinker;
         (target->thinker.cprev = cap)->cnext = &target->thinker;
      }
   }

   if((justhit = (P_Random (pr_painchance) < target->info->painchance 
      && !(target->flags & MF_SKULLFLY)))) //killough 11/98: see below
   { // haleyjd 10/06/99: remove pain for zero-damage projectiles
      
      if(!(demo_version >= 329 && damage <= 0))
         P_SetMobjState(target, target->info->painstate);
   }
   target->reactiontime = 0;           // we're awake now...

   // killough 9/9/98: cleaned up, made more consistent:
   // haleyjd 11/24/02: added MF3_DMGIGNORED and MF3_BOSSIGNORE flags

   // haleyjd: set bossignore
   if(source && (source->type != target->type) &&
      ((source->flags3 & target->flags3) & MF3_BOSSIGNORE))
   {
      // ignore if friendliness matches
      bossignore = !((source->flags ^ target->flags) & MF_FRIEND);
   }
   else
      bossignore = false;
   
   if(source && source != target && 
      !(source->flags3 & MF3_DMGIGNORED) && !bossignore &&
      (!target->threshold || (target->flags3 & MF3_NOTHRESHOLD)) &&
      ((source->flags ^ target->flags) & MF_FRIEND || 
       monster_infighting || demo_version < 203))
   {
      // if not intent on another player, chase after this one
      //
      // killough 2/15/98: remember last enemy, to prevent
      // sleeping early; 2/21/98: Place priority on players
      // killough 9/9/98: cleaned up, made more consistent:

      if(!target->lastenemy || target->lastenemy->health <= 0 ||
         (demo_version < 203 ? !target->lastenemy->player :
          !((target->flags ^ target->lastenemy->flags) & MF_FRIEND) &&
             target->target != source)) // remember last enemy - killough
      {
         P_SetTarget(&target->lastenemy, target->target);
      }

      P_SetTarget(&target->target, source);       // killough 11/98
      target->threshold = BASETHRESHOLD;
      if(target->state == &states[target->info->spawnstate]
         && target->info->seestate != NullStateNum)
         P_SetMobjState (target, target->info->seestate);
   }

   // killough 11/98: Don't attack a friend, unless hit by that friend.
   if(justhit && 
      (target->target == source || !target->target ||
       !(target->flags & target->target->flags & MF_FRIEND)))
   {
      target->flags |= MF_JUSTHIT;    // fight back!
   }
}

//
// P_Whistle
//
// haleyjd 01/11/04:
// Inspired in part by Lee Killough's changelog, this allows the
// a friend to be teleported to the player's location.
//
void P_Whistle(mobj_t *actor, int mobjtype)
{
   mobj_t *mo;
   thinker_t *t;
   fixed_t prevx, prevy, prevz, prestep, x, y, z;
   angle_t an;

   // look for a friend of the indicated type
   for(t = thinkercap.next; t != &thinkercap; t = t->next)
   {
      if(t->function != P_MobjThinker)
         continue;

      mo = (mobj_t *)t;

      // must be friendly, alive, and of the right type
      if(!(mo->flags & MF_FRIEND) || mo->health <= 0 ||
         mo->type != mobjtype)
         continue;

      prevx = mo->x;
      prevy = mo->y;
      prevz = mo->z;

      // pick a location a bit in front of the player
      an = actor->angle >> ANGLETOFINESHIFT;
      prestep = 4*FRACUNIT + 3*(actor->info->radius + mo->info->radius)/2;

      x = actor->x + FixedMul(prestep, finecosine[an]);
      y = actor->y + FixedMul(prestep, finesine[an]);
      z = actor->z;

      // don't cross "solid" lines
      if(Check_Sides(actor, x, y))
         return;

      // try the teleport
      // 06/06/05: use strict teleport now
      if(P_TeleportMoveStrict(mo, x, y, false))
      {
         mobj_t *fog = P_SpawnMobj(prevx, prevy, 
                                   prevz + gameModeInfo->teleFogHeight,
                                   gameModeInfo->teleFogType);
         S_StartSound(fog, gameModeInfo->teleSound);

         fog = P_SpawnMobj(x, y, z + gameModeInfo->teleFogHeight,
                           gameModeInfo->teleFogType);
         S_StartSound(fog, gameModeInfo->teleSound);

         // put the thing into its spawnstate and keep it still
         P_SetMobjState(mo, mo->info->spawnstate);
         mo->z = mo->floorz;
         mo->momx = mo->momy = mo->momz = 0;

         return;
      }
   }
}

//
// Small natives
//

//
// sm_thingkill
//
// Implements ThingKill(tid, damagetype = 0, mod = 0)
//
static cell AMX_NATIVE_CALL sm_thingkill(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   mobj_t *rover = NULL;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   while((rover = P_FindMobjFromTID(params[1], rover, context)))
   {
      int damage;
      
      switch(params[2])
      {
      case 1: // telefrag damage
         damage = 10000;
         break;
      default: // damage for health
         damage = rover->health;
         break;
      }
      
      P_DamageMobj(rover, NULL, NULL, damage, MOD_UNKNOWN);
   }

   return 0;
}

//
// sm_thinghurt
//
// Implements ThingHurt(tid, damage, mod = 0, inflictor = 0, source = 0)
//
static cell AMX_NATIVE_CALL sm_thinghurt(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   mobj_t *rover = NULL;
   mobj_t *inflictor = NULL;
   mobj_t *source = NULL;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   if(params[4] != 0)
      inflictor = P_FindMobjFromTID(params[4], inflictor, context);

   if(params[5] != 0)
      source = P_FindMobjFromTID(params[5], source, context);

   while((rover = P_FindMobjFromTID(params[1], rover, context)))
   {
      P_DamageMobj(rover, inflictor, source, params[2], params[3]);
   }

   return 0;
}

//
// sm_thinghate
//
// Implements ThingHate(object, target)
//
static cell AMX_NATIVE_CALL sm_thinghate(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   mobj_t *obj = NULL, *targ = NULL;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   if(params[2] != 0)
      targ = P_FindMobjFromTID(params[2], targ, context);

   while((obj = P_FindMobjFromTID(params[1], obj, context)))
   {
      P_SetTarget(&(obj->target), targ);
   }

   return 0;
}

AMX_NATIVE_INFO pinter_Natives[] =
{
   { "_ThingKill", sm_thingkill },
   { "_ThingHurt", sm_thinghurt },
   { "_ThingHate", sm_thinghate },
   { NULL, NULL }
};

//----------------------------------------------------------------------------
//
// $Log: p_inter.c,v $
// Revision 1.10  1998/05/03  23:09:29  killough
// beautification, fix #includes, move some global vars here
//
// Revision 1.9  1998/04/27  01:54:43  killough
// Prevent pickup sounds from silencing player weapons
//
// Revision 1.8  1998/03/28  17:58:27  killough
// Fix spawn telefrag bug
//
// Revision 1.7  1998/03/28  05:32:41  jim
// Text enabling changes for DEH
//
// Revision 1.6  1998/03/23  03:25:44  killough
// Fix weapon pickup sounds in spy mode
//
// Revision 1.5  1998/03/10  07:15:10  jim
// Initial DEH support added, minus text
//
// Revision 1.4  1998/02/23  04:44:33  killough
// Make monsters smarter
//
// Revision 1.3  1998/02/17  06:00:54  killough
// Save last enemy, change RNG calling sequence
//
// Revision 1.2  1998/01/26  19:24:05  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:59  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
