/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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

// killough 5/3/98: remove unnecessary headers

#include "doomstat.h"
#include "hu_stuff.h"
#include "hu_lib.h"
#include "hu_tracers.h"
#include "st_stuff.h" /* jff 2/16/98 need loc of status bar */
#include "s_sound.h"
#include "dstrings.h"
#include "sounds.h"
#include "d_deh.h"   /* Ty 03/27/98 - externalization of mapnamesx arrays */
#include "g_game.h"
#include "r_main.h"
#include "p_inter.h"
#include "p_tick.h"
#include "p_map.h"
#include "sc_man.h"
#include "m_misc.h"
#include "r_main.h"
#include "lprintf.h"
#include "e6y.h" //e6y
#include "g_overflow.h"

// global heads up display controls

int hud_displayed;    //jff 2/23/98 turns heads-up display on/off
int hud_num;

//
// Locally used constants, shortcuts.
//
// Ty 03/28/98 -
// These four shortcuts modifed to reflect char ** of mapnamesx[]
// e6y: why sizeof(mapnamest)/sizeof(mapnamest[0]) does not work?
#define HU_TITLE  (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (gamemap <= 33 ? *mapnames2[gamemap-1] : "")
#define HU_TITLEP (gamemap <= 32 ? *mapnamesp[gamemap-1] : "")
#define HU_TITLET (gamemap <= 32 ? *mapnamest[gamemap-1] : "")
#define HU_TITLEC (*mapnames[gamemap-1])
#define HU_TITLEX 0
//jff 2/16/98 change 167 to ST_Y-1
// CPhipps - changed to ST_TY
// proff - changed to 200-ST_HEIGHT for stretching
#define HU_TITLEY ((200-ST_HEIGHT) - 1 - hu_font[0].height)

//jff 2/16/98 add coord text widget coordinates
// proff - changed to SCREENWIDTH to 320 for stretching
#define HU_COORDX (320 - 13*hu_font2['A'-HU_FONTSTART].width)
//jff 3/3/98 split coord widget into three lines in upper right of screen
#define HU_COORDXYZ_Y (1 * hu_font['A'-HU_FONTSTART].height + 1)
#define HU_COORDX_Y (0 + 0*hu_font['A'-HU_FONTSTART].height + HU_COORDXYZ_Y)
#define HU_COORDY_Y (1 + 1*hu_font['A'-HU_FONTSTART].height + HU_COORDXYZ_Y)
#define HU_COORDZ_Y (2 + 2*hu_font['A'-HU_FONTSTART].height + HU_COORDXYZ_Y)

#define HU_MAP_STAT_X (0)
#define HU_MAP_STAT_Y (1 * hu_font['A'-HU_FONTSTART].height + 1)
#define HU_MAP_MONSTERS_Y  (0 + 0*hu_font['A'-HU_FONTSTART].height + HU_MAP_STAT_Y)
#define HU_MAP_SECRETS_Y   (1 + 1*hu_font['A'-HU_FONTSTART].height + HU_MAP_STAT_Y)
#define HU_MAP_ITEMS_Y     (2 + 2*hu_font['A'-HU_FONTSTART].height + HU_MAP_STAT_Y)
#define HU_MAP_TIME_Y      (4 + 4*hu_font['A'-HU_FONTSTART].height + HU_MAP_STAT_Y)
#define HU_MAP_TOTALTIME_Y (5 + 5*hu_font['A'-HU_FONTSTART].height + HU_MAP_STAT_Y)

//jff 2/16/98 add ammo, health, armor widgets, 2/22/98 less gap
#define HU_GAPY 8

#define HU_INPUTX HU_MSGX
#define HU_INPUTY (HU_MSGY + HU_MSGHEIGHT*(hu_font[0].height) +1)

#define HU_TRACERX (2)
#define HU_TRACERY (hu_font['A'-HU_FONTSTART].height)

#define key_alt KEYD_RALT
#define key_shift KEYD_RSHIFT

const char* chat_macros[] =
// Ty 03/27/98 - *not* externalized
// CPhipps - const char*
{
  HUSTR_CHATMACRO0,
  HUSTR_CHATMACRO1,
  HUSTR_CHATMACRO2,
  HUSTR_CHATMACRO3,
  HUSTR_CHATMACRO4,
  HUSTR_CHATMACRO5,
  HUSTR_CHATMACRO6,
  HUSTR_CHATMACRO7,
  HUSTR_CHATMACRO8,
  HUSTR_CHATMACRO9
};

const char* player_names[] =
// Ty 03/27/98 - *not* externalized
// CPhipps - const char*
{
  HUSTR_PLRGREEN,
  HUSTR_PLRINDIGO,
  HUSTR_PLRBROWN,
  HUSTR_PLRRED
};

//jff 3/17/98 translate player colmap to text color ranges
int plyrcoltran[MAXPLAYERS]={CR_GREEN,CR_GRAY,CR_BROWN,CR_RED};

char chat_char;                 // remove later.
static player_t*  plr;

// font sets
patchnum_t hu_font[HU_FONTSIZE];
patchnum_t hu_font2[HU_FONTSIZE];
patchnum_t hu_fontk[HU_FONTSIZE];//jff 3/7/98 added for graphic key indicators
patchnum_t hu_msgbg[9];          //jff 2/26/98 add patches for message background
patchnum_t hu_font_hud[HU_FONTSIZE];

// widgets
static hu_textline_t  w_title;
static hu_stext_t     w_message;
static hu_itext_t     w_chat;
static hu_itext_t     w_inputbuffer[MAXPLAYERS];
static hu_textline_t  w_coordx; //jff 2/16/98 new coord widget for automap
static hu_textline_t  w_coordy; //jff 3/3/98 split coord widgets automap
static hu_textline_t  w_coordz; //jff 3/3/98 split coord widgets automap
static hu_textline_t  w_ammo;   //jff 2/16/98 new ammo widget for hud
static hu_textline_t  w_health; //jff 2/16/98 new health widget for hud
static hu_textline_t  w_armor;  //jff 2/16/98 new armor widget for hud
static hu_textline_t  w_weapon; //jff 2/16/98 new weapon widget for hud
static hu_textline_t  w_keys;   //jff 2/16/98 new keys widget for hud
static hu_textline_t  w_gkeys;  //jff 3/7/98 graphic keys widget for hud
static hu_textline_t  w_monsec; //jff 2/16/98 new kill/secret widget for hud
static hu_mtext_t     w_rtext;  //jff 2/26/98 text message refresh widget

static hu_textline_t  w_map_monsters;  //e6y monsters widget for automap
static hu_textline_t  w_map_secrets;   //e6y secrets widgets automap
static hu_textline_t  w_map_items;     //e6y items widgets automap
static hu_textline_t  w_map_time;      //e6y level time widgets automap
static hu_textline_t  w_map_totaltime; //e6y total time widgets automap

static hu_textline_t  w_health_big;
static hu_textline_t  w_medict_icon_big;
static hu_textline_t  w_medict_icon_small;
static hu_textline_t  w_medict_icon_custom;
static hu_textline_t  w_armor_big;
static hu_textline_t  w_armor_icon_big;
static hu_textline_t  w_armor_icon_small;
static hu_textline_t  w_armor_icon_custom;
static hu_textline_t  w_medict_percent;
static hu_textline_t  w_armor_percent;
static hu_textline_t  w_ammo_big;
static hu_textline_t  w_ammo_icon;
static hu_textline_t  w_keys_icon;

static dboolean    always_off = false;
static char       chat_dest[MAXPLAYERS];
dboolean           chat_on;
static dboolean    message_on;
static dboolean    message_list; //2/26/98 enable showing list of messages
dboolean           message_dontfuckwithme;
static dboolean    message_nottobefuckedwith;
static int        message_counter;
extern int        showMessages;
static dboolean    headsupactive = false;

//jff 2/16/98 hud supported automap colors added
int hudcolor_titl;  // color range of automap level title
int hudcolor_xyco;  // color range of new coords on automap
int hudcolor_mapstat_title;
int hudcolor_mapstat_value;
int hudcolor_mapstat_time;
//jff 2/16/98 hud text colors, controls added
int hudcolor_mesg;  // color range of scrolling messages
int hudcolor_chat;  // color range of chat lines
int hud_msg_lines;  // number of message lines in window
//jff 2/26/98 hud text colors, controls added
int hudcolor_list;  // list of messages color
int hud_list_bgon;  // enable for solid window background for message list

//jff 2/16/98 initialization strings for ammo, health, armor widgets
static char hud_coordstrx[32];
static char hud_coordstry[32];
static char hud_coordstrz[32];
static char hud_ammostr[80];
static char hud_healthstr[80];
static char hud_armorstr[80];
static char hud_weapstr[80];
static char hud_keysstr[80];
static char hud_gkeysstr[80]; //jff 3/7/98 add support for graphic key display
static char hud_monsecstr[80];

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//
// Ty 03/27/98 - externalized map name arrays - now in d_deh.c
// and converted to arrays of pointers to char *
// See modified HUTITLEx macros
extern char **mapnames[];
extern char **mapnames2[];
extern char **mapnamesp[];
extern char **mapnamest[];

extern int map_point_coordinates;
extern int map_level_stat;

// key tables
// jff 5/10/98 french support removed,
// as it was not being used and couldn't be easily tested
//
const char* shiftxform;

static custom_message_t custom_message[MAXPLAYERS];
static custom_message_t *custom_message_p;
void HU_init_crosshair(void);

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

static void HU_SetLumpTrans(const char *name)
{
  int lump = W_CheckNumForName(name);
  if (lump > 0)
  {
    lumpinfo[lump].flags |= LUMP_CM2RGB;
  }
}

//
// HU_Init()
//
// Initialize the heads-up display, text that overwrites the primary display
//
// Passed nothing, returns nothing
//
void HU_Init(void)
{

  int   i;
  int   j;
  char  buffer[9];

  shiftxform = english_shiftxform;

  // load the heads-up font
  j = HU_FONTSTART;
  for (i=0;i<HU_FONTSIZE;i++,j++)
  {
    if ('0'<=j && j<='9')
    {
      sprintf(buffer, "DIG%.1d",j-48);
      R_SetPatchNum(&hu_font2[i], buffer);
      sprintf(buffer, "STCFN%.3d",j);
      R_SetPatchNum(&hu_font[i], buffer);
      sprintf(buffer, "STTNUM%.1d",j-48);
      R_SetPatchNum(&hu_font_hud[i], buffer);
    }
    else if ('A'<=j && j<='Z')
    {
      sprintf(buffer, "DIG%c",j);
      R_SetPatchNum(&hu_font2[i], buffer);
      sprintf(buffer, "STCFN%.3d",j);
      R_SetPatchNum(&hu_font[i], buffer);
    }
    else if (j=='-')
    {
      R_SetPatchNum(&hu_font2[i], "DIG45");
      R_SetPatchNum(&hu_font[i], "STCFN045");
    }
    else if (j=='/')
    {
      R_SetPatchNum(&hu_font2[i], "DIG47");
      R_SetPatchNum(&hu_font[i], "STCFN047");
    }
    else if (j==':')
    {
      R_SetPatchNum(&hu_font2[i], "DIG58");
      R_SetPatchNum(&hu_font[i], "STCFN058");
    }
    else if (j=='[')
    {
      R_SetPatchNum(&hu_font2[i], "DIG91");
      R_SetPatchNum(&hu_font[i], "STCFN091");
    }
    else if (j==']')
    {
      R_SetPatchNum(&hu_font2[i], "DIG93");
      R_SetPatchNum(&hu_font[i], "STCFN093");
    }
    else if (j<97)
    {
      sprintf(buffer, "STCFN%.3d",j);
      R_SetPatchNum(&hu_font2[i], buffer);
      R_SetPatchNum(&hu_font[i], buffer);
      //jff 2/23/98 make all font chars defined, useful or not
    }
    else if (j>122)
    {
      sprintf(buffer, "STBR%.3d",j);
      R_SetPatchNum(&hu_font2[i], buffer);
      R_SetPatchNum(&hu_font[i], buffer);
    }
    else
      hu_font[i] = hu_font[0]; //jff 2/16/98 account for gap
  }

  // these patches require cm to rgb translation
  for (i = 33; i < 96; i++)
  {
    sprintf(buffer, "STCFN%.3d", i);
    HU_SetLumpTrans(buffer);
  }
  for (i = 0; i < 10; i++)
  {
    sprintf(buffer, "STTNUM%d", i);
    HU_SetLumpTrans(buffer);
  }
  HU_SetLumpTrans("STTPRCNT");
  HU_SetLumpTrans("STTMINUS");

  // CPhipps - load patches for message background
  for (i=0; i<9; i++) {
    sprintf(buffer, "BOX%c%c", "UCL"[i/3], "LCR"[i%3]);
    R_SetPatchNum(&hu_msgbg[i], buffer);
  }

  // CPhipps - load patches for keys and double keys
  for (i=0; i<6; i++) {
    sprintf(buffer, "STKEYS%d", i);
    R_SetPatchNum(&hu_fontk[i], buffer);
  }

  R_SetSpriteByIndex(&hu_font_hud[4], SPR_MEDI);
  R_SetSpriteByIndex(&hu_font_hud[5], SPR_ARM1);
  R_SetSpriteByIndex(&hu_font_hud[6], SPR_ARM1);
  R_SetSpriteByIndex(&hu_font_hud[7], SPR_ARM2);

  R_SetSpriteByIndex(&hu_font_hud[8], SPR_STIM);
  R_SetSpriteByName(&hu_font_hud[9], "BON2A0");
  R_SetSpriteByName(&hu_font_hud[10], "BON2B0");
  R_SetSpriteByName(&hu_font_hud[11], "BON2D0");

  R_SetPatchNum(&hu_font_hud[12], "STTPRCNT");
  R_SetPatchNum(&hu_font_hud[13], "STTPRCNT");

  R_SetPatchByName(&hu_font_hud[30], "HUDMED");
  R_SetPatchByName(&hu_font_hud[31], "HUDARM1");
  R_SetPatchByName(&hu_font_hud[32], "HUDARM1");
  R_SetPatchByName(&hu_font_hud[33], "HUDARM2");

  R_SetSpriteByName(&hu_font_hud[40], "CLIPA0");
  R_SetSpriteByName(&hu_font_hud[41], "SHELA0");
  R_SetSpriteByName(&hu_font_hud[42], "CELLA0");
  R_SetSpriteByName(&hu_font_hud[43], "ROCKA0");

}

//
// HU_Stop()
//
// Make the heads-up displays inactive
//
// Passed nothing, returns nothing
//
static void HU_Stop(void)
{
  headsupactive = false;
}

//
// HU_Start(void)
//
// Create and initialize the heads-up widgets, software machines to
// maintain, update, and display information over the primary display
//
// This routine must be called after any change to the heads up configuration
// in order for the changes to take effect in the actual displays
//
// Passed nothing, returns nothing
//
void HU_Start(void)
{
  int   i;
  const char* s; /* cph - const */

  if (headsupactive)                    // stop before starting
    HU_Stop();

  plr = &players[displayplayer];        // killough 3/7/98
  custom_message_p = &custom_message[displayplayer];
  message_on = false;
  message_dontfuckwithme = false;
  message_nottobefuckedwith = false;
  chat_on = false;

  // create the message widget
  // messages to player in upper-left of screen
  HUlib_initSText
  (
    &w_message,
    HU_MSGX,
    HU_MSGY,
    HU_MSGHEIGHT,
    hu_font,
    HU_FONTSTART,
    hudcolor_mesg,
    VPT_ALIGN_LEFT_TOP,
    &message_on
  );

  //jff 2/16/98 added some HUD widgets
  // create the map title widget - map title display in lower left of automap
  HUlib_initTextLine
  (
    &w_title,
    HU_TITLEX,
    HU_TITLEY,
    hu_font,
    HU_FONTSTART,
    hudcolor_titl,
    VPT_ALIGN_LEFT_BOTTOM
  );

  // create the hud health widget
  // bargraph and number for amount of health,
  // lower left or upper right of screen
  HUlib_initTextLine
  (
    &w_health,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GREEN,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_health_big,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_medict_icon_big,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_medict_icon_small,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_medict_icon_custom,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  // create the hud armor widget
  // bargraph and number for amount of armor,
  // lower left or upper right of screen
  HUlib_initTextLine
  (
    &w_armor,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GREEN,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_armor_big,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_armor_icon_big,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_armor_icon_small,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_armor_icon_custom,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  // create the hud ammo widget
  // bargraph and number for amount of ammo for current weapon,
  // lower left or lower right of screen
  HUlib_initTextLine
  (
    &w_ammo,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GOLD,
    VPT_NONE
  );

  // create the hud weapons widget
  // list of numbers of weapons possessed
  // lower left or lower right of screen
  HUlib_initTextLine
  (
    &w_weapon,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );

  // create the hud keys widget
  // display of key letters possessed
  // lower left of screen
  HUlib_initTextLine
  (
    &w_keys,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );

  // create the hud graphic keys widget
  // display of key graphics possessed
  // lower left of screen
  HUlib_initTextLine
  (
    &w_gkeys,
    0, 0,
    hu_fontk,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  // create the hud monster/secret widget
  // totals and current values for kills, items, secrets
  // lower left of screen
  HUlib_initTextLine
  (
    &w_monsec,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_medict_percent,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_armor_percent,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_ammo_big,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_ammo_icon,
    0, 0,
    hu_font_hud,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  HUlib_initTextLine
  (
    &w_keys_icon,
    0, 0,
    hu_fontk,
    HU_FONTSTART,
    CR_RED,
    VPT_NONE
  );

  // create the hud text refresh widget
  // scrolling display of last hud_msg_lines messages received
  if (hud_msg_lines>HU_MAXMESSAGES)
    hud_msg_lines=HU_MAXMESSAGES;
  //jff 4/21/98 if setup has disabled message list while active, turn it off
  message_list = hud_msg_lines > 1; //jff 8/8/98 initialize both ways
  //jff 2/26/98 add the text refresh widget initialization
  HUlib_initMText
  (
    &w_rtext,
    0,
    0,
    320,
//    SCREENWIDTH,
    (hud_msg_lines+2)*HU_REFRESHSPACING,
    hu_font,
    HU_FONTSTART,
    hudcolor_list,
    hu_msgbg,
    VPT_ALIGN_LEFT_TOP,
    &message_list
  );

  if (gamemapinfo != NULL)
  {
	  s = gamemapinfo->mapname;
	  while (*s)
		  HUlib_addCharToTextLine(&w_title, *(s++));

	  HUlib_addCharToTextLine(&w_title, ':');
	  HUlib_addCharToTextLine(&w_title, ' ');
	  s = gamemapinfo->levelname;
	  if (!s) s = "Unnamed";
	  while (*s)
		  HUlib_addCharToTextLine(&w_title, *(s++));

  }
  else
  {
	  // initialize the automap's level title widget
	  // e6y: stop SEGV here when gamemap is not initialized
	  if (gamestate == GS_LEVEL && gamemap > 0) /* cph - stop SEGV here when not in level */
		  switch (gamemode)
		  {
		  case shareware:
		  case registered:
		  case retail:
			  s = HU_TITLE;
			  break;

		  case commercial:
		  default:  // Ty 08/27/98 - modified to check mission for TNT/Plutonia
			  s = (gamemission == pack_tnt) ? HU_TITLET :
				  (gamemission == pack_plut) ? HU_TITLEP : HU_TITLE2;
			  break;
		  }
	  else s = "";

	  // Chex.exe always uses the episode 1 level title
	  // eg. E2M1 gives the title for E1M1
	  if (gamemission == chex)
	  {
		  s = HU_TITLEC;
	  }
	  while (*s)
		  HUlib_addCharToTextLine(&w_title, *(s++));
  }


  // create the automaps coordinate widget
  // jff 3/3/98 split coord widget into three lines: x,y,z
  // jff 2/16/98 added
  HUlib_initTextLine
  (
    &w_coordx,
    HU_COORDX,
    HU_COORDX_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_xyco,
    VPT_ALIGN_RIGHT_TOP
  );
  HUlib_initTextLine
  (
    &w_coordy,
    HU_COORDX,
    HU_COORDY_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_xyco,
    VPT_ALIGN_RIGHT_TOP
  );
  HUlib_initTextLine
  (
    &w_coordz,
    HU_COORDX,
    HU_COORDZ_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_xyco,
    VPT_ALIGN_RIGHT_TOP
  );
//e6y
  HUlib_initTextLine
  (
    &w_map_monsters,
    HU_MAP_STAT_X,
    HU_MAP_MONSTERS_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_mapstat_title,
    VPT_ALIGN_LEFT_TOP
  );
  HUlib_initTextLine
  (
    &w_map_secrets,
    HU_MAP_STAT_X,
    HU_MAP_SECRETS_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_mapstat_title,
    VPT_ALIGN_LEFT_TOP
  );
  HUlib_initTextLine
  (
    &w_map_items,
    HU_MAP_STAT_X,
    HU_MAP_ITEMS_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_mapstat_title,
    VPT_ALIGN_LEFT_TOP
  );
  HUlib_initTextLine
  (
    &w_map_time,
    HU_MAP_STAT_X,
    HU_MAP_TIME_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_mapstat_time,
    VPT_ALIGN_LEFT_TOP
  );
  HUlib_initTextLine
  (
    &w_map_totaltime,
    HU_MAP_STAT_X,
    HU_MAP_TOTALTIME_Y,
    hu_font,
    HU_FONTSTART,
    hudcolor_mapstat_time,
    VPT_ALIGN_LEFT_TOP
  );
  HUlib_initTextLine
  (
    &w_hudadd,
    0, 0,
    hu_font2,
    HU_FONTSTART,
    CR_GRAY,
    VPT_NONE
  );
  HUlib_initTextLine
  (
    &w_centermsg,
    HU_CENTERMSGX,
    HU_CENTERMSGY,
    hu_font,
    HU_FONTSTART,
    hudcolor_titl,
    VPT_STRETCH
  );
  HUlib_initTextLine
  (
    &w_precache,
    16,
    186,
    hu_font,
    HU_FONTSTART,
    CR_RED,
    VPT_ALIGN_LEFT_BOTTOM
  );
  strcpy(hud_add,"");
  s = hud_add;
  while (*s)
    HUlib_addCharToTextLine(&w_hudadd, *(s++));

  for(i = 0; i < NUMTRACES; i++)
  {
    HUlib_initTextLine(
      &w_traces[i],
      HU_TRACERX,
      HU_TRACERY+i*HU_GAPY,
      hu_font2,
      HU_FONTSTART,
      CR_GRAY,
      VPT_ALIGN_LEFT_BOTTOM
    );

    strcpy(traces[i].hudstr, "");
    s = traces[i].hudstr;
    while (*s)
      HUlib_addCharToTextLine(&w_traces[i], *(s++));
    HUlib_drawTextLine(&w_traces[i], false);
  }


  //jff 2/16/98 initialize ammo widget
  strcpy(hud_ammostr,"AMM ");

  //jff 2/16/98 initialize health widget
  strcpy(hud_healthstr,"HEL ");

  //jff 2/16/98 initialize armor widget
  strcpy(hud_armorstr,"ARM ");

  //jff 2/17/98 initialize weapons widget
  strcpy(hud_weapstr,"WEA ");

  //jff 2/17/98 initialize keys widget
  //jff 3/17/98 show frags in deathmatch mode
  strcpy(hud_keysstr,(deathmatch ? "FRG " : "KEY "));

  //jff 2/17/98 initialize graphic keys widget
  strcpy(hud_gkeysstr," ");

  //jff 2/17/98 initialize kills/items/secret widget
  strcpy(hud_monsecstr,"STS ");

  // create the chat widget
  HUlib_initIText
  (
    &w_chat,
    HU_INPUTX,
    HU_INPUTY,
    hu_font,
    HU_FONTSTART,
    hudcolor_chat,
    VPT_NONE,
    &chat_on
  );

  // create the inputbuffer widgets, one per player
  for (i=0 ; i<MAXPLAYERS ; i++)
    HUlib_initIText
    (
      &w_inputbuffer[i],
      0,
      0,
      0,
      0,
      hudcolor_chat,
      VPT_NONE,
      &always_off
    );

  HU_init_crosshair();
  
  // now allow the heads-up display to run
  headsupactive = true;

  HU_LoadHUDDefs();

  HU_MoveHud(true);
}

void HU_NextHud(void)
{
  if (huds_count > 0)
  {
    hud_num = (hud_num + 1) % huds_count;    // cycle hud_active
  }
}

typedef void (*HU_widget_build_func)(void);
typedef void (*HU_widget_draw_func)(void);

typedef struct hud_cfg_item_s
{
  char name[80];
  int x;
  int y;
} hud_cfg_item_t;

typedef struct hud_widget_s
{
  hu_textline_t *hu_textline;
  int x;
  int y;
  enum patch_translation_e flags;
  HU_widget_build_func build;
  HU_widget_draw_func draw;
  const char *name;
} hud_widget_t;

typedef struct hud_widgets_list_s
{
  int count;
  hud_widget_t *items;
} hud_widgets_list_t;

int huds_count;
hud_widgets_list_t *huds;
hud_widgets_list_t *hud_current;

void HU_widget_build_ammo(void);
void HU_widget_draw_ammo(void);
void HU_widget_build_weapon(void);
void HU_widget_draw_weapon(void);
void HU_widget_build_keys(void);
void HU_widget_draw_keys(void);
void HU_widget_build_monsec(void);
void HU_widget_draw_monsec(void);
void HU_widget_build_health(void);
void HU_widget_draw_health(void);
void HU_widget_build_armor(void);
void HU_widget_draw_armor(void);
void HU_widget_build_hudadd(void);
void HU_widget_draw_hudadd(void);

void HU_widget_build_health_big(void);
void HU_widget_draw_health_big(void);
void HU_widget_build_armor_big(void);
void HU_widget_draw_armor_big(void);

void HU_widget_build_medict_icon_big(void);
void HU_widget_draw_medict_icon_big(void);
void HU_widget_build_armor_icon_big(void);
void HU_widget_draw_armor_icon_big(void);

void HU_widget_build_medict_icon_small(void);
void HU_widget_draw_medict_icon_small(void);
void HU_widget_build_armor_icon_small(void);
void HU_widget_draw_armor_icon_small(void);

void HU_widget_build_medict_icon_custom(void);
void HU_widget_draw_medict_icon_custom(void);
void HU_widget_build_armor_icon_custom(void);
void HU_widget_draw_armor_icon_custom(void);

void HU_widget_build_medict_percent(void);
void HU_widget_draw_medict_percent(void);
void HU_widget_build_armor_percent(void);
void HU_widget_draw_armor_percent(void);

void HU_widget_build_ammo_big(void);
void HU_widget_draw_ammo_big(void);
void HU_widget_build_ammo_icon(void);
void HU_widget_draw_ammo_icon(void);

void HU_widget_build_gkeys(void);
void HU_widget_draw_gkeys(void);

static hud_widget_t hud_name_widget[] =
{
  {&w_ammo,   0, 0, 0, HU_widget_build_ammo,   HU_widget_draw_ammo,   "ammo"},
  {&w_weapon, 0, 0, 0, HU_widget_build_weapon, HU_widget_draw_weapon, "weapon"},
  {&w_keys,   0, 0, 0, HU_widget_build_keys,   HU_widget_draw_keys,   "keys"},
  {&w_monsec, 0, 0, 0, HU_widget_build_monsec, HU_widget_draw_monsec, "monsec"},
  {&w_health, 0, 0, 0, HU_widget_build_health, HU_widget_draw_health, "health"},
  {&w_armor,  0, 0, 0, HU_widget_build_armor,  HU_widget_draw_armor,  "armor"},
  {&w_hudadd, 0, 0, 0, HU_widget_build_hudadd, HU_widget_draw_hudadd, "hudadd"},

  {&w_keys_icon, 0, 0, 0, HU_widget_build_gkeys, HU_widget_draw_gkeys, "gkeys"},

  {&w_traces[0], 0, 0, 0, NULL, NULL, "tracers"},

  {&w_health_big, 0, 0, VPT_NOOFFSET, HU_widget_build_health_big, HU_widget_draw_health_big, "health_big"},
  {&w_armor_big,  0, 0, VPT_NOOFFSET, HU_widget_build_armor_big,  HU_widget_draw_armor_big,  "armor_big"},

  {&w_medict_icon_big, 0, 0, VPT_NOOFFSET, HU_widget_build_medict_icon_big, HU_widget_draw_medict_icon_big, "medict_icon_big"},
  {&w_armor_icon_big,  0, 0, VPT_NOOFFSET, HU_widget_build_armor_icon_big,  HU_widget_draw_armor_icon_big,  "armor_icon_big"},

  {&w_medict_icon_small, 0, 0, VPT_NOOFFSET, HU_widget_build_medict_icon_small, HU_widget_draw_medict_icon_small, "medict_icon_small"},
  {&w_armor_icon_small,  0, 0, VPT_NOOFFSET, HU_widget_build_armor_icon_small,  HU_widget_draw_armor_icon_small,  "armor_icon_small"},

  {&w_medict_icon_custom, 0, 0, VPT_NOOFFSET, HU_widget_build_medict_icon_custom, HU_widget_draw_medict_icon_custom, "medict_icon_custom"},
  {&w_armor_icon_custom,  0, 0, VPT_NOOFFSET, HU_widget_build_armor_icon_custom,  HU_widget_draw_armor_icon_custom,  "armor_icon_custom"},

  {&w_medict_percent, 0, 0, VPT_NOOFFSET, HU_widget_build_medict_percent, HU_widget_draw_medict_percent, "medict_percent"},
  {&w_armor_percent,  0, 0, VPT_NOOFFSET, HU_widget_build_armor_percent,  HU_widget_draw_armor_percent,  "armor_percent"},

  {&w_ammo_big,  0, 0, VPT_NOOFFSET, HU_widget_build_ammo_big,  HU_widget_draw_ammo_big,  "ammo_big"},
  {&w_ammo_big,  0, 0, VPT_NOOFFSET, HU_widget_build_ammo_big,  HU_widget_draw_ammo_big,  "ammo_big"},
  {&w_ammo_icon, 0, 0, VPT_NOOFFSET, HU_widget_build_ammo_icon, HU_widget_draw_ammo_icon, "ammo_icon"},
  {&w_ammo_icon, 0, 0, VPT_NOOFFSET, HU_widget_build_ammo_icon, HU_widget_draw_ammo_icon, "ammo_icon"},

  {NULL, 0, 0, 0, NULL, NULL, NULL}
};

void HU_LoadHUDDefs(void)
{
  static int init = 0;

  int lump, i, params_count;
  hud_cfg_item_t cfg_item;
  hud_widgets_list_t *list = NULL;
  char st[200];

  if (init)
    return;

  init = true;

  huds_count = 0;
  huds = NULL;

  lump = (W_CheckNumForName)("-PRBHUD-", ns_prboom);
  if (lump != -1)
  {
    SC_OpenLumpByNum(lump);

    // Get actor class name.
    while (SC_GetString())
    {
      // declaration of hud
      if (SC_Compare("hud"))
      {
        // skip everything after "hud" signature
        while (SC_Check())
          SC_GetString();

        // setup new hud
        huds_count++;
        huds = realloc(huds, huds_count * sizeof(huds[0]));
        list = &huds[huds_count - 1];
        list->items = NULL;
        list->count = 0;

        // definition of hud is below
        continue;
      }

      // keep going until a valid HUD declaration has been found
      if (huds_count < 1)
        continue;

      strncpy(st, sc_String, sizeof(st) - 1);

      while (SC_Check() && SC_GetString())
      {
        strncat(st, " ", sizeof(st) - 1);
        strncat(st, sc_String, sizeof(st) - 1);
      }
      st[sizeof(st) - 1] = 0;

      // hud_widget x y
      params_count = sscanf(st, "%s %d %d", &cfg_item.name[0], &cfg_item.x, &cfg_item.y);
      if (params_count == 3)
      {
        for (i = 0; hud_name_widget[i].name; i++)
        {
          if (!strcasecmp(hud_name_widget[i].name, cfg_item.name))
          {
            hud_widget_t *item;

            list->count++;
            list->items = realloc(list->items, list->count * sizeof(list->items[0]));

            item = &list->items[list->count - 1];

            item->hu_textline = hud_name_widget[i].hu_textline;

            item->x = cfg_item.x;
            item->y = cfg_item.y;

            if (abs(cfg_item.x) < 160)
            {
              item->flags = (abs(cfg_item.y) > 100 ? VPT_ALIGN_LEFT_BOTTOM : VPT_ALIGN_LEFT_TOP);
            }
            else
            {
              item->flags = (abs(cfg_item.y) > 100 ? VPT_ALIGN_RIGHT_BOTTOM : VPT_ALIGN_RIGHT_TOP);
            }
            item->flags |= hud_name_widget[i].flags;

            item->build = hud_name_widget[i].build;
            item->draw = hud_name_widget[i].draw;

            break;
          }
        }
      }
    }

    SC_Close();
  }
}

//
// HU_MoveHud()
//
// Move the HUD display from distributed to compact mode or vice-versa
//
// Passed nothing, returns nothing
//
//jff 3/9/98 create this externally callable to avoid glitch
// when menu scatter's HUD due to delay in change of position
//

void HU_MoveHud(int force)
{
  static int ohud_num = -1;

  //jff 3/4/98 move displays around on F5 changing hud_distributed
  if ((huds_count > 0) && (force || hud_num != ohud_num))
  {
    int i;

    hud_current = &huds[hud_num % huds_count];

    for (i = 0; i < hud_current->count; i++)
    {
      hud_current->items[i].hu_textline->x = hud_current->items[i].x;
      hud_current->items[i].hu_textline->y = hud_current->items[i].y;
      hud_current->items[i].hu_textline->flags = hud_current->items[i].flags;
    }

    ohud_num = hud_num;
  }
}

int HU_GetHealthColor(int health, int def)
{
  int result;

  if (health < health_red)
    result = CR_RED;
  else if (health < health_yellow)
    result = CR_GOLD;
  else if (health <= health_green)
    result = CR_GREEN;
  else
    result = def;

  return result;
}

int HU_GetArmorColor(int armor, int def)
{
  int result;

  if (armor < armor_red)
    result = CR_RED;
  else if (armor < armor_yellow)
    result = CR_GOLD;
  else if (armor <= armor_green)
    result = CR_GREEN;
  else
    result = def;

  return result;
}

int HU_GetAmmoColor(int ammo, int fullammo, int def, int tofire, dboolean backpack)
{
  int result, ammopct;

  if (ammo < tofire)
    result = CR_BROWN;
  else if ((ammo==fullammo) || 
    (ammo_colour_behaviour == ammo_colour_behaviour_no && backpack && ammo*2 >= fullammo))
    result=def;
  else {
    ammopct = (100 * ammo) / fullammo;
    if (backpack && ammo_colour_behaviour != ammo_colour_behaviour_yes)
      ammopct *= 2;
    if (ammopct < ammo_red)
      result = CR_RED;
    else if (ammopct < ammo_yellow)
      result = CR_GOLD;
    else
      result = CR_GREEN;
  }

  return result;
}

void HU_widget_build_ammo(void)
{
  int i;
  char *s;
  char ammostr[80];  //jff 3/8/98 allow plenty room for dehacked mods
  int fullammo = plr->maxammo[weaponinfo[plr->readyweapon].ammo];

  // do the hud ammo display
  // clear the widgets internal line
  HUlib_clearTextLine(&w_ammo);
  strcpy(hud_ammostr,"AMM ");
  if (weaponinfo[plr->readyweapon].ammo == am_noammo || fullammo == 0)
  { // special case for weapon with no ammo selected - blank bargraph + N/A
    strcat(hud_ammostr,"\x7f\x7f\x7f\x7f\x7f\x7f\x7f N/A");
    w_ammo.cm = CR_GRAY;
  }
  else
  {
    int ammo = plr->ammo[weaponinfo[plr->readyweapon].ammo];
    int ammopct = (100*ammo)/fullammo;
    int ammobars = ammopct/4;

    // build the numeric amount init string
    sprintf(ammostr,"%d/%d",ammo,fullammo);
    // build the bargraph string
    // full bargraph chars
    for (i=4;i<4+ammobars/4;)
      hud_ammostr[i++] = 123;
    // plus one last character with 0,1,2,3 bars
    switch(ammobars%4)
    {
    case 0:
      break;
    case 1:
      hud_ammostr[i++] = 126;
      break;
    case 2:
      hud_ammostr[i++] = 125;
      break;
    case 3:
      hud_ammostr[i++] = 124;
      break;
    }

    // pad string with blank bar characters
    while(i<4+7)
      hud_ammostr[i++] = 127;
    hud_ammostr[i] = '\0';
    strcat(hud_ammostr,ammostr);

    // set the display color from the percentage of total ammo held
    w_ammo.cm = HU_GetAmmoColor(ammo, fullammo, CR_BLUE,
      ammopershot[plr->readyweapon], plr->backpack);
  }
  // transfer the init string to the widget
  s = hud_ammostr;
  while (*s)
    HUlib_addCharToTextLine(&w_ammo, *(s++));
}

void HU_widget_draw_ammo(void)
{
  // display the ammo widget every frame
  HUlib_drawTextLine(&w_ammo, false);
}

void HU_widget_build_health(void)
{
  int i;
  char *s;
  char healthstr[80];//jff
  int health = plr->health;
  int healthbars = health>100? 25 : health/4;

  if (w_health.val != -1 && w_health.val == health)
    return;
  w_health.val = health;

  // clear the widgets internal line
  HUlib_clearTextLine(&w_health);

  // build the numeric amount init string
  sprintf(healthstr,"%3d",health);
  // build the bargraph string
  // full bargraph chars
  for (i=4;i<4+healthbars/4;)
    hud_healthstr[i++] = 123;
  // plus one last character with 0,1,2,3 bars
  switch(healthbars%4)
  {
  case 0:
    break;
  case 1:
    hud_healthstr[i++] = 126;
    break;
  case 2:
    hud_healthstr[i++] = 125;
    break;
  case 3:
    hud_healthstr[i++] = 124;
    break;
  }
  // pad string with blank bar characters
  while(i<4+7)
    hud_healthstr[i++] = 127;
  hud_healthstr[i] = '\0';
  strcat(hud_healthstr,healthstr);

  // set the display color from the amount of health posessed
  w_health.cm = HU_GetHealthColor(health, CR_BLUE);

  // transfer the init string to the widget
  s = hud_healthstr;
  while (*s)
    HUlib_addCharToTextLine(&w_health, *(s++));
}

void HU_widget_draw_health(void)
{
  HUlib_drawTextLine(&w_health, false);
}

void HU_widget_build_health_big(void)
{
  char *s;
  char healthstr[80];//jff
  int health = plr->health;

  if (w_health_big.val != -1 && w_health_big.val == health)
    return;
  w_health_big.val = health;

  // clear the widgets internal line
  HUlib_clearTextLine(&w_health_big);

  // build the numeric amount init string
  sprintf(healthstr,"%d",health);

  // set the display color from the amount of health posessed
  if (!sts_always_red)
    w_health_big.cm = HU_GetHealthColor(health, CR_BLUE2);

  // transfer the init string to the widget
  s = healthstr;
  while (*s)
    HUlib_addCharToTextLine(&w_health_big, *(s++));
}

void HU_widget_draw_health_big(void)
{
  HUlib_drawTextLine(&w_health_big, false);
}

void HU_widget_build_medict_icon_big(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_medict_icon_big);
  HUlib_addCharToTextLine(&w_medict_icon_big, '!' + 0 + 4);
}

void HU_widget_draw_medict_icon_big(void)
{
  HUlib_drawTextLine(&w_medict_icon_big, false);
}

void HU_widget_build_medict_icon_small(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_medict_icon_small);
  HUlib_addCharToTextLine(&w_medict_icon_small, '!' + 0 + 8);
}

void HU_widget_draw_medict_icon_small(void)
{
  HUlib_drawTextLine(&w_medict_icon_small, false);
}

void HU_widget_build_medict_icon_custom(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_medict_icon_custom);
  HUlib_addCharToTextLine(&w_medict_icon_custom, '!' + 0 + 30);
}

void HU_widget_draw_medict_icon_custom(void)
{
  HUlib_drawTextLine(&w_medict_icon_custom, false);
}

void HU_widget_build_armor_icon_custom(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_armor_icon_custom);
  HUlib_addCharToTextLine(&w_armor_icon_custom, (char)('!' + plr->armortype + 1 + 30));
}

void HU_widget_draw_armor_icon_custom(void)
{
  HUlib_drawTextLine(&w_armor_icon_custom, false);
}

void HU_widget_build_armor(void)
{
  int i;
  char *s;
  char armorstr[80]; //jff
  int armor = plr->armorpoints;
  int armorbars = armor>100? 25 : armor/4;

  if (w_armor.val != -1 && w_armor.val == armor)
    return;
  w_armor.val = armor;

  // clear the widgets internal line
  HUlib_clearTextLine(&w_armor);
  // build the numeric amount init string
  sprintf(armorstr,"%3d",armor);
  // build the bargraph string
  // full bargraph chars
  for (i=4;i<4+armorbars/4;)
    hud_armorstr[i++] = 123;
  // plus one last character with 0,1,2,3 bars
  switch(armorbars%4)
  {
  case 0:
    break;
  case 1:
    hud_armorstr[i++] = 126;
    break;
  case 2:
    hud_armorstr[i++] = 125;
    break;
  case 3:
    hud_armorstr[i++] = 124;
    break;
  }
  // pad string with blank bar characters
  while(i<4+7)
    hud_armorstr[i++] = 127;
  hud_armorstr[i] = '\0';
  strcat(hud_armorstr,armorstr);

  // set the display color from the amount of armor posessed
  w_armor.cm = HU_GetArmorColor(armor, CR_BLUE);

  // transfer the init string to the widget
  s = hud_armorstr;
  while (*s)
    HUlib_addCharToTextLine(&w_armor, *(s++));
}

void HU_widget_draw_armor(void)
{
  HUlib_drawTextLine(&w_armor, false);
}

void HU_widget_build_armor_big(void)
{
  char *s;
  char armorstr[80]; //jff
  int armor = plr->armorpoints;

  if (w_armor_big.val != -1 && w_armor_big.val == armor)
    return;
  w_armor_big.val = armor;

  // clear the widgets internal line
  HUlib_clearTextLine(&w_armor_big);
  // build the numeric amount init string
  sprintf(armorstr,"%d",armor);

  // set the display color from the amount of armor posessed
  if (!sts_always_red)
    w_armor_big.cm = HU_GetArmorColor(armor, CR_BLUE2);

  // transfer the init string to the widget
  s = armorstr;
  while (*s)
    HUlib_addCharToTextLine(&w_armor_big, *(s++));
}

void HU_widget_draw_armor_big(void)
{
  HUlib_drawTextLine(&w_armor_big, false);
}

void HU_widget_build_armor_icon_big(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_armor_icon_big);
  HUlib_addCharToTextLine(&w_armor_icon_big, (char)('!' + plr->armortype + 1 + 4));
}

void HU_widget_draw_armor_icon_big(void)
{
  HUlib_drawTextLine(&w_armor_icon_big, false);
}

void HU_widget_build_armor_icon_small(void)
{
  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_armor_icon_small);
  HUlib_addCharToTextLine(&w_armor_icon_small, (char)('!' + plr->armortype + 1 + 8));
}

void HU_widget_draw_armor_icon_small(void)
{
  HUlib_drawTextLine(&w_armor_icon_small, false);
}


void HU_widget_build_weapon(void)
{
  int i;
  char *s;
  int w;
  int ammo,fullammo,ammopct;

  // clear the widgets internal line
  HUlib_clearTextLine(&w_weapon);
  i=4; hud_weapstr[i] = '\0';      //jff 3/7/98 make sure ammo goes away

  // do each weapon that exists in current gamemode
  for (w=0;w<=wp_supershotgun;w++) //jff 3/4/98 show fists too, why not?
  {
    int ok=1;
    //jff avoid executing for weapons that do not exist
    switch (gamemode)
    {
    case shareware:
      if (w>=wp_plasma && w!=wp_chainsaw)
        ok=0;
      break;
    case retail:
    case registered:
      if (w>=wp_supershotgun)
        ok=0;
      break;
    default:
    case commercial:
      break;
    }
    if (!ok) continue;

    ammo = plr->ammo[weaponinfo[w].ammo];
    fullammo = plr->maxammo[weaponinfo[w].ammo];
    ammopct=0;

    // skip weapons not currently posessed
    if (!plr->weaponowned[w])
      continue;

    // display each weapon number in a color related to the ammo for it
    hud_weapstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
    if (weaponinfo[w].ammo==am_noammo) //jff 3/14/98 show berserk on HUD
      hud_weapstr[i++] = plr->powers[pw_strength]? '0'+CR_GREEN : '0'+CR_GRAY;
    else if (ammo<ammopershot[w])
      hud_weapstr[i++] = '0'+CR_BROWN;
    else if (fullammo && ((ammo==fullammo) ||
      (ammo_colour_behaviour == ammo_colour_behaviour_no &&
      plr->backpack && ammo*2 >= fullammo)))
      hud_weapstr[i++] = '0'+CR_BLUE;
    else
    {
      ammopct = fullammo ? (100*ammo)/fullammo : 100;
      if (plr->backpack && fullammo &&
        ammo_colour_behaviour != ammo_colour_behaviour_yes)
        ammopct *= 2;
      if (ammopct<ammo_red)
        hud_weapstr[i++] = '0'+CR_RED;
      else if (ammopct<ammo_yellow)
        hud_weapstr[i++] = '0'+CR_GOLD;
      else
        hud_weapstr[i++] = '0'+CR_GREEN;
    }
    hud_weapstr[i++] = '0'+w+1;
    hud_weapstr[i++] = ' ';
    hud_weapstr[i] = '\0';
  }

  M_StrRTrim(hud_weapstr);

  // transfer the init string to the widget
  s = hud_weapstr;
  while (*s)
    HUlib_addCharToTextLine(&w_weapon, *(s++));
}

void HU_widget_draw_weapon(void)
{
  HUlib_drawTextLine(&w_weapon, false);
}

void HU_widget_build_keys(void)
{
  int i;
  int k;
  char *s;

  hud_keysstr[4] = '\0';    //jff 3/7/98 make sure deleted keys go away
  //jff add case for graphic key display
  if (!deathmatch)
  {
    i=0;
    hud_gkeysstr[i] = '\0'; //jff 3/7/98 init graphic keys widget string
    // build text string whose characters call out graphic keys from fontk
    for (k=0;k<6;k++)
    {
      // skip keys not possessed
      if (!plr->cards[k])
        continue;

      hud_gkeysstr[i++] = '!'+k;   // key number plus '!' is char for key
      hud_gkeysstr[i++] = ' ';     // spacing
      hud_gkeysstr[i++] = ' ';
    }
    hud_gkeysstr[i]='\0';
  }
  else // not possible in current code, unless deathmatching,
  {
    i=4;
    hud_keysstr[i] = '\0';  //jff 3/7/98 make sure deleted keys go away

    // if deathmatch, build string showing top four frag counts
    if (deathmatch) //jff 3/17/98 show frags, not keys, in deathmatch
    {
      int top1=-999,top2=-999,top3=-999,top4=-999;
      int idx1=-1,idx2=-1,idx3=-1,idx4=-1;
      int fragcount,m;
      char numbuf[32];

      // scan thru players
      for (k=0;k<MAXPLAYERS;k++)
      {
        // skip players not in game
        if (!playeringame[k])
          continue;

        fragcount = 0;
        // compute number of times they've fragged each player
        // minus number of times they've been fragged by them
        for (m=0;m<MAXPLAYERS;m++)
        {
          if (!playeringame[m]) continue;
          fragcount += (m!=k)?  players[k].frags[m] : -players[k].frags[m];
        }

        // very primitive sort of frags to find top four
        if (fragcount>top1)
        {
          top4=top3; top3=top2; top2 = top1; top1=fragcount;
          idx4=idx3; idx3=idx2; idx2 = idx1; idx1=k;
        }
        else if (fragcount>top2)
        {
          top4=top3; top3=top2; top2=fragcount;
          idx4=idx3; idx3=idx2; idx2=k;
        }
        else if (fragcount>top3)
        {
          top4=top3; top3=fragcount;
          idx4=idx3; idx3=k;
        }
        else if (fragcount>top4)
        {
          top4=fragcount;
          idx4=k;
        }
      }
      // if the biggest number exists, put it in the init string
      if (idx1>-1)
      {
        sprintf(numbuf,"%5d",top1);
        // make frag count in player's color via escape code
        hud_keysstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
        hud_keysstr[i++] = '0'+plyrcoltran[idx1&3];
        s = numbuf;
        while (*s)
          hud_keysstr[i++] = *(s++);
      }
      // if the second biggest number exists, put it in the init string
      if (idx2>-1)
      {
        sprintf(numbuf,"%5d",top2);
        // make frag count in player's color via escape code
        hud_keysstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
        hud_keysstr[i++] = '0'+plyrcoltran[idx2&3];
        s = numbuf;
        while (*s)
          hud_keysstr[i++] = *(s++);
      }
      // if the third biggest number exists, put it in the init string
      if (idx3>-1)
      {
        sprintf(numbuf,"%5d",top3);
        // make frag count in player's color via escape code
        hud_keysstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
        hud_keysstr[i++] = '0'+plyrcoltran[idx3&3];
        s = numbuf;
        while (*s)
          hud_keysstr[i++] = *(s++);
      }
      // if the fourth biggest number exists, put it in the init string
      if (idx4>-1)
      {
        sprintf(numbuf,"%5d",top4);
        // make frag count in player's color via escape code
        hud_keysstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
        hud_keysstr[i++] = '0'+plyrcoltran[idx4&3];
        s = numbuf;
        while (*s)
          hud_keysstr[i++] = *(s++);
      }
      hud_keysstr[i] = '\0';
    } //jff 3/17/98 end of deathmatch clause
    else // build alphabetical key display (not used currently)
    {
      // scan the keys
      for (k=0;k<6;k++)
      {
        // skip any not possessed by the displayed player's stats
        if (!plr->cards[k])
          continue;

        // use color escapes to make text in key's color
        hud_keysstr[i++] = '\x1b'; //jff 3/26/98 use ESC not '\' for paths
        switch(k)
        {
        case 0:
          hud_keysstr[i++] = '0'+CR_BLUE;
          hud_keysstr[i++] = 'B';
          hud_keysstr[i++] = 'C';
          hud_keysstr[i++] = ' ';
          break;
        case 1:
          hud_keysstr[i++] = '0'+CR_GOLD;
          hud_keysstr[i++] = 'Y';
          hud_keysstr[i++] = 'C';
          hud_keysstr[i++] = ' ';
          break;
        case 2:
          hud_keysstr[i++] = '0'+CR_RED;
          hud_keysstr[i++] = 'R';
          hud_keysstr[i++] = 'C';
          hud_keysstr[i++] = ' ';
          break;
        case 3:
          hud_keysstr[i++] = '0'+CR_BLUE;
          hud_keysstr[i++] = 'B';
          hud_keysstr[i++] = 'S';
          hud_keysstr[i++] = ' ';
          break;
        case 4:
          hud_keysstr[i++] = '0'+CR_GOLD;
          hud_keysstr[i++] = 'Y';
          hud_keysstr[i++] = 'S';
          hud_keysstr[i++] = ' ';
          break;
        case 5:
          hud_keysstr[i++] = '0'+CR_RED;
          hud_keysstr[i++] = 'R';
          hud_keysstr[i++] = 'S';
          hud_keysstr[i++] = ' ';
          break;
        }
        hud_keysstr[i]='\0';
      }
    }
  }

  HUlib_clearTextLine(&w_keys);      // clear the widget strings

  // transfer the built string (frags or key title) to the widget
  s = hud_keysstr; //jff 3/7/98 display key titles/key text or frags
  while (*s)
    HUlib_addCharToTextLine(&w_keys, *(s++));

  //jff 3/17/98 show graphic keys in non-DM only
  if (!deathmatch) //jff 3/7/98 display graphic keys
  {
    // clear the widget strings
    HUlib_clearTextLine(&w_gkeys);
    // transfer the graphic key text to the widget
    s = hud_gkeysstr;
    while (*s)
      HUlib_addCharToTextLine(&w_gkeys, *(s++));
  }

  w_gkeys.x = w_keys.x + 20;
  w_gkeys.y = w_keys.y;
  w_gkeys.flags = w_keys.flags;
}

void HU_widget_draw_keys(void)
{
  HUlib_drawTextLine(&w_keys, false);
  if (!deathmatch)
  {
    HUlib_drawTextLine(&w_gkeys, false);
  }
}

void HU_widget_build_monsec(void)
{
  int i;
  char *s;

  //e6y
  char allkills[200], allsecrets[200];
  int playerscount;
  int  fullkillcount, fullitemcount, fullsecretcount;
  int color, killcolor, itemcolor, secretcolor;

  // clear the internal widget text buffer
  HUlib_clearTextLine(&w_monsec);
  if (!hudadd_smarttotals || deathmatch)
  {
    sprintf
      (
      hud_monsecstr,
      "STS \x1b\x36K \x1b\x33%d \x1b\x36M \x1b\x33%d \x1b\x37I \x1b\x33%d/%d \x1b\x35S \x1b\x33%d/%d",
      plr->killcount,totallive,
      plr->itemcount,totalitems,
      plr->secretcount,totalsecret
      );
  }
  else
  {
    int allkills_len = 0;
    int allsecrets_len = 0;

    playerscount = 0;
    fullkillcount = 0;
    fullitemcount = 0;
    fullsecretcount = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i])
      {
        color = i==displayplayer?0x33:0x32;
        if (playerscount==0)
        {
          allkills_len = sprintf(allkills, "\x1b%c%d", color, players[i].killcount - players[i].resurectedkillcount);
          allsecrets_len = sprintf(allsecrets, "\x1b%c%d", color, players[i].secretcount);
        }
        else
        {
          if (allkills_len >= 0 && allsecrets_len >=0)
          {
            allkills_len += sprintf(&allkills[allkills_len], "\x1b%c+%d", color, players[i].killcount - players[i].resurectedkillcount);
            allsecrets_len += sprintf(&allsecrets[allsecrets_len], "\x1b%c+%d", color, players[i].secretcount);
          }
        }
        playerscount++;
        fullkillcount += players[i].killcount - players[i].resurectedkillcount;
        fullitemcount += players[i].itemcount;
        fullsecretcount += players[i].secretcount;
      }
    }
    killcolor = (fullkillcount >= totalkills ? 0x37 : 0x35);
    secretcolor = (fullsecretcount >= totalsecret ? 0x37 : 0x35);
    itemcolor = (fullitemcount >= totalitems ? 0x37 : 0x35);
    if (playerscount<2)
    {
      sprintf
        (
        hud_monsecstr,
        "STS \x1b\x36K \x1b%c%d/%d \x1b\x36I \x1b%c%d/%d \x1b\x36S \x1b%c%d/%d",
        killcolor, fullkillcount,totalkills,
        itemcolor,plr->itemcount,totalitems,
        secretcolor, fullsecretcount,totalsecret
        );
    }
    else
    {
      sprintf
        (
        hud_monsecstr,
        "STS \x1b\x36K %s \x1b%c%d/%d \x1b\x36I \x1b%c%d/%d \x1b\x36S %s \x1b%c%d/%d",
        allkills,killcolor,fullkillcount,totalkills,
        itemcolor,plr->itemcount,totalitems,
        allsecrets,secretcolor,fullsecretcount,totalsecret
        );
    }
  }

  // transfer the init string to the widget
  s = hud_monsecstr;
  while (*s)
    HUlib_addCharToTextLine(&w_monsec, *(s++));
}

void HU_widget_draw_monsec(void)
{
  HUlib_drawTextLine(&w_monsec, false);
}

void HU_widget_build_hudadd(void)
{
  char *s;
  hud_add[0] = 0;

  if (!hudadd_gamespeed && !hudadd_leveltime)
    return;

  if (hudadd_gamespeed)
    sprintf(hud_add,"\x1b\x32speed \x1b\x33%.2d ", realtic_clock_rate);
  if ((hudadd_leveltime) || (demoplayback && hudadd_demotime))
  {
    static char demo_len_null[1]={0};
    char *demo_len = demoplayback && hudadd_demotime ? demo_len_st : demo_len_null;
    if (totalleveltimes)
      sprintf(hud_add+strlen(hud_add),"\x1b\x32time \x1b\x35%d:%02d%s \x1b\x33%d:%05.2f ", 
      (totalleveltimes+leveltime)/35/60, ((totalleveltimes+leveltime)%(60*35))/35, demo_len, 
      leveltime/35/60, (float)(leveltime%(60*35))/35);
    else
      sprintf(hud_add+strlen(hud_add),"\x1b\x32time \x1b\x33%d:%05.2f%s ", 
      leveltime/35/60, (float)(leveltime%(60*35))/35, demo_len);
  }
  HUlib_clearTextLine(&w_hudadd);
  s = hud_add;
  while (*s)
    HUlib_addCharToTextLine(&w_hudadd, *(s++));
}

void HU_widget_draw_hudadd(void)
{
  if (hudadd_gamespeed || hudadd_leveltime)
  {
    HUlib_drawTextLine(&w_hudadd, false);
  }
}

void HU_widget_build_medict_percent(void)
{
  int health = plr->health;

  if (w_medict_percent.val != -1 && w_medict_percent.val == health)
    return;
  w_medict_percent.val = health;

  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_medict_percent);

  if (!sts_always_red)
  {
    if (sts_pct_always_gray)
      w_medict_percent.cm = CR_GRAY;
    else
      w_medict_percent.cm = HU_GetHealthColor(health, CR_BLUE2);
  }

  HUlib_addCharToTextLine(&w_medict_percent, (char)('!' + 12));
}

void HU_widget_draw_medict_percent(void)
{
  HUlib_drawTextLine(&w_medict_percent, false);
}

void HU_widget_build_armor_percent(void)
{
  int armor = plr->armorpoints;

  if (w_armor_percent.val != -1 && w_armor_percent.val == armor)
    return;
  w_armor_percent.val = armor;

  // transfer the graphic key text to the widget
  HUlib_clearTextLine(&w_armor_percent);

  if (!sts_always_red)
  {
    if (sts_pct_always_gray)
      w_armor_percent.cm = CR_GRAY;
    else
      w_armor_percent.cm = HU_GetArmorColor(armor, CR_BLUE2);
  }

  HUlib_addCharToTextLine(&w_armor_percent, (char)('!' + 13));
}

void HU_widget_draw_armor_percent(void)
{
  HUlib_drawTextLine(&w_armor_percent, false);
}

void HU_widget_build_ammo_big(void)
{
  char *s;
  char ammostr[80];
  int fullammo = plr->maxammo[weaponinfo[plr->readyweapon].ammo];

  // clear the widgets internal line
  HUlib_clearTextLine(&w_ammo_big);

  if (weaponinfo[plr->readyweapon].ammo != am_noammo && fullammo != 0)
  {
    int ammo = plr->ammo[weaponinfo[plr->readyweapon].ammo];

    // build the numeric amount init string
    sprintf(ammostr, "%d", ammo);

    // set the display color from the percentage of total ammo held
    if (!sts_always_red)
      w_ammo_big.cm = HU_GetAmmoColor(ammo, fullammo, CR_BLUE2,
        ammopershot[plr->readyweapon], plr->backpack);

    // transfer the init string to the widget
    s = ammostr;
    while (*s)
      HUlib_addCharToTextLine(&w_ammo_big, *(s++));
  }
}

void HU_widget_draw_ammo_big(void)
{
  HUlib_drawTextLine(&w_ammo_big, false);
}

void HU_widget_build_ammo_icon(void)
{
  int ammo = weaponinfo[plr->readyweapon].ammo;

  if (w_ammo_icon.val != -1 && w_ammo_icon.val == ammo)
    return;
  w_ammo_icon.val = ammo;

  if (ammo < NUMAMMO)
  {
    HUlib_clearTextLine(&w_ammo_icon);
    HUlib_addCharToTextLine(&w_ammo_icon, (char)('!' + ammo + 40));
  }
  else
  {
    HUlib_clearTextLine(&w_ammo_icon);
  }
}

void HU_widget_draw_ammo_icon(void)
{
  HUlib_drawTextLine(&w_ammo_icon, false);
}

void HU_widget_build_gkeys(void)
{
  int i, k;
  char *s;
  char gkeysstr[80];
  int mask = 0;

  // build text string whose characters call out graphic keys from fontk
  i = 0;
  for (k = 0; k < 6; k++)
  {
    // skip keys not possessed
    if (!plr->cards[k])
      continue;

    gkeysstr[i++] = '!' + k; // key number plus '!' is char for key
    gkeysstr[i++] = ' ';     // spacing
    gkeysstr[i++] = ' ';

    mask |= (1 << k);
  }

  if (w_keys_icon.val != -1 && w_keys_icon.val == mask)
    return;
  w_keys_icon.val = mask;

  gkeysstr[i] = '\0';
  while (((--i) > 0) && (gkeysstr[i] == ' '))
    gkeysstr[i] = '\0';

  // clear the widget strings
  HUlib_clearTextLine(&w_keys_icon);
  // transfer the graphic key text to the widget
  s = gkeysstr;
  while (*s)
    HUlib_addCharToTextLine(&w_keys_icon, *(s++));

}

void HU_widget_draw_gkeys(void)
{
  HUlib_drawTextLine(&w_keys_icon, false);
}

const char *crosshair_nam[HU_CROSSHAIRS]= { NULL, "CROSS1", "CROSS2", "CROSS3" };
const char *crosshair_str[HU_CROSSHAIRS]= { "none", "cross", "angle", "dot" };
crosshair_t crosshair;

void HU_init_crosshair(void)
{
  if (!hudadd_crosshair || !crosshair_nam[hudadd_crosshair])
    return;

  crosshair.lump = W_CheckNumForNameInternal(crosshair_nam[hudadd_crosshair]);
  if (crosshair.lump == -1)
    return;

  crosshair.w = R_NumPatchWidth(crosshair.lump);
  crosshair.h = R_NumPatchHeight(crosshair.lump);

  crosshair.flags = VPT_TRANS;
  if (hudadd_crosshair_scale)
    crosshair.flags |= VPT_STRETCH;
}

void SetCrosshairTarget(void)
{
  crosshair.target_screen_x = 0.0f;
  crosshair.target_screen_y = 0.0f;

  if (hudadd_crosshair_lock_target && crosshair.target_sprite >= 0)
  {
    float x, y, z;
    float winx, winy, winz;

    x = -(float)crosshair.target_x / MAP_SCALE;
    z =  (float)crosshair.target_y / MAP_SCALE;
    y =  (float)crosshair.target_z / MAP_SCALE;

    if (R_Project(x, y, z, &winx, &winy, &winz))
    {
      int top, bottom, h;
      stretch_param_t *params = &stretch_params[crosshair.flags & VPT_ALIGN_MASK];

      if (V_GetMode() != VID_MODEGL)
      {
        winy += (float)(viewheight/2 - centery);
      }

      top = SCREENHEIGHT - viewwindowy;
      h = crosshair.h;
      if (hudadd_crosshair_scale)
      {
        h = h * params->video->height / 200;
      }
      bottom = top - viewheight + h;
      winy = BETWEEN(bottom, top, winy);

      if (!hudadd_crosshair_scale)
      {
        crosshair.target_screen_x = winx;
        crosshair.target_screen_y = SCREENHEIGHT - winy;
      }
      else
      {
        crosshair.target_screen_x = (winx - params->deltax1) * 320.0f / params->video->width;
        crosshair.target_screen_y = 200 - (winy - params->deltay1) * 200.0f / params->video->height;
      }
    }
  }
}

void HU_draw_crosshair(void)
{
  int cm;

  crosshair.target_sprite = -1;

  if (!crosshair_nam[hudadd_crosshair] || crosshair.lump == -1 ||
    custom_message_p->ticks > 0 || automapmode & am_active ||
    menuactive != mnact_inactive || paused ||
    plr->readyweapon == wp_chainsaw || plr->readyweapon == wp_fist)
  {
    return;
  }

  if (hudadd_crosshair_health)
    cm = HU_GetHealthColor(plr->health, CR_BLUE2);
  else
    cm = hudadd_crosshair_color;

  if (hudadd_crosshair_target || hudadd_crosshair_lock_target)
  {
    fixed_t slope;
    angle_t an = plr->mo->angle;
    
    // intercepts overflow guard
    overflows_enabled = false;
    slope = P_AimLineAttack(plr->mo, an, 16*64*FRACUNIT, 0);
    if (plr->readyweapon == wp_missile || plr->readyweapon == wp_plasma || plr->readyweapon == wp_bfg)
    {
      if (!linetarget)
        slope = P_AimLineAttack(plr->mo, an += 1<<26, 16*64*FRACUNIT, 0);
      if (!linetarget)
        slope = P_AimLineAttack(plr->mo, an -= 2<<26, 16*64*FRACUNIT, 0);
    }
    overflows_enabled = true;

    if (linetarget && !(linetarget->flags & MF_SHADOW))
    {
      crosshair.target_x = linetarget->x;
      crosshair.target_y = linetarget->y;
      crosshair.target_z = linetarget->z;
      crosshair.target_z += linetarget->height / 2 + linetarget->height / 8;
      crosshair.target_sprite = linetarget->sprite;

      if (hudadd_crosshair_target)
        cm = hudadd_crosshair_target_color;
    }
  }

  SetCrosshairTarget();

  if (crosshair.target_screen_x != 0)
  {
    float x = crosshair.target_screen_x;
    float y = crosshair.target_screen_y;
    V_DrawNumPatchPrecise(x, y, 0, crosshair.lump, cm, crosshair.flags);
  }
  else
  {
    int x, y, st_height;

    if (!hudadd_crosshair_scale)
    {
      st_height = (viewheight != SCREENHEIGHT ? ST_SCALED_HEIGHT : 0);
      x = (SCREENWIDTH - crosshair.w) / 2;
      y = (SCREENHEIGHT - st_height - crosshair.h) / 2;
    }
    else
    {
      st_height = (viewheight != SCREENHEIGHT ? ST_HEIGHT : 0);
      x = (320 - crosshair.w) / 2;
      y = (200 - st_height - crosshair.h) / 2;
    }

    V_DrawNumPatch(x, y, 0, crosshair.lump, cm, crosshair.flags);
  }
}

//
// HU_Drawer()
//
// Draw all the pieces of the heads-up display
//
// Passed nothing, returns nothing
//
void HU_Drawer(void)
{
  char *s;
  player_t *plr;
  //jff 3/4/98 speed update up for slow systems
  //e6y: speed update for uncapped framerate
  static dboolean needupdate = false;
  if (realframe) needupdate = !needupdate;

  // don't draw anything if there's a fullscreen menu up
  if (menuactive == mnact_full)
    return;

  plr = &players[displayplayer];         // killough 3/7/98
  // draw the automap widgets if automap is displayed
  if (automapmode & am_active)
  {
    if (!(automapmode & am_overlay) || (viewheight != SCREENHEIGHT))//!hud_displayed)
    {
      // map title
      HUlib_drawTextLine(&w_title, false);
    }

    //jff 2/16/98 output new coord display
    // x-coord
    if (map_point_coordinates)
    {

      //e6y: speedup
      if (!realframe)
      {
        HUlib_drawTextLine(&w_coordx, false);
        HUlib_drawTextLine(&w_coordy, false);
        HUlib_drawTextLine(&w_coordz, false);
      }
      else
      {
        sprintf(hud_coordstrx,"X: %-5d", (plr->mo->x)>>FRACBITS);
        HUlib_clearTextLine(&w_coordx);
        s = hud_coordstrx;
        while (*s)
          HUlib_addCharToTextLine(&w_coordx, *(s++));
        HUlib_drawTextLine(&w_coordx, false);

        //jff 3/3/98 split coord display into x,y,z lines
        // y-coord
        sprintf(hud_coordstry,"Y: %-5d", (plr->mo->y)>>FRACBITS);
        HUlib_clearTextLine(&w_coordy);
        s = hud_coordstry;
        while (*s)
          HUlib_addCharToTextLine(&w_coordy, *(s++));
        HUlib_drawTextLine(&w_coordy, false);

        //jff 3/3/98 split coord display into x,y,z lines
        //jff 2/22/98 added z
        // z-coord
        sprintf(hud_coordstrz,"Z: %-5d", (plr->mo->z)>>FRACBITS);
        HUlib_clearTextLine(&w_coordz);
        s = hud_coordstrz;
        while (*s)
          HUlib_addCharToTextLine(&w_coordz, *(s++));
        HUlib_drawTextLine(&w_coordz, false);
      }
    }

    if (map_level_stat)
    {
      static char str[32];
      int time = leveltime / TICRATE;
      int ttime = (totalleveltimes + leveltime) / TICRATE;

      sprintf(str, "Monsters: \x1b%c%d/%d", '0' + hudcolor_mapstat_value,
        players[consoleplayer].killcount - players[consoleplayer].resurectedkillcount,
        totalkills);
      HUlib_clearTextLine(&w_map_monsters);
      s = str;
      while (*s)
        HUlib_addCharToTextLine(&w_map_monsters, *(s++));
      HUlib_drawTextLine(&w_map_monsters, false);

      sprintf(str, "Secrets: \x1b%c%d/%d", '0' + hudcolor_mapstat_value,
        players[consoleplayer].secretcount, totalsecret);
      HUlib_clearTextLine(&w_map_secrets);
      s = str;
      while (*s)
        HUlib_addCharToTextLine(&w_map_secrets, *(s++));
      HUlib_drawTextLine(&w_map_secrets, false);

      sprintf(str, "Items: \x1b%c%d/%d", '0' + hudcolor_mapstat_value,
        players[consoleplayer].itemcount, totalitems);
      HUlib_clearTextLine(&w_map_items);
      s = str;
      while (*s)
        HUlib_addCharToTextLine(&w_map_items, *(s++));
      HUlib_drawTextLine(&w_map_items, false);

      sprintf(str, "%02d:%02d:%02d", time/3600, (time%3600)/60, time%60);
      HUlib_clearTextLine(&w_map_time);
      s = str;
      while (*s)
        HUlib_addCharToTextLine(&w_map_time, *(s++));
      HUlib_drawTextLine(&w_map_time, false);

      if (totalleveltimes > 0)
      {
        sprintf(str, "%02d:%02d:%02d", ttime/3600, (ttime%3600)/60, ttime%60);
        HUlib_clearTextLine(&w_map_totaltime);
        s = str;
        while (*s)
          HUlib_addCharToTextLine(&w_map_totaltime, *(s++));
        HUlib_drawTextLine(&w_map_totaltime, false);
      }
    }
  }

  // draw the weapon/health/ammo/armor/kills/keys displays if optioned
  //jff 2/17/98 allow new hud stuff to be turned off
  // killough 2/21/98: really allow new hud stuff to be turned off COMPLETELY
  if
  (
    hud_num > 0 &&                   // hud optioned on
    hud_displayed &&                 // hud on from fullscreen key
    viewheight==SCREENHEIGHT &&      // fullscreen mode is active
    (!(automapmode & am_active) ||   // automap is not active
     (automapmode & am_overlay))
  )
  {
    int i;

    HU_MoveHud(false);                  // insure HUD display coords are correct

    if (hud_current)
    {
      for (i = 0; i < hud_current->count; i++)
      {
        if (hud_current->items[i].build && hud_current->items[i].draw)
        {
          if (realframe)
          {
            hud_current->items[i].build();
          }
          hud_current->items[i].draw();
        }
      }
    }

    //e6y
    if (traces_present)
    {
      int k, num = 0;
      for(k = 0; k < NUMTRACES; k++)
      {
        if (traces[k].count)
        {
          if (realframe)
          {
            w_traces[num].y = w_traces[0].y - num * 8;

            if (traces[k].ApplyFunc)
              traces[k].ApplyFunc(k);

            HUlib_clearTextLine(&w_traces[num]);
            s = traces[k].hudstr;
            while (*s)
              HUlib_addCharToTextLine(&w_traces[num], *(s++));

            if (traces[k].ResetFunc)
              traces[k].ResetFunc(k);
          }
          HUlib_drawTextLine(&w_traces[num], false);
          num++;
        }
      }
    }

  }

  //jff 3/4/98 display last to give priority
  HU_Erase(); // jff 4/24/98 Erase current lines before drawing current
              // needed when screen not fullsize

  //jff 4/21/98 if setup has disabled message list while active, turn it off
  if (hud_msg_lines<=1)
    message_list = false;

  // if the message review not enabled, show the standard message widget
  if (!message_list)
    HUlib_drawSText(&w_message);

  //e6y
  if (custom_message_p->ticks > 0)
    HUlib_drawTextLine(&w_centermsg, false);

  if (hudadd_crosshair)
    HU_draw_crosshair();

  // if the message review is enabled show the scrolling message review
  if (hud_msg_lines>1 && message_list)
    HUlib_drawMText(&w_rtext);

  // display the interactive buffer for chat entry
  HUlib_drawIText(&w_chat);
}

//
// HU_Erase()
//
// Erase hud display lines that can be trashed by small screen display
//
// Passed nothing, returns nothing
//
void HU_Erase(void)
{
  // erase the message display or the message review display
  if (!message_list)
    HUlib_eraseSText(&w_message);
  else
    HUlib_eraseMText(&w_rtext);

  //e6y
  if (custom_message_p->ticks > 0)
    HUlib_eraseTextLine(&w_centermsg);

  // erase the interactive text buffer for chat entry
  HUlib_eraseIText(&w_chat);

  // erase the automap title
  HUlib_eraseTextLine(&w_title);
}

//
// HU_Ticker()
//
// Update the hud displays once per frame
//
// Passed nothing, returns nothing
//
static dboolean bsdown; // Is backspace down?
static int bscounter;

void HU_Ticker(void)
{
  int i, rc;
  char c;

  // tick down message counter if message is up
  if (message_counter && !--message_counter)
  {
    message_on = false;
    message_nottobefuckedwith = false;
  }
  if (bsdown && bscounter++ > 9) {
    HUlib_keyInIText(&w_chat, (unsigned char)key_backspace);
    bscounter = 8;
  }

  // if messages on, or "Messages Off" is being displayed
  // this allows the notification of turning messages off to be seen
  if (showMessages || message_dontfuckwithme)
  {
    // display message if necessary
    if ((plr->message && !message_nottobefuckedwith)
        || (plr->message && message_dontfuckwithme))
    {
      //post the message to the message widget
      HUlib_addMessageToSText(&w_message, 0, plr->message);
      //jff 2/26/98 add message to refresh text widget too
      HUlib_addMessageToMText(&w_rtext, 0, plr->message);

      // clear the message to avoid posting multiple times
      plr->message = 0;
      // note a message is displayed
      message_on = true;
      // start the message persistence counter
      message_counter = HU_MSGTIMEOUT;
      // transfer "Messages Off" exception to the "being displayed" variable
      message_nottobefuckedwith = message_dontfuckwithme;
      // clear the flag that "Messages Off" is being posted
      message_dontfuckwithme = 0;
    }
  }
  
  // centered messages
  for (i = 0; i < MAXPLAYERS; i++)
  {
    if (custom_message[i].ticks > 0)
      custom_message[i].ticks--;
  }
  if (custom_message_p->msg)
  {
    const char *s = custom_message_p->msg; 
    HUlib_clearTextLine(&w_centermsg);
    while (*s)
    {
      HUlib_addCharToTextLine(&w_centermsg, *(s++));
    }
    HUlib_setTextXCenter(&w_centermsg);
    w_centermsg.cm = custom_message_p->cm;
    custom_message_p->msg = NULL;

    if (custom_message_p->sfx > 0 && custom_message_p->sfx < numsfx)
    {
      S_StartSound(NULL, custom_message_p->sfx);
    }
  }

  // check for incoming chat characters
  if (netgame)
  {
    for (i=0; i<MAXPLAYERS; i++)
    {
      if (!playeringame[i])
        continue;
      if (i != consoleplayer
          && (c = players[i].cmd.chatchar))
      {
        if (c <= HU_BROADCAST)
          chat_dest[i] = c;
        else
        {
          if (c >= 'a' && c <= 'z')
            c = (char) shiftxform[(unsigned char) c];
          rc = HUlib_keyInIText(&w_inputbuffer[i], c);
          if (rc && c == KEYD_ENTER)
          {
            if (w_inputbuffer[i].l.len
                && (chat_dest[i] == consoleplayer+1
                || chat_dest[i] == HU_BROADCAST))
            {
              HUlib_addMessageToSText(&w_message,
                                      player_names[i],
                                      w_inputbuffer[i].l.l);

              message_nottobefuckedwith = true;
              message_on = true;
              message_counter = HU_MSGTIMEOUT;
              if ( gamemode == commercial )
                S_StartSound(0, sfx_radio);
              else
                S_StartSound(0, sfx_tink);
            }
            HUlib_resetIText(&w_inputbuffer[i]);
          }
        }
        players[i].cmd.chatchar = 0;
      }
    }
  }
}

#define QUEUESIZE   128

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;

//
// HU_queueChatChar()
//
// Add an incoming character to the circular chat queue
//
// Passed the character to queue, returns nothing
//
static void HU_queueChatChar(char c)
{
  if (((head + 1) & (QUEUESIZE-1)) == tail)
  {
    plr->message = HUSTR_MSGU;
  }
  else
  {
    chatchars[head] = c;
    head = (head + 1) & (QUEUESIZE-1);
  }
}

//
// HU_dequeueChatChar()
//
// Remove the earliest added character from the circular chat queue
//
// Passed nothing, returns the character dequeued
//
char HU_dequeueChatChar(void)
{
  char c;

  if (head != tail)
  {
    c = chatchars[tail];
    tail = (tail + 1) & (QUEUESIZE-1);
  }
  else
  {
    c = 0;
  }
  return c;
}

//
// HU_Responder()
//
// Responds to input events that affect the heads up displays
//
// Passed the event to respond to, returns true if the event was handled
//
dboolean HU_Responder(event_t *ev)
{

  static char   lastmessage[HU_MAXLINELENGTH+1];
  const char*   macromessage; // CPhipps - const char*
  dboolean   eatkey = false;
  static dboolean  shiftdown = false;
  static dboolean  altdown = false;
  unsigned char   c;
  int     i;
  int     numplayers;

  static int    num_nobrainers = 0;

  numplayers = 0;
  for (i=0 ; i<MAXPLAYERS ; i++)
    numplayers += playeringame[i];

  if (ev->data1 == key_shift)
  {
    shiftdown = ev->type == ev_keydown;
    return false;
  }
  else if (ev->data1 == key_alt)
  {
    altdown = ev->type == ev_keydown;
    return false;
  }
  else if (ev->data1 == key_backspace)
  {
    bsdown = ev->type == ev_keydown;
    bscounter = 0;
  }

  if (ev->type != ev_keydown)
    return false;

  if (!chat_on)
  {
    if (ev->data1 == key_enter)                                 // phares
    {
#ifndef INSTRUMENTED  // never turn on message review if INSTRUMENTED defined
      if (hud_msg_lines>1)  // it posts multi-line messages that will trash
      {
        if (message_list) HU_Erase(); //jff 4/28/98 erase behind messages
        message_list = !message_list; //jff 2/26/98 toggle list of messages
      }
#endif
      if (!message_list)              // if not message list, refresh message
      {
        message_on = true;
        message_counter = HU_MSGTIMEOUT;
      }
      eatkey = true;
    }//jff 2/26/98 no chat if message review is displayed
    // killough 10/02/98: no chat if demo playback
    // no chat in -solo-net mode
    else if (!demoplayback && !message_list && netgame && numplayers > 1)
    {
      if (ev->data1 == key_chat)
    {
      eatkey = chat_on = true;
      HUlib_resetIText(&w_chat);
      HU_queueChatChar(HU_BROADCAST);
    }
    else if (numplayers > 2)
    {
      for (i=0; i<MAXPLAYERS ; i++)
      {
        if (ev->data1 == destination_keys[i])
        {
          if (playeringame[i] && i!=consoleplayer)
          {
            eatkey = chat_on = true;
            HUlib_resetIText(&w_chat);
            HU_queueChatChar((char)(i+1));
            break;
          }
          else if (i == consoleplayer)
          {
            num_nobrainers++;
            if (num_nobrainers < 3)
                plr->message = HUSTR_TALKTOSELF1;
            else if (num_nobrainers < 6)
                plr->message = HUSTR_TALKTOSELF2;
            else if (num_nobrainers < 9)
                plr->message = HUSTR_TALKTOSELF3;
            else if (num_nobrainers < 32)
                plr->message = HUSTR_TALKTOSELF4;
            else
                plr->message = HUSTR_TALKTOSELF5;
          }
        }
      }
    }
    }
  }//jff 2/26/98 no chat functions if message review is displayed
  else if (!message_list)
  {
    c = ev->data1;
    // send a macro
    if (altdown)
    {
      c = c - '0';
      if (c > 9)
        return false;
      macromessage = chat_macros[c];

      // kill last message with a '\n'
        HU_queueChatChar((char)key_enter); // DEBUG!!!                // phares

      // send the macro message
      while (*macromessage)
        HU_queueChatChar(*macromessage++);
      HU_queueChatChar((char)key_enter);                            // phares

      // leave chat mode and notify that it was sent
      chat_on = false;
      strcpy(lastmessage, chat_macros[c]);
      plr->message = lastmessage;
      eatkey = true;
    }
    else
    {
      if (shiftdown || (c >= 'a' && c <= 'z'))
        c = shiftxform[c];
      eatkey = HUlib_keyInIText(&w_chat, c);
      if (eatkey)
        HU_queueChatChar(c);

      if (c == key_enter)                                     // phares
      {
        chat_on = false;
        if (w_chat.l.len)
        {
          strcpy(lastmessage, w_chat.l.l);
          plr->message = lastmessage;
        }
      }
      else if (c == key_escape)                               // phares
        chat_on = false;
    }
  }
  return eatkey;
}

void T_ShowMessage (message_thinker_t* message)
{
  if (--message->delay > 0)
    return;

  SetCustomMessage(message->plr, message->msg.msg, 0,
    message->msg.ticks, message->msg.cm, message->msg.sfx);

  P_RemoveThinker(&message->thinker); // unlink and free
}

int SetCustomMessage(int plr, const char *msg, int delay, int ticks, int cm, int sfx)
{
  custom_message_t item;

  if (plr < 0 || plr >= MAXPLAYERS || !msg || ticks < 0 ||
      sfx < 0 || sfx >= numsfx || cm < 0 || cm >= CR_LIMIT)
  {
    return false;
  }

  item.msg = msg;
  item.ticks = ticks;
  item.cm = cm;
  item.sfx = sfx;

  if (delay <= 0)
  {
    custom_message[plr] = item;
  }
  else
  {
    message_thinker_t *message = Z_Calloc(1, sizeof(*message), PU_LEVEL, NULL);
    message->thinker.function = T_ShowMessage;
    message->delay = delay;
    message->plr = plr;

    message->msg = item;

    P_AddThinker(&message->thinker);
  }

  return true;
}
