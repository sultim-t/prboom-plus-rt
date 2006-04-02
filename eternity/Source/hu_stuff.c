// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
// Heads-Up Display
//
// haleyjd: Rewritten a second time. Displays widgets using a system
// of pseudoclass structures, complete with pseudo-inheritance and
// pseudo-polymorphism. This may as well be C++, except it's actually
// more flexible. This module is more open to scripting now, as a result.
//
// By James Haley and Simon Howard
//
//----------------------------------------------------------------------------

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_net.h"
#include "c_io.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "am_map.h"
#include "d_gi.h"
#include "m_qstr.h"
#include "a_small.h"
#include "d_dehtbl.h"

char *chat_macros[10];
const char* shiftxform;
const char english_shiftxform[];
//boolean chat_on;
boolean chat_active = false;
int obituaries = 0;
int obcolour = CR_BRICK;       // the colour of death messages

static boolean HU_ChatRespond(event_t *ev);

// haleyjd 06/04/05: Complete HUD rewrite.

// Widget Hashing

#define NUMWIDGETCHAINS 17

static hu_widget_t *hu_chains[NUMWIDGETCHAINS];

//
// Widget Superclass Functionality
//

//
// HU_WidgetForName
//
// Retrieves a widget given its name, using hashing.
// Returns NULL if no such widget exists.
//
static hu_widget_t *HU_WidgetForName(const char *name)
{
   int key = D_HashTableKey(name) % NUMWIDGETCHAINS;
   hu_widget_t *cur = hu_chains[key];

   while(cur && strncasecmp(name, cur->name, 33))
      cur = cur->next;

   return cur;
}

//
// HU_AddWidgetToHash
//
// Adds a widget to the hash table, but only if one of the given
// name doesn't exist. Returns true if successful.
//
static boolean HU_AddWidgetToHash(hu_widget_t *widget)
{
   int key;

   // make sure one doesn't already exist by this name
   if(HU_WidgetForName(widget->name))
   {
#ifdef RANGECHECK
      // for debug, cause an error to alert programmer of internal mishaps
      I_Error("HU_AddWidgetToHash: duplicate mnemonic %s\n", widget->name);
#endif
      return false;
   }

   key = D_HashTableKey(widget->name) % NUMWIDGETCHAINS;
   widget->next = hu_chains[key];
   hu_chains[key] = widget;

   return true;
}

//
// HU_ToggleWidget
//
// Sets the disable flag on a widget and properly propagates the state
// to the previous disable state member, so that widgets can still
// erase properly.
//
d_inline static void HU_ToggleWidget(hu_widget_t *widget, boolean disable)
{
   widget->prevdisabled = widget->disabled;
   widget->disabled = disable;
}

//
// HU_NeedsErase
//
// Returns true or false to indicate whether or not a widget needs
// erasing. Sets prevdisabled to disabled to end erasing after the
// first frame this is called for disabled widgets.
//
d_inline static boolean HU_NeedsErase(hu_widget_t *widget)
{
   // needs erase if enabled, or if WAS enabled on last frame
   boolean ret = !widget->disabled || !widget->prevdisabled;

   widget->prevdisabled = widget->disabled;

   return ret;
}

//
// Main HUD System Functions; Called externally.
//

static void HU_InitMsgWidget(void);
static void HU_InitCrossHair(void);
static void HU_InitWarnings(void);
static void HU_InitCenterMessage(void);
static void HU_InitLevelTime(void);
static void HU_InitLevelName(void);
static void HU_InitChat(void);
static void HU_InitCoords(void);

//
// HU_InitNativeWidgets
//
// Sets up all the native widgets. Called from HU_Init below at startup.
//
static void HU_InitNativeWidgets(void)
{   
   HU_InitMsgWidget();
   HU_InitCrossHair();
   HU_InitWarnings();
   HU_InitCenterMessage();
   HU_InitLevelTime();
   HU_InitLevelName();
   HU_InitChat();
   HU_InitCoords();

   // HUD_FIXME: generalize?
   HU_FragsInit();
}

//
// HU_Init
//
// Called once at game startup to initialize the HUD system.
//
void HU_Init(void)
{
   shiftxform = english_shiftxform;

   HU_LoadFont();
   HU_InitNativeWidgets();
}

//
// HU_Start
//
// Called to reinitialize the HUD system due to start of a new map
// or change of player viewpoint.
//
void HU_Start(void)
{
   int i;
   hu_widget_t *widget;

   // call all widget clear functions
   for(i = 0; i < NUMWIDGETCHAINS; ++i)
   {
      widget = hu_chains[i];

      while(widget)
      {
         if(widget->clear)
            widget->clear(widget);
         widget = widget->next;
      }
   }

   // execute script event handlers
   if(gameScriptLoaded)
      A_OptScriptCallback(&GameScript, "OnHUDStart");

   if(levelScriptLoaded)
      A_OptScriptCallback(&LevelScript, "OnHUDStart");
}

//
// HU_Drawer
//
// Called from D_Display to draw all HUD elements.
//
void HU_Drawer(void)
{
   int i;
   hu_widget_t *widget;

   // execute script event handlers
   if(gameScriptLoaded)
      A_OptScriptCallback(&GameScript, "OnHUDPreDraw");

   if(levelScriptLoaded)
      A_OptScriptCallback(&LevelScript, "OnHUDPreDraw");

   // call all widget drawer functions
   for(i = 0; i < NUMWIDGETCHAINS; ++i)
   {
      widget = hu_chains[i];

      while(widget)
      {
         if(widget->drawer)
            widget->drawer(widget);
         widget = widget->next;
      }
   }

   // HUD_FIXME: generalize?
   // draw different modules
   HU_FragsDrawer();
   HU_OverlayDraw();
}

//
// HU_Ticker
//
// Called from G_Ticker when gamestate is GS_LEVEL. Updates widgets
// for the current gametic.
//
void HU_Ticker(void)
{
   int i;
   hu_widget_t *widget;

   // call all widget ticker functions
   for(i = 0; i < NUMWIDGETCHAINS; ++i)
   {
      widget = hu_chains[i];

      while(widget)
      {
         if(widget->ticker)
            widget->ticker(widget);
         widget = widget->next;
      }
   }
}

// I don't know what this is doing here, but it can stay for now.
boolean altdown = false;

//
// HU_Responder
//
// Called from G_Responder. Has priority over any other events
// intercepted by that function.
//
boolean HU_Responder(event_t *ev)
{
   if(ev->data1 == KEYD_LALT)
      altdown = (ev->type == ev_keydown);
   
   // only the chat widget can respond to events (for now at least)
   return HU_ChatRespond(ev);
}

//
// HU_Erase
// 
// Called from D_Display to erase widgets displayed on the previous
// frame. This clears up areas of the screen not redrawn each tick.
//
void HU_Erase(void)
{
   int i;
   hu_widget_t *widget;

   if(!viewwindowx || automapactive)
      return;

   // call all widget erase functions
   for(i = 0; i < NUMWIDGETCHAINS; ++i)
   {
      widget = hu_chains[i];

      while(widget)
      {
         if(widget->eraser && HU_NeedsErase(widget))
            widget->eraser(widget);
         widget = widget->next;
      }
   }

   // HUD_FIXME: generalize?
   // run indiv. module erasers
   HU_FragsErase();
}

//
// Normal Player Message Widget
//

#define MAXHUDMESSAGES 16
#define MAXHUDMSGLEN   256

typedef struct hu_msgwidget_s
{
   hu_widget_t widget; // parent widget

   char messages[MAXHUDMESSAGES][MAXHUDMSGLEN]; // messages
   int  current_messages;              // number of messages
   int  scrolltime;                    // leveltime when message will scroll up
} hu_msgwidget_t;

static hu_msgwidget_t msg_widget;

// globals related to the msg_widget

int hud_msg_lines;   // number of message lines in window up to 16
int hud_msg_scrollup;// whether message list scrolls up
int message_timer;   // timer used for normal messages
int showMessages;    // Show messages has default, 0 = off, 1 = on
int mess_colour = CR_RED;      // the colour of normal messages

//
// HU_MessageTick
//
// Updates the message widget on each gametic.
//
static void HU_MessageTick(hu_widget_t *widget)
{
   int i;
   hu_msgwidget_t *mw = (hu_msgwidget_t *)widget;
   
   // check state of message scrolling
   if(!hud_msg_scrollup)
      return;

   // move up messages
   if(leveltime >= mw->scrolltime)
   {
      for(i = 0; i < mw->current_messages - 1; ++i)
         strncpy(mw->messages[i], mw->messages[i + 1], MAXHUDMSGLEN);

      if(mw->current_messages)
         mw->current_messages--;
      mw->scrolltime = leveltime + (message_timer * 35) / 1000;
   }
}

//
// HU_MessageDraw
//
// Drawer for the player message widget.
//
static void HU_MessageDraw(hu_widget_t *widget)
{
   int i, y;
   hu_msgwidget_t *mw = (hu_msgwidget_t *)widget;
   
   if(!showMessages)
      return;
   
   // go down a bit if chat active
   y = chat_active ? 8 : 0;
   
   for(i = 0; i < mw->current_messages; i++, y += 8)
   {
      int x = 0;
      char *msg = mw->messages[i];

      // haleyjd 12/26/02: center messages in Heretic
      // FIXME/TODO: make this an option in DOOM?
      if(gameModeInfo->type == Game_Heretic)
         x = (SCREENWIDTH - V_StringWidth(msg)) >> 1;
      
      // haleyjd 06/04/05: use V_WriteTextColoured like it should.
      // Color codes within strings will still override the default.
      V_WriteTextColoured(msg, mess_colour, x, y);
   }
}

//
// HU_MessageErase
//
// Erases the player message widget.
//
static void HU_MessageErase(hu_widget_t *widget)
{
   // haleyjd 06/04/05: added one to account for chat
   R_VideoErase(0, 0, SCREENWIDTH, 8 * (hud_msg_lines + 1));
}

//
// HU_MessageClear
//
// Sets the number of messages being displayed to zero.
//
static void HU_MessageClear(hu_widget_t *widget)
{
   ((hu_msgwidget_t *)widget)->current_messages = 0;
}

//
// HU_InitMsgWidget
//
// Called from HU_InitNativeWidgets. Sets up the player message widget.
//
static void HU_InitMsgWidget(void)
{
   // set up the object id
   strcpy(msg_widget.widget.name, "_HU_MsgWidget");

   msg_widget.widget.type = WIDGET_MISC;

   // set up object virtuals
   msg_widget.widget.ticker = HU_MessageTick;
   msg_widget.widget.drawer = HU_MessageDraw;
   msg_widget.widget.eraser = HU_MessageErase;
   msg_widget.widget.clear  = HU_MessageClear;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&msg_widget);
}

//
// Global functions that manipulate the message widget
//

//
// HU_PlayerMsg
//
// Adds a new message to the player.
//
void HU_PlayerMsg(const char *s)
{
   if(msg_widget.current_messages == hud_msg_lines)  // display full
   {
      int i;
      
      // scroll up      
      for(i = 0; i < hud_msg_lines - 1; ++i)
         strncpy(msg_widget.messages[i], msg_widget.messages[i+1], MAXHUDMSGLEN);
      
      strncpy(msg_widget.messages[hud_msg_lines - 1], s, MAXHUDMSGLEN);
   }
   else            // add one to the end
   {
      strncpy(msg_widget.messages[msg_widget.current_messages], s, MAXHUDMSGLEN);
      msg_widget.current_messages++;
   }
   
   msg_widget.scrolltime = leveltime + (message_timer * 35) / 1000;
}

//
// Patch Widget: this type of widget just displays a screen patch.
//

typedef struct hu_patchwidget_s
{
   hu_widget_t widget; // parent widget

   int x, y;           // screen location
   char *color;        // color range translation to use
   int tl_level;       // translucency level
   patch_t *patch;     // screen patch
} hu_patchwidget_t;

//
// HU_PatchWidgetDraw
//
// Default drawing function for patch widgets. All the logic for
// deciding whether the patch is translucent and/or translated is
// already implemented in V_DrawPatchTL via the generalized patch
// drawing system.
//
static void HU_PatchWidgetDraw(hu_widget_t *widget)
{
   hu_patchwidget_t *pw = (hu_patchwidget_t *)widget;

   if(!pw->patch)
      return;

   V_DrawPatchTL(pw->x, pw->y, &vbscreen, pw->patch, pw->color, pw->tl_level);
}

//
// HU_PatchWidgetErase
//
// Default erase function for patch widgets.
//
static void HU_PatchWidgetErase(hu_widget_t *widget)
{
   unsigned int x, y;
   hu_patchwidget_t *pw = (hu_patchwidget_t *)widget;
   patch_t *patch = pw->patch;

   if(patch && pw->tl_level != 0)
   {
      // haleyjd 06/08/05: must adjust for patch offsets
      x = pw->x - SHORT(patch->leftoffset);
      y = pw->y - SHORT(patch->topoffset);
      
      R_VideoErase(x, y, SHORT(patch->width), SHORT(patch->height));
   }
}

//
// HU_PatchWidgetDefaults
//
// Sets up default handler functions for a patch widget.
//
static void HU_PatchWidgetDefaults(hu_patchwidget_t *pw)
{
   pw->widget.drawer = HU_PatchWidgetDraw;
   pw->widget.eraser = HU_PatchWidgetErase;
   pw->widget.type   = WIDGET_PATCH;
}

//
// HU_DynamicPatchWidget
//
// Adds a dynamically allocated patch widget to the hash table.
// For scripting.
//
static void HU_DynamicPatchWidget(char *name, int x, int y, int color,
                                  int tl_level, char *patch)
{
   hu_patchwidget_t *newpw = malloc(sizeof(hu_patchwidget_t));

   memset(newpw, 0, sizeof(hu_patchwidget_t));

   // set id
   strncpy(newpw->widget.name, name, 33);

   // add to hash
   if(!HU_AddWidgetToHash((hu_widget_t *)newpw))
   {
      // if addition was unsuccessful, we delete it now
      free(newpw);
      return;
   }

   // set virtuals
   HU_PatchWidgetDefaults(newpw);

   // set properties
   newpw->x = x;
   newpw->y = y;
   if(color >= 0 && color < CR_LIMIT)
      newpw->color = colrngs[color];
   newpw->tl_level = tl_level;

   // load patch
   newpw->patch = W_CacheLumpName(patch, PU_STATIC);
}

//
// Crosshair Widget
//

static hu_patchwidget_t crosshair_widget;

// globals related to the crosshair
patch_t *crosshairs[CROSSHAIRS];
char *targetcolour, *notargetcolour, *friendcolour;
int crosshairnum;       // 0 = none
boolean crosshair_hilite; // haleyjd 06/07/05
char *cross_str[]= { "none", "cross", "angle" }; // for console

//
// HU_CrossHairTick
//
// Updates the crosshair on each gametic.
//
static void HU_CrossHairTick(hu_widget_t *widget)
{
   hu_patchwidget_t *crosshair = (hu_patchwidget_t *)widget;

   // default to no target
   crosshair->color = notargetcolour;

   // fast as possible: don't bother with this crap if the crosshair 
   // isn't going to be displayed anyway   
   if(!crosshair->patch || !crosshairnum || !crosshair_hilite)
      return;

   // search for targets
   
   P_AimLineAttack(players[displayplayer].mo,
                   players[displayplayer].mo->angle, 
                   16*64*FRACUNIT, 0);

   if(linetarget)
   {
      // target found
      crosshair->color = 
         (linetarget->flags & MF_FRIEND) ? friendcolour : targetcolour;
   }
}

//
// HU_CrossHairDraw
//
// Draws the crosshair patch.
//
static void HU_CrossHairDraw(hu_widget_t *widget)
{
   int drawx, drawy, h, w;
   hu_patchwidget_t *crosshair = (hu_patchwidget_t *)widget;
   patch_t *patch = crosshair->patch;
   char *pal = crosshair->color;
   
   if(!patch || viewcamera || automapactive)
      return;
  
   // where to draw??

   w = SHORT(patch->width);
   h = SHORT(patch->height);
   
   drawx = (SCREENWIDTH - w) / 2;

   // haleyjd 04/09/05: this kludge moves the crosshair to within
   // a tolerable distance of the player's true vertical aim when
   // the screen size is less than full.
   if(scaledviewheight != SCREENHEIGHT)
   {
      // use 1/5 of the displayplayer's pitch angle in integer degrees
      int angle = players[displayplayer].pitch / (ANGLE_1*5);
      drawy = scaledwindowy + (scaledviewheight - h) / 2 + angle;
   }
   else
      drawy = scaledwindowy + (scaledviewheight - h) / 2;
  
   if(pal == notargetcolour)
      V_DrawPatchTL(drawx, drawy, &vbscreen, patch, pal, FTRANLEVEL);
   else
      V_DrawPatchTranslated(drawx, drawy, &vbscreen, patch, pal, false);
}

//
// HU_InitCrossHair
//
// Sets up the crosshair widget and associated globals.
//
void HU_InitCrossHair(void)
{
   // haleyjd TODO: support user-added crosshairs
   crosshairs[0] = W_CacheLumpName("CROSS1", PU_STATIC);
   crosshairs[1] = W_CacheLumpName("CROSS2", PU_STATIC);
   
   notargetcolour = cr_red;
   targetcolour = cr_green;
   friendcolour = cr_blue;

   // set up widget object
   crosshair_widget.color = notargetcolour;
   crosshair_widget.patch = crosshairnum ? crosshairs[crosshairnum-1] : NULL;

   // set up the object id
   strcpy(crosshair_widget.widget.name, "_HU_CrosshairWidget");

   // set type
   crosshair_widget.widget.type = WIDGET_PATCH;

   // set up object virtuals
   crosshair_widget.widget.ticker = HU_CrossHairTick;
   crosshair_widget.widget.drawer = HU_CrossHairDraw;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&crosshair_widget);
}

//
// Pop-up Warning Boxes
//
// Several different things that appear, quake-style, to warn you of
// problems.
//

// Open Socket Warning
//
// Problem with network leads or something like that

static hu_patchwidget_t opensocket_widget;

// VPO Warning indicator
//
// most ports nowadays have removed the visplane overflow problem.
// however, many developers still make wads for plain vanilla doom.
// this should give them a warning for when they have 'a few
// planes too many'

static hu_patchwidget_t vpo_widget;

// globals needed for VPO widget
extern int num_visplanes;
int show_vpo = 0;
int vpo_threshold; // haleyjd 09/29/04: customizable VPO threshold

//
// HU_WarningsDrawer
//
// Draws the warnings widgets. Calls down to the default drawer for
// patches when the proper conditions are met. Normally I'd set the
// conditions via a ticker but for opensocket that won't work if the
// game is frozen.
//
static void HU_WarningsDrawer(hu_widget_t *widget)
{
   hu_patchwidget_t *pw = (hu_patchwidget_t *)widget;

   if(pw == &vpo_widget)
   {
      if(show_vpo && num_visplanes > vpo_threshold)
         HU_PatchWidgetDraw(widget);
   }
   else if(pw == &opensocket_widget)
   {
      if(opensocket)
         HU_PatchWidgetDraw(widget);
   }
}

//
// HU_InitWarnings
//
// Sets up the VPO and open socket warning patch widgets.
//
static void HU_InitWarnings(void)
{
   // set up VPO
   strcpy(vpo_widget.widget.name, "_HU_VPOWidget");

   vpo_widget.widget.type = WIDGET_PATCH;

   vpo_widget.widget.drawer = HU_WarningsDrawer;
   vpo_widget.widget.eraser = HU_PatchWidgetErase;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&vpo_widget);

   vpo_widget.patch = W_CacheLumpName("VPO", PU_STATIC);
   vpo_widget.color = NULL;
   vpo_widget.tl_level = FRACUNIT;
   vpo_widget.x = 250;
   vpo_widget.y = 10;
   
   // set up socket
   strcpy(opensocket_widget.widget.name, "_HU_OpenSocketWidget");

   opensocket_widget.widget.type = WIDGET_PATCH;

   opensocket_widget.widget.drawer = HU_WarningsDrawer;
   opensocket_widget.widget.eraser = HU_PatchWidgetErase;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&opensocket_widget);
   
   opensocket_widget.patch = W_CacheLumpName("OPENSOCK", PU_STATIC);
   opensocket_widget.color = NULL;
   opensocket_widget.tl_level = FRACUNIT;
   opensocket_widget.x = 20;
   opensocket_widget.y = 20;
}

//
// Text Widgets
//

// erase data rect
typedef struct tw_erase_s
{
   int x1, y1, x2, y2;
} tw_erase_t;

// text widget flag values
enum
{
   TW_AUTOMAP_ONLY = 0x00000001, // appears in automap only
   TW_NOCLEAR      = 0x00000002, // dynamic widget with no clear func
};

typedef struct hu_textwidget_s
{
   hu_widget_t widget; // parent widget
   
   int x, y;             // coords on screen
   int font;             // 0 = normal, 1 = heads up font
   char *message;        // text to draw
   char *alloc;          // if non-NULL, widget owns the message
   int cleartic;         // gametic in which to clear the widget (0=never)
   tw_erase_t erasedata; // rect area to erase
   int flags;            // special flags
} hu_textwidget_t;

//
// HU_TextWidgetDraw
//
// Default drawing function for a text widget.
//
static void HU_TextWidgetDraw(hu_widget_t *widget)
{
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;

   // Do not ever draw automap-only widgets if not in automap mode.
   // This fixes a long-standing bug automatically.
   if(tw->flags & TW_AUTOMAP_ONLY && !automapactive)
      return;

   // haleyjd TODO: better font support
   if(tw->message && (!tw->cleartic || leveltime < tw->cleartic))
   {
      (tw->font ? HU_WriteText : V_WriteText)(tw->message, tw->x, tw->y);
   }
}

//
// HU_ClearEraseData
//
// haleyjd 04/10/05: Sets the erase area data for a text widget to its 
// initial state.  This is called after the erase area has been wiped, 
// so that the area does not continue to grow in size indefinitely and 
// take up the entire screen.
//
d_inline static void HU_ClearEraseData(hu_textwidget_t *tw)
{
   tw->erasedata.x1 = tw->erasedata.y1 =  D_MAXINT;
   tw->erasedata.x2 = tw->erasedata.y2 = -D_MAXINT;
}

//
// HU_UpdateEraseData
//
// haleyjd 04/10/05: Updates a text widget's erase data structure
// by expanding the rect to include the boundaries of a new message. The
// erase area must expand until an erasure actually occurs.
//
static void HU_UpdateEraseData(hu_textwidget_t *tw)
{
   int w, h;

   if(!tw->message)
      return;

   // haleyjd TODO: better font support
   w = (tw->font ? HU_StringWidth : V_StringWidth)(tw->message);
   h = (tw->font ? HU_StringHeight : V_StringHeight)(tw->message);

   if(tw->x < tw->erasedata.x1)
      tw->erasedata.x1 = tw->x;
   if(tw->y < tw->erasedata.y1)
      tw->erasedata.y1 = tw->y;
   if(tw->x + w > tw->erasedata.x2)
      tw->erasedata.x2 = tw->x + w;
   if(tw->y + h > tw->erasedata.y2)
      tw->erasedata.y2 = tw->y + h;
}

//
// HU_TextWidgetErase
//
// haleyjd 04/10/05: Default function for erasing HUD text widgets. 
//
static void HU_TextWidgetErase(hu_widget_t *widget)
{
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;

   // is erasedata "empty" ?
   if(tw->erasedata.x1 == D_MAXINT)
      return;

   // erase the largest rect the message widget has occupied since the
   // last time it was cleared
   R_VideoErase(tw->erasedata.x1,  tw->erasedata.y1, 
                tw->erasedata.x2 - tw->erasedata.x1, 
                tw->erasedata.y2 - tw->erasedata.y1);

   // do not keep erasing larger and larger portions of the screen when 
   // it is totally unnecessary.
   if(!tw->message || (tw->cleartic != 0 && leveltime > tw->cleartic + 1))
      HU_ClearEraseData(tw);
}

//
// HU_TextWidgetClear
//
// Default clear function for a text widget.
//
static void HU_TextWidgetClear(hu_widget_t *widget)
{
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;

   // if this widget owns its message, free it
   if(tw->alloc)
   {
      free(tw->alloc);
      tw->alloc = NULL;
   }
   
   tw->message = NULL;
}

//
// HU_TextWidgetDefaults
//
// Sets up default values for a normal text widget.
//
static void HU_TextWidgetDefaults(hu_textwidget_t *tw)
{
   HU_ClearEraseData(tw);

   tw->widget.drawer = HU_TextWidgetDraw;
   tw->widget.eraser = HU_TextWidgetErase;
   tw->widget.clear  = HU_TextWidgetClear;
   tw->widget.type   = WIDGET_TEXT;
}

//
// HU_DynAutomapTick
//
// A suitable ticker for dynamic automap-only text widgets.
//
static void HU_DynAutomapTick(hu_widget_t *widget)
{
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;

   tw->message = automapactive ? tw->alloc : NULL;
}

//
// HU_DynamicTextWidget
//
// Adds a dynamically allocated text widget to the hash table.
// For scripting.
// 
static void HU_DynamicTextWidget(const char *name, int x, int y, int font,
                                 char *message, int cleartic, int flags)
{
   hu_textwidget_t *newtw = malloc(sizeof(hu_textwidget_t));

   memset(newtw, 0, sizeof(hu_textwidget_t));

   // set id
   strncpy(newtw->widget.name, name, 33);

   // add to hash
   if(!HU_AddWidgetToHash((hu_widget_t *)newtw))
   {
      // if addition was unsuccessful, we delete it now
      free(newtw);
      return;
   }

   // set virtuals
   HU_TextWidgetDefaults(newtw);

   // for automap-only widgets, add a ticker
   if(flags & TW_AUTOMAP_ONLY)
      newtw->widget.ticker = HU_DynAutomapTick;

   // if no clear, remove the clear func
   if(flags & TW_NOCLEAR)
      newtw->widget.clear = NULL;

   // set properties
   newtw->x = x;
   newtw->y = y;
   newtw->font = font;
   newtw->cleartic = cleartic;
   newtw->flags = flags;

   // set message
   newtw->message = newtw->alloc = strdup(message);

   HU_UpdateEraseData(newtw);
}

//
// Center-of-screen, Quake-style Message
//

static hu_textwidget_t centermessage_widget;

//
// HU_InitCenterMessage
//
// Sets up the center message widget.
//
static void HU_InitCenterMessage(void)
{
   // set id
   strcpy(centermessage_widget.widget.name, "_HU_CenterMsgWidget");

   // set virtuals
   HU_TextWidgetDefaults(&centermessage_widget);

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&centermessage_widget);

   // set data
   centermessage_widget.x = 0;
   centermessage_widget.y = 0;
   centermessage_widget.message = NULL;
   centermessage_widget.font = 0;
   centermessage_widget.cleartic = 0;
}

//
// HU_CenterMessage
//
// haleyjd 04/27/04: rewritten to use qstring
//
void HU_CenterMessage(const char *s)
{
   static qstring_t qstr;
   static boolean first = true;  
   int st_height = gameModeInfo->StatusBar->height;
   hu_textwidget_t *tw = &centermessage_widget;

   if(first)
   {
      M_QStrCreate(&qstr);
      first = false;
   }
   else
      M_QStrClear(&qstr);
   
   M_QStrCat(&qstr, s);
  
   tw->message = M_QStrBuffer(&qstr);
   tw->x = (SCREENWIDTH-V_StringWidth(s)) / 2;
   tw->y = (SCREENHEIGHT-V_StringHeight(s) -
            ((scaledviewheight==SCREENHEIGHT) ? 0 : st_height-8)) / 2;
   tw->cleartic = leveltime + (message_timer * 35) / 1000;

   HU_UpdateEraseData(tw);
   
   // print message to console also
   C_Printf("%s\n", s);
}

//
// HU_CenterMessageTimed
//
// haleyjd: timed center message. Originally for FraggleScript,
// now revived for Small.
//
void HU_CenterMessageTimed(const char *s, int tics)
{
   HU_CenterMessage(s);
   centermessage_widget.cleartic = leveltime + tics;
}

//
// Elapsed level time (automap)
//

static hu_textwidget_t leveltime_widget;

//
// HU_LevelTimeTick
//
// Updates the automap level timer.
//
static void HU_LevelTimeTick(hu_widget_t *widget)
{
   static char timestr[32];
   int seconds;
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;
   
   if(!automapactive)
   {
      tw->message = NULL;
      return;
   }
   
   seconds = levelTime / 35;
   timestr[0] = '\0';
   
   psnprintf(timestr, sizeof(timestr), "%02i:%02i:%02i", 
             seconds/3600, (seconds%3600)/60, seconds%60);
   
   tw->message = timestr;        
}

//
// HU_InitLevelTime
//
// Sets up the level time widget for the automap.
//
static void HU_InitLevelTime(void)
{
   // set id
   strcpy(leveltime_widget.widget.name, "_HU_LevelTimeWidget");

   leveltime_widget.widget.type = WIDGET_TEXT;

   // set virtuals
   leveltime_widget.widget.drawer = HU_TextWidgetDraw;
   leveltime_widget.widget.ticker = HU_LevelTimeTick;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&leveltime_widget);

   // set data
   if(gameModeInfo->type == Game_Heretic)
   {
      leveltime_widget.x = 240;
      leveltime_widget.y = 10;
   }
   else
   {
      leveltime_widget.x = SCREENWIDTH - 60;
      leveltime_widget.y = SCREENHEIGHT - ST_HEIGHT - 8;
   }
   leveltime_widget.message = NULL;
   leveltime_widget.font = 0;
   leveltime_widget.cleartic = 0;
   leveltime_widget.flags = TW_AUTOMAP_ONLY;
}

//
// Automap level name display
//

static hu_textwidget_t levelname_widget;

//
// HU_LevelNameTick
//
// Updates the automap level name widget.
//
static void HU_LevelNameTick(hu_widget_t *widget)
{
   ((hu_textwidget_t *)widget)->message = 
      automapactive ? LevelInfo.levelName : NULL;
}

//
// HU_InitLevelName
//
// Sets up the level name widget for the automap.
//
static void HU_InitLevelName(void)
{
   // set id
   strcpy(levelname_widget.widget.name, "_HU_LevelNameWidget");

   levelname_widget.widget.type = WIDGET_TEXT;

   // set virtuals
   levelname_widget.widget.drawer = HU_TextWidgetDraw;
   levelname_widget.widget.ticker = HU_LevelNameTick;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&levelname_widget);

   // set data
   if(gameModeInfo->type == Game_Heretic)
   {
      levelname_widget.x = 20;
      levelname_widget.y = 145;
   }
   else
   {
      levelname_widget.x = 0;
      levelname_widget.y = SCREENHEIGHT - ST_HEIGHT - 8;
   }
   levelname_widget.message = NULL;
   levelname_widget.font = 0;
   levelname_widget.cleartic = 0;
   levelname_widget.flags = TW_AUTOMAP_ONLY;
}

//
// Chat message display
//

static hu_textwidget_t chat_widget;

char chatinput[100] = "";

//
// HU_ChatTick
//
// Updates the chat message widget.
//
static void HU_ChatTick(hu_widget_t *widget)
{
   static char tempchatmsg[128];
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;
   
   if(chat_active)
   {
      psnprintf(tempchatmsg, sizeof(tempchatmsg), "%s_", chatinput);
      tw->message = tempchatmsg;
      HU_UpdateEraseData(tw);
   }
   else
      tw->message = NULL;
}

//
// HU_InitChat
//
// Sets up the chat message widget.
//
static void HU_InitChat(void)
{
   // set id
   strcpy(chat_widget.widget.name, "_HU_ChatWidget");

   // set virtuals
   HU_TextWidgetDefaults(&chat_widget);
   
   // overrides
   chat_widget.widget.ticker = HU_ChatTick;
   chat_widget.widget.clear  = NULL;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&chat_widget);

   // set data
   chat_widget.x = 0;
   chat_widget.y = 0;
   chat_widget.message = NULL;
   chat_widget.font = 0;
   chat_widget.cleartic = 0;
}

//
// HU_ChatRespond
//
// Responds to chat-related events.
//
static boolean HU_ChatRespond(event_t *ev)
{
   char ch;
   static boolean shiftdown;
   
   if(ev->data1 == KEYD_RSHIFT) 
      shiftdown = (ev->type == ev_keydown);
   
   if(ev->type != ev_keydown)
      return false;
   
   if(!chat_active)
   {
      if(ev->data1 == key_chat && netgame) 
      {       
         chat_active = true; // activate chat
         chatinput[0] = 0;   // empty input string
         return true;
      }
      return false;
   }
  
   if(altdown && ev->type == ev_keydown &&
      ev->data1 >= '0' && ev->data1 <= '9')
   {
      // chat macro
      char tempstr[100];
      psnprintf(tempstr, sizeof(tempstr),
                "say \"%s\"", chat_macros[ev->data1-'0']);
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
      psnprintf(tempstr, sizeof(tempstr), "say \"%s\"", chatinput);
      C_RunTextCmd(tempstr);
      chat_active = false;
      return true;
   }

   ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?
   
   if(ch > 31 && ch < 127)
   {
      psnprintf(chatinput, sizeof(chatinput), "%s%c", chatinput, ch);
      C_InitTab();
      return true;
   }
   return false;
}

//
// Automap coordinate display
//
//   Yet Another Lost MBF Feature
//   Restored by Quasar (tm)
//

static hu_textwidget_t coordx_widget;
static hu_textwidget_t coordy_widget;
static hu_textwidget_t coordz_widget;

//
// HU_CoordTick
//
// Updates automap coordinate widgets.
//
static void HU_CoordTick(hu_widget_t *widget)
{
   player_t *plyr;
   fixed_t x, y, z;
   hu_textwidget_t *tw = (hu_textwidget_t *)widget;
   
   // haleyjd: wow, big bug here -- these buffers were not static
   // and thus corruption was occuring when the function returned
   static char coordxstr[16];
   static char coordystr[16];
   static char coordzstr[16];

   if(!automapactive)
   {
      tw->message = NULL;
      return;
   }
   plyr = &players[displayplayer];

   AM_Coordinates(plyr->mo, &x, &y, &z);

   if(tw == &coordx_widget)
   {
      sprintf(coordxstr, "X: %-5d", x >> FRACBITS);
      tw->message = coordxstr;
   }
   else if(tw == &coordy_widget)
   {
      sprintf(coordystr, "Y: %-5d", y >> FRACBITS);
      tw->message = coordystr;
   }
   else
   {
      sprintf(coordzstr, "Z: %-5d", z >> FRACBITS);
      tw->message = coordzstr;
   }
}

//
// HU_InitCoords
//
// Initializes the automap coordinate widgets.
//   
static void HU_InitCoords(void)
{
   // set ids
   strcpy(coordx_widget.widget.name, "_HU_CoordXWidget");
   strcpy(coordy_widget.widget.name, "_HU_CoordYWidget");
   strcpy(coordz_widget.widget.name, "_HU_CoordZWidget");

   coordx_widget.widget.type =
      coordy_widget.widget.type =
      coordz_widget.widget.type = WIDGET_TEXT;

   // set virtuals
   coordx_widget.widget.drawer = HU_TextWidgetDraw;
   coordy_widget.widget.drawer = HU_TextWidgetDraw;
   coordz_widget.widget.drawer = HU_TextWidgetDraw;
   coordx_widget.widget.ticker = HU_CoordTick;
   coordy_widget.widget.ticker = HU_CoordTick;
   coordz_widget.widget.ticker = HU_CoordTick;

   // add to hash
   HU_AddWidgetToHash((hu_widget_t *)&coordx_widget);
   HU_AddWidgetToHash((hu_widget_t *)&coordy_widget);
   HU_AddWidgetToHash((hu_widget_t *)&coordz_widget);

   // set data
   if(gameModeInfo->type == Game_Heretic)
   {
      coordx_widget.x = coordy_widget.x = coordz_widget.x = 20;
      coordx_widget.y = 10;
      coordy_widget.y = 19;
      coordz_widget.y = 28;
   }
   else
   {
      coordx_widget.x = coordy_widget.x = coordz_widget.x = SCREENWIDTH - 64;
      coordx_widget.y = 8;
      coordy_widget.y = 17;
      coordz_widget.y = 25;
   }
   coordx_widget.message = coordy_widget.message = coordz_widget.message = NULL;
   coordx_widget.font = coordy_widget.font = coordz_widget.font = 0;
   coordx_widget.cleartic = coordy_widget.cleartic = coordz_widget.cleartic = 0;
   coordx_widget.flags = coordy_widget.flags = coordz_widget.flags = TW_AUTOMAP_ONLY;
}

////////////////////////////////////////////////////////////////////////
//
// Tables
//

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

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

VARIABLE_BOOLEAN(showMessages,  NULL,                   onoff);
VARIABLE_INT(mess_colour,       NULL, 0, CR_LIMIT-1,    textcolours);

VARIABLE_BOOLEAN(obituaries,    NULL,                   onoff);
VARIABLE_INT(obcolour,          NULL, 0, CR_LIMIT-1,    textcolours);
VARIABLE_INT(crosshairnum,      NULL, 0, CROSSHAIRS-1,  cross_str);
VARIABLE_BOOLEAN(show_vpo,      NULL,                   yesno);
VARIABLE_INT(vpo_threshold,     NULL, 1, 128,      NULL);
VARIABLE_INT(hud_msg_lines,     NULL, 0, 14,            NULL);
VARIABLE_INT(message_timer,     NULL, 0, 100000,        NULL);

VARIABLE_BOOLEAN(hud_msg_scrollup,  NULL,               yesno);
VARIABLE_BOOLEAN(crosshair_hilite,  NULL,               onoff);

CONSOLE_VARIABLE(obituaries, obituaries, 0) {}
CONSOLE_VARIABLE(obcolour, obcolour, 0) {}
CONSOLE_VARIABLE(crosshair, crosshairnum, 0)
{
   int a;
   
   a = atoi(c_argv[0]);
   
   crosshair_widget.patch = a ? crosshairs[a - 1] : NULL;
   crosshairnum = a;
}
CONSOLE_VARIABLE(crosshair_hilite, crosshair_hilite, 0) {}
CONSOLE_VARIABLE(show_vpo, show_vpo, 0) {}
CONSOLE_VARIABLE(vpo_threshold, vpo_threshold, 0) {}
CONSOLE_VARIABLE(messages, showMessages, 0) {}
CONSOLE_VARIABLE(mess_colour, mess_colour, 0) {}
CONSOLE_NETCMD(say, cf_netvar, netcmd_chat)
{
   S_StartSound(NULL, gameModeInfo->c_ChatSound);
   
   doom_printf("%s: %s", players[cmdsrc].name, c_args);
}

CONSOLE_VARIABLE(mess_lines, hud_msg_lines, 0) {}
CONSOLE_VARIABLE(mess_scrollup, hud_msg_scrollup, 0) {}
CONSOLE_VARIABLE(mess_timer, message_timer, 0) {}

extern void HU_FragsAddCommands(void);
extern void HU_OverAddCommands(void);

void HU_AddCommands(void)
{
   C_AddCommand(obituaries);
   C_AddCommand(obcolour);
   C_AddCommand(crosshair);
   C_AddCommand(crosshair_hilite);
   C_AddCommand(show_vpo);
   C_AddCommand(messages);
   C_AddCommand(mess_colour);
   C_AddCommand(say);
   
   C_AddCommand(mess_lines);
   C_AddCommand(mess_scrollup);
   C_AddCommand(mess_timer);

   C_AddCommand(vpo_threshold);
   
   HU_FragsAddCommands();
   HU_OverAddCommands();
}

//
// Script functions
//

static cell AMX_NATIVE_CALL sm_movewidget(AMX *amx, cell *params)
{
   int err;
   char *name;
   hu_widget_t *widget;

   // get name of widget
   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   if(!(widget = HU_WidgetForName(name)))
   {
      free(name);
      return 0;
   }

   switch(widget->type)
   {
   case WIDGET_TEXT:
      {
         hu_textwidget_t *tw = (hu_textwidget_t *)widget;

         tw->x = params[2];
         tw->y = params[3];

         HU_UpdateEraseData(tw);
      }
      break;
   case WIDGET_PATCH:
      {
         hu_patchwidget_t *pw = (hu_patchwidget_t *)widget;

         pw->x = params[2];
         pw->y = params[3];
      }
      break;
   default:
      break;
   }

   free(name);
   return 0;
}

static cell AMX_NATIVE_CALL sm_newpatchwidget(AMX *amx, cell *params)
{
   int err;
   char *name, *patch;

   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }
   if((err = A_GetSmallString(amx, &patch, params[2])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      free(name);
      return -1;
   }
   
   HU_DynamicPatchWidget(name, params[3], params[4], params[5], params[6], patch);

   free(name);
   free(patch);

   return 0;
}

static cell AMX_NATIVE_CALL sm_setwidgetpatch(AMX *amx, cell *params)
{
   int err;
   char *name, *patch;
   hu_widget_t *widget;
   hu_patchwidget_t *pw;

   // get name of widget
   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   // get name of patch
   if((err = A_GetSmallString(amx, &patch, params[2])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      free(name);
      return -1;
   }

   if((widget = HU_WidgetForName(name)) && widget->type == WIDGET_PATCH)
   {
      pw = (hu_patchwidget_t *)widget;

      pw->patch = W_CacheLumpName(patch, PU_STATIC);
   }

   free(name);
   free(patch);

   return 0;
}

static cell AMX_NATIVE_CALL sm_patchwidgetcolor(AMX *amx, cell *params)
{
   int err;
   char *name;
   hu_widget_t *widget;
   hu_patchwidget_t *pw;

   // get name of widget
   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   if((widget = HU_WidgetForName(name)) && widget->type == WIDGET_PATCH)
   {
      pw = (hu_patchwidget_t *)widget;

      if(params[2] != -2)
         pw->color = params[2] >= 0 && params[2] < CR_LIMIT ? colrngs[params[1]] : NULL;

      if(params[3] >= 0 && params[3] <= FRACUNIT)
         pw->tl_level = params[3];
   }

   free(name);

   return 0;
}

static cell AMX_NATIVE_CALL sm_newtextwidget(AMX *amx, cell *params)
{
   int err;
   char *name, *msg;

   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }
   if((err = A_GetSmallString(amx, &msg, params[2])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      free(name);
      return -1;
   }

   HU_DynamicTextWidget(name, params[3], params[4], params[5], msg,
                        leveltime + params[6], params[7]);

   free(name);
   free(msg);

   return 0;
}

static cell AMX_NATIVE_CALL sm_getwidgettext(AMX *amx, cell *params)
{
   int err, size, packed;
   cell *deststr;
   char *name;
   hu_widget_t *widget;
   hu_textwidget_t *tw;

   size   = (int)params[3];
   packed = (int)params[4];
   
   // resolve address of destination buffer
   if((err = amx_GetAddr(amx, params[2], &deststr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   // get name of widget
   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   if((widget = HU_WidgetForName(name)) && widget->type == WIDGET_TEXT)
   {
      char *tempbuf = malloc(size+1);
      
      tw = (hu_textwidget_t *)widget;

      if(tw->message)
         psnprintf(tempbuf, size+1, "%s", tw->message);
      else
         tempbuf[0] = '\0';
      
      amx_SetString(deststr, tempbuf, packed, 0);

      free(tempbuf);
   }

   free(name);

   return 0;
}

static cell AMX_NATIVE_CALL sm_setwidgettext(AMX *amx, cell *params)
{
   int err;
   char *name, *value;
   hu_widget_t *widget;
   hu_textwidget_t *tw;

   // get name of widget
   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   // get value
   if((err = A_GetSmallString(amx, &value, params[2])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      free(name);
      return -1;
   }

   if((widget = HU_WidgetForName(name)) && widget->type == WIDGET_TEXT)
   {
      tw = (hu_textwidget_t *)widget;

      if(tw->alloc)
         free(tw->alloc);

      tw->message = tw->alloc = strdup(value);

      tw->cleartic = leveltime + params[3];

      HU_UpdateEraseData(tw);
   }

   free(name);
   free(value);

   return 0;
}

static cell AMX_NATIVE_CALL sm_togglewidget(AMX *amx, cell *params)
{
   int err;
   char *name;
   hu_widget_t *widget;

   if((err = A_GetSmallString(amx, &name, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   if((widget = HU_WidgetForName(name)))
      HU_ToggleWidget(widget, params[2]);

   free(name);

   return 0;
}

static cell AMX_NATIVE_CALL sm_centermsgtimed(AMX *amx, cell *params)
{
   int tics, err;
   char *text;

   if((err = A_GetSmallString(amx, &text, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   tics = params[2];

   HU_CenterMessageTimed(text, tics);

   free(text);

   return 0;
}

AMX_NATIVE_INFO hustuff_Natives[] =
{
   { "_MoveWidget",       sm_movewidget       },
   { "_NewPatchWidget",   sm_newpatchwidget   },
   { "_SetWidgetPatch",   sm_setwidgetpatch   },
   { "_PatchWidgetColor", sm_patchwidgetcolor },
   { "_NewTextWidget",    sm_newtextwidget    },
   { "_GetWidgetText",    sm_getwidgettext    },
   { "_SetWidgetText",    sm_setwidgettext    },
   { "_ToggleWidget",     sm_togglewidget     },
   { "_CenterMsgTimed",   sm_centermsgtimed   },
   { NULL, NULL }
};

// EOF
