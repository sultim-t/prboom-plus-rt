/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: d_main.h,v 1.1 2000/05/04 08:00:41 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
 *      Main startup and splash screenstuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"
#include "w_wad.h"

#ifdef __GNUG__
#pragma interface
#endif

/* CPhipps - removed wadfiles[] stuff to w_wad.h */

// jff make startskill globally visible
extern skill_t startskill;

char *D_DoomExeDir(void); // killough 2/16/98: path to executable's dir

extern char basesavegame[];     // killough 2/16/98: savegame path

//jff 1/24/98 make command line copies of play modes available
extern boolean clnomonsters; // checkparm of -nomonsters
extern boolean clrespawnparm;  // checkparm of -respawn
extern boolean clfastparm; // checkparm of -fast
//jff end of external declaration of command line playmode

extern boolean nosfxparm;
extern boolean nomusicparm;

// Called by IO functions when input is detected.
void D_PostEvent(event_t* ev);
void D_ProcessEvents (void);

// Demo stuff
extern boolean advancedemo;
void D_AdvanceDemo(void);
void D_DoAdvanceDemo (void);

//
// BASE LEVEL
//

void D_PageTicker(void);
void D_PageDrawer(void);
void D_StartTitle(void);
void D_DoomMain(void);
void D_AddFile (const char *file, wad_source_t source);

#endif

//----------------------------------------------------------------------------
//
// $Log: d_main.h,v $
// Revision 1.1  2000/05/04 08:00:41  proff_fs
// Initial revision
//
// Revision 1.7  1999/10/12 13:01:15  cphipps
// Changed header to GPL
//
// Revision 1.6  1999/04/01 22:00:01  cphipps
// Readd D_AddFile() prototype, add w_wad.h to support that
//
// Revision 1.5  1999/03/28 11:31:04  cphipps
// Add a few functions declarations
//
// Revision 1.4  1998/12/22 20:58:30  cphipps
// Remove old wadfiles array prototype
//
// Revision 1.3  1998/10/27 15:21:09  cphipps
// Boom v2.02 update
//
// Revision 1.2  1998/10/16 20:56:50  cphipps
// Removed out-of-date D_AddFile decl.
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
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
