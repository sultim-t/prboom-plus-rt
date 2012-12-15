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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lprintf.h"
#include "midifile.h"
#include "i_sound.h" // for snd_mididev

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
  int i;
  char devname[64];

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

  // look for a device that matches the user preference

  lprintf (LO_INFO, "portmidiplayer device list:\n");
  for (i = 0; i < Pm_CountDevices (); i++)
  {
    oinfo = Pm_GetDeviceInfo (i);
    if (!oinfo || !oinfo->output)
      continue;
    doom_snprintf (devname, 64, "%s:%s", oinfo->interf, oinfo->name);
    if (strlen (snd_mididev) && strstr (devname, snd_mididev))
    {
      outputdevice = i;
      lprintf (LO_INFO, ">>%s\n", devname);
    }
    else
    {
      lprintf (LO_INFO, "  %s\n", devname);
    }
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
  if (pm_stream)
  {
    /* ugly deadlock in portmidi win32 implementation:

    main thread gets stuck in Pm_Close
    midi thread (started by windows) gets stuck in winmm_streamout_callback

    winapi ref says:
    "Applications should not call any multimedia functions from inside the callback function,
     as doing so can cause a deadlock. Other system functions can safely be called from the callback."
    
    winmm_streamout_callback calls midiOutUnprepareHeader.  oops?

    
    since timestamps are slightly in the future, it's very possible to have some messages still in
    the windows midi queue when Pm_Close is called.  this is normally no problem, but if one so happens
    to dequeue and call winmm_streamout_callback at the exact right moment...

    fix: at this point, we've stopped generating midi messages.  sleep for more than DRIVER_LATENCY to ensure
    all messages are flushed.
    
    not a fix: calling Pm_Abort(); then midiStreamStop deadlocks instead of midiStreamClose. 
    */
    #ifdef _WIN32
    Pt_Sleep (DRIVER_LATENCY * 2);
    #endif

    Pm_Close (pm_stream);
    Pm_Terminate ();
    pm_stream = NULL;
  }
}





static const void *pm_registersong (const void *data, unsigned len)
{
  midimem_t mf;

  mf.len = len;
  mf.pos = 0;
  mf.data = data;

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

static void writeevent (unsigned long when, int eve, int channel, int v1, int v2)
{
  PmMessage m;

  m = Pm_Message (eve | channel, v1, v2);
  Pm_WriteShort (pm_stream, when, m);
}

/*
portmidi has no overall volume control.  we have two options:
1. use a win32-specific hack (only if mus_extend_volume is set)
2. monitor the controller volume events and tweak them to serve our purpose
*/

#ifdef _WIN32
extern int mus_extend_volume; // from e6y.h
void I_midiOutSetVolumes (int volume); // from e6y.h
#endif

static int channelvol[16];

static void pm_setchvolume (int ch, int v, unsigned long when)
{
  channelvol[ch] = v;
  writeevent (when, MIDI_EVENT_CONTROLLER, ch, 7, channelvol[ch] * pm_volume / 15);
}

static void pm_refreshvolume (void)
{
  int i;
  unsigned long when = Pt_Time ();

  for (i = 0; i < 16; i ++)
    writeevent (when, MIDI_EVENT_CONTROLLER, i, 7, channelvol[i] * pm_volume / 15);
}

static void pm_clearchvolume (void)
{
  int i;
  for (i = 0; i < 16; i++)
    channelvol[i] = 127; // default: max

}

static void pm_setvolume (int v)
{ 
  static int firsttime = 1;

  if (pm_volume == v && !firsttime)
    return;
  firsttime = 0;

  pm_volume = v;
  
  // this is a bit of a hack
  // fix: add non-win32 version
  // fix: change win32 version to only modify the device we're using?
  // (portmidi could know what device it's using, but the numbers
  //  don't match up with the winapi numbers...)

  #ifdef _WIN32
  if (mus_extend_volume)
    I_midiOutSetVolumes (pm_volume);
  else
  #endif
    pm_refreshvolume ();
}


static void pm_unregistersong (const void *handle)
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
  int i;
  unsigned long when = Pt_Time ();
  pm_paused = 1;
  for (i = 0; i < 16; i++)
  {
    writeevent (when, MIDI_EVENT_CONTROLLER, i, 123, 0); // all notes off
  }
}
static void pm_resume (void)
{
  pm_paused = 0;
  trackstart = Pt_Time ();
}
static void pm_play (const void *handle, int looping)
{
  eventpos = 0;
  pm_looping = looping;
  pm_playing = 1;
  //pm_paused = 0;
  pm_delta = 0.0;
  pm_clearchvolume ();
  pm_refreshvolume ();
  trackstart = Pt_Time ();
  
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
            int i;
            eventpos = 0;
            pm_delta += eventdelta;
            // fix buggy songs that forget to terminate notes held over loop point
            // sdl_mixer does this as well
            for (i = 0; i < 16; i++)
              writeevent (when, MIDI_EVENT_CONTROLLER, i, 123, 0); // all notes off
            continue;
          }
          // stop
          pm_stop ();
          return;
        }
        break; // not interested in most metas
      case MIDI_EVENT_CONTROLLER:
        if (currevent->data.channel.param1 == 7)
        { // volume event
          #ifdef _WIN32
          if (!mus_extend_volume)
          #endif
          {
            pm_setchvolume (currevent->data.channel.channel, currevent->data.channel.param2, when);
            break;
          }
        } // fall through
      default:
        writeevent (when, currevent->event_type, currevent->data.channel.channel, currevent->data.channel.param1, currevent->data.channel.param2);
        break;
      
    }
    // if the event was a "reset all controllers", we need to additionally re-fix the volume (which itself was reset)
    if (currevent->event_type == MIDI_EVENT_CONTROLLER && currevent->data.channel.param1 == 121)
      pm_setchvolume (currevent->data.channel.channel, 127, when);

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

