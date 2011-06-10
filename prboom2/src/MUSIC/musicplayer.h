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


#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

/*
Anything that implements all of these functions can play music in prboomplus.

render: If audio output isn't delivered by render (say, for midi out), then the
player won't be recordable with video capture.  In that case, still write 0s to
the buffer when render is called.

thread safety: Locks are handled in i_sound.c.  Don't worry about it.

Timing: If you're outputting to render, your timing should come solely from the
calls to render, and not some external timing source.  That's why things stay
synced.
*/


typedef struct
{
  // descriptive name of the player, such as "OPL2 Synth"
  const char *(*name)(void);

  // samplerate is in hz.  return is 1 for success
  int (*init)(int samplerate);

  // deallocate structures, cleanup, ...
  void (*shutdown)(void);

  // set volume, 0 = off, 15 = max
  void (*setvolume)(int v);

  // pause currently running song.
  void (*pause)(void);

  // undo pause
  void (*resume)(void);

  // return a player-specific handle, or NULL on failure.
  // data does not belong to player, but it will persist as long as unregister is not called
  const void *(*registersong)(const void *data, unsigned len);

  // deallocate structures, etc.  data is no longer valid
  void (*unregistersong)(const void *handle);

  void (*play)(const void *handle, int looping);

  // stop
  void (*stop)(void);

  // s16 stereo, with samplerate as specified in init.  player needs to be able to handle
  // just about anything for nsamp.  render can be called even during pause+stop.
  void (*render)(void *dest, unsigned nsamp);
} music_player_t;



// helper for deferred load dll

#ifdef _MSC_VER
#if 1
#define TESTDLLLOAD(a,b)
#else
#define TESTDLLLOAD(a,b)                                                           \
  if (1)                                                                           \
  {                                                                                \
    HMODULE h = LoadLibrary (a);                                                   \
    if (!h)                                                                        \
    {                                                                              \
      lprintf (LO_INFO, a " not found!\n");                                        \
      return 0;                                                                    \
    }                                                                              \
    FreeLibrary (h);                                                               \
    if (b && FAILED (__HrLoadAllImportsForDll (a)))                                \
    {                                                                              \
      lprintf (LO_INFO, "Couldn't get all symbols from " a "\n");                  \
      return 0;                                                                    \
    }                                                                              \
  }
#endif

#else // _MSC_VER
#define TESTDLLLOAD(a,b)

#endif // _MSC_VER







#endif // MUSICPLAYER_H
