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
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"
#include "p_chase.h"

extern char **wadfiles;       // killough 11/98

// jff make startskill globally visible
extern skill_t startskill;
extern char *startlevel;

void D_AddFile(char *file);
void D_ListWads(void);
void D_ReInitWadfiles(void);
void D_NewWadLumps(int handle, int sound_update_type);
boolean D_AddNewFile(char *s);


char *D_DoomExeDir(void);       // killough 2/16/98: path to executable's dir
char *D_DoomExeName(void);      // killough 10/98: executable's name
void NormalizeSlashes(char *);  // killough 11/98
extern char basesavegame[];     // killough 2/16/98: savegame path

//jff 1/24/98 make command line copies of play modes available
extern boolean clnomonsters; // checkparm of -nomonsters
extern boolean clrespawnparm;  // checkparm of -respawn
extern boolean clfastparm; // checkparm of -fast
//jff end of external declaration of command line playmode

extern boolean nosfxparm;
extern boolean nomusicparm;

extern int use_startmap;
extern boolean redrawsbar, redrawborder;


// Called by IO functions when input is detected.
void D_PostEvent(event_t* ev);

extern camera_t *camera;
extern char firstlevel[9];       // sf: first level of new wads

//
// BASE LEVEL
//

void D_PageTicker(void);
void D_PageDrawer(void);
void D_AdvanceDemo(void);
void D_StartTitle(void);
void D_DoomMain(void);

// sf: display a message to the player: either in text mode or graphics
void usermsg(const char *s, ...);

#endif

//----------------------------------------------------------------------------
//
// $Log: d_main.h,v $
// Revision 1.7  1998/05/06  15:32:19  jim
// document g_game.c, change externals
//
// Revision 1.5  1998/05/03  22:27:08  killough
// Add external declarations
//
// Revision 1.4  1998/02/23  04:15:01  killough
// Remove obsolete function prototype
//
// Revision 1.3  1998/02/17  06:10:39  killough
// Add D_DoomExeDir prototype, basesavegame decl.
//
// Revision 1.2  1998/01/26  19:26:28  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
