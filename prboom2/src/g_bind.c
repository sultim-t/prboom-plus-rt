/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: g_bind.c,v 1.7 2002/01/12 16:16:40 cph Exp $
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
 *
 * Key Bindings
 *
 * Rather than use the old system of binding a key to an action, we bind
 * an action to a key. This way we can have as many actions as we want
 * bound to a particular key.
 *
 * By Simon Howard, Revised by James Haley, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: g_bind.c,v 1.7 2002/01/12 16:16:40 cph Exp $";

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
#include "lprintf.h"

// Action variables
// These variables are asserted as positive values when the action
// they represent has been performed by the player via key pressing.

int action_forward; // forward movement
int action_backward;  // backward movement
int action_left;  // left movement
int action_right; // right movement
int action_moveleft;  // key-strafe left
int action_moveright; // key-strafe right
int action_use;   // object activation
int action_speed; // running
int action_attack;  // firing current weapon
int action_strafe;  // strafe in any direction
int action_flip;  // 180 degree turn

int action_weapon1; // select weapon 1
int action_weapon2; // select weapon 2
int action_weapon3; // select weapon 3
int action_weapon4; // select weapon 4
int action_weapon5; // select weapon 5
int action_weapon6; // select weapon 6
int action_weapon7; // select weapon 7
int action_weapon8; // select weapon 8
int action_weapon9; // select weapon 9

int action_toggleweapon;  // toggle to next-favored weapon

//
// Handler Functions
//
// Functions that perform special activities in relation to the
// assertion of the above events should be placed here.
//

//
// Actions List
//

typedef struct keyaction_s
{
  const char *name;     /* text description - cphipps: const char * */
  enum
    {
      at_variable,
      at_function,
      at_conscmd,       // console command
    } type;
  union
  {
  int *variable;   // variable -- if non-zero, action activated (key down)
    void (*Handler)();
  } value;

  struct keyaction_s *next; // haleyjd: used for console bindings

} keyaction_t;

keyaction_t keyactions[] =
{
   {"forward",          at_variable,     {&action_forward}},
   {"backward",         at_variable,     {&action_backward}},
   {"left",             at_variable,     {&action_left}},
   {"right",            at_variable,     {&action_right}},
   {"moveleft",         at_variable,     {&action_moveleft}},
   {"moveright",        at_variable,     {&action_moveright}},
   {"use",              at_variable,     {&action_use}},
   {"strafe",           at_variable,     {&action_strafe}},
   {"attack",           at_variable,     {&action_attack}},
   {"flip",             at_variable,     {&action_flip}},
   {"speed",            at_variable,     {&action_speed}},

   {"weapon1",          at_variable,     {&action_weapon1}},
   {"weapon2",          at_variable,     {&action_weapon2}},
   {"weapon3",          at_variable,     {&action_weapon3}},
   {"weapon4",          at_variable,     {&action_weapon4}},
   {"weapon5",          at_variable,     {&action_weapon5}},
   {"weapon6",          at_variable,     {&action_weapon6}},
   {"weapon7",          at_variable,     {&action_weapon7}},
   {"weapon8",          at_variable,     {&action_weapon8}},
   {"weapon9",          at_variable,     {&action_weapon9}},
   {"toggleweapon",     at_variable,     {&action_toggleweapon}},
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

#define NUM_KEYS 512

typedef struct
{
  const char *name;
  boolean keydown;
  keyaction_t *binding;
} doomkey_t;

static doomkey_t keybindings[NUM_KEYS];

//
// G_InitKeyBindings
//
// Set up key names and various details
//
void G_InitKeyBindings()
{
  int i;
  
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
  keybindings[KEYD_MOUSE4].name      = "mouse4";
  keybindings[KEYD_MOUSE5].name      = "mouse5";
  keybindings[KEYD_MOUSED1].name      = "moused1";
  keybindings[KEYD_MOUSED2].name      = "moused2";
  keybindings[KEYD_MOUSED3].name      = "moused3";

  keybindings[KEYD_JOY1].name        = "joy1";
  keybindings[KEYD_JOY2].name        = "joy2";
  keybindings[KEYD_JOY3].name        = "joy3";
  keybindings[KEYD_JOY4].name        = "joy4";
  keybindings[KEYD_JOY5].name        = "joy5";
  keybindings[KEYD_JOY6].name        = "joy6";
  keybindings[KEYD_JOY7].name        = "joy7";
  keybindings[KEYD_JOY8].name        = "joy8";
  keybindings[KEYD_JOY9].name        = "joy9";
  keybindings[KEYD_JOY10].name       = "joy10";
  keybindings[KEYD_JOY11].name       = "joy11";
  keybindings[KEYD_JOY12].name       = "joy12";
  keybindings[KEYD_JOY13].name       = "joy13";
  keybindings[KEYD_JOY14].name       = "joy14";
  keybindings[KEYD_JOY15].name       = "joy15";
  keybindings[KEYD_JOY16].name       = "joy16";

  keybindings[','].name = "<";
  keybindings['.'].name = ">";
  
  for(i=0; i<NUM_KEYS; i++)
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
    
      keybindings[i].name = strdup(tempstr);
    }

    keybindings[i].binding = NULL;
  }
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

  for(i=0; i<num_keyactions; i++)
    if(!strcasecmp(name, keyactions[i].name))
      return &keyactions[i];

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
     cons_keyactions = malloc(sizeof(keyaction_t));
     cons_keyactions->type = at_conscmd;
     cons_keyactions->name = strdup(name);
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
  newaction = malloc(sizeof(keyaction_t));
  newaction->type = at_conscmd;
  newaction->name = strdup(name);
  newaction->next = NULL;

  if(prev) prev->next = newaction;

  return newaction;  
}

//
// G_KeyForName
//
// Obtain a keyaction from its name
//
static int G_KeyForName(const char *name)
{
  int i;

  for(i=0; i<NUM_KEYS; i++)
    if(!strcasecmp(keybindings[i].name, name))
      return tolower(i);

  return -1;
}

//
// G_BindKeyToAction
//

static void G_BindKeyToAction(const char *key_name, const char *action_name)
{
  int key;
  keyaction_t *action;
  
  // get key
  
  key = G_KeyForName(key_name);

  if(key < 0)
  {
     C_Printf("unknown key '%s'\n", key_name);
     return;
  }

  // get action
  
  action = G_KeyActionForName(action_name);

  if(!action)
  {
     C_Printf("unknown action '%s'\n", action_name);
     return;
  }

  keybindings[key].binding = action;
}

//
// G_BoundKeys
//
// Get an ascii description of the keys bound to a particular action
//
const char *G_BoundKeys(char *action)
{
  int i;
  static char ret[NUM_KEYS];   // store list of keys bound to this

  keyaction_t *ke = G_KeyActionForName(action);
  
  if(!ke)
    return "unknown action";

  ret[0] = '\0';   // clear ret
  
  // sequential search -ugh

  for(i=0; i<NUM_KEYS; i++)
    {
      if(keybindings[i].binding == ke)
  {
    if(ret[0])
      strcat(ret, " + ");
    strcat(ret, keybindings[i].name);
  }
    }
  
  return ret[0] ? ret : "none";
}

//
// G_KeyResponder
//
// The main driver function for the entire key binding system
//
boolean G_KeyResponder(event_t *ev)
{
  static boolean ctrldown;

  if(ev->data1 == KEYD_RCTRL)      // ctrl
    ctrldown = ev->type == ev_keydown;
 
  if(ev->type == ev_keydown)
  {
    int key = tolower(ev->data1);

/*
      if(opensocket &&                 // netgame disconnect binding
	 ctrldown && ev->data1 == 'd')
	{
	  char buffer[128];

	  sprintf(buffer, "disconnect from server?\n\n%s", s_PRESSYN);
	  MN_Question(buffer, "disconnect leaving");

	  // dont get stuck thinking ctrl is down
	  ctrldown = false;
	  return true;
	}
*/      
    if(!keybindings[key].keydown)
    {
      keybindings[key].keydown = true;
    
      if(keybindings[key].binding)
	    {
	      switch(keybindings[key].binding->type)
		{
		  case at_variable:
		    (*keybindings[key].binding->value.variable)++;
		    break;

		  case at_function:
		    keybindings[key].binding->value.Handler();
		    break;

		  case at_conscmd:
		    C_RunTextCmd(keybindings[key].binding->name);
		    break;
		    
		  default:
		    break;
		}
	    }
    }
  }

  if(ev->type == ev_keyup)
  {
    int key = tolower(ev->data1);

    keybindings[key].keydown = false;

    if(keybindings[key].binding)
    {
	  switch(keybindings[key].binding->type)
	    {
	      case at_variable:
		if(*keybindings[key].binding->value.variable > 0)
		  (*keybindings[key].binding->value.variable)--;
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
void G_BindDrawer()
{
  char temp[100];
  int wid, height;
  
  // draw the menu in the background

  MN_DrawMenu(current_menu);

  // create message
  
  strcpy(temp, "\n -= input new key =- \n");
  
  wid = V_StringWidth(temp, 0);
  height = V_StringHeight(temp);

  // draw box
  
/*
  V_DrawBox((SCREENWIDTH - wid) / 2 - 4,
	    (SCREENHEIGHT - height) / 2 - 4,
	    wid + 8,
	    height + 8);
*/

  // write text in box

  V_WriteText(temp,
	      (320 - wid) / 2,
	      (200 - height) / 2, 0);
}

//
// G_BindResponder
//
// Responder for widget
//
boolean G_BindResponder(event_t *ev)
{
  keyaction_t *action;
  int i;
  int bound;
  
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

  // find how many keys bound

  bound = 0;
  for(i=0; i<NUM_KEYS; i++)
    bound += (keybindings[i].binding == action);

  // if already 2 or more then clear them
  
  if(bound >= 2)
    for(i=0; i<NUM_KEYS; i++)
      if(keybindings[i].binding == action)
	keybindings[i].binding = NULL;

  // bind new key to action

  keybindings[ev->data1].binding = action;

  return true;
}

menuwidget_t binding_widget = {G_BindDrawer, G_BindResponder};

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

void G_LoadDefaults(const char *file)
{
  byte *cfg_data;

  cfg_file = strdup(file);

  if(M_ReadFile(cfg_file, &cfg_data) <= 0)
  {
      C_Printf("cfg not found.\n");
      //C_Printf("cfg not found. using default\n");
      //cfg_data = W_CacheLumpName("DEFAULT", PU_STATIC);
  } else {
    C_RunScript(cfg_data);

    free(cfg_data);
  }
}

void G_SaveDefaults()
{
  FILE *file;
  command_t *cmd;
  int i;

  if(!cfg_file)         // check defaults have been loaded
     return;

  file = fopen(cfg_file, "w");

  // write console variables
  
  for(i=0; i<CMDCHAINS; i++)
  {
    for(cmd = cmdroots[i]; cmd; cmd = cmd->next)
	  {
	    if(cmd->type != ct_variable)    // only write variables
	      continue;
	    if(cmd->flags & cf_nosave)      // do not save if cf_nosave set
	      continue;
	    
	    fprintf(file, "%s \"%s\"\n", cmd->name,
		    C_VariableValue(cmd->variable));
	  }
  }

  // write key bindings

  for(i=0; i<NUM_KEYS; i++)
    {
      if(keybindings[i].binding)
	{
	  fprintf(file, "bind %s \"%s\"\n",
		keybindings[i].name,
		keybindings[i].binding->name);
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
	C_Printf("no such key!\n");
     else
     {
	if(keybindings[key].binding)
	   C_Printf("%s bound to %s\n", keybindings[key].name,
		    keybindings[key].binding->name);
	else
	   C_Printf("%s not bound\n", keybindings[key].name);
     }
  }
  else
  {
     C_Printf("usage: bind key action\n");
  }
}

// haleyjd: utility functions
CONSOLE_COMMAND(listactions, 0)
{
   int i;
   
   for(i=0; i<num_keyactions; i++)
   {
      C_Printf("%s\n", keyactions[i].name);
   }
}

CONSOLE_COMMAND(listkeys, 0)
{
   int i;

   for(i=0; i<NUM_KEYS; i++)
   {
      C_Printf("%s, ", keybindings[i].name);
      if(!((i+1)%4))
        C_Printf("\n");
   }
}

CONSOLE_COMMAND(unbind, 0)
{
   int key;

   if(c_argc != 1)
   {
   	C_Printf("usage: unbind key\n");
   	return;
   }
   
   if((key = G_KeyForName(c_argv[0])) != -1)
   {
   	C_Printf("unbound key %s\n", c_argv[0]);
   	keybindings[key].binding = NULL;
   }
   else
     C_Printf("unknown key %s\n", c_argv[0]);   
}

CONSOLE_COMMAND(unbindall, 0)
{
   int i;
   
   C_Printf("clearing all key bindings\n");
   
   for(i=0; i<NUM_KEYS; i++)
   {
   	keybindings[i].binding = NULL;
   }
}   	

void G_Bind_AddCommands()
{
  C_AddCommand(bind);
  C_AddCommand(listactions);
  C_AddCommand(listkeys);
  C_AddCommand(unbind);
  C_AddCommand(unbindall);
}
