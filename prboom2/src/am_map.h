/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: am_map.h,v 1.1 2000/05/04 07:58:43 proff_fs Exp $
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
 *  AutoMap module.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __AMMAP_H__
#define __AMMAP_H__

#include "d_event.h"

// Used by ST StatusBar stuff.
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))

// Called by main loop.
boolean AM_Responder (event_t* ev);

// Called by main loop.
void AM_Ticker (void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer (void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop (void);

// killough 2/22/98: for saving automap information in savegame:

extern void AM_Start(void);

//jff 4/16/98 make externally available

extern void AM_clearMarks(void);

typedef struct
{
 fixed_t x,y;
} mpoint_t;

extern mpoint_t *markpoints;
extern int markpointnum, markpointnum_max;

// end changes -- killough 2/22/98

// killough 5/2/98: moved from m_misc.c

//jff 1/7/98 automap colors added
extern int mapcolor_back;     // map background
extern int mapcolor_grid;     // grid lines color
extern int mapcolor_wall;     // normal 1s wall color
extern int mapcolor_fchg;     // line at floor height change color
extern int mapcolor_cchg;     // line at ceiling height change color
extern int mapcolor_clsd;     // line at sector with floor=ceiling color
extern int mapcolor_rkey;     // red key color
extern int mapcolor_bkey;     // blue key color
extern int mapcolor_ykey;     // yellow key color
extern int mapcolor_rdor;     // red door color (diff from keys to allow option)
extern int mapcolor_bdor;     // blue door color (of enabling one not other)
extern int mapcolor_ydor;     // yellow door color
extern int mapcolor_tele;     // teleporter line color
extern int mapcolor_secr;     // secret sector boundary color
//jff 4/23/98
extern int mapcolor_exit;     // exit line
extern int mapcolor_unsn;     // computer map unseen line color
extern int mapcolor_flat;     // line with no floor/ceiling changes
extern int mapcolor_sprt;     // general sprite color
extern int mapcolor_hair;     // crosshair color
extern int mapcolor_sngl;     // single player arrow color
extern int mapcolor_plyr[4];  // colors for players in multiplayer
extern int mapcolor_me;       // consoleplayer's chosen colour
//jff 3/9/98
extern int map_secret_after;  // secrets do not appear til after bagged

#endif

//----------------------------------------------------------------------------
//
// $Log: am_map.h,v $
// Revision 1.1  2000/05/04 07:58:43  proff_fs
// Initial revision
//
// Revision 1.3  1999/10/12 13:01:15  cphipps
// Changed header to GPL
//
// Revision 1.2  1999/03/26 11:10:51  cphipps
// Removed some depreciated automap mode variables
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.7  1998/05/10  12:05:18  jim
// formatted/documented am_map
//
// Revision 1.6  1998/05/03  22:12:58  killough
// Add most external automap variable declarations
//
// Revision 1.5  1998/04/16  16:17:05  jim
// Fixed disappearing marks after new level
//
// Revision 1.4  1998/03/02  11:23:10  killough
// Add automap_grid decl for savegames
//
// Revision 1.3  1998/02/23  04:10:11  killough
// Remove limit on automap marks, save them in savegame
//
// Revision 1.2  1998/01/26  19:26:19  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:50  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
