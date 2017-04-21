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
 *  Copyright 2017 by
 *  Christoph Oelckers
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
 *      Game completion, final screen animation.
 *      This is an alternative version for UMAPINFO defined 
 *      intermission so that the feature can be cleanly implemented
 *      without worrying about demo sync implications
 *
 *-----------------------------------------------------------------------------
 */

#include "doomstat.h"
#include "d_event.h"
#include "g_game.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"
#include "sounds.h"
#include "d_deh.h"  // Ty 03/22/98 - externalizations
#include "f_finale.h" // CPhipps - hmm...

void F_StartCast (void);
void F_TextWrite(void);

void WI_checkForAccelerate(void);    // killough 3/28/98: used to
float Get_TextSpeed(void);
extern int acceleratestage;          // accelerate intermission screens
extern int midstage;
int using_FMI;

//
// F_StartFinale
//
extern dboolean secretexit;

void FMI_StartFinale(void)
{
	if (gamemapinfo->intertextsecret && secretexit && gamemapinfo->intertextsecret[0] != '-') // '-' means that any default intermission was cleared.
	{
		finaletext = gamemapinfo->intertextsecret;
	}
	else if (gamemapinfo->intertext && !secretexit && gamemapinfo->intertext[0] != '-') // '-' means that any default intermission was cleared.
	{
		finaletext = gamemapinfo->intertext;
	}

	if (!finaletext) finaletext = "The End";	// this is to avoid a crash on a missing text in the last map.

	finaleflat = gamemapinfo->interbackdrop[0] ? gamemapinfo->interbackdrop : "FLOOR4_8";	// use a single fallback for all maps.
	if (gamemapinfo->intermusic[0])
	{
		int l = W_CheckNumForName(gamemapinfo->intermusic);
		if (l >= 0) S_ChangeMusInfoMusic(l, true);
	}
	else
	{
		S_ChangeMusic(gamemode == commercial ? mus_read_m : mus_victor, true);
	}

	finalestage = 0;
	finalecount = 0;
	using_FMI = true;
}



//
// F_Ticker
//
// killough 3/28/98: almost totally rewritten, to use
// player-directed acceleration instead of constant delays.
// Now the player can accelerate the text display by using
// the fire/use keys while it is being printed. The delay
// automatically responds to the user, and gives enough
// time to read.
//
// killough 5/10/98: add back v1.9 demo compatibility
//


void FMI_Ticker(void)
{
	int i;
	if (!demo_compatibility) WI_checkForAccelerate();  // killough 3/28/98: check for acceleration
	else for (i = 0; i < MAXPLAYERS; i++)	if (players[i].cmd.buttons) goto next_level;      // go on to the next level

  // advance animation
	finalecount++;

	if (!finalestage)
	{
		float speed = demo_compatibility ? TEXTSPEED : Get_TextSpeed();
		/* killough 2/28/98: changed to allow acceleration */
		if (finalecount > strlen(finaletext)*speed +
			(midstage ? NEWTEXTWAIT : TEXTWAIT) ||
			(midstage && acceleratestage)) {

		next_level:
			if (gamemapinfo->endpic[0])
			{
				if (!stricmp(gamemapinfo->endpic, "$CAST"))
				{
					F_StartCast();
					using_FMI = false;
				}
				else
				{
					finalecount = 0;
					finalestage = 1;
					wipegamestate = -1;         // force a wipe
					if (!stricmp(gamemapinfo->endpic, "$BUNNY"))
					{
						S_StartMusic(mus_bunny);
						using_FMI = false;
					}
				}
			}
			else
			{
				gameaction = ga_worlddone;  // next level, e.g. MAP07
			}
		}
	}
}


//
// F_Drawer
//
void FMI_Drawer(void)
{
	if (!finalestage || !gamemapinfo->endpic[0])
	{
		F_TextWrite();
	}
	else
	{
		V_DrawNamePatch(0, 0, 0, gamemapinfo->endpic, CR_DEFAULT, VPT_STRETCH);
		// e6y: wide-res
		V_FillBorder(-1, 0);
	}
}
