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

// Julian: 6/6/2001: replaced by local inclusion
#include "allegro.h"

#include "mmus2mid.h"   //jff 1/16/98 declarations for MUS->MIDI converter

#include "../z_zone.h"
#include "../c_runcmd.h"
#include "../doomstat.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../w_wad.h"
#include "../g_game.h"     //jff 1/21/98 added to use dprintf in I_RegisterSong
#include "../d_main.h"
#include "../v_misc.h"
#include "../d_gi.h"

void I_CacheSound(sfxinfo_t *sound);

// Needed for calling the actual sound output.
#define SAMPLECOUNT             512

// Factor volume is increased before sending to allegro
#define VOLSCALE                16

        // sf: adjust temp when changing gamespeed
extern int realtic_clock_rate;

int snd_card;   // default.cfg variables for digi and midi drives
int mus_card;   // jff 1/18/98

        // sf: default_snd_card and default_mus_card removed (purpose?)

int detect_voices; //jff 3/4/98 enables voice detection prior to install_sound
//jff 1/22/98 make these visible here to disable sound/music on install err

static SAMPLE *raw2SAMPLE(unsigned char *rawdata, int len)
{
  SAMPLE *spl = malloc(sizeof(SAMPLE));
  spl->bits = 8;
  // killough 1/22/98: Get correct frequency
  spl->freq = (rawdata[3]<<8)+rawdata[2];
  spl->len = len;
  spl->priority = 255;
  spl->loop_start = 0;
  spl->loop_end = len;
  spl->param = -1;
  spl->data = rawdata + 8;
  _go32_dpmi_lock_data(rawdata+8, len);   // killough 3/8/98: lock sound data
  return spl;
}

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
static void *getsfx(sfxinfo_t *sfxinfo)
{
  unsigned char *sfx, *paddedsfx;
  int  i;
  int  size;
  int  paddedsize;
  int  sfxlump;

  // Get the sound data from the WAD, allocate lump
  //  in zone memory.

  // haleyjd 10/10/02: rewrote to use a sfxinfo_t *
  sfxlump = I_GetSfxLumpNum(sfxinfo);

  // Now, there is a severe problem with the
  //  sound handling, in it is not (yet/anymore)
  //  gamemode aware. That means, sounds from
  //  DOOM II will be requested even with DOOM
  //  shareware.
  // The sound list is wired into sounds.c,
  //  which sets the external variable.
  // I do not do runtime patches to that
  //  variable. Instead, we will use a
  //  default sound for replacement.

  if(sfxlump == -1)
  {
     if(gameModeInfo->flags & GIF_HERETIC)
        sfxlump = W_GetNumForName("gldhit");
     else
        sfxlump = W_GetNumForName("dspistol");
  }

  size = W_LumpLength(sfxlump);
  
  sfx = W_CacheLumpNum(sfxlump, PU_STATIC);

  // Pads the sound effect out to the mixing buffer size.
  // The original realloc would interfere with zone memory.
  paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

  // Allocate from zone memory.
  paddedsfx = (unsigned char*) Z_Malloc(paddedsize+8, PU_STATIC, 0);

  // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
  // This should interfere with zone memory handling,
  //  which does not kick in in the soundserver.

  // Now copy and pad.
  memcpy(paddedsfx, sfx, size);
  for (i=size; i<paddedsize+8; i++)
    paddedsfx[i] = 128;

  // Remove the cached lump.
  Z_Free(sfx);

  // Preserve padded length.
  sfxinfo->length = paddedsize;

  // Return allocated padded data.
  return raw2SAMPLE(paddedsfx,paddedsize);  // killough 1/22/98: pass all data
}

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
  // no-op.
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
   
   // haleyjd 09/03/03: determine whether to add DS prefix to
   // name or not using new prefix flag
   if(sfx->prefix)
      psnprintf(namebuf, sizeof(namebuf), "ds%s", sfx->name);
   else
      strcpy(namebuf, sfx->name);
   
   return W_CheckNumForName(namebuf);
}

// Almost all of the sound code from this point on was
// rewritten by Lee Killough, based on Chi's rough initial
// version.

// killough 2/21/98: optionally use varying pitched sounds

#define PITCH(x) (pitched_sounds ? ((x)*1000)/128 : 1000)

// This is the number of active sounds that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to some ridiculously large number, to avoid
// any chances that these routines will stop the sounds.
// killough

#define NUM_CHANNELS 256

// "Channels" used to buffer requests. Distinct SAMPLEs
// must be used for each active sound, or else clipping
// will occur.

static SAMPLE channel[NUM_CHANNELS];

// This function adds a sound to the list of currently
// active sounds, which is maintained as a given number
// of internal channels. Returns a handle.

int I_StartSound(sfxinfo_t *sound, int cnum, int vol, int sep, int pitch, int pri)
{
  static int handle;
  int correctSep;

  // move up one slot, with wraparound
  if (++handle >= NUM_CHANNELS)
    handle = 0;

  // destroy anything still in the slot
  stop_sample(&channel[handle]);

  if(!sound->data) I_CacheSound(sound);

  // Copy the sound's data into the sound sample slot
  memcpy(&channel[handle], sound->data, sizeof(SAMPLE));

  // haleyjd 12/08/01: range-check panning value and possibly
  // force a swap from left to right, right to left based on
  // a user variable
  if(sep < 1) 
     sep = 1;
  else if(sep > 256) 
     sep = 256;

  correctSep = forceFlipPan ? sep - 1 : 256 - sep;
  
  // Start the sound
  play_sample(&channel[handle],vol*VOLSCALE+VOLSCALE-1,correctSep,
          PITCH(pitch),0);

  // Reference for s_sound.c to use when calling functions below
  return handle;
}

// Stop the sound. Necessary to prevent runaway chainsaw,
// and to stop rocket launches when an explosion occurs.

void I_StopSound (int handle)
{
  stop_sample(channel+handle);
}

// Update the sound parameters. Used to control volume,
// pan, and pitch changes such as when a player turns.

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
   int correctSep;

   // haleyjd: range check panning value and force a flip pan
   // based on user-specified value
   if(sep < 1)
      sep = 1;
   else if(sep > 256)
      sep = 256;

   correctSep = forceFlipPan ? sep - 1 : 256 - sep;

   adjust_sample(&channel[handle], vol*VOLSCALE+VOLSCALE-1,
                 correctSep, PITCH(pitch), 0);
}

// We can pretend that any sound that we've associated a handle
// with is always playing.

int I_SoundIsPlaying(int handle)
{
  return 1;
}

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
//  allegro does this now

void I_UpdateSound( void )
{
}

// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.

void I_SubmitSound(void)
{
  //this should no longer be necessary because
  //allegro is doing all the sound mixing now
}

void I_ShutdownSound(void)
{
  remove_sound();
}

// sf: dynamic sound resource loading
void I_CacheSound(sfxinfo_t *sound)
{
  if(sound->data) return;     // already cached
  
  // sf: changed
  if(sound->link)
    I_CacheSound(sound->link);
  else
    sound->data = getsfx(sound);
}

void I_InitSound(void)
{
  // Secure and configure sound device first.
  
  if (detect_voices && snd_card>=0 && mus_card>=0)
    {
      int mv;                          //jff 3/3/98 try it according to Allegro
      int dv = detect_digi_driver(snd_card); // detect the digital sound driver
      if (dv == 0)
        snd_card = 0;
      mv = detect_midi_driver(mus_card);     // detect the midi driver
      if (mv == -1)
        dv = mv = dv/2;          //note stealing driver, uses digital voices
      if (mv == 0xffff)
        mv = -1;               //extern MPU-401 - unknown use default voices
      reserve_voices(dv,mv); // reserve the number of voices detected
    }                                  //jff 3/3/98 end of sound init changes


  //jff 1/18/98 autodect MIDI
  
  if (install_sound(snd_card, mus_card, "none") == -1)
    {
      usermsg("\tSound init error: %s", allegro_error); // killough 8/8/98
      //jff 1/22/98 on error, disable sound this invocation
      //in future - nice to detect if either sound or music might be ok
      nosfxparm = true;
      nomusicparm = true;
      //jff end disable sound this invocation
    }
  else //jff 1/22/98 don't register I_ShutdownSound if errored
    {
      // killough 2/7/98: prevent VM swapping of sfx
      LOCK_VARIABLE(channel); 
      atexit(I_ShutdownSound); // killough
    }

  // Finished initialization.
}


///
// MUSIC API.
//

// This is the number of active musics that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to 1 to allow just one music at a time for now.

#define NUM_MIDICHAN 1

// mididata is used to buffer the current music.

static MIDI mididata;

void I_ShutdownMusic(void)
{
  stop_midi();          //jff 1/16/98 shut down midi
}

void I_InitMusic(void)
{
  atexit(I_ShutdownMusic); //jff 1/16/98 enable atexit routine for shutdown
}

// jff 1/18/98 changed interface to make mididata destroyable

void I_PlaySong(int handle, int looping)
{
  if (handle>=0)
    play_midi(&mididata,looping);       // start registered midi playing
}

void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.

  //jff 01/17/98 - add VOLSCALE-1 to get most out of volume
  set_volume(-1,snd_MusicVolume*VOLSCALE+VOLSCALE-1);   // jff 1/18/98
}

void I_PauseSong (int handle)
{
  if (handle>=0)
    midi_pause();       // jff 1/16/98 pause midi playing
}

void I_ResumeSong (int handle)
{
  if (handle>=0)
    midi_resume();      // jff 1/16/98 resume midi playing
}

void I_StopSong(int handle)
{
  if (handle>=0)
    stop_midi();        // jff 1/16/98 stop midi playing
}

void I_UnRegisterSong(int handle)
{
}

// jff 1/16/98 created to convert data to MIDI ala Allegro

int I_RegisterSong(void *data)
{
  int handle, err;

  //jff 1/21/98 just stop any midi currently playing
  stop_midi();

  // convert the MUS lump data to a MIDI structure
  //jff 1/17/98 make divisions 89, compression allowed
  if    //jff 02/08/98 add native midi support
    (
     (err=MidiToMIDI(data, &mididata)) &&       // try midi first
              // now try mus    sf: change tempo with gamespeed
     (err=mmus2mid(data, &mididata, 89*realtic_clock_rate/100, 1))
     )
    {
      handle=-1;
      doom_printf(FC_ERROR"Error loading midi: %d",err);
    }
  else
    {
      handle=0;
      lock_midi(&mididata);     // data must be locked for Allegro
    }
  //jff 02/08/98 add native midi support:
  return handle;                        // 0 if successful, -1 otherwise
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  return 0;
}

/************************
        CONSOLE COMMANDS
 ************************/

// system specific sound console commands

char *sndcardstr[] =
   {"autodetect","none", "SB", "SB 1.0", "SB 1.5",
     "SB 2.0", "SB Pro", "SB16", "GUS"};
char *muscardstr[] =
   {"autodetect","none", "adlib", "OPL2", "2xOPL2",
   "OPL3", "SB MIDI", "MPU-401", "GUS","DIGMID", "AWE32"};

VARIABLE_INT(snd_card, NULL,            -1, 7, sndcardstr);
VARIABLE_INT(mus_card, NULL,            -1, 9, muscardstr);
VARIABLE_INT(detect_voices, NULL,       0, 1, yesno);

CONSOLE_VARIABLE(snd_card, snd_card, 0) {}
CONSOLE_VARIABLE(mus_card, mus_card, 0) {}
CONSOLE_VARIABLE(detect_voices, detect_voices, 0) {}

void I_Sound_AddCommands()
{
  C_AddCommand(snd_card);
  C_AddCommand(mus_card);
  C_AddCommand(detect_voices);
}


//----------------------------------------------------------------------------
//
// $Log: i_sound.c,v $
// Revision 1.15  1998/05/03  22:32:33  killough
// beautification, use new headers/decls
//
// Revision 1.14  1998/03/09  07:11:29  killough
// Lock sound sample data
//
// Revision 1.13  1998/03/05  00:58:46  jim
// fixed autodetect not allowed in allegro detect routines
//
// Revision 1.12  1998/03/04  11:51:37  jim
// Detect voices in sound init
//
// Revision 1.11  1998/03/02  11:30:09  killough
// Make missing sound lumps non-fatal
//
// Revision 1.10  1998/02/23  04:26:44  killough
// Add variable pitched sound support
//
// Revision 1.9  1998/02/09  02:59:51  killough
// Add sound sample locks
//
// Revision 1.8  1998/02/08  15:15:51  jim
// Added native midi support
//
// Revision 1.7  1998/01/26  19:23:27  phares
// First rev with no ^Ms
//
// Revision 1.6  1998/01/23  02:43:07  jim
// Fixed failure to not register I_ShutdownSound with atexit on install_sound error
//
// Revision 1.4  1998/01/23  00:29:12  killough
// Fix SSG reload by using frequency stored in lump
//
// Revision 1.3  1998/01/22  05:55:12  killough
// Removed dead past changes, changed destroy_sample to stop_sample
//
// Revision 1.2  1998/01/21  16:56:18  jim
// Music fixed, defaults for cards added
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
