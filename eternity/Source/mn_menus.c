// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
// Menus
//
// the actual menus: structs and handler functions (if any)
// console commands to activate each menu
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "d_io.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_main.h"
#include "dstrings.h"
#include "dhticstr.h" // haleyjd
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "m_random.h"
#include "mn_engin.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"
#include "d_dialog.h" // haleyjd
#include "mn_htic.h"
#include "d_gi.h"
#include "g_dmflag.h"
#include "e_states.h"

// haleyjd 04/15/02: SDL joystick stuff
#ifdef _SDL_VER
#include "i_system.h"
#endif

// menus: all in this file (not really extern)
extern menu_t menu_newgame;
extern menu_t menu_main;
extern menu_t menu_episode;
extern menu_t menu_startmap;

// This is the original cfg variable, now reintroduced.
int traditional_menu;

// Blocky mode, has default, 0 = high, 1 = normal
//int     detailLevel;    obsolete -- killough
int screenSize;      // screen size

char *mn_phonenum;           // phone number to dial
char *mn_demoname;           // demo to play
char *mn_wadname;            // wad to load

// haleyjd: moved these up here to fix Z_Free error

// haleyjd: was 7
#define SAVESLOTS 8

char *savegamenames[SAVESLOTS];

// haleyjd: keep track of valid save slots
boolean savegamepresent[SAVESLOTS];

static void MN_PatchOldMainMenu(void);

void MN_InitMenus(void)
{
   int i; // haleyjd
   
   mn_phonenum = Z_Strdup("555-1212", PU_STATIC, 0);
   mn_demoname = Z_Strdup("demo1", PU_STATIC, 0);
   mn_wadname = Z_Strdup("", PU_STATIC, 0);
   
   // haleyjd: initialize via zone memory
   for(i = 0; i < SAVESLOTS; ++i)
   {
      savegamenames[i] = Z_Strdup("", PU_STATIC, 0);
      savegamepresent[i] = false;
   }

   if(gamemode == commercial)
      MN_PatchOldMainMenu(); // haleyjd 05/16/04
}

//////////////////////////////////////////////////////////////////////////
//
// THE MENUS
//
//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
//
// Main Menu
//

void MN_MainMenuDrawer();

menu_t menu_main =
{
  {
    // 'doom' title drawn by the drawer
    
    {it_runcmd, "new game",             "mn_newgame",            "M_NGAME"},
    {it_runcmd, "options",              "mn_optfeat",            "M_OPTION"},
    {it_runcmd, "load game",            "mn_loadgame",           "M_LOADG"},
    {it_runcmd, "save game",            "mn_savegame",           "M_SAVEG"},
    {it_runcmd, "end game",             "mn_endgame",            "M_ENDGAM"},
    {it_runcmd, "quit",                 "mn_quit",               "M_QUITG"},
    {it_end},
  },
  100, 65,                // x, y offsets
  0,                     // start with 'new game' selected
  mf_skullmenu,          // a skull menu
  MN_MainMenuDrawer
};

// haleyjd 05/16/04: traditional DOOM main menu support

menu_t menu_old_main =
{
   {
      { it_runcmd, "new game",   "mn_newgame",  "M_NGAME" },
      { it_runcmd, "options",    "mn_optfeat",  "M_OPTION" },
      { it_runcmd, "save game",  "mn_savegame", "M_SAVEG" },
      { it_runcmd, "load game",  "mn_loadgame", "M_LOADG" },
      { it_runcmd, "read this!", "credits",     "M_RDTHIS" },
      { it_runcmd, "quit",       "mn_quit",     "M_QUITG" },
      { it_end }
   },
   97, 64,
   0,
   mf_skullmenu,
   MN_MainMenuDrawer
};

//
// MN_PatchOldMainMenu
//
// haleyjd 05/16/04: patches the old main menu for DOOM II, for full
// compatibility.
//
static void MN_PatchOldMainMenu(void)
{
   // turn "Read This!" into "Quit Game" and move down 8 pixels
   menu_old_main.menuitems[4] = menu_old_main.menuitems[5];
   menu_old_main.menuitems[5].type = it_end;
   menu_old_main.y += 8;
}

void MN_MainMenuDrawer(void)
{
   // hack for m_doom compatibility
   V_DrawPatch(94, 2, &vbscreen, W_CacheLumpName("M_DOOM", PU_CACHE));
}

// mn_newgame called from main menu:
// goes to start map OR
// starts menu
// according to use_startmap, gametype and modifiedgame

CONSOLE_COMMAND(mn_newgame, 0)
{
   if(netgame && !demoplayback)
   {
      MN_Alert(s_NEWGAME);
      return;
   }
   
   if(gamemode == commercial)
   {
      // determine startmap presence and origin
      int startMapLump = W_CheckNumForName("START");
      boolean mapPresent = true;

      // if lump not found or the game is modified and the
      // lump comes from the first loaded wad, consider it not
      // present -- this assumes the resource wad is loaded first.
      if(startMapLump < 0 || 
         (modifiedgame && 
          lumpinfo[startMapLump]->handle == firstWadHandle))
         mapPresent = false;


      // dont use new game menu if not needed
      if(!(modifiedgame && startOnNewMap) && use_startmap && mapPresent)
      {
         if(use_startmap == -1)              // not asked yet
            MN_StartMenu(&menu_startmap);
         else
         {  
            // use start map 
            G_DeferedInitNew(defaultskill, "START");
            MN_ClearMenus();
         }
      }
      else
         MN_StartMenu(&menu_newgame);
   }
   else
   {
      // hack -- cut off thy flesh consumed if not retail
      if(gamemode != retail)
         menu_episode.menuitems[5].type = it_end;
      
      MN_StartMenu(&menu_episode);
   }
}

// menu item to quit doom:
// pop up a quit message as in the original

CONSOLE_COMMAND(mn_quit, 0)
{
   int quitmsgnum;
   char quitmsg[128];

   if(cmdtype != c_menu && menuactive)
      return;
   
   quitmsgnum = M_Random() % 14;

   // sf: use s_QUITMSG if it has been replaced in a dehacked file
   // haleyjd 1/17/00: fixed this to work with NULL pointer 
   // dereference fix in d_deh.c

   if(strcmp(s_QUITMSG, ""))
   {
      // support dehacked replacement
      psnprintf(quitmsg, sizeof(quitmsg), "%s\n\n%s", 
                s_QUITMSG, s_DOSY);
   }
   else if(gameModeInfo->type == Game_Heretic)
   {
      // haleyjd: heretic support
      // I went ahead and kept the s_DOSY because its better to
      // have it there. Heretic just asked the plain question.
      psnprintf(quitmsg, sizeof(quitmsg), "%s\n\n%s", 
                HTICQUITMSG, s_DOSY);
   }
   else
   {
      // normal doom messages
      psnprintf(quitmsg, sizeof(quitmsg), "%s\n\n%s", 
                endmsg[quitmsgnum], s_DOSY);
   }
   
   MN_Question(quitmsg, "quit");
}

/////////////////////////////////////////////////////////
//
// Episode Selection
//

int start_episode;

menu_t menu_episode =
{
  {
    {it_title,  "which episode?",            NULL,            "M_EPISOD"},
    {it_gap},
    {it_runcmd, "knee deep in the dead",     "mn_episode 1",  "M_EPI1"},
    {it_runcmd, "the shores of hell",        "mn_episode 2",  "M_EPI2"},
    {it_runcmd, "inferno!",                  "mn_episode 3",  "M_EPI3"},
    {it_runcmd, "thy flesh consumed",        "mn_episode 4",  "M_EPI4"},
    {it_end},
  },
  40, 30,              // x, y offsets
  2,                   // select episode 1
  mf_skullmenu,        // skull menu
};

// console command to select episode

CONSOLE_COMMAND(mn_episode, cf_notnet)
{
   if(!c_argc)
   {
      C_Printf("usage: episode <epinum>\n");
      return;
   }
   
   start_episode = atoi(c_argv[0]);
   
   if(gamemode == shareware && start_episode > 1)
   {
      MN_Alert(s_SWSTRING);
      return;
   }
   
   MN_StartMenu(&menu_newgame);
}

//////////////////////////////////////////////////////////
//
// New Game Menu: Skill Level Selection
//

menu_t menu_newgame =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "choose skill level",   NULL,                   "M_SKILL"},
    {it_gap},
    {it_runcmd, "i'm too young to die", "newgame 0",            "M_JKILL"},
    {it_runcmd, "hey, not too rough",   "newgame 1",            "M_ROUGH"},
    {it_runcmd, "hurt me plenty.",      "newgame 2",            "M_HURT"},
    {it_runcmd, "ultra-violence",       "newgame 3",            "M_ULTRA"},
    {it_runcmd, "nightmare!",           "newgame 4",            "M_NMARE"},
    {it_end},
  },
  40, 15,               // x,y offsets
  6,                    // starting item: hurt me plenty
  mf_skullmenu,         // is a skull menu
};

static void MN_DoNightmare(void)
{
   if(gamemode == commercial && modifiedgame && startOnNewMap)
   {
      // start on newest level from wad
      G_DeferedInitNew(sk_nightmare, firstlevel);
   }
   else
   {
      // start on first level of selected episode
      G_DeferedInitNewNum(sk_nightmare, start_episode, 1);
   }
   
   MN_ClearMenus();
}

CONSOLE_COMMAND(newgame, cf_notnet)
{
   int skill = gameskill;
   
   // skill level is argv 0
   
   if(c_argc)
      skill = atoi(c_argv[0]);

   // haleyjd 07/27/05: restored nightmare behavior
   if(skill == sk_nightmare)
   {
      MN_QuestionFunc(s_NIGHTMARE, MN_DoNightmare);
      return;
   }
   
   // haleyjd 03/02/03: changed to use config variable
   if(gamemode == commercial && modifiedgame && startOnNewMap)
   {
      // start on newest level from wad
      G_DeferedInitNew(skill, firstlevel);
   }
   else
   {
      // start on first level of selected episode
      G_DeferedInitNewNum(skill, start_episode, 1);
   }
   
   MN_ClearMenus();
}

//////////////////////////////////////////////////
//
// First-time Query menu to use start map
//

menu_t menu_startmap =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "Eternity includes a 'start map' to let"},
    {it_info,   "you start new games from in a level."},
    {it_gap},
    {it_info,   FC_GOLD "in the future would you rather:"},
    {it_gap},
    {it_runcmd, "use the start map",            "use_startmap 1; mn_newgame"},
    {it_runcmd, "use the menu",                 "use_startmap 0; mn_newgame"},
    {it_end},
  },
  40, 15,               // x,y offsets
  7,                    // starting item: start map
  mf_leftaligned | mf_background, 
};

char *str_startmap[] = {"ask", "no", "yes"};
VARIABLE_INT(use_startmap, NULL, -1, 1, str_startmap);
CONSOLE_VARIABLE(use_startmap, use_startmap, 0) {}

// haleyjd 04/18/03: rearranged menus

menu_t menu_optfeat =
{
   {
      {it_title,  FC_GOLD "options",  NULL,          "M_OPTION"},
      {it_gap},
      {it_gap},
      {it_runcmd, "setup",            "mn_options",  "M_SETUP",  MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "features",         "mn_features", "M_FEAT",   MENUITEM_BIGFONT},
      {it_end},
   },
   100, 15,
   3,
   mf_leftaligned | mf_skullmenu
};

CONSOLE_COMMAND(mn_optfeat, 0)
{
   MN_StartMenu(&menu_optfeat);
}

/////////////////////////////////////////////////////
//
// Features Menu
//
// Access to new SMMU features
//

menu_t menu_features =
{
   {
      {it_title, FC_GOLD "features", NULL,         "M_FEAT"},
      {it_gap},
      {it_gap},
      {it_runcmd, "player setup",    "mn_player",  "M_PLAYER", MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "game settings",   "mn_gset",    "M_GSET",   MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "multiplayer",     "mn_multi",   "M_MULTI",  MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "load wad",        "mn_loadwad", "M_WAD",    MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "demos",           "mn_demos",   "M_DEMOS",  MENUITEM_BIGFONT},
      {it_gap},
      {it_runcmd, "about",           "credits",    "M_ABOUT",  MENUITEM_BIGFONT},
      {it_end},
   },
   100, 15,
   3,
   mf_leftaligned | mf_skullmenu
};

CONSOLE_COMMAND(mn_features, 0)
{
   MN_StartMenu(&menu_features);
}

////////////////////////////////////////////////
//
// Demos Menu
//
// Allow Demo playback and (recording),
// also access to camera angles
//

menu_t menu_demos = 
{
  {
    {it_title,      FC_GOLD "demos",          NULL,             "m_demos"},
    {it_gap},
    {it_info,       FC_GOLD "play demo"},
    {it_variable,   "demo name",              "mn_demoname"},
    {it_gap},
    {it_runcmd,     "play demo",              "mn_clearmenus; playdemo %mn_demoname"},
    {it_runcmd,     "time demo",              "mn_clearmenus; timedemo %mn_demoname 1"},
    {it_runcmd,     "stop playing demo",      "mn_clearmenus; stopdemo"},
    {it_gap},
    {it_info,       FC_GOLD "cameras"},
    {it_toggle,     "viewpoint changes",      "cooldemo"},
    {it_toggle,     "chasecam",               "chasecam"},
    {it_toggle,     "walkcam",                "walkcam"},
    {it_gap},
    {it_end},
  },
  150, 40,           // x,y
  3,                // start item
  mf_background,    // full screen
};

VARIABLE_STRING(mn_demoname,     NULL,           12);
CONSOLE_VARIABLE(mn_demoname,    mn_demoname,     0) {}

CONSOLE_COMMAND(mn_demos, cf_notnet)
{
   MN_StartMenu(&menu_demos);
}

//////////////////////////////////////////////////////////////////
//
// Load new pwad menu
//
// Using SMMU dynamic wad loading
//

menu_t menu_loadwad =
{
  {
    {it_title,     FC_GOLD "load wad",    NULL,                   "M_WAD"},
    {it_gap},
    {it_info,      FC_GOLD "load wad"},
    {it_variable,  "wad name",          "mn_wadname"},
    {it_runcmd,    "select wad...",     "mn_selectwad"},
    {it_gap},
    {it_runcmd,    "load wad",          "addfile %mn_wadname; starttitle"},
    {it_end},
  },
  150, 40,                     // x,y offsets
  3,                          // starting item
  mf_background               // full screen 
};

VARIABLE_STRING(mn_wadname,     NULL,           15);
CONSOLE_VARIABLE(mn_wadname,    mn_wadname,     0) {}

CONSOLE_COMMAND(mn_loadwad, cf_notnet)
{
   // haleyjd: generalized to all shareware modes
   if(gameModeInfo->flags & GIF_SHAREWARE)
   {
      MN_Alert("You must purchase the full version\n"
               "to load external wad files.\n"
               "\n"
               "%s", s_PRESSKEY);
      return;
   }
   
   MN_StartMenu(&menu_loadwad);
}

//////////////////////////////////////////////////////////////
//
// Multiplayer Menu
//
// Access to the new Multiplayer features of SMMU
//

//
// NETCODE_FIXME: Eliminate this and consolidate all unique options into
// the normal setup menu hierarchy. I do not intend to support the
// starting of netgames from within the engine, as it never even worked
// under SMMU to begin with (the one time this was tested, it crashed).
// Netgames will have to be started from the command line or via use of
// a launcher utility, like for most other ports.
//

menu_t menu_multiplayer =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_gap},
    {it_info,   FC_GOLD "connect:"},
    {it_runcmd, "serial/modem...",         "mn_serial"},
    {it_runcmd, "tcp/ip...",               "mn_tcpip"},
    {it_gap},
    {it_runcmd, "disconnect",              "disconnect"},
    {it_gap},
    {it_info,   FC_GOLD "setup"},
    {it_runcmd, "chat macros...",          "mn_chatmacros"},
    {it_runcmd, "player setup...",         "mn_player"},
    {it_runcmd, "game settings...",        "mn_gset"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  4,                                            // starting item
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_multi, 0)
{
   MN_StartMenu(&menu_multiplayer);
}
  
/////////////////////////////////////////////////////////////////
//
// Multiplayer Game settings
//

menu_t menu_gamesettings =
{
  {
    {it_title,    FC_GOLD "game settings",        NULL,       "M_GSET"},
    {it_gap},
    {it_info,     FC_GOLD "game settings"},
    {it_toggle,   "game type",                  "gametype"},
    {it_variable, "starting level",             "startlevel"},
    {it_toggle,   "skill level",                "skill"},
    {it_runcmd,   "advanced...",                "mn_advanced"},
    {it_runcmd,   "deathmatch flags...",        "mn_dmflags"},
    {it_runcmd,   "eternity options...",        "mn_etccompat"},
    {it_gap},
    {it_info,     FC_GOLD "dm auto-exit"},
    {it_variable, "time limit",                 "timelimit"},
    {it_variable, "frag limit",                 "fraglimit"},
    {it_end},
  },
  170, 15,
  3,                            // start
  mf_background,                // full screen
};

        // level to start on
VARIABLE_STRING(startlevel,    NULL,   8);
CONSOLE_VARIABLE(startlevel, startlevel, cf_handlerset)
{
   char *newvalue = c_argv[0];
   
   // check for a valid level
   if(W_CheckNumForName(newvalue) == -1)
      MN_ErrorMsg("level not found!");
   else
   {
      if(startlevel)
         Z_Free(startlevel);
      startlevel = Z_Strdup(newvalue, PU_STATIC, 0);
   }
}

CONSOLE_COMMAND(mn_gset, 0)            // just setting options from menu
{
   MN_StartMenu(&menu_gamesettings);
}

/////////////////////////////////////////////////////////////////
//
// Multiplayer Game settings
// Advanced menu
//

menu_t menu_advanced =
{
  {
    {it_title,    FC_GOLD "advanced",           NULL,             "M_MULTI"},
    {it_gap},
    {it_runcmd,   "done...",                    "mn_prevmenu"},
    {it_gap},
    {it_toggle,   "no monsters",                "nomonsters"},
    {it_toggle,   "fast monsters",              "fast"},
    {it_toggle,   "respawning monsters",        "respawn"},
    {it_gap},
    {it_toggle,   "allow mlook",                "allowmlook"},
    /*
    YSHEAR_FIXME: this feature may return after EDF for weapons
    {it_toggle,   "allow mlook with bfg",       "bfglook"},
    */
    {it_toggle,   "allow autoaim",              "autoaim"},
    {it_variable, "weapon change time",         "weapspeed"},
    {it_gap},
    {it_toggle,   "variable friction",          "varfriction"},
    {it_toggle,   "boom pusher objects",        "pushers"},
    {it_toggle,   "damaging floors",            "nukage"},
    {it_end},
  },
  170, 15,
  2,                            // start
  mf_background,                // full screen
};

CONSOLE_COMMAND(mn_advanced, cf_server)
{
   MN_StartMenu(&menu_advanced);
}

//
// Deathmatch Flags Menu
//

//
// NETCODE_FIXME -- CONSOLE_FIXME: Dm flags may require special treatment
//

static void MN_DMFlagsDrawer(void);

menu_t menu_dmflags =
{
   {
      {it_title,    FC_GOLD "deathmatch flags",   NULL,             "M_DMFLAG"},
      {it_gap},
      {it_runcmd,   "done...",                    "mn_prevmenu" },
      {it_gap},
      {it_runcmd,   "items respawn",              "mn_dfitem"},
      {it_runcmd,   "weapons stay",               "mn_dfweapstay"},
      {it_runcmd,   "respawning barrels",         "mn_dfbarrel"},
      {it_runcmd,   "players drop items",         "mn_dfplyrdrop"},
      {it_runcmd,   "respawning super items",     "mn_dfrespsupr"},
      {it_gap},
      {it_info,     "dmflags =" },
      {it_end},
   },
   170, 15,
   2,
   mf_background, // full screen
   MN_DMFlagsDrawer
};

CONSOLE_COMMAND(mn_dmflags, cf_server)
{
   MN_StartMenu(&menu_dmflags);
}

// haleyjd 04/14/03: dmflags menu drawer (a big hack, mostly)

static void MN_DMFlagsDrawer(void)
{
   int i;
   char buf[64];
   const char *values[2] = { "on", "off" };
   menuitem_t *menuitem;

   // don't draw anything before the menu has been initialized
   if(!(menu_dmflags.menuitems[10].flags & MENUITEM_POSINIT))
      return;

   for(i = 4; i < 9; i++)
   {
      menuitem = &(menu_dmflags.menuitems[i]);
                  
      V_WriteTextColoured
        (
         values[!(dmflags & (1<<(i-4)))],
         (i == menu_dmflags.selected) ? CR_GRAY : CR_RED,
         menuitem->x + 20, menuitem->y
        );
   }

   menuitem = &(menu_dmflags.menuitems[10]);
   // draw dmflags value
   psnprintf(buf, sizeof(buf), "%lu", dmflags);
   V_WriteText(buf, menuitem->x + 4, menuitem->y);
}

static void toggle_dm_flag(unsigned long flag)
{
   char cmdbuf[64];
   dmflags ^= flag;
   psnprintf(cmdbuf, sizeof(cmdbuf), "dmflags %lu", dmflags);
   C_RunTextCmd(cmdbuf);
}

CONSOLE_COMMAND(mn_dfitem, cf_server|cf_hidden)
{
   toggle_dm_flag(DM_ITEMRESPAWN);
}

CONSOLE_COMMAND(mn_dfweapstay, cf_server|cf_hidden)
{
   toggle_dm_flag(DM_WEAPONSTAY);
}

CONSOLE_COMMAND(mn_dfbarrel, cf_server|cf_hidden)
{
   toggle_dm_flag(DM_BARRELRESPAWN);
}

CONSOLE_COMMAND(mn_dfplyrdrop, cf_server|cf_hidden)
{
   toggle_dm_flag(DM_PLAYERDROP);
}

CONSOLE_COMMAND(mn_dfrespsupr, cf_server|cf_hidden)
{
   toggle_dm_flag(DM_RESPAWNSUPER);
}

/////////////////////////////////////////////////////////////////
//
// TCP/IP Menu
//
// When its done!
//

//
// NETCODE_FIXME: Ditch this.
//

menu_t menu_tcpip =
{
  {
    {it_title,  FC_GOLD "TCP/IP",            NULL,           "M_TCPIP"},
    {it_gap},
    {it_info,   "not implemented yet. :)"},
    {it_runcmd, "return...",                       "mn_prevmenu"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // full-screen
};

CONSOLE_COMMAND(mn_tcpip, 0)
{
   MN_StartMenu(&menu_tcpip);
}

/////////////////////////////////////////////////////////////////
//
// Serial/Modem Game
//

//
// NETCODE_FIXME: Ditch this.
//

menu_t menu_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,           FC_GOLD "settings"},
    {it_toggle,         "com port to use",      "com"},
    {it_variable,       "phone number",         "mn_phonenum"},
    {it_gap},
    {it_info,           FC_GOLD "connect:"},
    {it_runcmd,         "null modem link",      "mn_ser_connect"},
    {it_runcmd,         "dial",                 "dial %mn_phonenum"},
    {it_runcmd,         "wait for call",        "mn_ser_answer"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(mn_serial, 0)
{
   MN_StartMenu(&menu_serial);
}

VARIABLE_STRING(mn_phonenum,     NULL,           126);
CONSOLE_VARIABLE(mn_phonenum,    mn_phonenum,     0) {}

CONSOLE_COMMAND(mn_ser_answer, 0)           // serial wait-for-call
{
   C_SetConsole();               // dont want demos interfering
   C_RunTextCmd("answer");
}

CONSOLE_COMMAND(mn_ser_connect, 0)          // serial nullmodem
{
   C_SetConsole();               // dont want demos interfering
   C_RunTextCmd("nullmodem");
}

CONSOLE_COMMAND(mn_udpserv, 0)              // udp start server
{
   C_SetConsole();               // dont want demos interfering
   C_RunTextCmd("connect");
}

/////////////////////////////////////////////////////////////////
//
// Chat Macros
//

menu_t menu_chatmacros =
{
  {
    {it_title,  FC_GOLD "chat macros",           NULL,           "M_CHATM"},
    {it_gap},
    {it_variable,       "0",            "chatmacro0"},
    {it_variable,       "1",            "chatmacro1"},
    {it_variable,       "2",            "chatmacro2"},
    {it_variable,       "3",            "chatmacro3"},
    {it_variable,       "4",            "chatmacro4"},
    {it_variable,       "5",            "chatmacro5"},
    {it_variable,       "6",            "chatmacro6"},
    {it_variable,       "7",            "chatmacro7"},
    {it_variable,       "8",            "chatmacro8"},
    {it_variable,       "9",            "chatmacro9"},
    {it_end}
  },
  20,5,                                 // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen
};

CONSOLE_COMMAND(mn_chatmacros, 0)
{
   MN_StartMenu(&menu_chatmacros);
}

/////////////////////////////////////////////////////////////////
//
// Player Setup
//

void MN_PlayerDrawer(void);

menu_t menu_player =
{
  {
    {it_title,  FC_GOLD "player setup",           NULL,           "M_PLAYER"},
    {it_gap},
    {it_variable,       "player name",          "name"},
    {it_toggle,         "player colour",        "colour"},
    {it_toggle,         "player skin",          "skin"},
    {it_runcmd,         "skin viewer...",       "skinviewer"},
    {it_gap},
    {it_toggle,         "handedness",           "lefthanded"},
    {it_end}
  },
  150,5,                                // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen
  MN_PlayerDrawer
};

#define SPRITEBOX_X 200
#define SPRITEBOX_Y 80

void MN_PlayerDrawer(void)
{
   int lump;
   spritedef_t *sprdef;
   spriteframe_t *sprframe;
   patch_t *patch;
   
   V_DrawBox(SPRITEBOX_X, SPRITEBOX_Y, 80, 80);
   
   sprdef = &sprites[players[consoleplayer].skin->sprite];

   // haleyjd 08/15/02
   if(!(sprdef->spriteframes))
      return;
  
   sprframe = &sprdef->spriteframes[0];
   lump = sprframe->lump[1];
   
   patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);

   // haleyjd 01/12/04: changed translation handling
   V_DrawPatchTranslated
      (
       SPRITEBOX_X + 40,
       SPRITEBOX_Y + 70,
       &vbscreen,
       patch,
       players[consoleplayer].colormap ?
          (char *)translationtables[(players[consoleplayer].colormap - 1)] :
          NULL,
       false
      );
}

CONSOLE_COMMAND(mn_player, 0)
{
   MN_StartMenu(&menu_player);
}


/////////////////////////////////////////////////////////////////
//
// Load Game
//

//
// NETCODE_FIXME: Ensure that loading/saving are handled properly in
// netgames when it comes to the menus. Some deficiencies have already
// been caught in the past, so some may still exist.
//

// haleyjd: numerous fixes here from 8-17 version of SMMU

#define SAVESTRINGSIZE  24

// load/save box patches
patch_t *patch_left = NULL;
patch_t *patch_mid;
patch_t *patch_right;

void MN_SaveGame(void)
{
   int save_slot = 
      (char **)c_command->variable->variable - savegamenames;
   
   if(gamestate != GS_LEVEL) 
      return; // only save in level
   
   if(save_slot < 0 || save_slot >= SAVESLOTS)
      return; // sanity check
   
   G_SaveGame(save_slot, savegamenames[save_slot]);
   MN_ClearMenus();
   
   // haleyjd 02/23/02: restored from MBF
   if(quickSaveSlot == -2)
      quickSaveSlot = save_slot;
   
   // haleyjd: keep track of valid saveslots
   savegamepresent[save_slot] = true;
}

// create the savegame console commands
void MN_CreateSaveCmds(void)
{
   // haleyjd: something about the way these commands are being created
   //          is causing the console code to free a ptr with no zone id...
   //          08/08/00: fixed -- was trying to Z_Free a string on the C
   //                    heap - see initializers above in MN_InitMenus

   int i;
   
   for(i=0; i<SAVESLOTS; i++)  // haleyjd
   {
      command_t *save_command;
      variable_t *save_variable;
      char tempstr[16];

      // create the variable first
      save_variable = Z_Malloc(sizeof(*save_variable), PU_STATIC, 0); // haleyjd
      save_variable->variable = &savegamenames[i];
      save_variable->v_default = NULL;
      save_variable->type = vt_string;      // string value
      save_variable->min = 0;
      save_variable->max = SAVESTRINGSIZE;
      save_variable->defines = NULL;
      
      // now the command
      save_command = Z_Malloc(sizeof(*save_command), PU_STATIC, 0); // haleyjd
      
      sprintf(tempstr, "savegame_%i", i);
      save_command->name = strdup(tempstr);
      save_command->type = ct_variable;
      save_command->flags = 0;
      save_command->variable = save_variable;
      save_command->handler = MN_SaveGame;
      save_command->netcmd = 0;
      
      (C_AddCommand)(save_command); // hook into cmdlist
   }
}


//
// MN_ReadSaveStrings
//  read the strings from the savegame files
// based on the mbf sources
//
void MN_ReadSaveStrings(void)
{
   int i;
   
   for(i = 0; i < SAVESLOTS; i++)
   {
      char name[PATH_MAX+1];    // killough 3/22/98
      char description[SAVESTRINGSIZE]; // sf
      FILE *fp;  // killough 11/98: change to use stdio

      G_SaveGameName(name, sizeof(name), i);

      // haleyjd: fraggle got rid of this - perhaps cause of the crash?
      //          I've re-implemented it below to try to resolve the
      //          zoneid check error -- bingo, along with new init code.
      // if(savegamenames[i])
      //  Z_Free(savegamenames[i]);

      fp = fopen(name,"rb");
      if(!fp)
      {   // Ty 03/27/98 - externalized:
         // haleyjd
         if(savegamenames[i])
            Z_Free(savegamenames[i]);
         savegamenames[i] = Z_Strdup(s_EMPTYSTRING, PU_STATIC, 0);
         continue;
      }

      fread(description, SAVESTRINGSIZE, 1, fp);
      if(savegamenames[i])
         Z_Free(savegamenames[i]);
      savegamenames[i] = Z_Strdup(description, PU_STATIC, 0);  // haleyjd
      savegamepresent[i] = true;
      fclose(fp);
   }
}

void MN_DrawLoadBox(int x, int y)
{
   int i;
   
   if(!patch_left)        // initial load
   {
      patch_left = W_CacheLumpName("M_LSLEFT", PU_STATIC);
      patch_mid = W_CacheLumpName("M_LSCNTR", PU_STATIC);
      patch_right = W_CacheLumpName("M_LSRGHT", PU_STATIC);
   }

   V_DrawPatch(x, y, &vbscreen, patch_left);
   x += SHORT(patch_left->width);
   
   for(i=0; i<24; i++)
   {
      V_DrawPatch(x, y, &vbscreen, patch_mid);
      x += SHORT(patch_mid->width);
   }
   
   V_DrawPatch(x, y, &vbscreen, patch_right);
}

void MN_LoadGameDrawer(void);

// haleyjd: all saveslot names changed to be consistent

menu_t menu_loadgame =
{
  {
    {it_title,  FC_GOLD "load game",           NULL,              "M_LGTTL"},
    {it_gap},
    {it_runcmd, "save slot 0",                 "mn_load 0"},
    {it_gap},
    {it_runcmd, "save slot 1",                 "mn_load 1"},
    {it_gap},
    {it_runcmd, "save slot 2",                 "mn_load 2"},
    {it_gap},
    {it_runcmd, "save slot 3",                 "mn_load 3"},
    {it_gap},
    {it_runcmd, "save slot 4",                 "mn_load 4"},
    {it_gap},
    {it_runcmd, "save slot 5",                 "mn_load 5"},
    {it_gap},
    {it_runcmd, "save slot 6",                 "mn_load 6"},
    {it_gap},
    // haleyjd: the previously missing slot!
    {it_runcmd, "save slot 7",                 "mn_load 7"},
    {it_end},
  },
  50, 15,                           // x, y
  2,                                // starting slot
  mf_skullmenu | mf_leftaligned,    // skull menu
  MN_LoadGameDrawer,
};


void MN_LoadGameDrawer()
{
   int i, y;
   
   for(i=0, y=46; i<SAVESLOTS; i++, y+=16) // haleyjd
   {
      MN_DrawLoadBox(45, y);
   }
   
   // this is lame
   for(i=0, y=2; i<SAVESLOTS; i++, y+=2)  // haleyjd
   {
      menu_loadgame.menuitems[y].description =
         savegamenames[i] ? savegamenames[i] : s_EMPTYSTRING;
   }
}

CONSOLE_COMMAND(mn_loadgame, 0)
{
   if(netgame && !demoplayback)
   {
      MN_Alert(s_LOADNET);
      return;
   }
   
   // haleyjd 02/23/02: restored from MBF
   if(demorecording) // killough 5/26/98: exclude during demo recordings
   {
      MN_Alert("you can't load a game\n"
               "while recording a demo!\n\n"PRESSKEY);
      return;
   }
   
   MN_ReadSaveStrings();  // get savegame descriptions
   MN_StartMenu(&menu_loadgame);
}

CONSOLE_COMMAND(mn_load, 0)
{
   char name[PATH_MAX+1];     // killough 3/22/98
   int slot;
   
   if(c_argc < 1)
      return;
   
   slot = atoi(c_argv[0]);
   
   // haleyjd 08/25/02: giant bug here
   if(!savegamepresent[slot])
   {
      MN_Alert("You can't load an empty game!\n%s", s_PRESSKEY);
      return;     // empty slot
   }
   
   G_SaveGameName(name, sizeof(name), slot);
   G_LoadGame(name, slot, false);
   
   MN_ClearMenus();
}

// haleyjd 02/23/02: Quick Load -- restored from MBF and converted
// to use console commands
CONSOLE_COMMAND(quickload, 0)
{
   char tempstring[80];
   
   if(netgame && !demoplayback)
   {
      MN_Alert(s_QLOADNET);
      return;
   }

   if(demorecording)
   {
      MN_Alert("you can't quickload\n"
               "while recording a demo!\n\n"PRESSKEY);
      return;
   }

   if(quickSaveSlot < 0)
   {
      MN_Alert(s_QSAVESPOT);
      return;
   }
   
   psnprintf(tempstring, sizeof(tempstring),
             s_QLPROMPT, savegamenames[quickSaveSlot]);
   MN_Question(tempstring, "qload");
}

CONSOLE_COMMAND(qload, cf_hidden)
{
   char name[PATH_MAX+1];     // killough 3/22/98
   
   G_SaveGameName(name, sizeof(name), quickSaveSlot);
   G_LoadGame(name, quickSaveSlot, false);
}

/////////////////////////////////////////////////////////////////
//
// Save Game
//

// haleyjd: fixes continue here from 8-17 build

void MN_SaveGameDrawer(void)
{
   int i, y;
   
   for(i = 0, y = 46; i < SAVESLOTS; i++, y += 16) // haleyjd
   {
      MN_DrawLoadBox(45, y);
   }
}

menu_t menu_savegame = 
{
  {
    {it_title,  FC_GOLD "save game",           NULL,              "M_SGTTL"},
    {it_gap},
    {it_variable, "",                          "savegame_0"},
    {it_gap},
    {it_variable, "",                          "savegame_1"},
    {it_gap},
    {it_variable, "",                          "savegame_2"},
    {it_gap},
    {it_variable, "",                          "savegame_3"},
    {it_gap},
    {it_variable, "",                          "savegame_4"},
    {it_gap},
    {it_variable, "",                          "savegame_5"},
    {it_gap},
    {it_variable, "",                          "savegame_6"},
    // haleyjd: missing slot
    {it_gap},
    {it_variable, "",                          "savegame_7"},
    {it_end},
  },
  50, 15,                           // x, y
  2,                                // starting slot
  mf_skullmenu | mf_leftaligned,    // skull menu
  MN_SaveGameDrawer,
};

CONSOLE_COMMAND(mn_savegame, 0)
{
   // haleyjd 02/23/02: restored from MBF
   // killough 10/6/98: allow savegames during single-player demo 
   // playback
   
   if(!usergame && (!demoplayback || netgame))
   {
      MN_Alert(s_SAVEDEAD); // Ty 03/27/98 - externalized
      return;
   }
   
   if(gamestate != GS_LEVEL || cinema_pause || currentdialog)
      return;    // only save in levels -- haleyjd: never in cinemas
   
   MN_ReadSaveStrings();
   MN_StartMenu(&menu_savegame);
}

// haleyjd 02/23/02: Quick Save -- restored from MBF, converted to
// use console commands
CONSOLE_COMMAND(quicksave, 0)
{
   char tempstring[80];

   if(!usergame && (!demoplayback || netgame))  // killough 10/98
   {
      S_StartSound(NULL, sfx_oof);
      return;
   }
   
   if(gamestate != GS_LEVEL || cinema_pause || currentdialog)
      return;
  
   if(quickSaveSlot < 0)
   {
      quickSaveSlot = -2; // means to pick a slot now
      MN_ReadSaveStrings();
      MN_StartMenu(&menu_savegame);
      return;
   }
   
   psnprintf(tempstring, sizeof(tempstring),
             s_QSPROMPT, savegamenames[quickSaveSlot]);
   MN_Question(tempstring, "qsave");
}

CONSOLE_COMMAND(qsave, cf_hidden)
{
   G_SaveGame(quickSaveSlot, savegamenames[quickSaveSlot]);
}

/////////////////////////////////////////////////////////////////
//
// Options Menu
//
// Massively re-organised from the original version
//
// haleyjd 04/18/03: renamed to "setup"
//

menu_t menu_options =
{
  {
    {it_title,  FC_GOLD "setup",              NULL,             "M_SETUP"},
    {it_gap},
    {it_info,   FC_GOLD "input"},
    {it_runcmd, "key bindings",                 "mn_keybindings"},
    {it_runcmd, "mouse/gamepad options",        "mn_mouse"},
    {it_gap},
    {it_info,   FC_GOLD "output"},
    {it_runcmd, "video options",                "mn_video"},
    {it_runcmd, "sound options",                "mn_sound"},
    {it_gap},
    {it_info,   FC_GOLD "game options"},
    {it_runcmd, "compatibility",                "mn_compat"},
    {it_runcmd, "eternity options",             "mn_etccompat"}, // haleyjd
    {it_runcmd, "enemies",                      "mn_enemies"},
    {it_runcmd, "weapons",                      "mn_weapons"},
    {it_gap},
    {it_info,   FC_GOLD "game widgets"},
    {it_runcmd, "hud settings",                 "mn_hud"},
    {it_runcmd, "status bar",                   "mn_status"},
    {it_runcmd, "automap",                      "mn_automap"},
    {it_end},
  },
  100, 15,                              // x,y offsets
  3,                                    // starting item: first selectable
  mf_background|mf_leftaligned,         // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_options, 0)
{
   MN_StartMenu(&menu_options);
}

CONSOLE_COMMAND(mn_endgame, 0)
{
   // haleyjd 04/18/03: restored netgame behavior from MBF
   if(netgame)
   {
      MN_Alert("%s", s_NETEND);
      return;
   }

   MN_Question(s_ENDGAME, "starttitle");
}

/////////////////////////////////////////////////////////////////
//
// Video Options
//

void MN_VideoModeDrawer();

menu_t menu_video =
{
  {
    {it_title,        FC_GOLD "video",                NULL, "m_video"},
    {it_gap},
    {it_runcmd,       "set video mode...",            "mn_vidmode"},

    {it_gap},
    {it_info,         FC_GOLD "mode"},
    {it_toggle,       "wait for retrace",             "v_retrace"},
    {it_runcmd,       "test framerate..",             "timedemo demo2; mn_clearmenus"},
    {it_slider,       "gamma correction",             "gamma"},
    
    {it_gap},
    {it_info,         FC_GOLD "rendering"},
    {it_runcmd,       "particle options...",          "mn_particle"},
    {it_slider,       "screen size",                  "screensize"},
    {it_toggle,       "hom detector flashes",         "r_homflash"},
    {it_toggle,       "translucency",                 "r_trans"},
    {it_variable,     "translucency percentage",      "r_tranpct"},
    
    {it_gap},
    {it_info,         FC_GOLD "misc."},
#ifndef _SDL_VER
    {it_toggle,       "\"loading\" disk icon",        "v_diskicon"},
#endif
    {it_toggle,       "screenshot format",            "shot_type"},
    {it_toggle,       "text mode startup",            "textmode_startup"},
    
    {it_end},
  },
  200, 15,              // x,y offset
  2,                    // start on first selectable
  mf_background,        // full-screen menu
  MN_VideoModeDrawer
};

void MN_VideoModeDrawer(void)
{
   int lump, y;
   patch_t *patch;
   spritedef_t *sprdef;
   spriteframe_t *sprframe;
   int frame = E_SafeState(gameModeInfo->transFrame);

   // draw an imp fireball

   // don't draw anything before the menu has been initialized
   if(!(menu_video.menuitems[13].flags & MENUITEM_POSINIT))
      return;
   
   sprdef = &sprites[states[frame].sprite];
   // haleyjd 08/15/02
   if(!(sprdef->spriteframes))
      return;
   sprframe = &sprdef->spriteframes[0];
   lump = sprframe->lump[0];
   
   patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
   
   // approximately center box on "translucency" item in menu
   y = menu_video.menuitems[13].y - 5;
   V_DrawBox(270, y, 20, 20);
   V_DrawPatchTL(282, y + 12, &vbscreen, patch, NULL, FTRANLEVEL);
}

CONSOLE_COMMAND(mn_video, 0)
{
   MN_StartMenu(&menu_video);
}

menu_t menu_particles =
{
   {
      {it_title,        FC_GOLD "particle options", NULL, "m_video"},
      {it_gap},
      {it_toggle,       "render particle effects",     "draw_particles"},
      {it_gap},
      {it_toggle,       "blood splats",                "bloodsplattype"},
      {it_toggle,       "bullet puffs",                "bulletpufftype"},
      {it_toggle,       "rocket trails",               "rocket_trails"},
      {it_toggle,       "grenade trails",              "grenade_trails"},
      {it_toggle,       "bfg cloud",                   "bfg_cloud"},
      {it_toggle,       "rocket explosions",           "pevt_rexpl"},
      {it_toggle,       "bfg explosions",              "pevt_bfgexpl"},
      {it_end},
   },
   200, 15,              // x,y offset
   2,                    // start on first selectable
   mf_background,        // full-screen menu
};


CONSOLE_COMMAND(mn_particle, 0)
{
   MN_StartMenu(&menu_particles);
}

/////////////////////////////////////////////////////////////////
//
// Set vid mode

void MN_VidModeDrawer();

menu_t menu_vidmode =
{
   {
      {it_title,        FC_GOLD "video modes",       NULL, "m_video"},
      {it_gap},
      {it_info,         FC_GOLD "current mode:"},
      {it_info,         "(current mode)"},
      {it_gap},
      {it_info,         FC_GOLD "select video mode:"},
      // .... video modes filled in by console cmd function ......
      {it_end},
   },
   50, 15,              // x,y offset
   7,                    // start on first selectable
   mf_leftaligned|mf_background,        // full-screen menu
   MN_VidModeDrawer,
};

void MN_VidModeDrawer()
{
   menu_vidmode.menuitems[3].description = 
      videomodes[v_mode].description;
}

CONSOLE_COMMAND(mn_vidmode, 0)
{
   static boolean menu_built = false;
   
   // dont build multiple times
   if(!menu_built)
   {
      int menuitem, vidmode;
      char tempstr[20];
      
      // start on item 6
      
      for(menuitem=6, vidmode=0; videomodes[vidmode].description;
          menuitem++, vidmode++)
      {
         menu_vidmode.menuitems[menuitem].type = it_runcmd;
         menu_vidmode.menuitems[menuitem].description =
            videomodes[vidmode].description;
         sprintf(tempstr, "v_mode %i", vidmode);
         menu_vidmode.menuitems[menuitem].data = strdup(tempstr);
      }
     
      menu_vidmode.menuitems[menuitem].type = it_end; // mark end
          
      menu_built = true;
   }

   MN_StartMenu(&menu_vidmode);
}


/////////////////////////////////////////////////////////////////
//
// Sound Options
//

menu_t menu_sound =
{
  {
    {it_title,      FC_GOLD "sound",                NULL, "m_sound"},
    {it_gap},
    {it_info,       FC_GOLD "volume"},
    {it_slider,     "sfx volume",                   "sfx_volume"},
//#ifndef _SDL_VER
    {it_slider,     "music volume",                 "music_volume"},
//#endif
    {it_gap},
    {it_info,       FC_GOLD "setup"},
    {it_toggle,     "sound card",                   "snd_card"},
    {it_toggle,     "music card",                   "mus_card"},
    {it_toggle,     "autodetect voices",            "detect_voices"},
    {it_toggle,     "sound channels",               "snd_channels"},
    {it_toggle,     "force reverse stereo",         "s_flippan"},
    {it_gap},
    {it_info,       FC_GOLD "misc"},
    {it_toggle,     "precache sounds",              "s_precache"},
    {it_toggle,     "pitched sounds",               "s_pitched"},
    {it_end},
  },
  180, 15,                     // x, y offset
  3,                           // first selectable
  mf_background,               // full-screen menu
};

CONSOLE_COMMAND(mn_sound, 0)
{
   MN_StartMenu(&menu_sound);
}

/////////////////////////////////////////////////////////////////
//
// Mouse Options
//

menu_t menu_mouse =
{
   {
      {it_title,      FC_GOLD "mouse",                NULL,   "m_mouse"},
      {it_gap},
      {it_toggle,     "enable mouse",                 "use_mouse"},
      {it_gap},
      {it_info,       FC_GOLD "sensitivity"},
      {it_slider,     "horizontal",                   "sens_horiz"},
      {it_slider,     "vertical",                     "sens_vert"},
      {it_gap},
      {it_info,       FC_GOLD "misc."},
      {it_toggle,     "invert mouse",                 "invertmouse"},
      {it_toggle,     "smooth turning",               "smooth_turning"},
      {it_toggle,     "enable joystick",              "use_joystick"},
#ifdef _SDL_VER
      {it_runcmd,     "configure joystick...",        "mn_joymenu"},
#endif
      {it_gap},
      {it_info,       FC_GOLD"mouselook"},
      {it_toggle,     "enable mouselook",             "allowmlook" },
      {it_toggle,     "always mouselook",             "alwaysmlook"},
      {it_toggle,     "stretch short skies",          "r_stretchsky"},
      {it_end},
   },
   200, 15,                      // x, y offset
   2,                            // first selectable
   mf_background,                // full-screen menu
};

CONSOLE_COMMAND(mn_mouse, 0)
{
   MN_StartMenu(&menu_mouse);
}


/////////////////////////////////////////////////////////////////
//
// HUD Settings
//

menu_t menu_hud =
{
  {
    {it_title,      FC_GOLD "hud settings",         NULL,      "m_hud"},
    {it_gap},
    {it_info,       FC_GOLD "hud messages"},
    {it_toggle,     "messages",                     "messages"},
    {it_toggle,     "message colour",               "mess_colour"},
    {it_toggle,     "messages scroll",              "mess_scrollup"},
    {it_toggle,     "message lines",                "mess_lines"},
    {it_variable,   "message time (ms)",            "mess_timer"},
    {it_toggle,     "obituaries",                   "obituaries"},
    {it_toggle,     "obituary colour",              "obcolour"},
    {it_gap},
    {it_info,       FC_GOLD "full screen display"},
    {it_toggle,     "display type",                 "hu_overlay"},
    {it_toggle,     "hide secrets",                 "hu_hidesecrets"},
    {it_gap},
    {it_info,       FC_GOLD "misc."},
    {it_toggle,     "crosshair type",               "crosshair"},
    {it_toggle,     "crosshair highlighting",       "crosshair_hilite"},
    {it_toggle,     "show frags in DM",             "show_scores"},
    {it_toggle,     "map coords follow pointer",    "map_coords"},
    {it_end},
  },
  200, 15,                             // x,y offset
  3,
  mf_background,
};

CONSOLE_COMMAND(mn_hud, 0)
{
   MN_StartMenu(&menu_hud);
}


/////////////////////////////////////////////////////////////////
//
// Status Bar Settings
//

menu_t menu_statusbar =
{
  {
    {it_title,      FC_GOLD "status bar",           NULL,           "m_stat"},
    {it_gap},
    {it_toggle,     "numbers always red",           "st_rednum"},
    {it_toggle,     "percent sign grey",            "st_graypct"},
    {it_toggle,     "single key display",           "st_singlekey"},
    {it_gap},
    {it_info,       FC_GOLD "status bar colours"},
    {it_variable,   "ammo ok percentage",           "ammo_yellow"},
    {it_variable,   "ammo low percentage",          "ammo_red"},
    {it_gap},
    {it_variable,   "armour high percentage",       "armor_green"},
    {it_variable,   "armour ok percentage",         "armor_yellow"},
    {it_variable,   "armour low percentage",        "armor_red"},
    {it_gap},
    {it_variable,   "health high percentage",       "health_green"},
    {it_variable,   "health ok percentage",         "health_yellow"},
    {it_variable,   "health low percentage",        "health_red"},
    {it_end},
  },
  200, 15,
  2,
  mf_background,
};

CONSOLE_COMMAND(mn_status, 0)
{
   MN_StartMenu(&menu_statusbar);
}


/////////////////////////////////////////////////////////////////
//
// Automap colours
//

menu_t menu_automap = 
{
  {
    {it_title,    FC_GOLD "automap",              NULL,  "m_auto"},
    {it_gap},
    {it_automap,  "background colour",            "mapcolor_back"},
    {it_automap,  "walls",                        "mapcolor_wall"},
    {it_automap,  "closed door",                  "mapcolor_clsd"},
    {it_automap,  "change in floor height",       "mapcolor_fchg"},
    {it_automap,  "red door",                     "mapcolor_rdor"},
    {it_automap,  "yellow door",                  "mapcolor_ydor"},
    {it_automap,  "blue door",                    "mapcolor_bdor"},
    {it_automap,  "teleport line",                "mapcolor_tele"},
    {it_automap,  "secret",                       "mapcolor_secr"},
    {it_automap,  "exit",                         "mapcolor_exit"},
    {it_automap,  "unseen line",                  "mapcolor_unsn"},

    {it_automap,  "sprite",                       "mapcolor_sprt"},
    {it_automap,  "crosshair",                    "mapcolor_hair"},
    {it_automap,  "single player arrow",          "mapcolor_sngl"},
    {it_automap,  "friend",                       "mapcolor_frnd"},
    {it_automap,  "red key",                      "mapcolor_rkey"},
    {it_automap,  "yellow key",                   "mapcolor_ykey"},
    {it_automap,  "blue key",                     "mapcolor_bkey"},

    {it_end},
  },
  200, 15,              // x,y
  2,                    // starting item
  mf_background,        // fullscreen
};

CONSOLE_COMMAND(mn_automap, 0)
{
   MN_StartMenu(&menu_automap);
}


/////////////////////////////////////////////////////////////////
//
// Weapon Options
//

//
// NETCODE_FIXME -- WEAPONS_FIXME
//
// Weapon prefs and related values may need to change the way they
// work. See other notes for info about bfg type and autoaiming.
//

menu_t menu_weapons =
{
  {
    {it_title,      FC_GOLD "weapons",              NULL,        "m_weap"},
    {it_gap},
    {it_info,       FC_GOLD "weapon options"},
    {it_toggle,     "bfg type",                       "bfgtype"},
    {it_toggle,     "bobbing",                        "bobbing"},
    {it_toggle,     "recoil",                         "recoil"},
    {it_info,       FC_BRICK "fist/chainsaw switch"},
    {it_gap},
    {it_info,       FC_GOLD "weapon prefs."},
    {it_variable,   "1st choice",                     "weappref_1"},
    {it_variable,   "2nd choice",                     "weappref_2"},
    {it_variable,   "3rd choice",                     "weappref_3"},
    {it_variable,   "4th choice",                     "weappref_4"},
    {it_variable,   "5th choice",                     "weappref_5"},
    {it_variable,   "6th choice",                     "weappref_6"},
    {it_variable,   "7th choice",                     "weappref_7"},
    {it_variable,   "8th choice",                     "weappref_8"},
    {it_variable,   "9th choice",                     "weappref_9"},
    {it_end},
  },
  150, 15,                             // x,y offset
  3,                                   // starting item
  mf_background,                       // full screen
};

CONSOLE_COMMAND(mn_weapons, 0)
{
   MN_StartMenu(&menu_weapons);
}

/////////////////////////////////////////////////////////////////
//
// Compatibility vectors
//

//
// NETCODE_FIXME -- COMPAT_FIXME: Compatibility system needs
// overhaul.
//

menu_t menu_compat =
{
  {
    {it_title,      FC_GOLD "compatibility",        NULL,        "m_compat"},
    {it_gap},
    
    {it_toggle,   "some objects don't hang over cliffs",    "comp_dropoff"},
    {it_toggle,   "torque simulation disabled",             "comp_falloff"},
    
    {it_toggle,   "god mode isn't absolute",                "comp_god"},
    {it_toggle,   "power-up cheats have limited duration",  "comp_infcheat"},
    
    {it_toggle,   "sky unaffected by invulnerability",      "comp_skymap"},
    
    {it_toggle,   "blazing doors, double closing sound",    "comp_blazing"},
    {it_toggle,   "tagged door lighting effect off",        "comp_doorlight"},
    
    {it_toggle,   "pain elemental 20 lost soul limit",      "comp_pain"},
    {it_toggle,   "lost souls get stuck behind walls",      "comp_skull"},
    {it_toggle,   "monsters walk off lifts",                "comp_staylift"},
    {it_toggle,   "monsters get stuck to doortracks",       "comp_doorstuck"},
    {it_toggle,   "monsters don't give up pursuit",         "comp_pursuit"},
    {it_toggle,   "any monster can telefrag on map30",      "comp_telefrag"},
    {it_toggle,   "arch-vile resurrects invincible ghosts", "comp_vile"},
    
    {it_toggle,   "zombie players can exit levels",         "comp_zombie"},
    {it_toggle,   "use doom's stairbuilding method",        "comp_stairs"},
    {it_toggle,   "use doom's floor motion behaviour",      "comp_floors"},
    {it_toggle,   "use doom's linedef trigger model",       "comp_model"},
    {it_toggle,   "linedef effects with sector tag = 0",    "comp_zerotags"},
    {it_end},
  },
  270, 5,                     // x,y
  2,                           // starting item
  mf_background,               // full screen
};

CONSOLE_COMMAND(mn_compat, 0)
{
   MN_StartMenu(&menu_compat);
}

// haleyjd: New compatibility/functionality options for Eternity
menu_t menu_etccompat =
{
  {
    {it_title, FC_GOLD "eternity options", NULL, "M_ETCOPT"},
    {it_gap},
    {it_toggle, "use start map",                            "use_startmap"},
    {it_toggle, "new game starts on first new map",         "startonnewmap"},
    {it_toggle, "terrain effects inactive", 		    "comp_terrain"},
    {it_toggle, "creatures may respawn outside map",        "comp_respawnfix"},
    {it_toggle, "falling damage inactive",                  "comp_fallingdmg"},
    {it_toggle, "extended z clipping inactive",             "comp_overunder"},
    {it_toggle, "some thing heights are inaccurate",        "comp_theights"},
    {it_toggle, "lost souls don't bounce on floors",        "comp_soul"},
    {it_end},
  },
  270, 5,              // x, y
  2,                    // starting item
  mf_background,	// full screen
};

CONSOLE_COMMAND(mn_etccompat, 0)
{
   MN_StartMenu(&menu_etccompat);
}

/////////////////////////////////////////////////////////////////
//
// Enemies
//

menu_t menu_enemies =
{
  {
    {it_title,      FC_GOLD "enemies",              NULL,      "m_enem"},
    {it_gap},
    {it_info,       FC_GOLD "monster options"},
    {it_toggle,     "monsters remember target",     "mon_remember"},
    {it_toggle,     "monster infighting",           "mon_infight"},
    {it_toggle,     "monsters back out",            "mon_backing"},
    {it_toggle,     "monsters avoid hazards",       "mon_avoid"},
    {it_toggle,     "affected by friction",         "mon_friction"},
    {it_toggle,     "climb tall stairs",            "mon_climb"},
    {it_gap},
    {it_info,       FC_GOLD "mbf friend options"},
    {it_variable,   "friend distance",              "mon_distfriend"},
    {it_toggle,     "rescue dying friends",         "mon_helpfriends"},
    {it_end},
  },
  220,15,                             // x,y offset
  3,                                  // starting item
  mf_background                       // full screen
};

CONSOLE_COMMAND(mn_enemies, 0)
{
   MN_StartMenu(&menu_enemies);
}


/////////////////////////////////////////////////////////////////
//
// Framerate test
//
// Option on "video options" menu allows you to timedemo your
// computer on demo2. When you finish you are presented with
// this menu

// FIXME: This is junk and doesn't work right.

// test framerates
// SoM: ANYRES
// tons of new video modes
int framerates[12] = {2462, 1870, 2460, 698, 489};
int this_framerate;
void MN_FrameRateDrawer(void);

menu_t menu_framerate = 
{
  {
    {it_title,    FC_GOLD "framerate"},
    {it_gap},
    {it_info,     "this graph shows your framerate against that"},
    {it_info,     "of a fast modern computer (using the same"},
    {it_info,     "vidmode)"},
    {it_gap},
    {it_runcmd,   "ok",          "mn_prevmenu"},
    {it_gap},
    {it_end},
  },
  15, 15,                                // x, y
  6,                                     // starting item
  mf_background | mf_leftaligned,        // align left
  MN_FrameRateDrawer,
};

#define BARCOLOR 208

// SoM: ANYRES
void MN_FrameRateDrawer(void)
{
   int x, y, ytop;
   int scrwidth = v_width;
   int linelength;
   char tempstr[50];
   
   // fast computers framerate is always 3/4 of screen
   
   psnprintf(tempstr, sizeof(tempstr), "your computer: %i.%i fps",
             this_framerate/10, this_framerate%10);
   MN_WriteText(tempstr, 50, 80);
  
   ytop = realyarray[93];
   linelength = (3 * scrwidth * this_framerate) / (4 * framerates[v_mode]);
   if(linelength > scrwidth) linelength = scrwidth-2;
   
   // draw your computers framerate
   for(x=0; x<linelength; x++)
   {
      y = ytop + (globalyscale >> FRACBITS);
      while(y-- >= ytop)
         *(screens[0] + y * scrwidth + x) = BARCOLOR;
   }

   psnprintf(tempstr, sizeof(tempstr),
             "fast computer (k6-2 450): %i.%i fps",
             framerates[v_mode]/10, framerates[v_mode]%10);
   MN_WriteText(tempstr, 50, 110);

   ytop = realyarray[103];
   
   // draw my computers framerate
   for(x=0; x<(scrwidth*3)/4; x++)
   {
      y = ytop + (globalyscale >> FRACBITS);
      while(y-- >= ytop)
         *(screens[0] + y*scrwidth + x) = BARCOLOR;
   }
}

void MN_ShowFrameRate(int framerate)
{
   this_framerate = framerate;
   MN_StartMenu(&menu_framerate);
   D_StartTitle();      // user does not need to see the console
}

//------------------------------------------------------------------------
//
// Key Bindings
//

menu_t menu_keybindings =
  {
    {
        {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
        {it_gap},
        {it_runcmd,       "basic movement...",      "mn_movekeys"},
        {it_runcmd,       "weapon keys...",        "mn_weaponkeys"},
        {it_runcmd,       "environment...",        "mn_envkeys"},
        {it_runcmd,       "menu actions...",       "mn_menukeys"},
        {it_runcmd,       "automap...",            "mn_automapkeys"},
        {it_end},
    },
    110, 15,                       // x,y offsets
    2,
    mf_background|mf_leftaligned, // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_keybindings, 0)
{
   MN_StartMenu(&menu_keybindings);
}

//------------------------------------------------------------------------
//
// Key Bindings: basic movement keys
//

menu_t menu_movekeys =
{
   {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
      {it_gap},
      {it_info, FC_GOLD "basic movement"},
      {it_gap},
      {it_binding,      "move forward",          "forward"},
      {it_binding,      "move backward",         "backward"},
      {it_binding,      "run",                   "speed"},
      {it_binding,      "turn left",             "left"},
      {it_binding,      "turn right",            "right"},
      {it_binding,      "strafe on",             "strafe"},
      {it_binding,      "strafe left",           "moveleft"},
      {it_binding,      "strafe right",          "moveright"},
      {it_binding,      "180 degree turn",       "flip"},
      {it_gap},
      {it_binding,      "mlook on",              "mlook"},
      {it_binding,      "look up",               "lookup"},
      {it_binding,      "look down",             "lookdown"},
      {it_binding,      "center view",           "center"},
      {it_gap},
      {it_binding,      "use",                   "use"},
      {it_end},
   },
   150, 15,                       // x,y offsets
   4,
   mf_background, // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_movekeys, 0)
{
   MN_StartMenu(&menu_movekeys);
}

//------------------------------------------------------------------------
//
// Key Bindings: weapon keys
//

menu_t menu_weaponbindings =
  {
    {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
      {it_gap},
      {it_info, FC_GOLD "weapon keys"},
      {it_gap},
      {it_binding,      "weapon 1",              "weapon1"},
      {it_binding,      "weapon 2",              "weapon2"},
      {it_binding,      "weapon 3",              "weapon3"},
      {it_binding,      "weapon 4",              "weapon4"},
      {it_binding,      "weapon 5",              "weapon5"},
      {it_binding,      "weapon 6",              "weapon6"},
      {it_binding,      "weapon 7",              "weapon7"},
      {it_binding,      "weapon 8",              "weapon8"},
      {it_binding,      "weapon 9",              "weapon9"},
      {it_gap},
      {it_binding,      "next weapon",           "nextweapon"},
      {it_binding,      "attack/fire",           "attack"},
      {it_end},
    },
    150, 15,                        // x,y offsets
    4,
    mf_background,  // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_weaponkeys, 0)
{
   MN_StartMenu(&menu_weaponbindings);
}

//------------------------------------------------------------------------
//
// Key Bindings: Environment
//

// haleyjd 06/24/02: added quicksave/load

menu_t menu_envbindings =
{
   {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
      {it_gap},
      {it_info,         FC_GOLD "environment"},
      {it_gap},
      {it_binding,      "screen size up",        "screensize +"},
      {it_binding,      "screen size down",      "screensize -"},
      {it_binding,      "take screenshot",       "screenshot"},
      {it_gap},
      {it_binding,      "save game",             "mn_savegame"}, 
      {it_binding,      "load game",             "mn_loadgame"},
      {it_binding,      "volume",                "mn_sound"},
      {it_binding,      "toggle hud",            "hu_overlay /"},
      {it_binding,      "quick save",            "quicksave"},
      {it_binding,      "end game",              "mn_endgame"},
      {it_binding,      "toggle messages",       "messages /"},
      {it_binding,      "quick load",            "quickload"},
      {it_binding,      "quit",                  "mn_quit"},
      {it_binding,      "gamma correction",      "gamma /"},
      {it_end},
   },
   150, 15,                        // x,y offsets
   4,
   mf_background,  // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_envkeys, 0)
{
   MN_StartMenu(&menu_envbindings);
}

//------------------------------------------------------------------------
//
// Key Bindings: Menu Keys
//

menu_t menu_menukeys =
{
   {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
      {it_gap},
      {it_info, FC_GOLD "menu actions"},
      {it_gap},
      {it_binding,      "toggle menus",          "menu_toggle"},
      {it_binding,      "previous menu",         "menu_previous"},
      {it_binding,      "next item",             "menu_down"},
      {it_binding,      "previous item",         "menu_up"},
      {it_binding,      "next value",            "menu_right"},
      {it_binding,      "previous value",        "menu_left"},
      {it_binding,      "confirm",               "menu_confirm"},
      {it_binding,      "display help",          "menu_help"},
      {it_binding,      "display setup",         "menu_setup"},
      {it_end},
   },
   150, 15,                       // x,y offsets
   4,
   mf_background, // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_menukeys, 0)
{
   MN_StartMenu(&menu_menukeys);
}

//------------------------------------------------------------------------
//
// Key Bindings: Automap Keys
//

menu_t menu_automapkeys =
{
   {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
      {it_gap},
      {it_info, FC_GOLD "automap"},
      {it_gap},
      {it_binding,      "toggle map",            "map_toggle"},
      {it_binding,      "move up",               "map_up"},
      {it_binding,      "move down",             "map_down"},
      {it_binding,      "move left",             "map_left"},
      {it_binding,      "move right",            "map_right"},
      {it_binding,      "zoom in",               "map_zoomin"},
      {it_binding,      "zoom out",              "map_zoomout"},
      {it_binding,      "show full map",         "map_gobig"},
      {it_binding,      "follow mode",           "map_follow"},
      {it_binding,      "mark spot",             "map_mark"},
      {it_binding,      "clear spots",           "map_clear"},
      {it_binding,      "show grid",             "map_grid"},
      {it_end},
   },
   150, 15,                       // x,y offsets
   4,
   mf_background, // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_automapkeys, 0)
{
   MN_StartMenu(&menu_automapkeys);
}

//------------------------------------------------------------------------
//
// SDL-specific joystick configuration menu
//

#ifdef _SDL_VER

void MN_JSDrawer(void);

menu_t menu_joystick =
{
   {
      {it_title,        FC_GOLD "configure joystick", NULL, "M_JOYSET" },
      {it_gap},
      {it_info,         FC_GOLD "sensitivity"},
      {it_variable,     "horizontal",               "joySens_x" },
      {it_variable,     "vertical",                 "joySens_y" },
      {it_gap},
      {it_info,         FC_GOLD "current device:"},
      {it_info,         "(current device)"},
      {it_gap},
      {it_info,         FC_GOLD "select device:"},
      // .... devices filled in by console cmd function ......
      {it_end},
   },
   200, 15,              // x,y offset
   3,                    // start on first selectable
   mf_background,        // full-screen menu
   MN_JSDrawer,
};

void MN_JSDrawer(void)
{
   if(i_SDLJoystickNum != -1)
      menu_joystick.menuitems[7].description = joysticks[i_SDLJoystickNum].description;
   else
      menu_joystick.menuitems[7].description = "NONE";
}

CONSOLE_COMMAND(mn_joymenu, 0)
{
   static boolean menu_built = false;
   
   // dont build multiple times
   if(!menu_built)
   {
      int menuitem, jsdevice;
      char tempstr[20];
      
      menu_joystick.menuitems[10].type = it_runcmd;
      menu_joystick.menuitems[10].description = "NONE";
      menu_joystick.menuitems[10].data = "i_joystick -1";
      
      // start on item 11
      
      for(menuitem = 11, jsdevice = 0; joysticks[jsdevice].description;
          menuitem++, jsdevice++)
      {
         menu_joystick.menuitems[menuitem].type = it_runcmd;
         menu_joystick.menuitems[menuitem].description =
            joysticks[jsdevice].description;
         sprintf(tempstr, "i_joystick %i", jsdevice);
         menu_joystick.menuitems[menuitem].data = strdup(tempstr);
      }
     
      menu_joystick.menuitems[menuitem].type = it_end; // mark end

      menu_built = true;
   }
   
   MN_StartMenu(&menu_joystick);
}

#endif // _SDL_VER

extern void MN_InitSkinViewer(void);

CONSOLE_COMMAND(skinviewer, 0)
{
   MN_InitSkinViewer();
}

// haleyjd 05/16/04: traditional menu support
VARIABLE_BOOLEAN(traditional_menu, NULL, yesno);
CONSOLE_VARIABLE(use_traditional_menu, traditional_menu, 0) {}

void MN_AddMenus(void)
{
   C_AddCommand(mn_newgame);
   C_AddCommand(mn_episode);
   C_AddCommand(startlevel);
   C_AddCommand(use_startmap);
   
   C_AddCommand(mn_loadgame);
   C_AddCommand(mn_load);
   C_AddCommand(mn_savegame);

   C_AddCommand(mn_optfeat); // haleyjd 04/18/03
   C_AddCommand(mn_features);
   C_AddCommand(mn_loadwad);
   C_AddCommand(mn_wadname);
   C_AddCommand(mn_demos);
   C_AddCommand(mn_demoname);
   
   C_AddCommand(mn_multi);
   C_AddCommand(mn_serial);
   C_AddCommand(mn_phonenum);
   C_AddCommand(mn_tcpip);
   C_AddCommand(mn_chatmacros);
   C_AddCommand(mn_player);
   C_AddCommand(mn_advanced);

   // haleyjd: dmflags
   C_AddCommand(mn_dmflags);
   C_AddCommand(mn_dfitem);
   C_AddCommand(mn_dfweapstay);
   C_AddCommand(mn_dfbarrel);
   C_AddCommand(mn_dfplyrdrop);
   C_AddCommand(mn_dfrespsupr);
   
   // different connect types
   C_AddCommand(mn_ser_answer);
   C_AddCommand(mn_ser_connect);
   C_AddCommand(mn_udpserv);
   C_AddCommand(mn_gset);
   
   C_AddCommand(mn_options);
   C_AddCommand(mn_mouse);
   C_AddCommand(mn_video);
   C_AddCommand(mn_particle);  // haleyjd: particle options menu
   C_AddCommand(mn_vidmode);
   C_AddCommand(mn_sound);
   C_AddCommand(mn_weapons);
   C_AddCommand(mn_compat);
   C_AddCommand(mn_etccompat); // haleyjd: new eternity options menu
   C_AddCommand(mn_enemies);
   C_AddCommand(mn_hud);
   C_AddCommand(mn_status);
   C_AddCommand(mn_automap);

   C_AddCommand(mn_movekeys);
   C_AddCommand(mn_keybindings);
   C_AddCommand(mn_weaponkeys);
   C_AddCommand(mn_envkeys);
   C_AddCommand(mn_menukeys);
   C_AddCommand(mn_automapkeys);
   
   C_AddCommand(newgame);
   
   // prompt messages
   C_AddCommand(mn_quit);
   C_AddCommand(mn_endgame);
   
   // haleyjd: quicksave, quickload
   C_AddCommand(quicksave);
   C_AddCommand(quickload);
   C_AddCommand(qsave);
   C_AddCommand(qload);
   
   // haleyjd 04/15/02: SDL joystick devices
#ifdef _SDL_VER
   C_AddCommand(mn_joymenu);
#endif

   C_AddCommand(skinviewer);
   C_AddCommand(use_traditional_menu);
   
   // haleyjd: add Heretic-specific menus (in mn_htic.c)
   MN_AddHMenus();
   
   MN_CreateSaveCmds();
}

// EOF
