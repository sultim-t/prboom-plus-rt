// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
// Game console variables 
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include <stdio.h>

#include "c_io.h"
//#include "c_net.h"
#include "c_runcmd.h"
#include "doomstat.h"

#include "d_main.h"
#include "f_wipe.h"
#include "g_game.h"
#include "m_random.h"
//#include "p_info.h"
#include "p_mobj.h"
#include "p_inter.h"
#include "p_spec.h"
#include "r_draw.h"

///////////////////////////////////////////////////////////////////////////
//
// 'Defines' -- string aliases for variables
//

char *yesno[]={"no","yes"};
char *onoff[]={"off","on"};

char *colournames[]= {"green","indigo","brown","red","tomato","dirt","blue",
                      "gold","sea","black","purple","vomit", "pink", "cream","white"};
char *textcolours[]=
{
  FC_BRICK  "brick" FC_RED,
  FC_TAN    "tan"   FC_RED,
  FC_GRAY   "gray"  FC_RED,
  FC_GREEN  "green" FC_RED,
  FC_BROWN  "brown" FC_RED,
  FC_GOLD   "gold"  FC_RED,
  FC_RED    "red"   FC_RED,
  FC_BLUE   "blue"  FC_RED,
  FC_ORANGE "orange"FC_RED,
  FC_YELLOW "yellow"FC_RED
};

char *skills[]=
{"im too young to die", "hey not too rough", "hurt me plenty",
 "ultra violence", "nightmare"};
char *bfgtypestr[3]= {"bfg9000", "classic", "bfg11k"};

////////////////////////////////////////////////////////////////////////////
//
// Game variables
//

// player colour
#if 0
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

char *dmstr[] = {"co-op", "deathmatch", "altdeath", "trideath"};

VARIABLE_INT(deathmatch, NULL,                  0, 3, dmstr);
CONSOLE_NETVAR(deathmatch, deathmatch, cf_server|cf_nosave, netcmd_deathmatch) {}

// skill level

VARIABLE_INT(gameskill, &defaultskill,          0, 4, skills);
CONSOLE_NETVAR(skill, gameskill, cf_server, netcmd_skill)
{
  startskill = gameskill = atoi(c_argv[0]);
  if(cmdsrc == consoleplayer)
    defaultskill = gameskill + 1;
}
#endif
// autoaiming 

//VARIABLE_BOOLEAN(autoaim, &default_autoaim,         onoff);
//CONSOLE_NETVAR(autoaim, autoaim, cf_server, netcmd_autoaim) {}

// weapons recoil 

CONSOLE_BOOLEAN(recoil, weapon_recoil, &default_weapon_recoil, onoff, cf_server) {}

// allow pushers

CONSOLE_BOOLEAN(pushers, allow_pushers, &default_allow_pushers, onoff, cf_server|cf_nosave) {}

// varying friction

CONSOLE_BOOLEAN(varfriction, variable_friction, &default_variable_friction, onoff, cf_server|cf_nosave) {}


////////////////////////////////////////////////////////////
//
// Monster variables
//

// fast monsters

CONSOLE_BOOLEAN(fast, fastparm, NULL, onoff, cf_server|cf_nosave)
{
  G_SetFastParms(fastparm); // killough 4/10/98: set -fast parameter correctly
}

// no monsters

CONSOLE_BOOLEAN(nomonsters, nomonsters, NULL, onoff, cf_server|cf_nosave) {}

// respawning monsters

CONSOLE_BOOLEAN(respawn, respawnparm, NULL, onoff, cf_server|cf_nosave) {}

// monsters remember

CONSOLE_BOOLEAN(mon_remember, monsters_remember, &default_monsters_remember, onoff, cf_server) {}

// infighting among monsters

CONSOLE_BOOLEAN(mon_infight, monster_infighting, &default_monster_infighting, onoff, cf_server) {}

// monsters backing out

CONSOLE_BOOLEAN(mon_backing, monster_backing, &default_monster_backing, onoff, cf_server) {}

// monsters avoid hazards

CONSOLE_BOOLEAN(mon_avoid, monster_avoid_hazards, &default_monster_avoid_hazards, onoff, cf_server) {}

// monsters affected by friction

CONSOLE_BOOLEAN(mon_friction, monster_friction, &default_monster_friction, onoff, cf_server) {}

// monsters climb tall steps

CONSOLE_BOOLEAN(mon_climb, monkeys, &default_monkeys, onoff, cf_server) {}

// help dying friends

CONSOLE_BOOLEAN(mon_helpfriends, help_friends, &default_help_friends, onoff, cf_server) {}

// distance friends keep from player

CONSOLE_INT(mon_distfriend, distfriend, &default_distfriend, 0, 1024, NULL, cf_server) {}

void P_Chase_AddCommands();            // p_chase.c
//void P_Skin_AddCommands();             // p_skin.c
//void P_Info_AddCommands();             // p_info.c

void P_AddCommands()
{
  //C_AddCommand(colour);
  //C_AddCommand(deathmatch);
  //C_AddCommand(skill);
  //C_AddCommand(allowmlook);
  //C_AddCommand(bfgtype);
  //C_AddCommand(autoaim);
  C_AddCommand(recoil);
  C_AddCommand(pushers);
  C_AddCommand(varfriction);
  //C_AddCommand(nukage);
  //C_AddCommand(weapspeed);
  //C_AddCommand(bfglook);
  
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
  
  //C_AddCommand(timelimit);
  //C_AddCommand(fraglimit);
  
  P_Chase_AddCommands();
  //P_Skin_AddCommands();
  //P_Info_AddCommands();
}
