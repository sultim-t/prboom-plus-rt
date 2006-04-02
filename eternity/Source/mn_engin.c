// Emacs style mode select -*- C++ -*-
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
// New menu
//
// The menu engine. All the menus themselves are in mn_menus.c
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "d_deh.h"
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"
#include "g_bind.h"    // haleyjd: dynamic key bindings
#include "d_gi.h"      // haleyjd: global game mode info
#include "v_font.h"    // haleyjd: new font engine

boolean inhelpscreens; // indicates we are in or just left a help screen

// menu error message
char menu_error_message[128];
int menu_error_time = 0;

        // input for typing in new value
static command_t *input_command = NULL;       // NULL if not typing in

// haleyjd 04/29/02: needs to be unsigned
static unsigned char input_buffer[128] = "";

/////////////////////////////////////////////////////////////////////////////
// 
// MENU DRAWING
//
/////////////////////////////////////////////////////////////////////////////

        // gap from variable description to value
#define GAP 20
// haleyjd: changed to use gameModeInfo
#define background_flat (gameModeInfo->menuBackground)
#define SKULL_HEIGHT 19
#define BLINK_TIME 8

// colours
#define unselect_colour    CR_RED
#define select_colour      CR_GRAY
#define var_colour         CR_GREEN

enum
{
   slider_left,
   slider_right,
   slider_mid,
   slider_slider,
   num_slider_gfx
};
patch_t *slider_gfx[num_slider_gfx];
static menu_t *drawing_menu;
static patch_t *skulls[2];

static void MN_GetItemVariable(menuitem_t *item)
{
   // get variable if neccesary
   if(!item->var)
   {
      command_t *cmd;
      // use data for variable name
      if(!(cmd = C_GetCmdForName(item->data)))
      {
         C_Printf(FC_ERROR"variable not found: %s\n", item->data);
         item->type = it_info;   // turn into normal unselectable text
         item->var = NULL;
         return;
      }
      item->var = cmd->variable;
   }
}

        // width of slider, in mid-patches
#define SLIDE_PATCHES 9

// draw a 'slider' (for sound volume, etc)

static void MN_DrawSlider(int x, int y, int pct)
{
   int i;
   int draw_x = x;
   int slider_width = 0;       // find slider width in pixels
  
   V_DrawPatch(draw_x, y, &vbscreen, slider_gfx[slider_left]);
   draw_x += slider_gfx[slider_left]->width;
  
   for(i=0; i<SLIDE_PATCHES; i++)
   {
      V_DrawPatch(draw_x, y, &vbscreen, slider_gfx[slider_mid]);
      draw_x += slider_gfx[slider_mid]->width - 1;
   }
   
   V_DrawPatch(draw_x, y, &vbscreen, slider_gfx[slider_right]);
  
   // find position to draw
   
   slider_width = (slider_gfx[slider_mid]->width - 1) * SLIDE_PATCHES;
   draw_x = slider_gfx[slider_left]->width +
      (pct * (slider_width - slider_gfx[slider_slider]->width)) / 100;
   
   V_DrawPatch(x + draw_x, y, &vbscreen, slider_gfx[slider_slider]);
}

// draw a menu item. returns the height in pixels

static int MN_DrawMenuItem(menuitem_t *item, int x, int y, int colour)
{
   boolean leftaligned =
      (drawing_menu->flags & mf_skullmenu) ||
      (drawing_menu->flags & mf_leftaligned);

   // skip drawing if a gap
   if(item->type == it_gap)
      return 8;

   item->x = x; item->y = y;       // save x,y to item
   item->posinit = true;           // haleyjd 04/14/03
 
   // draw an alternate patch?
 
   if(item->patch)
   {
      patch_t *patch;
      int lumpnum;
      
      lumpnum = W_CheckNumForName(item->patch);
      
      // default to text-based message if patch missing
      if(lumpnum >= 0)
      {
         int height;
         
         patch = W_CacheLumpNum(lumpnum, PU_CACHE);
         height = patch->height;
         
         // check for left-aligned
         if(!leftaligned) x-= patch->width;
         
         // adjust x if a centered title
         if(item->type == it_title)
            x = (SCREENWIDTH-patch->width)/2;
         
         V_DrawPatchTranslated(x, y, &vbscreen, patch, colrngs[colour], 0);

         // haleyjd 05/16/04: hack for traditional menu support
         if(gamemode <= retail && traditional_menu &&
            drawing_menu == &menu_old_main)
         {
            height = 15; // this was hard-coded in the old system
         }

         return height + 1;   // 1 pixel gap
      }
   }

   // draw description text
   
   if(item->type == it_title)
   {
      // if it_title, we draw the description centered
      
      // FIXME: gross hack!
      if(gameModeInfo->type == Game_Heretic)
      {
         V_WriteTextBig
            (
            item->description, 
            (SCREENWIDTH-V_StringWidthBig(item->description))/2,
            y
            );
         return 20;
      }
      else
      {
         MN_WriteTextColoured
            (
            item->description,
            colour,
            (SCREENWIDTH-MN_StringWidth(item->description))/2,
            y
            );
      }
   }
   else
   {
      // FIXME: gross hack!
      if((item->type == it_hruncmd || item->type == it_hinfo) && 
         gameModeInfo->type == Game_Heretic) // only if init'd
      {
         V_WriteTextBig(item->description, x, y);
         return 20;
      }
      else
      {
         // write description
         MN_WriteTextColoured
            (
            item->description,
            colour,
            x - (leftaligned ? 0 : MN_StringWidth(item->description)),
            y
            );
      }
   }

   // draw other data: variable data etc.
   
   switch(item->type)      
   {
   case it_title:              // just description drawn
   case it_info:
   case it_runcmd:
   case it_hruncmd:
   case it_hinfo:
   case it_gap:                // just a gap, draw nothing
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
         MN_WriteTextColoured
            (
            boundkeys,
            colour,
            x + (leftaligned ? MN_StringWidth(item->description): 0),
            y
            );
         
         break;
      }

      // it_toggle and it_variable are drawn the same
      
   case it_toggle:
   case it_variable:
      {
         unsigned char varvalue[1024];             // temp buffer
         
         MN_GetItemVariable(item);
         
         // create variable description:
         // Use console variable descriptions.
         
         // display input buffer if inputting new var value
         if(input_command && item->var == input_command->variable)
            psnprintf(varvalue, sizeof(varvalue), "%s_", input_buffer);
         else
            strncpy(varvalue, C_VariableStringValue(item->var), 1024);
         
         if(drawing_menu->flags & mf_background)
         {
            // include gap on fullscreen menus
            x += GAP;
            // adjust colour for different coloured variables
            if(colour == unselect_colour) colour = var_colour;
         }

         // draw it
         MN_WriteTextColoured
            (
            varvalue,
            colour,
            x + (leftaligned ? MN_StringWidth(item->description) : 0),
            y
            );
         break;
      }

      // slider

   case it_slider:
      { 
         MN_GetItemVariable(item);
         
         // draw slider
         // only ints
         
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
         int bx, by;
         int colour;
         char block[BLOCK_SIZE*BLOCK_SIZE];
         
         MN_GetItemVariable(item);
         
         if(!item->var || item->var->type != vt_int) break;
         
         // find colour of this variable from console variable
         colour = *(int *)item->var->variable;
         
         // create block
         // border
         memset(block, 0, BLOCK_SIZE*BLOCK_SIZE);
         
         if(colour)
         {
            // middle
            for(bx=1; bx<BLOCK_SIZE-1; bx++)
               for(by=1; by<BLOCK_SIZE-1; by++)
                  block[by*BLOCK_SIZE+bx] = colour;
         }
         // draw it
         
         V_DrawBlock(x+GAP, y-1, &vbscreen, BLOCK_SIZE, BLOCK_SIZE, block);
         
         if(!colour)
         {
            // draw patch w/cross
            V_DrawPatch(x+GAP+1, y, &vbscreen, W_CacheLumpName("M_PALNO", PU_CACHE));
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

   if(!menu) // haleyjd 04/20/03
      return;

   drawing_menu = menu;    // needed by DrawMenuItem
   y = menu->y;
   
   // draw background

   if(menu->flags & mf_background)
      V_DrawBackground(background_flat, &vbscreen);
  
   // menu-specific drawer function

   if(menu->drawer) menu->drawer();

   // draw items in menu

   for(itemnum = 0; menu->menuitems[itemnum].type != it_end; itemnum++)
   {
      int item_height;
      int item_colour;

      // choose item colour based on selected item

      item_colour = menu->selected == itemnum &&
         !(menu->flags & mf_skullmenu) ? select_colour : unselect_colour;
      
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
      {
         // haleyjd 05/16/04: hack for traditional menu support
         int item_x = menu->x - 30;                         // 30 left
         int item_y = y + (item_height - SKULL_HEIGHT) / 2; // midpoint

         if(gamemode <= retail && traditional_menu &&
            menu == &menu_old_main)
         {
            item_x = menu->x - 32;
            item_y = menu->y - 5 + itemnum * 16;
         }

         V_DrawPatch(item_x, item_y, &vbscreen,
                     skulls[(menutime / BLINK_TIME) % 2]);
      }
      
      y += item_height;            // go down by item height
   }

   if(menu->flags & mf_skullmenu)
      return; // no help msg in skull menu

   // choose help message to print
   
   if(menu_error_time)             // error message takes priority
   {
      // make it flash

      // haleyjd: fix y coordinate to use appropriate text metrics
      int m_y = SCREENHEIGHT - gameModeInfo->vtextinfo->absh;

      if((menu_error_time / 8) % 2)
         MN_WriteTextColoured(menu_error_message, CR_TAN, 10, m_y);
   }
   else
   {
      // haleyjd: fix y coordinate to use appropriate text metrics
      char msgbuffer[64];
      char *helpmsg = "";
      int m_y = SCREENHEIGHT - gameModeInfo->vtextinfo->absh;

      // write some help about the item
      menuitem_t *menuitem = &menu->menuitems[menu->selected];
      
      if(menuitem->type == it_variable)       // variable
      {
         if(input_command)
            helpmsg = "press escape to cancel";
         else
         {
            char *key = G_FirstBoundKey("menu_confirm");
            psnprintf(msgbuffer, 64, "press %s to change", key);
            helpmsg = msgbuffer;
         }
      }
      
      if(menuitem->type == it_toggle)         // togglable variable
      {
         // enter to change boolean variables
         // left/right otherwise
         
         if(menuitem->var->type == vt_int &&
            menuitem->var->max - menuitem->var->min == 1)
         {
            char *key = G_FirstBoundKey("menu_confirm");
            psnprintf(msgbuffer, 64, "press %s to change", key);
            helpmsg = msgbuffer;
         }
         else
            helpmsg = "use left/right to change value";
      }
      MN_WriteTextColoured(helpmsg, CR_GOLD, 10, m_y);
   }
}

//
// MN_CheckFullScreen
//
// Called by D_Drawer to see if the menu is in full-screen mode --
// this allows the game to skip all other drawing, keeping the
// framerate at 35 fps.
//
boolean MN_CheckFullScreen(void)
{
   if(!menuactive || !current_menu)
      return false;

   if(!(current_menu->flags & mf_background) || hide_menu ||
      (current_menuwidget && !current_menuwidget->fullscreen))
      return false;

   return true;
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

int quickSaveSlot;  // haleyjd 02/23/02: restored from MBF

        // init menu
void MN_Init(void)
{
   char *cursorPatch1 = gameModeInfo->menuCursor->patch1;
   char *cursorPatch2 = gameModeInfo->menuCursor->patch2;

   skulls[0] = W_CacheLumpName(cursorPatch1, PU_STATIC);
   skulls[1] = W_CacheLumpName(cursorPatch2, PU_STATIC);
   
   // load slider gfx
   
   slider_gfx[slider_left]   = W_CacheLumpName("M_SLIDEL", PU_STATIC);
   slider_gfx[slider_right]  = W_CacheLumpName("M_SLIDER", PU_STATIC);
   slider_gfx[slider_mid]    = W_CacheLumpName("M_SLIDEM", PU_STATIC);
   slider_gfx[slider_slider] = W_CacheLumpName("M_SLIDEO", PU_STATIC);
   
   quickSaveSlot = -1; // haleyjd: -1 == no slot selected yet

   // haleyjd: init heretic stuff if appropriate
   if(gameModeInfo->type == Game_Heretic)
      MN_HInitSkull(); // initialize spinning skulls
   
   MN_InitMenus();   // create menu commands in mn_menus.c
}

//////////////////////////////////
// ticker

void MN_Ticker(void)
{
   if(menu_error_time)
      menu_error_time--;
   if(hide_menu)                   // count down hide_menu
      hide_menu--;
   menutime++;
}

////////////////////////////////
// drawer

void MN_Drawer(void)
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

#define is_a_gap(it) ((it)->type == it_gap || (it)->type == it_info ||  \
                      (it)->type == it_title || (it)->type == it_hinfo)

extern menu_t menu_sound;
extern menu_t menu_savegame;

//
// MN_Responder
//
// haleyjd 07/03/04: rewritten to use enhanced key binding system
//
boolean MN_Responder(event_t *ev)
{
   // haleyjd 04/29/02: these need to be unsigned
   unsigned char tempstr[128];
   unsigned char ch;
   int *menuSounds = gameModeInfo->menuSounds; // haleyjd

   // haleyjd 07/03/04: call G_KeyResponder with kac_menu to filter
   // for menu-class actions
   G_KeyResponder(ev, kac_menu);

   // we only care about key presses

   if(ev->type != ev_keydown)
      return false;

   // are we displaying a widget?

   if(current_menuwidget)
   {
      return
         current_menuwidget->responder ?
         current_menuwidget->responder(ev) : false;
   }

   // are we inputting a new value into a variable?
   
   if(input_command)
   {
      unsigned char ch = ev->data1;
      variable_t *var = input_command->variable;
      
      if(ev->data1 == KEYD_ESCAPE)        // cancel input
         input_command = NULL;
      
      if(ev->data1 == KEYD_ENTER && input_buffer[0])
      {
         unsigned char *temp;
         
         // place " marks round the new value
         temp = strdup(input_buffer);
         psnprintf(input_buffer, sizeof(input_buffer), "\"%s\"", temp);
         free(temp);
         
         // set the command
         cmdtype = c_menu;
         C_RunCommand(input_command, input_buffer);
         input_command = NULL;
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

      if((ch > 31 && ch < 127) && 
         strlen(input_buffer) <=
         ((var->type == vt_string) ? var->max :
          (var->type == vt_int) ? 10 : 20))
      {
         input_buffer[strlen(input_buffer) + 1] = 0;
         input_buffer[strlen(input_buffer)] = ch;
      }
      
      return true;
   } 

   if(devparm && ev->data1 == key_help)
   {
      G_ScreenShot();
      return true;
   }
  
   if(action_menu_toggle)
   {
      // toggle menu
      action_menu_toggle = false;
      
      // start up main menu or kill menu
      if(menuactive)
      {
         MN_ClearMenus();
         S_StartSound(NULL, menuSounds[MN_SND_DEACTIVATE]);
      }
      else 
         MN_StartControlPanel();
      
      // haleyjd: was making unnecessary sounds
      // S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);
      
      return true;
   }

   if(action_menu_help)
   {
      action_menu_help = false;
      C_RunTextCmd("help");
      return true;
   }

   if(action_menu_setup)
   {
      action_menu_setup = false;
      C_RunTextCmd("mn_options");
      return true;
   }
   
   // not interested in keys if not in menu
   if(!menuactive)
      return false;

   if(action_menu_up)
   {
      action_menu_up = false;

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
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL, menuSounds[MN_SND_KEYUPDOWN]); // make sound

      return true;  // eatkey
   }
  
   if(action_menu_down)
   {
      action_menu_down = false;

      do
      {
         ++current_menu->selected;
         if(current_menu->menuitems[current_menu->selected].type == it_end)
         {
            current_menu->selected = 0;     // jump back to start
         }
      }
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL, menuSounds[MN_SND_KEYUPDOWN]); // make sound

      return true;  // eatkey
   }
   
   if(action_menu_confirm)
   {
      menuitem_t *menuitem = &current_menu->menuitems[current_menu->selected];

      action_menu_confirm = false;
      
      switch(menuitem->type)
      {
      case it_runcmd:
      case it_hruncmd:
         {
            S_StartSound(NULL,menuSounds[MN_SND_COMMAND]); // make sound
            cmdtype = c_menu;
            C_RunTextCmd(menuitem->data);
            break;
         }
         
      case it_toggle:
         {
            // boolean values only toggled on enter
            if(menuitem->var->type != vt_int ||
               menuitem->var->max-menuitem->var->min > 1) break;
            
            // toggle value now
            psnprintf(tempstr, sizeof(tempstr), "%s /", menuitem->data);
            cmdtype = c_menu;
            C_RunTextCmd(tempstr);
            
            S_StartSound(NULL,menuSounds[MN_SND_COMMAND]); // make sound
            break;
         }
         
      case it_variable:
         {
            menuitem_t *menuitem =
               &current_menu->menuitems[current_menu->selected];
            
            // get input for new value
            input_command = C_GetCmdForName(menuitem->data);

            // haleyjd 07/23/04: restore starting input_buffer with
            // the current value of string variables
            if(input_command->variable->type == vt_string)
            {
               char *str = *(char **)(input_command->variable->variable);

               if(current_menu != &menu_savegame || strcmp(str, s_EMPTYSTRING))
                  strcpy(input_buffer, str);
               else
                  input_buffer[0] = 0;
            }
            else
               input_buffer[0] = 0;             // clear input buffer
            break;
         }

      case it_automap:
         {
            menuitem_t *menuitem =
               &current_menu->menuitems[current_menu->selected];
            
            MN_SelectColour(menuitem->data);
            
            return true;
         }

      case it_binding:
         {
            G_EditBinding(menuitem->data);
            
            return true;
         }
         
      default:
         break;
      }

      return true;
   }
  
   if(action_menu_previous)
   {
      action_menu_previous = false;
      MN_PrevMenu();
      return true;
   }
   
   // decrease value of variable
   if(action_menu_left)
   {
      menuitem_t *menuitem =
         &current_menu->menuitems[current_menu->selected];

      action_menu_left = false;
      
      switch(menuitem->type)
      {
      case it_slider:
      case it_toggle:
         {
            // no on-off int values
            if(menuitem->var->type == vt_int &&
               menuitem->var->max-menuitem->var->min == 1) break;
            
            // change variable
            psnprintf(tempstr, sizeof(tempstr), "%s -", menuitem->data);
            cmdtype = c_menu;
            C_RunTextCmd(tempstr);
            S_StartSound(NULL, menuSounds[MN_SND_KEYLEFTRIGHT]);
         }
      default:
         {
            break;
         }
      }

      return true;
   }
  
   // increase value of variable
   if(action_menu_right)
   {
      menuitem_t *menuitem =
         &current_menu->menuitems[current_menu->selected];

      action_menu_right = false;
      
      switch(menuitem->type)
      {
      case it_slider:
      case it_toggle:
         {
            // no on-off int values
            if(menuitem->var->type == vt_int &&
               menuitem->var->max-menuitem->var->min == 1) break;
            
            // change variable
            psnprintf(tempstr, sizeof(tempstr), "%s +", menuitem->data);
            cmdtype = c_menu;
            C_RunTextCmd(tempstr);
            S_StartSound(NULL, menuSounds[MN_SND_KEYLEFTRIGHT]);
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
   if(ch >= 'a' && ch <= 'z')
   {  
      // sf: experimented with various algorithms for this
      //     this one seems to work as it should

      int n = current_menu->selected;
      
      do
      {
         n++;
         if(current_menu->menuitems[n].type == it_end) 
            n = 0; // loop round

         // ignore unselectables
         if(!is_a_gap(&current_menu->menuitems[n])) 
         {
            if(tolower(current_menu->menuitems[n].description[0]) == ch)
            {
               // found a matching item!
               current_menu->selected = n;
               return true; // eat key
            }
         }
      } while(n != current_menu->selected);
   }
   
   return false;
}

///////////////////////////////////////////////////////////////////////////
//
// Other Menu Functions
//

// make menu 'clunk' sound on opening

void MN_ActivateMenu(void)
{
   if(!menuactive)  // activate menu if not already
   {
      menuactive = true;
      S_StartSound(NULL, gameModeInfo->menuSounds[MN_SND_ACTIVATE]);
   }
}

// start a menu:

void MN_StartMenu(menu_t *menu)
{
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
   
   menu_error_time = 0;      // clear error message
   redrawsbar = redrawborder = true;  // need redraw
}

// go back to a previous menu

void MN_PrevMenu(void)
{
   if(--menu_history_num < 0)
      MN_ClearMenus();
   else
      current_menu = menu_history[menu_history_num];
   
   menu_error_time = 0;          // clear errors
   redrawsbar = redrawborder = true;  // need redraw
   S_StartSound(NULL, gameModeInfo->menuSounds[MN_SND_PREVIOUS]);
}

// turn off menus
void MN_ClearMenus(void)
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

CONSOLE_COMMAND(forceload, cf_hidden)
{
   G_ForcedLoadGame();
   MN_ClearMenus();
}

void MN_ForcedLoadGame(char *msg)
{
   MN_Question(msg, "forceload");
}

// display error msg in popup display at bottom of screen

void MN_ErrorMsg(const char *s, ...)
{
   va_list args;
   
   va_start(args, s);
   pvsnprintf(menu_error_message, sizeof(menu_error_message), s, args);
   va_end(args);
   
   menu_error_time = 140;
}

// activate main menu

void MN_StartControlPanel(void)
{
   // haleyjd 05/16/04: traditional DOOM main menu support
   if(gamemode <= retail && traditional_menu)
   {
      MN_StartMenu(&menu_old_main);
   }
   else
      MN_StartMenu(gameModeInfo->mainMenu);
}

///////////////////////////////////////////////////////////////////////////
//
// Menu Font Drawing
//
// copy of V_* functions
// these do not leave a 1 pixel-gap between chars, I think it looks
// better for the menu
//
// haleyjd: duplicate code eliminated via use of vfont engine.
// A copy of the small_font object is made here and given a different
// dw value to affect the change in drawing.
//

extern vfont_t small_font;
static vfont_t menu_font;
static boolean menu_font_init = false;

static void MN_InitFont(void)
{
   // copy over all properties of the normal small font
   menu_font = small_font;

   // set width delta to 1 to move characters together
   menu_font.dw = 1;

   menu_font_init = true;
}

void MN_WriteText(unsigned char *s, int x, int y)
{
   if(!menu_font_init)
      MN_InitFont();

   V_FontWriteText(&menu_font, s, x, y);
}

void MN_WriteTextColoured(unsigned char *s, int colour, int x, int y)
{
   if(!menu_font_init)
      MN_InitFont();

   V_FontWriteTextColored(&menu_font, s, colour, x, y);
}

int MN_StringWidth(unsigned char *s)
{
   if(!menu_font_init)
      MN_InitFont();

   return V_FontStringWidth(&menu_font, s);
}

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

extern void MN_AddMenus(void);              // mn_menus.c
extern void MN_AddMiscCommands(void);       // mn_misc.c

void MN_AddCommands(void)
{
   C_AddCommand(mn_clearmenus);
   C_AddCommand(mn_prevmenu);
   C_AddCommand(forceload);
   
   MN_AddMenus();               // add commands to call the menus
   MN_AddMiscCommands();
}

// EOF
