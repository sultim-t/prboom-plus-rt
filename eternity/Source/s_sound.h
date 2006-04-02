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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      The not so system specific sound interface.
//
//-----------------------------------------------------------------------------

#ifndef __S_SOUND__
#define __S_SOUND__

#include "sounds.h"

// haleyjd 07/13/05: moved these defines to the header

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST_I 1200
#define S_CLIPPING_DIST (S_CLIPPING_DIST_I << FRACBITS)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
//
// killough 12/98: restore original
// #define S_CLOSE_DIST (160<<FRACBITS)

#define S_CLOSE_DIST_I 200
#define S_CLOSE_DIST (S_CLOSE_DIST_I << FRACBITS)

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void);

//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void S_StartSound(const mobj_t *origin, int sound_id);
void S_StartSoundName(const mobj_t *origin, char *name);
void S_StartSfxInfo(const mobj_t *origin, sfxinfo_t *sfx);

// Stop sound for thing at <origin>
void S_StopSound(const mobj_t *origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusicNum(int music_id, int looping);
void S_ChangeMusicName(char *name, int looping);
void S_ChangeMusic(musicinfo_t *music, int looping);

// Stops the music fer sure.
void S_StopMusic(void);
void S_StopSounds();

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

sfxinfo_t *S_SfxInfoForName(char *name);
void S_UpdateSound(int lumpnum);
void S_UpdateSoundDeferred(int lumpnum);
void S_ProcDeferredSounds(void);
void S_Chgun(void);

musicinfo_t *S_MusicForName(char *name);
void S_UpdateMusic(int lumpnum);

//
// Updates music & sounds
//
void S_UpdateSounds(const mobj_t *listener);
void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);

// haleyjd: rudimentary sound checker
boolean S_CheckSoundPlaying(mobj_t *, sfxinfo_t *sfx);

// precache sound?
extern int s_precache;

// machine-independent sound params
extern int numChannels;
extern int default_numChannels;  // killough 10/98

//jff 3/17/98 holds last IDMUS number, or -1
extern int idmusnum;

#endif

//----------------------------------------------------------------------------
//
// $Log: s_sound.h,v $
// Revision 1.4  1998/05/03  22:57:36  killough
// beautification, add external declarations
//
// Revision 1.3  1998/04/27  01:47:32  killough
// Fix pickups silencing player weapons
//
// Revision 1.2  1998/01/26  19:27:51  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
