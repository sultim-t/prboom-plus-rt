// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//  System interface for music.
//
//-----------------------------------------------------------------------------


#ifndef OPLPLAYER_H
#define OPLPLAYER_H

#include "opl.h"
#include "midifile.h"
#include "musicplayer.h"

typedef struct
{
	byte tremolo;
	byte attack;
	byte sustain;
	byte waveform;
	byte scale;
	byte level;
} PACKEDATTR genmidi_op_t;

typedef struct
{
	genmidi_op_t modulator;
	byte feedback;
	genmidi_op_t carrier;
	byte unused;
	short base_note_offset;
} PACKEDATTR genmidi_voice_t;

typedef struct
{
	unsigned short flags;
	byte fine_tuning;
	byte fixed_note;

	genmidi_voice_t voices[2];
} PACKEDATTR genmidi_instr_t;

typedef struct opl_voice_s opl_voice_t;

// Data associated with a channel of a track that is currently playing.

typedef struct
{
	// The instrument currently used for this track.

	const genmidi_instr_t *instrument;

	// Volume level

	int volume;

	// Pitch bend value:

	int bend;

} opl_channel_data_t;

// Data associated with a track that is currently playing.

typedef struct
{
	// Data for each channel.

	opl_channel_data_t channels[MIDI_CHANNELS_PER_TRACK];

	// Track iterator used to read new events.

	midi_track_iter_t *iter;

	// Tempo control variables

	unsigned int ticks_per_beat;
	unsigned int ms_per_beat;
} opl_track_data_t;

struct opl_voice_s
{
	// Index of this voice:
	int index;

	// The operators used by this voice:
	int op1, op2;

	// Currently-loaded instrument data
	const genmidi_instr_t *current_instr;

	// The voice number in the instrument to use.
	// This is normally set to zero; if this is a double voice
	// instrument, it may be one.
	unsigned int current_instr_voice;

	// The channel currently using this voice.
	opl_channel_data_t *channel;

	// The midi key that this voice is playing.
	unsigned int key;

	// The note being played.  This is normally the same as
	// the key, but if the instrument is a fixed pitch
	// instrument, it is different.
	unsigned int note;

	// The frequency value being used.
	unsigned int freq;

	// The volume of the note being played on this channel.
	unsigned int note_volume;

	// The current volume (register value) that has been set for this channel.
	unsigned int reg_volume;

	// Next in linked list; a voice is always either in the
	// free list or the allocated list.
	opl_voice_t *next;
};

typedef struct opl_synth_player_s {
	music_player_t music;
	dboolean music_initialized;

	//static dboolean musicpaused = false;
	int current_music_volume;

	// GENMIDI lump instrument data:

	const genmidi_instr_t *main_instrs;
	const genmidi_instr_t *percussion_instrs;

	// Voices:

	opl_voice_t voices[OPL_NUM_VOICES];
	opl_voice_t *voice_free_list;
	opl_voice_t *voice_alloced_list;

	// Track data for playing tracks:

	opl_track_data_t *tracks;
	unsigned int num_tracks;
	unsigned int running_tracks;
	dboolean song_looping;

} opl_synth_player_t; 

extern opl_synth_player_t opl_synth_player;


#endif
