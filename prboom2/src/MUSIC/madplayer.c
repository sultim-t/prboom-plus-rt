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

#ifndef HAVE_LIBMAD
#include <string.h>

static const char *mp_name (void)
{
  return "mad mp3 player (DISABLED)";
}


static int mp_init (int samplerate)
{
  return 0;
}

mp_player_t mp_player =
{
  mp_name,
  mp_init,
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

#else // HAVE_LIBMAD


#include <stdlib.h>
#include <string.h>
#include "lprintf.h"

#include <mad.h>

#include "i_sound.h"
#include "madplayer.h"


static const char *mp_name (music_player_t *music)
{
  return "mad mp3 player";
}


static int mp_init (music_player_t *music, int samplerate)
{
	mp_player_t *mp = (mp_player_t*)music;
  mad_stream_init (&mp->Stream);
  mad_frame_init (&mp->Frame);
  mad_synth_init (&mp->Synth);
  mad_header_init (&mp->Header);
  mp->mp_samplerate_target = samplerate;
  return 1;
}

static void mp_shutdown (music_player_t *music)
{
       mp_player_t *mp = (mp_player_t*)music;            
  mad_synth_finish (&mp->Synth);
  mad_frame_finish (&mp->Frame);
  mad_stream_finish (&mp->Stream);
  mad_header_finish (&mp->Header);
}

static const void *mp_registersong (music_player_t *music, const void *data, unsigned len)
{
	mp_player_t *mp = (mp_player_t*)music;
  int i;
  int maxtry;
  int success = 0;

  // the MP3 standard doesn't include any global file header.  the only way to tell filetype
  // is to start decoding stuff.  you can't be too strict however because MP3 is resilient to
  // crap in the stream.

  // this routine is a bit slower than it could be, but apparently there are lots of files out
  // there with some dodgy stuff at the beginning.
    
  // if the stream begins with an ID3v2 magic, search hard and long for our first valid header
  if (memcmp (data, "ID3", 3) == 0)
    maxtry = 100;
  // otherwise, search for not so long
  else
    maxtry = 20;

  mad_stream_buffer (&mp->Stream, (const unsigned char*)data, len);

  for (i = 0; i < maxtry; i++)
  {
    if (mad_header_decode (&mp->Header, &mp->Stream) != 0)
    {
      if (!MAD_RECOVERABLE (mp->Stream.error))
      {
        lprintf (LO_WARN, "mad_registersong failed: %s\n", mad_stream_errorstr (&mp->Stream));
        return NULL;
      }  
    }
    else
    {
      success++;
    }
  }

  // 80% to pass
  if (success < maxtry * 8 / 10)
  {
    lprintf (LO_WARN, "mad_registersong failed\n");
    return NULL;
  }
  
  lprintf (LO_INFO, "mad_registersong succeed. bitrate %lu samplerate %d\n", mp->Header.bitrate, mp->Header.samplerate);
 
  mp->mp_data = data;
  mp->mp_len = len;
  // handle not used
  return data;
}

static void mp_setvolume (music_player_t *music, int v)
{
	mp_player_t *mp = (mp_player_t*)music;
  mp->mp_volume = v;
}

static void mp_pause (music_player_t *music)
{
	mp_player_t *mp = (mp_player_t*)music;
  mp->mp_paused = 1;
}

static void mp_resume (music_player_t *music)
{
	mp_player_t *mp = (mp_player_t*)music;
  mp->mp_paused = 0;
}

static void mp_unregistersong (music_player_t *music, const void *handle)
{ // nothing to do
	mp_player_t *mp = (mp_player_t*)music;
  mp->mp_data = NULL;
  mp->mp_playing = 0;
}

static void mp_play (music_player_t *music, const void *handle, int looping)
{
	mp_player_t *mp = (mp_player_t*)music;
  mad_stream_buffer (&mp->Stream, (const unsigned char*)mp->mp_data, mp->mp_len);

  mp->mp_playing = 1;
  mp->mp_looping = looping;
  mp->mp_leftoversamps = 0;
  mp->mp_leftoversamppos = 0;
}

static void mp_stop (music_player_t *music)
{
	mp_player_t *mp = (mp_player_t*)music;
  mp->mp_playing = 0;
}

// convert from mad's internal fixed point representation
static inline short mp_fixtoshort (music_player_t *music, mad_fixed_t f)
{
	mp_player_t *mp = (mp_player_t*)music;
  // clip
  if (f < -MAD_F_ONE)
    f = -MAD_F_ONE;
  if (f > MAD_F_ONE)
    f = MAD_F_ONE;    
  // apply volume before conversion to 16bit
  f /= 15;
  f *= mp->mp_volume;
  f >>= (MAD_F_FRACBITS - 15);

  return (short) f;
}

static void mp_render_ex (music_player_t *music, void *dest, unsigned nsamp)
{
	mp_player_t *mp = (mp_player_t*)music;
  short *sout = (short *) dest;

  int localerrors = 0;

  if (!mp->mp_playing || mp->mp_paused)
  {
    memset (dest, 0, nsamp * 4);
    return;
  }

  while (1)
  {
    // write any leftover data from last MP3 frame
    while (mp->mp_leftoversamps > 0 && nsamp > 0)
    {
      short s = mp_fixtoshort (&mp->music, mp->Synth.pcm.samples[0][mp->mp_leftoversamppos]);
      *sout++ = s;
      if (mp->Synth.pcm.channels == 2)
        s = mp_fixtoshort (&mp->music, mp->Synth.pcm.samples[1][mp->mp_leftoversamppos]);
      // if mono, just duplicate the first channel again
      *sout++ = s;

      mp->mp_leftoversamps -= 1;
      mp->mp_leftoversamppos += 1;
      nsamp -= 1;
    }
    if (nsamp == 0)
      return; // done
    
    // decode next valid MP3 frame
    while (mad_frame_decode (&mp->Frame, &mp->Stream) != 0)
    {
      if (MAD_RECOVERABLE (mp->Stream.error))
      { // unspecified problem with one frame.
        // try the next frame, but bail if we get a bunch of crap in a row;
        // likely indicates a larger problem (and if we don't bail, we could
        // spend arbitrarily long amounts of time looking for the next good
        // packet)
        localerrors++;
        if (localerrors == 10)
        {
          lprintf (LO_WARN, "mad_frame_decode: Lots of errors.  Most recent %s\n", mad_stream_errorstr (&mp->Stream));
          mp->mp_playing = 0;
          memset (sout, 0, nsamp * 4);
          return;
        }
      }  
      else if (mp->Stream.error == MAD_ERROR_BUFLEN)
      { // EOF
        // FIXME: in order to not drop the last frame, there must be at least MAD_BUFFER_GUARD
        // of extra bytes (with value 0) at the end of the file.  current implementation
        // drops last frame
        if (mp->mp_looping)
        { // rewind, then go again
          mad_stream_buffer (&mp->Stream, mp->mp_data, mp->mp_len);
          continue;
        }
        else
        { // stop
          mp->mp_playing = 0;
          memset (sout, 0, nsamp * 4);
          return;
        }
      }
      else
      { // oh well.
        lprintf (LO_WARN, "mad_frame_decode: Unrecoverable error %s\n", mad_stream_errorstr (&mp->Stream));
        mp->mp_playing = 0;
        memset (sout, 0, nsamp * 4);
        return;
      }
    }

    // got a good frame, so synth it and dispatch it.
    mad_synth_frame (&mp->Synth, &mp->Frame);
    mp->mp_leftoversamps = mp->Synth.pcm.length;
    mp->mp_leftoversamppos = 0;

  }
  // NOT REACHED
}

static void mp_render (music_player_t *music, void *dest, unsigned nsamp)
{ 
	mp_player_t *mp = (mp_player_t*)music;
  I_ResampleStream (music, dest, nsamp, mp_render_ex, mp->Header.samplerate, mp->mp_samplerate_target);
}


mp_player_t mp_player =
{
  mp_name,
  mp_init,
  mp_shutdown,
  mp_setvolume,
  mp_pause,
  mp_resume,
  mp_registersong,
  mp_unregistersong,
  mp_play,
  mp_stop,
  mp_render
};

#endif // HAVE_LIBMAD
