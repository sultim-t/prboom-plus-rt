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
 * DESCRIPTION:  Support MUSINFO lump (dynamic music changing)
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doomstat.h"
#include "doomtype.h"
#include "d_main.h"
#include "p_mobj.h"
#include "m_misc.h"
#include "sounds.h"
#include "s_sound.h"
#include "i_sound.h"
#include "r_defs.h"
#include "sc_man.h"
#include "w_wad.h"
#include "lprintf.h"

#include "s_advsound.h"

#define TIDNUM(x) ((int)(x->iden_nums & 0xFFFF))		// thing identifier

musinfo_t musinfo;

//
// S_ParseMusInfo
// Parses MUSINFO lump.
//
void S_ParseMusInfo(const char *mapid)
{
  memset(&musinfo, 0, sizeof(musinfo));
  musinfo.current_item = -1;

  S_music[NUMMUSIC].lumpnum = -1;

  if (W_CheckNumForName("MUSINFO") != -1)
  {
    int num, lumpnum;
    int inMap = false;

    SC_OpenLump("MUSINFO");

    while (SC_GetString())
    {
      if (inMap || SC_Compare(mapid))
      {
        if (!inMap)
        {
          SC_GetString();
          inMap = true;
        }

        if (sc_String[0] == 'E' || sc_String[0] == 'e' ||
            sc_String[0] == 'M' || sc_String[0] == 'm')
        {
          break;
        }

        // Check number in range
        if (M_StrToInt(sc_String, &num) && num > 0 && num < MAX_MUS_ENTRIES)
        {
          if (SC_GetString())
          {
            lumpnum = W_CheckNumForName(sc_String);

            if (lumpnum >= 0)
            {
              musinfo.items[num] = lumpnum;
            }
            else
            {
              lprintf(LO_ERROR, "S_ParseMusInfo: Unknown MUS lump %s", sc_String);
            }
          }
        }
        else
        {
          lprintf(LO_ERROR, "S_ParseMusInfo: Number not in range 1 to %d", MAX_MUS_ENTRIES);
        }
      }
    }

    SC_Close();
  }
}

void MusInfoThinker(mobj_t *thing)
{
  if (musinfo.mapthing != thing &&
      thing->subsector->sector == players[displayplayer].mo->subsector->sector)
  {
    musinfo.lastmapthing = musinfo.mapthing;
    musinfo.mapthing = thing;
    musinfo.tics = 30;
  }
}

void T_MAPMusic(void)
{
  if (musinfo.tics < 0 || !musinfo.mapthing)
  {
    return;
  }

  if (musinfo.tics > 0)
  {
    musinfo.tics--;
  }
  else
  {
    if (!musinfo.tics && musinfo.lastmapthing != musinfo.mapthing)
    {
      int arraypt = TIDNUM(musinfo.mapthing);

      if (arraypt >= 0 && arraypt < MAX_MUS_ENTRIES)
      {
        int lumpnum = musinfo.items[arraypt];

        if (lumpnum >= 0 && lumpnum < numlumps)
        {
          S_ChangeMusInfoMusic(lumpnum, true);
        }
      }

      musinfo.tics = -1;
    }
  }
}
