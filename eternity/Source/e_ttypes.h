// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:  
//   New dynamic TerrainTypes system. Inspired heavily by zdoom, but
//   all-original code.
//
//-----------------------------------------------------------------------------

#ifndef E_TTYPES_H__
#define E_TTYPES_H__

#include "doomtype.h"

#ifdef NEED_EDF_DEFINITIONS

#define EDF_SEC_SPLASH   "splash"
#define EDF_SEC_TERRAIN  "terrain"
#define EDF_SEC_FLOOR    "floor"
#define EDF_SEC_TERDELTA "terraindelta"
extern cfg_opt_t edf_splash_opts[];
extern cfg_opt_t edf_terrn_opts[];
extern cfg_opt_t edf_terdelta_opts[];
extern cfg_opt_t edf_floor_opts[];

void E_ProcessTerrainTypes(cfg_t *cfg);
boolean E_NeedDefaultTerrain(void);

#endif

typedef struct ETerrainSplash_s
{
   int smallclass;        // mobjtype used for small splash
   int smallclip;         // amount of floorclip to apply to small splash
   char smallsound[17];   // sound to play for small splash
   
   int baseclass;         // mobjtype used for normal splash
   int chunkclass;        // mobjtype used for normal splash chunk
   int chunkxvelshift;    // chunk's x velocity factor
   int chunkyvelshift;    // chunk's y velocity factor
   int chunkzvelshift;    // chunk's z velocity factor
   fixed_t chunkbasezvel; // base amount of z velocity for chunk
   char sound[17];        // sound to play for normal splash

   struct ETerrainSplash_s *next; // hash link
   char   name[33];               // hash name
} ETerrainSplash;

typedef struct ETerrain_s
{
   ETerrainSplash *splash;  // pointer to splash object
   int damageamount;        // damage amount at each chance to hurt
   int damagetype;          // MOD to use for damage
   int damagetimemask;      // time mask for damage chances
   fixed_t footclip;        // footclip amount
   boolean liquid;          // is liquid?
   boolean splashalert;     // normal splash causes P_NoiseAlert?
   boolean usepcolors;      // use particle colors?   
   byte pcolor_1;           // particle color 1
   byte pcolor_2;           // particle color 2

   int minversion;          // minimum demo version for this terrain

   struct ETerrain_s *next; // hash link
   char name[33];           // hash name
} ETerrain;

typedef struct EFloor_s
{
   char name[9];          // flat name
   ETerrain *terrain;     // terrain definition
   struct EFloor_s *next; // hash link
} EFloor;

void E_InitTerrainTypes(void);
ETerrain *E_GetThingFloorType(mobj_t *thing);
ETerrain *E_GetTerrainTypeForPt(fixed_t x, fixed_t y, int pos);
fixed_t E_SectorFloorClip(sector_t *sector);
boolean E_HitWater(mobj_t *thing, sector_t *sector);
boolean E_HitFloor(mobj_t *thing);
void E_PtclTerrainHit(particle_t *);

#endif

// EOF

