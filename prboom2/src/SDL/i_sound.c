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
#ifdef HAVE_LIBSDL2_MIXER
#define HAVE_MIXER
#endif

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mutex.h"

#include "SDL_endian.h"

#include "SDL_version.h"
#include "SDL_thread.h"
#ifdef HAVE_MIXER
#define USE_RWOPS
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

//e6y
#include "i_pcsound.h"
#include "e6y.h"

int snd_pcspeaker;

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Variables used by Boom from Allegro
// created here to avoid changes to core Boom files
int snd_card = 1;
int mus_card = 1;
int detect_voices = 0; // God knows

static dboolean sound_inited = false;
static dboolean first_sound_init = true;

// Needed for calling the actual sound output.
static int SAMPLECOUNT =   512;
#define MAX_CHANNELS    32

// MWM 2000-01-08: Sample rate in samples/second
int snd_samplerate = 11025;

// The actual output device.
int audio_fd;

typedef struct
{
  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  int id;
  // The channel step amount...
  unsigned int step;
  // ... and a 0.16 bit remainder of last step.
  unsigned int stepremainder;
  unsigned int samplerate;
  // The channel data pointers, start and end.
  const unsigned char *data;
  const unsigned char *enddata;
  // Time/gametic that the channel started playing,
  //  used to determine oldest, which automatically
  //  has lowest priority.
  // In case number of active sounds exceeds
  //  available channels.
  int starttime;
  // left and right channel volume (0-127)
  int leftvol;
  int rightvol;
} channel_info_t;

channel_info_t channelinfo[MAX_CHANNELS];

// Pitch to stepping lookup, unused.
int   steptable[256];

// Volume lookups.
//int   vol_lookup[128 * 256];

// NSM
static int dumping_sound = 0;


// lock for updating any params related to sfx
SDL_mutex *sfxmutex;
// lock for updating any params related to music
SDL_mutex *musmutex;


/* cph
 * stopchan
 * Stops a sound, unlocks the data
 */

static void stopchan(int i)
{
  if (channelinfo[i].data) /* cph - prevent excess unlocks */
  {
    channelinfo[i].data = NULL;
  }
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
static int addsfx(int sfxid, int channel, const unsigned char *data, size_t len)
{
  stopchan(channel);

  channelinfo[channel].data = data;
  /* Set pointer to end of raw data. */
  channelinfo[channel].enddata = channelinfo[channel].data + len - 1;
  channelinfo[channel].samplerate = (channelinfo[channel].data[3] << 8) + channelinfo[channel].data[2];
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

  if (snd_pcspeaker)
    return;

  // Set stepping
  // MWM 2000-12-24: Calculates proportion of channel samplerate
  // to global samplerate for mixing purposes.
  // Patched to shift left *then* divide, to minimize roundoff errors
  // as well as to use SAMPLERATE as defined above, not to assume 11025 Hz
  if (pitched_sounds)
    channelinfo[slot].step = step + (((channelinfo[slot].samplerate << 16) / snd_samplerate) - 65536);
  else
    channelinfo[slot].step = ((channelinfo[slot].samplerate << 16) / snd_samplerate);

  // Separation, that is, orientation/stereo.
  //  range is: 1 - 256
  seperation += 1;

  // Per left/right channel.
  //  x^2 seperation,
  //  adjust volume properly.
  leftvol = volume - ((volume * seperation * seperation) >> 16);
  seperation = seperation - 257;
  rightvol = volume - ((volume * seperation * seperation) >> 16);

  // Sanity check, clamp volume.
  if (rightvol < 0 || rightvol > 127)
    I_Error("rightvol out of bounds");

  if (leftvol < 0 || leftvol > 127)
    I_Error("leftvol out of bounds");

  // Get the proper lookup table piece
  //  for this volume level???
  channelinfo[slot].leftvol = leftvol;
  channelinfo[slot].rightvol = rightvol;
}

void I_UpdateSoundParams(int handle, int volume, int seperation, int pitch)
{
  SDL_LockMutex (sfxmutex);
  updateSoundParams(handle, volume, seperation, pitch);
  SDL_UnlockMutex (sfxmutex);
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
  //int   j;

  int  *steptablemid = steptable + 128;

  // Okay, reset internal mixing channels to zero.
  for (i = 0; i < MAX_CHANNELS; i++)
  {
    memset(&channelinfo[i], 0, sizeof(channel_info_t));
  }

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i = -128 ; i < 128 ; i++)
    steptablemid[i] = (int)(pow(1.2, ((double)i / (64.0 * snd_samplerate / 11025))) * 65536.0);


  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  /*
  for (i = 0 ; i < 128 ; i++)
    for (j = 0 ; j < 256 ; j++)
    {
      // proff - made this a little bit softer, because with
      // full volume the sound clipped badly
      vol_lookup[i * 256 + j] = (i * (j - 128) * 256) / 191;
      //vol_lookup[i*256+j] = (i*(j-128)*256)/127;
    }
  */
}

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
  char namebuf[9];
  const char *prefix;

  // Different prefix for PC speaker sound effects.
  prefix = (snd_pcspeaker ? "dp" : "ds");

  sprintf(namebuf, "%s%s", prefix, sfx->name);
  return W_SafeGetNumForName(namebuf); //e6y: make missing sounds non-fatal
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
  const unsigned char *data;
  int lump;
  size_t len;

  if ((channel < 0) || (channel >= MAX_CHANNELS))
#ifdef RANGECHECK
    I_Error("I_StartSound: handle out of range");
#else
    return -1;
#endif

  if (snd_pcspeaker)
    return I_PCS_StartSound(id, channel, vol, sep, pitch, priority);

  lump = S_sfx[id].lumpnum;

  // We will handle the new SFX.
  // Set pointer to raw data.
  len = W_LumpLength(lump);

  // e6y: Crash with zero-length sounds.
  // Example wad: dakills (http://www.doomworld.com/idgames/index.php?id=2803)
  // The entries DSBSPWLK, DSBSPACT, DSSWTCHN and DSSWTCHX are all zero-length sounds
  if (len <= 8) return -1;

  /* Find padded length */
  len -= 8;
  // do the lump caching outside the SDL_LockAudio/SDL_UnlockAudio pair
  // use locking which makes sure the sound data is in a malloced area and
  // not in a memory mapped one
  data = W_LockLumpNum(lump);

  SDL_LockMutex (sfxmutex);

  // Returns a handle (not used).
  addsfx(id, channel, data, len);
  updateSoundParams(channel, vol, sep, pitch);

  SDL_UnlockMutex (sfxmutex);


  return channel;
}



void I_StopSound (int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_StopSound: handle out of range");
#endif

  if (snd_pcspeaker)
  {
    I_PCS_StopSound(handle);
    return;
  }

  SDL_LockMutex (sfxmutex);
  stopchan(handle);
  SDL_UnlockMutex (sfxmutex);
}


dboolean I_SoundIsPlaying(int handle)
{
#ifdef RANGECHECK
  if ((handle < 0) || (handle >= MAX_CHANNELS))
    I_Error("I_SoundIsPlaying: handle out of range");
#endif

  if (snd_pcspeaker)
    return I_PCS_SoundIsPlaying(handle);

  return channelinfo[handle].data != NULL;
}


dboolean I_AnySoundStillPlaying(void)
{
  dboolean result = false;
  int i;

  if (snd_pcspeaker)
    return false;

  for (i = 0; i < MAX_CHANNELS; i++)
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

#ifndef HAVE_OWN_MUSIC
static void Exp_UpdateMusic (void *buff, unsigned nsamp);
#endif

// from pcsound_sdl.c
void PCSound_Mix_Callback(void *udata, Uint8 *stream, int len);

static void I_UpdateSound(void *unused, Uint8 *stream, int len)
{
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  // register unsigned char sample;
  register int    dl;
  register int    dr;

  // Pointers in audio stream, left, right, end.
  signed short   *leftout;
  signed short   *rightout;
  signed short   *leftend;
  // Step in stream, left and right, thus two.
  int       step;

  // Mixing channel index.
  int       chan;

  // NSM: when dumping sound, ignore the callback calls and only
  // service dumping calls
  if (dumping_sound && unused != (void *) 0xdeadbeef)
    return;

#ifndef HAVE_OWN_MUSIC
  // do music update
  if (use_experimental_music)
  {
    SDL_LockMutex (musmutex);
    Exp_UpdateMusic (stream, len / 4);
    SDL_UnlockMutex (musmutex);
  }
#endif

  if (snd_pcspeaker)
  {
    PCSound_Mix_Callback (NULL, stream, len);
    // no sfx mixing
    return;
  }

  SDL_LockMutex (sfxmutex);
  // Left and right channel
  //  are in audio stream, alternating.
  leftout = (signed short *)stream;
  rightout = ((signed short *)stream) + 1;
  step = 2;

  // Determine end, for left channel only
  //  (right channel is implicit).
  leftend = leftout + (len / 4) * step;

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
        //int s = channelinfo[chan].data[0] * 0x10000 - 0x800000;

        // linear filtering
        // the old SRC did linear interpolation back into 8 bit, and then expanded to 16 bit.
        // this does interpolation and 8->16 at same time, allowing slightly higher quality
        int s = ((unsigned int)channelinfo[chan].data[0] * (0x10000 - channelinfo[chan].stepremainder))
              + ((unsigned int)channelinfo[chan].data[1] * (channelinfo[chan].stepremainder))
              - 0x800000; // convert to signed


        // Add left and right part
        //  for this channel (sound)
        //  to the current data.
        // Adjust volume accordingly.

        // full loudness (vol=127) is actually 127/191

        dl += channelinfo[chan].leftvol * s / 49152;  // >> 15;
        dr += channelinfo[chan].rightvol * s / 49152; // >> 15;

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
  SDL_UnlockMutex (sfxmutex);
}

void I_ShutdownSound(void)
{
  if (sound_inited)
  {
    lprintf(LO_INFO, "I_ShutdownSound: ");
#ifdef HAVE_MIXER
    Mix_CloseAudio();
#endif
    SDL_CloseAudio();
    lprintf(LO_INFO, "\n");
    sound_inited = false;

    if (sfxmutex)
    {
      SDL_DestroyMutex (sfxmutex);
      sfxmutex = NULL;
    }
  }
}

//static SDL_AudioSpec audio;

void I_InitSound(void)
{
  int audio_rate;
  int audio_channels;
  int audio_buffers;
  SDL_AudioSpec audio;

  // haleyjd: the docs say we should do this
  if (SDL_InitSubSystem(SDL_INIT_AUDIO))
  {
    lprintf(LO_INFO, "Couldn't initialize SDL audio (%s))\n", SDL_GetError());
    nosfxparm = true;
    nomusicparm = true;
    return;
  }
  if (sound_inited)
      I_ShutdownSound();

  // Secure and configure sound device first.
  lprintf(LO_INFO, "I_InitSound: ");

  if (!use_experimental_music)
  {
#ifdef HAVE_MIXER

    /* Initialize variables */
    audio_rate = snd_samplerate;
    audio_channels = 2;
    SAMPLECOUNT = 512;
    audio_buffers = SAMPLECOUNT*snd_samplerate/11025;

    if (Mix_OpenAudio(audio_rate, MIX_DEFAULT_FORMAT, audio_channels, audio_buffers) < 0)
    {
      lprintf(LO_INFO,"couldn't open audio with desired format (%s)\n", SDL_GetError());
      nosfxparm = true;
      nomusicparm = true;
      return;
    }
    sound_inited_once = true;//e6y
    sound_inited = true;
    SAMPLECOUNT = audio_buffers;
    Mix_SetPostMix(I_UpdateSound, NULL);
    lprintf(LO_INFO," configured audio device with %d samples/slice\n", SAMPLECOUNT);
  }
  else
#else // HAVE_MIXER
  }
#endif // HAVE_MIXER
  {
    // Open the audio device
    audio.freq = snd_samplerate;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
    audio.format = AUDIO_S16MSB;
#else
    audio.format = AUDIO_S16LSB;
#endif
    audio.channels = 2;
    audio.samples = SAMPLECOUNT * snd_samplerate / 11025;
    audio.callback = I_UpdateSound;
    if ( SDL_OpenAudio(&audio, NULL) < 0 )
    {
      lprintf(LO_INFO, "couldn't open audio with desired format (%s))\n", SDL_GetError());
      nosfxparm = true;
      nomusicparm = true;
      return;
    }
    sound_inited_once = true;//e6y
    sound_inited = true;
    SAMPLECOUNT = audio.samples;
    lprintf(LO_INFO, " configured audio device with %d samples/slice\n", SAMPLECOUNT);
  }
  if (first_sound_init)
  {
    atexit(I_ShutdownSound);
    first_sound_init = false;
  }

  sfxmutex = SDL_CreateMutex ();

  // If we are using the PC speaker, we now need to initialise it.
  if (snd_pcspeaker)
    I_PCS_InitSound();

  if (!nomusicparm)
    I_InitMusic();

  // Finished initialization.
  lprintf(LO_INFO, "I_InitSound: sound module ready\n");
  SDL_PauseAudio(0);
}


// NSM sound capture routines

// silences sound output, and instead allows sound capture to work
// call this before sound startup
void I_SetSoundCap (void)
{
  dumping_sound = 1;
}

// grabs len samples of audio (16 bit interleaved)
unsigned char *I_GrabSound (int len)
{
  static unsigned char *buffer = NULL;
  static size_t buffer_size = 0;
  size_t size;

  if (!dumping_sound)
    return NULL;

  size = len * 4;
  if (!buffer || size > buffer_size)
  {
    buffer_size = size * 4;
    buffer = realloc (buffer, buffer_size);
  }

  if (buffer)
  {
    memset (buffer, 0, size);
    I_UpdateSound ((void *) 0xdeadbeef, buffer, size);
  }
  return buffer;
}




// NSM helper routine for some of the streaming audio
void I_ResampleStream (void *dest, unsigned nsamp, void (*proc) (void *dest, unsigned nsamp), unsigned sratein, unsigned srateout)
{ // assumes 16 bit signed interleaved stereo
  
  unsigned i;
  int j = 0;
  
  short *sout = dest;
  
  static short *sin = NULL;
  static unsigned sinsamp = 0;

  static unsigned remainder = 0;
  unsigned step = (sratein << 16) / (unsigned) srateout;

  unsigned nreq = (step * nsamp + remainder) >> 16;

  if (nreq > sinsamp)
  {
    sin = realloc (sin, (nreq + 1) * 4);
    if (!sinsamp) // avoid pop when first starting stream
      sin[0] = sin[1] = 0;
    sinsamp = nreq;
  }

  proc (sin + 2, nreq);

  for (i = 0; i < nsamp; i++)
  {
    *sout++ = ((unsigned) sin[j + 0] * (0x10000 - remainder) +
               (unsigned) sin[j + 2] * remainder) >> 16;
    *sout++ = ((unsigned) sin[j + 1] * (0x10000 - remainder) +
               (unsigned) sin[j + 3] * remainder) >> 16;
    remainder += step;
    j += remainder >> 16 << 1;
    remainder &= 0xffff;
  }
  sin[0] = sin[nreq * 2];
  sin[1] = sin[nreq * 2 + 1];
}  
  

#ifndef HAVE_OWN_MUSIC

//
// MUSIC API.
//

int use_experimental_music = -1;

static void Exp_UpdateMusic (void *buff, unsigned nsamp);
static int Exp_RegisterMusic (const char *filename, musicinfo_t *song);
static int Exp_RegisterSong (const void *data, size_t len);
static int Exp_RegisterSongEx (const void *data, size_t len, int try_mus2mid);
static void Exp_SetMusicVolume (int volume);
static void Exp_UnRegisterSong(int handle);
static void Exp_StopSong(int handle);
static void Exp_ResumeSong (int handle);
static void Exp_PauseSong (int handle);
static void Exp_PlaySong(int handle, int looping);
static void Exp_InitMusic(void);
static void Exp_ShutdownMusic(void);





#ifdef HAVE_MIXER

#include "mus2mid.h"


static Mix_Music *music[2] = { NULL, NULL };

// Some tracks are directly streamed from the RWops;
// we need to free them in the end
static SDL_RWops *rw_midi = NULL;

static char *music_tmp = NULL; /* cph - name of music temporary file */

// List of extensions that can be appended to music_tmp. First must be "".
static const char *music_tmp_ext[] = { "", ".mp3", ".ogg" };
#define MUSIC_TMP_EXT (sizeof(music_tmp_ext)/sizeof(*music_tmp_ext))

#endif

void I_ShutdownMusic(void)
{
  if (use_experimental_music)
  {
    Exp_ShutdownMusic ();
    return;
  }
#ifdef HAVE_MIXER
  if (music_tmp) {
    int i;
    char *name;

    S_StopMusic();
    for (i = 0; i < MUSIC_TMP_EXT; i++)
    {
      name = malloc(strlen(music_tmp) + strlen(music_tmp_ext[i]) + 1);
      sprintf(name, "%s%s", music_tmp, music_tmp_ext[i]);
      if (!unlink(name))
        lprintf(LO_DEBUG, "I_ShutdownMusic: removed %s\n", name);
      free(name);
    }
    free(music_tmp);
    music_tmp = NULL;
  }
#endif
}

void I_InitMusic(void)
{
  if (use_experimental_music)
  {
    Exp_InitMusic ();
    return;
  }
#ifdef HAVE_MIXER
  if (!music_tmp) {
#ifndef _WIN32
    music_tmp = strdup("/tmp/"PACKAGE_TARNAME"-music-XXXXXX");
    {
      int fd = mkstemp(music_tmp);
      if (fd<0) {
        lprintf(LO_ERROR, "I_InitMusic: failed to create music temp file %s", music_tmp);
        free(music_tmp); music_tmp = NULL; return;
      } else 
        close(fd);
    }
#else /* !_WIN32 */
    music_tmp = strdup("doom.tmp");
#endif
    atexit(I_ShutdownMusic);
  }
  return;
#endif
  lprintf (LO_INFO, "I_InitMusic: Was compiled without SDL_Mixer support.  You should enable experimental music.\n");
}

void I_PlaySong(int handle, int looping)
{
  if (use_experimental_music)
  {
    Exp_PlaySong (handle, looping);
    return;
  }
#ifdef HAVE_MIXER
  if ( music[handle] ) {
    //Mix_FadeInMusic(music[handle], looping ? -1 : 0, 500);
    Mix_PlayMusic(music[handle], looping ? -1 : 0);

    // haleyjd 10/28/05: make sure volume settings remain consistent
    I_SetMusicVolume(snd_MusicVolume);
  }
#endif
}

extern int mus_pause_opt; // From m_misc.c

void I_PauseSong (int handle)
{
  if (use_experimental_music)
  {
    Exp_PauseSong (handle);
    return;
  }
#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_StopSong(handle);
    break;
  case 1:
    switch (Mix_GetMusicType(NULL))
    {
    case MUS_NONE:
      break;
    case MUS_MID:
      // SDL_mixer's native MIDI music playing does not pause properly.
      // As a workaround, set the volume to 0 when paused.
      I_SetMusicVolume(0);
      break;
    default:
      Mix_PauseMusic();
      break;
    }
    break;
  }
#endif
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
  if (use_experimental_music)
  {
    Exp_ResumeSong (handle);
    return;
  }
#ifdef HAVE_MIXER
  switch(mus_pause_opt) {
  case 0:
      I_PlaySong(handle,1);
    break;
  case 1:
    switch (Mix_GetMusicType(NULL))
    {
    case MUS_NONE:
      break;
    case MUS_MID:
      I_SetMusicVolume(snd_MusicVolume);
      break;
    default:
      Mix_ResumeMusic();
      break;
    }
    break;
  }
#endif
  /* Otherwise, music wasn't stopped */
}

void I_StopSong(int handle)
{
  if (use_experimental_music)
  {
    Exp_StopSong (handle);
    return;
  }
#ifdef HAVE_MIXER
  // halt music playback
  Mix_HaltMusic();
#endif
}

void I_UnRegisterSong(int handle)
{
  if (use_experimental_music)
  {
    Exp_UnRegisterSong (handle);
    return;
  }
#ifdef HAVE_MIXER
  if ( music[handle] ) {
    Mix_FreeMusic(music[handle]);
    music[handle] = NULL;

    // Free RWops
    if (rw_midi != NULL)
    {
      //SDL_FreeRW(rw_midi);
      rw_midi = NULL;
    }
  }
#endif
}

int I_RegisterSong(const void *data, size_t len)
{
  int i;
  char *name;
  dboolean io_errors = false;


  if (use_experimental_music)
  {
    return Exp_RegisterSong (data, len);
  }
#ifdef HAVE_MIXER

  if (music_tmp == NULL)
    return 0;

  // e6y: new logic by me
  // Now you can hear title music in deca.wad
  // http://www.doomworld.com/idgames/index.php?id=8808
  // Ability to use mp3 and ogg as inwad lump

  music[0] = NULL;

  if (len > 4 && memcmp(data, "MUS", 3) != 0)
  {
    // The header has no MUS signature
    // Let's try to load this song with SDL
    for (i = 0; i < MUSIC_TMP_EXT; i++)
    {
      // Current SDL_mixer (up to 1.2.8) cannot load some MP3 and OGG
      // without proper extension
      name = malloc(strlen(music_tmp) + strlen(music_tmp_ext[i]) + 1);
      sprintf(name, "%s%s", music_tmp, music_tmp_ext[i]);

      if (strlen(music_tmp_ext[i]) == 0)
      {
        //midi
        rw_midi = SDL_RWFromConstMem(data, len);
        if (rw_midi)
        {
          music[0] = Mix_LoadMUS_RW(rw_midi, SDL_FALSE);
        }
      }

      if (!music[0])
      {
        io_errors = (M_WriteFile(name, data, len) == 0);
        if (!io_errors)
        {
          music[0] = Mix_LoadMUS(name);
        }
      }

      free(name);
      if (music[0])
        break; // successfully loaded
    }
  }

  // e6y: from Chocolate-Doom
  // Assume a MUS file and try to convert
  if (!music[0])
  {
    MEMFILE *instream;
    MEMFILE *outstream;
    void *outbuf;
    size_t outbuf_len;
    int result;

    instream = mem_fopen_read(data, len);
    outstream = mem_fopen_write();

    // e6y: from chocolate-doom
    // New mus -> mid conversion code thanks to Ben Ryves <benryves@benryves.com>
    // This plays back a lot of music closer to Vanilla Doom - eg. tnt.wad map02
    result = mus2mid(instream, outstream);

    if (result != 0)
    {
      size_t muslen = len;
      const unsigned char *musptr = data;

      // haleyjd 04/04/10: scan forward for a MUS header. Evidently DMX was 
      // capable of doing this, and would skip over any intervening data. That, 
      // or DMX doesn't use the MUS header at all somehow.
      while (musptr < (const unsigned char*)data + len - sizeof(musheader))
      {
        // if we found a likely header start, reset the mus pointer to that location,
        // otherwise just leave it alone and pray.
        if (!strncmp((const char*)musptr, "MUS\x1a", 4))
        {
          mem_fclose(instream);
          instream = mem_fopen_read(musptr, muslen);
          result = mus2mid(instream, outstream);
          break;
        }

        musptr++;
        muslen--;
      }
    }

    if (result == 0)
    {
      mem_get_buf(outstream, &outbuf, &outbuf_len);
      
      rw_midi = SDL_RWFromMem(outbuf, outbuf_len);
      if (rw_midi)
      {
        music[0] = Mix_LoadMUS_RW(rw_midi, SDL_FALSE);
      }
      
      if (!music[0])
      {
        io_errors = M_WriteFile(music_tmp, outbuf, outbuf_len) == 0;

        if (!io_errors)
        {
          // Load the MUS
          music[0] = Mix_LoadMUS(music_tmp);
        }
      }
    }

    mem_fclose(instream);
    mem_fclose(outstream);
  }
  
  // Failed to load
  if (!music[0])
  {
    // Conversion failed, free everything
    if (rw_midi != NULL)
    {
      //SDL_FreeRW(rw_midi);
      rw_midi = NULL;
    }

    if (io_errors)
    {
      lprintf(LO_ERROR, "Error writing song\n");
    }
    else
    {
      lprintf(LO_ERROR, "Error loading song: %s\n", Mix_GetError());
    }
  }

#endif
  return (0);
}

// cournia - try to load a music file into SDL_Mixer
//           returns true if could not load the file
int I_RegisterMusic( const char* filename, musicinfo_t *song )
{
  if (use_experimental_music)
  {
    return Exp_RegisterMusic (filename, song);
    
  }


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
      song->lumpnum = 0;
      return 0;
    }
#else
  return 1;
#endif
}

void I_SetMusicVolume(int volume)
{
  if (use_experimental_music)
  {
    Exp_SetMusicVolume (volume);
    return;
  }
#ifdef HAVE_MIXER
  Mix_VolumeMusic(volume*8);

#ifdef _WIN32
  // e6y: workaround
  if (mus_extend_volume && Mix_GetMusicType(NULL) == MUS_MID)
    I_midiOutSetVolumes(volume  /* *8  */);
#endif

#endif
}







/********************************************************

experimental music API

********************************************************/



// note that the "handle" passed around by s_sound is ignored
// however, a handle is maintained for the individual music players

const char *snd_soundfont; // soundfont name for synths that use it
const char *snd_mididev; // midi device to use (portmidiplayer)

#include "mus2mid.h"

#include "MUSIC/musicplayer.h"

#include "MUSIC/oplplayer.h"
#include "MUSIC/madplayer.h"
#include "MUSIC/dumbplayer.h"
#include "MUSIC/flplayer.h"
#include "MUSIC/vorbisplayer.h"
#include "MUSIC/portmidiplayer.h"

// list of possible music players
static const music_player_t *music_players[] =
{ // until some ui work is done, the order these appear is the autodetect order.
  // of particular importance:  things that play mus have to be last, because
  // mus2midi very often succeeds even on garbage input
  &vorb_player, // vorbisplayer.h
  &mp_player, // madplayer.h
  &db_player, // dumbplayer.h
  &fl_player, // flplayer.h
  &opl_synth_player, // oplplayer.h
  &pm_player, // portmidiplayer.h
  NULL
};
#define NUM_MUS_PLAYERS ((int)(sizeof (music_players) / sizeof (music_player_t *) - 1))


static int music_player_was_init[NUM_MUS_PLAYERS];

#define PLAYER_VORBIS     "vorbis player"
#define PLAYER_MAD        "mad mp3 player"
#define PLAYER_DUMB       "dumb tracker player"
#define PLAYER_FLUIDSYNTH "fluidsynth midi player"
#define PLAYER_OPL2       "opl2 synth player"
#define PLAYER_PORTMIDI   "portmidi midi player"

// order in which players are to be tried
char music_player_order[NUM_MUS_PLAYERS][200] =
{
  PLAYER_VORBIS,
  PLAYER_MAD,
  PLAYER_DUMB,
  PLAYER_FLUIDSYNTH,
  PLAYER_OPL2,
  PLAYER_PORTMIDI,
};

// prefered MIDI device
const char *snd_midiplayer;

const char *midiplayers[midi_player_last + 1] = {
  "sdl", "fluidsynth", "opl2", "portmidi", NULL};

static int current_player = -1;
static const void *music_handle = NULL;

// songs played directly from wad (no mus->mid conversion)
// won't have this
static void *song_data = NULL;

int mus_fluidsynth_chorus;
int mus_fluidsynth_reverb;
int mus_fluidsynth_gain; // NSM  fine tune fluidsynth output level
int mus_opl_gain; // NSM  fine tune OPL output level


static void Exp_ShutdownMusic(void)
{
  int i;
  S_StopMusic ();

  for (i = 0; music_players[i]; i++)
  {
    if (music_player_was_init[i])
      music_players[i]->shutdown ();
  }

  if (musmutex)
  {
    SDL_DestroyMutex (musmutex);
    musmutex = NULL;
  }
}


static void Exp_InitMusic(void)
{
  int i;
  musmutex = SDL_CreateMutex ();


  // todo not so greedy
  for (i = 0; music_players[i]; i++)
    music_player_was_init[i] = music_players[i]->init (snd_samplerate);
  atexit(Exp_ShutdownMusic);
}

static void Exp_PlaySong(int handle, int looping)
{
  if (music_handle)
  {
    SDL_LockMutex (musmutex);
    music_players[current_player]->play (music_handle, looping);
    music_players[current_player]->setvolume (snd_MusicVolume);
    SDL_UnlockMutex (musmutex);
  }

}

extern int mus_pause_opt; // From m_misc.c

static void Exp_PauseSong (int handle)
{
  if (!music_handle)
    return;

  SDL_LockMutex (musmutex);
  switch (mus_pause_opt)
  {
    case 0:
      music_players[current_player]->stop ();
      break;
    case 1:
      music_players[current_player]->pause ();
      break;
    default: // Default - let music continue
      break;
  }  
  SDL_UnlockMutex (musmutex);
}

static void Exp_ResumeSong (int handle)
{
  if (!music_handle)
    return;
  
  SDL_LockMutex (musmutex);
  switch (mus_pause_opt)
  {
    case 0: // i'm not sure why we can guarantee looping=true here,
            // but that's what the old code did
      music_players[current_player]->play (music_handle, 1);
      break;
    case 1:
      music_players[current_player]->resume ();
      break;
    default: // Default - music was never stopped
      break;
  }
  SDL_UnlockMutex (musmutex);
}

static void Exp_StopSong(int handle)
{
  if (music_handle)
  {
    SDL_LockMutex (musmutex);
    music_players[current_player]->stop ();
    SDL_UnlockMutex (musmutex);
  }
}

static void Exp_UnRegisterSong(int handle)
{
  if (music_handle)
  {
    SDL_LockMutex (musmutex);
    music_players[current_player]->unregistersong (music_handle);
    music_handle = NULL;
    if (song_data)
    {
      free (song_data);
      song_data = NULL;
    }
    SDL_UnlockMutex (musmutex);
  }
}

static void Exp_SetMusicVolume (int volume)
{
  if (music_handle)
  {
    SDL_LockMutex (musmutex);
    music_players[current_player]->setvolume (volume);
    SDL_UnlockMutex (musmutex);
  }
}

// returns 1 on success, 0 on failure
static int Exp_RegisterSongEx (const void *data, size_t len, int try_mus2mid)
{
  int i, j;
  dboolean io_errors = false;

  MEMFILE *instream;
  MEMFILE *outstream;
  void *outbuf;
  size_t outbuf_len;
  int result;

  //try_mus2mid = 0; // debug: supress mus2mid conversion completely


  if (music_handle)
    Exp_UnRegisterSong (0);


  // e6y: new logic by me
  // Now you can hear title music in deca.wad
  // http://www.doomworld.com/idgames/index.php?id=8808
  // Ability to use mp3 and ogg as inwad lump

  if (len > 4 && memcmp(data, "MUS", 3) != 0)
  {
    // The header has no MUS signature
    // Let's try to load this song directly
  
    // go through music players in order
    int found = 0;

    for (j = 0; j < NUM_MUS_PLAYERS; j++)
    {
      found = 0;
      for (i = 0; music_players[i]; i++)
      {
        if (strcmp (music_players[i]->name (), music_player_order[j]) == 0)
        {
          found = 1;
          if (music_player_was_init[i])
          {
            const void *temp_handle = music_players[i]->registersong (data, len);
            if (temp_handle)
            {
              SDL_LockMutex (musmutex);
              current_player = i;
              music_handle = temp_handle;
              SDL_UnlockMutex (musmutex);
              lprintf (LO_INFO, "Exp_RegisterSongEx: Using player %s\n", music_players[i]->name ());
              return 1;
            }
          }
          else
            lprintf (LO_INFO, "Exp_RegisterSongEx: Music player %s on preferred list but it failed to init\n", music_players[i]-> name ());
        }
      }
      if (!found)
        lprintf (LO_INFO, "Exp_RegisterSongEx: Couldn't find preferred music player %s in list\n  (typo or support not included at compile time)\n", music_player_order[j]);
    }
    // load failed
  }




  // load failed? try mus2mid
  if (try_mus2mid)
  {

    instream = mem_fopen_read (data, len);
    outstream = mem_fopen_write ();

    // e6y: from chocolate-doom
    // New mus -> mid conversion code thanks to Ben Ryves <benryves@benryves.com>
    // This plays back a lot of music closer to Vanilla Doom - eg. tnt.wad map02
    result = mus2mid(instream, outstream);
    if (result != 0)
    {
      size_t muslen = len;
      const unsigned char *musptr = data;

      // haleyjd 04/04/10: scan forward for a MUS header. Evidently DMX was
      // capable of doing this, and would skip over any intervening data. That,
      // or DMX doesn't use the MUS header at all somehow.
      while (musptr < (const unsigned char*)data + len - sizeof(musheader))
      {
        // if we found a likely header start, reset the mus pointer to that location,
        // otherwise just leave it alone and pray.
        if (!strncmp ((const char*) musptr, "MUS\x1a", 4))
        {
          mem_fclose (instream);
          instream = mem_fopen_read (musptr, muslen);
          result = mus2mid (instream, outstream);
          break;
        }

        musptr++;
        muslen--;
      }
    }
    if (result == 0)
    {
      mem_get_buf(outstream, &outbuf, &outbuf_len);

      // recopy so we can free the MEMFILE
      song_data = malloc (outbuf_len);
      if (song_data)
        memcpy (song_data, outbuf, outbuf_len);

      mem_fclose(instream);
      mem_fclose(outstream);

      if (song_data)
      { 
        return Exp_RegisterSongEx (song_data, outbuf_len, 0);
      }
    }
  }

  lprintf (LO_ERROR, "Exp_RegisterSongEx: Failed\n");
  return 0;
}


static int Exp_RegisterSong (const void *data, size_t len)
{
  Exp_RegisterSongEx (data, len, 1);
  return 0;
}

// try register external music file (not in WAD)

static int Exp_RegisterMusic (const char *filename, musicinfo_t *song)
{
  int len;

  len = M_ReadFile (filename, (byte **) &song_data);

  if (len == -1)
  {
    lprintf (LO_WARN, "Couldn't read %s\nAttempting to load default MIDI music.\n", filename);
    return 1;
  }

  if (!Exp_RegisterSongEx (song_data, len, 1))
  {
    free (song_data);
    song_data = NULL;
    lprintf(LO_WARN, "Couldn't load music from %s\nAttempting to load default MIDI music.\n", filename);
    return 1; // failure
  }

  song->data = 0;
  song->handle = 0;
  song->lumpnum = 0;
  return 0;
}

static void Exp_UpdateMusic (void *buff, unsigned nsamp)
{

  if (!music_handle)
  {
    memset (buff, 0, nsamp * 4);
    return;
  }


  music_players[current_player]->render (buff, nsamp);
}

void M_ChangeMIDIPlayer(void)
{
  int experimental_music;

#ifdef HAVE_OWN_MUSIC
  // do not bother about small memory leak
  snd_midiplayer = strdup(midiplayers[midi_player_sdl]);
  use_experimental_music = 0;
  return;
#endif

  if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_sdl]))
  {
    experimental_music = false;
  }
  else
  {
    experimental_music = true;

    if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_fluidsynth]))
    {
      strcpy(music_player_order[3], PLAYER_FLUIDSYNTH);
      strcpy(music_player_order[4], PLAYER_OPL2);
      strcpy(music_player_order[5], PLAYER_PORTMIDI);
    }
    else if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_opl2]))
    {
      strcpy(music_player_order[3], PLAYER_OPL2);
      strcpy(music_player_order[4], PLAYER_FLUIDSYNTH);
      strcpy(music_player_order[5], PLAYER_PORTMIDI);
    }
    else if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_portmidi]))
    {
      strcpy(music_player_order[3], PLAYER_PORTMIDI);
      strcpy(music_player_order[4], PLAYER_FLUIDSYNTH);
      strcpy(music_player_order[5], PLAYER_OPL2);
    }
  }

#if 1
  if (use_experimental_music == -1)
  {
    use_experimental_music = experimental_music;
  }
  else
  {
    if (experimental_music && use_experimental_music)
    {
      S_StopMusic();
      S_RestartMusic();
    }
  }
#else
  S_StopMusic();

  if (use_experimental_music != experimental_music)
  {
    I_ShutdownMusic();

    S_Stop();
    I_ShutdownSound();

    use_experimental_music = experimental_music;

    I_InitSound();
  }

  S_RestartMusic();
#endif
}

#endif
