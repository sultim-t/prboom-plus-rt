// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
// Game console variables 
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>

#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "doomstat.h"

#include "d_main.h"
#include "f_wipe.h"
#include "g_game.h"
#include "m_random.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_inter.h"
#include "p_spec.h"
#include "r_draw.h"
#include "mn_engin.h"

/***************************************************************************
                'defines': string values for variables
***************************************************************************/

char *yesno[]={"no","yes"};
char *onoff[]={"off","on"};

char *colournames[]= {"green","indigo","brown","red","tomato","dirt","blue",
                      "gold","sea","black","purple","vomit", "pink", "cream","white"};

char *textcolours[]=
{
   FC_BRICK  "brick" FC_NORMAL,
   FC_TAN    "tan"   FC_NORMAL,
   FC_GRAY   "gray"  FC_NORMAL,
   FC_GREEN  "green" FC_NORMAL,
   FC_BROWN  "brown" FC_NORMAL,
   FC_GOLD   "gold"  FC_NORMAL,
   FC_RED    "red"   FC_NORMAL,
   FC_BLUE   "blue"  FC_NORMAL,
   FC_ORANGE "orange"FC_NORMAL,
   FC_YELLOW "yellow"FC_NORMAL
};

char *skills[]=
{
   "im too young to die",
   "hey not too rough",
   "hurt me plenty",
   "ultra violence",
   "nightmare"
};

char *bfgtypestr[5]= { "bfg9000", "classic", "bfg11k", "bouncing bfg", "plasma burst bfg"};
char *dmstr[] = { "single", "coop", "deathmatch" };

/*************************************************************************
        Constants
 *************************************************************************/

// haleyjd: had to change this into a command
CONSOLE_COMMAND(creator, 0)
{
   C_Printf("creator is '%s'\n", LevelInfo.creator);
}

/*************************************************************************
                Game variables
 *************************************************************************/

//
// NETCODE_FIXME: NETWORK VARIABLES
//

// player colour

//
// NETCODE_FIXME: See notes in d_net.c about the handling of player
// colors. It needs serious refinement.
//

VARIABLE_INT(default_colour, NULL, 0, TRANSLATIONCOLOURS-1, colournames);
CONSOLE_NETVAR(colour, default_colour, cf_handlerset, netcmd_colour)
{
   int playernum, colour;
   
   playernum = cmdsrc;
   colour = atoi(c_argv[0]) % TRANSLATIONCOLOURS;
   
   players[playernum].colormap = colour;
   if(gamestate == GS_LEVEL)
      players[playernum].mo->colour = colour;
   
   if(playernum == consoleplayer) default_colour = colour; // typed
}

// deathmatch

//VARIABLE_INT(deathmatch, NULL,                  0, 3, dmstr);
//CONSOLE_NETVAR(deathmatch, deathmatch, cf_server, netcmd_deathmatch) {}

//
// NETCODE_FIXME: Changing gametype at run time could present problems
// unless handled very carefully.
//

VARIABLE_INT(GameType, &DefaultGameType, gt_single, gt_dm, dmstr);
CONSOLE_NETVAR(gametype, GameType, cf_server, netcmd_deathmatch)
{
   if(netgame && GameType == gt_single) // not allowed
      GameType = DefaultGameType = gt_coop;
}

// skill level

VARIABLE_INT(gameskill, &defaultskill,          0, 4, skills);
CONSOLE_NETVAR(skill, gameskill, cf_server, netcmd_skill)
{
   startskill = gameskill = atoi(c_argv[0]);
   if(cmdsrc == consoleplayer)
      defaultskill = gameskill + 1;
}

// allow mlook

VARIABLE_BOOLEAN(allowmlook, &default_allowmlook, onoff);
CONSOLE_NETVAR(allowmlook, allowmlook, cf_server, netcmd_allowmlook) {}

// bfg type

//
// NETCODE_FIXME: Each player should be able to have their own BFG
// type. Changing this will also require propagated changes to the
// weapon system.
//

VARIABLE_INT(bfgtype, &default_bfgtype,         0, 4, bfgtypestr);
CONSOLE_NETVAR(bfgtype, bfgtype, cf_server, netcmd_bfgtype) {}

// autoaiming 

//
// NETCODE_FIXME: Players should be able to have their own autoaim
// settings. Changing this will also require propagated changes to 
// the weapon system.
//

VARIABLE_BOOLEAN(autoaim, &default_autoaim,         onoff);
CONSOLE_NETVAR(autoaim, autoaim, cf_server, netcmd_autoaim) {}

// weapons recoil 

VARIABLE_BOOLEAN(weapon_recoil, &default_weapon_recoil, onoff);
CONSOLE_NETVAR(recoil, weapon_recoil, cf_server, netcmd_recoil) {}

// allow pushers

VARIABLE_BOOLEAN(allow_pushers, &default_allow_pushers, onoff);
CONSOLE_NETVAR(pushers, allow_pushers, cf_server, netcmd_pushers) {}

// varying friction

VARIABLE_BOOLEAN(variable_friction, &default_variable_friction, onoff);
CONSOLE_NETVAR(varfriction, variable_friction, cf_server, netcmd_varfriction){}

// enable nukage

extern int enable_nuke;         // p_spec.c
VARIABLE_BOOLEAN(enable_nuke, NULL, onoff);
CONSOLE_NETVAR(nukage, enable_nuke, cf_server, netcmd_nukage) {}

// weapon changing speed

VARIABLE_INT(weapon_speed, &default_weapon_speed, 1, 200, NULL);
CONSOLE_NETVAR(weapspeed, weapon_speed, cf_server, netcmd_weapspeed) {}

// allow mlook with bfg

//
// NETCODE_FIXME: bfglook is currently a dead option.
//

char *str_bfglook[] = { "off", "on", "fixedgun" };
VARIABLE_INT(bfglook,   NULL,                   0, 2, str_bfglook);
CONSOLE_NETVAR(bfglook, bfglook, cf_server, netcmd_bfglook) {}

// 'auto exit' variables

VARIABLE_INT(levelTimeLimit,    NULL,           0, 100,         NULL);
CONSOLE_NETVAR(timelimit, levelTimeLimit, cf_server, netcmd_timelimit) {}

VARIABLE_INT(levelFragLimit,    NULL,           0, 100,         NULL);
CONSOLE_NETVAR(fraglimit, levelFragLimit, cf_server, netcmd_fraglimit) {}


  /************** monster variables ***********/

// fast monsters

VARIABLE_BOOLEAN(fastparm, &clfastparm,                    onoff);
CONSOLE_NETVAR(fast, fastparm, cf_server, netcmd_fast)
{
   G_SetFastParms(fastparm); // killough 4/10/98: set -fast parameter correctly
}

// no monsters

VARIABLE_BOOLEAN(nomonsters, &clnomonsters,                  onoff);
CONSOLE_NETVAR(nomonsters, nomonsters, cf_server, netcmd_nomonsters)
{
   if(gamestate == GS_LEVEL)
      C_Printf("note: nomonsters will not change until next level\n");
   if(menuactive)
      MN_ErrorMsg("does not take effect until next level");
}

// respawning monsters

VARIABLE_BOOLEAN(respawnparm, &clrespawnparm,                 onoff);
CONSOLE_NETVAR(respawn, respawnparm, cf_server, netcmd_respawn)
{
   if(gamestate == GS_LEVEL)
      C_Printf("note: respawn will change on new game\n");
   if(menuactive)
      MN_ErrorMsg("will take effect on new game");
}

// monsters remember

VARIABLE_BOOLEAN(monsters_remember, &default_monsters_remember, onoff);
CONSOLE_NETVAR(mon_remember, monsters_remember, cf_server, netcmd_monremember) {}

// infighting among monsters

VARIABLE_BOOLEAN(monster_infighting, &default_monster_infighting, onoff);
CONSOLE_NETVAR(mon_infight, monster_infighting, cf_server, netcmd_moninfight) {}

// monsters backing out

VARIABLE_BOOLEAN(monster_backing, &default_monster_backing, onoff);
CONSOLE_NETVAR(mon_backing, monster_backing, cf_server, netcmd_monbacking) {}

// monsters avoid hazards

VARIABLE_BOOLEAN(monster_avoid_hazards, &default_monster_avoid_hazards, onoff);
CONSOLE_NETVAR(mon_avoid, monster_avoid_hazards, cf_server, netcmd_monavoid) {}

// monsters affected by friction

VARIABLE_BOOLEAN(monster_friction, &default_monster_friction, onoff);
CONSOLE_NETVAR(mon_friction, monster_friction, cf_server, netcmd_monfriction) {}

// monsters climb tall steps

VARIABLE_BOOLEAN(monkeys, &default_monkeys,         onoff);
CONSOLE_NETVAR(mon_climb, monkeys, cf_server, netcmd_monclimb) {}

// help dying friends

VARIABLE_BOOLEAN(help_friends, &default_help_friends, onoff);
CONSOLE_NETVAR(mon_helpfriends, help_friends, cf_server, netcmd_monhelpfriends) {}

// distance friends keep from player

VARIABLE_INT(distfriend, &default_distfriend,   0, 1024, NULL);
CONSOLE_NETVAR(mon_distfriend, distfriend, cf_server, netcmd_mondistfriend) {}

void P_Chase_AddCommands(void);
void P_Skin_AddCommands(void);

void P_AddCommands(void)
{
   C_AddCommand(creator);
   
   C_AddCommand(colour);
   C_AddCommand(gametype);
   C_AddCommand(skill);
   C_AddCommand(allowmlook);
   C_AddCommand(bfgtype);
   C_AddCommand(autoaim);
   C_AddCommand(recoil);
   C_AddCommand(pushers);
   C_AddCommand(varfriction);
   C_AddCommand(nukage);
   C_AddCommand(weapspeed);
   C_AddCommand(bfglook);
   
   C_AddCommand(fast);
   C_AddCommand(nomonsters);
   C_AddCommand(respawn);
   C_AddCommand(mon_remember);
   C_AddCommand(mon_infight);
   C_AddCommand(mon_backing);
   C_AddCommand(mon_avoid);
   C_AddCommand(mon_friction);
   C_AddCommand(mon_climb);
   C_AddCommand(mon_helpfriends);
   C_AddCommand(mon_distfriend);
   
   C_AddCommand(timelimit);
   C_AddCommand(fraglimit);
   
   P_Chase_AddCommands();
   P_Skin_AddCommands();
}

// EOF
