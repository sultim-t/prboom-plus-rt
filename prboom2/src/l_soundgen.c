/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_soundgen.c,v 1.1 2000/05/04 08:07:43 proff_fs Exp $
 *
 *  Sound server for LxDoom, based on the sound server sources released
 *   with the original linuxdoom.
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999 by Colin Phipps
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
 *  Mixes the playing sounds, and sends them to /dev/dsp
 *-----------------------------------------------------------------------------
 */

static const char 
rcsid[] = "$Id: l_soundgen.c,v 1.1 2000/05/04 08:07:43 proff_fs Exp $";

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif defined(HAVE_SOUNDCARD_H)
#include <soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#else
#error No sound card header?
#endif

#include "l_soundgen.h"
#include "sounds.h"
#include "m_swap.h"

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
#define SAMPLECOUNT		512
#define NUM_CHANNELS		8
// It is 2 for 16bit, and 2 for two channels.
#define BUFMUL                  4

#define SAMPLERATE		11025	// Hz
#define SAMPLESIZE		2   	// 16bit

// Format that corresponds to unsigned bytes, Doom's own internal format
#define UNSIGNED_BYTES AFMT_U8
// The 'zero' sample for that format
#define BYTE_SAMP_ZERO 128

// Format that corresponds to signed words stereo output
// endianness compensated for
#ifndef WORDS_BIGENDIAN
#define SIGNED_WORDS AFMT_S16_LE
#else
#define SIGNED_WORDS AFMT_S16_BE
#endif

// Maximum volume that should ever be generated/requested
#define VOL_MAX                 128

// Handle on /dev/dsp
static int audio_fd = -1;

// Sound format
static unsigned int out_format;
static size_t MIXBUFFERSIZE;

// The actual lengths of all sound effects.
int *lengths;

// The actual output device.
static int audio_fd;

// The global mixing buffer.
// Basically, samples from all active internal channels
//  are modifed and added, and stored in the buffer
//  that is submitted to the audio device.
static void *mixbuffer;

static int      startnum = 0;

// Pitch to stepping lookup, unused.
static int*	steptable;

// Volume lookups.
static signed short *	sw_vol_lookup;
static unsigned char *	ub_vol_lookup;

typedef struct {
  // The channel data pointers, start and end.
  const unsigned char* data;
  const unsigned char* end;

  // The channel step amount...
  unsigned int	step;
  // ... and a 0.16 bit remainder of last step.
  unsigned int stepremainder;

  // Time/gametic that the channel started playing,
  //  used to determine oldest, which automatically
  //  has lowest priority.
  // In case number of active sounds exceeds
  //  available channels.
  int starttime;
  
  // The sound in channel handles,
  //  determined on registration,
  //  might be used to unregister/stop/modify,
  //  currently unused.
  int handle;

  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  int sfxid;			

  // Hardware left and right channel volume lookup.
  const unsigned char* vol_lookup;
  const signed short* leftvol_lookup;
  const signed short* rightvol_lookup;
} channel_t;

static channel_t* channel;

//
// Safe ioctl, convenience.
//
static void I_Ioctl( int fd, int command, int*	arg )
{   
  int		rc;
  extern int	errno;
  
  rc = ioctl(fd, command, arg);  
  if (rc < 0) {
    fprintf(stderr, "ioctl(dsp,%d,arg) failed\n", command);
    fprintf(stderr, "errno=%d\n", errno);
    exit(-1);
  }
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
int I_AddSfx(int sfxid, int volume, int pitch, int seperation )
{
  static unsigned short	handlenums = 0;
  
  int         step = steptable[pitch];
  int		i;
  int		rc = -1;
  
  int		oldest = INT_MAX;
  int		oldestnum = 0;
  int		slot;
  
  signed int	rightvol;
  signed int	leftvol;
  
  if (audio_fd < 0) return 0;
  
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
      for (i=0 ; i<NUM_CHANNELS ; i++) {
	// Active, and using the same SFX?
	if ( (channel[i].data) && (channel[i].sfxid == sfxid) ) {
	  // Reset.
	  channel[i].data = channel[i].end = NULL;
	  // We are sure that iff,
	  //  there will only be one.
	  break;
	}
      }
    }
  
  // Loop all channels to find oldest SFX.
  for (i=0; (i<NUM_CHANNELS) && (channel[i].data); i++) {
    if (channel[i].starttime < oldest) {
      oldestnum = i;
      oldest = channel[i].starttime;
    }
  }
  
  // Tales from the cryptic.
  // If we found a channel, fine.
  // If not, we simply overwrite the first one, 0.
  // Probably only happens at startup.
  if (i == NUM_CHANNELS)
    slot = oldestnum;
  else
    slot = i;
  
  // Okay, in the less recent channel,
  //  we will handle the new SFX.
  // Set pointer to raw data.
  channel[slot].data = ((unsigned char *) S_sfx[sfxid].data) + 8;
  // Set pointer to end of raw data.
  channel[slot].end = channel[slot].data + lengths[sfxid];
  
  // Reset current handle number, limited to 0..100.
  if (!handlenums)
    handlenums = 100;
  
  // Assign current handle number.
  // Preserved so sounds could be stopped (unused).
  channel[slot].handle = rc = handlenums++;
  
  // Set stepping
  channel[slot].step = step;
  // Amount of stepping hanging from last write
  channel[slot].stepremainder = 0;
  // Should be gametic, I presume.
  channel[slot].starttime = startnum++;
  
  // Get the proper lookup table piece
  //  for this volume level??
  if (out_format == SIGNED_WORDS) {
    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;
    
    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol = volume - ((volume*seperation*seperation) >> 16); ///(256*256);
    seperation = seperation - 257;
    rightvol = volume - ((volume*seperation*seperation) >> 16);	
    
    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol >= VOL_MAX) {
      fprintf(stderr, "I_AddSfx: rightvol out of bounds (%d)\n", rightvol);
      rightvol = (rightvol < 0) ? 0 : VOL_MAX-1;
    }
    
    if (leftvol < 0 || leftvol >= VOL_MAX) {
      fprintf(stderr, "I_AddSfx: leftvol out of bounds (%d)\n", leftvol);
      leftvol = (leftvol < 0) ? 0 : VOL_MAX-1;
    }
    
    channel[slot].leftvol_lookup = &sw_vol_lookup[leftvol*256];
    channel[slot].rightvol_lookup = &sw_vol_lookup[rightvol*256];
  } else {
    if (volume < 0 || volume >= VOL_MAX) {
      volume = 0;
      fprintf(stderr, "I_AddSfx: volume out of bounds\n");
    }

    channel[slot].vol_lookup = &ub_vol_lookup[volume*256];
  }
  
  // Preserve sound SFX id,
  //  e.g. for avoiding duplicates of chainsaw.
  channel[slot].sfxid = sfxid;
  
  // You tell me.
  return rc;
}

const void* I_PadSfx(const void* data, int* size)
{
  unsigned char* paddedsfx;
  int paddedsize, i;

  if (audio_fd < 0) return NULL;

  // Pads the sound effect out to the mixing buffer size.
  // The original realloc would interfere with zone memory.
  paddedsize=(((*size)-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;
  
  // Allocate
  paddedsfx = (unsigned char*)malloc( paddedsize+8);
  
  // Now copy and pad.
  memcpy(  paddedsfx, data, *size );
  for (i=*size ; i<paddedsize+8 ; i++)
    paddedsfx[i] = BYTE_SAMP_ZERO;

  *size = paddedsize;
  return paddedsfx;
}

//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.

// CPhipps - rewritten. This code doesn't assemble individual samples one-by-one// but instead does a pass of the mixing buffer for each playing sample. This 
// gives a performance saving when fewer sounds are playing. Saves the CPU
// having to query every channel structure for every sound sample.

void I_UpdateSound(void)
{
  int chan;
  int active_chans = 0;

  if (audio_fd <=0) return;

  for (chan=0; chan<NUM_CHANNELS; chan++)
    if (channel[chan].data != NULL) {

      switch(out_format) {
      case SIGNED_WORDS:
	{
	  register channel_t* pchan = &channel[chan];
	  register signed short* pbuf = (signed short*)mixbuffer;
	  
	  int n = SAMPLECOUNT;
	  
	  if (!active_chans++) {
	    // First channel to output
	    do {
	      { // Output sample
		register int sample = pchan->data[0];
		
		pbuf[0] = pchan->leftvol_lookup[sample];
		pbuf[1] = pchan->rightvol_lookup[sample];
		pbuf+=2;
	      }
	      {// Move data pointer
		register unsigned int pos = pchan->stepremainder + pchan->step;
		
		pchan->data += pos >> 16;
		pchan->stepremainder = pos & ((1 << 16) - 1); 
	      }
	    } while (--n && (pchan->data < pchan->end));
	  } else
	    do {
	      { // Output sample
		register int sample = pchan->data[0];
		
		pbuf[0] += pchan->leftvol_lookup[sample];
		pbuf[1] += pchan->rightvol_lookup[sample];
		pbuf+=2;
	      }
	      {// Move data pointer
		register unsigned int pos = pchan->stepremainder + pchan->step;
		
		pchan->data += pos >> 16;
		pchan->stepremainder = pos & ((1 << 16) - 1); 
	      }
	    } while (--n && (pchan->data < pchan->end));
	  
	  if (pchan->data >= pchan->end) {
	    // End sound effect
	    pchan->data = pchan->end = NULL;
	  }
	  break;
	}
      case UNSIGNED_BYTES:
	{
	  register channel_t* pchan = &channel[chan];
	  register unsigned char* pbuf = (unsigned char*)mixbuffer;
	  register const unsigned char* const vol_lookup = pchan->vol_lookup;
	  
	  int n = SAMPLECOUNT;
	  
	  if (!active_chans++) {
	    // First channel to output
	    // Output sample
	    register size_t bytes = SAMPLECOUNT;
	    
	    if (pchan->data + bytes > pchan->end)
	      bytes = pchan->end - pchan->data;
	    
	    do {
	      *pbuf++ = vol_lookup[*((pchan->data)++)];
	    } while (--bytes);
	    
	  } else
	    do {
	      // Output sample
	      *(pbuf++) += vol_lookup[*((pchan->data)++)] - BYTE_SAMP_ZERO;
	    } while (--n && (pchan->data < pchan->end));
	  
	  if (pchan->data >= pchan->end) {
	    // End sound effect
	    pchan->data = pchan->end = NULL;
	  }
	}
      }
    }

  if (!active_chans) {
    // Write 0's so soundcard doesn't stutter
    memset(mixbuffer, 
	   (out_format == SIGNED_WORDS) ? 0 : BYTE_SAMP_ZERO, 
	   MIXBUFFERSIZE);
  }
}

// 
// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime. 
// It is called during Timer interrupt with SNDINTR.
// Mixing now done synchronous, and
//  only output be done asynchronous?
//
void I_SubmitSound(void)
{
  // Write it to DSP device.
  if (audio_fd >= 0)
    write(audio_fd, mixbuffer, MIXBUFFERSIZE);
}

void I_EndSoundGen(void)
{
  if (audio_fd < 0) return; // Never init'ed or already cleaned

  free(mixbuffer);
  free(lengths);
  free(channel);
  free(steptable);
  free((out_format == SIGNED_WORDS) ? (void*)sw_vol_lookup : (void*)ub_vol_lookup);

  close(audio_fd); audio_fd = -1;
}

void I_InitSoundGen(const char* snd_dev)
{
  // Secure and configure sound device first.
  fprintf( stderr, "I_InitSoundGen: ");
  
  audio_fd = open(snd_dev, O_WRONLY);
  if (audio_fd<0) {
    fprintf(stderr, "Could not open %s\n", snd_dev);
    return;
  }
  {
    int i = 11 | (2<<16);                                           
    I_Ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
    I_Ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
    
    i=SAMPLERATE;
    I_Ioctl(audio_fd, SNDCTL_DSP_SPEED, &i);
    
    I_Ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &i);

    // Choose your poison
    out_format = SIGNED_WORDS;
    
    if ((i & UNSIGNED_BYTES) && 
	(!(i & SIGNED_WORDS) || !strcmp(snd_dev, "/dev/pcsp16"))) 
      out_format = UNSIGNED_BYTES;

    if (!(i & out_format)) {
      fprintf(stderr, "Could not find supported output format (got 0x%08x)\n", i);
      exit(-1);
    }

    if (out_format == SIGNED_WORDS) {
      i=1;
      I_Ioctl(audio_fd, SNDCTL_DSP_STEREO, &i);
    }

    i = out_format;
    I_Ioctl(audio_fd, SNDCTL_DSP_SETFMT, &i);
    
    fprintf(stderr, " configured %s for %s data\n", snd_dev, 
	    (out_format == SIGNED_WORDS) ? "16bit signed" : "8bit unsigned");

    MIXBUFFERSIZE= (out_format==SIGNED_WORDS) ? SAMPLECOUNT*BUFMUL : SAMPLECOUNT;
  }
    
  // Initialize external data (all sounds) at start, keep static.
  // CPhipps - dynamically allocate all data structures, to save memory
  // for non-sound users.
  mixbuffer   = calloc(MIXBUFFERSIZE, 1);
  lengths     = calloc(NUMSFX, sizeof(*lengths));
  steptable   = calloc(256, sizeof(*steptable));

  if (out_format == SIGNED_WORDS) 
    sw_vol_lookup = calloc(VOL_MAX*256, sizeof(*sw_vol_lookup));
  else 
    ub_vol_lookup = calloc(VOL_MAX*256, sizeof(*ub_vol_lookup));

  channel     = calloc(NUM_CHANNELS, sizeof(*channel));

  // CPhipps - used to be in I_SetChannels, but might as well do it now
  {
    // Init internal lookups (raw data, mixing buffer, channels).
    // This function sets up internal lookups used during
    //  the mixing process. 
    register signed int i, j;
    
    // CPhipps - remove non-portable addressing before start of array
    // This table provides step widths for pitch parameters.
    for (i=-128 ; i<128 ; i++)
      steptable[128+i] = // CPhipps - replace pow call, to save -lm
	(unsigned int)(1<<16) + (i << ((i>0) ? 9 : 8));
   
    // CPhipps - replace /127 by >>7 for speed
    // Generates volume lookup tables
    if (out_format == SIGNED_WORDS) {
      //  Also turn the unsigned char samples
      //  into signed word samples.
      for (i=0 ; i<VOL_MAX ; i++)
	for (j=0 ; j<256 ; j++)
	  sw_vol_lookup[i*256+j] = i*(j-BYTE_SAMP_ZERO);
    } else {
      for (i=0 ; i<VOL_MAX ; i++)
	for (j=0 ; j<256 ; j++)
	  ub_vol_lookup[i*256+j] = 
	    BYTE_SAMP_ZERO + (((signed int)i*(j-BYTE_SAMP_ZERO)) >> 6);
    }
  }
}

/*
 * $Log: l_soundgen.c,v $
 * Revision 1.1  2000/05/04 08:07:43  proff_fs
 * Initial revision
 *
 * Revision 1.21  2000/05/01 17:50:34  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.20  2000/04/05 10:47:31  cph
 * Remove dead #ifdef magic, rely on config.h now
 * Make sndserv work on (Open|Net)BSD, using libossaudio
 * Make --enable-debug compile with -g
 * Make asm stuff only compile on Linux and FreeBSD
 * (draw(col|span).s failed on OpenBSD, linker troubles)
 *
 * Revision 1.19  1999/10/12 13:01:11  cphipps
 * Changed header to GPL
 *
 * Revision 1.18  1999/09/08 21:24:24  cphipps
 * Changed to use autoconf-made config.h to detect the sound driver header
 *
 * Revision 1.17  1999/05/16 08:43:53  cphipps
 * Use endianness detection from m_swap.h
 *
 * Revision 1.16  1999/04/30 09:49:42  cphipps
 * Fix clicking at start of sounds (was playing the sound header by mistake)
 *
 * Revision 1.15  1999/01/27 19:58:02  cphipps
 * Fix headers to avoid FreeBSD warnings
 *
 * Revision 1.14  1998/12/25 17:16:15  cphipps
 * Actively clamp volumes to the range where needed, to prevent static
 * on volume-too-high errors
 *
 * Revision 1.13  1998/12/25 17:04:07  cphipps
 * Add log
 * Scale signed-word volumes to avoid garbage
 *
 */
