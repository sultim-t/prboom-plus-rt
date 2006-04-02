// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley, Simon Howard, et al.
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
// Level info.
//
// Under SMMU, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//
// By Simon Howard
//
// Eternity Modifications:
// -----------------------
// As of 07/22/04, Eternity can now support global MapInfo lumps in
// addition to MapInfo placed in level headers. Header MapInfo always
// overrides any global lump data. The global lumps cascade in such
// a way that a map will get its global MapInfo from the newest 
// "EMAPINFO" lump that contains a block with the map's name.
//
// I have also moved much of the initialization code from many modules
// into this file, making the LevelInfo structure the default place to
// get map information, rather than an alternative to many other
// sources as it was previously. This simplifies code outside this
// module and encapsulates more level-dependent decisions here.
//
// -- haleyjd
//
//-----------------------------------------------------------------------------

/* includes ************************/

#include "z_zone.h"
#include "d_io.h"
#include "doomstat.h"
#include "doomdef.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "w_wad.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "sounds.h"
#include "d_gi.h"
#include "d_deh.h"
#include "e_sound.h"
#include "g_game.h"

extern char gamemapname[9];

// haleyjd: moved everything into the LevelInfo struct

LevelInfo_t LevelInfo;

static void P_ParseLevelInfo(int lumpnum);

static void P_LowerCase(char *line);
static void P_StripSpaces(char *line);
static void P_RemoveEqualses(char *line);
static void P_CleanLine(char *line);

static int  P_ParseInfoCmd(char *line);
static void P_ParseLevelVar(char *cmd);

static void P_ClearLevelVars(void);
static void P_InitWeapons(void);

// post-processing routine prototypes
static void P_LoadInterTextLump(void);
static void P_SetSky2Texture(void);
static void P_SetParTime(void);

static enum lireadtype_e
{
   RT_LEVELINFO,
   RT_OTHER,
} readtype;

static enum limode_e
{
   LI_MODE_GLOBAL, // global: we're reading a global MapInfo lump
   LI_MODE_LEVEL,  // level:  we're reading a level header
   LI_NUMMODES
} limode;

static boolean foundGlobalMap;

//
// P_LoadLevelInfo
//
// Parses global MapInfo lumps looking for the first entry that
// matches this map, then parses the map's own header MapInfo if
// it has any. This is the main external function of this module.
// Called from P_SetupLevel.
//
void P_LoadLevelInfo(int lumpnum)
{
   lumpinfo_t *lump;
   int glumpnum;

   // set all the level defaults
   P_ClearLevelVars();

   // parse global lumps
   limode = LI_MODE_GLOBAL;
   foundGlobalMap = false;

   // run down the hash chain for EMAPINFO
   lump = lumpinfo[W_LumpNameHash("EMAPINFO") % (unsigned)numlumps];   
   
   for(glumpnum = lump->index; glumpnum >= 0; glumpnum = lump->next)
   {
      lump = lumpinfo[glumpnum];

      if(!strncasecmp(lump->name, "EMAPINFO", 8) &&
         lump->li_namespace == ns_global)
      {
         // reset the parser state         
         readtype = RT_OTHER;
         P_ParseLevelInfo(glumpnum);
         if(foundGlobalMap) // parsed an entry for this map, so stop
            break;
      }
   }
   
   // parse level lump
   limode   = LI_MODE_LEVEL;
   readtype = RT_OTHER;
   P_ParseLevelInfo(lumpnum);
   
   // haleyjd: call post-processing routines
   P_LoadInterTextLump();
   P_SetSky2Texture();
   P_SetParTime();

   // haleyjd 03/15/03: handle level scripts
   if(LevelInfo.scriptLump)
      LevelInfo.hasScripts = true;

   P_InitWeapons();
}

//
// P_ParseLevelInfo
//
// Parses one individual MapInfo lump.
//
static void P_ParseLevelInfo(int lumpnum)
{
   char *lump;
   char *rover;
   char *startofline;
   int  size;

   lump = W_CacheLumpNum(lumpnum, PU_STATIC);
   
   rover = startofline = lump;
   size  = lumpinfo[lumpnum]->size;
   
   while(rover < lump + size)
   {
      if(*rover == '\n') // end of line
      {
         *rover = 0;               // make it an end of string (0)
         // hack for global mapinfo: if P_ParseInfoCmd returns -1,
         // we can break out of parsing early
         if(P_ParseInfoCmd(startofline) == -1)
            break;
         startofline = rover + 1;  // next line
         *rover = '\n';            // back to end of line
      }
      rover++;
   }

   // contents of the cached lump have been altered, so free it
   Z_Free(lump);
}

//
// P_ParseInfoCmd
//
// Parses a single line of a MapInfo lump.
//
static int P_ParseInfoCmd(char *line)
{
   P_CleanLine(line);

   P_StripSpaces(line);
   P_LowerCase(line);
   
   while(*line == ' ') 
      line++;
   
   if(!*line) 
      return 0;
   
   if((line[0] == '/' && line[1] == '/') ||     // comment
      line[0] == '#' || line[0] == ';') return 0;
  
   if(*line == '[')                // a new section seperator
   {
      line++;

      if(limode == LI_MODE_GLOBAL)
      {
         // when in global mode, returning -1 will make
         // P_ParseLevelInfo break out early, saving time
         if(foundGlobalMap)
            return -1;

         if(!strncasecmp(line, gamemapname, strlen(gamemapname)))
         {
            foundGlobalMap = true;
            readtype = RT_LEVELINFO;
         }
      }
      else
      {
         if(!strncmp(line, "level info", 10))
            readtype = RT_LEVELINFO;
      }
   }
  
   switch(readtype)
   {
   case RT_LEVELINFO:
      P_ParseLevelVar(line);
      break;

   case RT_OTHER:
      break;
   }

   return 0;
}

//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

enum
{
   IVT_STRING,
   IVT_INT,
   IVT_BOOLEAN,
   IVT_END
};

typedef struct
{
   int type;
   char *name;
   void *variable;
} levelvar_t;

levelvar_t levelvars[]=
{
   {IVT_STRING, "levelpic",      &LevelInfo.levelPic},
   {IVT_STRING, "levelname",     &LevelInfo.levelName},
   {IVT_INT,    "partime",       &LevelInfo.partime},
   {IVT_STRING, "music",         &LevelInfo.musicName},
   {IVT_STRING, "skyname",       &LevelInfo.skyName},
   {IVT_STRING, "creator",       &LevelInfo.creator},
   {IVT_STRING, "interpic",      &LevelInfo.interPic},
   {IVT_STRING, "nextlevel",     &LevelInfo.nextLevel},
   {IVT_INT,    "gravity",       &LevelInfo.gravity},
   {IVT_STRING, "inter-backdrop",&LevelInfo.backDrop},
   //{IVT_STRING, "defaultweapons",&info_weapons},
   {IVT_STRING, "altskyname",    &LevelInfo.altSkyName},
   {IVT_STRING, "colormap",      &LevelInfo.colorMap},
   {IVT_BOOLEAN,"fullbright",    &LevelInfo.useFullBright},
   {IVT_BOOLEAN,"unevenlight",   &LevelInfo.unevenLight},
   {IVT_BOOLEAN,"lightning",     &LevelInfo.hasLightning},
   {IVT_STRING, "sky2name",      &LevelInfo.sky2Name},
   {IVT_BOOLEAN,"doublesky",     &LevelInfo.doubleSky},
   {IVT_INT,    "skydelta",      &LevelInfo.skyDelta},
   {IVT_INT,    "sky2delta",     &LevelInfo.sky2Delta},
   {IVT_STRING, "sound-swtchn",  &LevelInfo.sound_swtchn},
   {IVT_STRING, "sound-swtchx",  &LevelInfo.sound_swtchx},
   {IVT_STRING, "sound-stnmov",  &LevelInfo.sound_stnmov},
   {IVT_STRING, "sound-pstop",   &LevelInfo.sound_pstop},
   {IVT_STRING, "sound-bdcls",   &LevelInfo.sound_bdcls},
   {IVT_STRING, "sound-bdopn",   &LevelInfo.sound_bdopn},
   {IVT_STRING, "sound-dorcls",  &LevelInfo.sound_dorcls},
   {IVT_STRING, "sound-doropn",  &LevelInfo.sound_doropn},
   {IVT_STRING, "sound-pstart",  &LevelInfo.sound_pstart},
   {IVT_STRING, "nextsecret",    &LevelInfo.nextSecret},
   {IVT_BOOLEAN,"killfinale",    &LevelInfo.killFinale},
   {IVT_BOOLEAN,"finale-secret", &LevelInfo.finaleSecretOnly},
   {IVT_BOOLEAN,"endofgame",     &LevelInfo.endOfGame},
   {IVT_STRING, "intertext",     &LevelInfo.interTextLump}, // haleyjd 12/13/01
   {IVT_STRING, "intermusic",    &LevelInfo.interMusic},
   {IVT_STRING, "levelscript",   &LevelInfo.scriptLump}, // haleyjd
   {IVT_STRING, "extradata",     &LevelInfo.extraData}, // haleyjd 04/02/03
   {IVT_END,    0,               0}
};

//
// P_ParseLevelVar
//
// Tokenizes the line parsed by P_ParseInfoCmd and then sets
// any appropriate matching MapInfo variable to the retrieved
// value.
//
static void P_ParseLevelVar(char *cmd)
{
   char varname[50];
   char *equals;
   levelvar_t *current = levelvars;

   if(!*cmd) return;
   
   P_RemoveEqualses(cmd);
   
   // right, first find the variable name
   
   sscanf(cmd, "%s", varname);
   
   // find what it equals
   equals = cmd+strlen(varname);
   while(*equals == ' ') equals++; // cut off the leading spaces

   // TODO: improve linear search? fixed small set, so may not matter
   while(current->type != IVT_END)
   {
      if(!strcmp(current->name, varname))
      {
         switch(current->type)
         {
         case IVT_STRING:
            *(char**)current->variable = Z_Strdup(equals, PU_LEVEL, NULL);
            break;
            
         case IVT_INT:
            *(int*)current->variable = atoi(equals);
            break;

            // haleyjd 03/15/03: boolean support
         case IVT_BOOLEAN:
            *(boolean *)current->variable = 
               !strcasecmp(equals, "true") ? true : false;
            break;
         }
      }
      current++;
   }
}

//
// Default Setup and Post-Processing Routines
//

// automap name macros (moved from hu_stuff.c)

#define HU_TITLE  (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (*mapnames2[gamemap-1])
#define HU_TITLEP (*mapnamesp[gamemap-1])
#define HU_TITLET (*mapnamest[gamemap-1])
#define HU_TITLEH (*mapnamesh[(gameepisode-1)*9+gamemap-1])

//
// SynthLevelName
//
// Makes up a level name for new maps and Heretic secrets.
// Moved here from hu_stuff.c
//
// secret == true  -> Heretic hidden map
// secret == false -> Just a plain new level
//
static void SynthLevelName(boolean secret)
{
   // haleyjd 12/14/01: halved size of this string, max length
   // is deterministic since gamemapname is 8 chars long
   static char newlevelstr[25];

   sprintf(newlevelstr, 
           secret ? "%s: hidden level" : "%s: new level", 
           gamemapname);
   
   LevelInfo.levelName = newlevelstr;
}

//
// P_InfoDefaultLevelName
//
// Figures out the name to use for this map.
// Moved here from hu_stuff.c
//
static void P_InfoDefaultLevelName(void)
{
   // TODO: make sensitive to which names are actually replaced
   // via DeHackEd -- a bit challenging given how DEH/BEX works.

   if(!newlevel || deh_loaded)
   {
      if(isMAPxy(gamemapname) && gamemap > 0 && gamemap <= 32)
      {
         // DOOM II
         LevelInfo.levelName = gamemission == pack_tnt  ? HU_TITLET :
                               gamemission == pack_plut ? HU_TITLEP :
                               HU_TITLE2;
      }
      else if(isExMy(gamemapname) &&
              gameepisode > 0 && gameepisode <= gameModeInfo->numEpisodes &&
              gamemap > 0 && gamemap <= 9)
      {
         if(gameModeInfo->type == Game_Heretic) // Heretic
         {
            int maxEpisode = gameModeInfo->numEpisodes;

            // For Heretic, the last episode isn't "real" and contains
            // "secret" levels -- a name is synthesized for those
            if(gameepisode < maxEpisode)
               LevelInfo.levelName = HU_TITLEH;
            else
               SynthLevelName(true); // put "hidden level"
         }
         else
            LevelInfo.levelName = HU_TITLE; // DOOM
      }
      else
         SynthLevelName(false); // oddly named maps
   }
   else
      SynthLevelName(false); // put "new level"
}

static void GetDefSound(char **var, int dehnum)
{
   sfxinfo_t *sfx;

   sfx = E_SoundForDEHNum(dehnum);

   *var = sfx ? sfx->mnemonic : "none";
}

static void P_InfoDefaultSoundNames(void)
{
   // haleyjd 03/17/03: now uses gameModeInfo for defaults
   int *infoSounds = gameModeInfo->infoSounds;

   GetDefSound(&LevelInfo.sound_doropn, infoSounds[INFO_DOROPN]);
   GetDefSound(&LevelInfo.sound_dorcls, infoSounds[INFO_DORCLS]);
   GetDefSound(&LevelInfo.sound_bdopn,  infoSounds[INFO_BDOPN ]);
   GetDefSound(&LevelInfo.sound_bdcls,  infoSounds[INFO_BDCLS ]);
   GetDefSound(&LevelInfo.sound_swtchn, infoSounds[INFO_SWTCHN]);
   GetDefSound(&LevelInfo.sound_swtchx, infoSounds[INFO_SWTCHX]);
   GetDefSound(&LevelInfo.sound_pstart, infoSounds[INFO_PSTART]);
   GetDefSound(&LevelInfo.sound_pstop,  infoSounds[INFO_PSTOP ]);
   GetDefSound(&LevelInfo.sound_stnmov, infoSounds[INFO_STNMOV]);
}

//
// P_LoadInterTextLump
//
// Post-processing routine.
//
// If the intertext lump name was set via MapInfo, this loads the
// lump and sets LevelInfo.interText to it.
// Moved here from f_finale.c
//
static void P_LoadInterTextLump(void)
{
   if(LevelInfo.interTextLump)
   {
      int lumpNum, lumpLen;
            
      lumpNum = W_GetNumForName(LevelInfo.interTextLump);
      lumpLen = W_LumpLength(lumpNum);
      
      LevelInfo.interText = Z_Malloc(lumpLen + 1, PU_LEVEL, 0);
      
      W_ReadLump(lumpNum, LevelInfo.interText);
      
      // null-terminate the string
      (LevelInfo.interText)[lumpLen] = '\0';
   }
}

//
// P_InfoDefaultFinale
//
// Sets up default MapInfo values related to f_finale.c code.
// Moved here from f_finale.c and altered for new features, etc.
//
static void P_InfoDefaultFinale(void)
{
   // set lump to NULL
   LevelInfo.interTextLump = NULL;
   LevelInfo.finaleSecretOnly = false;
   LevelInfo.killFinale = false;
   LevelInfo.endOfGame = false;

   switch(gamemode)
   {
   case shareware:
   case retail:
   case registered:
      // DOOM modes
      LevelInfo.interMusic = S_music[mus_victor].name;
      
      if((gameepisode >= 1 && gameepisode <= 4) && gamemap == 8)
      {
         switch(gameepisode)
         {
         case 1:
            LevelInfo.backDrop  = bgflatE1;
            LevelInfo.interText = s_E1TEXT;
            break;
         case 2:
            LevelInfo.backDrop  = bgflatE2;
            LevelInfo.interText = s_E2TEXT;
            break;
         case 3:
            LevelInfo.backDrop  = bgflatE3;
            LevelInfo.interText = s_E3TEXT;
            break;
         case 4:
            LevelInfo.backDrop  = bgflatE4;
            LevelInfo.interText = s_E4TEXT;
            break;
         }
      }
      else
      {
         LevelInfo.backDrop  = bgflatE1;
         LevelInfo.interText = NULL;
      }
      break;
   
   case commercial:
      // DOOM II modes
      LevelInfo.interMusic = S_music[mus_read_m].name;

      switch(gamemap)
      {
      case 6:
         LevelInfo.backDrop  = bgflat06;
         LevelInfo.interText = 
            gamemission == pack_tnt  ? s_T1TEXT :
            gamemission == pack_plut ? s_P1TEXT : s_C1TEXT;
         break;
      case 11:
         LevelInfo.backDrop  = bgflat11;
         LevelInfo.interText =
            gamemission == pack_tnt  ? s_T2TEXT :
            gamemission == pack_plut ? s_P2TEXT : s_C2TEXT;
         break;
      case 20:
         LevelInfo.backDrop  = bgflat20;
         LevelInfo.interText =
            gamemission == pack_tnt  ? s_T3TEXT :
            gamemission == pack_plut ? s_P3TEXT : s_C3TEXT;
         break;
      case 30:
         LevelInfo.backDrop  = bgflat30;
         LevelInfo.interText =
            gamemission == pack_tnt  ? s_T4TEXT :
            gamemission == pack_plut ? s_P4TEXT : s_C4TEXT;
         LevelInfo.endOfGame = true;
         break;
      case 15:
         LevelInfo.backDrop  = bgflat15;
         LevelInfo.interText =
            gamemission == pack_tnt  ? s_T5TEXT :
            gamemission == pack_plut ? s_P5TEXT : s_C5TEXT;
         LevelInfo.finaleSecretOnly = true; // only after secret exit
         break;
      case 31:
         LevelInfo.backDrop  = bgflat31;
         LevelInfo.interText =
            gamemission == pack_tnt  ? s_T6TEXT :
            gamemission == pack_plut ? s_P6TEXT : s_C6TEXT;
         LevelInfo.finaleSecretOnly = true; // only after secret exit
         break;
      default:
         LevelInfo.backDrop  =  bgflat06;
         LevelInfo.interText = NULL;
         break;
      }
      break;

   case hereticsw:
   case hereticreg:
      // Heretic modes
      LevelInfo.interMusic = H_music[hmus_cptd].name;

      if((gameepisode >= 1 && gameepisode <= 5) && gamemap == 8)
      {
         switch(gameepisode)
         {
         case 1:
            LevelInfo.backDrop  = bgflathE1;
            LevelInfo.interText = s_H1TEXT;
            break;
         case 2:
            LevelInfo.backDrop  = bgflathE2;
            LevelInfo.interText = s_H2TEXT;
            break;
         case 3:
            LevelInfo.backDrop  = bgflathE3;
            LevelInfo.interText = s_H3TEXT;
            break;
         case 4:
            LevelInfo.backDrop  = bgflathE4;
            LevelInfo.interText = s_H4TEXT;
            break;
         case 5:
            LevelInfo.backDrop  = bgflathE5;
            LevelInfo.interText = s_H5TEXT;
            break;
         }
      }
      else
      {
         LevelInfo.backDrop  = bgflathE1;
         LevelInfo.interText = NULL;
      }
      break;

   default:
      LevelInfo.interMusic = NULL;
      LevelInfo.backDrop   = "F_SKY2";
      LevelInfo.interText  = NULL;
      break;
   }
}

//
// P_InfoDefaultSky
//
// Sets the default sky texture for the level.
// Moved here from r_sky.c
//
static void P_InfoDefaultSky(void)
{
   // DOOM determines the sky texture to be used
   // depending on the current episode, and the game version.

   if(gamemode == commercial)
   {      
      if(gamemap < 12)
         LevelInfo.skyName = "SKY1";
      else if(gamemap < 21) 
         LevelInfo.skyName = "SKY2";
      else
         LevelInfo.skyName = "SKY3";
   }
   else if(gameModeInfo->type == Game_Heretic)
   {
      // haleyjd: heretic skies
      switch(gameepisode)
      {
      default: // haleyjd: episode 6, and default to avoid NULL
      case 1:
      case 4:
         LevelInfo.skyName = "SKY1";
         break;
      case 2:
         LevelInfo.skyName = "SKY2";
         break;
      case 3:
      case 5:
         LevelInfo.skyName = "SKY3";
         break;
      }
   }
   else //jff 3/27/98 and lets not forget about DOOM and Ultimate DOOM huh?
   {
      switch(gameepisode)
      {
      case 1:
         LevelInfo.skyName = "SKY1";
         break;
      case 2:
         LevelInfo.skyName = "SKY2";
         break;
      case 3:
         LevelInfo.skyName = "SKY3";
         break;
      case 4:  // Special Edition sky
      default: // haleyjd: don't let sky name end up NULL
         LevelInfo.skyName = "SKY4";
         break;
      }//jff 3/27/98 end sky setting fix
   }

   // set sky2Name to NULL now, we'll test it later
   LevelInfo.sky2Name = NULL;
   LevelInfo.doubleSky = false; // double skies off by default

   // sky deltas for Hexen-style double skies - default to 0
   LevelInfo.skyDelta  = 0;
   LevelInfo.sky2Delta = 0;

   // altSkyName -- this is used for lightning flashes --
   // starts out NULL to indicate none.
   LevelInfo.altSkyName = NULL;
}

//
// P_SetSky2Texture
//
// Post-processing routine.
//
// Sets the sky2Name, if it is still NULL, to whatever 
// skyName has ended up as. This may be the default, or 
// the value from MapInfo.
//
static void P_SetSky2Texture(void)
{
   if(!LevelInfo.sky2Name)
      LevelInfo.sky2Name = LevelInfo.skyName;
}

//
// P_SetParTime
//
// Post-processing routine.
//
// Sets the map's par time, allowing for DeHackEd replacement and
// status as a "new" level. Moved here from g_game.c
//
static void P_SetParTime(void)
{
   if(LevelInfo.partime == -1) // if not set via MapInfo already
   {
      if(!newlevel || deh_pars) // if not a new map, OR deh pars loaded
      {
         switch(gamemode)
         {
         case shareware:
         case retail:
         case registered:
            if(gameepisode >= 1 && gameepisode <= 3 &&
               gamemap >= 1 && gamemap <= 9)
               LevelInfo.partime = TICRATE * pars[gameepisode][gamemap];
            break;
         case commercial:
            if(gamemap >= 1 && gamemap <= 34)
               LevelInfo.partime = TICRATE * cpars[gamemap - 1];
            break;
         case hereticsw:
         case hereticreg:
            LevelInfo.partime = 0; // no par times in Heretic
            break;
         }
      }
   }
   else
      LevelInfo.partime *= TICRATE; // multiply MapInfo value by TICRATE
}

//
// P_ClearLevelVars
//
// Clears all the level variables so that none are left over from a
// previous level. Calls all the default construction functions
// defined above. Post-processing routines are called *after* the
// MapInfo processing is complete, and thus some of the values set
// here are used to detect whether or not MapInfo set a value.
//
static void P_ClearLevelVars(void)
{
   LevelInfo.levelPic   = "";
   LevelInfo.musicName  = "";
   LevelInfo.creator    = "unknown";
   LevelInfo.interPic   = "INTERPIC";
   LevelInfo.partime    = -1;

   LevelInfo.colorMap      = "COLORMAP";
   LevelInfo.useFullBright = true;
   LevelInfo.unevenLight   = true;
   
   LevelInfo.hasLightning = false;
   LevelInfo.nextSecret = "";
   //info_weapons       = "";
   LevelInfo.gravity    = DEFAULTGRAVITY;
   LevelInfo.hasScripts = false;
   LevelInfo.scriptLump = NULL;
   LevelInfo.extraData  = NULL;

   // haleyjd: construct defaults
   P_InfoDefaultLevelName();
   P_InfoDefaultSoundNames();
   P_InfoDefaultFinale();
   P_InfoDefaultSky();
   
   // special handling for ExMy maps under DOOM II
   if(gamemode == commercial && isExMy(levelmapname))
   {
      static char nextlevel[10];
      LevelInfo.nextLevel = nextlevel;
      
      // set the next episode
      strcpy(nextlevel, levelmapname);
      nextlevel[3]++;
      if(nextlevel[3] > '9')  // next episode
      {
         nextlevel[3] = '1';
         nextlevel[1]++;
      }
      LevelInfo.musicName = levelmapname;
   }
   else
      LevelInfo.nextLevel = "";
}

//
// Parsing Utility Functions
//

//
// P_CleanLine
//
// Gets rid of control characters such as \t or \n.
// 
static void P_CleanLine(char *line)
{
   char *temp;
   
   for(temp = line; *temp; ++temp)
      *temp = *temp < 32 ? ' ' : *temp;
}

//
// P_LowerCase
//
// Converts the line into lower case for simplicity of comparisons.
//
static void P_LowerCase(char *line)
{
   char *temp;
   
   for(temp = line; *temp; ++temp)
      *temp = tolower(*temp);
}

//
// P_StripSpaces
//
// Strips whitespace from the end of the line.
//
static void P_StripSpaces(char *line)
{
   char *temp;
   
   temp = line + strlen(line) - 1;
   
   while(*temp == ' ')
   {
      *temp = 0;
      temp--;
   }
}

//
// P_RemoveEqualses
//
// Weirdly-named routine to turn optional = chars into spaces.
//
static void P_RemoveEqualses(char *line)
{
   char *temp;
   
   temp = line;
   
   while(*temp)
   {
      if(*temp == '=')
      {
         *temp = ' ';
      }
      temp++;
   }
}

boolean default_weaponowned[NUMWEAPONS];

// haleyjd: note -- this is considered deprecated and is a
// candidate for replacement/rewrite

static void P_InitWeapons(void)
{
#if 0
   char *s;
#endif
   
   memset(default_weaponowned, 0, sizeof(default_weaponowned));
#if 0   
   s = info_weapons;
   
   while(*s)
   {
      switch(*s)
      {
      case '3': default_weaponowned[wp_shotgun] = true; break;
      case '4': default_weaponowned[wp_chaingun] = true; break;
      case '5': default_weaponowned[wp_missile] = true; break;
      case '6': default_weaponowned[wp_plasma] = true; break;
      case '7': default_weaponowned[wp_bfg] = true; break;
      case '8': default_weaponowned[wp_supershotgun] = true; break;
      default: break;
      }
      s++;
   }
#endif
}

// EOF

