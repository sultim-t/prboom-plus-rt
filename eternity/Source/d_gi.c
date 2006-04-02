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

#include "doomstat.h"
#include "doomdef.h"
#include "sounds.h"
#include "hi_stuff.h"
#include "wi_stuff.h"
#include "d_gi.h"
#include "p_info.h"
#include "info.h"
#include "e_edf.h"

// definitions

// resource wad format strings
#define DOOMRESWAD   "%s/%s.wad"
#define HTICRESWAD   "%s/%.4shtic.wad"

// menu background flats
#define DOOMMENUBACK "FLOOR4_8"
#define HTICMENUBACK "FLOOR04"

// credit background flats
#define DOOMCREDITBK "NUKAGE1"
#define DM2CREDITBK  "SLIME05"
#define HTICCREDITBK "FLTWAWA1"

// border flats
#define DOOMBRDRFLAT "FLOOR7_2"
#define DM2BRDRFLAT  "GRNROCK"
#define HREGBRDRFLAT "FLAT513"
#define HSWBRDRFLAT  "FLOOR04"

// Default sound names
// Note: these are lump names, not sound mnemonics
#define DOOMDEFSOUND "DSPISTOL"
#define HTICDEFSOUND "GLDHIT"

// globals

// holds the address of the gameinfo_t for the current gamemode,
// determined at startup
gameinfo_t *gameModeInfo;

// data

//
// Menu Cursors
//

// doom menu skull cursor
static gimenucursor_t giSkullCursor =
{
   "M_SKULL1",
   "M_SKULL2",
};

// heretic menu arrow cursor
static gimenucursor_t giArrowCursor =
{
   "M_SLCTR1",
   "M_SLCTR2",
};

//
// Display Borders
//

static giborder_t giDoomBorder =
{
   8, // offset
   8, // size

   "BRDR_TL", "BRDR_T", "BRDR_TR",
   "BRDR_L",            "BRDR_R",
   "BRDR_BL", "BRDR_B", "BRDR_BR",
};

static giborder_t giHticBorder =
{
   4,  // offset
   16, // size

   "BORDTL", "BORDT", "BORDTR",
   "BORDL",           "BORDR",
   "BORDBL", "BORDB", "BORDBR",
};

//
// V-Font metrics
//

static gitextmetric_t giDoomVText =
{
   0, 0, // x,y (not used)
   8,    // cy
   4,    // space
   0,    // dw
   8,    // absh
};

static gitextmetric_t giHticVText =
{
   0, 0, // x,y (not used)
   9,    // cy
   5,    // space
   1,    // dw
   10,   // absh
};

//
// Finale font metrics
//

static gitextmetric_t giDoomFText =
{
   10, 10, // x,y
   11,     // cy
   4,      // space
   0,      // dw
};

static gitextmetric_t giHticFText =
{
   20, 5, // x,y
   9,     // cy
   5,     // space
   0,     // dw
};

//
// Intermission function pointer sets
//

static giinterfuncs_t giDoomIntrm =
{
   WI_Ticker,
   WI_DrawBackground,
   WI_Drawer,
   WI_Start,
};

static giinterfuncs_t giHticIntrm =
{
   HI_Ticker,
   HI_DrawBackground,
   HI_Drawer,
   HI_Start,
};

//
// MapInfo environmental sound defaults
//

static int doomInfoSounds[INFO_NUMSOUNDS] =
{
   sfx_doropn,
   sfx_dorcls,
   sfx_bdopn,
   sfx_bdcls,
   sfx_swtchn,
   sfx_swtchx,
   sfx_pstart,
   sfx_pstop,
   sfx_stnmov,
};

static int hticInfoSounds[INFO_NUMSOUNDS] =
{
   sfx_hdoropn,
   sfx_hdorcls,
   sfx_hdoropn,
   sfx_hdorcls,
   sfx_hswitch,
   sfx_hswitch,
   sfx_hpstart,
   sfx_hpstop,
   sfx_hstnmov,
};

//
// Menu Sounds
//

static int doomMenuSounds[MN_SND_NUMSOUNDS] =
{
   sfx_swtchn, // activate
   sfx_swtchx, // deactivate
   sfx_pstop,  // key up/down
   sfx_pistol, // command
   sfx_swtchx, // previous menu
   sfx_stnmov, // key left/right
};

static int hticMenuSounds[MN_SND_NUMSOUNDS] =
{
   sfx_hdorcls, // activate
   sfx_hdorcls, // deactivate
   sfx_hswitch, // key up/down
   sfx_hdorcls, // command
   sfx_hdorcls, // previous menu
   sfx_keyup,   // key left/right
};

//
// Game Mode Information Structures
//

//
// DOOM Shareware
//
gameinfo_t giDoomSW =
{
   Game_DOOM,        // type
   GIF_SHAREWARE,    // flags

   DOOMRESWAD,       // resourceFmt

   170,              // titleTics
   0,                // advisorTics
   false,            // hasAdvisory
   11*TICRATE,       // pageTics
   mus_intro,        // titleMusNum

   DOOMMENUBACK,     // menuBackground
   DOOMCREDITBK,     // creditBackground
   17,               // creditY
   &giSkullCursor,   // menuCursor
   &menu_main,       // mainMenu
   doomMenuSounds,   // menuSounds
   S_TBALL1,         // transFrame
   sfx_shotgn,       // skvAtkSound

   DOOMBRDRFLAT,     // borderFlat
   &giDoomBorder,    // border
   
   &cr_red,          // defTextTrans
   CR_RED,           // colorNormal
   CR_GRAY,          // colorHigh
   CR_GOLD,          // colorError
   40,               // c_numCharsPerLine
   sfx_tink,         // c_BellSound
   sfx_tink,         // c_ChatSound
   &giDoomVText,     // vtextinfo
   0,                // blackIndex
   4,                // whiteIndex

   &DoomStatusBar,   // StatusBar

   "M_PAUSE",        // pausePatch
   
   1,                // numEpisodes
   MT_TFOG,          // teleFogType
   0,                // teleFogHeight
   sfx_telept,       // teleSound
   100,              // thrustFactor
   false,            // hasMadMelee

   mus_inter,        // interMusNum
   &giDoomFText,     // ftextinfo
   &giDoomIntrm,     // interfuncs

   S_music,          // s_music
   mus_None,         // musMin
   NUMMUSIC,         // numMusic
   "d_",             // musPrefix
   doomInfoSounds,   // infoSounds
   DOOMDEFSOUND,     // defSoundName

   &LevelInfo.sound_dorcls, // normalDoorClose
   &LevelInfo.sound_bdcls,  // blazingDoorClose
};

//
// DOOM Registered (3 episodes)
//
gameinfo_t giDoomReg =
{
   Game_DOOM,        // type
   0,                // flags

   DOOMRESWAD,       // resourceFmt
   
   170,              // titleTics
   0,                // advisorTics
   false,            // hasAdvisory
   11*TICRATE,       // pageTics
   mus_intro,        // titleMusNum

   DOOMMENUBACK,     // menuBackground
   DOOMCREDITBK,     // creditBackground
   17,               // creditY
   &giSkullCursor,   // menuCursor
   &menu_main,       // mainMenu
   doomMenuSounds,   // menuSounds
   S_TBALL1,         // transFrame
   sfx_shotgn,       // skvAtkSound
   
   DOOMBRDRFLAT,     // borderFlat
   &giDoomBorder,    // border

   &cr_red,          // defTextTrans
   CR_RED,           // colorNormal
   CR_GRAY,          // colorHigh
   CR_GOLD,          // colorError
   40,               // c_numCharsPerLine
   sfx_tink,         // c_BellSound
   sfx_tink,         // c_ChatSound
   &giDoomVText,     // vtextinfo
   0,                // blackIndex
   4,                // whiteIndex

   &DoomStatusBar,   // StatusBar

   "M_PAUSE",        // pausePatch

   3,                // numEpisodes
   MT_TFOG,          // teleFogType
   0,                // teleFogHeight
   sfx_telept,       // teleSound
   100,              // thrustFactor
   false,            // hasMadMelee

   mus_inter,        // interMusNum
   &giDoomFText,     // ftextinfo
   &giDoomIntrm,     // interfuncs

   S_music,          // s_music
   mus_None,         // musMin
   NUMMUSIC,         // numMusic
   "d_",             // musPrefix
   doomInfoSounds,   // infoSounds
   DOOMDEFSOUND,     // defSoundName

   &LevelInfo.sound_dorcls, // normalDoorClose
   &LevelInfo.sound_bdcls,  // blazingDoorClose
};

//
// The Ultimate DOOM (4 episodes)
//
gameinfo_t giDoomRetail =
{
   Game_DOOM,        // type
   0,                // flags

   DOOMRESWAD,       // resourceFmt

   170,              // titleTics
   0,                // advisorTics
   false,            // hasAdvisory
   11*TICRATE,       // pageTics
   mus_intro,        // titleMusNum

   DOOMMENUBACK,     // menuBackground
   DOOMCREDITBK,     // creditBackground
   17,               // creditY
   &giSkullCursor,   // menuCursor
   &menu_main,       // mainMenu
   doomMenuSounds,   // menuSounds
   S_TBALL1,         // transFrame
   sfx_shotgn,       // skvAtkSound

   DOOMBRDRFLAT,     // borderFlat
   &giDoomBorder,    // border

   &cr_red,          // defTextTrans
   CR_RED,           // colorNormal
   CR_GRAY,          // colorHigh
   CR_GOLD,          // colorError
   40,               // c_numCharsPerLine
   sfx_tink,         // c_BellSound
   sfx_tink,         // c_ChatSound
   &giDoomVText,     // vtextinfo
   0,                // blackIndex
   4,                // whiteIndex

   &DoomStatusBar,   // StatusBar

   "M_PAUSE",        // pausePatch

   4,                // numEpisodes
   MT_TFOG,          // teleFogType
   0,                // teleFogHeight
   sfx_telept,       // teleSound
   100,              // thrustFactor
   false,            // hasMadMelee

   mus_inter,        // interMusNum
   &giDoomFText,     // ftextinfo
   &giDoomIntrm,     // interfuncs

   S_music,          // s_music
   mus_None,         // musMin
   NUMMUSIC,         // numMusic
   "d_",             // musPrefix
   doomInfoSounds,   // infoSounds
   DOOMDEFSOUND,     // defSoundName

   &LevelInfo.sound_dorcls, // normalDoorClose
   &LevelInfo.sound_bdcls,  // blazingDoorClose
};

//
// DOOM II / Final DOOM
//
gameinfo_t giDoomCommercial =
{
   Game_DOOM,        // type
   0,                // flags

   DOOMRESWAD,       // resourceFmt

   11*TICRATE,       // titleTics
   0,                // advisorTics
   false,            // hasAdvisory
   11*TICRATE,       // pageTics
   mus_dm2ttl,       // titleMusNum

   DOOMMENUBACK,     // menuBackground
   DM2CREDITBK,      // creditBackground
   17,               // creditY
   &giSkullCursor,   // menuCursor
   &menu_main,       // mainMenu
   doomMenuSounds,   // menuSounds
   S_TBALL1,         // transFrame
   sfx_shotgn,       // skvAtkSound

   DM2BRDRFLAT,      // borderFlat
   &giDoomBorder,    // border

   &cr_red,          // defTextTrans
   CR_RED,           // colorNormal
   CR_GRAY,          // colorHigh
   CR_GOLD,          // colorError
   40,               // c_numCharsPerLine
   sfx_tink,         // c_BellSound
   sfx_radio,        // c_ChatSound
   &giDoomVText,     // vtextinfo
   0,                // blackIndex
   4,                // whiteIndex

   &DoomStatusBar,   // StatusBar

   "M_PAUSE",        // pausePatch

   1,                // numEpisodes
   MT_TFOG,          // teleFogType
   0,                // teleFogHeight
   sfx_telept,       // teleSound
   100,              // thrustFactor
   false,            // hasMadMelee

   mus_dm2int,       // interMusNum
   &giDoomFText,     // ftextinfo
   &giDoomIntrm,     // interfuncs

   S_music,          // s_music
   mus_None,         // musMin
   NUMMUSIC,         // numMusic
   "d_",             // musPrefix
   doomInfoSounds,   // infoSounds
   DOOMDEFSOUND,     // defSoundName

   &LevelInfo.sound_dorcls, // normalDoorClose
   &LevelInfo.sound_bdcls,  // blazingDoorClose
};

//
// Heretic Shareware
//
gameinfo_t giHereticSW =
{
   Game_Heretic,     // type
   GIF_PAGERAW | GIF_SHAREWARE, // flags

   HTICRESWAD,       // resourceFmt

   210,              // titleTics
   140,              // advisorTics
   true,             // hasAdvisory
   200,              // pageTics
   hmus_titl,        // titleMusNum

   HTICMENUBACK,     // menuBackground
   HTICCREDITBK,     // creditBackground
   10,               // creditY
   &giArrowCursor,   // menuCursor
   &menu_hmain,      // mainMenu
   hticMenuSounds,   // menuSounds
   S_MUMMYFX1_1,     // transFrame
   sfx_gldhit,       // skvAtkSound

   HSWBRDRFLAT,      // borderFlat
   &giHticBorder,    // border

   &cr_gray,         // defTextTrans
   CR_GRAY,          // colorNormal
   CR_GOLD,          // colorHigh
   CR_RED,           // colorError
   43,               // c_numCharsPerLine
   sfx_chat,         // c_BellSound
   sfx_chat,         // c_ChatSound
   &giHticVText,     // vtextinfo
   0,                // blackIndex
   35,               // whiteIndex

   &HticStatusBar,   // StatusBar

   "PAUSED",         // pausePatch

   1,                // numEpisodes
   MT_HTFOG,         // teleFogType
   32*FRACUNIT,      // teleFogHeight
   sfx_htelept,      // teleSound
   150,              // thrustFactor
   true,             // hasMadMelee

   hmus_intr,        // interMusNum
   &giHticFText,     // ftextinfo
   &giHticIntrm,     // interfuncs

   H_music,          // s_music
   hmus_None,        // musMin
   NUMHTICMUSIC,     // numMusic
   "mus_",           // musPrefix
   hticInfoSounds,   // infoSounds
   HTICDEFSOUND,     // defSoundName

   &LevelInfo.sound_doropn,  // normalDoorClose
   &LevelInfo.sound_bdopn,   // blazingDoorClose
};

//
// Heretic Registered / Heretic: Shadow of the Serpent Riders
//
gameinfo_t giHereticReg =
{
   Game_Heretic,     // type   
   GIF_PAGERAW,      // flags

   HTICRESWAD,       // resourceFmt

   210,              // titleTics
   140,              // advisorTics
   true,             // hasAdvisory
   200,              // pageTics
   hmus_titl,        // titleMusNum

   HTICMENUBACK,     // menuBackground
   HTICCREDITBK,     // creditBackground
   10,               // creditY
   &giArrowCursor,   // menuCursor
   &menu_hmain,      // mainMenu
   hticMenuSounds,   // menuSounds
   S_MUMMYFX1_1,     // transFrame
   sfx_gldhit,       // skvAtkSound

   HREGBRDRFLAT,     // borderFlat
   &giHticBorder,    // border

   &cr_gray,         // defTextTrans
   CR_GRAY,          // colorNormal
   CR_GOLD,          // colorHigh
   CR_RED,           // colorError
   43,               // c_numCharsPerLine
   sfx_chat,         // c_BellSound
   sfx_chat,         // c_ChatSound
   &giHticVText,     // vtextinfo
   0,                // blackIndex
   35,               // whiteIndex

   &HticStatusBar,   // StatusBar

   "PAUSED",         // pausePatch

   4,                // numEpisodes -- note 6 for SoSR gamemission
   MT_HTFOG,         // teleFogType
   32*FRACUNIT,      // teleFogHeight
   sfx_htelept,      // teleSound
   150,              // thrustFactor
   true,             // hasMadMelee

   hmus_intr,        // interMusNum
   &giHticFText,     // ftextinfo
   &giHticIntrm,     // interfuncs

   H_music,          // s_music
   hmus_None,        // musMin
   NUMHTICMUSIC,     // numMusic
   "mus_",           // musPrefix
   hticInfoSounds,   // infoSounds
   HTICDEFSOUND,     // defSoundName

   &LevelInfo.sound_doropn,  // normalDoorClose
   &LevelInfo.sound_bdopn,   // blazingDoorClose
};

//
// D_InitGameInfo
//
// Called after gameModeInfo is set to normalize some fields.
// EDF makes this necessary.
//
void D_InitGameInfo(void)
{
   int teletype;

#ifdef RANGECHECK
   if(!gameModeInfo)
      I_Error("D_InitGameInfo: called before gameModeInfo set\n");
#endif

   teletype = gameModeInfo->teleFogType;
   gameModeInfo->teleFogType = E_SafeThingType(teletype);
}

// EOF
