/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_sound.c,v 1.25 2002/11/17 18:34:54 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000,2002 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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

static const char
rcsid[] = "$Id: i_sound.c,v 1.25 2002/11/17 18:34:54 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mutex.h"
#include "SDL_byteorder.h"
#include "SDL_version.h"
#include "SDL_mixer.h"

#include "z_zone.h"

#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "lprintf.h"
#include "s_sound.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "d_main.h"

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Variables used by Boom from Allegro
// created here to avoid changes to core Boom files
int snd_card = -1;
int mus_card = -1;
int detect_voices = 0; // God knows

// MWM 2000-01-08: Sample rate in samples/second
int snd_samplerate=11025;

/* cph 2002/08/18 - This is a function that ought to be provided by SDL IMO.
 * It converts a lump of audio data of known format to a Mix_Chunk.
 * This code is basically the latter half of Mix_LoadWAV_RW from SDL_mixer/mixer.
 */
static Mix_Chunk* I_Mix_LoadAudioSpec(SDL_AudioSpec* wavespec, Uint8* buf, Uint32 len)
{
	SDL_AudioCVT wavecvt;
	int samplesize;
	Mix_Chunk *chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));

	chunk->abuf = buf; chunk->alen = len;

	/* Build the audio converter and create conversion buffers */
	if ( SDL_BuildAudioCVT(&wavecvt,
			wavespec->format, wavespec->channels, wavespec->freq,
			MIX_DEFAULT_FORMAT, 2, snd_samplerate) < 0 ) {
		free(chunk);
		return(NULL);
	}
	samplesize = ((wavespec->format & 0xFF)/8)*wavespec->channels;
	wavecvt.len = chunk->alen & ~(samplesize-1);
	wavecvt.buf = (Uint8 *)malloc(wavecvt.len*wavecvt.len_mult);
	if ( wavecvt.buf == NULL ) {
		SDL_SetError("Out of memory");
		free(chunk);
		return(NULL);
	}
	memcpy(wavecvt.buf, chunk->abuf, chunk->alen);

	/* Run the audio converter */
	if ( SDL_ConvertAudio(&wavecvt) < 0 ) {
		free(wavecvt.buf);
		free(chunk);
		return(NULL);
	}
	chunk->allocated = 1;
	chunk->abuf = wavecvt.buf;
	chunk->alen = wavecvt.len_cvt;
	chunk->volume = MIX_MAX_VOLUME;
	return(chunk);
}

static Mix_Chunk* LumpToMixerChunk(int lumpnum)
{
  SDL_AudioSpec doomspec;
  const Uint8* lump = W_CacheLumpNum(lumpnum);
  doomspec.freq = (lump[3] << 8) + lump[2];
  doomspec.format = AUDIO_U8;
  doomspec.channels = 1;
  doomspec.samples = 4096; // ?
  {
    Mix_Chunk* c = I_Mix_LoadAudioSpec(&doomspec,lump+8,W_LumpLength(lumpnum)-8);
    W_UnlockLumpNum(lumpnum);
    return c;
  }
}

static void updateSoundParams(int handle, int volume, int seperation, int pitch)
{
  int leftvol, rightvol;
  // Separation, that is, orientation/stereo.
  //  range is: 1 - 256
  seperation += 1;
  volume *= 9; /* was 8, increased range for SDL */

  // Per left/right channel.
  //  x^2 seperation,
  //  adjust volume properly.
  leftvol = volume - ((volume*seperation*seperation) >> 16);
  seperation = seperation - 257;
  rightvol= volume - ((volume*seperation*seperation) >> 16);  

  // Sanity check, clamp volume.
  if (rightvol < 0 || rightvol > 255)
    I_Error("rightvol out of bounds");
    
  if (leftvol < 0 || leftvol > 255)
    I_Error("leftvol out of bounds");
    
  Mix_SetPanning(handle, leftvol, rightvol);
}

void I_UpdateSoundParams(int handle, int volume, int seperation, int pitch)
{
  SDL_LockAudio();
  updateSoundParams(handle, volume, seperation, pitch);
  SDL_UnlockAudio();
}

//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
void I_SetChannels()
{
} 

 
void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_GetNumForName(namebuf);
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int I_StartSound(int id, int channel, int vol, int sep, int pitch, int priority)
{
  int handle;

  // UNUSED
  priority = 0;

  if (!S_sfx[id].data) {
    S_sfx[id].data = LumpToMixerChunk(S_sfx[id].lumpnum);
  }
  
  SDL_LockAudio();
  handle = Mix_PlayChannel(-1,(Mix_Chunk*)S_sfx[id].data,0);
  updateSoundParams(handle, vol, sep, pitch);
  SDL_UnlockAudio();

  return handle;
}



void I_StopSound (int handle)
{
  SDL_LockAudio();
  Mix_HaltChannel(handle);
  SDL_UnlockAudio();
}


boolean I_SoundIsPlaying(int handle)
{
  return Mix_Playing(handle);
}

void I_ShutdownSound(void)
{    
  lprintf(LO_INFO, "I_ShutdownSound: ");
  Mix_CloseAudio();
  lprintf(LO_INFO, "\n");
}

//static SDL_AudioSpec audio;

void
I_InitSound()
{ 
  int audio_buffers;
  // Needed for calling the actual sound output.
  int SAMPLECOUNT = 512;

  // Secure and configure sound device first.
  lprintf(LO_INFO,"I_InitSound: ");

  /* Initialize variables */
  audio_buffers = SAMPLECOUNT*snd_samplerate/11025;
  
  if (Mix_OpenAudio(snd_samplerate, MIX_DEFAULT_FORMAT, 2, audio_buffers) < 0) {
    lprintf(LO_INFO,"couldn't open audio with desired format\n");
    return;
  }
  SAMPLECOUNT = audio_buffers;
  lprintf(LO_INFO," configured audio device with %d samples/slice\n", SAMPLECOUNT);
  
  atexit(I_ShutdownSound);
  
  if (!nomusicparm)
    I_InitMusic();
  
  // Finished initialization.
  lprintf(LO_INFO,"I_InitSound: sound module ready\n");
}




//
// MUSIC API.
//

#include "SDL_mixer.h"
#include "mmus2mid.h"

static Mix_Music *music[2] = { NULL, NULL };

char* music_tmp; /* cph - name of music temporary file */

void I_ShutdownMusic(void) 
{
  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
  }
}

void I_InitMusic(void)
{
#ifndef _WIN32
  music_tmp = strdup("/tmp/prboom-music-XXXXXX");
  {
    int fd = mkstemp(music_tmp);
    if (fd<0) {
      lprintf(LO_ERROR, "I_InitMusic: failed to create music temp file %s", music_tmp);
      free(music_tmp); return;
    } else 
      close(fd);
  }
#else /* !_WIN32 */
  music_tmp = strdup("doom.tmp");
#endif
  atexit(I_ShutdownMusic);
}

void I_PlaySong(int handle, int looping)
{
  if ( music[handle] ) {
    Mix_FadeInMusic(music[handle], looping ? -1 : 0, 500);
  }
}

int mus_pause_opt = 2;

void I_PauseSong (int handle)
{
  switch(mus_pause_opt) {
  case 0:
    I_StopSong(handle);
    break;
  case 1:
    Mix_PauseMusic();
    break;
  }
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
  switch(mus_pause_opt) {
  case 0:
    I_PlaySong(handle,1);
    break;
  case 1:
    Mix_ResumeMusic();
    break;
  }
  /* Otherwise, music wasn't stopped */
}

void I_StopSong(int handle)
{
  Mix_FadeOutMusic(500);
}

void I_UnRegisterSong(int handle)
{
  if ( music[handle] ) {
    Mix_FreeMusic(music[handle]);
    music[handle] = NULL;
  }
}

int I_RegisterSong(const void *data, size_t len)
{
  MIDI mididata;
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

    memset(&mididata,0,sizeof(MIDI));
    mmus2mid(data, &mididata, 89, 0);
    MIDIToMidi(&mididata,&mid,&midlen);
    M_WriteFile(music_tmp,mid,midlen);
    free(mid);
  } else {
    fwrite(data, len, 1, midfile);
  }
  fclose(midfile);

  music[0] = Mix_LoadMUS(music_tmp);
  if ( music[0] == NULL ) {
    lprintf(LO_ERROR,"Couldn't load MIDI from %s: %s\n", music_tmp, Mix_GetError());
  }
  return (0);
}

// cournia - try to load a music file into SDL_Mixer
//           returns true if could not load the file
int I_RegisterMusic( const char* filename, musicinfo_t *song )
{
#ifdef HAVE_MIXER
  if (!filename) return 1;
  if (!song) return 1;
  music[0] = Mix_LoadMUS(filename);
  if (music[0] == NULL)
    {
      lprintf(LO_WARN,"Couldn't load music from %s: %s\nAttempting to load default MIDI music.\n", filename, Mix_GetError());
      return 1;
    }
  else
    {
      song->data = 0;
      song->handle = 0;
      song->lumpnum = -1; //this doesn't seem to cause problems
      return 0;
    }
#else
  return 1;
#endif
}

void I_SetMusicVolume(int volume)
{
  Mix_VolumeMusic(volume*8);
}


