// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      System interface, sound.
//
//-----------------------------------------------------------------------------

#ifndef __I_SOUND__
#define __I_SOUND__

#include <stdio.h>

#ifdef DJGPP
  #include <allegro.h>
#endif

#include "sounds.h"

// UNIX hack, to be removed.
#ifdef SNDSERV
extern FILE* sndserver;
extern char* sndserver_filename;
#endif

// Init at program start...
void I_InitSound();

void I_CacheSound(sfxinfo_t *sound);

// ... update sound buffer and audio device at runtime...
void I_UpdateSound(void);
void I_SubmitSound(void);

// ... shut down and relase at program termination.
void I_ShutdownSound(void);

//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels();

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfxinfo);

// Starts a sound in a particular sound channel.
// haleyjd: requires different API for VC++ version
// 07/12/02: adjusted DOS code to be consistent

int I_StartSound(sfxinfo_t *sound, int cnum, int vol, int sep, int pitch, int pri);

// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);

//
//  MUSIC I/O
//
void I_InitMusic(void);
void I_ShutdownMusic(void);

// Volume.
void I_SetMusicVolume(int volume);

// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

// Registers a song handle to song data.
int I_RegisterSong(void *data);

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int handle, int looping);

// Stops a song over 3 seconds.
void I_StopSong(int handle);

// See above (register), then think backwards
void I_UnRegisterSong(int handle);

// Allegro card support jff 1/18/98
extern  int snd_card;
extern  int mus_card;
extern  int detect_voices; // jff 3/4/98 option to disable voice detection

#endif

//----------------------------------------------------------------------------
//
// $Log: i_sound.h,v $
// Revision 1.4  1998/05/03  22:31:58  killough
// beautification, add some external declarations
//
// Revision 1.3  1998/02/23  04:27:08  killough
// Add variable pitched sound support
//
// Revision 1.2  1998/01/26  19:26:57  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
