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

const music_player_t vorb_player =
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

// uncomment to allow (experiemntal) support for
// zdoom-style audio loops
#define ZDOOM_AUDIO_LOOP

static int vorb_looping = 0;
static int vorb_volume = 0; // 0-15
static int vorb_samplerate_target = 0;
static int vorb_samplerate_in = 0;
static int vorb_paused = 0;
static int vorb_playing = 0;

#ifdef ZDOOM_AUDIO_LOOP
static unsigned vorb_loop_from;
static unsigned vorb_loop_to;
static unsigned vorb_total_pos;
#endif // ZDOOM_AUDIO_LOOP

static const char *vorb_data;
static size_t vorb_len;
static size_t vorb_pos;

OggVorbis_File vf;

// io callbacks

static size_t vread (void *dst, size_t s, size_t n, void *src)
{
  size_t size = s * n;

  if (vorb_pos + size >= vorb_len)
    size = vorb_len - vorb_pos;

  memcpy (dst, vorb_data + vorb_pos, size);
  vorb_pos += size;
  return size;
}

static int vseek (void *src, ogg_int64_t offset, int whence)
{
  size_t desired_pos;

  switch (whence)
  {
    case SEEK_SET:
      desired_pos = (size_t) offset;
      break;
    case SEEK_CUR:
      desired_pos = vorb_pos + (size_t) offset;
      break;
    case SEEK_END:
    default:
      desired_pos = vorb_len + (size_t) offset;
      break;
  }
  if (desired_pos > vorb_len) // placing exactly at the end is allowed)
    return -1;
  vorb_pos = desired_pos;
  return 0;
}

static long vtell (void *src)
{
  // correct to vorbisfile spec, this is a long, not 64 bit 
  return (long) vorb_pos;
}


ov_callbacks vcallback =
{
  vread,
  vseek,
  NULL,
  vtell
};


static const char *vorb_name (void)
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

static int vorb_init (int samplerate)
{
  TESTDLLLOAD ("libogg-0.dll", FALSE)
  TESTDLLLOAD ("libvorbis-0.dll", FALSE)
  TESTDLLLOAD ("libvorbisfile-3.dll", TRUE)
  vorb_samplerate_target = samplerate;
  return 1;
}

static void vorb_shutdown (void)
{
  // nothing to do                 


}

static const void *vorb_registersong (const void *data, unsigned len)
{
  int i;
  vorbis_info *vinfo;
  #ifdef ZDOOM_AUDIO_LOOP
  vorbis_comment *vcom;
  #endif // ZDOOM_AUDIO_LOOP

  vorb_data = data;
  vorb_len = len;
  vorb_pos = 0;

  i = ov_test_callbacks ((void *) data, &vf, NULL, 0, vcallback);

  if (i != 0)
  {
    lprintf (LO_WARN, "vorb_registersong: failed\n");
    return NULL;
  }
  i = ov_test_open (&vf);
  
  if (i != 0)
  {
    lprintf (LO_WARN, "vorb_registersong: failed\n");
    ov_clear (&vf);
    return NULL;
  }
  
  vinfo = ov_info (&vf, -1);
  vorb_samplerate_in = vinfo->rate;

  #ifdef ZDOOM_AUDIO_LOOP
  // parse LOOP_START LOOP_END tags
  vorb_loop_from = 0;
  vorb_loop_to = 0;

  vcom = ov_comment (&vf, -1);
  for (i = 0; i < vcom->comments; i++)
  {
    if (strncmp ("LOOP_START=", vcom->user_comments[i], 11) == 0)
      vorb_loop_to = parsetag (vcom->user_comments[i] + 11, vorb_samplerate_in);
    else if (strncmp ("LOOP_END=", vcom->user_comments[i], 9) == 0)
      vorb_loop_from = parsetag (vcom->user_comments[i] + 9, vorb_samplerate_in);
  }
  if (vorb_loop_from == 0)
    vorb_loop_from = 0xffffffff;
  else if (vorb_loop_to >= vorb_loop_from)
    vorb_loop_to = 0;
  #endif // ZDOOM_AUDIO_LOOP

  // handle not used
  return data;
}

static void vorb_setvolume (int v)
{
  vorb_volume = v;
}

static void vorb_pause (void)
{
  vorb_paused = 1;
}

static void vorb_resume (void)
{
  vorb_paused = 0;
}

static void vorb_unregistersong (const void *handle)
{ 
  vorb_data = NULL;
  ov_clear (&vf);
  vorb_playing = 0;
}

static void vorb_play (const void *handle, int looping)
{
  ov_raw_seek_lap (&vf, 0);
 

  vorb_playing = 1;
  vorb_looping = looping;
  #ifdef ZDOOM_AUDIO_LOOP
  vorb_total_pos = 0;
  #endif // ZDOOM_AUDIO_LOOP
}

static void vorb_stop (void)
{
  vorb_playing = 0;
}

static void vorb_render_ex (void *dest, unsigned nsamp)
{
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
  vorbis_info *vinfo = ov_info (&vf, -1);


  if (!vorb_playing || vorb_paused)
  {
    memset (dest, 0, nsamp * 4);
    return;
  }

  while (nsamp > 0)
  {
    #ifdef ZDOOM_AUDIO_LOOP
    // don't use custom loop end point when not in looping mode
    if (vorb_looping && vorb_total_pos + nsamp > vorb_loop_from)
      numread = ov_read_float (&vf, &pcmdata, vorb_loop_from - vorb_total_pos, &bitstreamnum);
    else
    #endif // ZDOOM_AUDIO_LOOP
      numread =  ov_read_float (&vf, &pcmdata, nsamp, &bitstreamnum);
    if (numread == OV_HOLE)
    { // recoverable error, but discontinue if we get too many
      localerrors++;
      if (localerrors == 10)
      {
        lprintf (LO_WARN, "vorb_render: many errors.  aborting\n");
        vorb_playing = 0;
        memset (sout, 0, nsamp * 4);
        return;
      }
      continue;
    }
    else if (numread == 0)
    { // EOF
      if (vorb_looping)
      {
        #ifdef ZDOOM_AUDIO_LOOP
        ov_pcm_seek_lap (&vf, vorb_loop_to);
        vorb_total_pos = vorb_loop_to;
        #else // ZDOOM_AUDIO_LOOP
        ov_raw_seek_lap (&vf, 0);
        #endif // ZDOOM_AUDIO_LOOP
        continue;
      }
      else
      {
        vorb_playing = 0;
        memset (sout, 0, nsamp * 4);
        return;
      }
    }
    else if (numread < 0)
    { // unrecoverable errror
      lprintf (LO_WARN, "vorb_render: unrecoverable error\n");
      vorb_playing = 0;
      memset (sout, 0, nsamp * 4);
      return;
    }
  
    multiplier = 16384.0f / 15.0f * vorb_volume;
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
    vorb_total_pos += numread;
    #endif // ZDOOM_AUDIO_LOOP
  }

}

static void vorb_render (void *dest, unsigned nsamp)
{ 
  I_ResampleStream (dest, nsamp, vorb_render_ex, vorb_samplerate_in, vorb_samplerate_target);
}


const music_player_t vorb_player =
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
  vorb_render
};

#endif // HAVE_LIBVORBISFILE
