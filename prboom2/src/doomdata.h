/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: doomdata.h,v 1.5 2002/02/10 21:03:45 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *  all external data is defined here
 *  most of the data is loaded into different structures at run time
 *  some internal structures shared by many modules are here
 *
 *-----------------------------------------------------------------------------*/

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum {
  ML_LABEL,             // A separator, name, ExMx or MAPxx
  ML_THINGS,            // Monsters, items..
  ML_LINEDEFS,          // LineDefs, from editing
  ML_SIDEDEFS,          // SideDefs, from editing
  ML_VERTEXES,          // Vertices, edited and BSP splits generated
  ML_SEGS,              // LineSegs, from LineDefs split by BSP
  ML_SSECTORS,          // SubSectors, list of LineSegs
  ML_NODES,             // BSP nodes
  ML_SECTORS,           // Sectors, from editing
  ML_REJECT,            // LUT, sector-sector visibility
  ML_BLOCKMAP           // LUT, motion clipping, walls/grid element
};

#ifdef _MSC_VER
#define GCC_PACKED
#else //_MSC_VER
/* we might want to make this sensitive to whether configure has detected
   a build with GCC */
#define GCC_PACKED __attribute__((packed))
#endif //_MSC_VER

#ifdef _MSC_VER // proff: This is the same as __attribute__ ((packed)) in GNUC
#pragma pack(push)
#pragma pack(1)
#endif //_MSC_VER

// A single Vertex.
typedef struct {
  short x,y GCC_PACKED;
} GCC_PACKED mapvertex_t;

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct {
  short textureoffset GCC_PACKED;
  short rowoffset GCC_PACKED;
  char  toptexture[8] GCC_PACKED;
  char  bottomtexture[8] GCC_PACKED;
  char  midtexture[8] GCC_PACKED;
  short sector GCC_PACKED;  // Front sector, towards viewer.
} GCC_PACKED mapsidedef_t;

// A LineDef, as used for editing, and as input to the BSP builder.

typedef struct {
  short v1 GCC_PACKED;
  short v2 GCC_PACKED;
  short flags GCC_PACKED;
  short special GCC_PACKED;
  short tag GCC_PACKED;
  short sidenum[2] GCC_PACKED;  // sidenum[1] will be -1 if one sided
} GCC_PACKED maplinedef_t;

//
// LineDef attributes.
//

// Solid, is an obstacle.
#define ML_BLOCKING             1

// Blocks monsters only.
#define ML_BLOCKMONSTERS        2

// Backside will not be drawn if not two sided.
#define ML_TWOSIDED             4

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures always have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
#define ML_DONTPEGTOP           8

// lower texture unpegged
#define ML_DONTPEGBOTTOM        16

// In AutoMap: don't map as two sided: IT'S A SECRET!
#define ML_SECRET               32

// Sound rendering: don't let sound cross two of these.
#define ML_SOUNDBLOCK           64

// Don't draw on the automap at all.
#define ML_DONTDRAW             128

// Set if already seen, thus drawn in automap.
#define ML_MAPPED               256

//jff 3/21/98 Set if line absorbs use by player
//allow multiple push/switch triggers to be used on one push
#define ML_PASSUSE      512

// Sector definition, from editing.
typedef struct {
  short floorheight GCC_PACKED;
  short ceilingheight GCC_PACKED;
  char  floorpic[8] GCC_PACKED;
  char  ceilingpic[8] GCC_PACKED;
  short lightlevel GCC_PACKED;
  short special GCC_PACKED;
  short tag GCC_PACKED;
} GCC_PACKED mapsector_t;

// SubSector, as generated by BSP.
typedef struct {
  unsigned short numsegs GCC_PACKED;
  unsigned short firstseg GCC_PACKED;    // Index of first one; segs are stored sequentially.
} GCC_PACKED mapsubsector_t;

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct {
  short v1 GCC_PACKED;
  short v2 GCC_PACKED;
  short angle GCC_PACKED;
  short linedef GCC_PACKED;
  short side GCC_PACKED;
  short offset GCC_PACKED;
} GCC_PACKED mapseg_t;

// BSP node structure.

// Indicate a leaf.
#define NF_SUBSECTOR    0x8000

typedef struct {
  short x GCC_PACKED;  // Partition line from (x,y) to x+dx,y+dy)
  short y GCC_PACKED;
  short dx GCC_PACKED;
  short dy GCC_PACKED;
  // Bounding box for each child, clip against view frustum.
  short bbox[2][4] GCC_PACKED;
  // If NF_SUBSECTOR its a subsector, else it's a node of another subtree.
  unsigned short children[2] GCC_PACKED;
} GCC_PACKED mapnode_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
typedef struct {
  short x GCC_PACKED;
  short y GCC_PACKED;
  short angle GCC_PACKED;
  short type GCC_PACKED;
  short options GCC_PACKED;
} GCC_PACKED mapthing_t;

#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER

#endif // __DOOMDATA__
