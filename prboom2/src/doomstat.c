/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *      Put all global state variables here.
 *
 *-----------------------------------------------------------------------------
 */

static const char

#ifdef __GNUG__
#pragma implementation "doomstat.h"
#endif
#include "doomstat.h"
#include "c_runcmd.h"

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode = indetermined;
GameMission_t   gamemission = doom;

// Language.
Language_t   language = english;

// Set if homebrew PWAD stuff has been added.
boolean modifiedgame;

//-----------------------------------------------------------------------------

// CPhipps - compatibility vars
complevel_t default_compatibility_level = -1;
CONSOLE_INT(default_compatibility_level, default_compatibility_level, NULL, -1, MAX_COMPATIBILITY_LEVEL, NULL, 0) {}

complevel_t compatibility_level = -1;
CONSOLE_INT(compatibility_level, compatibility_level, NULL, -1, MAX_COMPATIBILITY_LEVEL, NULL, 0) {}

int comp[COMP_TOTAL];
int default_comp[COMP_TOTAL] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0 ,0,
  0, 0, 0, 0, 0, 0, 0, 0, 0 ,0,
  0, 0
};    // killough 10/98

// v1.1-like pitched sounds
int pitched_sounds = 0;        // killough

int     default_translucency = 1; // config file says           // phares
boolean general_translucency; // true if translucency is ok // phares

int demo_insurance;
int default_demo_insurance = 2;        // killough 1/16/98

int  allow_pushers = 1;      // MT_PUSH Things              // phares 3/10/98
int  default_allow_pushers;  // killough 3/1/98: make local to each game

int  variable_friction = 1;      // ice & mud               // phares 3/10/98
int  default_variable_friction;  // killough 3/1/98: make local to each game

int  weapon_recoil;              // weapon recoil                   // phares
int  default_weapon_recoil = 1;  // killough 3/1/98: make local to each game

int player_bobbing;  // whether player bobs or not          // phares 2/25/98
int default_player_bobbing = 1;  // killough 3/1/98: make local to each game

int monsters_remember;          // killough 3/1/98
int default_monsters_remember = 1;

int monster_infighting=1;       // killough 7/19/98: monster<=>monster attacks
int default_monster_infighting=1;

int monster_friction=1;       // killough 10/98: monsters affected by friction 
int default_monster_friction=1;

#ifdef DOGS
int dogs, default_dogs = 0;         // killough 7/19/98: Marine's best friend :)
int dog_jumping, default_dog_jumping = 1;   // killough 10/98
#endif

// killough 8/8/98: distance friends tend to move towards players
int distfriend = 128, default_distfriend = 128;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
int monster_backing;
int default_monster_backing = 1;

// killough 9/9/98: whether monsters are able to avoid hazards (e.g. crushers)
int monster_avoid_hazards;
int default_monster_avoid_hazards = 1;

// killough 9/9/98: whether monsters help friends
int help_friends;
int default_help_friends = 1;

int flashing_hom = 0;     // killough 10/98

int doom_weapon_toggles = 1; // killough 10/98

int monkeys;
int default_monkeys = 1;

