// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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

#ifndef P_INFO_H__
#define P_INFO_H__

#include "doomtype.h"

void P_LoadLevelInfo(int lumpnum);

// boss special flags
enum
{
   BSPEC_MAP07_1 = 0x00000001,
   BSPEC_MAP07_2 = 0x00000002,
   BSPEC_E1M8    = 0x00000004,
   BSPEC_E2M8    = 0x00000008,
   BSPEC_E3M8    = 0x00000010,
   BSPEC_E4M6    = 0x00000020,
   BSPEC_E4M8    = 0x00000040,
   BSPEC_E5M8    = 0x00000080,
};

typedef struct LevelInfo_s
{
   // intermission and finale stuff
   int  partime;             // intermission par time in seconds
   char *interPic;           // intermission background
   char *interText;          // presence of this determines if a finale will occur
   char *interTextLump;      // this can be set in the file
   char *backDrop;           // pic used during text finale
   char *interMusic;         // text finale music
   char *levelPic;           // intermission level name graphics lump
   boolean killStats;        // level has no statistics intermission
   boolean killFinale;       // level has no finale even if text is given
   boolean finaleSecretOnly; // level only has finale if secret exit
   boolean endOfGame;        // DOOM II: last map, trigger cast call
   boolean useEDFInterName;  // use an intermission map name from EDF

   // level transfer stuff
   char *nextLevel;          // name of next map for normal exit
   char *nextSecret;         // name of next map for secret exit
   unsigned long bossSpecs;  // boss specials

   // level variables
   char *levelName;          // name used in automap
   char *musicName;          // name of music to play during level

   // color map stuff
   char *colorMap;           // global colormap replacement
   boolean useFullBright;    // use fullbright on this map?
   boolean unevenLight;      // use uneven wall lighting?

   // sky stuff
   char *skyName;            // normal sky name (F_SKY1 or top of double)
   char *altSkyName;         // alt sky - replaces skyName during lightning
   char *sky2Name;           // secondary sky (F_SKY2 or bottom of double)
   boolean doubleSky;        // use hexen-style double skies?
   boolean hasLightning;     // map has lightning flashes?
   int  skyDelta;            // double-sky scroll speeds (units/tic)
   int  sky2Delta;

   // misc
   int gravity;              // gravity factor
   char *creator;            // creator: name of who made this map

   // attached scripts
   boolean hasScripts;       // true if scriptLump is valid
   char *scriptLump;         // name of Levelscript lump
   char *extraData;          // name of ExtraData lump

   // per-level sound replacements
   char *sound_swtchn;       // switch on
   char *sound_swtchx;       // switch off
   char *sound_stnmov;       // plane move
   char *sound_pstop;        // plat stop
   char *sound_bdcls;        // blazing door close
   char *sound_bdopn;        // blazing door open
   char *sound_doropn;       // normal door open
   char *sound_dorcls;       // normal door close
   char *sound_pstart;       // plat start

} LevelInfo_t;

extern LevelInfo_t LevelInfo;

extern boolean default_weaponowned[NUMWEAPONS];

#endif

// EOF

