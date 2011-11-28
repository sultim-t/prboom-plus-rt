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
 *  System interface for sound.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "z_zone.h"

#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "lprintf.h"
#include "s_sound.h"

#include "d_main.h"

//
// MUSIC API.
//

// placeholder for unused option
const char *snd_mididev;

const char *midiplayers[2] = {"quicktime", NULL};
const char *snd_midiplayer = "quicktime";
const char *snd_soundfont;
void M_ChangeMIDIPlayer(void)
{
}

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#include "mus2mid.h"

char *music_tmp = 0; /* cph - name of music temporary file */
QTMovie *movie = 0;
float movieVolume = 1.0;
int inLoopedMode = YES;

void I_ShutdownMusic(void)
{
  if(movie)
  {
    [movie release];
    movie = 0;
  }

  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
  }
}

void I_InitMusic(void)
{
  music_tmp = strdup("/tmp/prboom-music-XXXXXX");
  {
    int fd = mkstemp(music_tmp);
    if (fd<0)
    {
      lprintf(LO_ERROR, "I_InitMusic: failed to create music temp file %s", music_tmp);
      unlink(music_tmp);
      free(music_tmp);
      return;
    }
    close(fd);
  }
  music_tmp = realloc(music_tmp, strlen(music_tmp) + 4);
  strcat(music_tmp, ".mid");
  atexit(I_ShutdownMusic);
}

void I_PlaySong(int handle, int looping)
{
  inLoopedMode = looping ? YES : NO;

  [movie gotoBeginning];
  [movie setAttribute:[NSNumber numberWithBool:inLoopedMode]
         forKey:QTMovieLoopsAttribute];
  [movie setVolume:movieVolume];
  [movie play];
}

void I_UpdateMusic(void)
{
}

void I_PauseSong (int handle)
{
  if(!movie) return;
  [movie stop];
}

void I_ResumeSong (int handle)
{
  if(!movie) return;
  [movie play];
}

void I_StopSong(int handle)
{
  if(!movie) return;
  [movie stop];
}

void I_UnRegisterSong(int handle)
{
  if(!movie) return;
  [movie stop];
  [movie release];
  movie = 0;
}

int I_RegisterSong(const void *data, size_t len)
{
  FILE *midfile;
  bool MidiIsReady = false;

  if ( music_tmp == NULL )
    return 0;
  midfile = fopen(music_tmp, "wb");
  if ( midfile == NULL ) {
    lprintf(LO_ERROR,"Couldn't write MIDI to %s\n", music_tmp);
    return 0;
  }
  /* Convert MUS chunk to MIDI? */
  if ( memcmp(data, "MUS", 3) == 0 )
  {
    // e6y
    // New mus -> mid conversion code thanks to Ben Ryves <benryves@benryves.com>
    // This plays back a lot of music closer to Vanilla Doom - eg. tnt.wad map02
    void *outbuf;
    size_t outbuf_len;
    int result;

    MEMFILE *instream = mem_fopen_read((void*)data, len);
    MEMFILE *outstream = mem_fopen_write();

    result = mus2mid(instream, outstream);

    if (result == 0)
    {
      mem_get_buf(outstream, &outbuf, &outbuf_len);
      MidiIsReady = M_WriteFile(music_tmp, outbuf, outbuf_len);
    }

    mem_fclose(instream);
    mem_fclose(outstream);
  } else {
    MidiIsReady = fwrite(data, len, 1, midfile) == 1;
  }
  fclose(midfile);

  if (!MidiIsReady)
  {
    lprintf(LO_ERROR,"Couldn't write MIDI to %s\n", music_tmp);
    return 0;
  }

  /* Now play in QTKit */
  NSError *error = 0;
  if(movie)
  {
  	[movie stop];
  	[movie release];
  }
  movie = [QTMovie movieWithFile:[NSString stringWithUTF8String:music_tmp]
                   error:&error];
  if(error)
  {
    lprintf(LO_ERROR,"Failed to create QTMovie: %s",
            [[error localizedDescription] UTF8String]);
    return 0;
  }

  [movie retain];

  [movie gotoBeginning];
  [movie setAttribute:[NSNumber numberWithBool:inLoopedMode]
         forKey:QTMovieLoopsAttribute];
  [movie setVolume:movieVolume];
  [movie play];

  return 1;
}

int I_RegisterMusic( const char* filename, musicinfo_t *song )
{
  // TODO
  return 1;
}

void I_SetMusicVolume(int value)
{
  movieVolume = (float)value / 15.0;
  if(movie)
    [movie setVolume:movieVolume];
}
