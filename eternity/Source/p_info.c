// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
// Under smmu, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//
// By Simon Howard
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
#include "e_sound.h"

// haleyjd: TODO
// Shouldn't all of this really be in a struct?

char *info_interpic;
char *info_levelname;
int  info_partime;
char *info_music;
char *info_skyname;
char *info_creator = "unknown";
char *info_levelpic;
char *info_nextlevel;
char *info_intertext;
char *info_backdrop;
char *info_weapons;

boolean info_scripts;       // has the current level got scripts?
char *info_scriptlump;      // haleyjd: name of script lump

char *info_altskyname; // haleyjd : new mapinfo stuff
char *info_colormap;
boolean info_lightning;
char *info_sky2name;
int  info_skydelta;
int  info_sky2delta;
char *info_nextsecret;
boolean info_killfinale;
boolean info_endofgame;
char *info_extradata;  // haleyjd: name of ExtraData lump

char *info_sound_swtchn; // haleyjd: environment sounds
char *info_sound_swtchx;
char *info_sound_stnmov;
char *info_sound_pstop;
char *info_sound_bdcls;
char *info_sound_bdopn;
char *info_sound_doropn;
char *info_sound_dorcls;
char *info_sound_pstart;

void P_LowerCase(char *line);
void P_StripSpaces(char *line);
static void P_RemoveEqualses(char *line);

void P_ParseInfoCmd(char *line);
void P_ParseLevelVar(char *cmd);
void P_ParseInfoCmd(char *line);

void P_ClearLevelVars(void);
void P_InitWeapons(void);

enum
{
   RT_LEVELINFO,
   RT_OTHER,
} readtype;

void P_LoadLevelInfo(int lumpnum)
{
   char *lump;
   char *rover;
   char *startofline;
   
   readtype = RT_OTHER;
   P_ClearLevelVars();
   
   lump = W_CacheLumpNum(lumpnum, PU_STATIC);
   
   rover = startofline = lump;
   
   while(rover < lump+lumpinfo[lumpnum]->size)
   {
      if(*rover == '\n') // end of line
      {
         *rover = 0;               // make it an end of string (0)
         P_ParseInfoCmd(startofline);
         startofline = rover+1; // next line
         *rover = '\n';            // back to end of line
      }
      rover++;
   }
   Z_Free(lump);
   
   P_InitWeapons();
}

void P_ParseInfoCmd(char *line)
{
   P_CleanLine(line);

   P_StripSpaces(line);
   P_LowerCase(line);
   
   while(*line == ' ') 
      line++;
   
   if(!*line) 
      return;
   
   if((line[0] == '/' && line[1] == '/') ||     // comment
      line[0] == '#' || line[0] == ';') return;
  
   if(*line == '[')                // a new section seperator
   {
      line++;
      
      if(!strncmp(line, "level info", 10))
         readtype = RT_LEVELINFO;
   }
  
   switch(readtype)
   {
   case RT_LEVELINFO:
      P_ParseLevelVar(line);
      break;

   case RT_OTHER:
      break;
   }
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
   {IVT_STRING, "levelpic",      &info_levelpic},
   {IVT_STRING, "levelname",     &info_levelname},
   {IVT_INT,    "partime",       &info_partime},
   {IVT_STRING, "music",         &info_music},
   {IVT_STRING, "skyname",       &info_skyname},
   {IVT_STRING, "creator",       &info_creator},
   {IVT_STRING, "interpic",      &info_interpic},
   {IVT_STRING, "nextlevel",     &info_nextlevel},
   {IVT_INT,    "gravity",       &gravity},
   {IVT_STRING, "inter-backdrop",&info_backdrop},
   {IVT_STRING, "defaultweapons",&info_weapons},
   {IVT_STRING, "altskyname",    &info_altskyname},
   {IVT_STRING, "colormap",      &info_colormap},   // haleyjd
   {IVT_BOOLEAN,"lightning",     &info_lightning},
   {IVT_STRING, "sky2name",      &info_sky2name},
   {IVT_INT,    "skydelta",      &info_skydelta},
   {IVT_INT,    "sky2delta",     &info_sky2delta},
   {IVT_STRING, "sound-swtchn",  &info_sound_swtchn},
   {IVT_STRING, "sound-swtchx",  &info_sound_swtchx},
   {IVT_STRING, "sound-stnmov",  &info_sound_stnmov},
   {IVT_STRING, "sound-pstop",   &info_sound_pstop},
   {IVT_STRING, "sound-bdcls",   &info_sound_bdcls},
   {IVT_STRING, "sound-bdopn",   &info_sound_bdopn},
   {IVT_STRING, "sound-dorcls",  &info_sound_dorcls},
   {IVT_STRING, "sound-doropn",  &info_sound_doropn},
   {IVT_STRING, "sound-pstart",  &info_sound_pstart},
   {IVT_STRING, "nextsecret",    &info_nextsecret},
   {IVT_BOOLEAN,"killfinale",    &info_killfinale},
   {IVT_BOOLEAN,"endofgame",     &info_endofgame},
   {IVT_STRING, "intertext",     &info_intertext}, // haleyjd 12/13/01
   {IVT_STRING, "levelscript",   &info_scriptlump}, // haleyjd
   {IVT_STRING, "extradata",     &info_extradata}, // haleyjd 04/02/03
   {IVT_END,    0,               0}
};

void P_ParseLevelVar(char *cmd)
{
   char varname[50];
   char *equals;
   levelvar_t* current;

   if(!*cmd) return;
   
   P_RemoveEqualses(cmd);
   
   // right, first find the variable name
   
   sscanf(cmd, "%s", varname);
   
   // find what it equals
   equals = cmd+strlen(varname);
   while(*equals == ' ') equals++; // cut off the leading spaces
  
   current = levelvars;
   
   while(current->type != IVT_END)
   {
      if(!strcmp(current->name, varname))
      {
         switch(current->type)
         {
         case IVT_STRING:
            *(char**)current->variable         // +5 for safety
               = Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
            strcpy(*(char**)current->variable, equals);
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

   // haleyjd 03/15/03: handle level scripts
   if(info_scriptlump)
      info_scripts = true;
}

static void GetDefSound(char **var, int dehnum)
{
   sfxinfo_t *sfx;

   sfx = E_SoundForDEHNum(dehnum);

   *var = sfx ? sfx->mnemonic : "none";
}

static void LoadDefaultSoundNames(void)
{
   // haleyjd 03/17/03: now uses gameModeInfo for defaults
   int *infoSounds = gameModeInfo->infoSounds;

   GetDefSound(&info_sound_doropn, infoSounds[INFO_DOROPN]);
   GetDefSound(&info_sound_dorcls, infoSounds[INFO_DORCLS]);
   GetDefSound(&info_sound_bdopn,  infoSounds[INFO_BDOPN ]);
   GetDefSound(&info_sound_bdcls,  infoSounds[INFO_BDCLS ]);
   GetDefSound(&info_sound_swtchn, infoSounds[INFO_SWTCHN]);
   GetDefSound(&info_sound_swtchx, infoSounds[INFO_SWTCHX]);
   GetDefSound(&info_sound_pstart, infoSounds[INFO_PSTART]);
   GetDefSound(&info_sound_pstop,  infoSounds[INFO_PSTOP ]);
   GetDefSound(&info_sound_stnmov, infoSounds[INFO_STNMOV]);
}

// clear all the level variables so that none are left over from a
// previous level

void P_ClearLevelVars(void)
{
   info_levelname  = "";
   info_skyname    = "";
   info_levelpic   = "";
   info_music      = "";
   info_creator    = "unknown";
   info_interpic   = "INTERPIC";
   info_partime    = -1;
   info_altskyname = "";         // haleyjd
   info_colormap   = "COLORMAP";
   info_lightning  = false;
   info_killfinale = false; // TODO: adjust defaults?
   info_sky2name   = "-";
   info_skydelta   = 0;
   info_sky2delta  = 0;
   info_nextsecret = "";
   info_weapons    = "";
   gravity         = FRACUNIT;     // default gravity
   info_intertext  = NULL;
   info_backdrop   = NULL;
   info_scripts    = false;
   info_scriptlump = NULL;
   info_extradata  = NULL;

   LoadDefaultSoundNames(); // haleyjd

   // variables with special defaults
   
   if(gamemode == commercial && gamemap == 30)
      info_endofgame = true;
   else
      info_endofgame = false;
   
   if(gamemode == commercial && isExMy(levelmapname))
   {
      static char nextlevel[10];
      info_nextlevel = nextlevel;
      
      // set the next episode
      strcpy(nextlevel, levelmapname);
      nextlevel[3]++;
      if(nextlevel[3] > '9')  // next episode
      {
         nextlevel[3] = '1';
         nextlevel[1] ++;
      }
      info_music = levelmapname;
   }
   else
      info_nextlevel = "";
}

void P_CleanLine(char *line)
{
   char *temp;
   
   for(temp=line; *temp; temp++)
      *temp = *temp<32 ? ' ' : *temp;
}

void P_LowerCase(char *line)
{
   char *temp;
   
   for(temp=line; *temp; temp++)
      *temp = tolower(*temp);
}

void P_StripSpaces(char *line)
{
   char *temp;
   
   temp = line+strlen(line)-1;
   
   while(*temp == ' ')
   {
      *temp = 0;
      temp--;
   }
}

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

void P_InitWeapons(void)
{
   char *s;
   
   memset(default_weaponowned, 0, sizeof(default_weaponowned));
   
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
}

// EOF

