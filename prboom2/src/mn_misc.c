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
 * Menu - misc functions
 *
 * Pop up alert/question messages
 * Miscellaneous stuff
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

static const char rcsid[] =

#include <stdarg.h>

#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_video.h"
//#include "v_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "mn_engin.h"
#include "mn_misc.h"

/////////////////////////////////////////////////////////////////////////
//
// Pop-up Messages
//

char popup_message[128];
const char *popup_message_command;            // console command to run
enum
{
  popup_alert,
  popup_question
} popup_message_type;

static void WriteCentredText(char *message)
{
  static char *tempbuf = NULL;
  static int allocedsize=0;
  char *rover, *addrover;
  int x, y;

  // rather than reallocate memory every time we draw it,
  // use one buffer and increase the size as neccesary
  if(strlen(message) > (size_t)allocedsize)
    {
      if(tempbuf)
	tempbuf = Z_Realloc(tempbuf, strlen(message)+3, PU_STATIC, 0);
      else
	tempbuf = Z_Malloc(strlen(message)+3, PU_STATIC, 0);

      allocedsize = strlen(message);
    }

  y = (200 - V_StringHeight(popup_message)) / 2;
  addrover = tempbuf;
  rover = message;

  while(*rover)
    {
      if(*rover == '\n')
	{
	  *addrover = 0;  // end string
	  x = (320 - V_StringWidth(tempbuf, 0)) / 2;
	  V_WriteText(tempbuf, x, y, 0);
	  addrover = tempbuf;  // reset addrover
	  y += 7; // next line
	}
      else      // add next char
	{
	  *addrover = *rover;
	  addrover++;
	}
      rover++;
    }

  // dont forget the last line.. prob. not \n terminated

  *addrover = 0;
  x = (320 - V_StringWidth(tempbuf, 0)) / 2;
  V_WriteText(tempbuf, x, y, 0);
}

void MN_PopupDrawer()
{
  WriteCentredText(popup_message);
}

boolean MN_PopupResponder(event_t *ev)
{
  if(ev->type != ev_keydown) return false;

  switch(popup_message_type)
    {
    case popup_alert:
	{
	  // kill message
	  redrawsbar = redrawborder = true; // need redraw
	  current_menuwidget = NULL;
	  S_StartSound(NULL, sfx_swtchx);
	}
      break;

    case popup_question:
      if(tolower(ev->data1) == 'y')     // yes!
	{
	  // run command and kill message
	  menuactive = false; // kill menu
	  cmdtype = c_menu;
	  C_RunTextCmd(popup_message_command);
	  S_StartSound(NULL, sfx_pistol);
	  redrawsbar = redrawborder = true; // need redraw
	  current_menuwidget = NULL;  // kill message
	}
      if(tolower(ev->data1) == 'n' || ev->data1 == KEYD_ESCAPE
	 || ev->data1 == KEYD_BACKSPACE)     // no!
	{
	  // kill message
	  menuactive = false; // kill menu
	  S_StartSound(NULL, sfx_swtchx);
	  redrawsbar = redrawborder = true; // need redraw
	  current_menuwidget = NULL; // kill message
	}
      break;
      
    default:
      break;
    }
  
  return true; // always eatkey
}

// widget for popup message alternate draw
menuwidget_t popup_widget = {MN_PopupDrawer, MN_PopupResponder};

// alert message
// -- just press enter

void MN_Alert(const char *message, ...)
{
  va_list args;

  MN_ActivateMenu();

  // hook in widget so message will be displayed
  current_menuwidget = &popup_widget;
  popup_message_type = popup_alert;
  
  va_start(args, message);
  vsprintf(popup_message, message, args);
  va_end(args);
}

// question message
// console command will be run if user responds with 'y'

void MN_Question(const char *message, const char *command)
{
  MN_ActivateMenu();

  // hook in widget so message will be displayed
  current_menuwidget = &popup_widget;

  strncpy(popup_message, message, 126);
  popup_message_type = popup_question;
  popup_message_command = command;
}

//////////////////////////////////////////////////////////////////////////
//
// Credits Screens
//

void MN_DrawCredits(void);

typedef struct
{
  int lumpnum;
  void (*Drawer)(); // alternate drawer
} helpscreen_t;

static helpscreen_t helpscreens[120];  // 100 + credit/built-in help screens
static int num_helpscreens;
static int viewing_helpscreen;     // currently viewing help screen
extern boolean inhelpscreens; // indicates we are in or just left a help screen

static void AddHelpScreen(char *screenname)
{
  int lumpnum;

  if(-1 != (lumpnum = W_CheckNumForName(screenname)))
    {
      helpscreens[num_helpscreens].Drawer = NULL;   // no drawer
      helpscreens[num_helpscreens++].lumpnum = lumpnum;
    }  
}

// build help screens differently according to whether displaying
// help or displaying credits

static void MN_FindCreditScreens()
{
  num_helpscreens = 0;  // reset

  // add dynamic smmu credits screen

  helpscreens[num_helpscreens++].Drawer = MN_DrawCredits;

  // other help screens

  AddHelpScreen("HELP2");       // shareware screen
  AddHelpScreen("CREDIT");      // credits screen
}

static void MN_FindHelpScreens()
{
  int custom;

  num_helpscreens = 0;

  // add custom menus first

  for(custom = 0; custom<100; custom++)
    {
      char tempstr[10];
      sprintf(tempstr, "HELP%.02i", custom);
      AddHelpScreen(tempstr);
    }

  // now the default original doom ones

  // sf: keep original help screens until key bindings rewritten
  // and i can restore the dynamic help screens

  AddHelpScreen("HELP");
  AddHelpScreen("HELP1");

  // promote the registered version at every availability

  AddHelpScreen("HELP2"); 
}

void MN_DrawCredits(void)
{
  inhelpscreens = true;

  V_DrawBackground(gamemode==commercial ? "SLIME05" : "NUKAGE1", 0);

  V_DrawNamePatch(115,9,0, "PRBOOM",CR_GOLD, VPT_TRANS | VPT_STRETCH);

  V_WriteText("\n"
              FC_GRAY "Programmers:\n"
              FC_TAN "Colin Robert Phipps" FC_BRICK ", "
	      FC_TAN "Nicolas Kalkhof" FC_BRICK ",\n"
	      FC_TAN "Florian 'Proff' Schulze" FC_BRICK " and "
	      FC_TAN "Jess Haas\n"
              "\n"
              FC_GRAY "Thanks to:\n"
	      FC_TAN "id Software" FC_GRAY " - " FC_GREEN "DOOM\n"
	      FC_TAN "TeamTNT" FC_GRAY " - " FC_GREEN "BOOM\n"
	      FC_TAN "Lee Killough" FC_GRAY " - " FC_GREEN "MBF\n"
	      FC_TAN "Simon 'fraggle' Howard" FC_GRAY " - " FC_GREEN "SMMU\n"
	      FC_TAN "James 'Quasar' Haley" FC_BRICK" and\n"
	      FC_TAN "Steven 'SoM' McGranahan" FC_GRAY " - " FC_GREEN "Eternity\n"
	      FC_TAN "The DOSDoom-Team" FC_GRAY " - " FC_GREEN "DOSDOOM\n"
	      FC_TAN "Randy Heit" FC_GRAY " - " FC_GREEN "ZDOOM\n"
	      FC_TAN "Michael 'Kodak' Ryssen" FC_GRAY " - " FC_GREEN "DOOMGL\n"
	      FC_TAN "all contributors (see AUTHORS file)\n"
	      FC_GOLD "\nhttp://prboom.sourceforge.net",
              10, 25, 0);
}

void MN_HelpDrawer()
{
  if(helpscreens[viewing_helpscreen].Drawer)
    {
      helpscreens[viewing_helpscreen].Drawer();   // call drawer
    }
  else
    {
      // display lump
      V_DrawNumPatch(0, 0, 0, helpscreens[viewing_helpscreen].lumpnum, CR_DEFAULT, VPT_STRETCH);
    }
}

boolean MN_HelpResponder(event_t *ev)
{
  if(ev->type != ev_keydown) return false;

  if(ev->data1 == KEYD_BACKSPACE)
    {
      // go to previous screen
      viewing_helpscreen--;
      if(viewing_helpscreen < 0)
	{
	  viewing_helpscreen = 0;
	}
      S_StartSound(NULL, sfx_swtchx);
    }
  if(ev->data1 == KEYD_ENTER)
    {
      // go to next helpscreen
      viewing_helpscreen++;
      if(viewing_helpscreen >= num_helpscreens)
	{	  
          // cancel
	  ev->data1 = KEYD_ESCAPE;
	}
      else
	S_StartSound(NULL, sfx_pistol);
    }
  if(ev->data1 == KEYD_ESCAPE)
    {
      // cancel helpscreen
      redrawsbar = redrawborder = true; // need redraw
      current_menuwidget = NULL;
      menuactive = false;
      S_StartSound(NULL, sfx_swtchx);
    }

  // always eatkey
  return true;
}

menuwidget_t helpscreen_widget = {MN_HelpDrawer, MN_HelpResponder};

CONSOLE_COMMAND(help, 0)
{
  MN_ActivateMenu();
  MN_FindHelpScreens();        // search for help screens
  
  // hook in widget to display menu
  current_menuwidget = &helpscreen_widget;
  
  // start on first screen
  viewing_helpscreen = 0;
}

CONSOLE_COMMAND(credits, 0)
{
  MN_ActivateMenu();
  MN_FindCreditScreens();        // search for help screens
  
  // hook in widget to display menu
  current_menuwidget = &helpscreen_widget;
  
  // start on first screen
  viewing_helpscreen = 0;
}

///////////////////////////////////////////////////////////////////////////
//
// Automap Colour selection
//

// selection of automap colours for menu.

command_t *colour_command;
int selected_colour;

#define HIGHLIGHT_COLOUR 4

void MN_MapColourDrawer()
{
  int x, y;

  // draw the menu in the background

  MN_DrawMenu(current_menu);

  // draw colours table

  x = (320 - R_NamePatchWidth("M_COLORS")) / 2;
  y = (200 - R_NamePatchHeight("M_COLORS")) / 2;

  V_DrawNamePatch(x, y, 0, "M_COLORS", CR_DEFAULT, VPT_STRETCH);

  x += 4 + 8 * (selected_colour % 16);
  y += 4 + 8 * (selected_colour / 16);

  // draw block
  // proff 12/6/98: Drawing of colorchips completly changed for hi-res, it now uses a patch
  V_DrawNamePatch(x, y, 0, "M_PALSEL", CR_DEFAULT, VPT_STRETCH);

  if(!selected_colour)
    V_DrawNamePatch(x+1, y+1, 0, "M_PALNO", CR_DEFAULT, VPT_STRETCH);

}

boolean MN_MapColourResponder(event_t *ev)
{
  if(ev->type != ev_keydown) return false;

  if(ev->data1 == KEYD_LEFTARROW)
    selected_colour--;
  if(ev->data1 == KEYD_RIGHTARROW)
    selected_colour++;
  if(ev->data1 == KEYD_UPARROW)
    selected_colour -= 16;
  if(ev->data1 == KEYD_DOWNARROW)
    selected_colour += 16;

  if(ev->data1 == KEYD_ESCAPE)
    {
      // cancel colour selection
      current_menuwidget = NULL;
      return true;
    }

  if(ev->data1 == KEYD_ENTER)
    {
      static char tempstr[128];
      sprintf(tempstr, "%i", selected_colour);

      // run command
      cmdtype = c_menu;
      C_RunCommand(colour_command, tempstr);
      
      // kill selecter
      current_menuwidget = NULL;
      return true;
    }

  if(selected_colour < 0) selected_colour = 0;
  if(selected_colour > 255) selected_colour = 255;

  return true; // always eatkey
}

menuwidget_t colour_widget = {MN_MapColourDrawer, MN_MapColourResponder};

void MN_SelectColour(const char *variable_name)
{
  current_menuwidget = &colour_widget;
  colour_command = C_GetCmdForName(variable_name);
  selected_colour = *(int *)colour_command->variable->variable;
}


void MN_AddMiscCommands()
{
  C_AddCommand(credits);
  C_AddCommand(help);
}
