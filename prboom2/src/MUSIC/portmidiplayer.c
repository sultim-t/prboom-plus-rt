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

// TODO: some duplicated code with this and the fluidplayer should be
// split off or something

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicplayer.h"

#ifndef HAVE_LIBPORTMIDI
#include <string.h>

static const char *pm_name (void)
{
  return "portmidi midi player (DISABLED)";
}


static int pm_init (int samplerate)
{
  return 0;
}

const music_player_t pm_player =
{
  pm_name,
  pm_init,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

#else // HAVE_LIBPORTMIDI

#include <portmidi.h>
#include <porttime.h>
#include <stdlib.h>
#include <string.h>
#include "lprintf.h"
#include "midifile.h"

static midi_event_t **events;
static int eventpos;
static midi_file_t *midifile;

static int pm_playing;
static int pm_paused;
static int pm_looping;
static int pm_volume;
static double spmc;
static double pm_delta;

static unsigned long trackstart;

static PortMidiStream *pm_stream;

#define SYSEX_BUFF_SIZE 1024
static unsigned char sysexbuff[SYSEX_BUFF_SIZE];
static int sysexbufflen;

// latency: we're generally writing timestamps slightly in the past (from when the last time
// render was called to this time.  portmidi latency instruction must be larger than that window
// so the messages appear in the future.  ~46-47ms is the nominal length if i_sound.c gets its way
#define DRIVER_LATENCY 80 // ms
// driver event buffer needs to be big enough to hold however many events occur in latency time
#define DRIVER_BUFFER 100 // events



static const char *pm_name (void)
{
  return "portmidi midi player";
}



#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>
#endif


static int pm_init (int samplerate)
{
  PmDeviceID outputdevice;
  const PmDeviceInfo *oinfo;

  TESTDLLLOAD ("portmidi.dll", TRUE)

  if (Pm_Initialize () != pmNoError)
  {
    lprintf (LO_WARN, "portmidiplayer: Pm_Initialize () failed\n");
    return 0;
  }

  outputdevice = Pm_GetDefaultOutputDeviceID ();

  if (outputdevice == pmNoDevice)
  {
    lprintf (LO_WARN, "portmidiplayer: No output devices available\n");
    Pm_Terminate ();
    return 0;
  }

  oinfo = Pm_GetDeviceInfo (outputdevice);

  lprintf (LO_INFO, "portmidiplayer: Opening device %s:%s for output\n", oinfo->interf, oinfo->name);

  if (Pm_OpenOutput(&pm_stream, outputdevice, NULL, DRIVER_BUFFER, NULL, NULL, DRIVER_LATENCY) != pmNoError)
  {
    lprintf (LO_WARN, "portmidiplayer: Pm_OpenOutput () failed\n");
    Pm_Terminate ();
    return 0;
  }

  return 1;
}

static void pm_shutdown (void)
{
  Pm_Close (pm_stream);
  Pm_Terminate ();
}





static const void *pm_registersong (const void *data, unsigned len)
{
  midimem_t mf;

  mf.len = len;
  mf.pos = 0;
  mf.data = (void *) data;

  midifile = MIDI_LoadFile (&mf);

  if (!midifile)
  {
    lprintf (LO_WARN, "pm_registersong: Failed to load MIDI.\n");
    return NULL;
  }
  
  events = MIDI_GenerateFlatList (midifile);
  if (!events)
  {
    MIDI_FreeFile (midifile);
    return NULL;
  }
  eventpos = 0;

  // implicit 120BPM (this is correct to spec)
  //spmc = compute_spmc (MIDI_GetFileTimeDivision (midifile), 500000, 1000);
  spmc = MIDI_spmc (midifile, NULL, 1000);

  // handle not used
  return data;
}

static void pm_unregistersong (void *handle)
{
  if (events)
  {
    MIDI_DestroyFlatList (events);
    events = NULL;
  }
  if (midifile)
  {
    MIDI_FreeFile (midifile);
    midifile = NULL;
  }
}

static void pm_pause (void)
{
  pm_paused = 1;
}
static void pm_resume (void)
{
  pm_paused = 0;
  trackstart = Pt_Time ();
}
static void pm_play (void *handle, int looping)
{
  eventpos = 0;
  pm_looping = looping;
  pm_playing = 1;
  //pm_paused = 0;
  pm_delta = 0.0;
  trackstart = Pt_Time ();

}

static void writeevent (int when, int eve, int channel, int v1, int v2)
{
  PmMessage m;

  m = Pm_Message (eve | channel, v1, v2);
  Pm_WriteShort (pm_stream, when, m);
}


static void writesysex (unsigned long when, int etype, unsigned char *data, int len)
{
  // sysex code is untested
  // it's possible to use an auto-resizing buffer here, but a malformed
  // midi file could make it grow arbitrarily large (since it must grow
  // until it hits an 0xf7 terminator)
  if (len + sysexbufflen > SYSEX_BUFF_SIZE)
  {
    lprintf (LO_WARN, "portmidiplayer: ignoring large or malformed sysex message\n");
    sysexbufflen = 0;
    return;
  }
  memcpy (sysexbuff + sysexbufflen, data, len);
  sysexbufflen += len;
  if (sysexbuff[sysexbufflen - 1] == 0xf7) // terminator
  {
    Pm_WriteSysEx (pm_stream, when, sysexbuff);
    sysexbufflen = 0;
  }
}  

static void pm_stop (void)
{
  int i;
  unsigned long when = Pt_Time ();
  pm_playing = 0;
  

  // songs can be stopped at any time, so reset everything
  for (i = 0; i < 16; i++)
  {
    writeevent (when, MIDI_EVENT_CONTROLLER, i, 123, 0); // all notes off
    writeevent (when, MIDI_EVENT_CONTROLLER, i, 121, 0); // reset all parameters
  }
  // abort any partial sysex
  sysexbufflen = 0;
}


void I_midiOutSetVolumes (int volume); // from e6y.h

static void pm_setvolume (int v)
{ // portmidi has no "overall volume" control.  you can get volume control
  // through various controller messages, but those can all be overwritten
  // by the midi stream you're playing at any time.

  pm_volume = v;
  
  // this is a bit of a hack
  // fix: add non-win32 version
  // fix: change win32 version to only modify the device we're using?
  // (portmidi could know what device it's using, but the numbers
  //  don't match up with the winapi numbers...)
  I_midiOutSetVolumes (pm_volume);
}

static void pm_render (void *vdest, unsigned bufflen)
{
  // wherever you see samples in here, think milliseconds
  
  unsigned long newtime = Pt_Time ();
  unsigned long length = newtime - trackstart;

  //timerpos = newtime;
  unsigned long when;

  midi_event_t *currevent;
  
  unsigned sampleswritten = 0;
  unsigned samples;

  memset (vdest, 0, bufflen * 4);



  if (!pm_playing || pm_paused)
    return;

  
  while (1)
  {
    double eventdelta;
    currevent = events[eventpos];
    
    // how many samples away event is
    eventdelta = currevent->delta_time * spmc;


    // how many we will render (rounding down); include delta offset
    samples = (unsigned) (eventdelta + pm_delta);


    if (samples + sampleswritten > length)
    { // overshoot; render some samples without processing an event
      break;
    }


    sampleswritten += samples;
    pm_delta -= samples;
 
    
    // process event
    when = trackstart + sampleswritten;
    switch (currevent->event_type)
    {
      case MIDI_EVENT_SYSEX:
      case MIDI_EVENT_SYSEX_SPLIT:        
        writesysex (when, currevent->event_type, currevent->data.sysex.data, currevent->data.sysex.length);
        break;
      case MIDI_EVENT_META: // tempo is the only meta message we're interested in
        if (currevent->data.meta.type == MIDI_META_SET_TEMPO)
          spmc = MIDI_spmc (midifile, currevent, 1000);
        else if (currevent->data.meta.type == MIDI_META_END_OF_TRACK)
        {
          if (pm_looping)
          {
            eventpos = 0;
            pm_delta += eventdelta;
            continue;
          }
          // stop
          pm_stop ();
          return;
        }
        break; // not interested in most metas
      default:
        writeevent (when, currevent->event_type, currevent->data.channel.channel, currevent->data.channel.param1, currevent->data.channel.param2);
        break;
      
    }
    // event processed so advance midiclock
    pm_delta += eventdelta;
    eventpos++;

  }

  if (samples + sampleswritten > length)
  { // broke due to next event being past the end of current render buffer
    // finish buffer, return
    samples = length - sampleswritten;
    pm_delta -= samples; // save offset
  }

  trackstart = newtime;
}  


const music_player_t pm_player =
{
  pm_name,
  pm_init,
  pm_shutdown,
  pm_setvolume,
  pm_pause,
  pm_resume,
  pm_registersong,
  pm_unregistersong,
  pm_play,
  pm_stop,
  pm_render
};


#endif // HAVE_LIBPORTMIDI

