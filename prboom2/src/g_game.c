/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: g_game.c,v 1.59 2002/08/10 20:57:57 cph Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2001 by
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
 * DESCRIPTION:  none
 *  The original Doom description was none, basically because this file
 *  has everything. This ties up the game logic, linking the menu and 
 *  input code to the underlying game by creating & respawning players, 
 *  building game tics, calling the underlying thing logic.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: g_game.c,v 1.59 2002/08/10 20:57:57 cph Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#if ((defined _MSC_VER) || (defined DREAMCAST))
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#endif
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#ifndef DREAMCAST
#include <fcntl.h>
#endif

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#include "z_zone.h"

#include "doomstat.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "mn_engin.h"
#include "m_random.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_draw.h"
#include "p_map.h"
#include "s_sound.h"
#include "dstrings.h"
#include "sounds.h"
#include "r_data.h"
#include "r_sky.h"
#include "d_deh.h"              // Ty 3/27/98 deh declarations
#include "p_inter.h"
#include "g_game.h"
#include "g_bind.h" // keybinding
#include "g_bindaxes.h"
#include "lprintf.h"
#include "i_main.h"
#include "i_system.h"

#define SAVEGAMESIZE  0x20000
#define SAVESTRINGSIZE  24

static size_t   savegamesize = SAVEGAMESIZE; // killough
static boolean  netdemo;
static const byte *demobuffer;   /* cph - only used for playback */
static FILE    *demofp; /* cph - record straight to file */
static const byte *demo_p;
static short    consistancy[MAXPLAYERS][BACKUPTICS];

gameaction_t    gameaction;
gamestate_t     gamestate = GS_DEMOSCREEN;
skill_t         gameskill;
boolean         respawnmonsters;
int             gameepisode;
int             gamemap;
boolean         paused;
// CPhipps - moved *_loadgame vars here
static boolean forced_loadgame = false;
static boolean command_loadgame = false;

boolean         usergame;      // ok to save / end game
boolean         timingdemo;    // if true, exit with report on completion
boolean         fastdemo;      // if true, run at full speed -- killough
boolean         nodrawers;     // for comparative timing purposes
boolean         noblit;        // for comparative timing purposes
int             starttime;     // for comparative timing purposes
boolean         deathmatch;    // only if started as net death
boolean         netgame;       // only true if packets are broadcast
boolean         playeringame[MAXPLAYERS];
player_t        players[MAXPLAYERS];
int             consoleplayer; // player taking events and displaying
int             displayplayer; // view being displayed
int             gametic;
int             levelstarttic; // gametic at level start
int             basetic;       /* killough 9/29/98: for demo sync */
int             totalkills, totalitems, totalsecret;    // for intermission
boolean         demorecording;
boolean         demoplayback;
boolean         singledemo;           // quit after playing a demo from cmdline
wbstartstruct_t wminfo;               // parms for world map / intermission
boolean         haswolflevels = false;// jff 4/18/98 wolf levels present
static byte     *savebuffer;          // CPhipps - static
int             autorun = false;      // always running?          // phares
int             totalleveltimes;      // CPhipps - total time for all completed levels

int             mouseSensitivity_horiz = 10; // has default   //  killough
int             mouseSensitivity_vert = 10;  // has default

//
// controls (have defaults)
//

int     key_right = KEYD_RIGHTARROW;
int     key_left = KEYD_LEFTARROW;
int     key_up = KEYD_UPARROW;
int     key_down = KEYD_DOWNARROW;
int     key_menu_right = KEYD_RIGHTARROW;
int     key_menu_left = KEYD_LEFTARROW;
int     key_menu_up = KEYD_UPARROW;
int     key_menu_down = KEYD_DOWNARROW;
int     key_menu_backspace = KEYD_BACKSPACE;
int     key_menu_escape = KEYD_ESCAPE;
int     key_menu_enter = KEYD_ENTER;
int     key_strafeleft = ',';
int     key_straferight = '.';
int     key_fire = KEYD_RCTRL;
int     key_use = ' ';
int     key_strafe = KEYD_RALT;
int     key_speed = KEYD_RSHIFT;
int     key_escape = KEYD_ESCAPE;
int     key_savegame = KEYD_F2;
int     key_loadgame = KEYD_F3;
int     key_autorun = KEYD_CAPSLOCK;
int     key_reverse = '/';
int     key_zoomin = '=';
int     key_zoomout = '-';
int     key_chat = 't';
int     key_backspace = KEYD_BACKSPACE;
int     key_enter = KEYD_ENTER;
int     key_map_right = KEYD_RIGHTARROW;
int     key_map_left = KEYD_LEFTARROW;
int     key_map_up = KEYD_UPARROW;
int     key_map_down = KEYD_DOWNARROW;
int     key_map_zoomin = '=';
int     key_map_zoomout = '-';
int     key_map = KEYD_TAB;
int     key_map_gobig = '0';
int     key_map_follow = 'f';
int     key_map_mark = 'm';
int     key_map_clear = 'c';
int     key_map_grid = 'g';
int     key_map_overlay = 'o'; // cph - map overlay
int     key_map_rotate = 'r';  // cph - map rotation
int     key_help = KEYD_F1;
int     key_soundvolume = KEYD_F4;
int     key_hud = KEYD_F5;
int     key_quicksave = KEYD_F6;
int     key_endgame = KEYD_F7;
int     key_messages = KEYD_F8;
int     key_quickload = KEYD_F9;
int     key_quit = KEYD_F10;
int     key_gamma = KEYD_F11;
int     key_spy = KEYD_F12;
int     key_pause = KEYD_PAUSE;
int     key_setup;
int     destination_keys[MAXPLAYERS] = {'g','i','b','r'};
int     key_weapontoggle = '0';
int     key_weapon1 = '1';
int     key_weapon2 = '2';
int     key_weapon3 = '3';
int     key_weapon4 = '4';
int     key_weapon5 = '5';
int     key_weapon6 = '6';
int     key_weapon7 = '7';
int     key_weapon8 = '8';
int     key_weapon9 = '9';

int     key_screenshot = '*';             // killough 2/22/98: screenshot key

#define MAXPLMOVE   (forwardmove[1])
#define TURBOTHRESHOLD  0x32
#define SLOWTURNTICS  6
#define QUICKREVERSE (short)32768 // 180 degree reverse                    // phares
#define NUMKEYS   512

fixed_t forwardmove[2] = {0x19, 0x32};
fixed_t sidemove[2]    = {0x18, 0x28};
fixed_t angleturn[3]   = {640, 1280, 320};  // + slow turn

// CPhipps - made lots of key/button state vars static
static int     turnheld;       // for accelerative turning

// mouse values are used once
static int   mousex;
static int   mousey;
static int   dclicktime;
static int   dclickstate;
static int   dclicks;
static int   dclicktime2;
static int   dclickstate2;
static int   dclicks2;

// Game events info
static buttoncode_t special_event; // Event triggered by local player, to send
static byte  savegameslot;         // Slot to load if gameaction == ga_loadgame
char         savedescription[SAVEDESCLEN];  // Description to save in savegame if gameaction == ga_savegame

//jff 3/24/98 declare startskill external, define defaultskill here
extern skill_t startskill;      //note 0-based
int defaultskill = 3;           //note 1-based

// killough 2/8/98: make corpse queue variable in size
int    bodyqueslot;
int    bodyquesize = 32;        // killough 2/8/98
mobj_t **bodyque = 0;                   // phares 8/10/98

void   *statcopy;       // for statistics driver

static void G_DoSaveGame (boolean menu);
static const byte* G_ReadDemoHeader(const byte* demo_p);

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//

void G_BuildTiccmd(ticcmd_t* cmd)
{
  boolean strafe;
  int speed;
  int tspeed;
  int forward;
  int side;
  int newweapon;                                          // phares
  /* cphipps - remove needless I_BaseTiccmd call, just set the ticcmd to zero */
  memset(cmd,0,sizeof*cmd);
  cmd->consistancy = consistancy[consoleplayer][maketic%BACKUPTICS];

  strafe = action_strafe;
  speed = autorun || action_speed; // phares

  forward = side = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
  if (action_left || action_right)
    turnheld += ticdup;
  else
    turnheld = 0;

  if (turnheld < SLOWTURNTICS)
    tspeed = 2;             // slow turn
  else
    tspeed = speed;

  // turn 180 degrees in one keystroke?                           // phares
                                                                  //    |
  if (action_flip)                                                //    V
    {
      cmd->angleturn += QUICKREVERSE;                             //    ^
      action_flip = false;                                        //    |
    }                                                             // phares

  // let movement keys cancel each other out

  if (strafe)
    {
      if (action_right)
        side += sidemove[speed];
      if (action_left)
        side -= sidemove[speed];
    }
  else
    {
      if (action_right)
        cmd->angleturn -= angleturn[tspeed];
      if (action_left)
        cmd->angleturn += angleturn[tspeed];
    }

  if (action_forward)
    forward += forwardmove[speed];
  if (action_backward)
    forward -= forwardmove[speed];
  if (action_moveright)
    side += sidemove[speed];
  if (action_moveleft)
    side -= sidemove[speed];

  // Read Axes
  forward += forwardmove[1] * axis_forward_value / 256;
  side += sidemove[1] * axis_side_value / 256;
  cmd->angleturn += angleturn[1] * axis_turn_value / 256;

    // buttons
  cmd->chatchar = HU_dequeueChatChar();

  if (action_attack)
    cmd->buttons |= BT_ATTACK;

  if (action_use)
    {
      cmd->buttons |= BT_USE;
      // clear double clicks if hit use button
      dclicks = 0;
    }

  // Toggle between the top 2 favorite weapons.                   // phares
  // If not currently aiming one of these, switch to              // phares
  // the favorite. Only switch if you possess the weapon.         // phares

  // killough 3/22/98:
  //
  // Perform automatic weapons switch here rather than in p_pspr.c,
  // except in demo_compatibility mode.
  //
  // killough 3/26/98, 4/2/98: fix autoswitch when no weapons are left

  if ((!demo_compatibility && players[consoleplayer].attackdown && // killough
       !P_CheckAmmo(&players[consoleplayer])) || action_toggleweapon)
    newweapon = P_SwitchWeapon(&players[consoleplayer]);           // phares
  else
    {                                 // phares 02/26/98: Added gamemode checks
      newweapon =
        action_weapon1 ? wp_fist :    // killough 5/2/98: reformatted
        action_weapon2 ? wp_pistol :
        action_weapon3 ? wp_shotgun :
        action_weapon4 ? wp_chaingun :
        action_weapon5 ? wp_missile :
        action_weapon6 && gamemode != shareware ? wp_plasma :
        action_weapon7 && gamemode != shareware ? wp_bfg :
        action_weapon8 ? wp_chainsaw :
        action_weapon9 && gamemode == commercial ? wp_supershotgun :
        wp_nochange;

      // killough 3/22/98: For network and demo consistency with the
      // new weapons preferences, we must do the weapons switches here
      // instead of in p_user.c. But for old demos we must do it in
      // p_user.c according to the old rules. Therefore demo_compatibility
      // determines where the weapons switch is made.

      // killough 2/8/98:
      // Allow user to switch to fist even if they have chainsaw.
      // Switch to fist or chainsaw based on preferences.
      // Switch to shotgun or SSG based on preferences.

      if (!demo_compatibility)
        {
          const player_t *player = &players[consoleplayer];

          // only select chainsaw from '1' if it's owned, it's
          // not already in use, and the player prefers it or
          // the fist is already in use, or the player does not
          // have the berserker strength.

          if (newweapon==wp_fist && player->weaponowned[wp_chainsaw] &&
              player->readyweapon!=wp_chainsaw &&
              (player->readyweapon==wp_fist ||
               !player->powers[pw_strength] ||
               P_WeaponPreferred(wp_chainsaw, wp_fist)))
            newweapon = wp_chainsaw;

          // Select SSG from '3' only if it's owned and the player
          // does not have a shotgun, or if the shotgun is already
          // in use, or if the SSG is not already in use and the
          // player prefers it.

          if (newweapon == wp_shotgun && gamemode == commercial &&
              player->weaponowned[wp_supershotgun] &&
              (!player->weaponowned[wp_shotgun] ||
               player->readyweapon == wp_shotgun ||
               (player->readyweapon != wp_supershotgun &&
                P_WeaponPreferred(wp_supershotgun, wp_shotgun))))
            newweapon = wp_supershotgun;
        }
      // killough 2/8/98, 3/22/98 -- end of weapon selection changes
    }

  if (newweapon != wp_nochange)
    {
      cmd->buttons |= BT_CHANGE;
      cmd->buttons |= newweapon<<BT_WEAPONSHIFT;
    }

  forward += mousey;
  if (strafe)
    side += mousex / 4;       /* mead  Don't want to strafe as fast as turns.*/
  else
    cmd->angleturn -= mousex; /* mead now have enough dynamic range 2-10-00 */

  mousex = mousey = 0;

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;

  cmd->forwardmove += forward;
  cmd->sidemove += side;

  // CPhipps - special events (game new/load/save/pause)
  if (special_event & BT_SPECIAL) {
    cmd->buttons = special_event;
    special_event = 0;
  }
}

// 
// G_RestartLevel
//

void G_RestartLevel(void)
{
  special_event = BT_SPECIAL | (BTS_RESTARTLEVEL & BT_SPECIALMASK);
}

#include "z_bmalloc.h"
//
// G_DoLoadLevel
//

extern gamestate_t wipegamestate;

static void G_DoLoadLevel (void)
{
  int i;

  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.

  skyflatnum = R_FlatNumForName ( SKYFLATNAME );

  // DOOM determines the sky texture to be used
  // depending on the current episode, and the game version.
  if (gamemode == commercial)
    // || gamemode == pack_tnt   //jff 3/27/98 sorry guys pack_tnt,pack_plut
    // || gamemode == pack_plut) //aren't gamemodes, this was matching retail
    {
      skytexture = R_TextureNumForName ("SKY3");
      if (gamemap < 12)
        skytexture = R_TextureNumForName ("SKY1");
      else
        if (gamemap < 21)
          skytexture = R_TextureNumForName ("SKY2");
    }
  else //jff 3/27/98 and lets not forget about DOOM and Ultimate DOOM huh?
    switch (gameepisode)
      {
      case 1:
        skytexture = R_TextureNumForName ("SKY1");
        break;
      case 2:
        skytexture = R_TextureNumForName ("SKY2");
        break;
      case 3:
        skytexture = R_TextureNumForName ("SKY3");
        break;
      case 4: // Special Edition sky
        skytexture = R_TextureNumForName ("SKY4");
        break;
      }//jff 3/27/98 end sky setting fix

  levelstarttic = gametic;        // for time calculation

  if (!demo_compatibility && !mbf_features)   // killough 9/29/98
    basetic = gametic;

  if (wipegamestate == GS_LEVEL)
    wipegamestate = -1;             // force a wipe

  gamestate = GS_LEVEL;

  for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i] && players[i].playerstate == PST_DEAD)
        players[i].playerstate = PST_REBORN;
      memset (players[i].frags,0,sizeof(players[i].frags));
    }

  // initialize the msecnode_t freelist.                     phares 3/25/98
  // any nodes in the freelist are gone by now, cleared
  // by Z_FreeTags() when the previous level ended or player
  // died.

  {
    DECLARE_BLOCK_MEMORY_ALLOC_ZONE(secnodezone);
    NULL_BLOCK_MEMORY_ALLOC_ZONE(secnodezone);
      //extern msecnode_t *headsecnode; // phares 3/25/98
      //headsecnode = NULL;
  }

  P_SetupLevel (gameepisode, gamemap, 0, gameskill);
  displayplayer = consoleplayer;    // view the guy you are playing
  gameaction = ga_nothing;
  Z_CheckHeap ();

  // clear cmd building stuff
  mousex = mousey = 0;
  special_event = 0; paused = false;

  // killough 5/13/98: in case netdemo has consoleplayer other than green
  ST_Start();
  HU_Start();

  // killough: make -timedemo work on multilevel demos
  // Move to end of function to minimize noise -- killough 2/22/98:

  if (timingdemo)
    {
      static int first=1;
      if (first)
        {
          starttime = I_GetTime_RealTime ();
          first=0;
        }
    }
}


//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//

boolean G_Responder (event_t* ev)
{
  // allow spy mode changes even during the demo
  // killough 2/22/98: even during DM demo
  //
  // killough 11/98: don't autorepeat spy mode switch

  if (ev->data1 == key_spy && netgame && (demoplayback || !deathmatch) &&
      gamestate == GS_LEVEL)
  {
    if (ev->type == ev_keydown)
	  {
	    do                                          // spy mode
	      if (++displayplayer >= MAXPLAYERS)
	        displayplayer = 0;
	    while (!playeringame[displayplayer] && displayplayer!=consoleplayer);

	    ST_Start();    // killough 3/7/98: switch status bar views too
	    HU_Start();
	    S_UpdateSounds(players[displayplayer].mo);
	  }
    return true;
  }

  // any other key pops up menu if in demos
  //
  // killough 8/2/98: enable automap in -timedemo demos
  //
  // killough 9/29/98: make any key pop up menu regardless of
  // which kind of demo, and allow other events during playback

  if (gameaction == ga_nothing && (demoplayback || gamestate == GS_DEMOSCREEN))
    {
      // killough 9/29/98: allow user to pause demos during playback
      if (ev->type == ev_keydown && ev->data1 == key_pause)
	{
	  if (paused ^= 2)
	    S_PauseSound();
	  else
	    S_ResumeSound();
	  return true;
	}

      // killough 10/98:
      // Don't pop up menu, if paused in middle
      // of demo playback, or if automap active.
      // Don't suck up keys, which may be cheats

      return gamestate == GS_DEMOSCREEN &&
	!(paused & 2) && !(automapmode & am_active) &&
	((ev->type == ev_keydown) ||
	 (ev->type == ev_mouse && ev->data1)) ?
	MN_StartControlPanel(), true : false;
    }

  if (gamestate == GS_FINALE && F_Responder(ev))
    return true;  // finale ate the event

  switch (ev->type)
    {
    case ev_keydown:
      if (ev->data1 == key_pause)           // phares
        {
          special_event = BT_SPECIAL | (BTS_PAUSE & BT_SPECIALMASK);
          return true;
        }
    	G_KeyResponder(ev); // keybinding
      return true;    // eat key down events

    case ev_keyup:
    	G_KeyResponder(ev); // keybinding
      return false;   // always let key up events filter down

    case ev_mouse:
      /*
       * bmead@surfree.com
       * Modified by Barry Mead after adding vastly more resolution
       * to the Mouse Sensitivity Slider in the options menu 1-9-2000
       * Removed the mouseSensitivity "*4" to allow more low end
       * sensitivity resolution especially for lsdoom users.
       */
      mousex += (ev->data2*(mouseSensitivity_horiz))/10;  /* killough */
      mousey += (ev->data3*(mouseSensitivity_vert))/10;  /*Mead rm *4 */
      return true;    // eat events

    case ev_axis:
      G_AxisResponder(ev);
      return true;

    default:
      break;
    }
  return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//

extern int mapcolor_me;

void G_Ticker (void)
{
  int i;
  static gamestate_t prevgamestate;

  // CPhipps - player colour changing
  if (!demoplayback && mapcolor_plyr[consoleplayer] != mapcolor_me) {
    // Changed my multiplayer colour - Inform the whole game
    int net_cl = LONG(mapcolor_me);
#ifdef HAVE_NET
    D_NetSendMisc(nm_plcolour, sizeof(net_cl), &net_cl);
#endif
    G_ChangedPlayerColour(consoleplayer, mapcolor_me);
  }
  // do player reborns if needed
  for (i=0 ; i<MAXPLAYERS ; i++)
    if (playeringame[i] && players[i].playerstate == PST_REBORN)
      G_DoReborn (i);

  // do things to change the game state
  while (gameaction != ga_nothing)
    {
      switch (gameaction)
        {
        case ga_loadlevel:
	  // force players to be initialized on level reload
	  for (i=0 ; i<MAXPLAYERS ; i++)
	    players[i].playerstate = PST_REBORN;
          G_DoLoadLevel ();
          break;
        case ga_newgame:
          G_DoNewGame ();
          break;
        case ga_loadgame:
          G_DoLoadGame ();
          break;
        case ga_savegame:
          G_DoSaveGame (false);
          break;
        case ga_playdemo:
          G_DoPlayDemo ();
          break;
        case ga_completed:
          G_DoCompleted ();
          break;
        case ga_victory:
          F_StartFinale ();
          break;
        case ga_worlddone:
          G_DoWorldDone ();
          break;
        case ga_nothing:
          break;
        }
    }

  if (paused & 2 || (!demoplayback && menuactive && !netgame))
    basetic++;  // For revenant tracers and RNG -- we must maintain sync
  else {
    // get commands, check consistancy, and build new consistancy check
    int buf = (gametic/ticdup)%BACKUPTICS;

    for (i=0 ; i<MAXPLAYERS ; i++) {
      if (playeringame[i])
        {
          ticcmd_t *cmd = &players[i].cmd;

          memcpy(cmd, &netcmds[i][buf], sizeof *cmd);

          if (demoplayback)
            G_ReadDemoTiccmd (cmd);
          if (demorecording)
            G_WriteDemoTiccmd (cmd);

          // check for turbo cheats
          // killough 2/14/98, 2/20/98 -- only warn in netgames and demos

          if ((netgame || demoplayback) && cmd->forwardmove > TURBOTHRESHOLD &&
              !(gametic&31) && ((gametic>>5)&3) == i )
            {
	      extern char *player_names[];
	      /* cph - don't use sprintf, use doom_printf */
              doom_printf ("%s is turbo!", player_names[i]);
            }

          if (netgame && !netdemo && !(gametic%ticdup) )
            {
              if (gametic > BACKUPTICS
                  && consistancy[i][buf] != cmd->consistancy)
                I_Error("G_Ticker: Consistency failure (%i should be %i)",
						cmd->consistancy, consistancy[i][buf]);
              if (players[i].mo)
                consistancy[i][buf] = players[i].mo->x;
              else
                consistancy[i][buf] = 0; // killough 2/14/98
            }
        }
    }

    // check for special buttons
    for (i=0; i<MAXPLAYERS; i++) {
      if (playeringame[i])
        {
          if (players[i].cmd.buttons & BT_SPECIAL)
            {
              switch (players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                  paused ^= 1;
                  if (paused)
                    S_PauseSound ();
                  else
                    S_ResumeSound ();
                  break;

                case BTS_SAVEGAME:
                  if (!savedescription[0])
                    strcpy(savedescription, "NET GAME");
                  savegameslot =
                    (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
                  gameaction = ga_savegame;
                  break;

		  // CPhipps - remote loadgame request
                case BTS_LOADGAME: 
                  savegameslot =
                    (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
                  gameaction = ga_loadgame;
		  forced_loadgame = netgame; // Force if a netgame 
		  command_loadgame = false;
                  break;

		  // CPhipps - Restart the level
		case BTS_RESTARTLEVEL:
                  if (demoplayback || (compatibility_level < lxdoom_1_compatibility))
                    break;     // CPhipps - Ignore in demos or old games
 		  gameaction = ga_loadlevel;
		  break;
                }
	      players[i].cmd.buttons = 0;
            }
        }
    }
  }

  // cph - if the gamestate changed, we may need to clean up the old gamestate
  if (gamestate != prevgamestate) {
    switch (prevgamestate) {
    case GS_LEVEL:
      Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);
      break;
    case GS_INTERMISSION:
      WI_End();
    default:
      break;
    }
    prevgamestate = gamestate;
  }

  // do main actions
  switch (gamestate)
    {
    case GS_LEVEL:
      P_Ticker ();
      ST_Ticker ();
      AM_Ticker ();
      HU_Ticker ();
      break;

    case GS_INTERMISSION:
      WI_Ticker ();
      break;

    case GS_FINALE:
      F_Ticker ();
      break;

    case GS_DEMOSCREEN:
      D_PageTicker ();
      break;
    }
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//

void G_PlayerFinishLevel(int player)
{
  player_t *p = &players[player];
  memset(p->powers, 0, sizeof p->powers);
  memset(p->cards, 0, sizeof p->cards);
  p->mo = NULL;           // cph - this is allocated PU_LEVEL so it's gone
  p->extralight = 0;      // cancel gun flashes
  p->fixedcolormap = 0;   // cancel ir gogles
  p->damagecount = 0;     // no palette changes
  p->bonuscount = 0;
}

// CPhipps - G_SetPlayerColour
// Player colours stuff
// 
// G_SetPlayerColour

#include "r_draw.h"
extern byte playernumtotrans[MAXPLAYERS];

void G_ChangedPlayerColour(int pn, int cl)
{
  int i;

  if (!netgame) return;

  mapcolor_plyr[pn] = cl;

  // Rebuild colour translation tables accordingly
  R_InitTranslationTables();
  // Change translations on existing player mobj's
  for (i=0; i<MAXPLAYERS; i++) {
    if ((gamestate == GS_LEVEL) && playeringame[i] && (players[i].mo != NULL)) {
      players[i].mo->flags &= ~MF_TRANSLATION;
      players[i].mo->flags |= playernumtotrans[i] << MF_TRANSSHIFT;      
    }
  }
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//

void G_PlayerReborn (int player)
{
  player_t *p;
  int i;
  int frags[MAXPLAYERS];
  int killcount;
  int itemcount;
  int secretcount;

  memcpy (frags, players[player].frags, sizeof frags);
  killcount = players[player].killcount;
  itemcount = players[player].itemcount;
  secretcount = players[player].secretcount;

  p = &players[player];

  // killough 3/10/98,3/21/98: preserve cheats across idclev
  {
    int cheats = p->cheats;
    memset (p, 0, sizeof(*p));
    p->cheats = cheats;
  }

  memcpy(players[player].frags, frags, sizeof(players[player].frags));
  players[player].killcount = killcount;
  players[player].itemcount = itemcount;
  players[player].secretcount = secretcount;

  p->usedown = p->attackdown = true;  // don't do anything immediately
  p->playerstate = PST_LIVE;
  p->health = initial_health;  // Ty 03/12/98 - use dehacked values
  p->readyweapon = p->pendingweapon = wp_pistol;
  p->weaponowned[wp_fist] = true;
  p->weaponowned[wp_pistol] = true;
  p->ammo[am_clip] = initial_bullets; // Ty 03/12/98 - use dehacked values

  for (i=0 ; i<NUMAMMO ; i++)
    p->maxammo[i] = maxammo[i];
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//

boolean G_CheckSpot(int playernum, mapthing_t *mthing)
{
  fixed_t     x,y;
  subsector_t *ss;
  unsigned    an;
  mobj_t      *mo;
  int         i;

  if (!players[playernum].mo)
    {
      // first spawn of level, before corpses
      for (i=0 ; i<playernum ; i++)
        if (players[i].mo->x == mthing->x << FRACBITS
            && players[i].mo->y == mthing->y << FRACBITS)
          return false;
      return true;
    }

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  // killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
  // corpse to detect collisions with other players in DM starts
  //
  // Old code:
  // if (!P_CheckPosition (players[playernum].mo, x, y))
  //    return false;

  players[playernum].mo->flags |=  MF_SOLID;
  i = P_CheckPosition(players[playernum].mo, x, y);
  players[playernum].mo->flags &= ~MF_SOLID;
  if (!i)
    return false;

  // flush an old corpse if needed
  // killough 2/8/98: make corpse queue have an adjustable limit
  if (bodyquesize > 0)
    {
//    static mobj_t **bodyque;       // phares 8/10/98: moved outside routine
      if (!bodyque)
        bodyque = calloc(bodyquesize,sizeof*bodyque);
      if (bodyque[bodyqueslot])
        P_RemoveMobj(bodyque[bodyqueslot]);
      bodyque[bodyqueslot] = players[playernum].mo;
      if (++bodyqueslot >= bodyquesize)
        bodyqueslot -= bodyquesize;
    }
  else
    if (!bodyquesize)
      P_RemoveMobj(players[playernum].mo);

  // spawn a teleport fog
  ss = R_PointInSubsector (x,y);
  an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;

  mo = P_SpawnMobj(x+20*finecosine[an], y+20*finesine[an],
                   ss->sector->floorheight, MT_TFOG);

  if (players[consoleplayer].viewz != 1)
    S_StartSound(mo, sfx_telept);  // don't start sound on first frame

  return true;
}


// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer (int playernum)
{
  int j, selections = deathmatch_p - deathmatchstarts;

  if (selections < MAXPLAYERS)
    I_Error("G_DeathMatchSpawnPlayer: Only %i deathmatch spots, %d required",
		selections, MAXPLAYERS);

  for (j=0 ; j<20 ; j++)
    {
      int i = P_Random(pr_dmspawn) % selections;
      if (G_CheckSpot (playernum, &deathmatchstarts[i]) )
        {
          deathmatchstarts[i].type = playernum+1;
          P_SpawnPlayer (playernum, &deathmatchstarts[i]);
          return;
        }
    }

  // no good spot, so the player will probably get stuck
  P_SpawnPlayer (playernum, &playerstarts[playernum]);
}

//
// G_DoReborn
//

void G_DoReborn (int playernum)
{
  if (!netgame)
    gameaction = ga_loadlevel;      // reload the level from scratch
  else
    {                               // respawn at the start
      int i;

      // first dissasociate the corpse
      players[playernum].mo->player = NULL;

      // spawn at random spot if in death match
      if (deathmatch)
        {
          G_DeathMatchSpawnPlayer (playernum);
          return;
        }

      if (G_CheckSpot (playernum, &playerstarts[playernum]) )
        {
          P_SpawnPlayer (playernum, &playerstarts[playernum]);
          return;
        }

      // try to spawn at one of the other players spots
      for (i=0 ; i<MAXPLAYERS ; i++)
        {
          if (G_CheckSpot (playernum, &playerstarts[i]) )
            {
              P_SpawnPlayer (playernum, &playerstarts[i]);
              return;
            }
          // he's going to be inside something.  Too bad.
        }
      P_SpawnPlayer (playernum, &playerstarts[playernum]);
    }
}

// DOOM Par Times
int pars[4][10] = {
  {0},
  {0,30,75,120,90,165,180,180,30,165},
  {0,90,90,90,120,90,360,240,30,170},
  {0,90,45,90,150,90,90,165,30,135}
};

// DOOM II Par Times
int cpars[32] = {
  30,90,120,120,90,150,120,120,270,90,  //  1-10
  210,150,150,150,210,150,420,150,210,150,  // 11-20
  240,150,180,150,150,300,330,420,300,180,  // 21-30
  120,30          // 31-32
};

static boolean secretexit;

void G_ExitLevel (void)
{
  secretexit = false;
  gameaction = ga_completed;
}

// Here's for the german edition.
// IF NO WOLF3D LEVELS, NO SECRET EXIT!

void G_SecretExitLevel (void)
{
  if (gamemode!=commercial || haswolflevels)
    secretexit = true;
  else
    secretexit = false;
  gameaction = ga_completed;
}

//
// G_DoCompleted
//

void G_DoCompleted (void)
{
  int i;

  gameaction = ga_nothing;

  for (i=0; i<MAXPLAYERS; i++)
    if (playeringame[i])
      G_PlayerFinishLevel(i);        // take away cards and stuff

  if (automapmode & am_active)
    AM_Stop();

  if (gamemode != commercial) // kilough 2/7/98
    switch(gamemap)
      {
	// cph - Remove ExM8 special case, so it gets summary screen displayed
      case 9:
        for (i=0 ; i<MAXPLAYERS ; i++)
          players[i].didsecret = true;
        break;
      }

  wminfo.didsecret = players[consoleplayer].didsecret;
  wminfo.epsd = gameepisode -1;
  wminfo.last = gamemap -1;

  // wminfo.next is 0 biased, unlike gamemap
  if (gamemode == commercial)
    {
      if (secretexit)
        switch(gamemap)
          {
          case 15:
            wminfo.next = 30; break;
          case 31:
            wminfo.next = 31; break;
          }
      else
        switch(gamemap)
          {
          case 31:
          case 32:
            wminfo.next = 15; break;
          default:
            wminfo.next = gamemap;
          }
    }
  else
    {
      if (secretexit)
        wminfo.next = 8;  // go to secret level
      else
        if (gamemap == 9)
          {
            // returning from secret level
            switch (gameepisode)
              {
              case 1:
                wminfo.next = 3;
                break;
              case 2:
                wminfo.next = 5;
                break;
              case 3:
                wminfo.next = 6;
                break;
              case 4:
                wminfo.next = 2;
                break;
              }
          }
        else
          wminfo.next = gamemap;          // go to next level
    }

  wminfo.maxkills = totalkills;
  wminfo.maxitems = totalitems;
  wminfo.maxsecret = totalsecret;
  wminfo.maxfrags = 0;

  if ( gamemode == commercial )
    wminfo.partime = TICRATE*cpars[gamemap-1];
  else
    wminfo.partime = TICRATE*pars[gameepisode][gamemap];

  wminfo.pnum = consoleplayer;

  for (i=0 ; i<MAXPLAYERS ; i++)
    {
      wminfo.plyr[i].in = playeringame[i];
      wminfo.plyr[i].skills = players[i].killcount;
      wminfo.plyr[i].sitems = players[i].itemcount;
      wminfo.plyr[i].ssecret = players[i].secretcount;
      wminfo.plyr[i].stime = leveltime;
      memcpy (wminfo.plyr[i].frags, players[i].frags,
              sizeof(wminfo.plyr[i].frags));
    }

  /* cph - modified so that only whole seconds are added to the totalleveltimes
   *  value; so our total is compatible with the "naive" total of just adding
   *  the times in seconds shown for each level. Also means our total time
   *  will agree with Compet-n.
   */
  wminfo.totaltimes = (totalleveltimes += (leveltime - leveltime%35));

  gamestate = GS_INTERMISSION;
  automapmode &= ~am_active;

  if (statcopy)
    memcpy (statcopy, &wminfo, sizeof(wminfo));

  WI_Start (&wminfo);
}

//
// G_WorldDone
//

void G_WorldDone (void)
{
  gameaction = ga_worlddone;

  if (secretexit)
    players[consoleplayer].didsecret = true;

  if (gamemode == commercial)
    {
      switch (gamemap)
        {
        case 15:
        case 31:
          if (!secretexit)
            break;
        case 6:
        case 11:
        case 20:
        case 30:
          F_StartFinale ();
          break;
        }
    }
  else if (gamemap == 8)
    gameaction = ga_victory; // cph - after ExM8 summary screen, show victory stuff
}

void G_DoWorldDone (void)
{
  idmusnum = -1;             //jff 3/17/98 allow new level's music to be loaded
  gamestate = GS_LEVEL;
  gamemap = wminfo.next+1;
  G_DoLoadLevel();
  gameaction = ga_nothing;
  AM_clearMarks();           //jff 4/12/98 clear any marks on the automap
}

// killough 2/28/98: A ridiculously large number
// of players, the most you'll ever need in a demo
// or savegame. This is used to prevent problems, in
// case more players in a game are supported later.

#define MIN_MAXPLAYERS 32

extern boolean setsizeneeded;
void R_ExecuteSetViewSize(void);

//CPhipps - savename variable redundant

/* killough 12/98:
 * This function returns a signature for the current wad.
 * It is used to distinguish between wads, for the purposes
 * of savegame compatibility warnings, and options lookups.
 */

static uint_64_t G_UpdateSignature(uint_64_t s, const char *name)
{
  int i, lump = W_CheckNumForName(name);
  if (lump != -1 && (i = lump+10) < numlumps)
    do
      {
	int size = W_LumpLength(i);
	const byte *p = W_CacheLumpNum(i);
	while (size--)
	  s <<= 1, s += *p++;
	W_UnlockLumpNum(i);
      }
    while (--i > lump);
  return s;
}

static uint_64_t G_Signature(void)
{
  static uint_64_t s = 0;
  static boolean computed = false;
  char name[9];
  int episode, map;

  if (!computed) {
   computed = true;
   if (gamemode == commercial)
    for (map = haswolflevels ? 32 : 30; map; map--)
      sprintf(name, "map%02d", map), s = G_UpdateSignature(s, name);
   else
    for (episode = gamemode==retail ? 4 :
	   gamemode==shareware ? 1 : 3; episode; episode--)
      for (map = 9; map; map--)
	sprintf(name, "E%dM%d", episode, map), s = G_UpdateSignature(s, name);
  }
  return s;
}

//
// killough 5/15/98: add forced loadgames, which allow user to override checks
//

void G_ForcedLoadGame(void)
{
  // CPhipps - net loadgames are always forced, so we only reach here 
  //  in single player
  gameaction = ga_loadgame;
  forced_loadgame = true;
}

// killough 3/16/98: add slot info
// killough 5/15/98: add command-line
void G_LoadGame(int slot, boolean command)
{
  if (!demoplayback && !command) {
    // CPhipps - handle savegame filename in G_DoLoadGame
    //         - Delay load so it can be communicated in net game
    //         - store info in special_event
    special_event = BT_SPECIAL | (BTS_LOADGAME & BT_SPECIALMASK) | 
      ((slot << BTS_SAVESHIFT) & BTS_SAVEMASK);
    forced_loadgame = netgame; // CPhipps - always force load netgames
  } else {
    // Do the old thing, immediate load
    gameaction = ga_loadgame;
    forced_loadgame = false;
    savegameslot = slot;
    demoplayback = false;
  }
  command_loadgame = command;
}

// killough 5/15/98:
// Consistency Error when attempting to load savegame.

static void G_LoadGameErr(const char *msg)
{
  free(savebuffer);                  // Free the savegame buffer
  MN_ForcedLoadGame(msg);             // Print message asking for 'Y' to force
  if (command_loadgame)              // If this was a command-line -loadgame
    {
      D_StartTitle();                // Start the title screen
      gamestate = GS_DEMOSCREEN;     // And set the game state accordingly
    }
}

// CPhipps - size of version header
#define VERSIONSIZE   16

const char * comp_lev_str[] = 
{ "doom v1.2", "doom v1.666", "doom2 v1.9",
  "ultimate doom v1.9", "final doom/doom95", 
  "early DosDoom", "TASDoom",
  "\"boom compatibility\"", "boom v2.01", "boom v2.02", "lxdoom v1.3.2+", 
  "MBF", "PrBoom 2.03beta", "PrBoom v2.1.0-2.1.1", "PrBoom v2.1.2-v2.2.0",
  "Current PrBoom"  };

static const struct {
  int comp_level;
  const char* ver_printf;
  int version;
} version_headers[] = {
  { prboom_3_compatibility, "PrBoom %d", 210},
  /* cph 2001/08/15 - need a new savegame format, which saves the compatibility
   *  level in the savegame file - should've done this from the start.
   * The entry above is only for compat with v2.2.1 savegames, should be dropped
   *  later.
   */
  { prboom_4_compatibility, "PrBoom %d", 211}
};

static const size_t num_version_headers = sizeof(version_headers) / sizeof(version_headers[0]);

void G_DoLoadGame(void)
{
  int  length, i;
  // CPhipps - do savegame filename stuff here
  char name[PATH_MAX+1];     // killough 3/22/98
  int savegame_compatibility = -1;

  G_SaveGameName(name,sizeof(name),savegameslot, demoplayback);

  gameaction = ga_nothing;

  length = M_ReadFile(name, &savebuffer);
  if (length<=0)
    I_Error("Couldn't read file %s: %s", name, "(Unknown Error)");
  save_p = savebuffer + SAVESTRINGSIZE;

  // CPhipps - read the description field, compare with supported ones
  for (i=0; (size_t)i<num_version_headers; i++) {
    char vcheck[VERSIONSIZE];
    // killough 2/22/98: "proprietary" version string :-)
    sprintf (vcheck, version_headers[i].ver_printf, version_headers[i].version);

    if (!strncmp(save_p, vcheck, VERSIONSIZE)) {
      savegame_compatibility = version_headers[i].comp_level;
      i = num_version_headers;
    }
  }
  if (savegame_compatibility == -1) {
    if (forced_loadgame) {
      savegame_compatibility = MAX_COMPATIBILITY_LEVEL-1;
    } else {
      G_LoadGameErr("Unrecognised savegame version!\nAre you sure? (y/n) ");
      return;
    }
  }

  save_p += VERSIONSIZE;

  // CPhipps - always check savegames even when forced, 
  //  only print a warning if forced
  {  // killough 3/16/98: check lump name checksum (independent of order)
    uint_64_t checksum = 0;

    checksum = G_Signature();

    if (memcmp(&checksum, save_p, sizeof checksum)) {
      if (!forced_loadgame) {
        char *msg = malloc(strlen(save_p + sizeof checksum) + 128);
        strcpy(msg,"Incompatible Savegame!!!\n");
        if (save_p[sizeof checksum])
          strcat(strcat(msg,"Wads expected:\n\n"), save_p + sizeof checksum);
        strcat(msg, "\nAre you sure?");
        G_LoadGameErr(msg);
        free(msg);
        return;
      } else 
	lprintf(LO_WARN, "G_DoLoadGame: Incompatible savegame\n");
    }
    save_p += sizeof checksum;
   }

  save_p += strlen(save_p)+1;

  compatibility_level = (savegame_compatibility >= prboom_4_compatibility)
    ? *save_p : savegame_compatibility;
  save_p++;

  gameskill = *save_p++;
  gameepisode = *save_p++;
  gamemap = *save_p++;

  for (i=0 ; i<MAXPLAYERS ; i++)
    playeringame[i] = *save_p++;
  save_p += MIN_MAXPLAYERS-MAXPLAYERS;         // killough 2/28/98

  idmusnum = *save_p++;           // jff 3/17/98 restore idmus music
  if (idmusnum==255) idmusnum=-1; // jff 3/18/98 account for unsigned byte

  /* killough 3/1/98: Read game options
   * killough 11/98: move down to here
   */
  save_p = (char*)G_ReadOptions(save_p);

  // load a base level
  G_InitNew (gameskill, gameepisode, gamemap);

  /* get the times - killough 11/98: save entire word */
  memcpy(&leveltime, save_p, sizeof leveltime);
  save_p += sizeof leveltime;

  /* cph - total episode time */
  if (compatibility_level >= prboom_2_compatibility) {
    memcpy(&totalleveltimes, save_p, sizeof totalleveltimes);
    save_p += sizeof totalleveltimes;
  }
  else totalleveltimes = 0;

  // killough 11/98: load revenant tracer state
  basetic = gametic - *save_p++;

  // dearchive all the modifications
  P_UnArchivePlayers ();
  P_UnArchiveWorld ();
  P_UnArchiveThinkers ();
  P_UnArchiveSpecials ();
  P_UnArchiveRNG ();    // killough 1/18/98: load RNG information
  P_UnArchiveMap ();    // killough 1/22/98: load automap information

  if (*save_p != 0xe6)
    I_Error ("G_DoLoadGame: Bad savegame");

  // done
  free (savebuffer);

  if (setsizeneeded)
    R_ExecuteSetViewSize ();

  // draw the pattern into the back screen
  R_FillBackScreen ();

  /* killough 12/98: support -recordfrom and -loadgame -playdemo */
  if (!command_loadgame)
    singledemo = false;  /* Clear singledemo flag if loading from menu */
  else
    if (singledemo) {
      gameaction = ga_loadgame; /* Mark that we're loading a game before demo */
      G_DoPlayDemo();           /* This will detect it and won't reinit level */
    } else /* Command line + record means it's a recordfrom */
      if (demorecording)
        G_BeginRecording();
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//

void G_SaveGame(int slot, char *description)
{
  strcpy(savedescription, description);
  if (demoplayback) {
    /* cph - We're doing a user-initiated save game while a demo is 
     * running so, go outside normal mechanisms 
     */
    savegameslot = slot;
    G_DoSaveGame(true);
  }
  // CPhipps - store info in special_event
  special_event = BT_SPECIAL | (BTS_SAVEGAME & BT_SPECIALMASK) | 
    ((slot << BTS_SAVESHIFT) & BTS_SAVEMASK);
#ifdef HAVE_NET
  D_NetSendMisc(nm_savegamename, strlen(savedescription)+1, savedescription);
#endif
}

// Check for overrun and realloc if necessary -- Lee Killough 1/22/98
void CheckSaveGame(size_t size)
{
  size_t pos = save_p - savebuffer;
  size += 1024;  // breathing room
  if (pos+size > savegamesize)
    save_p = (savebuffer = realloc(savebuffer,
           savegamesize += (size+1023) & ~1023)) + pos;
}

/* killough 3/22/98: form savegame name in one location
 * (previously code was scattered around in multiple places)
 * cph - Avoid possible buffer overflow problems by passing 
 * size to this function and using snprintf */

void G_SaveGameName(char *name, size_t size, int slot, boolean demoplayback)
{
  const char* sgn = demoplayback ? "demosav" : savegamename;
#ifdef HAVE_SNPRINTF
  snprintf (name, size, "%s/%s%d.dsg", basesavegame, sgn, slot);
#else 
  sprintf (name, "%s/%s%d.dsg", basesavegame, sgn, slot);
#endif
}

static void G_DoSaveGame (boolean menu)
{
  char name[PATH_MAX+1];
  char name2[VERSIONSIZE];
  char *description;
  int  length, i;

  gameaction = ga_nothing; // cph - cancel savegame at top of this function, 
    // in case later problems cause a premature exit

  G_SaveGameName(name,sizeof(name),savegameslot, demoplayback && !menu);

  description = savedescription;

  save_p = savebuffer = malloc(savegamesize);

  CheckSaveGame(SAVESTRINGSIZE+VERSIONSIZE+sizeof(unsigned long));
  memcpy (save_p, description, SAVESTRINGSIZE);
  save_p += SAVESTRINGSIZE;
  memset (name2,0,sizeof(name2));

  /* cph 2001/08/15 - look for an appropriate version string for the savegame, 
   *  otherwise falling back on the last one in the array, which should be the 
   *  current save format.
   */
  for (i=0; (size_t)i<num_version_headers-1; i++)
    if (version_headers[i].comp_level == compatibility_level) break;

  // killough 2/22/98: "proprietary" version string :-)
  sprintf (name2,version_headers[i].ver_printf,version_headers[i].version);
  memcpy (save_p, name2, VERSIONSIZE);

  save_p += VERSIONSIZE;

  { /* killough 3/16/98, 12/98: store lump name checksum */
    uint_64_t checksum = G_Signature();
    memcpy(save_p, &checksum, sizeof checksum);
    save_p += sizeof checksum;
  }

  // killough 3/16/98: store pwad filenames in savegame
  {
    // CPhipps - changed for new wadfiles handling
    int i = 0;
    for (*save_p = 0; (size_t)i<numwadfiles; i++)
      {
	const char *const w = wadfiles[i].name;
        CheckSaveGame(strlen(w)+2);
        strcat(strcat(save_p, w), "\n");
      }
    save_p += strlen(save_p)+1;
  }

  CheckSaveGame(GAME_OPTION_SIZE+MIN_MAXPLAYERS+10);

  /* cph - FIXME? - Save compatibility level */
  *save_p++ = compatibility_level;

  *save_p++ = gameskill;
  *save_p++ = gameepisode;
  *save_p++ = gamemap;

  for (i=0 ; i<MAXPLAYERS ; i++)
    *save_p++ = playeringame[i];

  for (;i<MIN_MAXPLAYERS;i++)         // killough 2/28/98
    *save_p++ = 0;

  *save_p++ = idmusnum;               // jff 3/17/98 save idmus state

  save_p = G_WriteOptions(save_p);    // killough 3/1/98: save game options

  /* cph - FIXME - endianness? */
  /* killough 11/98: save entire word */
  memcpy(save_p, &leveltime, sizeof leveltime);
  save_p += sizeof leveltime;

  /* cph - total episode time */
  if (compatibility_level >= prboom_2_compatibility) {
    memcpy(save_p, &totalleveltimes, sizeof totalleveltimes);
    save_p += sizeof totalleveltimes;
  }
  else totalleveltimes = 0;

  // killough 11/98: save revenant tracer state
  *save_p++ = (gametic-basetic) & 255;

  // killough 3/22/98: add Z_CheckHeap after each call to ensure consistency
  Z_CheckHeap();
  P_ArchivePlayers();
  Z_CheckHeap();

  // phares 9/13/98: Move mobj_t->index out of P_ArchiveThinkers so the
  // indices can be used by P_ArchiveWorld when the sectors are saved.
  // This is so we can save the index of the mobj_t of the thinker that
  // caused a sound, referenced by sector_t->soundtarget.
  P_ThinkerToIndex();

  P_ArchiveWorld();
  Z_CheckHeap();
  P_ArchiveThinkers();

  // phares 9/13/98: Move index->mobj_t out of P_ArchiveThinkers, simply
  // for symmetry with the P_ThinkerToIndex call above.
  
  P_IndexToThinker();

  Z_CheckHeap();
  P_ArchiveSpecials();
  P_ArchiveRNG();    // killough 1/18/98: save RNG information
  Z_CheckHeap();
  P_ArchiveMap();    // killough 1/22/98: save automap information

  *save_p++ = 0xe6;   // consistancy marker

  length = save_p - savebuffer;

  Z_CheckHeap();
  doom_printf( "%s", M_WriteFile(name, savebuffer, length) 
	       ? s_GGSAVED /* Ty - externalised */
	       : "Game save failed!"); // CPhipps - not externalised

  free(savebuffer);  // killough
  savebuffer = save_p = NULL;

  savedescription[0] = 0;
}

static skill_t d_skill;
static int     d_episode;
static int     d_map;

void G_DeferedInitNew(skill_t skill, int episode, int map)
{
  d_skill = skill;
  d_episode = episode;
  d_map = map;
  gameaction = ga_newgame;
}

extern int variable_friction;
extern int default_variable_friction;  // ice & mud

extern int weapon_recoil, default_weapon_recoil;    // weapon recoil

extern int allow_pushers;
extern int default_allow_pushers;     // MT_PUSH Things

extern int player_bobbing;
extern int default_player_bobbing;    // whether player bobs or not

extern int monsters_remember, default_monsters_remember;

/* cph - 
 * G_Compatibility
 *
 * Initialises the comp[] array based on the compatibility_level
 * For reference, MBF did:
 * for (i=0; i < COMP_TOTAL; i++)
 *   comp[i] = compatibility;
 *
 * Instead, we have a lookup table showing at what version a fix was 
 *  introduced.
 */

static byte comp_options_by_version[] = 
 { 0,0,0,0,0, /* Original Doom's don't have comp[] */
   0,0,0,0,0,0, /* Nor did DosDoom, Boom, LxDoom */
   19,19, /* MBF and early PrBoom had 19 */
   21,21, /* PrBoom v2.1-v2.2 have 21 */
   25, /* PrBoom v2.3 still counting... */
 };

static void G_Compatibility(void)
{
  static const complevel_t fix_levels[COMP_NUM] = {
    mbf_compatibility, /* comp_telefrag - monsters used to telefrag only 
			* on MAP30, now they do it for spawners only */
    mbf_compatibility, /* comp_dropoff - MBF encourages things to drop
			* off of overhangs */
    boom_compatibility,/* comp_vile - original Doom archville bugs like 
			* ghosts */
    boom_compatibility,/* comp_pain - original Doom limits Pain Elements
			* from spawning too many skulls */
    boom_compatibility,/* comp_skull - original Doom let skulls be spit 
			* through walls by Pain Elementals */
    boom_compatibility,/* comp_blazing - original Doom duplicated 
			* blazing door sound */
    mbf_compatibility, /* comp_doorlight - MBF made door lighting changes 
			* more gradual */
    boom_compatibility,/* comp_model - improvements to the game physics */
    boom_compatibility,/* comp_god - fixes to God mode */
    mbf_compatibility, /* comp_falloff - MBF encourages things to drop
			* off of overhangs */
    boom_compatibility_compatibility,
                       /* comp_floors - fixes for moving floors bugs */
    boom_compatibility,/* comp_skymap */
    mbf_compatibility, /* comp_pursuit - MBF AI change, limited pursuit? */
    boom_compatibility,/* comp_doorstuck - monsters stuck in doors fix */
    mbf_compatibility, /* comp_staylift - MBF AI change, monsters try
			* to stay on lifts */
    lxdoom_1_compatibility, /* comp_zombie - prevent dead players 
			     * triggering stuff */
    boom_compatibility_compatibility,  /* comp_stairs - see p_floor.c */
    mbf_compatibility, /* comp_infcheat - FIXME */
    boom_compatibility,/* comp_zerotags - allow zero tags in wads */
    lxdoom_1_compatibility, /* comp_moveblock - enables keygrab and 
			     * mancubi shots going thru walls */
    prboom_2_compatibility, /* comp_respawn - objects which aren't on the map 
                             * at game start respawn at (0,0) */
    doom_1666_compatibility, /* comp_666 - enables tag 666 in non-E1Mx levels */
    prboom_4_compatibility, /* comp_soul - enables lost souls bouncing (see P_ZMovement */
    doom_1666_compatibility, /* comp_maskedanim - 2s mid textures don't animate */
    boom_compatibility_compatibility,  /* comp_sound - see s_sound.c */
  };
  int i;
  if (sizeof(comp_options_by_version) != MAX_COMPATIBILITY_LEVEL)
    I_Error("G_Compatibility: consistency error");
  for (i=comp_options_by_version[compatibility_level]; i<COMP_NUM; i++)
    comp[i] = compatibility_level < fix_levels[i];
  for (; i<COMP_TOTAL; i++) comp[i] = 1;
}

#ifdef DOGS
/* killough 7/19/98: Marine's best friend :) */
static int G_GetHelpers(void)
{
  int j = M_CheckParm ("-dog");

  if (!j)
    j = M_CheckParm ("-dogs");
  return j ? j+1 < myargc ? atoi(myargv[j+1]) : 1 : default_dogs;
}
#endif

// killough 3/1/98: function to reload all the default parameter
// settings before a new game begins

void G_ReloadDefaults(void)
{
  // killough 3/1/98: Initialize options based on config file
  // (allows functions above to load different values for demos
  // and savegames without messing up defaults).

  weapon_recoil = default_weapon_recoil;    // weapon recoil

  player_bobbing = default_player_bobbing;  // whether player bobs or not

  variable_friction = allow_pushers = true;

  monsters_remember = default_monsters_remember;   // remember former enemies

  monster_infighting = default_monster_infighting; // killough 7/19/98

#ifdef DOGS
  dogs = netgame ? 0 : G_GetHelpers();             // killough 7/19/98
  dog_jumping = default_dog_jumping;
#endif

  distfriend = default_distfriend;                 // killough 8/8/98

  monster_backing = default_monster_backing;     // killough 9/8/98

  monster_avoid_hazards = default_monster_avoid_hazards; // killough 9/9/98

  monster_friction = default_monster_friction;     // killough 10/98

  help_friends = default_help_friends;             // killough 9/9/98

  monkeys = default_monkeys;

  // jff 1/24/98 reset play mode to command line spec'd version
  // killough 3/1/98: moved to here
  respawnparm = clrespawnparm;
  fastparm = clfastparm;
  nomonsters = clnomonsters;

  //jff 3/24/98 set startskill from defaultskill in config file, unless
  // it has already been set by a -skill parameter
  if (startskill==sk_none)
    startskill = (skill_t)(defaultskill-1);

  demoplayback = false;
  singledemo = false;            // killough 9/29/98: don't stop after 1 demo
  netdemo = false;

  // killough 2/21/98:
  memset(playeringame+1, 0, sizeof(*playeringame)*(MAXPLAYERS-1));

  consoleplayer = 0;

  compatibility_level = default_compatibility_level;
  {
    int i = M_CheckParm("-complevel");
    if (i && (1+i) < myargc) compatibility_level = atoi(myargv[i+1]);
  }
  if (compatibility_level == -1) 
    compatibility_level = best_compatibility;

  if (mbf_features)
    memcpy(comp, default_comp, sizeof comp);
  G_Compatibility();

  // killough 3/31/98, 4/5/98: demo sync insurance
  demo_insurance = default_demo_insurance == 1;

  rngseed += I_GetRandomTimeSeed() + gametic; // CPhipps
}

void G_DoNewGame (void)
{
  G_ReloadDefaults();            // killough 3/1/98
  netgame = false;               // killough 3/29/98
  deathmatch = false;
  G_InitNew (d_skill, d_episode, d_map);
  gameaction = ga_nothing;

  //jff 4/26/98 wake up the status bar in case were coming out of a DM demo
  ST_Start();
}

// killough 4/10/98: New function to fix bug which caused Doom
// lockups when idclev was used in conjunction with -fast.

void G_SetFastParms(int fast_pending)
{
  static int fast = 0;            // remembers fast state
  int i;
  if (fast != fast_pending) {     /* only change if necessary */
    if ((fast = fast_pending))
      {
        for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
          if (states[i].tics != 1 || demo_compatibility) // killough 4/10/98
            states[i].tics >>= 1;  // don't change 1->0 since it causes cycles
        mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
      }
    else
      {
        for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
          states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
      }
  }
}

// The sky texture to be used instead of the F_SKY1 dummy.
extern int skytexture;

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//

void G_InitNew(skill_t skill, int episode, int map)
{
  int i;

  if (paused)
    {
      paused = false;
      S_ResumeSound();
    }

  if (skill > sk_nightmare)
    skill = sk_nightmare;

  if (episode < 1)
    episode = 1;

  if (gamemode == retail)
    {
      if (episode > 4)
        episode = 4;
    }
  else
    if (gamemode == shareware)
      {
        if (episode > 1)
          episode = 1; // only start episode 1 on shareware
      }
    else
      if (episode > 3)
        episode = 3;

  if (map < 1)
    map = 1;
  if (map > 9 && gamemode != commercial)
    map = 9;

  G_SetFastParms(fastparm || skill == sk_nightmare);  // killough 4/10/98

  M_ClearRandom();

  respawnmonsters = skill == sk_nightmare || respawnparm;

  // force players to be initialized upon first level load
  for (i=0 ; i<MAXPLAYERS ; i++)
    players[i].playerstate = PST_REBORN;

  usergame = true;                // will be set false if a demo
  paused = false;
  automapmode &= ~am_active;
  gameepisode = episode;
  gamemap = map;
  gameskill = skill;

  totalleveltimes = 0; // cph

  //jff 4/16/98 force marks on automap cleared every new level start
  AM_clearMarks();

  G_DoLoadLevel ();
}

//
// DEMO RECORDING
//

#define DEMOMARKER    0x80

void G_ReadDemoTiccmd (ticcmd_t* cmd)
{
  if (*demo_p == DEMOMARKER)
    G_CheckDemoStatus();      // end of demo data stream
  else
    {
      cmd->forwardmove = ((signed char)*demo_p++);
      cmd->sidemove = ((signed char)*demo_p++);
      cmd->angleturn = ((unsigned char)*demo_p++)<<8;
      cmd->buttons = (unsigned char)*demo_p++;
    }
}

/* Demo limits removed -- killough
 * cph - record straight to file
 */
static inline signed char fudge(signed char b)
{
  b |= 1; if (b>2) b-=2;
  return b;
}

void G_WriteDemoTiccmd (ticcmd_t* cmd)
{
  char buf[4];

  buf[0] = (cmd->forwardmove && demo_compatibility) ?
    fudge(cmd->forwardmove) : cmd->forwardmove;
  buf[1] = cmd->sidemove;
  buf[2] = (cmd->angleturn+128)>>8;
  buf[3] = cmd->buttons;
  if (fwrite(buf, sizeof(buf), 1, demofp) != 1)
    I_Error("G_WriteDemoTiccmd: error writing demo");

  /* cph - alias demo_p to it so we can read it back */
  demo_p = buf;
  G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same
}

//
// G_RecordDemo
//

void G_RecordDemo (const char* name)
{
  char     demoname[PATH_MAX];
  usergame = false;  
  AddDefaultExtension(strcpy(demoname, name), ".lmp");  // 1/18/98 killough
  demorecording = true;
  /* cph - Record demos straight to file
   * If file already exists, try to continue existing demo
   */
  if (access(demoname, F_OK)) {
    demofp = fopen(demoname, "wb");
  } else {
    demofp = fopen(demoname, "r+");
    if (demofp) {
      int slot = -1;
      int rc;
      {
	byte buf[200];
	size_t len;
	fread(buf, 1, sizeof(buf), demofp);
	
	len = G_ReadDemoHeader(buf) - buf;
	fseek(demofp, len, SEEK_SET);
      }
      do {
	byte buf[4];
	rc = fread(buf, 1, sizeof(buf), demofp);
	if (buf[0] == DEMOMARKER) break;
	if (buf[3] & BT_SPECIAL)
	  if ((buf[3] & BT_SPECIALMASK) == BTS_SAVEGAME)
	    slot = (buf[3] & BTS_SAVEMASK)>>BTS_SAVESHIFT;
      } while (rc == /* sizeof(buf) is out of scope here */ 4 );
      if (slot == -1) I_Error("G_RecordDemo: No save in demo, can't continue");
      fseek(demofp, -rc, SEEK_CUR);
      G_LoadGame(slot, false);
      autostart = false;
    }
  }
  if (!demofp)
  	I_Error("G_RecordDemo: failed to open %s", name);
}

// These functions are used to read and write game-specific options in demos
// and savegames so that demo sync is preserved and savegame restoration is
// complete. Not all options (for example "compatibility"), however, should
// be loaded and saved here. It is extremely important to use the same
// positions as before for the variables, so if one becomes obsolete, the
// byte(s) should still be skipped over or padded with 0's.
// Lee Killough 3/1/98

extern int forceOldBsp;

byte *G_WriteOptions(byte *demo_p)
{
  byte *target = demo_p + GAME_OPTION_SIZE;

  *demo_p++ = monsters_remember;  // part of monster AI

  *demo_p++ = variable_friction;  // ice & mud

  *demo_p++ = weapon_recoil;      // weapon recoil

  *demo_p++ = allow_pushers;      // MT_PUSH Things

  *demo_p++ = 0;

  *demo_p++ = player_bobbing;  // whether player bobs or not

  // killough 3/6/98: add parameters to savegame, move around some in demos
  *demo_p++ = respawnparm;
  *demo_p++ = fastparm;
  *demo_p++ = nomonsters;

  *demo_p++ = demo_insurance;        // killough 3/31/98

  // killough 3/26/98: Added rngseed. 3/31/98: moved here
  *demo_p++ = (byte)((rngseed >> 24) & 0xff);
  *demo_p++ = (byte)((rngseed >> 16) & 0xff);
  *demo_p++ = (byte)((rngseed >>  8) & 0xff);
  *demo_p++ = (byte)( rngseed        & 0xff);

  // Options new to v2.03 begin here

  *demo_p++ = monster_infighting;   // killough 7/19/98

#ifdef DOGS
  *demo_p++ = dogs;                 // killough 7/19/98
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = 0;
  *demo_p++ = 0;

  *demo_p++ = (distfriend >> 8) & 0xff;  // killough 8/8/98  
  *demo_p++ =  distfriend       & 0xff;  // killough 8/8/98  

  *demo_p++ = monster_backing;         // killough 9/8/98

  *demo_p++ = monster_avoid_hazards;    // killough 9/9/98

  *demo_p++ = monster_friction;         // killough 10/98

  *demo_p++ = help_friends;             // killough 9/9/98

#ifdef DOGS
  *demo_p++ = dog_jumping;
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = monkeys;

  {   // killough 10/98: a compatibility vector now
    int i;
    for (i=0; i < COMP_TOTAL; i++)
      *demo_p++ = comp[i] != 0;
  }

  *demo_p++ = (compatibility_level >= prboom_2_compatibility) && forceOldBsp; // cph 2002/07/20

  //----------------
  // Padding at end
  //----------------
  while (demo_p < target)
    *demo_p++ = 0;

  if (demo_p != target)
    I_Error("G_WriteOptions: GAME_OPTION_SIZE is too small");

  return target;
}

/* Same, but read instead of write
 * cph - const byte*'s
 */

const byte *G_ReadOptions(const byte *demo_p)
{
  const byte *target = demo_p + GAME_OPTION_SIZE;

  monsters_remember = *demo_p++;

  variable_friction = *demo_p;  // ice & mud
  demo_p++;

  weapon_recoil = *demo_p;       // weapon recoil
  demo_p++;

  allow_pushers = *demo_p;      // MT_PUSH Things
  demo_p++;

  demo_p++;

  player_bobbing = *demo_p;     // whether player bobs or not
  demo_p++;

  // killough 3/6/98: add parameters to savegame, move from demo
  respawnparm = *demo_p++;
  fastparm = *demo_p++;
  nomonsters = *demo_p++;

  demo_insurance = *demo_p++;              // killough 3/31/98

  // killough 3/26/98: Added rngseed to demos; 3/31/98: moved here

  rngseed  = *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;

  // Options new to v2.03
  if (mbf_features)
    {
      monster_infighting = *demo_p++;   // killough 7/19/98

#ifdef DOGS
      dogs = *demo_p++;                 // killough 7/19/98
#else
      demo_p++;
#endif

      demo_p += 2;

      distfriend = *demo_p++ << 8;      // killough 8/8/98
      distfriend+= *demo_p++;

      monster_backing = *demo_p++;     // killough 9/8/98

      monster_avoid_hazards = *demo_p++; // killough 9/9/98

      monster_friction = *demo_p++;      // killough 10/98

      help_friends = *demo_p++;          // killough 9/9/98

#ifdef DOGS
      dog_jumping = *demo_p++;           // killough 10/98
#else
      demo_p++;
#endif

      monkeys = *demo_p++;

      {   // killough 10/98: a compatibility vector now
	int i;
	for (i=0; i < COMP_TOTAL; i++)
	  comp[i] = *demo_p++;
      }

      forceOldBsp = *demo_p++; // cph 2002/07/20
    }
  else  /* defaults for versions <= 2.02 */
    {
      monster_infighting = 1;           // killough 7/19/98

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98

      help_friends = 0;                 // killough 9/9/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monkeys = 0;
    }

  G_Compatibility();
  return target;
}

void G_BeginRecording (void)
{
  int i;
  byte *demostart, *demo_p;
  demostart = demo_p = malloc(1000);

  /* cph - 3 demo record formats supported: MBF+, BOOM, and Doom v1.9 */
  if (mbf_features) {
    { /* Write version code into demo */
      unsigned char v;
      switch(compatibility_level) {
	      case mbf_compatibility: v = 204; break;
	      case prboom_2_compatibility: v = 210; break;
	      case prboom_3_compatibility: v = 211; break;
	      case prboom_4_compatibility: v = 212; break;
      }
      *demo_p++ = v;
    }
    
    // signature
    *demo_p++ = 0x1d;
    *demo_p++ = 'M';
    *demo_p++ = 'B';
    *demo_p++ = 'F';
    *demo_p++ = 0xe6;
    *demo_p++ = '\0';
    
    /* killough 2/22/98: save compatibility flag in new demos
     * cph - FIXME? MBF demos will always be not in compat. mode */
    *demo_p++ = 0;
    
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = consoleplayer;
    
    demo_p = G_WriteOptions(demo_p); // killough 3/1/98: Save game options
    
    for (i=0 ; i<MAXPLAYERS ; i++)
      *demo_p++ = playeringame[i];
    
    // killough 2/28/98:
    // We always store at least MIN_MAXPLAYERS bytes in demo, to
    // support enhancements later w/o losing demo compatibility
    
    for (; i<MIN_MAXPLAYERS; i++)
      *demo_p++ = 0;

  } else if (compatibility_level >= boom_compatibility_compatibility) {
    byte v, c; /* Nominally, version and compatibility bits */
    switch (compatibility_level) {
    case boom_compatibility_compatibility: v = 202, c = 1; break;
    case boom_201_compatibility: v = 201; c = 0; break;
    case boom_202_compatibility: v = 202, c = 0; break;
    default: I_Error("G_BeginRecording: Boom compatibility level unrecognised?");
    }
    *demo_p++ = v;
    
    // signature
    *demo_p++ = 0x1d;
    *demo_p++ = 'B';
    *demo_p++ = 'o';
    *demo_p++ = 'o';
    *demo_p++ = 'm';
    *demo_p++ = 0xe6;
    
    /* CPhipps - save compatibility level in demos */
    *demo_p++ = c; 
    
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = consoleplayer;
    
    demo_p = G_WriteOptions(demo_p); // killough 3/1/98: Save game options
    
    for (i=0 ; i<MAXPLAYERS ; i++)
      *demo_p++ = playeringame[i];

    // killough 2/28/98:
    // We always store at least MIN_MAXPLAYERS bytes in demo, to
    // support enhancements later w/o losing demo compatibility
    
    for (; i<MIN_MAXPLAYERS; i++)
      *demo_p++ = 0;
  } else { // cph - write old v1.9 demos (might even sync)
    *demo_p++ = 109; // v1.9 has best chance of syncing these
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;
    for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
      *demo_p++ = playeringame[i];
  }

  if (fwrite(demostart, 1, demo_p-demostart, demofp) != (size_t)(demo_p-demostart))
    I_Error("G_BeginRecording: Error writing demo header");
  free(demostart);
}

//
// G_PlayDemo
//

static const char *defdemoname;

void G_DeferedPlayDemo (const char* name)
{
  defdemoname = name;
  gameaction = ga_playdemo;
}

static int demolumpnum = -1;

static int G_GetOriginalDoomCompatLevel(int ver)
{
  {
    int lev;
    int i = M_CheckParm("-complevel");
    if (i && (i+1 < myargc))
    {
      lev = atoi(myargv[i+1]);
      if (lev>=0)
        return lev;
    }
  }
  return (ver < 107 ? doom_1666_compatibility :
      (gamemode == retail) ? ultdoom_compatibility :
      (gamemission >= pack_tnt) ? finaldoom_compatibility :
      doom2_19_compatibility);
}

static const byte* G_ReadDemoHeader(const byte *demo_p)
{
  skill_t skill;
  int i, episode, map;
  int demover;
  const byte *option_p = NULL;      /* killough 11/98 */

  basetic = gametic;  // killough 9/29/98

  // killough 2/22/98, 2/28/98: autodetect old demos and act accordingly.
  // Old demos turn on demo_compatibility => compatibility; new demos load
  // compatibility flag, and other flags as well, as a part of the demo.

  demover = *demo_p++;

  if (demover < 200)     // Autodetect old demos
    {
      // killough 3/2/98: force these variables to be 0 in demo_compatibility

      variable_friction = 0;

      weapon_recoil = 0;

      allow_pushers = 0;

      monster_infighting = 1;           // killough 7/19/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98
      help_friends = 0;                 // killough 9/9/98
      monkeys = 0;

      // killough 3/6/98: rearrange to fix savegame bugs (moved fastparm,
      // respawnparm, nomonsters flags to G_LoadOptions()/G_SaveOptions())

      if ((skill=demover) >= 100)         // For demos from versions >= 1.4
        {
	  compatibility_level = G_GetOriginalDoomCompatLevel(demover);
          skill = *demo_p++;
          episode = *demo_p++;
          map = *demo_p++;
          deathmatch = *demo_p++;
          respawnparm = *demo_p++;
          fastparm = *demo_p++;
          nomonsters = *demo_p++;
          consoleplayer = *demo_p++;
        }
      else
        {
	  compatibility_level = doom_12_compatibility;
          episode = *demo_p++;
          map = *demo_p++;
          deathmatch = respawnparm = fastparm =
            nomonsters = consoleplayer = 0;
        }
      G_Compatibility();
    }
  else    // new versions of demos
    {
      demo_p += 6;               // skip signature;
      switch (demover) {
      case 200: /* BOOM */
      case 201:
        if (!*demo_p++)
	  compatibility_level = boom_201_compatibility;
        else
	  compatibility_level = boom_compatibility_compatibility;
	break;
      case 202:
        if (!*demo_p++)
	  compatibility_level = boom_202_compatibility; 
        else
	  compatibility_level = boom_compatibility_compatibility;
	break;
      case 203:
	/* LxDoom or MBF - determine from signature
	 * cph - load compatibility level */
	switch (demobuffer[2]) {
	case 'B': /* LxDoom */
	/* cph - DEMOSYNC - LxDoom demos recorded in compatibility modes support dropped */
	  compatibility_level = lxdoom_1_compatibility;
	  break;
	case 'M':
	  compatibility_level = mbf_compatibility;
	  demo_p++;
	  break;
	}
	break;
      case 210:
	compatibility_level = prboom_2_compatibility;
	demo_p++;
	break;
      case 211:
	compatibility_level = prboom_3_compatibility;
	demo_p++;
	break;
      case 212:
	compatibility_level = prboom_4_compatibility;
	demo_p++;
	break;
      }
      skill = *demo_p++;
      episode = *demo_p++;
      map = *demo_p++;
      deathmatch = *demo_p++;
      consoleplayer = *demo_p++;

      /* killough 11/98: save option pointer for below */
      if (mbf_features)
	option_p = demo_p;

      demo_p = G_ReadOptions(demo_p);  // killough 3/1/98: Read game options

      if (demover == 200)              // killough 6/3/98: partially fix v2.00 demos
        demo_p += 128-GAME_OPTION_SIZE;
    }

  if (sizeof(comp_lev_str)/sizeof(comp_lev_str[0]) != MAX_COMPATIBILITY_LEVEL)
    I_Error("G_ReadDemoHeader: compatibility level strings incomplete");
  lprintf(LO_INFO, "G_DoPlayDemo: playing demo with %s compatibility\n", 
	  comp_lev_str[compatibility_level]);

  if (demo_compatibility)  // only 4 players can exist in old demos
    {
      for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
        playeringame[i] = *demo_p++;
      for (;i < MAXPLAYERS; i++)
        playeringame[i] = 0;
    }
  else
    {
      for (i=0 ; i < MAXPLAYERS; i++)
        playeringame[i] = *demo_p++;
      demo_p += MIN_MAXPLAYERS - MAXPLAYERS;
    }

  if (playeringame[1])
    {
      netgame = true;
      netdemo = true;
    }

  if (gameaction != ga_loadgame) { /* killough 12/98: support -loadgame */
    G_InitNew(skill, episode, map);
  }

  for (i=0; i<MAXPLAYERS;i++)         // killough 4/24/98
    players[i].cheats = 0;

  return demo_p;
}

void G_DoPlayDemo(void) 
{
  char basename[9];

  ExtractFileBase(defdemoname,basename);           // killough
  printf("basename: %s\n",basename);
  demobuffer = demo_p = W_CacheLumpNum(demolumpnum = W_GetNumForName(basename));  
  /* cph - store lump number for unlocking later */

  demo_p = G_ReadDemoHeader(demo_p);

  gameaction = ga_nothing;
  usergame = false;

  demoplayback = true;
}

//
// G_TimeDemo
//

void G_TimeDemo(const char *name) // CPhipps - const char*
{
  timingdemo = true;
  singletics = true;
  defdemoname = name;
  gameaction = ga_playdemo;
}

/* G_CheckDemoStatus
 *
 * Called after a death or level completion to allow demos to be cleaned up
 * Returns true if a new demo loop action will take place
 */
boolean G_CheckDemoStatus (void)
{
  if (demorecording)
    {
      demorecording = false;
      fprintf(demofp, "%c", DEMOMARKER);
      I_Error("G_CheckDemoStatus: Demo recorded");
      return false;  // killough
    }

  if (timingdemo)
    {
      int endtime = I_GetTime_RealTime ();
      // killough -- added fps information and made it work for longer demos:
      unsigned realtics = endtime-starttime;
      I_Error ("Timed %u gametics in %u realtics = %-.1f frames per second",
               (unsigned) gametic,realtics,
               (unsigned) gametic * (double) TICRATE / realtics);
    }

  if (demoplayback)
    {
      if (singledemo)
        exit(0);  // killough

      if (demolumpnum != -1) {
	// cph - unlock the demo lump
	W_UnlockLumpNum(demolumpnum);
	demolumpnum = -1;
      }
      G_ReloadDefaults();    // killough 3/1/98
      netgame = false;       // killough 3/29/98
      deathmatch = false;
      D_AdvanceDemo ();
      return true;
    }
  return false;
}

// killough 1/22/98: this is a "Doom printf" for messages. I've gotten
// tired of using players->message=... and so I've added this dprintf.
//
// killough 3/6/98: Made limit static to allow z_zone functions to call
// this function, without calling realloc(), which seems to cause problems.

#define MAX_MESSAGE_SIZE 1024

// CPhipps - renamed to doom_printf to avoid name collision with glibc
void doom_printf(const char *s, ...)
{
  static char msg[MAX_MESSAGE_SIZE];
  va_list v;
  va_start(v,s);
#ifdef HAVE_VSNPRINTF
  vsnprintf(msg,sizeof(msg),s,v);        /* print message in buffer */
#else
  vsprintf(msg,s,v);
#endif
  va_end(v);
  players[consoleplayer].message = msg;  // set new message
}
