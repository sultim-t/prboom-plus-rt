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
 *      System interface, sound.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __G_OVERFLOW__
#define __G_OVERFLOW__

#include "doomtype.h"
#include "doomdata.h"
#include "p_maputl.h"

typedef struct overrun_param_s
{
  int warn;
  int emulate;
  int footer;
  int footer_emulate;
  int promted;
  int shit_happens;
} overrun_param_t;

typedef enum overrun_list_s
{
  OVERFLOW_SPECHIT,
  OVERFLOW_REJECT,
  OVERFLOW_INTERCEPT,
  OVERFLOW_PLYERINGAME,
  OVERFLOW_DONUT,
  OVERFLOW_MISSEDBACKSIDE,

  OVERFLOW_MAX //last
} overrun_list_t;

extern int overflows_enabled;
extern overrun_param_t overflows[];
extern const char *overflow_cfgname[OVERFLOW_MAX];

#define EMULATE(overflow) (overflows_enabled && (overflows[overflow].footer ? overflows[overflow].footer_emulate : overflows[overflow].emulate))
#define PROCESS(overflow) (overflows_enabled && (overflows[overflow].warn || EMULATE(overflow)))

// e6y
//
// intercepts overrun emulation
// See more information on:
// doomworld.com/vb/doom-speed-demos/35214-spechits-reject-and-intercepts-overflow-lists
//
// Thanks to Simon Howard (fraggle) for refactor the intercepts
// overrun code so that it should work properly on big endian machines
// as well as little endian machines.

#define MAXINTERCEPTS_ORIGINAL 128

typedef struct
{
    int len;
    void *addr;
    dboolean int16_array;
} intercepts_overrun_t;

extern intercepts_overrun_t intercepts_overrun[];
void InterceptsOverrun(int num_intercepts, intercept_t *intercept);

//
// playeringame overrun emulation
//

int PlayeringameOverrun(const mapthing_t* mthing);

//
// spechit overrun emulation
//

// Spechit overrun magic value.
#define DEFAULT_SPECHIT_MAGIC 0x01C09C98

typedef struct spechit_overrun_param_s
{
  line_t *line;

  line_t ***spechit;
  int *numspechit;

  fixed_t *tmbbox;
  fixed_t *tmfloorz;
  fixed_t *tmceilingz;

  dboolean *crushchange;
  dboolean *nofit;
} spechit_overrun_param_t;

extern unsigned int spechit_baseaddr;

void SpechitOverrun(spechit_overrun_param_t *params);

//
// reject overrun emulation
//

void RejectOverrun(int rejectlump, const byte **rejectmatrix, int totallines);

//
// donut overrun emulation (linedef action 9)
//

int DonutOverrun(fixed_t *pfloorheight, short *pfloorpic);

int MissedBackSideOverrun(line_t *line);
sector_t* GetSectorAtNullAddress(void);

#endif // __G_OVERFLOW__
