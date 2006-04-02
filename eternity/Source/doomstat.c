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
//      Put all global state variables here.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: doomstat.c,v 1.5 1998/05/12 12:46:12 phares Exp $";

#include "doomstat.h"

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode = indetermined;
GameMission_t   gamemission = doom;

// Language.
Language_t   language = english;

// Set if homebrew PWAD stuff has been added.
boolean modifiedgame;

//-----------------------------------------------------------------------------

boolean in_textmode = true;        // game in graphics mode yet?

// compatibility with old engines (monster behavior, metrics, etc.)
int compatibility, default_compatibility;          // killough 1/31/98

int comp[COMP_TOTAL], default_comp[COMP_TOTAL];    // killough 10/98

int demo_version;           // killough 7/19/98: Boom version of demo
int demo_subversion;        // haleyjd 06/17/01: subversion for betas

// v1.1-like pitched sounds
int pitched_sounds;  // killough 10/98

int general_translucency;    // killough 10/98

int demo_insurance, default_demo_insurance;        // killough 1/16/98

int  allow_pushers = 1;      // PUSH Things              // phares 3/10/98
int  default_allow_pushers;  // killough 3/1/98: make local to each game

int  variable_friction = 1;      // ice & mud               // phares 3/10/98
int  default_variable_friction;  // killough 3/1/98: make local to each game

int  weapon_recoil;              // weapon recoil                   // phares
int  default_weapon_recoil;      // killough 3/1/98: make local to each game

        // sf: bobbing doesnt affect game sync surely..
        // haleyjd: yes, it does
int player_bobbing;  // whether player bobs or not          // phares 2/25/98
int default_player_bobbing;

int monsters_remember=1;        // killough 3/1/98
int default_monsters_remember=1;

int monster_infighting=1;       // killough 7/19/98: monster<=>monster attacks
int default_monster_infighting=1;

int monster_friction=1;       // killough 10/98: monsters affected by friction 
int default_monster_friction=1;

int autoaim = 1;  //sf
int default_autoaim = 1;

// haleyjd: moved here from c_net.c (????)
int allowmlook = 0;
int default_allowmlook = 0;

// killough 7/19/98: classic Doom BFG
bfg_t bfgtype, default_bfgtype;

        // sf: removed beta_emulation

int dogs, default_dogs;         // killough 7/19/98: Marine's best friend :)
int dog_jumping, default_dog_jumping;   // killough 10/98

// killough 8/8/98: distance friends tend to move towards players
int distfriend = 128, default_distfriend = 128;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
int monster_backing, default_monster_backing;

// killough 9/9/98: whether monsters are able to avoid hazards (e.g. crushers)
int monster_avoid_hazards, default_monster_avoid_hazards;

// killough 9/9/98: whether monsters help friends
int help_friends, default_help_friends;

int flashing_hom;     // killough 10/98

int doom_weapon_toggles; // killough 10/98

int monkeys, default_monkeys;

boolean cinema_pause = false; // haleyjd 08/22/01

int drawparticles;  // haleyjd 09/28/01
int bloodsplat_particle;
int bulletpuff_particle;
int drawrockettrails;
int drawgrenadetrails;
int drawbfgcloud;

int forceFlipPan; // haleyjd 12/08/01

// haleyjd 08/14/02: temporary heretic vars TEMP_HTIC
int tempHereticMode = 0; // temporary variable indic. heretic mode

// haleyjd 9/22/99
int HelperThing = -1; // in P_SpawnMapThing to substitute helper thing

// haleyjd 03/02/03: optionalize starting on first map
int startOnNewMap = 0;

// haleyjd 04/10/03: unified game type variable
gametype_t GameType = gt_single, DefaultGameType = gt_single;

// haleyjd 05/20/04: option to wait at end of game
#ifdef _SDL_VER
int waitAtExit = 0;
#endif

//----------------------------------------------------------------------------
//
// $Log: doomstat.c,v $
// Revision 1.5  1998/05/12  12:46:12  phares
// Removed OVER_UNDER code
//
// Revision 1.4  1998/05/05  16:29:01  phares
// Removed RECOIL and OPT_BOBBING defines
//
// Revision 1.3  1998/05/03  23:12:13  killough
// beautify, move most global switch variables here
//
// Revision 1.2  1998/01/26  19:23:10  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
