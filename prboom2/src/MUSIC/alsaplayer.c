/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *
 *  Copyright
 *  (C) 2011 by Nicholai Main
 *  (C) 2021 by Gustavo Rehermann
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

// TODO: some duplicated code with this and the fluidsynth and portmidi
// players should be split off or something

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicplayer.h"

#ifndef HAVE_ALSA
#include <string.h>

static const char *alsa_name (void)
{
  return "alsa midi player (DISABLED)";
}


static int alsa_init (int samplerate)
{
  return 0;
}

const music_player_t alsa_player =
{
  alsa_name,
  alsa_init,
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

#else // HAVE_ALSA

#include "alsaplayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lprintf.h"
#include "midifile.h"
#include "i_sound.h" // for snd_mididev

#define CHK_RET(stmt, msg) if((stmt) < 0) { return (msg); }
#define CHK_LPRINT(stmt, ltype, ...) if((stmt) < 0) { lprintf(ltype, __VA_ARGS__); }
#define CHK_LPRINT_ERR(stmt, ltype, ...) { alsaplayer_err = (stmt); if(alsaplayer_err < 0) { lprintf(ltype, __VA_ARGS__, snd_strerror(alsaplayer_err)); } }
#define CHK_LPRINT_RET(stmt, ret, ltype, ...) if((stmt) < 0) { lprintf(ltype, __VA_ARGS__); return (ret); }
#define CHK_LPRINT_ERR_RET(stmt, ret, ltype, ...) { alsaplayer_err = (stmt); if(alsaplayer_err < 0) { lprintf(ltype, __VA_ARGS__, snd_strerror(alsaplayer_err)); return ret; } }

static midi_event_t **events;
static int eventpos;
static midi_file_t *midifile;

static int alsa_playing;
static int alsa_paused;
static int alsa_looping;
static int alsa_volume;
static int alsa_open = 0;

// if set to 0, can auto-connect to default port
static int alsa_first_connected = 0;

static double spmc;
static double alsa_delta;

static unsigned long trackstart;

static snd_seq_t *seq_handle = NULL;
snd_seq_event_t seq_ev;
static int out_id;
static int out_port;
static int out_queue;

#define SYSEX_BUFF_SIZE 1024
static unsigned char sysexbuff[SYSEX_BUFF_SIZE];
static int sysexbufflen;

int alsaplayer_err;

static snd_seq_queue_status_t *queue_status;

////////////////////

// alsa output list functionality

int alsaplayer_num_outs;
alsaplay_output_t alsaplayer_outputs[64];

void alsaplay_clear_outputs(void) {
  // clear output list
  alsaplayer_num_outs = 0;
}

static snd_seq_client_info_t *cinfo;
static snd_seq_port_info_t   *pinfo;

void alsaplay_refresh_outputs(void) {
  // port type and capabilities required from valid MIDI output
  const int OUT_CAPS_DESIRED = (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);

  snd_seq_client_info_malloc(&cinfo);
  snd_seq_port_info_malloc  (&pinfo);

  if (!seq_handle)
  {
    lprintf(LO_WARN, "alsaplay_refresh_outputs: Can't list ALSA output ports: seq_handle is not initialized\n");
    return;
  }

  alsaplay_clear_outputs();

  // clear client info
  snd_seq_client_info_set_client(cinfo, -1);

  while (snd_seq_query_next_client(seq_handle, cinfo) == 0)
  {
    // list ports of each client

    int client_num = snd_seq_client_info_get_client(cinfo);

    if (client_num == out_id)
    {
      // skip self
      continue;
    }

    if (!snd_seq_client_info_get_num_ports(cinfo))
    {
      // skip clients without ports
      continue;
    }

    // clear port info
    snd_seq_port_info_set_client(pinfo, client_num);
    snd_seq_port_info_set_port(pinfo, -1);

    while (snd_seq_query_next_port(seq_handle, pinfo) == 0)
    {
      int port_num = snd_seq_port_info_get_port(pinfo);
      int out_ind;
      const char *client_name;

      // check if port is valid midi output

      if (!(snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
      {
        continue;
      }

      if ((snd_seq_port_info_get_capability(pinfo) & OUT_CAPS_DESIRED) != OUT_CAPS_DESIRED)
      {
        continue;
      }

      // add to outputs list

      out_ind = alsaplayer_num_outs++;

      alsaplayer_outputs[out_ind].client = client_num;
      alsaplayer_outputs[out_ind].port = port_num;

      client_name = snd_seq_client_info_get_name(cinfo);

      lprintf(LO_INFO, "alsaplay_refresh_outputs: output #%d: (%d:%d) %s\n", out_ind, client_num, port_num, client_name);

      // client name only up to 100 chars, so it always fits within a 120 byte buffer
      sprintf(alsaplayer_outputs[out_ind].name, "%.*s (%d:%d)", 100, client_name, client_num, port_num);
    }
  }

  snd_seq_client_info_free(cinfo);
  snd_seq_port_info_free  (pinfo);
}

int alsaplay_connect_output(int which) {
  if (which >= alsaplayer_num_outs)
  {
    lprintf(LO_WARN, "alsaplay_connect_output: tried to connect to output listing at index out of bounds: %d\n", which);
    return -1;
  }

  return alsa_midi_set_dest(alsaplayer_outputs[which].client, alsaplayer_outputs[which].port);
}

const char *alsaplay_get_output_name(int which) {
  if (seq_handle == NULL)
  {
    return NULL;
  }

  if (which >= alsaplayer_num_outs)
  {
    return NULL;
  }

  return alsaplayer_outputs[which].name;
}

////////////////////

// alsa utility functions

int alsa_midi_default_dest (void)
{
  static int status; // alsa error code
  static int code;   // *our* error code, for control flow

  static const char *loopback_check_name = "MIDI THROUGH"; // uppercase for comparison
  static const signed char upper_diff = 'A' - 'a';
  const int loopback_check_len = strlen(loopback_check_name);

  // port type and capabilities required from valid MIDI output
  const int OUT_CAPS_DESIRED = (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);

  int loopback_cl = -1, loopback_prt = 0;
  const char *loopback_name;

  snd_seq_client_info_malloc(&cinfo);
  snd_seq_port_info_malloc  (&pinfo);

  if (!seq_handle)
  {
    lprintf(LO_WARN, "alsa_midi_default_dest: Can't list ALSA output ports: seq_handle is not initialized\n");
    return 0;
  }

  alsaplay_clear_outputs();

  // clear client info
  snd_seq_client_info_set_client(cinfo, -1);

  while (snd_seq_query_next_client(seq_handle, cinfo) == 0)
  {
    // list ports of each client

    int client_num = snd_seq_client_info_get_client(cinfo);
    const char *client_name;

    if (client_num == out_id)
    {
      // skip self
      continue;
    }

    if (!snd_seq_client_info_get_num_ports(cinfo))
    {
      // skip clients without ports
      continue;
    }

    client_name = snd_seq_client_info_get_name(cinfo);

    if (strlen(client_name) >= loopback_check_len)
    {
      // check for and skip loopback (eg MIDI Through)

      int i;

      for (i = 0; i < loopback_check_len; i++)
      {
        char a = client_name[i];
        a = (a < 'a') || (a > 'z') ? a : a + upper_diff;

        if (a != loopback_check_name[i])
        {
          break;
        }
      }

      if (i == loopback_check_len)
      {
        loopback_cl = client_num;
        loopback_name = client_name;

        continue;
      }
    }

    // clear port info
    snd_seq_port_info_set_client(pinfo, client_num);
    snd_seq_port_info_set_port(pinfo, -1);

    while (snd_seq_query_next_port(seq_handle, pinfo) == 0)
    {
      int port_num = snd_seq_port_info_get_port(pinfo);

      // check if port is valid midi output

      if (!(snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
      {
        continue;
      }

      if ((snd_seq_port_info_get_capability(pinfo) & OUT_CAPS_DESIRED) != OUT_CAPS_DESIRED)
      {
        continue;
      }

      if (loopback_cl == client_num)
      {
        // save as midi through port

        loopback_prt = port_num;
        break;
      }

      else
      {
        // connect to this port

        if (alsa_midi_set_dest(client_num, port_num) != 0)
        {
          lprintf(LO_WARN, "alsa_midi_default_dest: error connecting to default port %i:%i (%s): %s\n", client_num, port_num, client_name, snd_strerror(alsaplayer_err));

          code = 0;
          goto cleanup;
        }

        lprintf(LO_INFO, "alsa_midi_default_dest: connected to default port %i:%i (%s)\n", client_num, port_num, client_name);

        code = 1;
        goto cleanup;
      }
    }
  }

  // try midi through as last resort fallback

  if (loopback_cl != -1)
  {
    if ((status = alsa_midi_set_dest(loopback_cl, loopback_prt)) != 0)
    {
      lprintf(LO_WARN, "alsa_midi_default_dest: (fallback) error connecting to default port %i:%i (%s): %s\n", loopback_cl, loopback_prt, loopback_name, snd_strerror(status));
      code = 0;
      goto cleanup;
    }

    lprintf(LO_INFO, "alsa_midi_default_dest: (fallback) connected to default port %i:%i (%s)\n", loopback_cl, loopback_prt, loopback_name);

    code = 1;
    goto cleanup;
  }

  // no default port
  lprintf(LO_WARN, "alsa_midi_default_dest: no default port found\n");

  code = 0;

cleanup:
  snd_seq_client_info_free(cinfo);
  snd_seq_port_info_free  (pinfo);

  return code;
}

static const char *alsa_midi_open (void)
{
  CHK_RET(snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_OUTPUT, 0),
    "could not open sequencer")

  CHK_RET(snd_seq_set_client_name(seq_handle, "PrBoom+ MIDI"),
    "could not set client name")

  out_id = snd_seq_client_id(seq_handle);

  CHK_RET(
    out_port = snd_seq_create_simple_port(seq_handle, "Music",
      SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ,
      SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_SOFTWARE
    ),
    "could not open alsa port")

  out_queue = snd_seq_alloc_named_queue(seq_handle, "prboom music queue");

  snd_seq_queue_status_malloc(&queue_status);

  alsa_open = 1;
  return NULL;
}

int alsa_midi_set_dest (int client, int port)
{
  static int last_client = -1, last_port = 0;

  if (!seq_handle) {
    return -2;
  }

  // disconnect if previously connected

  if (last_client != 0) {
    snd_seq_disconnect_to(seq_handle, out_port, last_client, last_port);

    last_client = client;
    last_port = port;
  }

  // connects to a destination alsa-midi client and port

  CHK_LPRINT_ERR_RET(snd_seq_connect_to(seq_handle, out_port, client, port), -3,
    LO_WARN, "alsa_midi_set_dest: error connecting to (%d:%d): %s", last_client, last_port);

  alsa_first_connected = 1;
  return 0;
}

static unsigned long alsa_now (void)
{
  // get current position in millisecs

  const snd_seq_real_time_t *time;

  // update queue status
  CHK_LPRINT_ERR_RET(snd_seq_get_queue_status(seq_handle, out_queue, queue_status), 0,
    LO_WARN, "alsaplayer: alsa_now(): error getting queue status: %s\n");

  time = snd_seq_queue_status_get_real_time(queue_status);

  if (time == 0) {
    lprintf (LO_WARN, "alsaplayer: alsa_now(): error getting realtime position from queue status\n");
    return 0;
  }

  return time->tv_sec * 1000 + (time->tv_nsec / 1000000); // (s,ns) to ms
}

////////////////////

// alsa player callbacks


static const snd_seq_real_time_t *alsa_now_realtime (void)
{
  // get current position in millisecs

  const snd_seq_real_time_t *time;

  // update queue status
  CHK_LPRINT_ERR_RET(snd_seq_get_queue_status(seq_handle, out_queue, queue_status), 0,
    LO_WARN, "alsaplayer: alsa_now(): error getting queue status: %s\n");

  time = snd_seq_queue_status_get_real_time(queue_status);

  if (time == 0) {
    lprintf (LO_WARN, "alsaplayer: alsa_now(): error getting realtime position from queue status\n");
  }

  return time;
}

static void alsa_midi_evt_start (unsigned long when)
{
  snd_seq_ev_clear(&seq_ev);

  // source
  snd_seq_ev_set_source(&seq_ev, out_port);

  // schedule
  if (when != 0) {
    snd_seq_real_time_t rtime;

    // ms into (s,ns)
    rtime.tv_sec = when / 1000;
    rtime.tv_nsec = (when % 1000) * 1000000;

    snd_seq_ev_schedule_real(&seq_ev, out_queue, 0, &rtime);
  }

  else {
    snd_seq_ev_schedule_real(&seq_ev, out_queue, 0, alsa_now_realtime());
  }

  // priority
  snd_seq_ev_set_priority(&seq_ev, 0);

  // destination
  snd_seq_ev_set_subs(&seq_ev);
}

static void alsa_midi_evt_finish ()
{
  CHK_LPRINT_ERR(snd_seq_event_output(seq_handle, &seq_ev),
    LO_WARN, "alsa_midi_evt_finish: could not output alsa midi event: %s\n");
}

static void alsa_midi_evt_flush ()
{
  CHK_LPRINT_ERR(snd_seq_drain_output(seq_handle),
    LO_WARN, "alsa_midi_evt_finish: could not drain alsa sequencer output: %s\n");
}

static void alsa_midi_write_event (unsigned long when, midi_event_type_t type, int channel, int v1, int v2)
{
  // ported from portmidiplayer.c (no pun intended!)
  alsa_midi_evt_start(when);

  // set event value fields
  switch(type) {
    case MIDI_EVENT_NOTE_OFF:
      snd_seq_ev_set_noteoff(&seq_ev, channel, v1, v2);
      break;

    case MIDI_EVENT_NOTE_ON:
      snd_seq_ev_set_noteon(&seq_ev, channel, v1, v2);
      break;

    case MIDI_EVENT_AFTERTOUCH:
      snd_seq_ev_set_keypress(&seq_ev, channel, v1, v2);
      break;

    case MIDI_EVENT_PROGRAM_CHANGE:
      snd_seq_ev_set_pgmchange(&seq_ev, channel, v1);
      break;

    case MIDI_EVENT_CHAN_AFTERTOUCH:
      snd_seq_ev_set_chanpress(&seq_ev, channel, v1);
      break;

    case MIDI_EVENT_PITCH_BEND:
      snd_seq_ev_set_pitchbend(&seq_ev, channel, v1 << 8 | v2);
      break;

    case MIDI_EVENT_CONTROLLER:
      snd_seq_ev_set_controller(&seq_ev, channel, v1, v2);
      break;

    default:
      // unknown type
      lprintf(LO_WARN, "alsa_midi_write_event: unknown midi event type: %d\n", type);
      return;
  }

  alsa_midi_evt_finish();
}

static void alsa_midi_write_control (unsigned long when, int channel, int v1, int v2)
{
  alsa_midi_write_event(when, MIDI_EVENT_CONTROLLER, channel, v1, v2);
}

static void alsa_midi_write_control_now (int channel, int v1, int v2)
{
  // send event now, disregarding 'when'
  alsa_midi_write_control(0, channel, v1, v2);
}

static void alsa_midi_all_notes_off_chan (int channel)
{
  alsa_midi_write_control_now(channel, 123, 0);
  alsa_midi_evt_flush();
}

static void alsa_midi_all_notes_off (void)
{
  // sends All Notes Off event in all channels
  for (int i = 0; i < 16; i++) {
    alsa_midi_all_notes_off_chan(i);
  }
}

static int alsa_midi_init_connect_default_port (void)
{
  // load MIDI device specified in config

  if (snd_mididev && strlen(snd_mididev))
  {
    snd_seq_addr_t seqaddr;

    CHK_LPRINT_ERR(snd_seq_parse_address(seq_handle, &seqaddr, snd_mididev),
      LO_WARN, "alsa_init: Error connecting to configured MIDI output port \"%s\": %s", snd_mididev)

    return alsa_midi_set_dest(seqaddr.client, seqaddr.port) == 0;
  }

  else
  {
    // connect to default
    return alsa_midi_default_dest();
  }
}

////////////////////

// alsa player callbacks

static const char *alsa_name (void)
{
  return "alsa midi player";
}

static int alsa_init (int samplerate)
{
  const char *msg = alsa_midi_open();

  lprintf (LO_INFO, "alsaplayer: Trying to open ALSA output port\n");

  if (msg != NULL) {
    lprintf(LO_WARN, "alsa_init: alsa_midi_open() failed: %s\n", msg);
    return 0;
  }

  lprintf (LO_INFO, "alsaplayer: Successfully opened port: %d\n", out_port);

  alsaplay_refresh_outputs(); // make output list and print it out

  return 1;
}

static void alsa_shutdown (void)
{
  if (seq_handle) {
    alsa_midi_all_notes_off();
    alsa_midi_evt_flush();

    snd_seq_free_queue(seq_handle, out_queue);
    snd_seq_queue_status_free(queue_status);

    snd_seq_delete_simple_port(seq_handle, out_port);
    snd_seq_close(seq_handle);

    seq_handle = NULL;
  }

  alsa_open = 0;
}

static const void *alsa_registersong (const void *data, unsigned len)
{
  midimem_t mf;

  mf.len = len;
  mf.pos = 0;
  mf.data = (const byte*)data;

  midifile = MIDI_LoadFile (&mf);

  if (!midifile)
  {
    lprintf (LO_WARN, "alsa_registersong: Failed to load MIDI.\n");
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

static int channelvol[16];

static void alsa_setchvolume (int ch, int v, unsigned long when)
{
  channelvol[ch] = v;
  alsa_midi_write_control(when, ch, 7, channelvol[ch] * alsa_volume / 15);
  alsa_midi_evt_flush();
}

static void alsa_refreshvolume (void)
{
  int i;

  for (i = 0; i < 16; i ++)
    alsa_midi_write_control_now(i, 7, channelvol[i] * alsa_volume / 15);

  alsa_midi_evt_flush();
}

static void alsa_clearchvolume (void)
{
  int i;
  for (i = 0; i < 16; i++)
    channelvol[i] = 127; // default: max
}

static void alsa_setvolume (int v)
{ 
  static int firsttime = 1;

  if (alsa_volume == v && !firsttime)
    return;
  firsttime = 0;

  alsa_volume = v;

  alsa_refreshvolume ();
}


static void alsa_unregistersong (const void *handle)
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

static void alsa_pause (void)
{
  int i;
  alsa_paused = 1;
  alsa_midi_all_notes_off();

  snd_seq_stop_queue(seq_handle, out_queue, 0);
}

static void alsa_resume (void)
{
  alsa_paused = 0;
  trackstart = alsa_now ();

  snd_seq_continue_queue(seq_handle, out_queue, 0);
}

static void alsa_play (const void *handle, int looping)
{
  snd_seq_queue_timer_t *timer;
  int status;

  // reinit queue

  if (!alsa_first_connected)
  {
    // connect to default port if haven't connected at least once yet
    alsa_midi_init_connect_default_port();
  }

  if (out_queue) {
    snd_seq_free_queue(seq_handle, out_queue);
    snd_seq_queue_status_free(queue_status);
  }

  // make queue
  out_queue = snd_seq_alloc_named_queue(seq_handle, "prboom music queue");
  
  snd_seq_queue_status_malloc(&queue_status);

  // set queue resolution

  snd_seq_queue_timer_malloc(&timer);

  status = snd_seq_get_queue_timer(seq_handle, out_queue, timer);

  if (status < 0)
  {
    lprintf(LO_WARN, "alsa_play: error getting sched queue timer: %s\n", snd_strerror(status));

    snd_seq_queue_timer_free(timer);
    goto finish;
  }

  snd_seq_queue_timer_set_resolution(timer, 1000000 / 32); // 1000000 ns = 1 ms, so this is 1/32 ms

  status = snd_seq_set_queue_timer(seq_handle, out_queue, timer);

  if (status < 0)
  {
    lprintf(LO_WARN, "alsa_play: error setting sched queue timer with new resolution: %s\n", snd_strerror(status));

    snd_seq_queue_timer_free(timer);
    goto finish;
  }

  lprintf(LO_INFO, "alsa_play: success\n");

  snd_seq_queue_timer_free(timer);

finish:
  // initialize state stuff
  eventpos = 0;
  alsa_looping = looping;
  alsa_playing = 1;
    //alsa_paused = 0;
  alsa_delta = 0.0;
  alsa_clearchvolume ();
  alsa_refreshvolume ();
  trackstart = alsa_now ();

  // start scheduling queue
  snd_seq_start_queue(seq_handle, out_queue, 0);
}


static void alsa_midi_writesysex (unsigned long when, int etype, unsigned char *data, int len)
{
  // sysex code is untested
  // it's possible to use an auto-resizing buffer here, but a malformed
  // midi file could make it grow arbitrarily large (since it must grow
  // until it hits an 0xf7 terminator)
  if (len + sysexbufflen > SYSEX_BUFF_SIZE)
  {
    lprintf (LO_WARN, "alsaplayer: ignoring large or malformed sysex message\n");
    sysexbufflen = 0;
    return;
  }
  memcpy (sysexbuff + sysexbufflen, data, len);
  sysexbufflen += len;
  if (sysexbuff[sysexbufflen - 1] == 0xf7) // terminator
  {
    alsa_midi_evt_start(when);
    snd_seq_ev_set_sysex(&seq_ev, sysexbufflen, sysexbuff);
    alsa_midi_evt_finish();

    sysexbufflen = 0;
  }
}  

static void alsa_stop (void)
{
  int i;
  alsa_playing = 0;
  

  // songs can be stopped at any time, so reset everything
  for (i = 0; i < 16; i++)
  {
    alsa_midi_write_control_now(i, 123, 0); // all notes off
    alsa_midi_write_control_now(i, 121, 0); // reset all parameters

    // RPN sequence to adjust pitch bend range (RPN value 0x0000)
    alsa_midi_write_control_now(i, 0x65, 0x00);
    alsa_midi_write_control_now(i, 0x64, 0x00);
    // reset pitch bend range to central tuning +/- 2 semitones and 0 cents
    alsa_midi_write_control_now(i, 0x06, 0x02);
    alsa_midi_write_control_now(i, 0x26, 0x00);
    // end of RPN sequence
    alsa_midi_write_control_now(i, 0x64, 0x7f);
    alsa_midi_write_control_now(i, 0x65, 0x7f);
  }
  alsa_midi_evt_flush();
  // abort any partial sysex
  sysexbufflen = 0;

  snd_seq_stop_queue(seq_handle, out_queue, 0);
}

static void alsa_render (void *vdest, unsigned bufflen)
{
  // wherever you see samples in here, think milliseconds

  unsigned long newtime = alsa_now();
  unsigned long length = newtime - trackstart;

  //timerpos = newtime;
  unsigned long when;

  midi_event_t *currevent;
  
  unsigned sampleswritten = 0;
  unsigned samples;

  memset (vdest, 0, bufflen * 4);



  if (!alsa_playing || alsa_paused)
    return;

  
  while (1)
  {
    double eventdelta;
    currevent = events[eventpos];
    
    // how many samples away event is
    eventdelta = currevent->delta_time * spmc;


    // how many we will render (rounding down); include delta offset
    samples = (unsigned) (eventdelta + alsa_delta);


    if (samples + sampleswritten > length)
    { // overshoot; render some samples without processing an event
      break;
    }


    sampleswritten += samples;
    alsa_delta -= samples;
 
    
    // process event
    when = trackstart + sampleswritten;
    switch (currevent->event_type)
    {
      case MIDI_EVENT_SYSEX:
      case MIDI_EVENT_SYSEX_SPLIT:        
        alsa_midi_writesysex (when, currevent->event_type, currevent->data.sysex.data, currevent->data.sysex.length);
        break;
      case MIDI_EVENT_META: // tempo is the only meta message we're interested in
        if (currevent->data.meta.type == MIDI_META_SET_TEMPO)
          spmc = MIDI_spmc (midifile, currevent, 1000);
        else if (currevent->data.meta.type == MIDI_META_END_OF_TRACK)
        {
          if (alsa_looping)
          {
            int i;
            eventpos = 0;
            alsa_delta += eventdelta;
            // fix buggy songs that forget to terminate notes held over loop point
            // sdl_mixer does this as well
            for (i = 0; i < 16; i++)
              alsa_midi_write_control(when, i, 123, 0); // all notes off
            continue;
          }
          // stop
          alsa_stop ();
          return;
        }
        break; // not interested in most metas
      case MIDI_EVENT_CONTROLLER:
        if (currevent->data.channel.param1 == 7)
        { // volume event
          alsa_setchvolume (currevent->data.channel.channel, currevent->data.channel.param2, when);
          break;
        } // fall through
      default:
        alsa_midi_write_event (when, currevent->event_type, currevent->data.channel.channel, currevent->data.channel.param1, currevent->data.channel.param2);
        break;
      
    }
    // if the event was a "reset all controllers", we need to additionally re-fix the volume (which itself was reset)
    if (currevent->event_type == MIDI_EVENT_CONTROLLER && currevent->data.channel.param1 == 121)
      alsa_setchvolume (currevent->data.channel.channel, 127, when);

    // event processed so advance midiclock
    alsa_delta += eventdelta;
    eventpos++;

  }

  if (samples + sampleswritten > length)
  { // broke due to next event being past the end of current render buffer
    // finish buffer, return
    samples = length - sampleswritten;
    alsa_delta -= samples; // save offset
  }

  trackstart = newtime;

  alsa_midi_evt_flush();
}  

const music_player_t alsa_player =
{
  alsa_name,
  alsa_init,
  alsa_shutdown,
  alsa_setvolume,
  alsa_pause,
  alsa_resume,
  alsa_registersong,
  alsa_unregistersong,
  alsa_play,
  alsa_stop,
  alsa_render
};


#endif // HAVE_ALSA

