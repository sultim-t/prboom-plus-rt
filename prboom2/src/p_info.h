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
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __P_INFO_H__
#define __P_INFO_H__

#include "c_io.h"

void P_LoadLevelInfo(int lumpnum);

void P_CleanLine(char *line);

extern char *info_interpic;
extern char *info_levelname;
extern char *info_levelpic;
extern char *info_music;
extern int info_partime;
extern char *info_levelcmd[128];
extern char *info_skyname;
extern char *info_creator;
extern char *info_nextlevel;
extern char *info_intertext;
extern char *info_backdrop;
extern int info_scripts;        // whether the current level has scripts

extern boolean default_weaponowned[NUMWEAPONS];

// level menu
// level authors can include a menu in their level to
// activate special features

typedef struct
{
  char *description;
  int scriptnum;
} levelmenuitem_t;


#endif
