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

#include <Carbon/Carbon.h>
#include <QuickTime/Movies.h>
#include "mmus2mid.h"

static int playFile(const char *filename);

char* music_tmp; /* cph - name of music temporary file */
static boolean qtInited = false;
static boolean inLoopedMode = false;
static unsigned songSize = 0;
static char *song;
static Movie movie = NULL;
static short movieVolume = kFullVolume;

void I_ShutdownMusic(void)
{
  if(song)
    free(song);
  if(movie)
    DisposeMovie(movie);

  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
  }

  ExitMovies();
  qtInited = false;

  song = NULL;
  movie = NULL;
}

void I_InitMusic(void)
{
  qtInited = true;
  EnterMovies();
  music_tmp = strdup("/tmp/prboom-music-XXXXXX");
  {
    int fd = mkstemp(music_tmp);
    if (fd<0) {
      lprintf(LO_ERROR, "I_InitMusic: failed to create music temp file %s", music_tmp);
      free(music_tmp); return;
    } else
      close(fd);
  }
  music_tmp = realloc(music_tmp, strlen(music_tmp) + 4);
  strcat(music_tmp, ".mid");
  atexit(I_ShutdownMusic);
}

void I_PlaySong(int handle, int looping)
{
  inLoopedMode = looping;

  GoToBeginningOfMovie(movie);
  StartMovie(movie);
  SetMovieVolume(movie, movieVolume);
}

void I_UpdateMusic(void)
{
  if(!movie) return;

  MoviesTask(movie, 0);

  if(IsMovieDone(movie) && inLoopedMode)
  {
    GoToBeginningOfMovie(movie);
    StartMovie(movie);
  }
}

void I_PauseSong (int handle)
{
  if(!movie) return;

  StopMovie(movie);
}

void I_ResumeSong (int handle)
{
  if(!movie) return;

  StartMovie(movie);
}

void I_StopSong(int handle)
{
  if(!movie) return;

  StopMovie(movie);
}

void I_UnRegisterSong(int handle)
{
  if(!movie) return;

  DisposeMovie(movie);
}

int I_RegisterSong(const void *data, size_t len)
{
  MIDI *mididata;
  FILE *midfile;

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
    UBYTE *mid;
    int midlen;

    mididata = malloc(sizeof(MIDI));
    mmus2mid(data, mididata, 89, 0);
    MIDIToMidi(mididata,&mid,&midlen);
    M_WriteFile(music_tmp,mid,midlen);
    free(mid);
    free_mididata(mididata);
    free(mididata);
  } else {
    fwrite(data, len, 1, midfile);
  }
  fclose(midfile);

  return playFile(music_tmp);
}

int I_RegisterMusic( const char* filename, musicinfo_t *song )
{
  // TODO
  return 1;
}

void I_SetMusicVolume(int value)
{
  movieVolume = 0x000000ff * value / 15;
  if(movie)
  {
    // Update the volume of the running movie.
    SetMovieVolume(movie, movieVolume);
  }
}

static int playFile(const char *filename)
{
  OSErr error = noErr;
  CFStringRef pathStr;
  CFURLRef url;
  FSRef fsRef;
  FSSpec fsSpec;
  short refNum;

  if(!qtInited)
  {
    lprintf(LO_ERROR, "Music: Music system not initialized");
    return 0;
  }

  // Free any previously loaded music.
  if(movie)
  {
    DisposeMovie(movie);
    movie = NULL;
  }

  // Now we'll open the file using Carbon and QuickTime.
  pathStr = CFStringCreateWithCString(NULL, filename, 
            CFStringGetSystemEncoding());
  url = CFURLCreateWithString(NULL, pathStr, NULL);
  CFRelease(pathStr);

  // We've got the URL, get the FSSpec.
  if(!CFURLGetFSRef(url, &fsRef))
  {
    // File does not exist??
    CFRelease(url);
    lprintf(LO_ERROR, "Music: Error on CFURLGetFSRef");
    return 0;
  }
  CFRelease(url);
  url = NULL;
  if(FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, &fsSpec, NULL) 
     != noErr)
  {
    lprintf(LO_ERROR, "Music: Error on FSGetCatalogInfo");
    return 0;
  }

  // Open the 'movie' from the specified file.
  if(OpenMovieFile(&fsSpec, &refNum, fsRdPerm) != noErr)
  {
    lprintf(LO_ERROR, "Music: Error on OpenMovie");
    return 0;
  }
  error = NewMovieFromFile(&movie, refNum, NULL, NULL, 
          newMovieActive & newMovieDontAskUnresolvedDataRefs, NULL);
  CloseMovieFile(refNum);
  if(error != noErr)
  {
    lprintf(LO_ERROR, "Music: Error on NewMovie");
    return 0;
  }

  GoToBeginningOfMovie(movie);
  StartMovie(movie);
  SetMovieVolume(movie, movieVolume);

  return 1;
}
