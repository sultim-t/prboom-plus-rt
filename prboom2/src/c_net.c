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
 * DESCRIPTION:
 *
 * Console Network support 
 *
 * Network commands can be sent across netgames using 'C_SendCmd'. The
 * command is transferred byte by byte, 1 per tic cmd, using the 
 * chatchar variable (previously used for chat messages)
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdarg.h>
#include "lprintf.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "d_main.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"

#define CMD_END 0xff

int incomingdest[MAXPLAYERS];
char incomingmsg[MAXPLAYERS][256];
int cmdsrc = 0;           // the source of a network console command

command_t *c_netcmds[NUMNETCMDS];

char *default_name = NULL;
int default_colour;

// basic chat char stuff: taken from hu_stuff.c and renamed

#define QUEUESIZE   1024

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;

//
// C_queueChatChar() (used to be HU_*)
//
// Add an incoming character to the circular chat queue
//
// Passed the character to queue, returns nothing
//

void C_queueChatChar(unsigned char c)
{
  if (((head + 1) & (QUEUESIZE-1)) == tail)
    C_Printf("command unsent\n");
  else
    {
      chatchars[head++] = c;
      head &= QUEUESIZE-1;
    }
}

//
// C_dequeueChatChar() (used to be HU_*)
//
// Remove the earliest added character from the circular chat queue
//
// Passed nothing, returns the character dequeued
//

unsigned char C_dequeueChatChar(void)
{
  char c;

  if (head == tail)
    return 0;
  else
    {
      c = chatchars[tail];
      tail = (tail + 1) & (QUEUESIZE-1);
      return c;
    }
}

void C_SendCmd(int cmdnum, const char *s,...)
{
  va_list args;
  char tempstr[500];
  va_start(args, s);
  
  vsprintf(tempstr,s, args);
  s = tempstr;
  
  if(!netgame || demoplayback)
    {
      cmdsrc = consoleplayer;
      cmdtype = c_netcmd;
      C_RunCommand(c_netcmds[cmdnum], s);
      return;
    }

  C_queueChatChar(cmdnum);        // command num
  
  while(*s)
    {
      C_queueChatChar(*s);
      s++;
    }
  C_queueChatChar(CMD_END);     // end
}

void C_NetInit()
{
  int i;
  
  for(i=0; i<MAXPLAYERS; i++)
    {
      incomingdest[i] = -1;
      incomingmsg[i][0] = '\0';
    }
  
  players[consoleplayer].colormap = default_colour;
  strcpy(players[consoleplayer].name, default_name);
}

void C_DealWithChar(unsigned char c, int source);

void C_NetTicker()
{
  int i, n;
  
  if(netgame && !demoplayback)      // only deal with chat chars in
    // netgames
    
    // check for incoming chat chars
    for(i=0; i<MAXPLAYERS; i++)
      {
	if(!playeringame[i]) continue;
	for(n=0; n<CONS_BYTES; n++)
	  C_DealWithChar(players[i].cmd.consdata[n], i);
      }
  
  // run buffered commands essential for netgame sync
  C_RunBuffer(c_netcmd);
}

void C_DealWithChar(byte c, int source)
{
  if(!c)
    return;

  //  C_Printf("got '%c' (%i)", isprint(c) ? c : 'p', c);
  
  if(c == CMD_END)        // end of command
    {
      int netcmdnum = incomingmsg[source][0];
      
      //	  C_Printf("%i, %s, %s\n", dest,
      //	       c_netcmds[netcmdnum]->name,
      //	       incomingmsg + 2);
      
      if(netcmdnum >= NUMNETCMDS || netcmdnum <= 0)
	C_Printf("unknown netcmd: %i\n", netcmdnum);
      else
	{
	  command_t *cmd = c_netcmds[netcmdnum];

	  lprintf(LO_DEBUG, "change %s\n", cmd->name);
	  
	  if(cmd->flags & cf_server && source)
	    C_Printf("server variable changed by peon\n");
	  else
	    {
	      cmdsrc = source;
	      cmdtype = c_netcmd;
	      C_RunCommand(cmd, incomingmsg[source] + 1);
	    }
	}
      
      incomingmsg[source][0] = '\0';
    }
  else
    {
      incomingmsg[source][strlen(incomingmsg[source]) + 1] = '\0';
      incomingmsg[source][strlen(incomingmsg[source])] = c;
    }
}

char *G_GetNameForMap(int episode, int map);

void C_SendNetData()
{
  command_t *command;
  int i;
  
  C_SetConsole();
  C_NetInit();
  
  // display message according to what we're about to do

  C_Printf(consoleplayer ?
	   FC_GRAY"Please Wait"FC_RED" Receiving game data..\n" :
	   FC_GRAY"Please Wait"FC_RED" Sending game data..\n");
  
  // go thru all hash chains, check for net sync variables
  
  for(i=0; i<CMDCHAINS; i++)
    {
      command = cmdroots[i];
      
      while(command)
        {
	  if(command->type == ct_variable && command->flags & cf_netvar
	     && ( consoleplayer==0 || !(command->flags & cf_server)))
            {
	      C_UpdateVar(command);
            }
	  command = command->next;
        }
    }

  demo_insurance = 1;      // always use 1 in multiplayer
  
  if(consoleplayer == 0)      // if server, send command to warp to map
    {
      //C_RunTextCmdf("map %s", startlevel);
    }

  //  G_InitNew(gameskill, "map01");
}


int allowmlook = 1;

//
//      Update a network variable
//

void C_UpdateVar(command_t *command)
{
  const char *value = C_VariableValue(command->variable);

  // enclose in "" quotes if it contains a space
  
  if(strchr(value, ' '))
    C_SendCmd(command->netcmd, "\"%s\"", value);
  else
    C_SendCmd(command->netcmd, value);
}
