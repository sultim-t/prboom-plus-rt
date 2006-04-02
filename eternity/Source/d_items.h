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
//  Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------

#ifndef __D_ITEMS__
#define __D_ITEMS__

#include "doomdef.h"

// Weapon info: sprite frames, ammunition use.
typedef struct
{
  ammotype_t  ammo;
  int         upstate;
  int         downstate;
  int         readystate;
  int         atkstate;
  int         flashstate;
  int         ammopershot; // haleyjd 08/10/02: ammo per shot field
  int         enableaps;   // haleyjd: enables above field, off by default
} weaponinfo_t;

extern  weaponinfo_t    weaponinfo[NUMWEAPONS];

// haleyjd: temporary hack
void D_InitWeaponInfo(void);

#endif

//----------------------------------------------------------------------------
//
// $Log: d_items.h,v $
// Revision 1.3  1998/05/04  21:34:12  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:26  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
