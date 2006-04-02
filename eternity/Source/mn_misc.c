// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Menu - misc functions
//
// Pop up alert/question messages
// Miscellaneous stuff
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "doomstat.h"
#include "d_main.h"
#include "m_qstr.h"
#include "s_sound.h"
#include "v_video.h"
#include "v_misc.h"
#include "w_wad.h"

#include "mn_engin.h"
#include "mn_misc.h"

#include "d_gi.h" // haleyjd: gamemode info

/////////////////////////////////////////////////////////////////////////
//
// Pop-up Messages
//

// haleyjd 02/24/02: bug fix -- MN_Alert and MN_Question replaced
// M_StartMessage in SMMU, but fraggle didn't put code in them to
// properly set the menuactive state based on whether or not menus 
// were active at the time of the call, leading to weird menu 
// behavior
static boolean popupMenuActive;

static char popup_message[128];
static char *popup_message_command; // console command to run

//
// haleyjd 07/27/05: not all questions should have to run console
// commands. It's inefficient.
//
static void (*popup_callback)(void) = NULL;

enum
{
   popup_alert,
   popup_question
} popup_message_type;

//
// WriteCentredText
//
// Local routine to draw centered messages. Candidate for
// absorption into future generalized font code. Rewritten
// 02/22/04 to use qstring module. Beware British spelling
// in function name ;)
//
static void WriteCentredText(char *message)
{
   static qstring_t qstring;
   static qstring_t *pqstr = NULL;
   char *rover;
   const char *buffer;
   int x, y;

   if(!pqstr)
      pqstr = M_QStrCreate(&qstring);
   
   // rather than reallocate memory every time we draw it,
   // use one buffer and increase the size as neccesary
   // haleyjd 02/22/04: qstring handles this for us now

   y = (SCREENHEIGHT - V_StringHeight(popup_message)) / 2;
   M_QStrClear(pqstr);
   rover = message;

   while(*rover)
   {
      if(*rover == '\n')
      {
         buffer = M_QStrBuffer(pqstr);
         x = (SCREENWIDTH - V_StringWidth(buffer)) / 2;
         V_WriteText(buffer, x, y);         
         M_QStrClear(pqstr); // clear buffer
         y += 7; // next line
      }
      else      // add next char
      {
         M_QStrPutc(pqstr, *rover);
      }
      rover++;
   }

   // dont forget the last line.. prob. not \n terminated
   buffer = M_QStrBuffer(pqstr);
   x = (SCREENWIDTH - V_StringWidth(buffer)) / 2;
   V_WriteText(buffer, x, y);   
}

void MN_PopupDrawer(void)
{
   WriteCentredText(popup_message);
}

boolean MN_PopupResponder(event_t *ev)
{
   int *menuSounds = gameModeInfo->menuSounds;
   
   if(ev->type != ev_keydown)
      return false;
   
   switch(popup_message_type)
   {
   case popup_alert:
      {
         // haleyjd 02/24/02: restore saved menuactive state
         menuactive = popupMenuActive;
         // kill message
         redrawsbar = redrawborder = true; // need redraw
         current_menuwidget = NULL;
         S_StartSound(NULL, menuSounds[MN_SND_DEACTIVATE]);
      }
      break;

   case popup_question:
      if(tolower(ev->data1) == 'y')     // yes!
      {
         // run command and kill message
         // haleyjd 02/24/02: restore saved menuactive state
         // menuactive = false; // kill menu
         menuactive = popupMenuActive;
         if(popup_callback)
         {
            popup_callback();
            popup_callback = NULL;
         }
         else
         {
            cmdtype = c_menu;
            C_RunTextCmd(popup_message_command);
         }
         S_StartSound(NULL, menuSounds[MN_SND_COMMAND]);
         redrawsbar = redrawborder = true; // need redraw
         current_menuwidget = NULL;  // kill message
      }
      if(tolower(ev->data1) == 'n' || ev->data1 == KEYD_ESCAPE
         || ev->data1 == KEYD_BACKSPACE)     // no!
      {
         // kill message
         // haleyjd 02/24/02: restore saved menuactive state
         // menuactive = false; // kill menu
         menuactive = popupMenuActive;
         S_StartSound(NULL, menuSounds[MN_SND_DEACTIVATE]);
         redrawsbar = redrawborder = true; // need redraw
         current_menuwidget = NULL; // kill message
      }
      break;
      
   default:
      break;
   }
   
   return true; // always eat key
}

// widget for popup message alternate draw
menuwidget_t popup_widget = {MN_PopupDrawer, MN_PopupResponder};

// alert message
// -- just press enter

void MN_Alert(char *message, ...)
{
   va_list args;
   
   // haleyjd 02/24/02: bug fix for menuactive state
   popupMenuActive = menuactive;
   
   MN_ActivateMenu();
   
   // hook in widget so message will be displayed
   current_menuwidget = &popup_widget;
   popup_message_type = popup_alert;
   
   va_start(args, message);
   pvsnprintf(popup_message, sizeof(popup_message), message, args);
   va_end(args);
}

// question message
// console command will be run if user responds with 'y'

void MN_Question(char *message, char *command)
{
   // haleyjd 02/24/02: bug fix for menuactive state
   popupMenuActive = menuactive;
   
   MN_ActivateMenu();
   
   // hook in widget so message will be displayed
   current_menuwidget = &popup_widget;
   
   strncpy(popup_message, message, 128);
   popup_message_type = popup_question;
   popup_message_command = command;
}

void MN_QuestionFunc(char *message, void (*handler)(void))
{
   popupMenuActive = menuactive;
   
   MN_ActivateMenu();
   
   // hook in widget so message will be displayed
   current_menuwidget = &popup_widget;
   
   strncpy(popup_message, message, 128);
   popup_message_type = popup_question;
   popup_callback = handler;
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
   
   if((lumpnum = W_CheckNumForName(screenname)) != -1)
   {
      helpscreens[num_helpscreens].Drawer = NULL;   // no drawer
      helpscreens[num_helpscreens++].lumpnum = lumpnum;
   }  
}

// build help screens differently according to whether displaying
// help or displaying credits

static void MN_FindCreditScreens(void)
{
   num_helpscreens = 0;  // reset
   
   // add dynamic smmu credits screen
   
   helpscreens[num_helpscreens++].Drawer = MN_DrawCredits;
   
   // other help screens

   // haleyjd: do / do not want certain screens for different
   // game modes, even though its not necessary to weed them out
   // if they're not in the wad

   if(gameModeInfo->type == Game_Heretic)
      AddHelpScreen("ORDER");
   else
      AddHelpScreen("HELP2");
   
   AddHelpScreen("CREDIT");
}

static void MN_FindHelpScreens(void)
{
   int custom;
   
   num_helpscreens = 0;
   
   // add custom menus first
   
   for(custom = 0; custom < 100; custom++)
   {
      char tempstr[10];

      sprintf(tempstr, "HELP%.02i", custom);
      AddHelpScreen(tempstr);
   }
   
   // now the default original doom ones
   
   // sf: keep original help screens until key bindings rewritten
   // and i can restore the dynamic help screens

   if(gameModeInfo->type == Game_Heretic)
      AddHelpScreen("ORDER");
   else
      AddHelpScreen("HELP");
   
   AddHelpScreen("HELP1");
   
   // promote the registered version at every availability
   // haleyjd: HELP2 is a help screen in heretic too
   
   AddHelpScreen("HELP2"); 
}

void MN_DrawCredits(void)
{
  inhelpscreens = true;

  // sf: altered for SMMU
  // haleyjd: altered for Eternity :)

  V_DrawDistortedBackground(gameModeInfo->creditBackground, 
                            &vbscreen);

  // sf: SMMU credits

  V_WriteText(FC_HI "The Eternity Engine\n"
              "\n"
              FC_NORMAL "Enhancements by James 'Quasar' Haley\n"
              "         and Stephen McGranahan\n"
              "\n"
              FC_HI "SMMU:" FC_NORMAL " \"Smack my marine up\"\n"
              "\n"
              "Port by Simon Howard 'Fraggle'\n"
              "\n"
              "Based on the MBF port by Lee Killough\n"
              "\n"
              FC_HI "Programming:" FC_NORMAL " J. Haley, S. McGranahan\n"
              FC_HI "Graphics:" FC_NORMAL " Bob Satori\n"
              FC_HI "Start map:" FC_NORMAL " Derek MacDonald\n"
              FC_HI "Special thanks:" FC_NORMAL " Julian Aubourg,\n"
              "         Joel Murdoch, Anders Astrand\n"
              "\n"
              "Copyright(C) 2004 J. Haley, S. McGranahan,\n"
              "         S. Howard, et al.\n"              
              FC_HI"         http://doomworld.com/eternity/",
              10, gameModeInfo->creditY);
}

void MN_HelpDrawer(void)
{
   if(helpscreens[viewing_helpscreen].Drawer)
   {
      helpscreens[viewing_helpscreen].Drawer();   // call drawer
   }
   else if(gameModeInfo->flags & GIF_PAGERAW)
   {
      // haleyjd: heretic support
      byte *raw;

      raw = W_CacheLumpNum(helpscreens[viewing_helpscreen].lumpnum,
                           PU_CACHE);
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,raw);
   }
   else
   {
      patch_t *patch;
      
      // load lump
      patch = W_CacheLumpNum(helpscreens[viewing_helpscreen].lumpnum,
                             PU_CACHE);
      // display lump
      V_DrawPatch(0, 0, &vbscreen, patch);
   }
}

boolean MN_HelpResponder(event_t *ev)
{
   int *menuSounds = gameModeInfo->menuSounds;
   
   if(ev->type != ev_keydown) return false;
   
   if(ev->data1 == KEYD_BACKSPACE)
   {
      // go to previous screen
      // haleyjd: only make sound if we really went back
      viewing_helpscreen--;
      if(viewing_helpscreen < 0)
      {
         viewing_helpscreen = 0;
      }
      else
         S_StartSound(NULL, menuSounds[MN_SND_PREVIOUS]);
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
         S_StartSound(NULL, menuSounds[MN_SND_COMMAND]);
   }
   if(ev->data1 == KEYD_ESCAPE)
   {
      // cancel helpscreen
      redrawsbar = redrawborder = true; // need redraw
      current_menuwidget = NULL;
      menuactive = false;
      S_StartSound(NULL, menuSounds[MN_SND_DEACTIVATE]);
   }

   // always eatkey
   return true;
}

menuwidget_t helpscreen_widget = {MN_HelpDrawer, MN_HelpResponder, true};

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

void MN_MapColourDrawer(void)
{
   patch_t *patch;
   int x, y;
   int u, v;
   char block[BLOCK_SIZE*BLOCK_SIZE];

   // draw the menu in the background
   
   MN_DrawMenu(current_menu);

   // draw colours table
   
   patch = W_CacheLumpName("M_COLORS", PU_CACHE);
   
   x = (SCREENWIDTH - SHORT(patch->width)) / 2;
   y = (SCREENHEIGHT - SHORT(patch->height)) / 2;
   
   V_DrawPatch(x, y, &vbscreen, patch);
   
   x += 4 + 8 * (selected_colour % 16);
   y += 4 + 8 * (selected_colour / 16);

   // build block
   
   // border
   memset(block, HIGHLIGHT_COLOUR, BLOCK_SIZE*BLOCK_SIZE);
  
   // draw colour inside
   for(u=1; u<BLOCK_SIZE-1; u++)
      for(v=1; v<BLOCK_SIZE-1; v++)
         block[v*BLOCK_SIZE + u] = selected_colour;
  
   // draw block
   V_DrawBlock(x, y, &vbscreen, BLOCK_SIZE, BLOCK_SIZE, block);

   if(!selected_colour)
      V_DrawPatch(x+1, y+1, &vbscreen, W_CacheLumpName("M_PALNO", PU_CACHE));
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

menuwidget_t colour_widget = {MN_MapColourDrawer, MN_MapColourResponder, true};

void MN_SelectColour(char *variable_name)
{
   current_menuwidget = &colour_widget;
   colour_command = C_GetCmdForName(variable_name);
   selected_colour = *(int *)colour_command->variable->variable;
}


void MN_AddMiscCommands(void)
{
   C_AddCommand(credits);
   C_AddCommand(help);
}

// EOF
