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
#include "vorbisplayer.h"
#include "doomdef.h"

#ifndef HAVE_LIBVORBISFILE
#include <string.h>

static const char *vorb_name (void)
{
  return "vorbis player (DISABLED)";
}


static int vorb_init (int samplerate)
{
  return 0;
}

vorb_player_t vorb_player =
{
  vorb_name,
  vorb_init,
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

#else // HAVE_LIBVORBISFILE


#include <stdlib.h>
#include <string.h>
#include "lprintf.h"

#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#include "i_sound.h"

// io callbacks

static size_t vread (void *dst, size_t s, size_t n, void *src)
{
	vorb_player_t *vrb = (vorb_player_t*)src;
  size_t size = s * n;

  if (vrb->vorb_pos + size >= vrb->vorb_len)
    size = vrb->vorb_len - vrb->vorb_pos;

  memcpy (dst, vrb->vorb_data + vrb->vorb_pos, size);
  vrb->vorb_pos += size;
  return size;
}

static int vseek (void *src, ogg_int64_t offset, int whence)
{
	vorb_player_t *vrb = (vorb_player_t*)src;
  size_t desired_pos;

  switch (whence)
  {
    case SEEK_SET:
      desired_pos = (size_t) offset;
      break;
    case SEEK_CUR:
      desired_pos = vrb->vorb_pos + (size_t) offset;
      break;
    case SEEK_END:
    default:
      desired_pos = vrb->vorb_len + (size_t) offset;
      break;
  }
  if (desired_pos > vrb->vorb_len) // placing exactly at the end is allowed)
    return -1;
  vrb->vorb_pos = desired_pos;
  return 0;
}

static long vtell (void *src)
{
	vorb_player_t *vrb = (vorb_player_t*)src;
  // correct to vorbisfile spec, this is a long, not 64 bit 
  return (long) vrb->vorb_pos;
}


static const char *vorb_name (music_player_t *music)
{
  return "vorbis player";
}

// http://zdoom.org/wiki/Audio_loop
// it's hard to accurately implement a grammar with "etc" in the spec,
// so weird edge cases are likely not the same
#ifdef ZDOOM_AUDIO_LOOP
static unsigned parsetag (const char *str, int samplerate)
{
  int ret = 0;
  int seendot = 0; // number of dots seen so far
  int mult = 1; // place value of next digit out
  int seencolon = 0; // number of colons seen so far
  int digincol = 0; // number of digits in current group
                    // (needed to track switch between colon)

  const char *pos = str + strlen (str) - 1;

  for (; pos >= str; pos--)
  {
    if (*pos >= '0' && *pos <= '9')
    {
      ret += (*pos - '0') * mult;
      mult *= 10;
      digincol++;
    }
    else if (*pos == '.')
    {
      if (seencolon || seendot)
        return 0;
      seendot = 1;
      // convert decimal to samplerate and move on
      ret *= samplerate;
      ret /= mult;
      mult = samplerate;
      digincol = 0;
    }
    else if (*pos == ':')
    {
      if (seencolon == 2) // no mention of anything past hours in spec
        return 0;
      seencolon++;
      mult *= 6;

      // the spec is kind of vague and says lots of things can be left out,
      // so constructs like mmm:ss and hh::ss are allowed
      while (digincol > 1)
      {
        digincol--;
        mult /= 10;
      }
      while (digincol < 1)
      {
        digincol++;
        mult *= 10;
      }
      digincol = 0;
    }
    else
      return 0;
  }
  if (seencolon && !seendot)
  { // HH:MM:SS, but we never converted to samples
    return ret * samplerate;
  }
  // either flat pcm or :. in which case everything was converted already
  return ret;
}      
#endif // ZDOOM_AUDIO_LOOP

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>
#endif

static int vorb_init (music_player_t *music, int samplerate)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  TESTDLLLOAD ("libogg-0.dll", FALSE)
  TESTDLLLOAD ("libvorbis-0.dll", FALSE)
  TESTDLLLOAD ("libvorbisfile-3.dll", TRUE)
  vrb->vorb_samplerate_target = samplerate;
  return 1;
}

static void vorb_shutdown (music_player_t *music)
{
  // nothing to do                 


}

static const void *vorb_registersong (music_player_t *music, const void *data, unsigned len)
{
  int i;
  vorbis_info *vinfo;
  #ifdef ZDOOM_AUDIO_LOOP
  vorbis_comment *vcom;
  #endif // ZDOOM_AUDIO_LOOP
  vorb_player_t *vrb = (vorb_player_t*)music;

  vrb->vorb_data = data;
  vrb->vorb_len = len;
  vrb->vorb_pos = 0;

  i = ov_test_callbacks ((void *) music, &vrb->vf, NULL, 0, vrb->vcallback);

  if (i != 0)
  {
    lprintf (LO_WARN, "vorb_registersong: failed\n");
    return NULL;
  }
  i = ov_test_open (&vrb->vf);
  
  if (i != 0)
  {
    lprintf (LO_WARN, "vorb_registersong: failed\n");
    ov_clear (&vrb->vf);
    return NULL;
  }
  
  vinfo = ov_info (&vrb->vf, -1);
  vrb->vorb_samplerate_in = vinfo->rate;

  #ifdef ZDOOM_AUDIO_LOOP
  // parse LOOP_START LOOP_END tags
  vrb->vorb_loop_from = 0;
  vrb->vorb_loop_to = 0;

  vcom = ov_comment (&vrb->vf, -1);
  for (i = 0; i < vcom->comments; i++)
  {
    if (strncmp ("LOOP_START=", vcom->user_comments[i], 11) == 0)
      vrb->vorb_loop_to = parsetag (vcom->user_comments[i] + 11, vrb->vorb_samplerate_in);
    else if (strncmp ("LOOP_END=", vcom->user_comments[i], 9) == 0)
      vrb->vorb_loop_from = parsetag (vcom->user_comments[i] + 9, vrb->vorb_samplerate_in);
  }
  if (vrb->vorb_loop_from == 0)
    vrb->vorb_loop_from = 0xffffffff;
  else if (vrb->vorb_loop_to >= vrb->vorb_loop_from)
    vrb->vorb_loop_to = 0;
  #endif // ZDOOM_AUDIO_LOOP

  // handle not used
  return data;
}

static void vorb_setvolume (music_player_t *music, int v)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  vrb->vorb_volume = v;
}

static void vorb_pause (music_player_t *music)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  vrb->vorb_paused = 1;
}

static void vorb_resume (music_player_t *music)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  vrb->vorb_paused = 0;
}

static void vorb_unregistersong (music_player_t *music, const void *handle)
{ 
	vorb_player_t *vrb = (vorb_player_t*)music;
  vrb->vorb_data = NULL;
  ov_clear (&vrb->vf);
  vrb->vorb_playing = 0;
}

static void vorb_play (music_player_t *music, const void *handle, int looping)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  ov_raw_seek_lap (&vrb->vf, 0);
 

  vrb->vorb_playing = 1;
  vrb->vorb_looping = looping;
  #ifdef ZDOOM_AUDIO_LOOP
  vrb->vorb_total_pos = 0;
  #endif // ZDOOM_AUDIO_LOOP
}

static void vorb_stop (music_player_t *music)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  vrb->vorb_playing = 0;
}

static void vorb_render_ex (music_player_t *music, void *dest, unsigned nsamp)
{
	vorb_player_t *vrb = (vorb_player_t*)music;
  // no workie on files that dynamically change sampling rate

  short *sout = (short *) dest;

  int bitstreamnum; // not used

  float **pcmdata;

  float multiplier;

  int localerrors = 0;

  //int nchannel;
  //int freq;
  int numread;

  int i;
 
  // this call needs to be moved if support for changed number
  // of channels in middle of file is wanted
  vorbis_info *vinfo = ov_info (&vrb->vf, -1);


  if (!vrb->vorb_playing || vrb->vorb_paused)
  {
    memset (dest, 0, nsamp * 4);
    return;
  }

  while (nsamp > 0)
  {
    #ifdef ZDOOM_AUDIO_LOOP
    // don't use custom loop end point when not in looping mode
    if (vrb->vorb_looping && vrb->vorb_total_pos + nsamp > vrb->vorb_loop_from)
      numread = ov_read_float (&vrb->vf, &pcmdata, vrb->vorb_loop_from - vrb->vorb_total_pos, &bitstreamnum);
    else
    #endif // ZDOOM_AUDIO_LOOP
      numread =  ov_read_float (&vrb->vf, &pcmdata, nsamp, &bitstreamnum);
    if (numread == OV_HOLE)
    { // recoverable error, but discontinue if we get too many
      localerrors++;
      if (localerrors == 10)
      {
        lprintf (LO_WARN, "vorb_render: many errors.  aborting\n");
        vrb->vorb_playing = 0;
        memset (sout, 0, nsamp * 4);
        return;
      }
      continue;
    }
    else if (numread == 0)
    { // EOF
      if (vrb->vorb_looping)
      {
        #ifdef ZDOOM_AUDIO_LOOP
        ov_pcm_seek_lap (&vrb->vf, vrb->vorb_loop_to);
        vrb->vorb_total_pos = vrb->vorb_loop_to;
        #else // ZDOOM_AUDIO_LOOP
        ov_raw_seek_lap (&vf, 0);
        #endif // ZDOOM_AUDIO_LOOP
        continue;
      }
      else
      {
        vrb->vorb_playing = 0;
        memset (sout, 0, nsamp * 4);
        return;
      }
    }
    else if (numread < 0)
    { // unrecoverable errror
      lprintf (LO_WARN, "vorb_render: unrecoverable error\n");
      vrb->vorb_playing = 0;
      memset (sout, 0, nsamp * 4);
      return;
    }
  
    multiplier = 16384.0f / 15.0f * vrb->vorb_volume;
    // volume and downmix
    if (vinfo->channels == 2)
    {
      for (i = 0; i < numread; i++, sout += 2)
      { // data is preclipped?
        sout[0] = (short) (pcmdata[0][i] * multiplier);
        sout[1] = (short) (pcmdata[1][i] * multiplier);
      }
    }
    else // mono
    {
      for (i = 0; i < numread; i++, sout += 2)
      {
        sout[0] = sout [1] = (short) (pcmdata[0][i] * multiplier);
      }
    }
    nsamp -= numread;
    #ifdef ZDOOM_AUDIO_LOOP
    vrb->vorb_total_pos += numread;
    #endif // ZDOOM_AUDIO_LOOP
  }

}

static void vorb_render (music_player_t *music, void *dest, unsigned nsamp)
{ 
	vorb_player_t *vrb = (vorb_player_t*)music;
  I_ResampleStream (&vrb->music, dest, nsamp, vorb_render_ex, vrb->vorb_samplerate_in, vrb->vorb_samplerate_target);
}

static void vorb_seek (struct music_player_s *music, int pos)
{
	vorb_player_t *vrb = (vorb_player_t*)music;

	ov_time_seek(&vrb->vf, (double)pos / (double)TICRATE);
}


vorb_player_t vorb_player =
{
  vorb_name,
  vorb_init,
  vorb_shutdown,
  vorb_setvolume,
  vorb_pause,
  vorb_resume,
  vorb_registersong,
  vorb_unregistersong,
  vorb_play,
  vorb_stop,
  vorb_render,
  vorb_seek,
  0,
  0,
  0,
  vread,
  vseek,
  NULL,
  vtell,
};
vorb_player_t record_player  =
{
	vorb_name,
	vorb_init,
	vorb_shutdown,
	vorb_setvolume,
	vorb_pause,
	vorb_resume,
	vorb_registersong,
	vorb_unregistersong,
	vorb_play,
	vorb_stop,
	vorb_render,
	vorb_seek,
	0,
	0,
	0,
	vread,
	vseek,
	NULL,
	vtell,
};


#endif // HAVE_LIBVORBISFILE
