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
 *
 * Level info.
 *
 * Under smmu, level info is stored in the level marker: ie. "mapxx"
 * or "exmx" lump. This contains new info such as: the level name, music
 * lump to be played, par time etc.
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "doomdef.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "t_script.h"
#include "w_wad.h"
#include "z_zone.h"

//----------------------------------------------------------------------------
//
// Helper functions
//

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
      *temp = '\0';
      temp--;
    }
}

static void P_RemoveComments(char *line)
{
  char *temp = line;
  
  while(*temp)
    {
      if(*temp=='/' && *(temp+1)=='/')
	{
	  *temp = '\0'; return;
	}
      temp++;
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

//----------------------------------------------------------------------------
//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

char *info_interpic;
char *info_levelname;
int info_partime;
char *info_music;
char *info_skyname;
char *info_creator="unknown";
char *info_levelpic;
char *info_nextlevel;
char *info_intertext;
char *info_backdrop;
char *info_weapons;
int info_scripts;       // has the current level got scripts?

enum
{
  IVT_STRING,
  IVT_INT,
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
  {IVT_STRING,    "levelpic",     &info_levelpic},
  {IVT_STRING,    "levelname",    &info_levelname},
  {IVT_INT,       "partime",      &info_partime},
  {IVT_STRING,    "music",        &info_music},
  {IVT_STRING,    "skyname",      &info_skyname},
  {IVT_STRING,    "creator",      &info_creator},
  {IVT_STRING,    "interpic",     &info_interpic},
  {IVT_STRING,    "nextlevel",    &info_nextlevel},
  {IVT_INT,       "gravity",      &gravity},
  {IVT_STRING,    "inter-backdrop",&info_backdrop},
  {IVT_STRING,    "defaultweapons",&info_weapons},
  {IVT_END,       0,              0}
};

void P_ParseLevelVar(const char *_cmd)
{
  char varname[50];
  char *equals;
  levelvar_t* current;
  char *cmd = strdup(_cmd);
  
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
      if(!strcasecmp(current->name, varname))
	{
	  switch(current->type)
	    {
	    case IVT_STRING:
        // +5 for safety
	      *(char**)current->variable = Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
	      strcpy(*(char**)current->variable, equals);
	      break;
	      
	    case IVT_INT:
	      *(int*)current->variable = atoi(equals);
                     break;
	    }
	}
      current++;
    }
  free(cmd);
}

// clear all the level variables so that none are left over from a
// previous level

void P_ClearLevelVars(void)
{
  info_levelname = info_skyname = info_levelpic = "";
  info_music = "";
  info_creator = "unknown";
  info_interpic = "INTERPIC";
  info_partime = -1;
  
  if(gamemode == commercial && isExMy(levelmapname))
    {
      static char nextlevel[10];
      info_nextlevel = nextlevel;
      
      // set the next episode
      strcpy(nextlevel, levelmapname);
      nextlevel[3] ++;
      if(nextlevel[3] > '9')  // next episode
	{
	  nextlevel[3] = '1';
	  nextlevel[1] ++;
	}
      
      info_music = levelmapname;
    }
  else 
    info_nextlevel = "";
  
  info_weapons = "";
  gravity = FRACUNIT;     // default gravity

  if(info_intertext)
    {
      Z_Free(info_intertext);
      info_intertext = NULL;
    }

  info_backdrop = NULL;
  
  T_ClearScripts();
  info_scripts = false;
}

//----------------------------------------------------------------------------
//
// P_ParseScriptLine
//
// FraggleScript: if we are reading in script lines, we add the new lines
// into the levelscript
//

void P_ParseScriptLine(const char *line)
{
  int allocsize;

             // +10 for comfort
  allocsize = strlen(line) + strlen(levelscript.data) + 10;
  
  // realloc the script bigger
  levelscript.data =
    Z_Realloc(levelscript.data, allocsize, PU_LEVEL, 0);
  
  // add the new line to the current data using sprintf (ugh)
  sprintf(levelscript.data, "%s%s\n", levelscript.data, line);
}

//-------------------------------------------------------------------------
//
// P_ParseInterText
//
// Add line to the custom intertext
//

void P_ParseInterText(const char *line)
{
  while(*line==' ') line++;
  if(!*line) return;

  if(info_intertext)
    {
      int textlen = strlen(info_intertext);
      
      // realloc bigger
      
      info_intertext =
	Z_Realloc(info_intertext,
		  textlen + strlen(line) + 10,
		  PU_STATIC,
		  0);

      // newline
      
      info_intertext[textlen] = '\n';
      
      // add line to end
      strcpy(info_intertext + textlen + 1, line);
    }
  else
    info_intertext = Z_Strdup(line, PU_STATIC, 0);
}

//---------------------------------------------------------------------------
//
// Setup/Misc. Functions
//

boolean default_weaponowned[NUMWEAPONS];

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

// sf: moved level name finding from hu_stuff.c

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//
// Ty 03/27/98 - externalized map name arrays - now in d_deh.c
// and converted to arrays of pointers to char *
// See modified HUTITLEx macros
//
extern char **mapnames[];
extern char **mapnames2[];
extern char **mapnamesp[];
extern char **mapnamest[];

#define HU_TITLE  (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (*mapnames2[gamemap-1])
#define HU_TITLEP (*mapnamesp[gamemap-1])
#define HU_TITLET (*mapnamest[gamemap-1])

unsigned char *levelname;

void P_FindLevelName(void)
{
  extern char *gamemapname;
  
  // determine the level name        
  // there are a number of sources from which it can come from,
  // getting the right one is the tricky bit =)
  
  // if commerical mode, OLO loaded and inside the confines of the
  // new level names added, use the olo level name
  
  if(gamemode == commercial && olo_loaded
     && (gamemap-1 >= olo.levelwarp && gamemap-1 <= olo.lastlevel) )
    levelname = olo.levelname[gamemap-1];
  
        // info level name from level lump (p_info.c) ?
  
  else if(*info_levelname) levelname = info_levelname;
  
        // not a new level or dehacked level names ?
  
  else if(!newlevel || deh_loaded)
    {
      if(isMAPxy(gamemapname))
	levelname = gamemission == pack_tnt ? HU_TITLET :
	gamemission == pack_plut ? HU_TITLEP : HU_TITLE2;
      else if(isExMy(gamemapname))
	levelname = HU_TITLE;
      else
	levelname = gamemapname;
    }
  else        //  otherwise just put "new level"
    {
      static char newlevelstr[50];
      
      sprintf(newlevelstr, "%s: new level", gamemapname);
      levelname = newlevelstr;
    }
}

//-------------------------------------------------------------------------
//
// P_ParseInfoCmd
//
// We call the relevant function to deal with the line we are given,
// based on readtype. If we get a section divider ([] bracketed) we
// change readtype.
//

enum
{
  RT_LEVELINFO,
  RT_SCRIPT,
  RT_OTHER,
  RT_INTERTEXT
} readtype;

void P_ParseInfoCmd(const char *line)
{  
  if(readtype != RT_SCRIPT)       // not for scripts
    {
      //      P_LowerCase(line);
      while(*line == ' ') line++;
      if(!*line) return;
      if((line[0] == '/' && line[1] == '/') ||     // comment
	 line[0] == '#' || line[0] == ';') return;
    }
  
  if(*line == '[')                // a new section seperator
    {
      line++;
      if(!strncasecmp(line, "level info", 10))
	readtype = RT_LEVELINFO;
      if(!strncasecmp(line, "scripts", 7))
	{
	  readtype = RT_SCRIPT;
	  info_scripts = true;    // has scripts
	}
      if(!strncasecmp(line, "intertext", 9))
	readtype = RT_INTERTEXT;
      return;
    }
  
  switch(readtype)
    {
    case RT_LEVELINFO:
      P_ParseLevelVar(line);
      break;
      
    case RT_SCRIPT:
      P_ParseScriptLine(line);
      break;
      
    case RT_INTERTEXT:
      P_ParseInterText(line);
      break;

    case RT_OTHER:
      break;
    }
}

//-------------------------------------------------------------------------
//
// P_LoadLevelInfo
//
// Load the info lump for a level. Call P_ParseInfoCmd for each
// line of the lump.

void P_LoadLevelInfo(int lumpnum)
{
  const char *lump, *rover;
  char readline[256];
  
  readtype = RT_OTHER;
  P_ClearLevelVars();

  rover = lump = W_CacheLumpNum(lumpnum);
  
  readline[0] = '\0';
  
  while(rover < lump+lumpinfo[lumpnum].size)
    {
      if(*rover == '\n') // end of line
	{
	  P_ParseInfoCmd(readline);  // parse line
	  readline[0] = '\0';        // clear buffer for next line
	}
      else
	// add to line if valid char
	
	if(isprint(*rover))
	  {
	    // add char
	    readline[strlen(readline)+1] = '\0';
	    readline[strlen(readline)] = *rover;	    
	  }
      
      rover++;
    }
  
  // parse last line
  P_ParseInfoCmd(readline);
  
  W_UnlockLumpNum(lumpnum);
  
  P_InitWeapons();
  P_FindLevelName();
}

//-------------------------------------------------------------------------
//
// Console Commands
//

CONST_STRING(info_creator);
CONSOLE_CONST(creator, info_creator);

CONST_STRING(levelname);
CONSOLE_CONST(levelname, levelname);

void P_Info_AddCommands()
{
  C_AddCommand(creator);
  C_AddCommand(levelname);
}
