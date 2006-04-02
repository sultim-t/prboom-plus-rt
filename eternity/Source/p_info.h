// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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

#ifndef __P_INFO_H__
#define __P_INFO_H__

#include "c_io.h"

void P_LoadLevelInfo(int lumpnum);

typedef struct LevelInfo_s
{
   // intermission and finale stuff
   int  partime;             // intermission par time in seconds
   char *interPic;           // intermission background
   char *interText;          // presence of this determines if a finale will occur
   char *interTextLump;      // this can be set in the file
   char *backDrop;           // pic used during text finale
   char *interMusic;         // text finale music
   char *levelPic;           // intermission level name text lump
   boolean killFinale;       // level has no finale even if text is given
   boolean finaleSecretOnly; // level only has finale if secret exit
   boolean endOfGame;        // DOOM II: last map, trigger cast call

   // level transfer stuff
   char *nextLevel;
   char *nextSecret;

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

   // "private" data members
   int next;                 // index of next global LevelInfo object

} LevelInfo_t;

extern LevelInfo_t LevelInfo;

extern boolean default_weaponowned[NUMWEAPONS];

#endif

// EOF

