// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
//----------------------------------------------------------------------------
//
// EDF Sound Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef E_SOUND_H
#define E_SOUND_H

#include "doomtype.h"
#include "sounds.h"

sfxinfo_t *E_SoundForName(const char *);
sfxinfo_t *E_EDFSoundForName(const char *name);
sfxinfo_t *E_SoundForDEHNum(int);

void E_NewWadSound(const char *);
void E_PreCacheSounds(void);

#ifdef NEED_EDF_DEFINITIONS
extern cfg_opt_t edf_sound_opts[];
extern cfg_opt_t edf_sdelta_opts[];
void E_ProcessSounds(cfg_t *cfg);
void E_ProcessSoundDeltas(cfg_t *cfg);

#define EDF_SEC_SOUND  "sound"
#define EDF_SEC_SDELTA "sounddelta"
#endif

#endif

// EOF

