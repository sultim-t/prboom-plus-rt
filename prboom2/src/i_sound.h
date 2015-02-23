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

#ifndef __I_SOUND__
#define __I_SOUND__

#include "sounds.h"
#include "doomtype.h"

#define SNDSERV
#undef SNDINTR

#ifndef SNDSERV
#include "l_soundgen.h"
#endif

extern int snd_pcspeaker;

// Init at program start...
void I_InitSound(void);

// ... shut down and relase at program termination.
void I_ShutdownSound(void);

//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels(void);

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfxinfo);

// Starts a sound in a particular sound channel.
int I_StartSound(int id, int channel, int vol, int sep, int pitch, int priority);

// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
dboolean I_SoundIsPlaying(int handle);

// Called by m_menu.c to let the quit sound play and quit right after it stops
dboolean I_AnySoundStillPlaying(void);

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);

// NSM sound capture routines
// silences sound output, and instead allows sound capture to work
// call this before sound startup
void I_SetSoundCap (void);
// grabs len samples of audio (16 bit interleaved)
unsigned char *I_GrabSound (int len);

// NSM helper routine for some of the streaming audio
void I_ResampleStream (void *dest, unsigned nsamp, void (*proc) (void *dest, unsigned nsamp), unsigned sratein, unsigned srateout);

//
//  MUSIC I/O
//
extern const char *snd_soundfont;
extern const char *snd_mididev;
extern char music_player_order[][200];

void I_InitMusic(void);
void I_ShutdownMusic(void);

// Volume.
void I_SetMusicVolume(int volume);

// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

// Registers a song handle to song data.
int I_RegisterSong(const void *data, size_t len);

// cournia - tries to load a music file
int I_RegisterMusic( const char* filename, musicinfo_t *music );

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
extern int snd_card;
extern int mus_card;
// CPhipps - put these in config file
extern int snd_samplerate;

extern int use_experimental_music;

extern int mus_fluidsynth_chorus;
extern int mus_fluidsynth_reverb;
extern int mus_fluidsynth_gain; // NSM  fine tune fluidsynth output level
extern int mus_opl_gain; // NSM  fine tune OPL output level

// prefered MIDI player
typedef enum
{
  midi_player_sdl,
  midi_player_fluidsynth,
  midi_player_opl2,
  midi_player_portmidi,

  midi_player_last
} midi_player_name_t;

extern const char *snd_midiplayer;
extern const char *midiplayers[];

void M_ChangeMIDIPlayer(void);

#endif
