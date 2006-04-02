// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
// EDF Thing Types Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef E_THINGS_H__
#define E_THINGS_H__

#include "doomtype.h"

// Global Data
extern int UnknownThingType;

#ifdef NEED_EDF_DEFINITIONS

// Section Names
#define EDF_SEC_THING    "thingtype"
#define EDF_SEC_TNGDELTA "thingdelta"

// Section Options
extern cfg_opt_t edf_thing_opts[];
extern cfg_opt_t edf_tdelta_opts[];

#endif

// Global Functions

// For EDF Only:

#ifdef NEED_EDF_DEFINITIONS
void E_CollectThings(cfg_t *tcfg);
void E_ProcessThing(int i, cfg_t *thingsec, cfg_t *pcfg, boolean def);
void E_ProcessThings(cfg_t *cfg);
void E_ProcessThingDeltas(cfg_t *cfg);
#endif

// For Game Engine:
int E_ThingNumForDEHNum(int dehnum);        // dehnum lookup
int E_GetThingNumForDEHNum(int dehnum);     //   fatal error version
int E_SafeThingType(int dehnum);            //   fallback version
int E_ThingNumForName(const char *name);    // mnemonic lookup
int E_GetThingNumForName(const char *name); //   fatal error version
int E_SafeThingName(const char *name);      //   fallback version

#endif

// EOF

