/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: st_stuff.h,v 1.1 2000/05/04 08:17:32 proff_fs Exp $
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
 *      Status bar code.
 *      Does the face/direction indicator animatin.
 *      Does palette indicators as well (red pain/berserk, bright pickup)
 *
 *-----------------------------------------------------------------------------*/

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"

// Size of statusbar.
// Now sensitive for scaling.

#define ST_HEIGHT 32
// proff 08/18/98: Changed for high-res
#define ST_WIDTH  320
// CPhipps - if drawing status bar direct then add the offset, else draw 
// at the top of the dedicated buffer
#define ST_Y      (st_fgscreen ? 0 : (SCREENHEIGHT - st_height))
// CPhipps - the ST_T macros have the true, on-screen values always, as used by other
// code for screen positionings
#define ST_TY     (SCREENHEIGHT - st_height)
#define ST_TWIDTH st_width
extern int st_height, st_width; // Actual on-screen size of status bar
extern int st_fgscreen;

//
// STATUS BAR
//

// Called by main loop.
boolean ST_Responder(event_t* ev);

// Called by main loop.
void ST_Ticker(void);

// Called by main loop.
void ST_Drawer(boolean fullscreen, boolean refresh);

// Called when the console player is spawned on each level.
void ST_Start(void);

// Called by startup code.
void ST_Init(void);

// States for status bar code.
typedef enum
{
  AutomapState,
  FirstPersonState
} st_stateenum_t;

// States for the chat code.
typedef enum
{
  StartChatState,
  WaitDestState,
  GetChatState
} st_chatstateenum_t;

boolean ST_Responder(event_t* ev);

// killough 5/2/98: moved from m_misc.c:

extern int health_red;    // health amount less than which status is red
extern int health_yellow; // health amount less than which status is yellow
extern int health_green;  // health amount above is blue, below is green
extern int armor_red;     // armor amount less than which status is red
extern int armor_yellow;  // armor amount less than which status is yellow
extern int armor_green;   // armor amount above is blue, below is green
extern int ammo_red;      // ammo percent less than which status is red
extern int ammo_yellow;   // ammo percent less is yellow more green
extern int sts_always_red;// status numbers do not change colors
extern int sts_pct_always_gray;// status percents do not change colors
extern int sts_traditional_keys;  // display keys the traditional way

#endif

//----------------------------------------------------------------------------
//
// $Log: st_stuff.h,v $
// Revision 1.1  2000/05/04 08:17:32  proff_fs
// Initial revision
//
// Revision 1.6  1999/10/12 13:01:16  cphipps
// Changed header to GPL
//
// Revision 1.5  1999/09/05 20:15:34  cphipps
// Remove st_scale extern
//
// Revision 1.4  1999/02/08 09:09:00  cphipps
// Fix status bar drawing fix separate width variable
//
// Revision 1.3  1999/02/08 08:47:01  cphipps
// New variables and macros for status bar scaling
//
// Revision 1.2  1998/11/17 12:22:58  cphipps
// Hi-res changes
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.4  1998/05/03  22:50:55  killough
// beautification, move external declarations, remove cheats
//
// Revision 1.3  1998/04/19  01:10:39  killough
// Generalize cheat engine to add deh support
//
// Revision 1.2  1998/01/26  19:27:56  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:04  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
