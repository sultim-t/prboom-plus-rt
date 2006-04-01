/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  Sound server for LxDoom, based on the sound server released with the 
 *   original linuxdoom sources.
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999-2000 by Colin Phipps
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
 *  /dev/dsp compatible sound generation
 *-----------------------------------------------------------------------------
 */

/* ... update sound buffer and audio device at runtime... */
void I_UpdateSound(void);

void I_SubmitSound(void);

void I_InitSoundGen(const char* snd_dev);

void I_EndSoundGen(void);

const void* I_PadSfx(const void* data, int* size);

int I_AddSfx(int sfxid, int volume, int pitch, int seperation );

extern int* lengths;

