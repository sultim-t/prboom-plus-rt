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
 * DESCRIPTION:  Heads-up displays
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_net.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_stuff.h"
#include "hu_over.h"
//#include "p_info.h"
#include "st_stuff.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_draw.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_video.h"
#include "w_wad.h"

static boolean altdown = false;    // whether alt key is down

char *chat_macros[10];
const char* shiftxform;
const char english_shiftxform[];
//boolean chat_on;
static boolean chat_active = false;
int obituaries = 0;
int obcolour = CR_BRICK;       // the colour of death messages
int showMessages = 1;    // Show messages has default, 0 = off, 1 = on
int mess_colour = CR_RED;      // the colour of normal messages

extern char *levelname;        // p_info.c

//==========================================================================
//
// Normal Messages
//
// 'picked up a clip' etc.
// seperate from the widgets (below)
//
//==========================================================================

static char hu_messages[MAXHUDMESSAGES][256];
static int current_messages;   // the current number of messages
static int scrolltime;  // leveltime when the message list next needs
                        // to scroll up

int hud_msg_lines = 1;      // number of message lines in window up to 16
int hud_msg_scrollup = 1;   // whether message list scrolls up
int message_timer = 4000;   // timer used for normal messages

void HU_PlayerMsg(char *s)
{
  if(current_messages == hud_msg_lines)  // display full
    {
      int i;

      // scroll up

      for(i=0; i<hud_msg_lines-1; i++)
	strcpy(hu_messages[i], hu_messages[i+1]);

      strcpy(hu_messages[hud_msg_lines-1], s);
    }
  else            // add one to the end
    {
      strcpy(hu_messages[current_messages], s);
      current_messages++;
    }
  scrolltime = leveltime + (message_timer * 35) / 1000;
}

// erase the text before drawing

static void HU_MessageErase()
{
  int y;

  for(y=0; y<8*hud_msg_lines; y++)
    R_VideoErase(y*SCREENWIDTH, SCREENWIDTH);
}

static void HU_MessageDraw()
{
  int i;
  int x;

  if(!showMessages)
    return;

  // go down a bit if chat active
  x = chat_active ? 8 : 0;

  for(i=0; i<current_messages; i++, x += 8)
    V_WriteText(hu_messages[i], 0, x, 0);
}

static void HU_MessageClear()
{
  current_messages = 0;
}

static void HU_MessageTick()
{
  int i;

  if(!hud_msg_scrollup) return;   // messages not to scroll

  if(leveltime >= scrolltime)
    {
      for(i=0; i<current_messages-1; i++)
	strcpy(hu_messages[i], hu_messages[i+1]);
      current_messages = current_messages ? current_messages-1 : 0;
      scrolltime = leveltime + (message_timer * 35) / 1000;
    }
}

//==========================================================================
//
// Crosshair
//
//===========================================================================
#if 0
static patchnum_t crosshairs[CROSSHAIRS];
static patchnum_t crosshair=NULL;
static char *crosshairpal;
static char *targetcolour, *notargetcolour, *friendcolour;

int crosshairnum;       // 0= none
char *cross_str[]= {"none", "cross", "angle"}; // for console

static void HU_CrossHairDraw()
{
  int drawx, drawy;

  if(!crosshair) return;
  if(viewcamera || automapactive) return;

  // where to draw??

  drawx = SCREENWIDTH/2 - crosshair->width/2;
  drawy = scaledviewheight == SCREENHEIGHT ? SCREENHEIGHT/2 :
    (SCREENHEIGHT-ST_HEIGHT)/2;

  // check for bfglook: make crosshair face forward

  if(bfglook == 2 && players[displayplayer].readyweapon == wp_bfg)
    drawy += (players[displayplayer].updownangle * scaledviewheight)/100;

  drawy -= crosshair->height/2;

  if((drawy + crosshair->height) > ((viewwindowy + viewheight)>>hires) )
    return;

  if(crosshairpal == notargetcolour)
    V_DrawPatchTL(drawx, drawy, 0, crosshair, crosshairpal);
  else
    V_DrawPatchTranslated(drawx, drawy, 0, crosshair, crosshairpal, 0);
}

static void HU_CrossHairInit()
{
  crosshairs[0] = W_CacheLumpName("CROSS1", PU_STATIC);
  crosshairs[1] = W_CacheLumpName("CROSS2", PU_STATIC);

  notargetcolour = cr_red;
  targetcolour = cr_green;
  friendcolour = cr_blue;
  crosshairpal = notargetcolour;
  crosshair = crosshairnum ? crosshairs[crosshairnum-1] : NULL;
}

static void HU_CrossHairTick()
{
  player_t *player;
  mobj_t *playerobj;

  // don't bother with this crap if
  // the crosshair isn't going to be displayed anyway

  if(!crosshairnum)
    return;

  // default to no target
  crosshairpal = notargetcolour;

  // sf: use player prediction

  player = &players[displayplayer];
  if(player->predicted)
    player = player->predicted;
  playerobj = player->mo;

  // search for targets

  P_AimLineAttack(playerobj, playerobj->angle, 16*64*FRACUNIT, 0);

  if(linetarget && !(linetarget->flags & MF_SHADOW))
    {
      // target found

      crosshairpal = targetcolour;
      if(linetarget->flags & MF_FRIEND)
	crosshairpal = friendcolour;
    }
}

//======================================================================
//
// Pop-up Warning Boxes
//
// several different things that appear, quake-style, to warn you of
// problems
//
//======================================================================

// Network Sync Error
//
// We do not shutdown the game if we have a sync error but instead
// flash up a sync warning box

// Open Socket Warning
//
// Problem with network leads or something like that

//
// VPO Warning indicator
//
// most ports nowadays have removed the visplane overflow problem.
// however, many developers still make wads for plain vanilla doom.
// this should give them a warning for when they have 'a few
// planes too many'

/*
static patch_t *vpo;
static patch_t *socket;
static patch_t *sync;
*/

static void HU_WarningsInit()
{
  vpo = W_CacheLumpName("VPO", PU_STATIC);
  socket = W_CacheLumpName("OPENSOCK", PU_STATIC);
  sync = W_CacheLumpName("SYNC", PU_STATIC);
}

#define GAP 10

extern int num_visplanes;
extern boolean out_of_sync;
int show_vpo = 0;

#define WARNING_X 20
#define WARNING_Y 20

static void HU_WarningsDrawer()
{
  int x = WARNING_X;

  // the number of visplanes drawn is less in boom.
  // i lower the threshold to 85

  if(show_vpo && num_visplanes > 85)
    {
      V_DrawPatch(x, WARNING_Y, 0, vpo);
      x += vpo->width + GAP;
    }

  if(opensocket)
    {
      V_DrawPatch(x, WARNING_Y, 0, socket);
      x += socket->width + GAP;

      V_WriteText("ctrl-d to disconnect from server",
		  WARNING_X, WARNING_Y + socket->height+5);
    }

  // out of sync?

  if(out_of_sync)
    {
      V_DrawPatch(x, WARNING_Y, 0, sync);
      x += sync->width + GAP;
    }
}
#endif

//=========================================================================
//
// Text Widgets
//
// the main text widgets. does not include the normal messages
// 'picked up a clip' etc
//
//=========================================================================

static textwidget_t *widgets[MAXWIDGETS];
static int num_widgets = 0;

static void HU_AddWidget(textwidget_t *widget)
{
  widgets[num_widgets] = widget;
  num_widgets++;
}

// draw widgets

static void HU_WidgetsDraw()
{
  int i;

  // check each widget.
  // draw according to font type, and only if message being displayed

  for(i=0; i<num_widgets; i++)
  {
    if (widgets[i]->message && (!widgets[i]->cleartic || leveltime < widgets[i]->cleartic) )
	    if (widgets[i]->font)
        HU_WriteText(widgets[i]->message, widgets[i]->x, widgets[i]->y);
      else
        V_WriteText(widgets[i]->message, widgets[i]->x, widgets[i]->y, 0);
  }
}

static void HU_WidgetsTick()
{
  int i;

  for(i=0; i<num_widgets; i++)
    {
      if(widgets[i]->handler)
	widgets[i]->handler();
    }
}

        // erase all the widget text
static void HU_WidgetsErase()
{
  int i, y;

  for(i=0; i<num_widgets; i++)
    {
      for(y=widgets[i]->y; y<widgets[i]->y+8; y++)
	R_VideoErase(y*SCREENWIDTH, SCREENWIDTH);
    }
}

static void HU_LevelTimeHandler();
static void HU_CentreMessageHandler();
static void HU_LevelNameHandler();
static void HU_ChatHandler();

//--------------------------------------------------------------------------
//
// Centre-of-screen, quake-style message
//

static textwidget_t hu_centremessage =
  {
    0, 0,                      // x,y set by HU_CentreMsg
    0,                         // normal font
    NULL,                      // init to nothing
    HU_CentreMessageHandler    // handler
  };
static int centremessage_timer = 1500;         // 1.5 seconds

static void HU_CentreMessageHandler(void)
{
  return;         // do nothing
}

static void HU_CentreMessageClear(void)
{
  hu_centremessage.message = NULL;
}

void HU_CentreMsg(char *s)
{
  static char *centremsg = NULL;
  static int allocedsize = 0;

  // removed centremsg limit
  if((int)strlen(s) > allocedsize)
  {
    centremsg = centremsg ? Z_Realloc(centremsg, strlen(s)+3, PU_STATIC, 0) : Z_Malloc(strlen(s)+3, PU_STATIC, 0);
    allocedsize = strlen(s);
  }
  strcpy(centremsg, s);

  hu_centremessage.message = centremsg;
  hu_centremessage.x = (SCREENWIDTH-V_StringWidth(s, 0)) / 2;
  hu_centremessage.y = (SCREENHEIGHT-V_StringHeight(s) - ((viewheight==SCREENHEIGHT) ? 0 : ST_SCALED_HEIGHT-8)) / 2;
  hu_centremessage.cleartic = leveltime + (centremessage_timer * 35) / 1000;

  // print to console
  C_Printf("%s\n", s);
}

//-----------------------------------------------------------------------
//
// Elapsed level time (automap)
//

static textwidget_t hu_leveltime =
{
  0, 0,                                          // x, y
  0,                                             // normal font
  NULL,                                          // null msg
  HU_LevelTimeHandler                            // handler
};

static void HU_LevelTimeHandler()
{
  static char timestr[100];
  int seconds = 0;

  if(!(automapmode & am_active))
    {
      hu_leveltime.message = NULL;
      return;
    }

  //seconds = levelTime / 35;
  timestr[0] = 0;

  sprintf(timestr, "%02i:%02i:%02i", seconds/3600, (seconds%3600)/60,
	  seconds%60);

  hu_leveltime.x = SCREENWIDTH-60;
  hu_leveltime.y = SCREENHEIGHT-ST_SCALED_HEIGHT-8;
  hu_leveltime.message = timestr;
}

//------------------------------------------------------------------------
//
// Automap level name display
//

static textwidget_t hu_levelname =
{
  0, 0,                              // x,y
  0,                                 // normal font
  NULL,                              // init to nothing
  HU_LevelNameHandler                // handler
};

static void HU_LevelNameHandler()
{
  hu_leveltime.y = SCREENHEIGHT-ST_SCALED_HEIGHT-8;
/*
  if (automapmode & am_active)
    hu_levelname.message = levelname;
  else
*/
    hu_levelname.message = NULL;
}

//----------------------------------------------------------------------
//
// Chat message display
//

static textwidget_t hu_chat =
{
  0, 0,                 // x,y
  0,                    // use normal font
  NULL,                 // empty message
  HU_ChatHandler        // handler
};
static char chatinput[100] = "";

static void HU_ChatHandler()
{
  static char tempchatmsg[128];

  if(chat_active)
    {
      sprintf(tempchatmsg, "%s_", chatinput);
      hu_chat.message = tempchatmsg;
    }
  else
    hu_chat.message = NULL;
}

static boolean HU_ChatRespond(event_t *ev)
{
  char ch;
  static boolean shiftdown;

  if(ev->data1 == KEYD_RSHIFT) shiftdown = ev->type == ev_keydown;

  if(ev->type != ev_keydown) return false;

  if(!chat_active)
    {
      if(ev->data1 == key_chat && netgame)
	{
	  chat_active = true;     // activate chat
	  chatinput[0] = 0;       // empty input string
	  return true;
	}
      return false;
    }

  if(altdown && ev->type == ev_keydown &&
     ev->data1 >= '0' && ev->data1 <= '9')
    {
      // chat macro
      char tempstr[100];
      sprintf(tempstr, "say \"%s\"", chat_macros[ev->data1-'0']);
      C_RunTextCmd(tempstr);
      chat_active = false;
      return true;
    }

  if(ev->data1 == KEYD_ESCAPE)    // kill chat
    {
      chat_active = false;
      return true;
    }

  if(ev->data1 == KEYD_BACKSPACE && chatinput[0])
    {
      chatinput[strlen(chatinput)-1] = 0;      // remove last char
      return true;
    }

  if(ev->data1 == KEYD_ENTER)
    {
      char tempstr[100];
      sprintf(tempstr, "say \"%s\"", chatinput);
      C_RunTextCmd(tempstr);
      chat_active = false;
      return true;
    }

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

  if(ch>31 && ch<127)
    {
      sprintf(chatinput, "%s%c", chatinput, ch);
      C_InitTab();
      return true;
    }
  return false;
}

// Widgets Init

static void HU_WidgetsInit()
{
  num_widgets = 0;
  HU_AddWidget(&hu_centremessage);
  HU_AddWidget(&hu_levelname);
  HU_AddWidget(&hu_leveltime);
  HU_AddWidget(&hu_chat);
}


//=========================================================================
//
// Main Functions
//
// Init, Drawer, Ticker etc.
//
//=========================================================================

void HU_Start()
{
  HU_MessageClear();
  HU_CentreMessageClear();
}

void HU_End()
{
}

void HU_Init()
{
  shiftxform = english_shiftxform;

  // init different modules
  //HU_CrossHairInit();
  HU_FragsInit();
  //HU_WarningsInit();
  HU_WidgetsInit();
  HU_LoadFont();
}

void HU_Drawer()
{
  // draw different modules
  HU_MessageDraw();
  //HU_CrossHairDraw();
  HU_FragsDrawer();
  //HU_WarningsDrawer();
  HU_WidgetsDraw();
  HU_OverlayDraw();
#ifdef INSTRUMENTED
  {
    extern void Z_DrawStats(void);
    Z_DrawStats();           // draw memory allocation stats
  }
#endif
}

void HU_Ticker()
{
  // run tickers for some modules
  //HU_CrossHairTick();
  HU_WidgetsTick();
  HU_MessageTick();
}

boolean HU_Responder(event_t *ev)
{
  if(ev->data1 == KEYD_LALT)
    altdown = ev->type == ev_keydown;

  return HU_ChatRespond(ev);
}

// hu_newlevel called when we enter a new level
// determine the level name and display it in
// the console

void HU_NewLevel()
{
  // print the new level name into the console

  C_Printf("\n");
  C_Seperator();
  C_Printf(FC_GRAY " %s\n\n", levelname);
  C_InstaPopup();       // put console away
  //  C_Update();
}

        // erase text that can be trashed by small screens
void HU_Erase()
{
  if(!viewwindowx || (automapmode & am_active))
    return;

  // run indiv. module erasers
  HU_MessageErase();
  HU_WidgetsErase();
  HU_FragsErase();
}


//===========================================================================
//
// Tables
//
//===========================================================================

const char* shiftxform;

const char english_shiftxform[] =
{
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};

//==========================================================================
//
// Console Commands
//
//==========================================================================

CONSOLE_INT(mess_timer, message_timer, NULL, 0, 100000, NULL, 0) {}
CONSOLE_INT(mess_lines, hud_msg_lines, NULL, 0, 14, NULL, 0) {}
CONSOLE_BOOLEAN(mess_scrollup, hud_msg_scrollup, NULL, yesno, 0) {}
//CONSOLE_BOOLEAN(show_vpo, show_vpo, NULL, onoff, cf_nosave) {}
CONSOLE_BOOLEAN(obituaries, obituaries, NULL, onoff, 0) {}
CONSOLE_INT(obcolour, obcolour, NULL, 0, CR_LIMIT-1, textcolours, 0) {}
/*
CONSOLE_INT(crosshair, crosshairnum, NULL, 0, CROSSHAIRS-1, cross_str, 0)
{
  int a;

  a=atoi(c_argv[0]);

  crosshair = a ? crosshairs[a-1] : NULL;
  crosshairnum = a;
}
*/
CONSOLE_BOOLEAN(messages, showMessages, NULL, onoff, 0) {}
CONSOLE_INT(mess_colour, mess_colour, NULL, 0, CR_LIMIT-1, textcolours, 0) {}

CONSOLE_NETCMD(say, cf_netvar, netcmd_chat)
{
  S_StartSound(0, gamemode == commercial ? sfx_radio : sfx_tink);

  doom_printf("%s: %s", players[cmdsrc].name, c_args);
}

extern void HU_FragsAddCommands();
extern void HU_OverAddCommands();

void HU_AddCommands()
{
  C_AddCommand(obituaries);
  C_AddCommand(obcolour);
  //C_AddCommand(crosshair);
  //C_AddCommand(show_vpo);
  C_AddCommand(messages);
  C_AddCommand(mess_colour);
  C_AddCommand(say);

  C_AddCommand(mess_lines);
  C_AddCommand(mess_scrollup);
  C_AddCommand(mess_timer);

  HU_FragsAddCommands();
  HU_OverAddCommands();
}
