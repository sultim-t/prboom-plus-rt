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
 * New menu
 *
 * The menu engine. All the menus themselves are in mn_menus.c
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "lprintf.h"
#include "d_main.h"
#include "g_game.h"
//#include "hu_over.h"
#include "i_video.h"
//#include "m_misc.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "v_video.h"

#include "g_bind.h"    // haleyjd: dynamic key bindings

#ifdef COMPILE_VIDD
#include "vidd/vidd.h"
#endif

// useful macro: finds if a particular item can be selected on the menu
#define selectable(it)   (                  \
            (it)->type == it_runcmd   ||    \
	    (it)->type == it_variable ||    \
	    (it)->type == it_toggle   ||    \
	    (it)->type == it_slider   ||    \
	    (it)->type == it_automap  ||    \
	    (it)->type == it_binding        \
					    )

#define selected_item(menu) ( &(menu)->menuitems[(menu)->selected] )

boolean inhelpscreens; // indicates we are in or just left a help screen

// menu error message
char menu_error_message[128];

        // input for typing in new value
static command_t *input_command = NULL;       // NULL if not typing in
static char input_buffer[128] = "";

/////////////////////////////////////////////////////////////////////////////
// 
// MENU DRAWING
//
/////////////////////////////////////////////////////////////////////////////

        // gap from variable description to value
#define GAP 20
#define SKULL_HEIGHT 19
#define BLINK_TIME 8

// colours
#define unselect_colour    CR_RED
#define select_colour      CR_GRAY
#define var_colour         CR_GREEN
#define disabled_colour    CR_TRANS

enum
{
  slider_left,
  slider_right,
  slider_mid,
  slider_slider,
  num_slider_gfx
};
patchnum_t slider_gfx[num_slider_gfx];
static menu_t *drawing_menu;
static patchnum_t skulls[2];

        // width of slider, in mid-patches
#define SLIDE_PATCHES 9

// draw a 'slider' (for sound volume, etc)

static void MN_DrawSlider(int x, int y, int pct)
{
  int i;
  int draw_x = x;
  int slider_width = 0;       // find slider width in pixels
  
  V_DrawNumPatch(draw_x, y, 0, slider_gfx[slider_left].lumpnum, CR_DEFAULT, VPT_STRETCH);
  draw_x += slider_gfx[slider_left].width;
  
  for(i=0; i<SLIDE_PATCHES; i++)
    {
      V_DrawNumPatch(draw_x, y, 0, slider_gfx[slider_mid].lumpnum, CR_DEFAULT, VPT_STRETCH);
      draw_x += slider_gfx[slider_mid].width - 1;
    }
  
  V_DrawNumPatch(draw_x, y, 0, slider_gfx[slider_right].lumpnum, CR_DEFAULT, VPT_STRETCH);
  
  // find position to draw
  
  slider_width = (slider_gfx[slider_mid].width - 1) * SLIDE_PATCHES;
  draw_x = slider_gfx[slider_left].width +
    (pct * (slider_width - slider_gfx[slider_slider].width)) / 100;
  
  V_DrawNumPatch(x + draw_x, y, 0, slider_gfx[slider_slider].lumpnum, CR_DEFAULT, VPT_STRETCH);
}

// draw a menu item. returns the height in pixels

static int MN_DrawMenuItem(menuitem_t *item, int x, int y, int colour)
{
  boolean leftaligned =
    (drawing_menu->flags & mf_skullmenu) ||
    (drawing_menu->flags & mf_leftaligned);

  if(item->type == it_gap) return 8;    // skip drawing if a gap

  item->x = x; item->y = y;       // save x,y to item
 
  // draw an alternate patch instead of text?
 
  if(item->patch)
  {
    int lumpnum;
      
    lumpnum = W_CheckNumForName(item->patch);
      
    // default to text-based message if patch missing
    if(lumpnum >= 0)
	  {
	    int height;
	  
	    height = R_NumPatchHeight(lumpnum);
	  
	    // check for left-aligned
	    if(!leftaligned) x-= R_NumPatchWidth(lumpnum);
	    
	    // adjust x if a centered title
	    if(item->type == it_title)
	      x = (320-R_NumPatchWidth(lumpnum))/2;
	    
	    V_DrawNumPatch(x, y, 0, lumpnum, colour, VPT_STRETCH | VPT_TRANS);
	    
	    return height + 1;   // 1 pixel gap
	} else {
		lprintf(LO_WARN, "mn_engin.c: patch %s not found.\n", item->patch);
	}
  }

  // draw description text
  
  if(item->type == it_title)
  {
    // if it_title, we draw the description centered

    V_WriteTextColoured (
      item->description,
      colour,
      (320-V_StringWidth(item->description, -1))/2,
	    y, -1
	  );
  }
  else
  {
      
    // write description
    V_WriteTextColoured (
      item->description,
      colour,
      x - (leftaligned ? 0 : V_StringWidth(item->description, -1)),
	    y, -1
	  );
  }

  // draw other data: variable data etc.
  
  switch(item->type)      
    {
    case it_title:              // just description drawn
    case it_info:
    case it_runcmd:
    case it_gap:                // just a gap, draw nothing
    case it_disabled: 
      {
        break;
      }

    case it_binding:            // key binding
      {
	 char *boundkeys = G_BoundKeys(item->data);
	 
	 if(drawing_menu->flags & mf_background)
	 {
	    // include gap on fullscreen menus
	    x += GAP;
	    // adjust colour for different coloured variables
	    if(colour == unselect_colour) colour = var_colour;
	 }
	  
	 // write variable value text
	 V_WriteTextColoured
	   (
	    boundkeys,
	    colour,
	    x + (leftaligned ? V_StringWidth(item->description, -1): 0),
	    y, -1
	   );
	 
	 break;
      }

      // it_toggle, it_variable and it_constant are drawn the same
      // the only difference is the way they are changed

      case it_constant:
    case it_toggle:
    case it_variable:
      {
        char varvalue[128];             // temp buffer 

	// create variable description:
	// Use console variable descriptions.
	
	// display input buffer if inputting new var value
	if(input_command && item->var == input_command->variable)
          sprintf(varvalue, "%s_", input_buffer);
	else
          strcpy(varvalue, C_VariableStringValue(item->var));

	if(drawing_menu->flags & mf_background)
	  {
	    // include gap on fullscreen menus
	    x += GAP;
	    // adjust colour for different coloured variables
	    if(colour == unselect_colour) colour = var_colour;
	  }

        // draw it
        V_WriteTextColoured
	  (
	   varvalue,
           colour,
           x + (leftaligned ? V_StringWidth(item->description, -1) : 0),
	   y, -1
	   );
	break;
      }

    // slider

    case it_slider:
      { 
	// draw slider
	  // only int variables

	if(item->var && item->var->type == vt_int)
	  {
	    int range = item->var->max - item->var->min;
	    int posn = *(int *)item->var->variable - item->var->min;
	    
	    MN_DrawSlider(x + GAP, y, (posn*100) / range);
	  }
	
	break;
      }

    // automap colour block

    case it_automap:
      {
	int colour;
	
	  // only with variables, only int variables
	  if(!item->var || item->var->type != vt_int)
	    break;
	
	// find colour of this variable from console variable
	colour = *(int *)item->var->variable;

	// draw it

  V_FillRect(0, (x+GAP)*SCREENWIDTH/320, y*SCREENHEIGHT/200, 8*SCREENWIDTH/320, 8*SCREENHEIGHT/200, (byte)colour);
  //V_DrawNamePatch(x+GAP, y-1, 0, "M_PALSEL", CR_DEFAULT,VPT_STRETCH);
	
	if(!colour)
	  {
	    // draw patch w/cross
	    V_DrawNamePatch(x+GAP+1, y, 0, "M_PALNO", CR_DEFAULT, VPT_STRETCH);
	  }
      }    

    default:
      {
        break;
      }
    }
  
  return 8;   // text height
}

///////////////////////////////////////////////////////////////////////
//
// MAIN FUNCTION
//
// Draw a menu
//

void MN_DrawMenu(menu_t *menu)
{
  int y;
  int itemnum;

  drawing_menu = menu;    // needed by MN_DrawMenuItem
  y = menu->y;
  
  // draw background

  if(menu->flags & mf_background)
    V_DrawBackground(background_flat, 0);
  
  // menu-specific drawer function

  if(menu->drawer) menu->drawer();

  // draw items in menu

  for(itemnum = 0; menu->menuitems[itemnum].type != it_end; itemnum++)
    {
      int item_height;
      int item_colour;

      // choose item colour:
      // if item is the selected item on the menu, use select_colour
      // if item is disabled use disabled_colour
      // otherwise use unselect_colour

      if(menu->selected == itemnum && !(menu->flags & mf_skullmenu))
	item_colour = select_colour;
      else if(menu->menuitems[itemnum].type == it_disabled)
	item_colour = disabled_colour;
      else
	item_colour = unselect_colour;
      
      // draw item

      item_height =
	MN_DrawMenuItem
	(
	 &menu->menuitems[itemnum],
	 menu->x,
	 y,
	 item_colour
	 );
      
      // if selected item, draw skull next to it

      if(menu->flags & mf_skullmenu && menu->selected == itemnum)
        V_DrawNumPatch (
	        menu->x - 30,                                // 30 left
	        y + (item_height - SKULL_HEIGHT) / 2,        // midpoint
	        0,
	        skulls[(menutime / BLINK_TIME) % 2].lumpnum,
          CR_DEFAULT, VPT_STRETCH
	      );
      
      y += item_height;            // go down by item height
    }

  /////////////////////////////////////////////////////////
  //
  // Item help
  //

  // no help msg in skull menu

  if(menu->flags & mf_skullmenu)
    return; 

  // choose help message to print
  
  if(menu_error_message[0])             // error message takes priority
    {
      // make it flash
      V_WriteTextColoured
	(
	 menu_error_message,
	 CR_TAN,
	 10,
	 200 - V_StringHeight(menu_error_message),
   -1
	 );
    }
  else
    {
      char *helpmsg = "";

      // write some help about the item
      menuitem_t *menuitem = selected_item(menu);
      
      if(menuitem->type == it_variable)       // variable
	helpmsg = "press enter to change";
      
      if(menuitem->type == it_toggle)         // togglable variable
	{
	    helpmsg = "use left/right to change value";
	}
      V_WriteTextColoured(helpmsg, CR_GOLD, 10, 200 - V_StringHeight(helpmsg), -1);
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Menu Module Functions
//
// Drawer, ticker etc.
//
/////////////////////////////////////////////////////////////////////////

#define MENU_HISTORY 128

boolean menuactive = false;             // menu active?
menu_t *current_menu;   // the current menu_t being displayed
static menu_t *menu_history[MENU_HISTORY];   // previously selected menus
static int menu_history_num;                 // location in history
int hide_menu = 0;      // hide the menu for a duration of time
int menutime = 0;

// menu widget for alternate drawer + responder
menuwidget_t *current_menuwidget = NULL; 

void MN_LoadData(void)
{
  R_SetPatchNum(&skulls[0], "M_SKULL1");
  R_SetPatchNum(&skulls[1], "M_SKULL2");
  
  // load slider gfx

  R_SetPatchNum(&slider_gfx[slider_left], "M_SLIDEL");
  R_SetPatchNum(&slider_gfx[slider_right], "M_SLIDER");
  R_SetPatchNum(&slider_gfx[slider_mid], "M_SLIDEM");
  R_SetPatchNum(&slider_gfx[slider_slider], "M_SLIDEO");
}

        // init menu
void MN_Init()
{
  MN_LoadData();
  MN_InitMenus();   // create menu commands in mn_menus.c
}

//////////////////////////////////
// ticker

void MN_Ticker()
{
  if(hide_menu)                   // count down hide_menu
    hide_menu--;
  menutime++;
}

////////////////////////////////
// drawer

void MN_Drawer()
{ 
  // redraw needed if menu hidden
  if(hide_menu) redrawsbar = redrawborder = true;

  // activate menu if displaying widget
  if(current_menuwidget) menuactive = true; 

  if(!menuactive || hide_menu) return;

  if(current_menuwidget)
    {
      // alternate drawer
      if(current_menuwidget->drawer)
	current_menuwidget->drawer();
      return;
    }
 
  MN_DrawMenu(current_menu);  
}

// whether a menu item is a 'gap' item
// ie. one that cannot be selected

extern menu_t menu_sound;

/////////////////////////////////
// Responder

extern char *shiftxform; // hu_stuff.c

boolean MN_Responder (event_t *ev)
{
  unsigned char ch;
  static boolean shiftdown;

  // are we displaying a widget?

  if(current_menuwidget)
    return
      current_menuwidget->responder ?
      current_menuwidget->responder(ev) : false;

  // check for shift key
  
  if((ev->type == ev_keydown || ev->type == ev_keyup)
     && ev->data1 == KEYD_RSHIFT)
    {
      shiftdown = ev->type == ev_keydown;
      return false;
    }

  // we only care about key presses

  if(ev->type != ev_keydown)
    return false;

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1;
  
  // are we inputting a new value into a variable?
  
  if(input_command)
    {
      variable_t *var = input_command->variable;
      
      if(ev->data1 == KEYD_ESCAPE)        // cancel input
	input_command = NULL;
      
      if(ev->data1 == KEYD_ENTER)
	{
	  if(input_buffer[0])
	{
	  char *temp;

	  // place " marks round the new value
	  temp = strdup(input_buffer);
	  sprintf(input_buffer, "\"%s\"", temp);
	  free(temp);

	  // set the command
	  cmdtype = c_menu;
	  C_RunCommand(input_command, input_buffer);
	  input_command = NULL;
	    }

	  return true; // eat key
	}

      // check for backspace
      if(ev->data1 == KEYD_BACKSPACE && input_buffer[0])
	{
	  input_buffer[strlen(input_buffer)-1] = '\0';
	  return true; // eatkey
	}
  // probably just a normal character
  
  // only care about valid characters
  // dont allow too many characters on one command line

  if(V_IsPrint(ch) && ch <= 0x80 &&
	   (int)strlen(input_buffer) <=
	   (var->type == vt_string ? var->max :
	   var->type == vt_int ? 10 : 20))
	{
	  input_buffer[strlen(input_buffer) + 1] = 0;
    input_buffer[strlen(input_buffer)] = ch;
	}
	   
      return true;
    } 

  if(ev->data1 == key_escape)
    {
      // toggle menu
#ifdef COMPILE_VIDD
      if (VIDD_handleKeyInput(key_escape, 1)) return false; // POPE
#endif

      // start up main menu or kill menu
      if(menuactive) MN_ClearMenus();
      else MN_StartControlPanel();
      
      S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);

      return true;
    }

  // not interested in keys if not in menu
  if(!menuactive)
    return false;

  if(ev->data1 == key_menu_up)
    {
      // skip gaps
      do
	{
	  if(--current_menu->selected < 0)
            {
	      // jump to end of menu
              int i;
              for(i=0; current_menu->menuitems[i].type != it_end; i++);
              current_menu->selected = i-1;
            }
	}
      while(!selectable(selected_item(current_menu)));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_down)
    {
      do
	{
	  ++current_menu->selected;
	  if(selected_item(current_menu)->type == it_end)
	    {
	      current_menu->selected = 0;     // jump back to start
	    }
	}
      while(!selectable(selected_item(current_menu)));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_enter)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	case it_runcmd:
	  {
	    S_StartSound(NULL,sfx_pistol);  // make sound
	    cmdtype = c_menu;
	    C_RunTextCmd(menuitem->data);
	    break;
	  }
	case it_variable:
	  {
	    menuitem_t *menuitem =
	      &current_menu->menuitems[current_menu->selected];
	    
	    // get input for new value
	    input_command = C_GetCmdForName(menuitem->data);
	    input_buffer[0] = 0;             // clear input buffer
	    break;
	  }

	case it_automap:
	  {
	    MN_SelectColour(menuitem->data);

	    return true;
	  }

	case it_binding:
	  {
	     G_EditBinding(menuitem->data);
	      
	     return true;
	  }

	default: break;
	}
      return true;
    }
  
  if(ev->data1 == key_menu_backspace)
    {
      MN_PrevMenu();
      return true;          // eatkey
    }
  
  // decrease value of variable
  if(ev->data1 == key_menu_left)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
	  {
	    // change variable
	    cmdtype = c_menu;
	    C_RunTextCmdf("%s -", menuitem->data);
	  }
	default:
	  {
	    break;
	  }
	}
      return true;
    }
  
  // increase value of variable
  if(ev->data1 == key_menu_right)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
	  {
	    // change variable
	    cmdtype = c_menu;
	    C_RunTextCmdf("%s +", menuitem->data);
	  }
	
	default:
	  {
	    break;
	  }
	}
      return true;
    }

  // search for matching item in menu

  ch = tolower(ev->data1);
  if(isalnum(ch))
    {
      
      // sf: experimented with various algorithms for this
      //     this one seems to work as it should

      int n = current_menu->selected;

      do
	{
	  n++;
	  if(current_menu->menuitems[n].type == it_end) n = 0; // loop round

	  // ignore unselectables
	  if(selectable(&current_menu->menuitems[n])) 
	    if(tolower(current_menu->menuitems[n].description[0]) == ch)
	      {
		// found a matching item!
		current_menu->selected = n;
		return true; // eat key
	      }
      	} while(n != current_menu->selected);
    }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////
//
// Other Menu Functions
//

// ?
void MN_ResetMenu()
{
}

// make menu 'clunk' sound on opening

void MN_ActivateMenu()
{
  if(!menuactive)  // activate menu if not already
    {
      menuactive = true;
      S_StartSound(NULL, sfx_swtchn);
    }
}

////////////////////////////////////////////////////////////////////////////
//
// Start Menu
//

// ensure we have a valid item selected on a menu
static void MN_SelectValidItem(menu_t *menu)
{
  int start = menu->selected;
  
  while(!selectable(selected_item(menu)))
    {
      ++menu->selected;
      if(selected_item(menu)->type == it_end)
	{
	  menu->selected = 0;     // jump back to start
	}
      if(menu->selected == start)
	{
	  // no selectable items
	  return;  
	}
    }
}

/////////////////////////////
//
// Menu caching
//

static void MN_GetItemVariable(menuitem_t *item)
{
  // get variable if neccesary

  if(!item->var)
    {
      command_t *cmd;
      // use data for variable name
      if(!(cmd = C_GetCmdForName(item->data)))
        {
          C_Printf("variable not found: %s\n", item->data);
	  item->type = it_disabled;   // disable item
	  // possibly change selected item on menu
	  MN_SelectValidItem(drawing_menu);
	  return;
        }
      item->var = cmd->variable;
    }
}

// Called on menu activation.
// Get all variables for menu items
// cache any other stuff
static void MN_CacheMenu(menu_t *menu)
{
  int i;

  for(i=0; menu->menuitems[i].type != it_end; i++)
    {
      if(menu->menuitems[i].type == it_variable ||
	 menu->menuitems[i].type == it_toggle ||
	 menu->menuitems[i].type == it_slider ||
	 menu->menuitems[i].type == it_automap ||
	 menu->menuitems[i].type == it_constant)
	MN_GetItemVariable(&menu->menuitems[i]);
    }
}

// start a menu:

void MN_StartMenu(menu_t *menu)
{
  MN_CacheMenu(menu);

  // ensure we have a selectable item selected on new menu
  MN_SelectValidItem(menu);

  // no selectable items? then dont start menu
  if(!selectable(selected_item(menu)))
    return;
  
  if(!menuactive)
    {
      MN_ActivateMenu();
      current_menu = menu;
      menu_history_num = 0;  // reset history
    }
  else
    {
      menu_history[menu_history_num++] = current_menu;
      current_menu = menu;
    }
  
  menu_error_message[0] = '\0';      // clear error message
  redrawsbar = redrawborder = true;  // need redraw
}

// go back to a previous menu

void MN_PrevMenu()
{
  if(--menu_history_num < 0) MN_ClearMenus();
  else
    current_menu = menu_history[menu_history_num];
      
  menu_error_message[0] = '\0';          // clear errors
  redrawsbar = redrawborder = true;  // need redraw
  S_StartSound(NULL, sfx_swtchx);
}

// turn off menus
void MN_ClearMenus()
{
  menuactive = false;
  redrawsbar = redrawborder = true;  // need redraw
}

CONSOLE_COMMAND(mn_clearmenus, 0)
{
  MN_ClearMenus();
}

CONSOLE_COMMAND(mn_prevmenu, 0)
{
  MN_PrevMenu();
}

        // ??
void MN_ForcedLoadGame(const char *msg)
{
}

// display error msg in popup display at bottom of screen

void MN_ErrorMsg(const char *s, ...)
{
  va_list args;
  
  va_start(args, s);
  vsprintf(menu_error_message, s, args);
  va_end(args);
}

// activate main menu

extern menu_t menu_main;

void MN_StartControlPanel()
{
  MN_StartMenu(&menu_main);
  
  S_StartSound(NULL,sfx_swtchn);
}

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

extern void MN_AddMenus();              // mn_menus.c
extern void MN_AddMiscCommands();       // mn_misc.c
//extern void MN_File_AddCommands();       // mn_files.c
//extern void MN_Net_AddCommands();        // mn_net.c

void MN_AddCommands()
{
  C_AddCommand(mn_clearmenus);
  C_AddCommand(mn_prevmenu);

  MN_AddMenus();               // add commands to call the menus
  MN_AddMiscCommands();
//  MN_File_AddCommands();
//  MN_Net_AddCommands();
}
