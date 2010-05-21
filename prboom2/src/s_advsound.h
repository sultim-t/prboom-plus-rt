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
 * DESCRIPTION:
 *      Support MUSINFO lump (dynamic music changing)
 *
 *-----------------------------------------------------------------------------*/

#ifndef __S_ADVSOUND__
#define __S_ADVSOUND__

#include "p_mobj.h"

#ifdef __GNUG__
#pragma interface
#endif

//
//MUSINFO lump
//

#define MAX_MUS_ENTRIES 64

typedef struct musinfo_s
{
  mobj_t *mapthing;
  mobj_t *lastmapthing;
  int tics;
  int current_item;
  int items[MAX_MUS_ENTRIES];
} musinfo_t;

extern musinfo_t musinfo;

void S_ParseMusInfo(const char *mapid);
void MusInfoThinker(mobj_t *thing);
void T_MAPMusic(void);

#endif
