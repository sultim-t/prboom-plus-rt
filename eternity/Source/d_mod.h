// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Method of Death Flags
//
//-----------------------------------------------------------------------------

#ifndef __D_MOD_H__
#define __D_MOD_H__

enum MODTypes
{
   MOD_UNKNOWN,
   MOD_FIST,
   MOD_PISTOL,
   MOD_SHOTGUN,
   MOD_CHAINGUN,
   MOD_ROCKET,
   MOD_R_SPLASH,
   MOD_PLASMA,
   MOD_BFG,
   MOD_BFG_SPLASH,
   MOD_CHAINSAW,
   MOD_SSHOTGUN,
   MOD_SLIME,
   MOD_LAVA,
   MOD_CRUSH,
   MOD_TELEFRAG,
   MOD_FALLING,
   MOD_SUICIDE,
   MOD_BARREL,
   MOD_SPLASH,
   MOD_HIT,
   MOD_BFG11K_SPLASH,
   MOD_BETABFG,
   MOD_BFGBURST,
   MOD_PLAYERMISC,
   MOD_GRENADE,
   NUM_MOD_TYPES
};

// haleyjd 08/21/05: MOD strings for EDF modules
extern const char *MODNames[NUM_MOD_TYPES];

#endif

// EOF

