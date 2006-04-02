// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//      Networking stuff.
//
//-----------------------------------------------------------------------------

#ifndef __D_NET__
#define __D_NET__

#include "d_player.h"

//
// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.
//

#define DOOMCOM_ID              0x12345678l

// Max computers/players in a game.
#define MAXNETNODES             8


// Networking and tick handling related.
#define BACKUPTICS              12

enum
{
    CMD_SEND    = 1,
    CMD_GET     = 2
};


//
// Network packet data.
//
typedef struct
{
    // High bit is retransmit request.
    unsigned            checksum;
    // Only valid if NCMD_RETRANSMIT.
    byte                retransmitfrom;
    
    byte                starttic;
    byte                player;
    byte                numtics;
    ticcmd_t            cmds[BACKUPTICS];
} doomdata_t;

//
// Startup packet difference
// SG: 4/12/98
// Added so we can send more startup data to synch things like
// bobbing, recoil, etc.
// this is just mapped over the ticcmd_t array when setup packet is sent
//
// Note: the original code takes care of startskill, deathmatch, nomonsters
//       respawn, startepisode, startmap
// Note: for phase 1 we need to add monsters_remember, variable_friction,
//       weapon_recoil, allow_pushers, over_under, player_bobbing,
//       fastparm, demo_insurance, and the rngseed
//Stick all options into bytes so we don't need to mess with bitfields
//WARNING: make sure this doesn't exceed the size of the ticcmds area!
//sizeof(ticcmd_t)*BACKUPTICS
//This is the current length of our extra stuff
//
//killough 5/2/98: this should all be replaced by calls to G_WriteOptions()
//and G_ReadOptions(), which were specifically designed to set up packets.
//By creating a separate struct and functions to read/write the options,
//you now have two functions and data to maintain instead of just one.
//If the array in g_game.c which G_WriteOptions()/G_ReadOptions() operates
//on, is too large (more than sizeof(ticcmd_t)*BACKUPTICS), it can
//either be shortened, or the net code needs to divide it up
//automatically into packets. The STARTUPLEN below is non-portable.
//There's a portable way to do it without having to know the sizes.
//
// NETCODE_FIXME: The comment above is the one I make reference to in
// g_game.c that discusses the fact that this stuff below is pure garbage
// and that something similar to G_Read/WriteOptions needs to be employed
// when sending the values of sync-critical variables over the network at
// game startup. However, if properly designed, the console system should
// probably handle this itself by iterating over all console variables
// and transmitting the sync-critical ones. This will require various
// changes to the netcmds/cvars systems, and will require that ALL sync
// critical variables have cvars. Some currently may not.

#define STARTUPLEN 12
typedef struct
{
  byte monsters_remember;
  byte variable_friction;
  byte weapon_recoil;
  byte allow_pushers;
  byte over_under;
  byte player_bobbing;
  byte fastparm;
  byte demo_insurance;
  unsigned long rngseed;
  char filler[sizeof(ticcmd_t)*BACKUPTICS-STARTUPLEN];
} startup_t;

typedef struct
{
    // Supposed to be DOOMCOM_ID?
    long                id;
    
    // DOOM executes an int to execute commands.
    short               intnum;         
    // Communication between DOOM and the driver.
    // Is CMD_SEND or CMD_GET.
    short               command;
    // Is dest for send, set by get (-1 = no packet).
    short               remotenode;
    
    // Number of bytes in doomdata to be sent
    short               datalength;

    // Info common to all nodes.
    // Console is allways node 0.
    short               numnodes;
    // Flag: 1 = no duplication, 2-5 = dup for slow nets.
    short               ticdup;
    // Flag: 1 = send a backup tic in every packet.
    short               extratics;
    // Flag: 1 = deathmatch.
    short               deathmatch;
    // Flag: -1 = new game, 0-5 = load savegame
    short               savegame;
    short               episode;        // 1-3
    short               map;            // 1-9
    short               skill;          // 1-5

    // Info specific to this node.
    short               consoleplayer;
    short               numplayers;
    
    // These are related to the 3-display mode,
    //  in which two drones looking left and right
    //  were used to render two additional views
    //  on two additional computers.
    // Probably not operational anymore.
    // 1 = left, 0 = center, -1 = right
    short               angleoffset;
    // 1 = drone
    short               drone;          

    // The packet data to be sent.
    doomdata_t          data;
    
} doomcom_t;

// Create any new ticcmds and broadcast to other players.
void NetUpdate (void);

void D_InitNetGame();

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame (void);
void D_KickPlayer (int playernum);

//? how many ticks to run?
void TryRunTics (void);
void Tickers();

extern void (*netdisconnect)();  // function ptr for disconnect function

void ResetNet();

extern int isconsoletic;        // is the current tic a gametic
                                // or a list of console commands?
extern boolean opensocket;
extern doomcom_t singleplayer;
extern int newtics, ticnum;     //sf

#endif

//----------------------------------------------------------------------------
//
// $Log: d_net.h,v $
// Revision 1.8  1998/05/21  12:12:16  jim
// Removed conditional from net code
//
// Revision 1.7  1998/05/16  09:52:21  jim
// add temporary switch for Lee/Stan's code in d_net.c
//
// Revision 1.6  1998/05/03  23:40:38  killough
// Fix net consistency problems, using G_WriteOptions/G_Readoptions
//
// Revision 1.5  1998/04/13  10:40:53  stan
// Now synch up all items identified by Lee Killough as essential to
// game synch (including bobbing, recoil, rngseed).  Commented out
// code in g_game.c so rndseed is always set even in netgame.
//
// Revision 1.4  1998/02/05  12:14:33  phares
// removed dummy comment
//
// Revision 1.3  1998/01/26  19:26:29  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/19  16:38:12  rand
// Added dummy comments to be reomved later
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
