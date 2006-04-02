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
//	DOOM strings, by language.
//
//-----------------------------------------------------------------------------


#ifndef __DSTRINGS__
#define __DSTRINGS__


// All important printed strings.
// Language selection (message strings).
// Use -DFRENCH etc.

#ifdef FRENCH
#include "d_french.h"
#else
#include "d_englsh.h"
#endif

// Misc. other strings.
#define SAVEGAMENAME	"doomsav"


//
// File locations,
//  relative to current position.
// Path names are OS-sensitive.
//
#define DEVMAPS "devmaps"
#define DEVDATA "devdata"


// Not done in french?

// QuitDOOM messages
#define NUM_QUITMESSAGES   22

extern char* endmsg[];


#ifndef PD_BLUEC        // some files don't have boom-specific things

#define PD_BLUEC  PD_BLUEK
#define PD_REDC   PD_REDK
#define PD_YELLOWC  PD_YELLOWK
#define PD_BLUES    "You need a blue skull to open this door"
#define PD_REDS     "You need a red skull to open this door"
#define PD_YELLOWS  "You need a yellow skull to open this door"
#define PD_ANY      "Any key will open this door"
#define PD_ALL3     "You need all three keys to open this door"
#define PD_ALL6     "You need all six keys to open this door"
#define STSTR_COMPON    "Compatibility Mode On"            // phares
#define STSTR_COMPOFF   "Compatibility Mode Off"           // phares

#endif

// obituaries
#define OB_SUICIDE "suicides"
#define OB_FALLING "cratered"
#define OB_CRUSH   "was squished"
#define OB_LAVA    "burned to death"
#define OB_SLIME   "melted"
#define OB_BARREL  "was blown up"
#define OB_SPLASH  "OB_SPLASH"
#define OB_COOP    "scored a frag for the other team"
#define OB_DEFAULT "died"
#define OB_R_SPLASH_SELF "OB_R_SPLASH_SELF"
#define OB_GRENADE_SELF "lost track of a grenade"
#define OB_ROCKET_SELF "should have stood back"
#define OB_BFG11K_SELF "used a BFG11k up close"
#define OB_FIST     "was punched to death"
#define OB_CHAINSAW "was mistaken for a tree"
#define OB_PISTOL   "took a slug to the head"
#define OB_SHOTGUN  "couldn't duck the buckshot"
#define OB_SSHOTGUN "was shown some double barrels"
#define OB_CHAINGUN "did the chaingun cha-cha"
#define OB_ROCKET   "was blown apart by a rocket"
#define OB_R_SPLASH "OB_R_SPLASH"
#define OB_PLASMA   "admires the pretty blue stuff"
#define OB_BFG      "was utterly destroyed by the BFG"
#define OB_BFG_SPLASH "saw the green flash"
#define OB_BETABFG  "got a blast from the past"
#define OB_BFGBURST "was caught in a plasma storm"
#define OB_GRENADE  "tripped on a grenade"

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
