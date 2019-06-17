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


#ifndef VORBISPLAYER_H
#define VORBISPLAYER_H

#include <stdlib.h>
#include <string.h>
#include "MUSIC/musicplayer.h"


// uncomment to allow (experiemntal) support for
// zdoom-style audio loops
#define ZDOOM_AUDIO_LOOP

#ifdef HAVE_LIBVORBISFILE
#include <vorbis/vorbisfile.h>
#endif

typedef struct vorb_player_s {
	music_player_t music;
	ov_callbacks vcallback;
	int vorb_looping;
	int vorb_volume;
	int vorb_samplerate_target;
	int vorb_samplerate_in;
	int vorb_paused;
	int vorb_playing;
#ifdef ZDOOM_AUDIO_LOOP
	unsigned vorb_loop_from;
	unsigned vorb_loop_to;
	unsigned vorb_total_pos;
#endif // ZDOOM_AUDIO_LOOP
	const char *vorb_data;
	size_t vorb_len;
	size_t vorb_pos;
#ifdef HAVE_LIBVORBISFILE
	OggVorbis_File vf;
#else
	int vf;
#endif
} vorb_player_t;

extern vorb_player_t vorb_player;
// cybermind: this will play the recording
extern vorb_player_t record_player;









#endif // VORBISPLAYER_H
