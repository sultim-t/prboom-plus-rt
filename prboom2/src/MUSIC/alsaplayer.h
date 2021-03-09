/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *
 *  Copyright (C) 2021 by Gustavo Rehermann
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

#ifndef ALSAPLAYER_H
#define ALSAPLAYER_H

#include "musicplayer.h"

extern const music_player_t alsa_player;

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

// available outputs
typedef struct {
    int client;
    int port;
    char name[120]; // 120 + two 32-bit (4-byte) ints = 128, nice alignment
} alsaplay_output_t;

extern alsaplay_output_t alsaplayer_outputs[64];
extern int alsaplayer_num_outs;
extern int alsaplayer_err;

void alsaplay_clear_outputs(void);
void alsaplay_refresh_outputs(void);
const char *alsaplay_get_output_name(int which);
int alsaplay_connect_output(int which);
int alsa_midi_set_dest (int client, int port);

#endif // HAVE_ALSA

#endif // ALSAPLAYER_H
