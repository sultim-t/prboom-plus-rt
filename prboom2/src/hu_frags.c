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
 * Heads up frag counter
 *
 * Counts the frags by each player and sorts them so that the best
 * player is at the top of the list
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>

#include "psnprntf.h"

#include "hu_frags.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "r_draw.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "v_video.h"
#include "z_zone.h"

#define FRAGSX 125
#define FRAGSY 10

#define NAMEX 165
#define NAMEY 65

#define FRAGNUMX 175

extern int action_frags;  // g_bind.c

static player_t *sortedplayers[MAXPLAYERS];

static int num_players;
int show_scores;                // enable scores

static patchnum_t fragspic;
static patchnum_t fragbox;

void HU_FragsInit()
{
  //R_SetPatchNum(&fragspic, "HU_FRAGS");
  //R_SetPatchNum(&fragbox, "HU_FRGBX");
}

void HU_FragsDrawer()
{
  int i, y;
  char tempstr[50];
  
  if(!deathmatch)
    return;
  
//  if(!action_frags) FIXME
    {
      if(players[displayplayer].playerstate != PST_DEAD ||
	 walkcam_active ||
	 !show_scores)
	return;
    }
  
  // "frags"
  V_DrawNumPatch(FRAGSX, FRAGSY, 0, fragspic.lumpnum, CR_DEFAULT, VPT_STRETCH);
  
  y = NAMEY;
  
  for(i=0; i<num_players; i++)
    {
      // write their name
      psnprintf(tempstr, 50, "%s%s", !demoplayback && 
	      sortedplayers[i]==players+consoleplayer ? FC_GRAY : FC_RED,
	      sortedplayers[i]->name);
      
      V_WriteText(tempstr, NAMEX - V_StringWidth(tempstr, 0), y, 0);
      
      // box behind frag pic
      
      V_DrawNumPatch(FRAGNUMX, y, 0, fragbox.lumpnum, CR_DEFAULT, VPT_STRETCH);

      // draw the frags
      psnprintf(tempstr, 50, "%i", sortedplayers[i]->totalfrags);
      V_WriteText(tempstr, FRAGNUMX + 16 - V_StringWidth(tempstr, 0)/2, y, 0);
      y += 10;
    }
}

void HU_FragsUpdate()
{
  int i,j;
  int change;
  player_t *temp;
  
  num_players = 0;
  
  for(i=0; i<MAXPLAYERS; i++)
    {
      if(!playeringame[i]) continue;
      
      // found a real player
      // add to list
      
      sortedplayers[num_players] = &players[i];
      num_players++;

      players[i].totalfrags = 0; // reset frag count

      for(j=0; j<MAXPLAYERS; j++)  // add all frags for this player
	{
	  if(!playeringame[j]) continue;
	  if(i==j) players[i].totalfrags-=players[i].frags[j];
	  else players[i].totalfrags+=players[i].frags[j];
	}
    }

  // use the bubble sort algorithm to sort the players
  
  change = true;
  while(change)
    {
      change = false;
      for(i=0; i<num_players-1; i++)
	{
	  if(sortedplayers[i]->totalfrags <
	     sortedplayers[i+1]->totalfrags)
	    {
	      temp = sortedplayers[i];
	      sortedplayers[i] = sortedplayers[i+1];
	      sortedplayers[i+1] = temp;
	      change = true;
	    }
	}
    }
}

void HU_FragsErase()
{
  int i;
  
  if(!deathmatch)
    return;
  
  for(i=FRAGSY; i<SCREENHEIGHT-ST_SCALED_HEIGHT; i++)
    R_VideoErase(i*SCREENWIDTH, SCREENWIDTH);
}

////////////////////////////////////
//
// Console Commands
//

CONSOLE_COMMAND(frags, 0)
{
  int i;
  
  for(i=0; i<num_players; i++)
    {
      C_Printf(FC_GRAY"%i"FC_RED" %s\n",
	       sortedplayers[i]->totalfrags,
	       sortedplayers[i]->name);
    }
}

VARIABLE_BOOLEAN(show_scores,       NULL,           onoff);
CONSOLE_VARIABLE(show_scores,   show_scores,    0)      {}

void HU_FragsAddCommands()
{
  C_AddCommand(frags);
  C_AddCommand(show_scores);
}
