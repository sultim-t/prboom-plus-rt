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
// Key Bindings
//
// Rather than use the old system of binding a key to an action, we bind
// an action to a key. This way we can have as many actions as we want
// bound to a particular key.
//
// By Simon Howard, Revised by James Haley
//
//----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "z_zone.h"

#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "d_deh.h"
#include "g_game.h"
#include "m_argv.h"
#include "mn_engin.h"
#include "mn_misc.h"
#include "m_misc.h"
#include "w_wad.h"
#include "d_io.h" // SoM 3/14/2002: strncasecmp
#include "g_bind.h"

// Action variables
// These variables are asserted as positive values when the action
// they represent has been performed by the player via key pressing.

// Game Actions -- These are handled in g_game.c

int action_forward;     // forward movement
int action_backward;    // backward movement
int action_left;        // left movement
int action_right;       // right movement
int action_moveleft;    // key-strafe left
int action_moveright;   // key-strafe right
int action_use;         // object activation
int action_speed;       // running
int action_attack;      // firing current weapon
int action_strafe;      // strafe in any direction
int action_flip;        // 180 degree turn

int action_mlook;       // mlook activation
int action_lookup;      // key-look up
int action_lookdown;    // key-look down
int action_center;      // key-look centerview

int action_weapon1;     // select weapon 1
int action_weapon2;     // select weapon 2
int action_weapon3;     // select weapon 3
int action_weapon4;     // select weapon 4
int action_weapon5;     // select weapon 5
int action_weapon6;     // select weapon 6
int action_weapon7;     // select weapon 7
int action_weapon8;     // select weapon 8
int action_weapon9;     // select weapon 9

int action_nextweapon;  // toggle to next-favored weapon

int action_frags;       // show frags

// Menu Actions -- handled by MN_Responder

int action_menu_help;
int action_menu_toggle;
int action_menu_setup;
int action_menu_up;
int action_menu_down;
int action_menu_confirm;
int action_menu_previous;
int action_menu_left;
int action_menu_right;

// AutoMap Actions -- handled by AM_Responder

int action_map_toggle;
int action_map_gobig;
int action_map_follow;
int action_map_mark;
int action_map_clear;
int action_map_grid;

//
// Handler Functions
//

// haleyjd: removed autorun

//
// Actions List
//

typedef struct keyaction_s
{
   char *name;        // text description

   // haleyjd 07/03/04: key binding classes
   int bclass;
   
   enum keyactiontype
   {
      at_variable,
      at_function,
      at_conscmd,     // console command
   } type;
   
   union keyactiondata
   {
      int *variable;  // variable -- if non-zero, action activated (key down)
      binding_handler Handler;
   } value;
   
   struct keyaction_s *next; // haleyjd: used for console bindings

} keyaction_t;

keyaction_t keyactions[] =
{
   // Game Actions

   {"forward",   kac_game,       at_variable,     {&action_forward}},
   {"backward",  kac_game,       at_variable,     {&action_backward}},
   {"left",      kac_game,       at_variable,     {&action_left}},
   {"right",     kac_game,       at_variable,     {&action_right}},
   {"moveleft",  kac_game,       at_variable,     {&action_moveleft}},
   {"moveright", kac_game,       at_variable,     {&action_moveright}},
   {"use",       kac_game,       at_variable,     {&action_use}},
   {"strafe",    kac_game,       at_variable,     {&action_strafe}},
   {"attack",    kac_game,       at_variable,     {&action_attack}},
   {"flip",      kac_game,       at_variable,     {&action_flip}},
   {"speed",     kac_game,       at_variable,     {&action_speed}},

   {"mlook",     kac_game,       at_variable,     {&action_mlook}},
   {"lookup",    kac_game,       at_variable,     {&action_lookup}},
   {"lookdown",  kac_game,       at_variable,     {&action_lookdown}},
   {"center",    kac_game,       at_variable,     {&action_center}},

   {"weapon1",   kac_game,       at_variable,     {&action_weapon1}},
   {"weapon2",   kac_game,       at_variable,     {&action_weapon2}},
   {"weapon3",   kac_game,       at_variable,     {&action_weapon3}},
   {"weapon4",   kac_game,       at_variable,     {&action_weapon4}},
   {"weapon5",   kac_game,       at_variable,     {&action_weapon5}},
   {"weapon6",   kac_game,       at_variable,     {&action_weapon6}},
   {"weapon7",   kac_game,       at_variable,     {&action_weapon7}},
   {"weapon8",   kac_game,       at_variable,     {&action_weapon8}},
   {"weapon9",   kac_game,       at_variable,     {&action_weapon9}},
   {"nextweapon",kac_game,       at_variable,     {&action_nextweapon}},

   {"frags",     kac_game,       at_variable,     {&action_frags}},

   // Menu Actions

   {"menu_toggle",   kac_menu,   at_variable,     {&action_menu_toggle}},
   {"menu_help",     kac_menu,   at_variable,     {&action_menu_help}},
   {"menu_setup",    kac_menu,   at_variable,     {&action_menu_setup}},
   {"menu_up",       kac_menu,   at_variable,     {&action_menu_up}},
   {"menu_down",     kac_menu,   at_variable,     {&action_menu_down}},
   {"menu_confirm",  kac_menu,   at_variable,     {&action_menu_confirm}},
   {"menu_previous", kac_menu,   at_variable,     {&action_menu_previous}},
   {"menu_left",     kac_menu,   at_variable,     {&action_menu_left}},
   {"menu_right",    kac_menu,   at_variable,     {&action_menu_right}},

   // Automap Actions

   // these have their functions set below
   {"map_right",   kac_map,      at_function,     {NULL}},
   {"map_left",    kac_map,      at_function,     {NULL}},
   {"map_up",      kac_map,      at_function,     {NULL}},
   {"map_down",    kac_map,      at_function,     {NULL}},
   {"map_zoomin",  kac_map,      at_function,     {NULL}},
   {"map_zoomout", kac_map,      at_function,     {NULL}},

   {"map_toggle",  kac_map,      at_variable,     {&action_map_toggle}},
   {"map_gobig",   kac_map,      at_variable,     {&action_map_gobig}},
   {"map_follow",  kac_map,      at_variable,     {&action_map_follow}},
   {"map_mark",    kac_map,      at_variable,     {&action_map_mark}},
   {"map_clear",   kac_map,      at_variable,     {&action_map_clear}},
   {"map_grid",    kac_map,      at_variable,     {&action_map_grid}},
};

const int num_keyactions = sizeof(keyactions) / sizeof(*keyactions);

// Console Bindings
//
// Console bindings are stored seperately and are added to list 
// dynamically.
//
// haleyjd: fraggle used an array of these and reallocated it every
// time it was full. Not exactly model efficiency. I have modified the 
// system to use a linked list.

static keyaction_t *cons_keyactions = NULL;

//
// The actual key bindings
//

#define NUM_KEYS 256

typedef struct doomkey_s
{
   char *name;
   boolean keydown[NUMKEYACTIONCLASSES];
   keyaction_t *bindings[NUMKEYACTIONCLASSES];
} doomkey_t;

static doomkey_t keybindings[NUM_KEYS];

static keyaction_t *G_KeyActionForName(const char *name);
extern binding_handler AM_Handlers[];

//
// G_InitKeyBindings
//
// Set up key names and various details
//
void G_InitKeyBindings(void)
{
   int i;
   keyaction_t *ka;
   
   // various names for different keys
   
   keybindings[KEYD_RIGHTARROW].name  = "rightarrow";
   keybindings[KEYD_LEFTARROW].name   = "leftarrow";
   keybindings[KEYD_UPARROW].name     = "uparrow";
   keybindings[KEYD_DOWNARROW].name   = "downarrow";
   keybindings[KEYD_ESCAPE].name      = "escape";
   keybindings[KEYD_ENTER].name       = "enter";
   keybindings[KEYD_TAB].name         = "tab";
   
   keybindings[KEYD_F1].name          = "f1";
   keybindings[KEYD_F2].name          = "f2";
   keybindings[KEYD_F3].name          = "f3";
   keybindings[KEYD_F4].name          = "f4";
   keybindings[KEYD_F5].name          = "f5";
   keybindings[KEYD_F6].name          = "f6";
   keybindings[KEYD_F7].name          = "f7";
   keybindings[KEYD_F8].name          = "f8";
   keybindings[KEYD_F9].name          = "f9";
   keybindings[KEYD_F10].name         = "f10";
   keybindings[KEYD_F11].name         = "f11";
   keybindings[KEYD_F12].name         = "f12";
   
   keybindings[KEYD_BACKSPACE].name   = "backspace";
   keybindings[KEYD_PAUSE].name       = "pause";
   keybindings[KEYD_MINUS].name       = "-";
   keybindings[KEYD_RSHIFT].name      = "shift";
   keybindings[KEYD_RCTRL].name       = "ctrl";
   keybindings[KEYD_RALT].name        = "alt";
   keybindings[KEYD_CAPSLOCK].name    = "capslock";
   
   keybindings[KEYD_INSERT].name      = "insert";
   keybindings[KEYD_HOME].name        = "home";
   keybindings[KEYD_END].name         = "end";
   keybindings[KEYD_PAGEUP].name      = "pgup";
   keybindings[KEYD_PAGEDOWN].name    = "pgdn";
   keybindings[KEYD_SCROLLLOCK].name  = "scrolllock";
   keybindings[KEYD_SPACEBAR].name    = "space";
   keybindings[KEYD_NUMLOCK].name     = "numlock";
   
   keybindings[KEYD_MOUSE1].name      = "mouse1";
   keybindings[KEYD_MOUSE2].name      = "mouse2";
   keybindings[KEYD_MOUSE3].name      = "mouse3";
   
   keybindings[KEYD_JOY1].name        = "joy1";
   keybindings[KEYD_JOY2].name        = "joy2";
   keybindings[KEYD_JOY3].name        = "joy3";
   keybindings[KEYD_JOY4].name        = "joy4";
   
   keybindings[','].name = "<";
   keybindings['.'].name = ">";
   
   for(i = 0; i < NUM_KEYS; ++i)
   {
      // fill in name if not set yet
      
      if(!keybindings[i].name)
      {
         char tempstr[32];
         
         // build generic name
         if(isprint(i))
            sprintf(tempstr, "%c", i);
         else
            sprintf(tempstr, "key%02i", i);
         
         keybindings[i].name = Z_Strdup(tempstr, PU_STATIC, 0);
      }
      
      memset(keybindings[i].bindings, 0, 
             NUMKEYACTIONCLASSES * sizeof(keyaction_t *));
   }

   // init some actions
   ka = G_KeyActionForName("map_right");
   ka->value.Handler = AM_Handlers[0];

   ka = G_KeyActionForName("map_left");
   ka->value.Handler = AM_Handlers[1];

   ka = G_KeyActionForName("map_up");
   ka->value.Handler = AM_Handlers[2];

   ka = G_KeyActionForName("map_down");
   ka->value.Handler = AM_Handlers[3];

   ka = G_KeyActionForName("map_zoomin");
   ka->value.Handler = AM_Handlers[4];

   ka = G_KeyActionForName("map_zoomout");
   ka->value.Handler = AM_Handlers[5];
}

//
// G_KeyActionForName
//
// Obtain a keyaction from its name
//
static keyaction_t *G_KeyActionForName(const char *name)
{
   int i;
   keyaction_t *prev, *temp, *newaction;
   
   // sequential search
   // this is only called every now and then
   
   for(i = 0; i < num_keyactions; ++i)
   {
      if(!strcasecmp(name, keyactions[i].name))
         return &keyactions[i];
   }
   
   // check console keyactions
   // haleyjd: TOTALLY rewritten to use linked list
      
   if(cons_keyactions)
   {
      temp = cons_keyactions;
      while(temp)
      {
         if(!strcasecmp(name, temp->name))
            return temp;
         
         temp = temp->next;
      }
   }
   else
   {
      // first time only - cons_keyactions was NULL
      cons_keyactions = Z_Malloc(sizeof(keyaction_t), PU_STATIC, 0);
      cons_keyactions->bclass = kac_game;
      cons_keyactions->type = at_conscmd;
      cons_keyactions->name = Z_Strdup(name, PU_STATIC, 0);
      cons_keyactions->next = NULL;
      
      return cons_keyactions;
   }
  
   // not in list -- add
   prev = NULL;
   temp = cons_keyactions;
   while(temp)
   {
      prev = temp;
      temp = temp->next;
   }
   newaction = Z_Malloc(sizeof(keyaction_t), PU_STATIC, 0);
   newaction->bclass = kac_game;
   newaction->type = at_conscmd;
   newaction->name = Z_Strdup(name, PU_STATIC, 0);
   newaction->next = NULL;
   
   if(prev) prev->next = newaction;

   return newaction;
}

//
// G_KeyForName
//
// Obtain a key from its name
//
static int G_KeyForName(char *name)
{
   int i;
   
   for(i = 0; i < NUM_KEYS; ++i)
   {
      if(!strcasecmp(keybindings[i].name, name))
         return tolower(i);
   }
   
   return -1;
}

//
// G_BindKeyToAction
//
static void G_BindKeyToAction(char *key_name, char *action_name)
{
   int key;
   keyaction_t *action;
   
   // get key
   if((key = G_KeyForName(key_name)) < 0)
   {
      C_Printf("unknown key '%s'\n", key_name);
      return;
   }

   // get action   
   if(!(action = G_KeyActionForName(action_name)))
   {
      C_Printf("unknown action '%s'\n", action_name);
      return;
   }

   // haleyjd 07/03/04: support multiple binding classes
   keybindings[key].bindings[action->bclass] = action;
}

//
// G_BoundKeys
//
// Get an ascii description of the keys bound to a particular action
//
char *G_BoundKeys(char *action)
{
   int i;
   static char ret[1024];   // store list of keys bound to this   
   keyaction_t *ke;
   
   if(!(ke = G_KeyActionForName(action)))
      return "unknown action";
   
   ret[0] = '\0';   // clear ret
   
   // sequential search -ugh

   // FIXME: buffer overflow possible!
   
   for(i = 0; i < NUM_KEYS; ++i)
   {
      if(keybindings[i].bindings[ke->bclass] == ke)
      {
         if(ret[0])
            strcat(ret, " + ");
         strcat(ret, keybindings[i].name);
      }
   }
   
   return ret[0] ? ret : "none";
}

//
// G_FirstBoundKey
//
// Get an ascii description of the first key bound to a particular 
// action.
//
char *G_FirstBoundKey(char *action)
{
   int i;
   static char ret[1024];
   keyaction_t *ke;
   
   if(!(ke = G_KeyActionForName(action)))
      return "unknown action";
   
   ret[0] = '\0';   // clear ret
   
   // sequential search -ugh
   
   for(i = 0; i < NUM_KEYS; ++i)
   {
      if(keybindings[i].bindings[ke->bclass] == ke)
      {
         strcpy(ret, keybindings[i].name);
         break;
      }
   }
   
   return ret[0] ? ret : "none";
}

//
// G_KeyResponder
//
// The main driver function for the entire key binding system
//
boolean G_KeyResponder(event_t *ev, int bclass)
{
   static boolean ctrldown;
   
   if(ev->data1 == KEYD_RCTRL)      // ctrl
      ctrldown = ev->type == ev_keydown;
   
   if(ev->type == ev_keydown)
   {
      int key = tolower(ev->data1);
      
      if(opensocket &&                 // netgame disconnect binding
         ctrldown && ev->data1 == 'd')
      {
         char buffer[128];
         
         psnprintf(buffer, sizeof(buffer),
                   "disconnect from server?\n\n%s", s_PRESSYN);
         MN_Question(buffer, "disconnect leaving");
         
         // dont get stuck thinking ctrl is down
         ctrldown = false;
         return true;
      }

      //if(!keybindings[key].keydown[bclass])
      {
         keybindings[key].keydown[bclass] = true;
                  
         if(keybindings[key].bindings[bclass])
         {
            switch(keybindings[key].bindings[bclass]->type)
            {
            case at_variable:
               *(keybindings[key].bindings[bclass]->value.variable) = 1;
               break;

            case at_function:
               keybindings[key].bindings[bclass]->value.Handler(ev);
               break;
               
            case at_conscmd:
               C_RunTextCmd(keybindings[key].bindings[bclass]->name);
               break;
               
            default:
               break;
            }
         }
      }
   }
   else if(ev->type == ev_keyup)
   {
      int key = tolower(ev->data1);

      keybindings[key].keydown[bclass] = false;
      
      if(keybindings[key].bindings[bclass])
      {
         switch(keybindings[key].bindings[bclass]->type)
         {
         case at_variable:
            *(keybindings[key].bindings[bclass]->value.variable) = 0;
            break;

         case at_function:
            keybindings[key].bindings[bclass]->value.Handler(ev);
            break;
            
         default:
            break;
         }
      }
   }

   return true;
}

//===========================================================================
//
// Binding selection widget
//
// For menu: when we select to change a key binding the widget is used
// as the drawer and responder
//
//===========================================================================

static char *binding_action;       // name of action we are editing

//
// G_BindDrawer
//
// Draw the prompt box
//
void G_BindDrawer(void)
{
   char temp[48];
   int wid, height;
   
   // draw the menu in the background
   
   MN_DrawMenu(current_menu);
   
   // create message
   
   strcpy(temp, "\n -= input new key =- \n");
   
   wid = V_StringWidth(temp);
   height = V_StringHeight(temp);
   
   // draw box
   
   V_DrawBox((SCREENWIDTH - wid) / 2 - 4,
             (SCREENHEIGHT - height) / 2 - 4,
             wid + 8,
             height + 8);

   // write text in box
   
   V_WriteText(temp,
               (SCREENWIDTH - wid) / 2,
               (SCREENHEIGHT - height) / 2);
}

//
// G_BindResponder
//
// Responder for widget
//
boolean G_BindResponder(event_t *ev)
{
   keyaction_t *action;
   
   if(ev->type != ev_keydown)
      return false;
   
   if(ev->data1 == KEYD_ESCAPE)    // cancel
   {
      current_menuwidget = NULL;
      return true;
   }

   // got a key - close box
   current_menuwidget = NULL;
   
   action = G_KeyActionForName(binding_action);
   if(!action)
   {
      C_Printf("unknown binding '%s'\n", binding_action);
      return true;
   }

   // bind new key to action, if its not already bound -- if it is,
   // remove it
   
   if(keybindings[ev->data1].bindings[action->bclass] != action)
      keybindings[ev->data1].bindings[action->bclass] = action;
   else
      keybindings[ev->data1].bindings[action->bclass] = NULL;
   
   return true;
}

menuwidget_t binding_widget = {G_BindDrawer, G_BindResponder, true};

//
// G_EditBinding
//
// Main Function
//
void G_EditBinding(char *action)
{
   current_menuwidget = &binding_widget;
   binding_action = action;
}

//===========================================================================
//
// Load/Save defaults
//
//===========================================================================

// default script:

static char *cfg_file = NULL; 

void G_LoadDefaults(void)
{
   char temp[PATH_MAX+1];
   DWFILE dwfile, *file = &dwfile;

   // haleyjd 03/15/03: fix for -cdrom
   // haleyjd 07/03/04: FIXME: doesn't work for linux
#ifndef LINUX
   if(M_CheckParm("-cdrom"))
      psnprintf(temp, sizeof(temp), "%s/%s", "c:/doomdata", "keys.csc");
   else
#endif
      psnprintf(temp, sizeof(temp), "%s/%s", D_DoomExeDir(), "keys.csc");
   
   cfg_file = strdup(temp);

   if(access(cfg_file, R_OK))
   {
      C_Printf("keys.csc not found, using defaults\n");
      D_OpenLump(file, W_GetNumForName("KEYDEFS"));
   }
   else
      D_OpenFile(file, cfg_file, "r");

   if(!D_IsOpen(file))
      I_Error("G_LoadDefaults: couldn't open default key bindings\n");

   C_RunScript(file);

   D_Fclose(file);
}

void G_SaveDefaults(void)
{
   FILE *file;
   int i, j;

   if(!cfg_file)         // check defaults have been loaded
      return;
   
   if(!(file = fopen(cfg_file, "w")))
   {
      C_Printf(FC_ERROR"Couldn't open keys.csc for write access.\n");
      return;
   }

  // write key bindings

   for(i = 0; i < NUM_KEYS; ++i)
   {
      // haleyjd 07/03/04: support multiple binding classes
      for(j = 0; j < NUMKEYACTIONCLASSES; ++j)
      {
         if(keybindings[i].bindings[j])
         {
            fprintf(file, "bind %s \"%s\"\n",
                    keybindings[i].name,
                    keybindings[i].bindings[j]->name);
         }
      }
   }
   
   fclose(file);
}

//===========================================================================
//
// Console Commands
//
//===========================================================================

CONSOLE_COMMAND(bind, 0)
{
   if(c_argc >= 2)
   {
      G_BindKeyToAction(c_argv[0], c_argv[1]);
   }
   else if(c_argc == 1)
   {
      int key = G_KeyForName(c_argv[0]);

      if(key < 0)
         C_Printf(FC_ERROR "no such key!\n");
      else
      {
         // haleyjd 07/03/04: multiple binding class support
         int j;
         boolean foundBinding = false;
         
         for(j = 0; j < NUMKEYACTIONCLASSES; ++j)
         {
            if(keybindings[key].bindings[j])
            {
               C_Printf("%s bound to %s (class %d)\n",
                        keybindings[key].name,
                        keybindings[key].bindings[j]->name,
                        keybindings[key].bindings[j]->bclass);
               foundBinding = true;
            }
         }
         if(!foundBinding)
            C_Printf("%s not bound\n", keybindings[key].name);
      }
   }
   else
      C_Printf("usage: bind key action\n");
}

// haleyjd: utility functions
CONSOLE_COMMAND(listactions, 0)
{
   int i;
   
   for(i = 0; i < num_keyactions; ++i)
      C_Printf("%s\n", keyactions[i].name);
}

CONSOLE_COMMAND(listkeys, 0)
{
   int i;

   for(i = 0; i < NUM_KEYS; ++i)
      C_Printf("%s\n", keybindings[i].name);
}

CONSOLE_COMMAND(unbind, 0)
{
   int key;
   int bclass = -1;

   if(c_argc < 1)
   {
      C_Printf("usage: unbind key [class]\n");
      return;
   }

   // allow specification of a binding class
   if(c_argc == 2)
   {
      bclass = atoi(c_argv[1]);
      if(bclass < 0 || bclass >= NUMKEYACTIONCLASSES)
      {
         C_Printf(FC_ERROR "invalid action class %d\n", bclass);
         return;
      }
   }
   
   if((key = G_KeyForName(c_argv[0])) != -1)
   {
      if(bclass == -1)
      {
         // unbind all actions
         int j;

         C_Printf("unbound key %s from all actions\n", c_argv[0]);
         
         for(j = 0; j < NUMKEYACTIONCLASSES; ++j)
            keybindings[key].bindings[j] = NULL;
      }
      else
      {
         keyaction_t *ke = keybindings[key].bindings[bclass];

         if(ke)
         {
            C_Printf("unbound key %s from action %s\n", c_argv[0], ke->name);
            keybindings[key].bindings[bclass] = NULL;
         }
         else
            C_Printf("key %s has no binding in class %d\n", c_argv[0], bclass);
      }
   }
   else
     C_Printf("unknown key %s\n", c_argv[0]);
}

CONSOLE_COMMAND(unbindall, 0)
{
   int i, j;
   
   C_Printf("clearing all key bindings\n");
   
   for(i = 0; i < NUM_KEYS; ++i)
   {
      for(j = 0; j < NUMKEYACTIONCLASSES; ++j)
         keybindings[i].bindings[j] = NULL;
   }
}

//
// bindings
//
// haleyjd 12/11/01
// list all active bindings to the console
//
CONSOLE_COMMAND(bindings, 0)
{
   int i, j;

   for(i = 0; i < NUM_KEYS; i++)
   {
      for(j = 0; j < NUMKEYACTIONCLASSES; ++j)
      {
         if(keybindings[i].bindings[j])
         {
            C_Printf("%s : %s\n",
                     keybindings[i].name,
                     keybindings[i].bindings[j]->name);
         }
      }
   }
}

void G_Bind_AddCommands()
{
   C_AddCommand(bind);
   C_AddCommand(listactions);
   C_AddCommand(listkeys);
   C_AddCommand(unbind);
   C_AddCommand(unbindall);
   C_AddCommand(bindings);
}

// EOF
