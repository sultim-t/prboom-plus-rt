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

#ifndef __C_NET_H__
#define __C_NET_H__

#include "c_runcmd.h"

// net command numbers

enum
{
  netcmd_null,       // 0 is empty
  netcmd_colour,
  netcmd_deathmatch,
  netcmd_exitlevel,
  netcmd_fast,
  netcmd_kill,
  netcmd_name,
  netcmd_nomonsters,
  netcmd_nuke,
  netcmd_respawn,
  netcmd_skill,
  netcmd_skin,
  netcmd_allowmlook,
  netcmd_bobbing, // haleyjd
  netcmd_autoaim,
  netcmd_bfglook,
  netcmd_bfgtype,
  netcmd_recoil,
  netcmd_pushers,
  netcmd_varfriction,
  netcmd_chat,
  netcmd_monremember,
  netcmd_moninfight,
  netcmd_monbacking,
  netcmd_monavoid,
  netcmd_monfriction,
  netcmd_monclimb,
  netcmd_monhelpfriends,
  netcmd_mondistfriend,
  netcmd_map,
  netcmd_weapspeed,
  netcmd_nukage,
  netcmd_timelimit,
  netcmd_fraglimit,
  netcmd_dmflags, // haleyjd 04/14/03
  netcmd_comp_0,
  netcmd_comp_1,
  netcmd_comp_2,
  netcmd_comp_3,
  netcmd_comp_4,
  netcmd_comp_5,
  netcmd_comp_6,
  netcmd_comp_7,
  netcmd_comp_8,
  netcmd_comp_9,
  netcmd_comp_10,
  netcmd_comp_11,
  netcmd_comp_12,
  netcmd_comp_13,
  netcmd_comp_14,
  netcmd_comp_15,
  netcmd_comp_16,
  netcmd_comp_17,
  netcmd_comp_18,
  netcmd_comp_19,   // haleyjd: TerrainTypes
  netcmd_comp_20,   //          respawn fix
  netcmd_comp_21,   //		falling damage
  netcmd_comp_22,   //          lost soul bouncing
  netcmd_comp_23,   //          currently unused
  netcmd_comp_24,   //          extended z clipping
  NUMNETCMDS,
};

void C_InitPlayerName(void); // haleyjd
void C_SendCmd(int dest, int, char *s,...);
void C_queueChatChar(unsigned char c);
unsigned char C_dequeueChatChar(void);
void C_NetTicker(void);
void C_NetInit(void);
void C_SendNetData(void);
void C_UpdateVar(command_t *command);

extern command_t *c_netcmds[NUMNETCMDS];
extern char* default_name;
extern int default_colour;
extern int allowmlook;
extern int cmdsrc;           // the player which started the current command

#define CN_BROADCAST 128

// use the entire ticcmd for transferring console commands when
// in console mode ?

#define CONSHUGE

#endif

// EOF
