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
 *
 *-----------------------------------------------------------------------------*/

#ifndef __I_SMP__
#define __I_SMP__

#include "SDL.h"

#include "r_draw.h"

typedef struct smp_item_s
{
  volatile int size;
  volatile int index;
  volatile int count;

  union
  {
    void *item;
    draw_column_vars_t* segs;
    draw_span_vars_t* spans;
  } data;
} smp_item_t;

extern int use_smp;
extern int use_smp_defauls;

extern smp_item_t smp_segs;
extern smp_item_t smp_spans;

void SMP_Init(void);
void SMP_Free(void);

void SMP_WakeRenderer(void);
void SMP_RendererSleep(void);
void SMP_FrontEndSleep(void);

void SMP_ColFunc(draw_column_vars_t *dcvars);
void SMP_SpanFunc(draw_span_vars_t *dsvars);

#endif // __I_SMP__
