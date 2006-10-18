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
#ifdef HAVE_LIBSDL_MIXER
#define HAVE_MIXER
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
#ifdef HAVE_MIXER
#include "SDL_mixer.h"
#endif

#include "z_zone.h"

#include "m_swap.h"
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
int snd_card = 1;
int mus_card = 1;
int detect_voices = 0; // God knows

static boolean sound_inited = false;
static boolean first_sound_init = true;

// Needed for calling the actual sound output.
static int SAMPLECOUNT=   512;
#define MAX_CHANNELS    32

// MWM 2000-01-08: Sample rate in samples/second
int snd_samplerate=11025;

// The actual output device.
int audio_fd;

typedef struct {
  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  int id;
// The channel step amount...
  unsigned int step;
// ... and a 0.16 bit remainder of last step.
  unsigned int stepremainder;
  unsigned int samplerate;
// The channel data pointers, start and end.
  const unsigned char* data;
  const unsigned char* enddata;
// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
  int starttime;
  // Hardware left and right channel volume lookup.
  int *leftvol_lookup;
  int *rightvol_lookup;
} channel_info_t;

channel_info_t channelinfo[MAX_CHANNELS];

// Pitch to stepping lookup, unused.
int   steptable[256];

// Volume lookups.
int   vol_lookup[128*256];

/* cph
 * stopchan
 * Stops a sound, unlocks the data
 */

static void stopchan(int i)
{
  if (channelinfo[i].data) /* cph - prevent excess unlocks */
  {
    channelinfo[i].data=NULL;
    W_UnlockLumpNum(S_sfx[channelinfo[i].id].lumpnum);
  }
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
static int addsfx(int sfxid, int channel, const unsigned char* data, size_t len)
{
  stopchan(channel);

  channelinfo[channel].data = data;
  /* Set pointer to end of raw data. */
  channelinfo[channel].enddata = channelinfo[channel].data + len - 1;
  channelinfo[channel].samplerate = (channelinfo[channel].data[3]<<8)+channelinfo[channel].data[2];
  channelinfo[channel].data += 8; /* Skip header */

  channelinfo[channel].stepremainder = 0;
  // Should be gametic, I presume.
  channelinfo[channel].starttime = gametic;

  // Preserve sound SFX id,
  //  e.g. for avoiding duplicates of chainsaw.
  channelinfo[channel].id = sfxid;

  return channel;
}

static void updateSoundParams(int handle, int volume, int seperation, int pitch)
{
  int slot = handle;
    int   rightvol;
    int   leftvol;
    int         step = steptable[pitch];

#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_UpdateSoundParams: handle out of range");
#endif
  // Set stepping
  // MWM 2000-12-24: Calculates proportion of channel samplerate
  // to global samplerate for mixing purposes.
  // Patched to shift left *then* divide, to minimize roundoff errors
  // as well as to use SAMPLERATE as defined above, not to assume 11025 Hz
    if (pitched_sounds)
    channelinfo[slot].step = step + (((channelinfo[slot].samplerate<<16)/snd_samplerate)-65536);
    else
    channelinfo[slot].step = ((channelinfo[slot].samplerate<<16)/snd_samplerate);

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol = volume - ((volume*seperation*seperation) >> 16);
    seperation = seperation - 257;
    rightvol= volume - ((volume*seperation*seperation) >> 16);

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
  I_Error("rightvol out of bounds");

    if (leftvol < 0 || leftvol > 127)
  I_Error("leftvol out of bounds");

    // Get the proper lookup table piece
    //  for this volume level???
  channelinfo[slot].leftvol_lookup = &vol_lookup[leftvol*256];
  channelinfo[slot].rightvol_lookup = &vol_lookup[rightvol*256];
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
void I_SetChannels(void)
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int   i;
  int   j;

  int*  steptablemid = steptable + 128;

  // Okay, reset internal mixing channels to zero.
  for (i=0; i<MAX_CHANNELS; i++)
  {
    memset(&channelinfo[i],0,sizeof(channel_info_t));
  }

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(1.2, ((double)i/(64.0*snd_samplerate/11025)))*65536.0);


  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
    {
      // proff - made this a little bit softer, because with
      // full volume the sound clipped badly
      vol_lookup[i*256+j] = (i*(j-128)*256)/191;
      //vol_lookup[i*256+j] = (i*(j-128)*256)/127;
    }
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
  const unsigned char* data;
  int lump;
  size_t len;

  if ((channel < 0) || (channel >= MAX_CHANNELS))
#ifdef RANGECHECK
    I_Error("I_StartSound: handle out of range");
#else
    return -1;
#endif

  lump = S_sfx[id].lumpnum;

  // We will handle the new SFX.
  // Set pointer to raw data.
  len = W_LumpLength(lump);

  // e6y: Crash with zero-length sounds.
  // Example wad: dakills (http://www.doomworld.com/idgames/index.php?id=2803)
  // The entries DSBSPWLK, DSBSPACT, DSSWTCHN and DSSWTCHX are all zero-length sounds
  if (len<=8) return -1;

  /* Find padded length */
  len -= 8;
  // do the lump caching outside the SDL_LockAudio/SDL_UnlockAudio pair
  // use locking which makes sure the sound data is in a malloced area and
  // not in a memory mapped one
  data = W_LockLumpNum(lump);

  SDL_LockAudio();

  // Returns a handle (not used).
  addsfx(id, channel, data, len);
  updateSoundParams(channel, vol, sep, pitch);

  SDL_UnlockAudio();


  return channel;
}



void I_StopSound (int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_StopSound: handle out of range");
#endif
  SDL_LockAudio();
  stopchan(handle);
  SDL_UnlockAudio();
}


boolean I_SoundIsPlaying(int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_SoundIsPlaying: handle out of range");
#endif
  return channelinfo[handle].data != NULL;
}


boolean I_AnySoundStillPlaying(void)
{
  boolean result = false;
  int i;

  for (i=0; i<MAX_CHANNELS; i++)
    result |= channelinfo[i].data != NULL;

  return result;
}


//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//

static void I_UpdateSound(void *unused, Uint8 *stream, int len)
{
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned char sample;
  register int    dl;
  register int    dr;

  // Pointers in audio stream, left, right, end.
  signed short*   leftout;
  signed short*   rightout;
  signed short*   leftend;
  // Step in stream, left and right, thus two.
  int       step;

  // Mixing channel index.
  int       chan;

    // Left and right channel
    //  are in audio stream, alternating.
    leftout = (signed short *)stream;
    rightout = ((signed short *)stream)+1;
    step = 2;

    // Determine end, for left channel only
    //  (right channel is implicit).
    leftend = leftout + (len/4)*step;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
    while (leftout != leftend)
    {
  // Reset left/right value.
  //dl = 0;
  //dr = 0;
  dl = *leftout;
  dr = *rightout;

  // Love thy L2 chache - made this a loop.
  // Now more channels could be set at compile time
  //  as well. Thus loop those  channels.
    for ( chan = 0; chan < numChannels; chan++ )
  {
      // Check channel, if active.
      if (channelinfo[chan].data)
      {
    // Get the raw data from the channel.
        // no filtering
        // sample = *channelinfo[chan].data;
        // linear filtering
        sample = (((unsigned int)channelinfo[chan].data[0] * (0x10000 - channelinfo[chan].stepremainder))
                + ((unsigned int)channelinfo[chan].data[1] * (channelinfo[chan].stepremainder))) >> 16;

    // Add left and right part
    //  for this channel (sound)
    //  to the current data.
    // Adjust volume accordingly.
        dl += channelinfo[chan].leftvol_lookup[sample];
        dr += channelinfo[chan].rightvol_lookup[sample];
    // Increment index ???
        channelinfo[chan].stepremainder += channelinfo[chan].step;
    // MSB is next sample???
        channelinfo[chan].data += channelinfo[chan].stepremainder >> 16;
    // Limit to LSB???
        channelinfo[chan].stepremainder &= 0xffff;

    // Check whether we are done.
        if (channelinfo[chan].data >= channelinfo[chan].enddata)
      stopchan(chan);
      }
  }

  // Clamp to range. Left hardware channel.
  // Has been char instead of short.
  // if (dl > 127) *leftout = 127;
  // else if (dl < -128) *leftout = -128;
  // else *leftout = dl;

  if (dl > SHRT_MAX)
      *leftout = SHRT_MAX;
  else if (dl < SHRT_MIN)
      *leftout = SHRT_MIN;
  else
      *leftout = (signed short)dl;

  // Same for right hardware channel.
  if (dr > SHRT_MAX)
      *rightout = SHRT_MAX;
  else if (dr < SHRT_MIN)
      *rightout = SHRT_MIN;
  else
      *rightout = (signed short)dr;

  // Increment current pointers in stream
  leftout += step;
  rightout += step;
    }
}

void I_ShutdownSound(void)
{
  if (sound_inited) {
    lprintf(LO_INFO, "I_ShutdownSound: ");
#ifdef HAVE_MIXER
    Mix_CloseAudio();
#else
    SDL_CloseAudio();
#endif
    lprintf(LO_INFO, "\n");
    sound_inited = false;
  }
}

//static SDL_AudioSpec audio;

void I_InitSound(void)
{
#ifdef HAVE_MIXER
  int audio_rate;
  Uint16 audio_format;
  int audio_channels;
  int audio_buffers;

  if (sound_inited)
      I_ShutdownSound();

  // Secure and configure sound device first.
  lprintf(LO_INFO,"I_InitSound: ");

  /* Initialize variables */
  audio_rate = snd_samplerate;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio_format = AUDIO_S16MSB;
#else
  audio_format = AUDIO_S16LSB;
#endif
  audio_channels = 2;
  SAMPLECOUNT = 512;
  audio_buffers = SAMPLECOUNT*snd_samplerate/11025;

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0) {
    lprintf(LO_INFO,"couldn't open audio with desired format\n");
    return;
  }
  sound_inited = true;
  SAMPLECOUNT = audio_buffers;
  Mix_SetPostMix(I_UpdateSound, NULL);
  lprintf(LO_INFO," configured audio device with %d samples/slice\n", SAMPLECOUNT);
#else
  SDL_AudioSpec audio;

  // Secure and configure sound device first.
  lprintf(LO_INFO,"I_InitSound: ");

  // Open the audio device
  audio.freq = snd_samplerate;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio.format = AUDIO_S16MSB;
#else
  audio.format = AUDIO_S16LSB;
#endif
  audio.channels = 2;
  audio.samples = SAMPLECOUNT*snd_samplerate/11025;
  audio.callback = I_UpdateSound;
  if ( SDL_OpenAudio(&audio, NULL) < 0 ) {
    lprintf(LO_INFO,"couldn't open audio with desired format\n");
    return;
  }
  SAMPLECOUNT = audio.samples;
  lprintf(LO_INFO," configured audio device with %d samples/slice\n", SAMPLECOUNT);
#endif

  if (first_sound_init) {
    atexit(I_ShutdownSound);
    first_sound_init = false;
  }

  if (!nomusicparm)
    I_InitMusic();

  // Finished initialization.
  lprintf(LO_INFO,"I_InitSound: sound module ready\n");
#ifndef HAVE_MIXER
  SDL_PauseAudio(0);
#endif
}




//
// MUSIC API.
//

#ifndef HAVE_OWN_MUSIC

#ifdef HAVE_MIXER
#include "SDL_mixer.h"
#include "mmus2mid.h"

static Mix_Music *music[2] = { NULL, NULL };

char* music_tmp = NULL; /* cph - name of music temporary file */

#endif

void I_ShutdownMusic(void)
{
#ifdef HAVE_MIXER
  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
	music_tmp = NULL;
  }
#endif
}

void I_InitMusic(void)
{
#ifdef HAVE_MIXER
  if (!music_tmp) {
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
#endif
}

void I_PlaySong(int handle, int looping)
{
#ifdef HAVE_MIXER
  if ( music[handle] ) {
    Mix_FadeInMusic(music[handle], looping ? -1 : 0, 500);
  }
#endif
}

extern int mus_pause_opt; // From m_misc.c

void I_PauseSong (int handle)
{
#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_StopSong(handle);
    break;
  case 1:
      Mix_PauseMusic();
    break;
  }
#endif
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_PlaySong(handle,1);
    break;
  case 1:
      Mix_ResumeMusic();
    break;
  }
#endif
  /* Otherwise, music wasn't stopped */
}

void I_StopSong(int handle)
{
#ifdef HAVE_MIXER
    Mix_FadeOutMusic(500);
#endif
}

void I_UnRegisterSong(int handle)
{
#ifdef HAVE_MIXER
  if ( music[handle] ) {
    Mix_FreeMusic(music[handle]);
    music[handle] = NULL;
  }
#endif
}

int I_RegisterSong(const void *data, size_t len)
{
#ifdef HAVE_MIXER
  MIDI *mididata;
  FILE *midfile;

  if ( len < 32 )
    return 0; // the data should at least as big as the MUS header
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

  music[0] = Mix_LoadMUS(music_tmp);
  if ( music[0] == NULL ) {
    lprintf(LO_ERROR,"Couldn't load MIDI from %s: %s\n", music_tmp, Mix_GetError());
  }
#endif
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
#ifdef HAVE_MIXER
  Mix_VolumeMusic(volume*8);
#endif
}

#endif /* HAVE_OWN_MUSIC */

