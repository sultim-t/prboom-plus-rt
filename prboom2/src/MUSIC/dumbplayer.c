/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *
 *  Copyright (C) 2011 by
 *  Nicholai Main
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
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicplayer.h"

#ifndef HAVE_LIBDUMB
#include <string.h>

static const char *db_name (void)
{
  return "dumb tracker player (DISABLED)";
}


static int db_init (int samplerate)
{
  return 0;
}

const music_player_t db_player =
{
  db_name,
  db_init,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

#else // HAVE_DUMB

#include <dumb.h>
#include <string.h>
#include "lprintf.h"


static float db_delta;
static float db_volume;
static int db_looping;
static int db_playing = 0;
static int db_paused = 0;

static DUH_SIGRENDERER *dsren = NULL;
static DUH *duh = NULL;
static DUMBFILE *dfil = NULL;

static const char *db_name (void)
{
  return "dumb tracker player";
}


static int db_init (int samplerate)
{
  db_delta = 65536.0f / samplerate;

  return 1;
}

static void db_shutdown (void)
{
  dumb_exit ();
}

static void db_setvolume (int v)
{
  db_volume = (float) v / 15.0f;
}

static const void* db_registersong (const void *data, unsigned len)
{
  // because dumbfiles don't have any concept of backward seek or
  // rewind, you have to reopen if any loader fails

  if (1)
  {
    dfil = dumbfile_open_memory (data, len);
    duh = read_duh (dfil);
  }
  if (!duh)
  {
    dumbfile_close (dfil);
    dfil = dumbfile_open_memory (data, len);
    duh = dumb_read_it_quick (dfil);  
  }
  if (!duh)
  {
    dumbfile_close (dfil);
    dfil = dumbfile_open_memory (data, len);
    duh = dumb_read_xm_quick (dfil);  
  }
  if (!duh)
  {
    dumbfile_close (dfil);
    dfil = dumbfile_open_memory (data, len);
    duh = dumb_read_s3m_quick (dfil);  
  }
  if (!duh)
  {
    dumbfile_close (dfil);
    dfil = dumbfile_open_memory (data, len);
    duh = dumb_read_mod_quick (dfil);
    // No way to get the filename, so we can't check for a .mod extension, and
    // therefore, trying to load an old 15-instrument SoundTracker module is not
    // safe. We'll restrict MOD loading to 31-instrument modules with known
    // signatures and let the sound system worry about 15-instrument ones.
    // (Assuming it even supports them)
    {
      DUMB_IT_SIGDATA *sigdata = duh_get_it_sigdata(duh);
      if (sigdata)
      {
        int n_samples = dumb_it_sd_get_n_samples(sigdata);
        if (n_samples == 15)
        {
          unload_duh(duh);
          duh = NULL;
        }
      }
    }
  }
  if (!duh)
  {
    dumbfile_close (dfil);
    dfil = NULL;
    lprintf (LO_WARN, "db_registersong: couldn't load as tracker\n");
    return NULL;
  }
  // handle not used
  return data;
}

static void db_unregistersong (const void *handle)
{
  if (duh)
  {
    unload_duh (duh);
    duh = NULL;
  }

  if (dfil)
  {
    dumbfile_close (dfil);
    dfil = NULL;
  }
}

static void db_play (const void *handle, int looping)
{
  dsren = duh_start_sigrenderer (duh, 0, 2, 0);

  if (!dsren) // fail?
  {
    db_playing = 0;
    return;
  }

  db_looping = looping;
  db_playing = 1;
}

static void db_stop (void)
{
  duh_end_sigrenderer (dsren);
  dsren = NULL;
  db_playing = 0;
}
  
static void db_pause (void)
{
  db_paused = 1;
}

static void db_resume (void)
{
  db_paused = 0;
}

static void db_render (void *dest, unsigned nsamp)
{
  unsigned char *cdest = dest;
  unsigned nsampwrit = 0;

  if (db_playing && !db_paused)
  {
    nsampwrit = duh_render (dsren, 16, 0, db_volume, db_delta, nsamp, dest);
    if (nsampwrit != nsamp)
    { // end of file
      // tracker formats can have looping imbedded in them, in which case
      // we'll never reach this (even if db_looping is 0!!)
      
      cdest += nsampwrit * 4;


      if (db_looping)
      { // but if the tracker doesn't loop, and we want loop anyway, restart
        // from beginning
        
        if (nsampwrit == 0)
        { // special case: avoid infinite recursion
          db_stop ();
          lprintf (LO_WARN, "db_render: problem (0 length tracker file on loop?\n");
          return;
        }
        
        // im not sure if this is the best way to seek, but there isn't
        // a sigrenderer_rewind type function
        db_stop ();
        db_play (NULL, 1);
        db_render (cdest, nsamp - nsampwrit);
      }
      else
      { // halt
        db_stop ();
        memset (cdest, 0, (nsamp - nsampwrit) * 4);
      }
    }
  }
  else
    memset (dest, 0, nsamp * 4);
}

const music_player_t db_player =
{
  db_name,
  db_init,
  db_shutdown,
  db_setvolume,
  db_pause,
  db_resume,
  db_registersong,
  db_unregistersong,
  db_play,
  db_stop,
  db_render
};




#endif // HAVE_DUMB



