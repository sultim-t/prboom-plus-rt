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
//
// Various structures pertaining to the player.
//
//-----------------------------------------------------------------------------

#ifndef __D_PLAYER__
#define __D_PLAYER__

typedef struct player_s player_t;

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "d_items.h"
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"


// skins.
#include "p_skin.h"


//
// Player states.
//
typedef enum
{
  // Playing or camping.
  PST_LIVE,
  // Dead on the ground, view follows killer.
  PST_DEAD,
  // Ready to restart/respawn???
  PST_REBORN            

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
  // No clipping, walk through barriers.
  CF_NOCLIP           = 1,
  // No damage, no health loss.
  CF_GODMODE          = 2,
  // Not really a cheat, just a debug aid.
  CF_NOMOMENTUM       = 4,
  // haleyjd 03/18/03: infinite ammo
  CF_INFAMMO          = 8,
  
} cheat_t;


//
// Extended player object info: player_t
//
struct player_s
{
  mobj_t*             mo;
  playerstate_t       playerstate;
  ticcmd_t            cmd;

  // Determine POV,
  //  including viewpoint bobbing during movement.
  // Focal origin above r.z
  fixed_t             viewz;
  // Base height above floor for viewz.
  fixed_t             viewheight;
  // Bob/squat speed.
  fixed_t             deltaviewheight;
  // bounded/scaled total momentum.
  fixed_t             bob;    

  // killough 10/98: used for realistic bobbing (i.e. not simply overall speed)
  // mo->momx and mo->momy represent true momenta experienced by player.
  // This only represents the thrust that the player applies himself.
  // This avoids anomolies with such things as Boom ice and conveyors.
  fixed_t            momx, momy;      // killough 10/98

  // This is only used between levels,
  // mo->health is used during levels.
  int                 health; 
  int                 armorpoints;
  // Armor type is 0-2.
  int                 armortype;
  // haleyjd 10/10/02
  boolean             hereticarmor; // true if has heretic armor

  // Power ups. invinc and invis are tic counters.
  int                 powers[NUMPOWERS];
  boolean             cards[NUMCARDS];
  boolean             backpack;
  
  // Frags, kills of other players.
  int                 frags[MAXPLAYERS];
  int                 totalfrags;
  weapontype_t        readyweapon;
  
  // Is wp_nochange if not changing.
  weapontype_t        pendingweapon;

  boolean             weaponowned[NUMWEAPONS];
  int                 ammo[NUMAMMO];
  int                 maxammo[NUMAMMO];

  // True if button down last tic.
  int                 attackdown;
  int                 usedown;

  // Bit flags, for cheats and debug.
  // See cheat_t, above.
  int                 cheats;         

  // Refired shots are less accurate.
  int                 refire;         

   // For intermission stats.
  int                 killcount;
  int                 itemcount;
  int                 secretcount;

  // Hint messages.
  // sf: now done with dprintf
  //  char*               message;        
  
  // For screen flashing (red or bright).
  int                 damagecount;
  int                 bonuscount;

  // Who did damage (NULL for floors/ceilings).
  mobj_t*             attacker;
  
  // So gun flashes light up areas.
  int                 extralight;

  // Current PLAYPAL, ???
  //  can be set to REDCOLORMAP for pain, etc.
  int                 fixedcolormap;

  // Player skin and colorshift,
  //  0-3 for which color to draw player.
  int                 colormap;
  skin_t*             skin;

  // Overlay view sprites (gun, etc).
  pspdef_t            psprites[NUMPSPRITES];

  // haleyjd 04/03/05: true pitch angle (replaces updownangle)
  fixed_t             pitch;

  // True if secret level has been done.
  boolean             didsecret;      

  char                name[20];
};


//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
  boolean     in;     // whether the player is in game
    
  // Player stats, kills, collected items etc.
  int         skills;
  int         sitems;
  int         ssecret;
  int         stime; 
  int         frags[4];
  int         score;  // current score on entry, modified on return
  
} wbplayerstruct_t;

typedef struct
{
  int         epsd;   // episode # (0-2)

  // if true, splash the secret level
  boolean     didsecret;

  // haleyjd: if player is going to secret map
  boolean     gotosecret;
    
  // previous and next levels, origin 0
  int         last;
  int         next;   
    
  int         maxkills;
  int         maxitems;
  int         maxsecret;
  int         maxfrags;

  // the par time
  int         partime;
    
  // index of this player in game
  int         pnum;   

  wbplayerstruct_t    plyr[MAXPLAYERS];

} wbstartstruct_t;


#endif

//----------------------------------------------------------------------------
//
// $Log: d_player.h,v $
// Revision 1.3  1998/05/04  21:34:15  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:31  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
