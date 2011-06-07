/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2002 by
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SDL.h"

#include "i_video.h"
#include "i_sound.h"
#include "v_video.h"
#include "r_draw.h"
#include "lprintf.h"

#include "i_smp.h"

static SDL_Thread *smp_thread;
static SDL_mutex *smp_mutex;
static SDL_cond *renderCompletedEvent;
static SDL_cond *renderCommandsEvent;
static volatile int smp_ready;
static volatile int smp_state;

typedef enum
{
  SMP_DATA_SEGS,
  SMP_DATA_SPANS,

  SMP_DATA_MAX
} smp_datatype_e;

static smp_item_t smp_data[SMP_DATA_MAX];

int use_smp_defauls;
int use_smp;

static void SMP_ResetBuffers(void)
{
  int i;

  for (i = 0; i < SMP_DATA_MAX; i++)
  {
    smp_data[i].count = 0;
    smp_data[i].index = 0;
  }
}

static void SMP_SetState(int state)
{
  //SDL_LockMutex(segs_mutex);
  smp_state = state;
  //SDL_UnlockMutex(segs_mutex);
}

static int SMP_GetState(void)
{
  return smp_state;
}

static inline void SMP_CheckSize(smp_item_t *item, int size)
{
  if (item->count >= item->size)
  {
    while (item->index < item->count)
    {
      SDL_Delay(1);
    }

    item->size = (item->size == 0 ? 1024 : item->size * 2);
    item->data.item = realloc(item->data.item, item->size * size);
  }
}

void SMP_ColFunc(draw_column_vars_t *data)
{
  if (!use_smp)
  {
    data->colfunc(data);
  }
  else
  {
    smp_item_t *item = &smp_data[SMP_DATA_SEGS];

    SMP_CheckSize(item, sizeof(item->data.segs[0]));
    item->data.segs[item->count] = *data;
    item->count++;
  }
}

void SMP_SpanFunc(draw_span_vars_t *data)
{
  if (!use_smp)
  {
    R_DrawSpan(data);
  }
  else
  {
    smp_item_t *item = &smp_data[SMP_DATA_SPANS];

    SMP_CheckSize(item, sizeof(item->data.spans[0]));
    item->data.spans[item->count] = *data;
    item->count++;
  }
}

void SMP_RendererSleep(void)
{
  if (!use_smp)
    return;

  SDL_LockMutex(smp_mutex);
  {
    smp_ready = false;

    // after this, the front end can exit SMP_FrontEndSleep
    SDL_CondSignal(renderCompletedEvent);

    while (!smp_ready)
    {
      SDL_CondWait(renderCommandsEvent, smp_mutex);
    }
  }
  SDL_UnlockMutex(smp_mutex);
}

void SMP_FrontEndSleep(void)
{
  if (!use_smp)
    return;

  SMP_SetState(1);

  SDL_LockMutex(smp_mutex);
  {
    while (smp_ready)
    {
      SDL_CondWait(renderCompletedEvent, smp_mutex);
    }
  }
  SDL_UnlockMutex(smp_mutex);
}

void SMP_WakeRenderer(void)
{
  if (!use_smp)
    return;

  SMP_ResetBuffers();

  SDL_LockMutex(smp_mutex);
  {
    smp_ready = true;

    // after this, the renderer can continue through SMP_RendererSleep
    SDL_CondSignal(renderCommandsEvent);
  }
  SDL_UnlockMutex(smp_mutex);
}

static inline void smp_draw(void)
{
  smp_item_t *segs = &smp_data[SMP_DATA_SEGS];
  smp_item_t *spans = &smp_data[SMP_DATA_SPANS];

  while (segs->index < segs->count)
  {
    segs->data.segs[segs->index].colfunc(&segs->data.segs[segs->index]);
    segs->index++;
  }

  while (spans->index < spans->count)
  {
    R_DrawSpan(&spans->data.spans[spans->index]);
    spans->index++;
  }
}

int render_thread_func(void *unused)
{
  while (1)
  {
    // sleep until we have work to do
    SMP_RendererSleep();

    do
    {
      smp_draw();
    }
    while (SMP_GetState() != 1);

    smp_draw();

    SMP_SetState(0);
  }

  return 0;
}

void SMP_Init(void)
{
  static int first = 0;

  use_smp = 0;

  if (process_affinity_mask)
  {
    lprintf(LO_WARN,
      "SMP_Init: Unable to init SMP if 'process_affinity_mask' is not a zero\n");
    return;
  }

  if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_sdl]))
  {
    lprintf(LO_WARN,
      "SMP_Init: Unable to init SMP if 'Preffered MIDI Player' is '%s'\n",
      midiplayers[midi_player_sdl]);
    return;
  }

  if (V_GetMode() != VID_MODEGL)
  {
    if (use_smp_defauls && !smp_thread)
    {
      int i;

      for (i = 0; i < SMP_DATA_MAX; i++)
      {
        memset(&smp_data[i], 0, sizeof(smp_data[i]));
      }

      smp_mutex = SDL_CreateMutex();
      if (smp_mutex)
      {
        renderCommandsEvent = SDL_CreateCond();
        renderCompletedEvent = SDL_CreateCond();
        if (renderCommandsEvent && renderCompletedEvent)
        {
          smp_thread = SDL_CreateThread(render_thread_func, NULL);
          if (smp_thread)
          {
            use_smp = true;
          }
        }
      }

      if (use_smp)
      {
        if (!first)
        {
          first = 1;
          atexit(SMP_Free);
        }
      }
      else
      {
        lprintf(LO_WARN, "SMP_Init: Unable to init SMP: %s\n", SDL_GetError());
        SMP_Free();
      }
    }
  }
}

void SMP_Free(void)
{
  if (smp_thread)
  {
    SDL_KillThread(smp_thread);
    smp_thread = NULL;
  }

  if (renderCompletedEvent)
  {
    SDL_DestroyCond(renderCompletedEvent);
    renderCompletedEvent = NULL;
  }

  if (renderCommandsEvent)
  {
    SDL_DestroyCond(renderCommandsEvent);
    renderCommandsEvent = NULL;
  }

  if (smp_mutex)
  {
    SDL_DestroyMutex(smp_mutex);
    smp_mutex = NULL;
  }
}
