// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
// Gamemode information
//
// Stores all gamemode-dependent information in one location and 
// lends more structure to other modules.
//
// James Haley
//
//----------------------------------------------------------------------------

#ifndef __D_GI_H__
#define __D_GI_H__

#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_htic.h"
#include "sounds.h"
#include "v_video.h"
#include "st_stuff.h"

// inspired by, but not taken from, zdoom

// Menu cursor -- a 2 patch alternating graphic
typedef struct gimenucursor_s
{
   char *patch1;
   char *patch2;
} gimenucursor_t;

// Screen border used to fill backscreen for small screen sizes
typedef struct giborder_s
{
   int offset;
   int size;
   char *c_tl;
   char *top;
   char *c_tr;
   char *left;
   char *right;
   char *c_bl;
   char *bottom;
   char *c_br;
} giborder_t;

typedef struct gitextmetric_s
{
   int x, y;   // initial coordinates (for finale)
   int cy;     // step amount for \n
   int space;  // blank character step
   int dw;     // amount to subtract from character width
   int absh;   // absolute maximum height of any character
} gitextmetric_t;

// Intermission function pointer set
typedef struct giinterfuncs_s
{
   void (*Ticker)(void);         // called by IN_Ticker
   void (*DrawBackground)(void); // called various places
   void (*Drawer)(void);         // called by IN_Drawer
   void (*Start)(wbstartstruct_t *wbstartstruct); // called by IN_Start
} giinterfuncs_t;

// enum for sound index array used by p_info.c to remap environmental
// sound defaults

enum
{
   INFO_DOROPN,
   INFO_DORCLS,
   INFO_BDOPN,
   INFO_BDCLS,
   INFO_SWTCHN,
   INFO_SWTCHX,
   INFO_PSTART,
   INFO_PSTOP,
   INFO_STNMOV,
   INFO_NUMSOUNDS,
};

//
// enum for menu sounds
//
enum
{
   MN_SND_ACTIVATE,
   MN_SND_DEACTIVATE,
   MN_SND_KEYUPDOWN,
   MN_SND_COMMAND,
   MN_SND_PREVIOUS,
   MN_SND_KEYLEFTRIGHT,
   MN_SND_NUMSOUNDS,
};


// Game Mode Flags

#define GIF_PAGERAW      0x00000001  // page screens are raw format
#define GIF_SHAREWARE    0x00000002  // shareware game (no -file)

//
// Game Mode Types
//
enum
{
   Game_DOOM,
   Game_Heretic,
   NumGameModeTypes
};

//
// gameinfo_t
//
// This structure holds just about all the gamemode-pertinent
// information that could easily be pulled out of the rest of
// the source. This approach, as mentioned above, was inspired
// by zdoom, but I've taken it and really run with it.
//

typedef struct gameinfo_s
{
   int type;                  // main game mode type: doom, heretic, etc.
   int flags;                 // game mode flags

   // startup stuff
   const char *resourceFmt;   // format string for resource wad
   
   // demo state information
   int titleTics;             // length of time to show title
   int advisorTics;           // for Heretic, len. to show advisory
   boolean hasAdvisory;       // shows an advisory at title screen
   int pageTics;              // length of general demo state pages
   int titleMusNum;           // music number to use for title

   // menu stuff
   char *menuBackground;      // name of menu background flat
   char *creditBackground;    // name of dynamic credit bg flat
   int   creditY;             // y coord for credit text
   gimenucursor_t *menuCursor;   // pointer to the big menu cursor
   menu_t *mainMenu;          // pointer to main menu structure
   int *menuSounds;           // menu sound indices
   int transFrame;            // frame DEH # used on video menu
   int skvAtkSound;            // skin viewer attack sound

   // border stuff
   char *borderFlat;          // name of flat to fill backscreen
   giborder_t *border;        // pointer to a game border

   // HU / Video / Console stuff
   char **defTextTrans;       // default text color rng for msgs
   int colorNormal;           // color # for normal messages
   int colorHigh;             // color # for highlighted messages
   int colorError;            // color # for error messages
   int c_numCharsPerLine;     // number of chars per line in console
   int c_BellSound;           // sound used for \a in console
   int c_ChatSound;           // sound used by say command
   gitextmetric_t *vtextinfo; // v_font text info
   unsigned char blackIndex;  // palette index for black {16,16,16}
   unsigned char whiteIndex;  // palette index for white {255,255,255}

   // Status bar
   stbarfns_t *StatusBar;     // status bar function set

   // Miscellaneous graphics
   char *pausePatch;          // name of patch to show when paused

   // Game interaction stuff
   int numEpisodes;           // number of game episodes
   int teleFogType;           // DeHackEd number of telefog object
   fixed_t teleFogHeight;     // amount to add to telefog z coord
   int teleSound;             // sound id for teleportation
   short thrustFactor;        // damage thrust factor
   boolean hasMadMelee;       // game mode has mad melee when player dies

   // Intermission and Finale stuff
   int interMusNum;           // intermission music number
   gitextmetric_t *ftextinfo; // finale text info
   giinterfuncs_t *interfuncs;// intermission function pointers

   // Sound
   musicinfo_t *s_music;      // pointer to musicinfo_t (sounds.h)
   int musMin;                // smallest music index value (0)
   int numMusic;              // maximum music index value
   char *musPrefix;           // "D_" for DOOM, "MUS_" for Heretic
   int *infoSounds;           // p_info env. sound remapping array
   const char *defSoundName;  // default sound if one is missing

   // Line special sound variables -- TODO: replace with editable
   // sound sequences
   char **normalDoorClose;
   char **blazingDoorClose;

} gameinfo_t;

extern gameinfo_t *gameModeInfo;

extern gameinfo_t giDoomSW;
extern gameinfo_t giDoomReg;
extern gameinfo_t giDoomRetail;
extern gameinfo_t giDoomCommercial;
extern gameinfo_t giHereticSW;
extern gameinfo_t giHereticReg;

void D_InitGameInfo(void);

#endif

// EOF
