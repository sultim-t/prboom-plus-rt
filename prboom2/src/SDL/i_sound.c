/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_sound.c,v 1.11 2000/09/16 20:20:45 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
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
 *	System interface for sound.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: i_sound.c,v 1.11 2000/09/16 20:20:45 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
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
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "lprintf.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "d_main.h"

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

#define PIPE_CHECK(fh) if (broken_pipe) { fclose(fh); fh = NULL; broken_pipe = 0; }

// Variables used by Boom from Allegro
// created here to avoid changes to core Boom files
int snd_card = 1;
int mus_card = 1;
int detect_voices = 0; // God knows

// Needed for calling the actual sound output.
static int SAMPLECOUNT=		512;
#define NUM_CHANNELS		8

#define SAMPLERATE		11025	// Hz

// The actual output device.
int	audio_fd;


// The channel step amount...
unsigned int	channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int	channelstepremainder[NUM_CHANNELS];
unsigned int	channelsamplerate[NUM_CHANNELS];


// The channel data pointers, start and end.
const unsigned char*	channels[NUM_CHANNELS];
const unsigned char*	channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int		channelstart[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];			

// Pitch to stepping lookup, unused.
int		steptable[256];

// Volume lookups.
int		vol_lookup[128*256];

// Hardware left and right channel volume lookup.
int*		channelleftvol_lookup[NUM_CHANNELS];
int*		channelrightvol_lookup[NUM_CHANNELS];

/* cph 
 * stopchan
 * Stops a sound, unlocks the data 
 */

static void stopchan(int i)
{
  if (!channels[i]) return; /* cph - prevent excess unlocks */
  channels[i]=0;
  W_UnlockLumpNum(S_sfx[channelids[i]].lumpnum);
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
int
addsfx
( int		sfxid)
{
    int		i;
    
    int		oldest = gametic;
    int		oldestnum = 0;
    int		slot;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if ( sfxid == sfx_sawup
	 || sfxid == sfx_sawidl
	 || sfxid == sfx_sawful
	 || sfxid == sfx_sawhit
	 || sfxid == sfx_stnmov
	 || sfxid == sfx_pistol	 )
    {
	// Loop all channels, check.
	for (i=0 ; i<NUM_CHANNELS ; i++)
	{
	    // Active, and using the same SFX?
	    if ( (channels[i])
		 && (channelids[i] == sfxid) )
	    {
		// Reset.
		stopchan(i);
		// We are sure that iff,
		//  there will only be one.
		break;
	    }
	}
    }

    /* Loop all channels to find either an unused one 
     * or the one playing for the longest time */
    for (i=0; (i<NUM_CHANNELS) && (channels[i]); i++)
    {
	if (channelstart[i] < oldest)
	{
	    oldestnum = i;
	    oldest = channelstart[i];
	}
    }

    if (i == NUM_CHANNELS)
      stopchan(slot = oldestnum);
    else
	slot = i;

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    {
      int lump = S_sfx[sfxid].lumpnum;
      size_t len = W_LumpLength(lump);

      /* Find padded length */
      len = (((len-8) + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT + 8;
      channels[slot] = W_CacheLumpNumPadded(lump, len, 128);
      
      /* Set pointer to end of raw data. */
      channelsend[slot] = channels[slot] + len;
      channelsamplerate[slot] = (channels[slot][3]<<8)+channels[slot][2];
      channels[slot] += 8; /* Skip header */
    }

    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;

    /* cph - generate useful handle */
    return (slot + NUM_CHANNELS*sfxid);
}

void
I_UpdateSoundParams
( int	handle,
  int	volume,
  int	seperation,
  int	pitch)
{
    int         slot = handle & (NUM_CHANNELS-1);
    int		rightvol;
    int		leftvol;
    int         step = steptable[pitch];

    // Set stepping???
    // Kinda getting the impression this is never used.
    if (pitched_sounds)
      channelstep[slot] = step + (((channelsamplerate[slot]/11025)-1)<<16);
    else
      channelstep[slot] = ((channelsamplerate[slot]/11025)<<16);

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;
    seperation *= seperation;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    volume *= 8;
    leftvol =
	volume - ((volume*seperation) >> 16);
    seperation = seperation - 257;
    rightvol =
	volume - ((volume*seperation) >> 16);	

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
	I_Error("rightvol out of bounds");
    
    if (leftvol < 0 || leftvol > 127)
	I_Error("leftvol out of bounds");
    
    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

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
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process. 
  int		i;
  int		j;
    
  int*	steptablemid = steptable + 128;
  
  // Okay, reset internal mixing channels to zero.
  /*for (i=0; i<NUM_CHANNELS; i++)
  {
    channels[i] = 0;
  }*/

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(2.0, (i/64.0))*65536.0);
  
  
  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++) {
      // proff - made this a little bit softer, because with
      // full volume the sound clipped badly
      vol_lookup[i*256+j] = (i*(j-128)*256)/191;
      //vol_lookup[i*256+j] = (i*(j-128)*256)/127;
    }
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
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{

  // UNUSED
  priority = 0;
  
    // Debug.
    //fprintf( stderr, "starting sound %d", id );
    
    // Returns a handle (not used).
    SDL_LockAudio();
    id = addsfx( id);
    I_UpdateSoundParams(id, vol, sep, pitch);
    SDL_UnlockAudio();

    // fprintf( stderr, "/handle is %d\n", id );
    
    return id;
}



void I_StopSound (int handle)
{
  SDL_LockAudio();
  stopchan(handle & (NUM_CHANNELS-1));
  SDL_UnlockAudio();
}


boolean I_SoundIsPlaying(int handle)
{
  int chan = handle & (NUM_CHANNELS-1);
  return (channels[chan] && 
	  channelids[chan] == (handle / NUM_CHANNELS));
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
void I_UpdateSound(void *unused, Uint8 *stream, int len)
{
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned int	sample;
  register int		dl;
  register int		dr;
  
  // Pointers in audio stream, left, right, end.
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
  // Step in stream, left and right, thus two.
  int				step;

  // Mixing channel index.
  int				chan;
   
#ifdef HAVE_MIXER
    // Mix in the music
    //music_mixer(NULL, stream, len);
#endif

    // Left and right channel
    //  are in audio stream, alternating.
    leftout = (signed short *)stream;
    rightout = ((signed short *)stream)+1;
    step = 2;

    // Determine end, for left channel only
    //  (right channel is implicit).
    leftend = leftout + SAMPLECOUNT*step;

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
	for ( chan = 0; chan < NUM_CHANNELS; chan++ )
	{
	    // Check channel, if active.
	    if (channels[ chan ])
	    {
		// Get the raw data from the channel. 
		sample = *channels[ chan ];
		// Add left and right part
		//  for this channel (sound)
		//  to the current data.
		// Adjust volume accordingly.
		dl += channelleftvol_lookup[ chan ][sample];
		dr += channelrightvol_lookup[ chan ][sample];
		// Increment index ???
		channelstepremainder[ chan ] += channelstep[ chan ];
		// MSB is next sample???
		channels[ chan ] += channelstepremainder[ chan ] >> 16;
		// Limit to LSB???
		channelstepremainder[ chan ] &= 65536-1;

		// Check whether we are done.
		if (channels[ chan ] >= channelsend[ chan ])
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
  lprintf(LO_DEBUG, "I_ShutdownSound: ");
#ifdef HAVE_MIXER
  Mix_CloseAudio();
#else
  SDL_CloseAudio();
#endif
  lprintf(LO_DEBUG, "\n");
}

//static SDL_AudioSpec audio;

void
I_InitSound()
{ 
#ifdef HAVE_MIXER
  int audio_rate;
  Uint16 audio_format;
  int audio_channels;
  int audio_buffers;

  // Secure and configure sound device first.
  fprintf( stderr, "I_InitSound: ");
 
  /* Initialize variables */
  audio_rate = SAMPLERATE;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio_format = AUDIO_S16MSB;
#else
  audio_format = AUDIO_S16LSB;
#endif
  audio_channels = 2;
  audio_buffers = SAMPLECOUNT;
  
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0) {
    fprintf(stderr, "couldn't open audio with desired format\n");
    return;
  }
  Mix_SetPostMix(I_UpdateSound, NULL);
  fprintf(stderr, " configured audio device with %d samples/slice\n", SAMPLECOUNT);
#else
  // Secure and configure sound device first.
  fprintf( stderr, "I_InitSound: ");
  
  // Open the audio device
  audio.freq = SAMPLERATE;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio.format = AUDIO_S16MSB;
#else
  audio.format = AUDIO_S16LSB;
#endif
  audio.channels = 2;
  audio.samples = SAMPLECOUNT;
  audio.callback = I_UpdateSound;
  if ( SDL_OpenAudio(&audio, NULL) < 0 ) {
    fprintf(stderr, "couldn't open audio with desired format\n");
    return;
  }
  SAMPLECOUNT = audio.samples;
  fprintf(stderr, " configured audio device with %d samples/slice\n", SAMPLECOUNT);
#endif
  
  atexit(I_ShutdownSound);
  
  if (!nomusicparm)
    I_InitMusic();
  
  // Finished initialization.
  fprintf(stderr, "I_InitSound: sound module ready\n");
#ifndef HAVE_MIXER
  SDL_PauseAudio(0);
#endif
}




//
// MUSIC API.
//

#ifdef HAVE_MIXER
#include "SDL_mixer.h"
#include "mmus2mid.h"

static Mix_Music *music[2] = { NULL, NULL };

char* music_tmp; /* cph - name of music temporary file */

#endif

void I_ShutdownMusic(void) 
{
#ifdef HAVE_MIXER
  if (music_tmp) {
    unlink(music_tmp);
    lprintf(LO_DEBUG, "I_ShutdownMusic: removing %s\n", music_tmp);
    free(music_tmp);
  }
#endif
}

void I_InitMusic(void)
{
#ifdef HAVE_MIXER
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
#endif
  atexit(I_ShutdownMusic);
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
lprintf(LO_INFO,"Stopping song %d (pause)\n", handle);
    I_StopSong(handle);
    break;
  case 1:
lprintf(LO_INFO,"Pausing song %d (pause)\n", handle);
    Mix_PauseMusic();
    break;
  }
#endif
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
#ifdef HAVE_MIXER
  Mix_ResumeMusic();
#endif
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
  MIDI mididata;
  FILE *midfile;

  midfile = fopen(music_tmp, "wb");
  if ( midfile == NULL ) {
    lprintf(LO_ERROR,"Couldn't write MIDI to %s\n", music_tmp);
    return 0;
  }
  /* Convert MUS chunk to MIDI? */
  if ( memcmp(data, "MUS", 3) == 0 )
  {
    //qmus2mid(data, len, midfile, 1, 0, 0, 0);
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
#endif
  return (0);
}

void I_SetMusicVolume(int volume)
{
#ifdef HAVE_MIXER
  Mix_VolumeMusic(volume*8);
#endif
}

