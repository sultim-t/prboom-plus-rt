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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//      System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_sound.c,v 1.15 1998/05/03 22:32:33 killough Exp $";

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"

#include "../z_zone.h"
#include "../d_io.h"
#include "../c_runcmd.h"
#include "../c_io.h"
#include "../doomstat.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../w_wad.h"
#include "../g_game.h"     //jff 1/21/98 added to use dprintf in I_RegisterSong
#include "../d_main.h"
#include "../v_misc.h"
#include "../m_argv.h"
#include "../d_gi.h"
#include "../s_sound.h"
#include "../mn_engin.h"

void I_CacheSound(sfxinfo_t *sound);

// Needed for calling the actual sound output.
static int SAMPLECOUNT = 512;
#define MAX_CHANNELS 32

        // sf: adjust temp when changing gamespeed
extern int realtic_clock_rate;

int snd_card;   // default.cfg variables for digi and midi drives
int mus_card;   // jff 1/18/98

// haleyjd: safety variables to keep changes to *_card from making
// these routines think that sound has been initialized when it hasn't
boolean snd_init = false;
boolean mus_init = false;

int detect_voices; //jff 3/4/98 enables voice detection prior to install_sound
//jff 1/22/98 make these visible here to disable sound/music on install err

// MWM 2000-01-08: Sample rate in samples/second
int snd_samplerate=11025;

// The actual output device.
int audio_fd;

typedef struct {
  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  sfxinfo_t *id;
  // The channel step amount...
  unsigned int step;
  // ... and a 0.16 bit remainder of last step.
  unsigned int stepremainder;
  unsigned int samplerate;
  // The channel data pointers, start and end.
  unsigned char* data;
  unsigned char* enddata;
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
int steptable[256];

// Volume lookups.
int vol_lookup[128*256];

/* cph 
 * stopchan
 * Stops a sound, unlocks the data 
 */

static void stopchan(int i)
{
   if(!snd_init)
      return;

   if(channelinfo[i].data) /* cph - prevent excess unlocks */
   {
      channelinfo[i].data = NULL;
      
      // haleyjd: this is an nop in prboom? freeing the sound
      // data causes crashes, so I have no idea what this is 
      // really supposed to do...

      //W_UnlockLumpNum(S_sfx[channelinfo[i].id].lumpnum);
   }
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
// haleyjd: needs to take a sfxinfo_t ptr, not a sound id num
//
int addsfx(sfxinfo_t *sfx, int channel)
{
   size_t len;
   int lump;

   if(!snd_init)
      return channel;

   stopchan(channel);
   
   // We will handle the new SFX.
   // Set pointer to raw data.

   // haleyjd 11/05/03: rewrote to minimize work and fully support
   // precaching

   // haleyjd: Eternity sfxinfo_t does not have a lumpnum field
   lump = I_GetSfxLumpNum(sfx);
   
   // replace missing sounds with a reasonable default
   if(lump == -1)
      lump = W_GetNumForName(gameModeInfo->defSoundName);

   len = W_LumpLength(lump);

   // haleyjd 10/08/04: do not play zero-length sound lumps!
   if(len == 0)
      return 0;

   if(!sfx->data)
      sfx->data = W_CacheLumpNum(lump, PU_STATIC);

   /* Find padded length */   
   len -= 8;

   channelinfo[channel].data = sfx->data;
   
   /* Set pointer to end of raw data. */
   channelinfo[channel].enddata = channelinfo[channel].data + len - 1;
   channelinfo[channel].samplerate = (channelinfo[channel].data[3]<<8)+channelinfo[channel].data[2];
   channelinfo[channel].data += 8; /* Skip header */
   
   channelinfo[channel].stepremainder = 0;
   // Should be gametic, I presume.
   channelinfo[channel].starttime = gametic;
   
   // Preserve sound SFX id,
   //  e.g. for avoiding duplicates of chainsaw.
   channelinfo[channel].id = sfx;
   
   return channel;
}

static void updateSoundParams(int handle, int volume, int seperation, int pitch)
{
   int slot = handle;
   int rightvol;
   int leftvol;
   int step = steptable[pitch];
   
   if(!snd_init)
      return;

#ifdef RANGECHECK
   if(handle>=MAX_CHANNELS)
      I_Error("I_UpdateSoundParams: handle out of range");
#endif
   // Set stepping
   // MWM 2000-12-24: Calculates proportion of channel samplerate
   // to global samplerate for mixing purposes.
   // Patched to shift left *then* divide, to minimize roundoff errors
   // as well as to use SAMPLERATE as defined above, not to assume 11025 Hz
   if(pitched_sounds)
      channelinfo[slot].step = step + (((channelinfo[slot].samplerate<<16)/snd_samplerate)-65536);
   else
      channelinfo[slot].step = ((channelinfo[slot].samplerate<<16)/snd_samplerate);
   
   // Separation, that is, orientation/stereo.
   //  range is: 1 - 256
   seperation += 1;

   // SoM 7/1/02: forceFlipPan accounted for here
   if(forceFlipPan)
      seperation = 257 - seperation;
   
   // Per left/right channel.
   //  x^2 seperation,
   //  adjust volume properly.
   volume *= 8;

   leftvol = volume - ((volume*seperation*seperation) >> 16);
   seperation = seperation - 257;
   rightvol= volume - ((volume*seperation*seperation) >> 16);  

   // Sanity check, clamp volume.
   if(rightvol < 0 || rightvol > 127)
      I_Error("rightvol out of bounds");
   
   if(leftvol < 0 || leftvol > 127)
      I_Error("leftvol out of bounds");
   
   // Get the proper lookup table piece
   //  for this volume level???
   channelinfo[slot].leftvol_lookup = &vol_lookup[leftvol*256];
   channelinfo[slot].rightvol_lookup = &vol_lookup[rightvol*256];
}

//
// SFX API
//

//
// I_UpdateSoundParams
//
// Update the sound parameters. Used to control volume,
// pan, and pitch changes such as when a player turns.
//
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
   if(!snd_init)
      return;

   SDL_LockAudio();
   updateSoundParams(handle, vol, sep, pitch);
   SDL_UnlockAudio();
}

//
// I_SetChannels
//
// Init internal lookups (raw data, mixing buffer, channels).
// This function sets up internal lookups used during
//  the mixing process. 
//
void I_SetChannels(void)
{
   int i;
   int j;
   
   int *steptablemid = steptable + 128;
   
   // Okay, reset internal mixing channels to zero.
   for(i = 0; i < MAX_CHANNELS; i++)
   {
      memset(&channelinfo[i], 0, sizeof(channel_info_t));
   }
   
   // This table provides step widths for pitch parameters.
   // I fail to see that this is currently used.
   for(i=-128 ; i<128 ; i++)
   {
      steptablemid[i] = (int)(pow(1.2, ((double)i/(64.0*snd_samplerate/11025)))*65536.0);
   }
   
   
   // Generates volume lookup tables
   //  which also turn the unsigned samples
   //  into signed samples.
   for(i = 0; i < 128; i++)
   {
      for(j = 0; j < 256; j++)
      {
         // proff - made this a little bit softer, because with
         // full volume the sound clipped badly (191 was 127)
         vol_lookup[i*256+j] = (i*(j-128)*256)/191;
      }
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

// jff 1/21/98 moved music volume down into MUSIC API with the rest

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
   char namebuf[16];

   memset(namebuf, 0, sizeof(namebuf));

   // haleyjd 09/03/03: determine whether to apply DS prefix to
   // name or not using new prefix flag
   if(sfx->prefix)
      psnprintf(namebuf, sizeof(namebuf), "ds%s", sfx->name);
   else
      strcpy(namebuf, sfx->name);

   return W_CheckNumForName(namebuf);
}


//
// I_StartSound
//
int I_StartSound(sfxinfo_t *sound, int cnum, int vol, int sep, int pitch, int pri)
{
   static int handle = -1;
   
   if(!snd_init)
      return 0;

   // SoM: reimplement hardware channel wrap-around
   if(++handle >= MAX_CHANNELS)
      handle = 0;

   SDL_LockAudio();
   
   // haleyjd 09/03/03: this should use handle, NOT cnum, and
   // the return value is plain redundant. Whoever wrote this was
   // out of it.
   addsfx(sound, handle);
      
   updateSoundParams(handle, vol, sep, pitch);
   SDL_UnlockAudio();
   
   return handle;
}

//
// I_StopSound
//
// Stop the sound. Necessary to prevent runaway chainsaw,
// and to stop rocket launches when an explosion occurs.
//
void I_StopSound(int handle)
{
   if(!snd_init)
      return;

#ifdef RANGECHECK
   if(handle >= MAX_CHANNELS)
      I_Error("I_StopSound: handle out of range");
#endif
   
   SDL_LockAudio();
   stopchan(handle);
   SDL_UnlockAudio();
}

//
// I_SoundIsPlaying
//
// haleyjd: wow, this can actually do something in the Windows
// version :P
//
int I_SoundIsPlaying(int handle)
{
   if(!snd_init)
      return false;

#ifdef RANGECHECK
   if(handle >= MAX_CHANNELS)
      I_Error("I_SoundIsPlaying: handle out of range");
#endif
   return (channelinfo[handle].data != NULL);
}

//
// I_UpdateSound
//
// haleyjd: this version does nothing with respect to digital sound
// in Windows; sound-updating is done via the SDL callback function 
// below
// 
void I_UpdateSound(void)
{
}

static void I_SDLUpdateSound(void *userdata, Uint8 *stream, int len)
{
   // Mix current sound data.
   // Data, from raw sound, for right and left.
   register unsigned char sample;
   register int dl;
   register int dr;
   
   // Pointers in audio stream, left, right, end.
   short *leftout;
   short *rightout;
   short *leftend;

   // Step in stream, left and right, thus two.
   int step;
   
   // Mixing channel index.
   int chan;
   
   // Left and right channel
   //  are in audio stream, alternating.
   leftout  = (signed short *)stream;
   rightout = ((signed short *)stream)+1;
   step = 2;
   
   // Determine end, for left channel only
   //  (right channel is implicit).
   leftend = leftout + (len / 4) * step;
   
   // Mix sounds into the mixing buffer.
   // Loop over step*SAMPLECOUNT,
   //  that is 512 values for two channels.
   while(leftout != leftend)
   {
      // Reset left/right value. 
      dl = *leftout;
      dr = *rightout;
      
      // Love thy L2 cache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      for(chan = 0; chan < MAX_CHANNELS; chan++ )
      {
         // Check channel, if active.
         if(channelinfo[chan].data)
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
            if(channelinfo[chan].data >= channelinfo[chan].enddata)
               stopchan(chan);
         }
      }
      
      // Clamp to range. Left hardware channel.
      if(dl > SHRT_MAX)
      {
         *leftout = SHRT_MAX;
      }
      else if(dl < SHRT_MIN)
      {
         *leftout = SHRT_MIN;
      }
      else
      {
         *leftout = (short)dl;
      }
      
      // Same for right hardware channel.
      if(dr > SHRT_MAX)
      {
         *rightout = SHRT_MAX;
      }
      else if(dr < SHRT_MIN)
      {
         *rightout = SHRT_MIN;
      }
      else
      {
         *rightout = (short)dr;
      }
      
      // Increment current pointers in stream
      leftout += step;
      rightout += step;
   }
}

// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.

void I_SubmitSound(void)
{
   // haleyjd: whatever it is, SDL doesn't need it either
}

void I_ShutdownSound(void)
{
   if(snd_init)
   {
      Mix_CloseAudio();
      snd_init = 0;
   }
}

//
// haleyjd 11/05/03: fixed for SDL sound engine
//
void I_CacheSound(sfxinfo_t *sound)
{
   if(sound->data)
      return;     // already cached
   
   if(sound->link)
      I_CacheSound(sound->link);
   else
   {
      int lump = I_GetSfxLumpNum(sound);
 
      // replace missing sounds with a reasonable default
      if(lump == -1)
         lump = W_GetNumForName(gameModeInfo->defSoundName);

      sound->data = W_CacheLumpNum(lump, PU_STATIC);
   }
}

// SoM 9/14/02: Rewrite. code taken from prboom to use SDL_Mixer

void I_InitSound(void)
{   
   if(!nosfxparm)
   {
      int audio_buffers;

      puts("I_InitSound: ");

      /* Initialize variables */
      audio_buffers = SAMPLECOUNT * snd_samplerate / 11025;

      // haleyjd: the docs say we should do this
      if(SDL_InitSubSystem(SDL_INIT_AUDIO))
      {
         printf("Couldn't initialize SDL audio.\n");
         snd_card = 0;
         mus_card = 0;
         return;
      }
  
      if(Mix_OpenAudio(snd_samplerate, MIX_DEFAULT_FORMAT, 2, audio_buffers) < 0)
      {
         printf("Couldn't open audio with desired format.\n");
         snd_card = 0;
         mus_card = 0;
         return;
      }

      SAMPLECOUNT = audio_buffers;
      Mix_SetPostMix(I_SDLUpdateSound, NULL);
      printf("Configured audio device with %d samples/slice.\n", SAMPLECOUNT);

      atexit(I_ShutdownSound);

      snd_init = true;

      // haleyjd 04/11/03: don't use music if sfx aren't init'd
      // (may be dependent, docs are unclear)
      if(!nomusicparm)
         I_InitMusic();
   }   
}

//
// MUSIC API.
//

#include "mmus2mid.h"
#include "../m_misc.h"

static Mix_Music *music[2] = { NULL, NULL };

const char *music_name = "eetemp.mid";

void I_ShutdownMusic(void)
{
   I_StopSong(0);
}

void I_InitMusic(void)
{
   switch(mus_card)
   {
   case -1:
      printf("I_InitMusic: Using SDL_mixer.\n");
      mus_init = true;
      break;   
   default:
      printf("I_InitMusic: Using No MIDI Device.\n");
      break;
   }
   
   atexit(I_ShutdownMusic);
}

void I_PlaySong(int handle, int looping)
{
   if(!mus_init)
      return;

   if(handle >= 0 && music[handle])
   {
      if(Mix_PlayMusic(music[handle], looping ? -1 : 0) == -1)
         I_Error("I_PlaySong: please report this error\n");
   }
}

void I_SetMusicVolume(int volume)
{
   if(!mus_init)
      return;

   Mix_VolumeMusic(volume*8);
}

boolean mus_paused = false;

// haleyjd: SDL doesn't properly support native MIDI pausing 
// (yet anyways), so don't try it.

void I_PauseSong(int handle)
{
/*
   if(!mus_init || !paused)
      return;

   if(!mus_paused)
   {
      Mix_PauseMusic();
      mus_paused = true;
   }
*/
}

void I_ResumeSong(int handle)
{
/*
   if(!mus_init || paused)
      return;

   if(mus_paused)
   {
      Mix_ResumeMusic();
      mus_paused = false;
   }
*/
}

void I_StopSong(int handle)
{
   if(!mus_init)
      return;
   
   Mix_HaltMusic();
}

void I_UnRegisterSong(int handle)
{
   if(!mus_init)
      return;

   if(handle >= 0 && music[handle])
   {
      Mix_FreeMusic(music[handle]);
      music[handle] = NULL;
   }
}

// jff 1/16/98 created to convert data to MIDI ala Allegro

int I_RegisterSong(void *data)
{
   int err;
   MIDI mididata;
   char fullMusicName[PATH_MAX + 1];

   UBYTE *mid;
   int midlen;

   music[0] = NULL; // ensure its null

   // haleyjd: don't return negative music handles
   if(!mus_init)
      return 0;

   memset(&mididata,0,sizeof(MIDI));

   if((err = MidiToMIDI((byte *)data, &mididata)) &&    // try midi first
      (err = mmus2mid((byte *)data, &mididata, 89, 0))) // now try mus      
   {
      doom_printf(FC_ERROR"Error loading midi: %d", err);
      // haleyjd: ???
      // free(data);
      return 0;
   }

   MIDIToMidi(&mididata,&mid,&midlen);

   // haleyjd 03/15/03: fixed for -cdrom
   if(M_CheckParm("-cdrom"))
      psnprintf(fullMusicName, sizeof(fullMusicName),
                "%s/%s", "c:/doomdata", music_name);
   else
      psnprintf(fullMusicName, sizeof(fullMusicName),
                "%s/%s", D_DoomExeDir(), music_name);
   
   if(!M_WriteFile(fullMusicName, mid, midlen))
   {
      doom_printf(FC_ERROR"Error writing music to %s", music_name);
      free(mid);      
      return 0;
   }

   free(mid);

   music[0] = Mix_LoadMUS(fullMusicName);
   
   if(!music[0])
   {
      doom_printf(FC_ERROR"Couldn't load MIDI from %s: %s\n", 
                  fullMusicName, 
                  Mix_GetError());
   }
   
   return 0;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
   // haleyjd: this is never called
   return 0;
}

/************************
        CONSOLE COMMANDS
 ************************/

// system specific sound console commands

char *sndcardstr[] = {"SDL mixer", "none"};
char *muscardstr[] = {"SDL mixer", "none"};

VARIABLE_INT(snd_card, NULL,           -1, 0, sndcardstr);
VARIABLE_INT(mus_card, NULL,           -1, 0, muscardstr);
VARIABLE_INT(detect_voices, NULL,       0, 1, yesno);

CONSOLE_VARIABLE(snd_card, snd_card, 0) 
{
   if(!snd_init && menuactive)
      MN_ErrorMsg("you must restart the program to turn on sound");
}

CONSOLE_VARIABLE(mus_card, mus_card, 0)
{
   if(!mus_init && menuactive)
      MN_ErrorMsg("you must restart the program to turn on music");

   if(mus_card == 0)
      S_StopMusic();
}

CONSOLE_VARIABLE(detect_voices, detect_voices, 0) {}

void I_Sound_AddCommands(void)
{
   C_AddCommand(snd_card);
   C_AddCommand(mus_card);
   C_AddCommand(detect_voices);
}


//----------------------------------------------------------------------------
//
// $Log: i_sound.c,v $
//
//----------------------------------------------------------------------------
