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

void P_CleanLine(char *line);

extern char *info_interpic;
extern char *info_levelname;
extern char *info_levelpic;
extern char *info_music;
extern int  info_partime;
extern char *info_skyname;
extern char *info_creator;
extern char *info_nextlevel;
extern char *info_intertext;
extern char *info_backdrop;
extern boolean info_scripts; // whether the current level has scripts
extern char *info_scriptlump;
extern char *info_altskyname; // haleyjd : new mapinfo stuff
extern char *info_colormap;
extern boolean info_lightning;
extern char *info_sky2name;
extern int  info_skydelta;
extern int  info_sky2delta;
extern char *info_nextsecret;
extern boolean info_killfinale;
extern boolean info_endofgame;
extern char *info_extradata; // haleyjd: name of ExtraData lump

// map sound replacements

extern char *info_sound_swtchn;
extern char *info_sound_swtchx;
extern char *info_sound_stnmov;
extern char *info_sound_pstop;
extern char *info_sound_bdcls;
extern char *info_sound_bdopn;
extern char *info_sound_doropn;
extern char *info_sound_dorcls;
extern char *info_sound_pstart;

extern boolean default_weaponowned[NUMWEAPONS];


#endif

// EOF

