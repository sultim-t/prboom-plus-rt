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

#ifndef MADPLAYER_H
#define MADPLAYER_H

#include <mad.h>

typedef struct mp_player_s {
	music_player_t music;
	struct mad_stream Stream;
	struct mad_frame  Frame;
	struct mad_synth  Synth;
	struct mad_header Header;


	int mp_looping;
	int mp_volume; // 0-15
	int mp_samplerate_target;
	int mp_paused;
	int mp_playing;

	const void *mp_data;
	int mp_len;


	int mp_leftoversamps; // number of extra samples
	// left over in mad decoder
	int mp_leftoversamppos;
} mp_player_t;

extern mp_player_t mp_player;









#endif // MADPLAYER_H
