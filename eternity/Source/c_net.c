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
//
// Console Network support 
//
// Network commands can be sent across netgames using 'C_SendCmd'. The
// command is transferred byte by byte, 1 per tic cmd, using the 
// chatchar variable (previously used for chat messages)
//
// By Simon Howard
//
// NETCODE_FIXME RED LEVEL ALERT!!! -- CONSOLE_FIXME
// The biggest problems are in here by far. While all the netcode needs
// an overhaul, the incredibly terrible way netcmds/net vars are sent
// and the fact that they cannot be recorded in demos is the most
// pressing problem and requires a complete rewrite of more or less
// everything in here. zdoom has an elegant solution to this problem,
// but it requires much more complicated network packet parsing.
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "d_main.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"

int incomingdest[MAXPLAYERS];
char incomingmsg[MAXPLAYERS][256];
int cmdsrc = 0;           // the source of a network console command

command_t *c_netcmds[NUMNETCMDS];

/*
  haleyjd: fixed a bug here

  default_name was being set equal to a string on the C heap,
  and then free()'d in the player name console command handler --
  but of course free() is redefined to Z_Free(), and you can't do
  that -- similar to the bug that was crashing the savegame menu,
  but this one caused a segfault, and only occasionally, because
  of Z_Free's faulty assumption that a pointer will be in valid 
  address space to test the zone id, even if its not a ptr to a
  zone block.
*/
char *default_name; // = "player";
int default_colour;

// basic chat char stuff: taken from hu_stuff.c and renamed

#define QUEUESIZE   1024

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;

// haleyjd: this must be called from start-up -- see above.
void C_InitPlayerName(void)
{
   default_name = Z_Strdup("player", PU_STATIC, 0);
}


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

  if (head != tail)
    {
      c = chatchars[tail++];
      tail &= QUEUESIZE-1;
    }
  else
    c = 0;
  return c;
}

void C_SendCmd(int dest, int cmdnum, char *s,...)
{
   va_list args;
   char tempstr[500];

   va_start(args, s);  
   pvsnprintf(tempstr, sizeof(tempstr), s, args);
   va_end(args);

   s = tempstr;
  
   if(!netgame || demoplayback)
   {
      cmdsrc = consoleplayer;
      cmdtype = c_netcmd;
      C_RunCommand(c_netcmds[cmdnum], s);
      return;
   }

   C_queueChatChar(0); // flush out previous commands
   C_queueChatChar((unsigned char)(dest+1)); // the chat message destination
   C_queueChatChar((unsigned char)cmdnum);        // command num
   
   while(*s)
   {
      C_queueChatChar(*s);
      s++;
   }
   C_queueChatChar(0);
}

void C_NetInit()
{
  int i;
  
  for(i=0; i<MAXPLAYERS; i++)
    {
      incomingdest[i] = -1;
      *incomingmsg[i] = 0;
    }
  
  players[consoleplayer].colormap = default_colour;
  strcpy(players[consoleplayer].name, default_name);
}

void C_DealWithChar(unsigned char c, int source);

void C_NetTicker()
{
  int i;
  
  if(netgame && !demoplayback)      // only deal with chat chars in
    // netgames
    
    // check for incoming chat chars
    for(i=0; i<MAXPLAYERS; i++)
      {
	if(!playeringame[i]) continue;
#ifdef CONSHUGE
	if(gamestate == GS_CONSOLE)  // use the whole ticcmd in console mode
          {
	    int a;
	    for(a=0; a<sizeof(ticcmd_t); a++)
	      C_DealWithChar( ((unsigned char*)&players[i].cmd)[a], i);
          }
	else
#endif
	  C_DealWithChar(players[i].cmd.chatchar,i);
      }
  
  // run buffered commands essential for netgame sync
  C_RunBuffer(c_netcmd);
}

void C_DealWithChar(unsigned char c, int source)
{
   int netcmdnum;
   
   if(c)
   {
      if(incomingdest[source] == -1)  // first char: the destination
      {
         incomingdest[source] = c-1;
      }
      else                  // append to string
      {
         sprintf(incomingmsg[source], "%s%c", incomingmsg[source], c);
      }
   }
   else
   {
      if(incomingdest[source] != -1)        // end of message
      {
         if((incomingdest[source] == consoleplayer)
            || incomingdest[source] == CN_BROADCAST)
         {
            cmdsrc = source;
            cmdtype = c_netcmd;
            // the first byte is the command num
            netcmdnum = incomingmsg[source][0];
            
            if(netcmdnum >= NUMNETCMDS || netcmdnum <= 0)
               C_Printf(FC_ERROR"unknown netcmd: %i\n", netcmdnum);
            else
            {
               // C_Printf("%s, %s", c_netcmds[netcmdnum].name,
               //          incomingmsg[source]+1);
               C_RunCommand(c_netcmds[netcmdnum],
                  incomingmsg[source] + 1);
            }
         }
         *incomingmsg[source] = 0;
         incomingdest[source] = -1;
      }
   }
}

char *G_GetNameForMap(int episode, int map);

void C_SendNetData()
{
  char tempstr[100];
  command_t *command;
  int i;

  C_SetConsole();
  
  // display message according to what we're about to do

  C_Printf(consoleplayer ?
           FC_HI"Please Wait"FC_NORMAL" Receiving game data..\n" :
           FC_HI"Please Wait"FC_NORMAL" Sending game data..\n");


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
      sprintf(tempstr, "map %s", startlevel);
      C_RunTextCmd(tempstr);
    }
}

//
//      Update a network variable
//

void C_UpdateVar(command_t *command)
{
  char tempstr[100];
  
  sprintf(tempstr,"\"%s\"", C_VariableValue(command->variable) );
  
  C_SendCmd(CN_BROADCAST, command->netcmd, tempstr);
}

// EOF
