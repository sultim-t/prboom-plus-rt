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
 *-----------------------------------------------------------------------------
 */

#ifndef __MN_MENU__
#define __MN_MENU__

#include "c_runcmd.h"
#include "d_event.h"

typedef struct menu_s menu_t;
typedef struct menuitem_s menuitem_t;
typedef struct menuwidget_s menuwidget_t;

// responder for events

boolean MN_Responder (event_t *ev);

// Called by main loop,
// only used for menu (skull cursor) animation.

void MN_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.

void MN_DrawMenu(menu_t *menu);
void MN_Drawer (void);

// Called by D_DoomMain,
// loads the config file.

void MN_Init (void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.

void MN_StartControlPanel (void);

void MN_ForcedLoadGame(const char *msg); // killough 5/15/98: forced loadgames

void MN_ResetMenu(void);      // killough 11/98: reset main menu ordering

void MN_DrawBackground(char *patch, byte *screen);  // killough 11/98

void MN_DrawCredits(void);    // killough 11/98

void MN_ActivateMenu();
void MN_StartMenu(menu_t *menu);         // sf 10/99
void MN_PrevMenu();
void MN_ClearMenus();                    // sf 10/99

//
// menu_t
//

#define MAXMENUITEMS 128

struct menuitem_s
{
  // item types
  enum
  {
    it_gap,              // empty line
    it_runcmd,           // run console command
    it_constant,         // like variable, but cannot be changed
    it_variable,         // variable
                         // enter pressed to type in new value
    it_toggle,           // togglable variable
                         // can use left/right to change value
    it_title,            // the menu title
    it_info,             // information / section header
    it_slider,           // slider
    it_automap,          // an automap colour
    it_disabled,         // disabled item
    it_binding,          // key binding
    it_end,              // last menuitem in the list
  } type;
  
  // the describing name of this item
  const char *description;

  // useful data for the item:
  // console command if console
  // variable name if variable, etc

  const char *data;         

  // patch to use or NULL
  const char *patch;

                  /*** internal stuff used by menu code ***/
                  // messing with this is a bad idea(prob)

  int x, y;
  variable_t *var;        // ptr to console variable
};

struct menu_s
{
  menuitem_t menuitems[MAXMENUITEMS];

  // x,y offset of menu
  int x, y;
  
  // currently selected item
  int selected;

  // menu flags
  enum
  {
    mf_skullmenu =1,    // show skull rather than highlight
    mf_background=2,    // show background
    mf_leftaligned=4,   // left-aligned menu
  } flags;               
  void (*drawer)();       // seperate drawer function 
};

// menu 'widgets':
// A structured way for the menu to display things
// other than the usual menus
//
// if current_menuwidget is not NULL, the drawer in
// the menuwidget pointed to by it is called by
// MN_Drawer. Also events caught by MN_Responder are
// sent to current_menuwidget->responder

struct menuwidget_s
{
  void (*drawer)();
  boolean (*responder)(event_t *ev);
};

extern boolean menuactive;

extern menu_t *current_menu;                  // current menu being drawn
extern menuwidget_t *current_menuwidget;      // current widget being drawn

// size of automap colour blocks
#define BLOCK_SIZE 9

void MN_ErrorMsg(const char *s, ...);

// menu error message
extern char menu_error_message[128];

extern int hide_menu;
extern int menutime;

#endif
                            
