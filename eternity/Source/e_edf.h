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
// EDF Core
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef __E_EDF_H__
#define __E_EDF_H__

// sprite variables

extern int playerSpriteNum;
extern int blankSpriteNum;

// pickup effects enumeration

enum
{
   PFX_NONE,
   PFX_GREENARMOR,
   PFX_BLUEARMOR,
   PFX_POTION,
   PFX_ARMORBONUS,
   PFX_SOULSPHERE,
   PFX_MEGASPHERE,
   PFX_BLUEKEY,
   PFX_YELLOWKEY,
   PFX_REDKEY,
   PFX_BLUESKULL,
   PFX_YELLOWSKULL,
   PFX_REDSKULL,
   PFX_STIMPACK,
   PFX_MEDIKIT,
   PFX_INVULNSPHERE,
   PFX_BERZERKBOX,
   PFX_INVISISPHERE,
   PFX_RADSUIT,
   PFX_ALLMAP,
   PFX_LIGHTAMP,
   PFX_CLIP,
   PFX_CLIPBOX,
   PFX_ROCKET,
   PFX_ROCKETBOX,
   PFX_CELL,
   PFX_CELLPACK,
   PFX_SHELL,
   PFX_SHELLBOX,
   PFX_BACKPACK,
   PFX_BFG,
   PFX_CHAINGUN,
   PFX_CHAINSAW,
   PFX_LAUNCHER,
   PFX_PLASMA,
   PFX_SHOTGUN,
   PFX_SSG,
   PFX_HGREENKEY,
   PFX_HBLUEKEY,
   PFX_HYELLOWKEY,
   PFX_HPOTION,
   PFX_SILVERSHIELD,
   PFX_ENCHANTEDSHIELD,
   PFX_BAGOFHOLDING,
   PFX_HMAP,
   PFX_GWNDWIMPY,
   PFX_GWNDHEFTY,
   PFX_MACEWIMPY,
   PFX_MACEHEFTY,
   PFX_CBOWWIMPY,
   PFX_CBOWHEFTY,
   PFX_BLSRWIMPY,
   PFX_BLSRHEFTY,
   PFX_PHRDWIMPY,
   PFX_PHRDHEFTY,
   PFX_SKRDWIMPY,
   PFX_SKRDHEFTY,
   PFX_TOTALINVIS,
   PFX_NUMFX
};

extern int *pickupfx;

void E_ProcessEDF(const char *filename);
void E_ProcessEDFLumps(void);

void E_EDFSetEnableValue(const char *, int); // enables

void E_EDFLogPuts(const char *msg);
void E_EDFLogPrintf(const char *msg, ...);
void E_EDFLoggedErr(int lv, const char *msg, ...);

#endif

// EOF

