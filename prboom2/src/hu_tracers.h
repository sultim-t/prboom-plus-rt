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
 * DESCRIPTION:  tracers stuff
 *
 *-----------------------------------------------------------------------------*/

#ifndef __HU_TRACERS__
#define __HU_TRACERS__

#include "hu_lib.h"

#define MAXTRACEITEMS 8

typedef enum
{
  TRACE_HEALTH,
  TRACE_PICKUP,
  TRACE_CROSS,
  TRACE_DAMAGE,

  NUMTRACES
} tracertype_t;

typedef struct
{
  int index;
  char value[16];
  int data1;
} traceitem_t;

typedef void (*TRACERFUNC)(tracertype_t index);
typedef struct traceslist_s
{
  traceitem_t items[MAXTRACEITEMS];
  int count;

  char hudstr[80];
  char cmd[32];
  char prefix[32];
  TRACERFUNC ApplyFunc;
  TRACERFUNC ResetFunc;
} traceslist_t;

typedef struct traceslistinit_s
{
  char cmd[32];
  char prefix[32];
  TRACERFUNC ApplyFunc;
  TRACERFUNC ResetFunc;
} traceslistinit_t;

extern traceslist_t traces[];
extern dboolean traces_present;

extern hu_textline_t w_traces[];

void InitTracers(void);

void CheckGivenDamageTracer(mobj_t *mobj, int damage);
void CheckThingsPickupTracer(mobj_t *mobj);
void CheckThingsHealthTracer(mobj_t *mobj);
void CheckLinesCrossTracer(line_t *line);
void ClearLinesCrossTracer(void);

void TracerClearStarts(void);
void TracerAddDeathmatchStart(int num, int index);
void TracerAddPlayerStart(int num, int index);
int TracerGetDeathmatchStart(int index);
int TracerGetPlayerStart(int index);

#endif // __HU_TRACERS__
