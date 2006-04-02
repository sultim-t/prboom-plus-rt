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
// EDF States Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef E_STATES_H__
#define E_STATES_H__

int E_StateNumForDEHNum(int dehnum);        // dehnum lookup
int E_GetStateNumForDEHNum(int dehnum);     //    fatal error version
int E_SafeState(int dehnum);                //    fallback version
int E_StateNumForName(const char *name);    // mnemonic lookup
int E_GetStateNumForName(const char *name); //    fatal error version

extern int NullStateNum;

// EDF-Only Definitions/Declarations
#ifdef NEED_EDF_DEFINITIONS

#define EDF_SEC_FRAME    "frame"
#define EDF_SEC_FRMDELTA "framedelta"

extern cfg_opt_t edf_frame_opts[];
extern cfg_opt_t edf_fdelta_opts[];

void E_CollectStates(cfg_t *scfg);
void E_ProcessStates(cfg_t *cfg);
void E_ProcessStateDeltas(cfg_t *cfg);

#endif

#endif

// EOF

