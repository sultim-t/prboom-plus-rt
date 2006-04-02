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
//
// Event-handling structures.
//    
//-----------------------------------------------------------------------------

#ifndef __D_EVENT__
#define __D_EVENT__


#include "doomtype.h"


//
// Event handling.
//

// Input event types.
typedef enum
{
  ev_keydown,
  ev_keyup,
  ev_mouse,
  ev_joystick
} evtype_t;

// Event structure.
typedef struct
{
  evtype_t  type;
  int       data1;    // keys / mouse/joystick buttons
  int       data2;    // mouse/joystick x move
  int       data3;    // mouse/joystick y move
} event_t;

 
typedef enum
{
  ga_nothing,
  ga_loadlevel,
  ga_newgame,
  ga_loadgame,
  ga_savegame,
  ga_playdemo,
  ga_completed,
  ga_victory,
  ga_worlddone,
  ga_screenshot
} gameaction_t;



//
// Button/action code definitions.
//
typedef enum
{
  // Press "Fire".
  BT_ATTACK       = 1,

  // Use button, to open doors, activate switches.
  BT_USE          = 2,

  // Flag: game events, not really buttons.
  BT_SPECIAL      = 128,
  //  BT_SPECIALMASK  = 3,   killough 9/29/98: unused now
    
  // Flag, weapon change pending.
  // If true, the next 4 bits hold weapon num.
  BT_CHANGE       = 4,

  // The 4bit weapon mask and shift, convenience.
//BT_WEAPONMASK   = (8+16+32),
  BT_WEAPONMASK   = (8+16+32+64), // extended to pick up SSG        // phares
  BT_WEAPONSHIFT  = 3,

  // Pause the game.
  BTS_PAUSE       = 1,

  // Save the game at each console.
  BTS_SAVEGAME    = 2,

  // Savegame slot numbers occupy the second byte of buttons.    
  BTS_SAVEMASK    = (4+8+16),
  BTS_SAVESHIFT   = 2,
  
} buttoncode_t;


//
// GLOBAL VARIABLES
//
#define MAXEVENTS               64

extern event_t   events[MAXEVENTS];
extern int       eventhead;
extern int       eventtail;

extern gameaction_t gameaction;

#endif

//----------------------------------------------------------------------------
//
// $Log: d_event.h,v $
// Revision 1.4  1998/05/05  19:55:53  phares
// Formatting and Doc changes
//
// Revision 1.3  1998/02/15  02:48:04  phares
// User-defined keys
//
// Revision 1.2  1998/01/26  19:26:23  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:52  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
