/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_sound.h,v 1.1 2000/05/04 08:02:58 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
 *      System interface, sound.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __I_SOUND__
#define __I_SOUND__

#include <stdio.h>

#include "sounds.h"
#include "doomtype.h"

#define SNDSERV
#undef SNDINTR

#ifndef SNDSERV
#include "l_soundgen.h"
#endif

// Init at program start...
void I_InitSound(void);

// ... shut down and relase at program termination.
void I_ShutdownSound(void);

//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels(void);

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfxinfo);

// Starts a sound in a particular sound channel.
int I_StartSound(int id, int vol, int sep, int pitch, int priority);

// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
boolean I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);

//
//  MUSIC I/O
//
void I_InitMusic(void);
void I_ShutdownMusic(void);

// Volume.
void I_SetMusicVolume(int volume);

// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

// Registers a song handle to song data.
int I_RegisterSong(const void *data, size_t len);

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int handle, int looping);

// Stops a song over 3 seconds.
void I_StopSong(int handle);

// See above (register), then think backwards
void I_UnRegisterSong(int handle);

// Allegro card support jff 1/18/98
extern int snd_card;
extern int mus_card;
// CPhipps - put these in config file
extern const char* sndserver_filename;
extern const char* snd_device;
extern const char* musserver_filename; 

#endif

//----------------------------------------------------------------------------
//
// $Log: i_sound.h,v $
// Revision 1.1  2000/05/04 08:02:58  proff_fs
// Initial revision
//
// Revision 1.13  2000/04/09 13:24:44  cph
// Remove DOS-only options
//
// Revision 1.12  2000/04/03 22:10:12  cph
// Const parameter to I_RegisterSong
// Reduce duplication of music data
//
// Revision 1.11  1999/10/31 16:35:19  cphipps
// Include doomtype.h
//
// Revision 1.10  1999/10/12 13:01:15  cphipps
// Changed header to GPL
//
// Revision 1.9  1998/11/17 15:59:21  cphipps
// Changed I_SoundIsPlaying to an boolean (int)
// since it's more logical, to match DosDoom
//
// Revision 1.8  1998/10/20 18:15:51  cphipps
// Make config'able sound/music stuff const char*'s
// Added musserver path as extern var
//
// Revision 1.7  1998/10/20 14:45:01  cphipps
// Added length parameter to I_RegisterSong
//
// Revision 1.6  1998/10/16 13:34:02  cphipps
// Fix prototypes
//
// Revision 1.5  1998/10/10 20:34:22  cphipps
// Add variable decl for sound device
//
// Revision 1.4  1998/09/21 08:52:00  cphipps
// Minor fixes.
//
// Revision 1.3  1998/09/20 11:03:39  cphipps
// Removed decls for internal functions to l_soundgen.h
// This is included if the sound generation is not external
//
// Revision 1.2  1998/09/18 15:42:39  cphipps
// More rigidly define functions according to choice of sound method
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.3  1998/09/13 15:32:51  cphipps
// Added sndserver_filename
//
// Revision 1.2  1998/09/12 16:54:07  cphipps
// Removed ^M's
// Removed allegro.h
//
// Revision 1.1  1998/09/11 18:37:35  develop
// Initial revision
//
// Revision 1.4  1998/05/03  22:31:58  killough
// beautification, add some external declarations
//
// Revision 1.3  1998/02/23  04:27:08  killough
// Add variable pitched sound support
//
// Revision 1.2  1998/01/26  19:26:57  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
