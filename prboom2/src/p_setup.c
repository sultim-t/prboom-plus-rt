/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *  Do all the WAD I/O, get map description,
 *  set up initial state and misc. LUTs.
 *
 *-----------------------------------------------------------------------------*/

#include <math.h>

#include "doomstat.h"
#include "m_bbox.h"
#include "m_argv.h"
#include "g_game.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_things.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_enemy.h"
#include "s_sound.h"
#include "s_advsound.h"
#include "lprintf.h" //jff 10/6/98 for debug outputs
#include "v_video.h"
#include "r_demo.h"
#include "r_fps.h"
#include "hu_tracers.h"
#include "g_overflow.h"
#include "am_map.h"
#include "e6y.h"//e6y

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

int      numvertexes;
vertex_t *vertexes;

int      numsegs;
seg_t    *segs;

int      numsectors;
sector_t *sectors;

int      numsubsectors;
subsector_t *subsectors;

int      numnodes;
node_t   *nodes;

int      numlines;
line_t   *lines;

int      numsides;
side_t   *sides;

int      *sslines_indexes;
ssline_t *sslines;

byte     *map_subsectors;

////////////////////////////////////////////////////////////////////////////////////////////
// figgi 08/21/00 -- constants and globals for glBsp support
#define gNd2            0x32644E67  // figgi -- suppport for new GL_VERT format v2.0
#define gNd3            0x33644E67
#define gNd4            0x34644E67
#define gNd5            0x35644E67
#define ZNOD            0x444F4E5A
#define ZGLN            0x4E4C475A
#define GL_VERT_OFFSET  4

int     firstglvertex = 0;
int     nodesVersion  = 0;
dboolean forceOldBsp   = false;

// figgi 08/21/00 -- glSegs
typedef struct
{
  unsigned short  v1;    // start vertex    (16 bit)
  unsigned short  v2;    // end vertex      (16 bit)
  unsigned short  linedef; // linedef, or -1 for minisegs
  short     side;  // side on linedef: 0 for right, 1 for left
  unsigned short  partner; // corresponding partner seg, or -1 on one-sided walls
} glseg_t;

// fixed 32 bit gl_vert format v2.0+ (glBsp 1.91)
typedef struct
{
  fixed_t x,y;
} mapglvertex_t;

enum
{
   ML_GL_LABEL=0,  // A separator name, GL_ExMx or GL_MAPxx
   ML_GL_VERTS,     // Extra Vertices
   ML_GL_SEGS,     // Segs, from linedefs & minisegs
   ML_GL_SSECT,    // SubSectors, list of segs
   ML_GL_NODES     // GL BSP nodes
};
////////////////////////////////////////////////////////////////////////////////////////////


// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.

int       bmapwidth, bmapheight;  // size in mapblocks

// killough 3/1/98: remove blockmap limit internally:
int       *blockmap;              // was short -- killough

// offsets in blockmap are from here
int       *blockmaplump;          // was short -- killough

fixed_t   bmaporgx, bmaporgy;     // origin of block map

mobj_t    **blocklinks;           // for thing chains

// MAES: extensions to support 512x512 blockmaps.
// They represent the maximum negative number which represents
// a positive offset, otherwise they are left at -257, which
// never triggers a check.
// If a blockmap index is ever LE than either, then
// its actual value is to be interpreted as 0x01FF&x.
// Full 512x512 blockmaps get this value set to -1.
// A 511x511 blockmap would still have a valid negative number
// e.g. -1..510, so they would be set to -2
// Non-extreme maps remain unaffected.
int blockmapxneg = -257;
int blockmapyneg = -257;

//
// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without the special effect, this could
// be used as a PVS lookup as well.
//

static int rejectlump = -1;// cph - store reject lump num if cached
const byte *rejectmatrix; // cph - const*

// Maintain single and multi player starting spots.

// 1/11/98 killough: Remove limit on deathmatch starts
mapthing_t *deathmatchstarts;      // killough
size_t     num_deathmatchstarts;   // killough

mapthing_t *deathmatch_p;
mapthing_t playerstarts[MAXPLAYERS];

static int current_episode = -1;
static int current_map = -1;
static int current_nodesVersion = -1;
static int samelevel = false;

// e6y: Smart malloc
// Used by P_SetupLevel() for smart data loading
// Do nothing if level is the same
static void *malloc_IfSameLevel(void* p, size_t size)
{
  if (!samelevel || !p)
  {
    return malloc(size);
  }
  return p;
}

// e6y: Smart calloc
// Used by P_SetupLevel() for smart data loading
// Clear the memory without allocation if level is the same
static void *calloc_IfSameLevel(void* p, size_t n1, size_t n2)
{
  if (!samelevel)
  {
    return calloc(n1, n2);
  }
  else
  {
    memset(p, 0, n1 * n2);
    return p;
  }
}

//
// CheckForIdentifier
// Checks a lump for a magic string to identify its type (e.g. extended nodes)
//

static dboolean CheckForIdentifier(int lumpnum, const byte *id, size_t length)
{
  dboolean result = false;

  if (W_LumpLength(lumpnum) >= length)
  {
    const char *data = W_CacheLumpNum(lumpnum);

    if (!memcmp(data, id, length))
      result = true;

    W_UnlockLumpNum(lumpnum);
  }

  return result;
}

//
// P_CheckForZDoomNodes
//

static dboolean P_CheckForZDoomNodes(int lumpnum, int gl_lumpnum)
{
  if (CheckForIdentifier(lumpnum + ML_NODES, "ZNOD", 4))
    I_Error("P_CheckForZDoomNodes: compressed ZDoom nodes not supported yet");

  if (CheckForIdentifier(lumpnum + ML_SSECTORS, "ZGLN", 4))
    I_Error("P_CheckForZDoomNodes: ZDoom GL nodes not supported yet");

  return false;
}

//
// P_CheckForDeePBSPv4Nodes
// http://www.sbsoftware.com/files/DeePBSPV4specs.txt
//

static dboolean P_CheckForDeePBSPv4Nodes(int lumpnum, int gl_lumpnum)
{
  int result = CheckForIdentifier(lumpnum + ML_NODES, "xNd4\0\0\0\0", 8);

  if (result)
    lprintf(LO_INFO, "P_CheckForDeePBSPv4Nodes: DeePBSP v4 Extended nodes are detected\n");

  return result;
}

//
// P_CheckForZDoomUncompressedNodes
// http://zdoom.org/wiki/ZDBSP#Compressed_Nodes
//

static int P_CheckForZDoomUncompressedNodes(int lumpnum, int gl_lumpnum)
{
  int result = CheckForIdentifier(lumpnum + ML_NODES, "XNOD", 4);

  if (result)
    lprintf(LO_INFO, "P_CheckForZDoomUncompressedNodes: ZDoom uncompressed normal nodes are detected\n");

  return result;
}

//
// P_GetNodesVersion
//

static void P_GetNodesVersion(int lumpnum, int gl_lumpnum)
{
  int ver = -1;
  nodesVersion = 0;

  if ( (gl_lumpnum > lumpnum) && (forceOldBsp == false) && (compatibility_level >= prboom_2_compatibility) )
  {
    const void *data = W_CacheLumpNum(gl_lumpnum+ML_GL_VERTS);
    if (*(const int *)data == gNd2) {
      data = W_CacheLumpNum(gl_lumpnum+ML_GL_SEGS);
      if (*(const int *)data == gNd3) {
        ver = 3;
      } else {
        nodesVersion = gNd2;
        lprintf(LO_DEBUG, "P_GetNodesVersion: found version 2 nodes\n");
      }
    }
    if (*(const int *)data == gNd4) {
      ver = 4;
    }
    if (*(const int *)data == gNd5) {
      ver = 5;
    }
    //e6y: unknown gl nodes will be ignored
    if (nodesVersion == 0 && ver != -1)
    {
      lprintf(LO_DEBUG,"P_GetNodesVersion: found version %d nodes\n", ver);
      lprintf(LO_DEBUG,"P_GetNodesVersion: version %d nodes not supported\n", ver);
    }
  } else {
    nodesVersion = 0;
    lprintf(LO_DEBUG,"P_GetNodesVersion: using normal BSP nodes\n");
    if (P_CheckForZDoomNodes(lumpnum, gl_lumpnum))
      I_Error("P_GetNodesVersion: ZDoom nodes not supported yet");
  }
}

//
// P_LoadVertexes
//
// killough 5/3/98: reformatted, cleaned up
//
static void P_LoadVertexes (int lump)
{
  const mapvertex_t *data; // cph - const
  int i;

  // Determine number of lumps:
  //  total lump length / vertex record length.
  numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

  // Allocate zone memory for buffer.
  vertexes = calloc_IfSameLevel(vertexes, numvertexes, sizeof(vertex_t));

  // Load data into cache.
  // cph 2006/07/29 - cast to mapvertex_t here, making the loop below much neater
  data = (const mapvertex_t *)W_CacheLumpNum(lump);

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i=0; i<numvertexes; i++)
    {
      vertexes[i].x = LittleShort(data[i].x)<<FRACBITS;
      vertexes[i].y = LittleShort(data[i].y)<<FRACBITS;
    }

  // Free buffer memory.
  W_UnlockLumpNum(lump);
}

/*******************************************
 * Name     : P_LoadVertexes2        *
 * modified : 09/18/00, adapted for PrBoom *
 * author   : figgi              *
 * what   : support for gl nodes       *
 *******************************************/

// figgi -- FIXME: Automap showes wrong zoom boundaries when starting game
//           when P_LoadVertexes2 is used with classic BSP nodes.

static void P_LoadVertexes2(int lump, int gllump)
{
  const byte         *gldata;
  int                 i;
  const mapvertex_t*  ml;

  firstglvertex = W_LumpLength(lump) / sizeof(mapvertex_t);
  numvertexes   = W_LumpLength(lump) / sizeof(mapvertex_t);

  if (gllump >= 0)  // check for glVertices
  {
    gldata = W_CacheLumpNum(gllump);

    if (nodesVersion == gNd2) // 32 bit GL_VERT format (16.16 fixed)
    {
      const mapglvertex_t*  mgl;

      numvertexes += (W_LumpLength(gllump) - GL_VERT_OFFSET)/sizeof(mapglvertex_t);
      vertexes = malloc_IfSameLevel(vertexes, numvertexes * sizeof(vertex_t));
      mgl      = (const mapglvertex_t *) (gldata + GL_VERT_OFFSET);

      for (i = firstglvertex; i < numvertexes; i++)
      {
        vertexes[i].x = mgl->x;
        vertexes[i].y = mgl->y;
        mgl++;
      }
    }
    else
    {
      numvertexes += W_LumpLength(gllump)/sizeof(mapvertex_t);
      vertexes = malloc_IfSameLevel(vertexes, numvertexes * sizeof(vertex_t));
      ml       = (const mapvertex_t *)gldata;

      for (i = firstglvertex; i < numvertexes; i++)
      {
        vertexes[i].x = LittleShort(ml->x)<<FRACBITS;
        vertexes[i].y = LittleShort(ml->y)<<FRACBITS;
        ml++;
      }
    }
    W_UnlockLumpNum(gllump);
  }

  ml = (const mapvertex_t*) W_CacheLumpNum(lump);

  for (i=0; i < firstglvertex; i++)
  {
    vertexes[i].x = LittleShort(ml->x)<<FRACBITS;
    vertexes[i].y = LittleShort(ml->y)<<FRACBITS;
    ml++;
  }
  W_UnlockLumpNum(lump);
}


/*******************************************
 * created  : 08/13/00             *
 * modified : 09/18/00, adapted for PrBoom *
 * author   : figgi              *
 * what   : basic functions needed for   *
 *            computing  gl nodes      *
 *******************************************/

static int checkGLVertex(int num)
{
  if (num & 0x8000)
    num = (num&0x7FFF)+firstglvertex;
  return num;
}


static float GetDistance(int dx, int dy)
{
  float fx = (float)(dx)/FRACUNIT, fy = (float)(dy)/FRACUNIT;
  return (float)sqrt(fx*fx + fy*fy);
}

static float GetTexelDistance(int dx, int dy)
{
  //return (float)((int)(GetDistance(dx, dy) + 0.5f));
  float fx = (float)(dx)/FRACUNIT, fy = (float)(dy)/FRACUNIT;
  return (float)((int)(0.5f + (float)sqrt(fx*fx + fy*fy)));
}

static int GetOffset(vertex_t *v1, vertex_t *v2)
{
  float a, b;
  int r;
  a = (float)(v1->x - v2->x) / (float)FRACUNIT;
  b = (float)(v1->y - v2->y) / (float)FRACUNIT;
  r = (int)(sqrt(a*a+b*b) * (float)FRACUNIT);
  return r;
}



//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSegs (int lump)
{
  int  i;
  const mapseg_t *data; // cph - const

  numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
  segs = calloc_IfSameLevel(segs, numsegs, sizeof(seg_t));
  data = (const mapseg_t *)W_CacheLumpNum(lump); // cph - wad lump handling updated

  if ((!data) || (!numsegs))
    I_Error("P_LoadSegs: no segs in level");

  for (i=0; i<numsegs; i++)
    {
      seg_t *li = segs+i;
      const mapseg_t *ml = data + i;
      unsigned short v1, v2;

      int side, linedef;
      line_t *ldef;

      v1 = (unsigned short)LittleShort(ml->v1);
      v2 = (unsigned short)LittleShort(ml->v2);

      // e6y
      // moved down for additional checks to avoid overflow
      // if wrong vertexe's indexes are in SEGS lump
      // see below for more detailed information
      //li->v1 = &vertexes[v1];
      //li->v2 = &vertexes[v2];

      li->miniseg = false; // figgi -- there are no minisegs in classic BSP nodes

      // e6y: moved down, see below
      //li->length  = GetDistance(li->v2->x - li->v1->x, li->v2->y - li->v1->y);

      li->angle = (LittleShort(ml->angle))<<16;
      li->offset =(LittleShort(ml->offset))<<16;
      linedef = (unsigned short)LittleShort(ml->linedef);

      //e6y: check for wrong indexes
      if ((unsigned)linedef >= (unsigned)numlines)
      {
        I_Error("P_LoadSegs: seg %d references a non-existent linedef %d",
          i, (unsigned)linedef);
      }

      ldef = &lines[linedef];
      li->linedef = ldef;
      side = LittleShort(ml->side);

      //e6y: fix wrong side index
      if (side != 0 && side != 1)
      {
        lprintf(LO_WARN, "P_LoadSegs: seg %d contains wrong side index %d. Replaced with 1.\n", i, side);
        side = 1;
      }

      //e6y: check for wrong indexes
      if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
      {
        I_Error("P_LoadSegs: linedef %d for seg %d references a non-existent sidedef %d",
          linedef, i, (unsigned)ldef->sidenum[side]);
      }

      li->sidedef = &sides[ldef->sidenum[side]];

      /* cph 2006/09/30 - our frontsector can be the second side of the
       * linedef, so must check for NO_INDEX in case we are incorrectly
       * referencing the back of a 1S line */
      if (ldef->sidenum[side] != NO_INDEX)
        li->frontsector = sides[ldef->sidenum[side]].sector;
      else {
        li->frontsector = 0;
        lprintf(LO_WARN, "P_LoadSegs: front of seg %i has no sidedef\n", i);
      }

      if (ldef->flags & ML_TWOSIDED)
      {
        int sidenum = ldef->sidenum[side ^ 1];

        if (sidenum == NO_INDEX)
        {
          // this is wrong
          li->backsector = GetSectorAtNullAddress();
        }
        else
        {
          li->backsector = sides[sidenum].sector;
        }
      }
      else
      {
        li->backsector = 0;
      }

      // e6y
      // check and fix wrong references to non-existent vertexes
      // see e1m9 @ NIVELES.WAD
      // http://www.doomworld.com/idgames/index.php?id=12647
      if (v1 >= numvertexes || v2 >= numvertexes)
      {
        char str[200] = 
          "P_LoadSegs: compatibility loss - seg %d references a non-existent vertex %d\n";
        
        if (demorecording)
        {
          I_Error(strcat(str, "Demo recording on levels with invalid nodes is not allowed"),
            i, (v1 >= numvertexes ? v1 : v2));
        }

        if (v1 >= numvertexes)
          lprintf(LO_WARN, str, i, v1);
        if (v2 >= numvertexes)
          lprintf(LO_WARN, str, i, v2);

        if (li->sidedef == &sides[li->linedef->sidenum[0]])
        {
          li->v1 = lines[ml->linedef].v1;
          li->v2 = lines[ml->linedef].v2;
        }
        else
        {
          li->v1 = lines[ml->linedef].v2;
          li->v2 = lines[ml->linedef].v1;
        }
      }
      else
      {
        li->v1 = &vertexes[v1];
        li->v2 = &vertexes[v2];
      }

      // Recalculate seg offsets that are sometimes incorrect
      // with certain nodebuilders. Fixes among others, line 20365
      // of DV.wad, map 5
      li->offset = GetOffset(li->v1, (ml->side ? ldef->v2 : ldef->v1));
    }

  W_UnlockLumpNum(lump); // cph - release the data
}

static void P_LoadSegs_V4(int lump)
{
  int  i;
  const mapseg_v4_t *data;

  numsegs = W_LumpLength(lump) / sizeof(mapseg_v4_t);
  segs = calloc_IfSameLevel(segs, numsegs, sizeof(seg_t));
  data = (const mapseg_v4_t *)W_CacheLumpNum(lump);

  if ((!data) || (!numsegs))
    I_Error("P_LoadSegs_V4: no segs in level");

  for (i = 0; i < numsegs; i++)
  {
    seg_t *li = segs+i;
    const mapseg_v4_t *ml = data + i;
    int v1, v2;

    int side, linedef;
    line_t *ldef;

    v1 = ml->v1;
    v2 = ml->v2;

    li->miniseg = false; // figgi -- there are no minisegs in classic BSP nodes

    li->angle = (LittleShort(ml->angle))<<16;
    li->offset =(LittleShort(ml->offset))<<16;
    linedef = (unsigned short)LittleShort(ml->linedef);

    //e6y: check for wrong indexes
    if ((unsigned)linedef >= (unsigned)numlines)
    {
      I_Error("P_LoadSegs_V4: seg %d references a non-existent linedef %d",
        i, (unsigned)linedef);
    }

    ldef = &lines[linedef];
    li->linedef = ldef;
    side = LittleShort(ml->side);

    //e6y: fix wrong side index
    if (side != 0 && side != 1)
    {
      lprintf(LO_WARN, "P_LoadSegs_V4: seg %d contains wrong side index %d. Replaced with 1.\n", i, side);
      side = 1;
    }

    //e6y: check for wrong indexes
    if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
    {
      I_Error("P_LoadSegs_V4: linedef %d for seg %d references a non-existent sidedef %d",
        linedef, i, (unsigned)ldef->sidenum[side]);
    }

    li->sidedef = &sides[ldef->sidenum[side]];

    /* cph 2006/09/30 - our frontsector can be the second side of the
    * linedef, so must check for NO_INDEX in case we are incorrectly
    * referencing the back of a 1S line */
    if (ldef->sidenum[side] != NO_INDEX)
    {
      li->frontsector = sides[ldef->sidenum[side]].sector;
    }
    else
    {
      li->frontsector = 0;
      lprintf(LO_WARN, "P_LoadSegs_V4: front of seg %i has no sidedef\n", i);
    }

    if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side^1]!=NO_INDEX)
      li->backsector = sides[ldef->sidenum[side^1]].sector;
    else
      li->backsector = 0;

    // e6y
    // check and fix wrong references to non-existent vertexes
    // see e1m9 @ NIVELES.WAD
    // http://www.doomworld.com/idgames/index.php?id=12647
    if (v1 >= numvertexes || v2 >= numvertexes)
    {
      char str[200] = 
        "P_LoadSegs_V4: compatibility loss - seg %d references a non-existent vertex %d\n";

      if (demorecording)
      {
        I_Error(strcat(str, "Demo recording on levels with invalid nodes is not allowed"),
          i, (v1 >= numvertexes ? v1 : v2));
      }

      if (v1 >= numvertexes)
        lprintf(LO_WARN, str, i, v1);
      if (v2 >= numvertexes)
        lprintf(LO_WARN, str, i, v2);

      if (li->sidedef == &sides[li->linedef->sidenum[0]])
      {
        li->v1 = lines[ml->linedef].v1;
        li->v2 = lines[ml->linedef].v2;
      }
      else
      {
        li->v1 = lines[ml->linedef].v2;
        li->v2 = lines[ml->linedef].v1;
      }
    }
    else
    {
      li->v1 = &vertexes[v1];
      li->v2 = &vertexes[v2];
    }

    // Recalculate seg offsets that are sometimes incorrect
    // with certain nodebuilders. Fixes among others, line 20365
    // of DV.wad, map 5
    li->offset = GetOffset(li->v1, (ml->side ? ldef->v2 : ldef->v1));
  }

  W_UnlockLumpNum(lump); // cph - release the data
}


/*******************************************
 * Name     : P_LoadGLSegs           *
 * created  : 08/13/00             *
 * modified : 09/18/00, adapted for PrBoom *
 * author   : figgi              *
 * what   : support for gl nodes       *
 *******************************************/
static void P_LoadGLSegs(int lump)
{
  int     i;
  const glseg_t   *ml;
  line_t    *ldef;

  numsegs = W_LumpLength(lump) / sizeof(glseg_t);
  segs = malloc_IfSameLevel(segs, numsegs * sizeof(seg_t));
  memset(segs, 0, numsegs * sizeof(seg_t));
  ml = (const glseg_t*)W_CacheLumpNum(lump);

  if ((!ml) || (!numsegs))
    I_Error("P_LoadGLSegs: no glsegs in level");

  for(i = 0; i < numsegs; i++)
  {             // check for gl-vertices
    segs[i].v1 = &vertexes[checkGLVertex(LittleShort(ml->v1))];
    segs[i].v2 = &vertexes[checkGLVertex(LittleShort(ml->v2))];

    if(ml->linedef != (unsigned short)-1) // skip minisegs
    {
      ldef = &lines[ml->linedef];
      segs[i].linedef = ldef;
      segs[i].miniseg = false;
      segs[i].angle = R_PointToAngle2(segs[i].v1->x,segs[i].v1->y,segs[i].v2->x,segs[i].v2->y);

      segs[i].sidedef = &sides[ldef->sidenum[ml->side]];
      segs[i].frontsector = sides[ldef->sidenum[ml->side]].sector;
      if (ldef->flags & ML_TWOSIDED)
        segs[i].backsector = sides[ldef->sidenum[ml->side^1]].sector;
      else
        segs[i].backsector = 0;

      if (ml->side)
        segs[i].offset = GetOffset(segs[i].v1, ldef->v2);
      else
        segs[i].offset = GetOffset(segs[i].v1, ldef->v1);
    }
    else
    {
      segs[i].miniseg = true;
      segs[i].angle  = 0;
      segs[i].offset  = 0;
      segs[i].linedef = NULL;
      segs[i].sidedef = NULL;
      segs[i].frontsector = NULL;
      segs[i].backsector  = NULL;
    }
    ml++;
  }
  W_UnlockLumpNum(lump);
}

//
// P_LoadSubsectors
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSubsectors (int lump)
{
  /* cph 2006/07/29 - make data a const mapsubsector_t *, so the loop below is simpler & gives no constness warnings */
  const mapsubsector_t *data;
  int  i;

  numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
  subsectors = calloc_IfSameLevel(subsectors, numsubsectors, sizeof(subsector_t));
  data = (const mapsubsector_t *)W_CacheLumpNum(lump);

  if ((!data) || (!numsubsectors))
    I_Error("P_LoadSubsectors: no subsectors in level");

  for (i=0; i<numsubsectors; i++)
  {
    // e6y: support for extended nodes
    subsectors[i].numlines  = (unsigned short)LittleShort(data[i].numsegs );
    subsectors[i].firstline = (unsigned short)LittleShort(data[i].firstseg);
  }

  W_UnlockLumpNum(lump); // cph - release the data
}

static void P_LoadSubsectors_V4(int lump)
{
  /* cph 2006/07/29 - make data a const mapsubsector_t *, so the loop below is simpler & gives no constness warnings */
  const mapsubsector_v4_t *data;
  int i;

  numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_v4_t);
  subsectors = calloc_IfSameLevel(subsectors, numsubsectors, sizeof(subsector_t));
  data = (const mapsubsector_v4_t *)W_CacheLumpNum(lump);

  if ((!data) || (!numsubsectors))
    I_Error("P_LoadSubsectors_V4: no subsectors in level");

  for (i = 0; i < numsubsectors; i++)
  {
    subsectors[i].numlines = (int)data[i].numsegs;
    subsectors[i].firstline = (int)data[i].firstseg;
  }

  W_UnlockLumpNum(lump); // cph - release the data
}

//
// P_LoadSectors
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSectors (int lump)
{
  const byte *data; // cph - const*
  int  i;

  numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
  sectors = calloc_IfSameLevel(sectors, numsectors, sizeof(sector_t));
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  for (i=0; i<numsectors; i++)
    {
      sector_t *ss = sectors + i;
      const mapsector_t *ms = (const mapsector_t *) data + i;

      ss->iSectorID=i; // proff 04/05/2000: needed for OpenGL
      ss->floorheight = LittleShort(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = LittleShort(ms->ceilingheight)<<FRACBITS;
      ss->floorpic = R_FlatNumForName(ms->floorpic);
      ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
      ss->lightlevel = LittleShort(ms->lightlevel);
      ss->special = LittleShort(ms->special);
      ss->oldspecial = LittleShort(ms->special);
      ss->tag = LittleShort(ms->tag);
      ss->thinglist = NULL;
      ss->touching_thinglist = NULL;            // phares 3/14/98

      ss->nextsec = -1; //jff 2/26/98 add fields to support locking out
      ss->prevsec = -1; // stair retriggering until build completes

      // killough 3/7/98:
      ss->floor_xoffs = 0;
      ss->floor_yoffs = 0;      // floor and ceiling flats offsets
      ss->ceiling_xoffs = 0;
      ss->ceiling_yoffs = 0;
      ss->heightsec = -1;       // sector used to get floor and ceiling height
      ss->floorlightsec = -1;   // sector used to get floor lighting
      // killough 3/7/98: end changes

      // killough 4/11/98 sector used to get ceiling lighting:
      ss->ceilinglightsec = -1;

      // killough 4/4/98: colormaps:
      ss->bottommap = ss->midmap = ss->topmap = 0;

      // killough 10/98: sky textures coming from sidedefs:
      ss->sky = 0;

      // [kb] For R_WiggleFix
      ss->cachedheight = 0;
      ss->scaleindex = 0;
    }

  W_UnlockLumpNum(lump); // cph - release the data
}


//
// P_LoadNodes
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadNodes (int lump)
{
  const byte *data; // cph - const*
  int  i;

  numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
  nodes = malloc_IfSameLevel(nodes, numnodes * sizeof(node_t));
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  if ((!data) || (!numnodes))
  {
    // allow trivial maps
    if (numsubsectors == 1)
      lprintf(LO_INFO,
          "P_LoadNodes: trivial map (no nodes, one subsector)\n");
    else
      I_Error("P_LoadNodes: no nodes in level");
  }

  for (i=0; i<numnodes; i++)
    {
      node_t *no = nodes + i;
      const mapnode_t *mn = (const mapnode_t *) data + i;
      int j;

      no->x = LittleShort(mn->x)<<FRACBITS;
      no->y = LittleShort(mn->y)<<FRACBITS;
      no->dx = LittleShort(mn->dx)<<FRACBITS;
      no->dy = LittleShort(mn->dy)<<FRACBITS;

      for (j=0 ; j<2 ; j++)
        {
          int k;
          // e6y: support for extended nodes
          no->children[j] = (unsigned short)LittleShort(mn->children[j]);

          // e6y: support for extended nodes
          if (no->children[j] == 0xFFFF)
          {
            no->children[j] = -1;
          }
          else if (no->children[j] & 0x8000)
          {
            // Convert to extended type
            no->children[j] &= ~0x8000;

            // haleyjd 11/06/10: check for invalid subsector reference
            if(no->children[j] >= numsubsectors)
            {
              lprintf(LO_ERROR, "P_LoadNodes: BSP tree references invalid subsector %d.\n", no->children[j]);
              no->children[j] = 0;
            }

            no->children[j] |= NF_SUBSECTOR;
          }

          for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = LittleShort(mn->bbox[j][k])<<FRACBITS;
        }
    }

  W_UnlockLumpNum(lump); // cph - release the data
}

static void P_LoadNodes_V4(int lump)
{
  const byte *data; // cph - const*
  int  i;

  numnodes = (W_LumpLength (lump) - 8) / sizeof(mapnode_v4_t);
  nodes = malloc_IfSameLevel(nodes, numnodes * sizeof(node_t));
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  // skip header
  data = data + 8;

  if ((!data) || (!numnodes))
  {
    // allow trivial maps
    if (numsubsectors == 1)
      lprintf(LO_INFO, "P_LoadNodes_V4: trivial map (no nodes, one subsector)\n");
    else
      I_Error("P_LoadNodes_V4: no nodes in level");
  }

  for (i = 0; i < numnodes; i++)
    {
      node_t *no = nodes + i;
      const mapnode_v4_t *mn = (const mapnode_v4_t *) data + i;
      int j;

      no->x = LittleShort(mn->x)<<FRACBITS;
      no->y = LittleShort(mn->y)<<FRACBITS;
      no->dx = LittleShort(mn->dx)<<FRACBITS;
      no->dy = LittleShort(mn->dy)<<FRACBITS;

      for (j=0 ; j<2 ; j++)
        {
          int k;
          no->children[j] = (unsigned int)(mn->children[j]);

          for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = LittleShort(mn->bbox[j][k])<<FRACBITS;
        }
    }

  W_UnlockLumpNum(lump); // cph - release the data
}

static void CheckZNodesOverflow(int *size, int count)
{
  (*size) -= count;

  if ((*size) < 0)
  {
    I_Error("P_LoadZNodes: incorrect nodes");
  }
}

static void P_LoadZSegs (const byte *data)
{
  int i;

  for (i = 0; i < numsegs; i++)
  {
    line_t *ldef;
    unsigned int v1, v2;
    unsigned int linedef;
    unsigned char side;
    seg_t *li = segs+i;
    const mapseg_znod_t *ml = (const mapseg_znod_t *) data + i;

    v1 = ml->v1;
    v2 = ml->v2;

    li->miniseg = false;

    linedef = (unsigned short)LittleShort(ml->linedef);

    //e6y: check for wrong indexes
    if ((unsigned int)linedef >= (unsigned int)numlines)
    {
      I_Error("P_LoadZSegs: seg %d references a non-existent linedef %d",
        i, (unsigned)linedef);
    }

    ldef = &lines[linedef];
    li->linedef = ldef;
    side = ml->side;

    //e6y: fix wrong side index
    if (side != 0 && side != 1)
    {
      lprintf(LO_WARN, "P_LoadZSegs: seg %d contains wrong side index %d. Replaced with 1.\n", i, side);
      side = 1;
    }

    //e6y: check for wrong indexes
    if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
    {
      I_Error("P_LoadZSegs: linedef %d for seg %d references a non-existent sidedef %d",
        linedef, i, (unsigned)ldef->sidenum[side]);
    }

    li->sidedef = &sides[ldef->sidenum[side]];

    /* cph 2006/09/30 - our frontsector can be the second side of the
    * linedef, so must check for NO_INDEX in case we are incorrectly
    * referencing the back of a 1S line */
    if (ldef->sidenum[side] != NO_INDEX)
    {
      li->frontsector = sides[ldef->sidenum[side]].sector;
    }
    else
    {
      li->frontsector = 0;
      lprintf(LO_WARN, "P_LoadZSegs: front of seg %i has no sidedef\n", i);
    }

    if ((ldef->flags & ML_TWOSIDED) && (ldef->sidenum[side^1] != NO_INDEX))
      li->backsector = sides[ldef->sidenum[side^1]].sector;
    else
      li->backsector = 0;

    li->v1 = &vertexes[v1];
    li->v2 = &vertexes[v2];

    li->offset = GetOffset(li->v1, (side ? ldef->v2 : ldef->v1));
    li->angle = R_PointToAngle2(segs[i].v1->x, segs[i].v1->y, segs[i].v2->x, segs[i].v2->y);
    //li->angle = (int)((float)atan2(li->v2->y - li->v1->y,li->v2->x - li->v1->x) * (ANG180 / M_PI));
  }
}

static void P_LoadZNodes(int lump, int glnodes)
{
  const byte *data;
  unsigned int i;
  int len;

  unsigned int orgVerts, newVerts;
  unsigned int numSubs, currSeg;
  unsigned int numSegs;
  unsigned int numNodes;
  vertex_t *newvertarray = NULL;

  data = W_CacheLumpNum(lump);
  len =  W_LumpLength(lump);
  
  // skip header
  CheckZNodesOverflow(&len, 4);
  data += 4;

  // Read extra vertices added during node building
  CheckZNodesOverflow(&len, sizeof(orgVerts));
  orgVerts = *((const unsigned int*)data);
  data += sizeof(orgVerts);

  CheckZNodesOverflow(&len, sizeof(newVerts));
  newVerts = *((const unsigned int*)data);
  data += sizeof(newVerts);

  if (!samelevel)
  {
    if (orgVerts + newVerts == (unsigned int)numvertexes)
    {
      newvertarray = vertexes;
    }
    else
    {
      newvertarray = calloc(orgVerts + newVerts, sizeof(vertex_t));
      memcpy (newvertarray, vertexes, orgVerts * sizeof(vertex_t));
    }

    CheckZNodesOverflow(&len, newVerts * (sizeof(newvertarray[0].x) + sizeof(newvertarray[0].y)));
    for (i = 0; i < newVerts; i++)
    {
      newvertarray[i + orgVerts].x = *((const unsigned int*)data);
      data += sizeof(newvertarray[0].x);

      newvertarray[i + orgVerts].y = *((const unsigned int*)data);
      data += sizeof(newvertarray[0].y);
    }

    if (vertexes != newvertarray)
    {
      for (i = 0; i < (unsigned int)numlines; i++)
      {
        lines[i].v1 = lines[i].v1 - vertexes + newvertarray;
        lines[i].v2 = lines[i].v2 - vertexes + newvertarray;
      }
      free(vertexes);
      vertexes = newvertarray;
      numvertexes = orgVerts + newVerts;
    }
  }
  else
  {
    int size = newVerts * (sizeof(newvertarray[0].x) + sizeof(newvertarray[0].y));
    CheckZNodesOverflow(&len, size);
    data += size;

    // P_LoadVertexes reset numvertexes, need to increase it again
    numvertexes = orgVerts + newVerts;
  }

  // Read the subsectors
  CheckZNodesOverflow(&len, sizeof(numSubs));
  numSubs = *((const unsigned int*)data);
  data += sizeof(numSubs);

  numsubsectors = numSubs;
  if (numsubsectors <= 0)
    I_Error("P_LoadZNodes: no subsectors in level");
  subsectors = calloc_IfSameLevel(subsectors, numsubsectors, sizeof(subsector_t));

  CheckZNodesOverflow(&len, numSubs * sizeof(mapsubsector_znod_t));
  for (i = currSeg = 0; i < numSubs; i++)
  {
    const mapsubsector_znod_t *mseg = (const mapsubsector_znod_t *) data + i;

    subsectors[i].firstline = currSeg;
    subsectors[i].numlines = mseg->numsegs;
    currSeg += mseg->numsegs;
  }
  data += numSubs * sizeof(mapsubsector_znod_t);

  // Read the segs
  CheckZNodesOverflow(&len, sizeof(numSegs));
  numSegs = *((const unsigned int*)data);
  data += sizeof(numSegs);

  // The number of segs stored should match the number of
  // segs used by subsectors.
  if (numSegs != currSeg)
  {
    I_Error("P_LoadZNodes: Incorrect number of segs in nodes.");
  }

  numsegs = numSegs;
  segs = calloc_IfSameLevel(segs, numsegs, sizeof(seg_t));

  if (glnodes == 0)
  {
    CheckZNodesOverflow(&len, numsegs * sizeof(mapseg_znod_t));
    P_LoadZSegs(data);
    data += numsegs * sizeof(mapseg_znod_t);
  }
  else
  {
    //P_LoadGLZSegs (data, glnodes);
    I_Error("P_LoadZNodes: GL segs are not supported.");
  }

  // Read nodes
  CheckZNodesOverflow(&len, sizeof(numNodes));
  numNodes = *((const unsigned int*)data);
  data += sizeof(numNodes);

  numnodes = numNodes;
  nodes = calloc_IfSameLevel(nodes, numNodes, sizeof(node_t));

  CheckZNodesOverflow(&len, numNodes * sizeof(mapnode_znod_t));
  for (i = 0; i < numNodes; i++)
  {
    int j, k;
    node_t *no = nodes + i;
    const mapnode_znod_t *mn = (const mapnode_znod_t *) data + i;

    no->x = LittleShort(mn->x)<<FRACBITS;
    no->y = LittleShort(mn->y)<<FRACBITS;
    no->dx = LittleShort(mn->dx)<<FRACBITS;
    no->dy = LittleShort(mn->dy)<<FRACBITS;

    for (j = 0; j < 2; j++)
    {
      no->children[j] = (unsigned int)(mn->children[j]);

      for (k = 0; k < 4; k++)
        no->bbox[j][k] = LittleShort(mn->bbox[j][k])<<FRACBITS;
    }
  }

  W_UnlockLumpNum(lump); // cph - release the data
}

#ifdef GL_DOOM
static int no_overlapped_sprites;
#define GETXY(mobj) (mobj->x + (mobj->y >> 16))
static int C_DECL dicmp_sprite_by_pos(const void *a, const void *b)
{
  const mobj_t *m1 = (*((const mobj_t * const *)a));
  const mobj_t *m2 = (*((const mobj_t * const *)b));
  
  int res = GETXY(m2) - GETXY(m1);
  no_overlapped_sprites = no_overlapped_sprites && res;
  return res;
}
#endif

/*
 * P_LoadThings
 *
 * killough 5/3/98: reformatted, cleaned up
 * cph 2001/07/07 - don't write into the lump cache, especially non-idepotent
 * changes like byte order reversals. Take a copy to edit.
 */

static void P_LoadThings (int lump)
{
  int  i, numthings = W_LumpLength (lump) / sizeof(mapthing_t);
  const mapthing_t *data = W_CacheLumpNum (lump);

  mobj_t *mobj;
  int mobjcount = 0;
  mobj_t **mobjlist = malloc(numthings * sizeof(mobjlist[0]));

  if ((!data) || (!numthings))
    I_Error("P_LoadThings: no things in level");

  for (i=0; i<numthings; i++)
    {
      mapthing_t mt = data[i];

      mt.x = LittleShort(mt.x);
      mt.y = LittleShort(mt.y);
      mt.angle = LittleShort(mt.angle);
      mt.type = LittleShort(mt.type);
      mt.options = LittleShort(mt.options);

      if (!P_IsDoomnumAllowed(mt.type))
        continue;

      // Although all resources of the Wolf SS have been removed
      // off the BFG Edition, there is still one left in MAP33.
      // Replace with a Former Human instead.
      if (bfgedition && singleplayer && mt.type == 84)
        mt.type = 3004;

      // Do spawn all other stuff.
      mobj = P_SpawnMapThing(&mt, i);
      if (mobj && mobj->info->speed == 0)
        mobjlist[mobjcount++] = mobj;
    }

  W_UnlockLumpNum(lump); // cph - release the data

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL)
  {
    no_overlapped_sprites = true;
    qsort(mobjlist, mobjcount, sizeof(mobjlist[0]), dicmp_sprite_by_pos);
    if (!no_overlapped_sprites)
    {
      i = 1;
      while (i < mobjcount)
      {
        mobj_t *m1 = mobjlist[i - 1];
        mobj_t *m2 = mobjlist[i - 0];

        if (GETXY(m1) == GETXY(m2))
        {
          mobj_t *mo = (m1->index < m2->index ? m1 : m2);
          i++;
          while (i < mobjcount && GETXY(mobjlist[i]) == GETXY(m1))
          {
            if (mobjlist[i]->index < mo->index)
            {
              mo = mobjlist[i];
            }
            i++;
          }

          // 'nearest'
          mo->flags |= MF_FOREGROUND;
        }
        i++;
      }
    }
  }
#endif

  free(mobjlist);
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//        ^^^
// ??? killough ???
// Does this mean secrets used to be linedef-based, rather than sector-based?
//
// killough 4/4/98: split into two functions, to allow sidedef overloading
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadLineDefs (int lump)
{
  const byte *data; // cph - const*
  int  i;

  numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
  lines = calloc_IfSameLevel(lines, numlines, sizeof(line_t));
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  for (i=0; i<numlines; i++)
    {
      const maplinedef_t *mld = (const maplinedef_t *) data + i;
      line_t *ld = lines+i;
      vertex_t *v1, *v2;

      ld->flags = (unsigned short)LittleShort(mld->flags);
      ld->special = LittleShort(mld->special);
      ld->tag = LittleShort(mld->tag);
      v1 = ld->v1 = &vertexes[(unsigned short)LittleShort(mld->v1)];
      v2 = ld->v2 = &vertexes[(unsigned short)LittleShort(mld->v2)];
      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;
#ifdef GL_DOOM
      // e6y
      // Rounding the wall length to the nearest integer
      // when determining length instead of always rounding down
      // There is no more glitches on seams between identical textures.
      ld->texel_length = GetTexelDistance(ld->dx, ld->dy);
#endif

      ld->tranlump = -1;   // killough 4/11/98: no translucency by default

      ld->slopetype = !ld->dx ? ST_VERTICAL : !ld->dy ? ST_HORIZONTAL :
        FixedDiv(ld->dy, ld->dx) > 0 ? ST_POSITIVE : ST_NEGATIVE;

      if (v1->x < v2->x)
        {
          ld->bbox[BOXLEFT] = v1->x;
          ld->bbox[BOXRIGHT] = v2->x;
        }
      else
        {
          ld->bbox[BOXLEFT] = v2->x;
          ld->bbox[BOXRIGHT] = v1->x;
        }
      if (v1->y < v2->y)
        {
          ld->bbox[BOXBOTTOM] = v1->y;
          ld->bbox[BOXTOP] = v2->y;
        }
      else
        {
          ld->bbox[BOXBOTTOM] = v2->y;
          ld->bbox[BOXTOP] = v1->y;
        }

      /* calculate sound origin of line to be its midpoint */
      //e6y: fix sound origin for large levels
      // no need for comp_sound test, these are only used when comp_sound = 0
      ld->soundorg.x = ld->bbox[BOXLEFT] / 2 + ld->bbox[BOXRIGHT] / 2;
      ld->soundorg.y = ld->bbox[BOXTOP] / 2 + ld->bbox[BOXBOTTOM] / 2;

      ld->iLineID=i; // proff 04/05/2000: needed for OpenGL
      ld->sidenum[0] = LittleShort(mld->sidenum[0]);
      ld->sidenum[1] = LittleShort(mld->sidenum[1]);

      { 
        /* cph 2006/09/30 - fix sidedef errors right away.
         * cph 2002/07/20 - these errors are fatal if not fixed, so apply them
         * in compatibility mode - a desync is better than a crash! */
        int j;
        
        for (j=0; j < 2; j++)
        {
          if (ld->sidenum[j] != NO_INDEX && ld->sidenum[j] >= numsides) {
            ld->sidenum[j] = NO_INDEX;
            lprintf(LO_WARN, "P_LoadLineDefs: linedef %d"
                    " has out-of-range sidedef number\n", i);
          }
        }
        
        // killough 11/98: fix common wad errors (missing sidedefs):
        
        if (ld->sidenum[0] == NO_INDEX) {
          ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side
          // cph - print a warning about the bug
          lprintf(LO_WARN, "P_LoadLineDefs: linedef %d"
                  " missing first sidedef\n", i);
        }
        
        if ((ld->sidenum[1] == NO_INDEX) && (ld->flags & ML_TWOSIDED)) {
          // e6y
          // ML_TWOSIDED flag shouldn't be cleared for compatibility purposes
          // see CLNJ-506.LMP at http://doomedsda.us/wad1005.html
          MissedBackSideOverrun(ld);
          if (!demo_compatibility || !EMULATE(OVERFLOW_MISSEDBACKSIDE))
          {
            ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
          }

          // cph - print a warning about the bug
          lprintf(LO_WARN, "P_LoadLineDefs: linedef %d"
                  " has two-sided flag set, but no second sidedef\n", i);
        }
      }

      // killough 4/4/98: support special sidedef interpretation below
      if (ld->sidenum[0] != NO_INDEX && ld->special)
        sides[*ld->sidenum].special = ld->special;
    }

  W_UnlockLumpNum(lump); // cph - release the lump
}

// killough 4/4/98: delay using sidedefs until they are loaded
// killough 5/3/98: reformatted, cleaned up

static void P_LoadLineDefs2(int lump)
{
  int i = numlines;
  register line_t *ld = lines;
  for (;i--;ld++)
    {
      ld->frontsector = sides[ld->sidenum[0]].sector; //e6y: Can't be NO_INDEX here
      ld->backsector  = ld->sidenum[1]!=NO_INDEX ? sides[ld->sidenum[1]].sector : 0;
      switch (ld->special)
        {                       // killough 4/11/98: handle special types
          int lump, j;

        case 260:               // killough 4/11/98: translucent 2s textures
            transparentpresent = true;//e6y
            lump = sides[*ld->sidenum].special; // translucency from sidedef
            if (!ld->tag)                       // if tag==0,
              ld->tranlump = lump;              // affect this linedef only
            else
              for (j=0;j<numlines;j++)          // if tag!=0,
                if (lines[j].tag == ld->tag)    // affect all matching linedefs
                  lines[j].tranlump = lump;
            break;
        }
    }
}

//
// P_LoadSideDefs
//
// killough 4/4/98: split into two functions

static void P_LoadSideDefs (int lump)
{
  numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
  sides = calloc_IfSameLevel(sides, numsides, sizeof(side_t));
}

// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSideDefs2(int lump)
{
  const byte *data = W_CacheLumpNum(lump); // cph - const*, wad lump handling updated
  int  i;

  for (i=0; i<numsides; i++)
    {
      register const mapsidedef_t *msd = (const mapsidedef_t *) data + i;
      register side_t *sd = sides + i;
      register sector_t *sec;

      sd->textureoffset = LittleShort(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = LittleShort(msd->rowoffset)<<FRACBITS;

      { /* cph 2006/09/30 - catch out-of-range sector numbers; use sector 0 instead */
        unsigned short sector_num = LittleShort(msd->sector);
        if (sector_num >= numsectors) {
          lprintf(LO_WARN,"P_LoadSideDefs2: sidedef %i has out-of-range sector num %u\n", i, sector_num);
          sector_num = 0;
        }
        sd->sector = sec = &sectors[sector_num];
      }

      // killough 4/4/98: allow sidedef texture names to be overloaded
      // killough 4/11/98: refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.
      switch (sd->special)
        {
        case 242:                       // variable colormap via 242 linedef
          sd->bottomtexture =
            (sec->bottommap =   R_ColormapNumForName(msd->bottomtexture)) < 0 ?
            sec->bottommap = 0, R_TextureNumForName(msd->bottomtexture): 0 ;
          sd->midtexture =
            (sec->midmap =   R_ColormapNumForName(msd->midtexture)) < 0 ?
            sec->midmap = 0, R_TextureNumForName(msd->midtexture)  : 0 ;
          sd->toptexture =
            (sec->topmap =   R_ColormapNumForName(msd->toptexture)) < 0 ?
            sec->topmap = 0, R_TextureNumForName(msd->toptexture)  : 0 ;
          break;

        case 260: // killough 4/11/98: apply translucency to 2s normal texture
          sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
            (sd->special = W_CheckNumForName(msd->midtexture)) < 0 ||
            W_LumpLength(sd->special) != 65536 ?
            sd->special=0, R_TextureNumForName(msd->midtexture) :
              (sd->special++, 0) : (sd->special=0);
          sd->toptexture = R_TextureNumForName(msd->toptexture);
          sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
          break;

#ifdef GL_DOOM
        case 271:
        case 272:
          if (R_CheckTextureNumForName(msd->toptexture) == -1)
          {
            sd->skybox_index = R_BoxSkyboxNumForName(msd->toptexture);
          }
#endif

        default:                        // normal cases
          sd->midtexture = R_SafeTextureNumForName(msd->midtexture, i);
          sd->toptexture = R_SafeTextureNumForName(msd->toptexture, i);
          sd->bottomtexture = R_SafeTextureNumForName(msd->bottomtexture, i);
          break;
        }
    }

  W_UnlockLumpNum(lump); // cph - release the lump
}

//
// jff 10/6/98
// New code added to speed up calculation of internal blockmap
// Algorithm is order of nlines*(ncols+nrows) not nlines*ncols*nrows
//

#define blkshift 7               /* places to shift rel position for cell num */
#define blkmask ((1<<blkshift)-1)/* mask for rel position within cell */
#define blkmargin 0              /* size guardband around map used */
                                 // jff 10/8/98 use guardband>0
                                 // jff 10/12/98 0 ok with + 1 in rows,cols

typedef struct linelist_t        // type used to list lines in each block
{
  long num;
  struct linelist_t *next;
} linelist_t;

//
// Subroutine to add a line number to a block list
// It simply returns if the line is already in the block
//

static void AddBlockLine
(
  linelist_t **lists,
  int *count,
  int *done,
  int blockno,
  long lineno
)
{
  linelist_t *l;

  if (done[blockno])
    return;

  l = malloc(sizeof(linelist_t));
  l->num = lineno;
  l->next = lists[blockno];
  lists[blockno] = l;
  count[blockno]++;
  done[blockno] = 1;
}

//
// Actually construct the blockmap lump from the level data
//
// This finds the intersection of each linedef with the column and
// row lines at the left and bottom of each blockmap cell. It then
// adds the line to all block lists touching the intersection.
//

static void P_CreateBlockMap(void)
{
  int xorg,yorg;                 // blockmap origin (lower left)
  int nrows,ncols;               // blockmap dimensions
  linelist_t **blocklists=NULL;  // array of pointers to lists of lines
  int *blockcount=NULL;          // array of counters of line lists
  int *blockdone=NULL;           // array keeping track of blocks/line
  int NBlocks;                   // number of cells = nrows*ncols
  long linetotal=0;              // total length of all blocklists
  int i,j;
  int map_minx=INT_MAX;          // init for map limits search
  int map_miny=INT_MAX;
  int map_maxx=INT_MIN;
  int map_maxy=INT_MIN;

  // scan for map limits, which the blockmap must enclose

  for (i=0;i<numvertexes;i++)
  {
    fixed_t t;

    if ((t=vertexes[i].x) < map_minx)
      map_minx = t;
    else if (t > map_maxx)
      map_maxx = t;
    if ((t=vertexes[i].y) < map_miny)
      map_miny = t;
    else if (t > map_maxy)
      map_maxy = t;
  }
  map_minx >>= FRACBITS;    // work in map coords, not fixed_t
  map_maxx >>= FRACBITS;
  map_miny >>= FRACBITS;
  map_maxy >>= FRACBITS;

  // set up blockmap area to enclose level plus margin

  xorg = map_minx-blkmargin;
  yorg = map_miny-blkmargin;
  ncols = (map_maxx+blkmargin-xorg+1+blkmask)>>blkshift;  //jff 10/12/98
  nrows = (map_maxy+blkmargin-yorg+1+blkmask)>>blkshift;  //+1 needed for
  NBlocks = ncols*nrows;                                  //map exactly 1 cell

  // create the array of pointers on NBlocks to blocklists
  // also create an array of linelist counts on NBlocks
  // finally make an array in which we can mark blocks done per line

  // CPhipps - calloc's
  blocklists = calloc(NBlocks,sizeof(linelist_t *));
  blockcount = calloc(NBlocks,sizeof(int));
  blockdone = malloc(NBlocks*sizeof(int));

  // initialize each blocklist, and enter the trailing -1 in all blocklists
  // note the linked list of lines grows backwards

  for (i=0;i<NBlocks;i++)
  {
    blocklists[i] = malloc(sizeof(linelist_t));
    blocklists[i]->num = -1;
    blocklists[i]->next = NULL;
    blockcount[i]++;
  }

  // For each linedef in the wad, determine all blockmap blocks it touches,
  // and add the linedef number to the blocklists for those blocks

  for (i=0;i<numlines;i++)
  {
    int x1 = lines[i].v1->x>>FRACBITS;         // lines[i] map coords
    int y1 = lines[i].v1->y>>FRACBITS;
    int x2 = lines[i].v2->x>>FRACBITS;
    int y2 = lines[i].v2->y>>FRACBITS;
    int dx = x2-x1;
    int dy = y2-y1;
    int vert = !dx;                            // lines[i] slopetype
    int horiz = !dy;
    int spos = (dx^dy) > 0;
    int sneg = (dx^dy) < 0;
    int bx,by;                                 // block cell coords
    int minx = x1>x2? x2 : x1;                 // extremal lines[i] coords
    int maxx = x1>x2? x1 : x2;
    int miny = y1>y2? y2 : y1;
    int maxy = y1>y2? y1 : y2;

    // no blocks done for this linedef yet

    memset(blockdone,0,NBlocks*sizeof(int));

    // The line always belongs to the blocks containing its endpoints

    bx = (x1-xorg)>>blkshift;
    by = (y1-yorg)>>blkshift;
    AddBlockLine(blocklists,blockcount,blockdone,by*ncols+bx,i);
    bx = (x2-xorg)>>blkshift;
    by = (y2-yorg)>>blkshift;
    AddBlockLine(blocklists,blockcount,blockdone,by*ncols+bx,i);


    // For each column, see where the line along its left edge, which
    // it contains, intersects the Linedef i. Add i to each corresponding
    // blocklist.

    if (!vert)    // don't interesect vertical lines with columns
    {
      for (j=0;j<ncols;j++)
      {
        // intersection of Linedef with x=xorg+(j<<blkshift)
        // (y-y1)*dx = dy*(x-x1)
        // y = dy*(x-x1)+y1*dx;

        int x = xorg+(j<<blkshift);       // (x,y) is intersection
        int y = (dy*(x-x1))/dx+y1;
        int yb = (y-yorg)>>blkshift;      // block row number
        int yp = (y-yorg)&blkmask;        // y position within block

        if (yb<0 || yb>nrows-1)     // outside blockmap, continue
          continue;

        if (x<minx || x>maxx)       // line doesn't touch column
          continue;

        // The cell that contains the intersection point is always added

        AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j,i);

        // if the intersection is at a corner it depends on the slope
        // (and whether the line extends past the intersection) which
        // blocks are hit

        if (yp==0)        // intersection at a corner
        {
          if (sneg)       //   \ - blocks x,y-, x-,y
          {
            if (yb>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(yb-1)+j,i);
            if (j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
          }
          else if (spos)  //   / - block x-,y-
          {
            if (yb>0 && j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(yb-1)+j-1,i);
          }
          else if (horiz) //   - - block x-,y
          {
            if (j>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
          }
        }
        else if (j>0 && minx<x) // else not at corner: x-,y
          AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
      }
    }

    // For each row, see where the line along its bottom edge, which
    // it contains, intersects the Linedef i. Add i to all the corresponding
    // blocklists.

    if (!horiz)
    {
      for (j=0;j<nrows;j++)
      {
        // intersection of Linedef with y=yorg+(j<<blkshift)
        // (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1)
        // x = dx*(y-y1)/dy+x1;

        int y = yorg+(j<<blkshift);       // (x,y) is intersection
        int x = (dx*(y-y1))/dy+x1;
        int xb = (x-xorg)>>blkshift;      // block column number
        int xp = (x-xorg)&blkmask;        // x position within block

        if (xb<0 || xb>ncols-1)   // outside blockmap, continue
          continue;

        if (y<miny || y>maxy)     // line doesn't touch row
          continue;

        // The cell that contains the intersection point is always added

        AddBlockLine(blocklists,blockcount,blockdone,ncols*j+xb,i);

        // if the intersection is at a corner it depends on the slope
        // (and whether the line extends past the intersection) which
        // blocks are hit

        if (xp==0)        // intersection at a corner
        {
          if (sneg)       //   \ - blocks x,y-, x-,y
          {
            if (j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
            if (xb>0 && minx<x)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*j+xb-1,i);
          }
          else if (vert)  //   | - block x,y-
          {
            if (j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
          }
          else if (spos)  //   / - block x-,y-
          {
            if (xb>0 && j>0 && miny<y)
              AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb-1,i);
          }
        }
        else if (j>0 && miny<y) // else not on a corner: x,y-
          AddBlockLine(blocklists,blockcount,blockdone,ncols*(j-1)+xb,i);
      }
    }
  }

  // Add initial 0 to all blocklists
  // count the total number of lines (and 0's and -1's)

  memset(blockdone,0,NBlocks*sizeof(int));
  for (i=0,linetotal=0;i<NBlocks;i++)
  {
    AddBlockLine(blocklists,blockcount,blockdone,i,0);
    linetotal += blockcount[i];
  }

  // Create the blockmap lump

  blockmaplump = malloc_IfSameLevel(blockmaplump, sizeof(*blockmaplump) * (4 + NBlocks + linetotal));
  // blockmap header

  blockmaplump[0] = bmaporgx = xorg << FRACBITS;
  blockmaplump[1] = bmaporgy = yorg << FRACBITS;
  blockmaplump[2] = bmapwidth  = ncols;
  blockmaplump[3] = bmapheight = nrows;

  // offsets to lists and block lists

  for (i=0;i<NBlocks;i++)
  {
    linelist_t *bl = blocklists[i];
    long offs = blockmaplump[4+i] =   // set offset to block's list
      (i? blockmaplump[4+i-1] : 4+NBlocks) + (i? blockcount[i-1] : 0);

    // add the lines in each block's list to the blockmaplump
    // delete each list node as we go

    while (bl)
    {
      linelist_t *tmp = bl->next;
      blockmaplump[offs++] = bl->num;
      free(bl);
      bl = tmp;
    }
  }

  // free all temporary storage

  free (blocklists);
  free (blockcount);
  free (blockdone);
}

// jff 10/6/98
// End new code added to speed up calculation of internal blockmap

//
// P_VerifyBlockMap
//
// haleyjd 03/04/10: do verification on validity of blockmap.
//
static dboolean P_VerifyBlockMap(int count)
{
  int x, y;
  int *maxoffs = blockmaplump + count;

  for(y = 0; y < bmapheight; y++)
  {
    for(x = 0; x < bmapwidth; x++)
    {
      int offset;
      int *list, *tmplist;
      int *blockoffset;

      offset = y * bmapwidth + x;
      blockoffset = blockmaplump + offset + 4;

      // check that block offset is in bounds
      if(blockoffset >= maxoffs)
      {
        lprintf(LO_ERROR, "P_VerifyBlockMap: block offset overflow\n");
        return false;
      }

      offset = *blockoffset;

      // check that list offset is in bounds
      if(offset < 4 || offset >= count)
      {
        lprintf(LO_ERROR, "P_VerifyBlockMap: list offset overflow\n");
        return false;
      }

      list   = blockmaplump + offset;

      // scan forward for a -1 terminator before maxoffs
      for(tmplist = list; ; tmplist++)
      {
        // we have overflowed the lump?
        if(tmplist >= maxoffs)
        {
          lprintf(LO_ERROR, "P_VerifyBlockMap: open blocklist\n");
          return false;
        }
        if(*tmplist == -1) // found -1
          break;
      }

      // scan the list for out-of-range linedef indicies in list
      for(tmplist = list; *tmplist != -1; tmplist++)
      {
        if(*tmplist < 0 || *tmplist >= numlines)
        {
          lprintf(LO_ERROR, "P_VerifyBlockMap: index >= numlines\n");
          return false;
        }
      }
    }
  }

  return true;
}

//
// P_LoadBlockMap
//
// killough 3/1/98: substantially modified to work
// towards removing blockmap limit (a wad limitation)
//
// killough 3/30/98: Rewritten to remove blockmap limit,
// though current algorithm is brute-force and unoptimal.
//

static void P_LoadBlockMap (int lump)
{
  long count;

  if (M_CheckParm("-blockmap") || W_LumpLength(lump)<8 || (count = W_LumpLength(lump)/2) >= 0x10000) //e6y
    P_CreateBlockMap();
  else
    {
      long i;
      // cph - const*, wad lump handling updated
      const short *wadblockmaplump = W_CacheLumpNum(lump);
      blockmaplump = malloc_IfSameLevel(blockmaplump, sizeof(*blockmaplump) * count);

      // killough 3/1/98: Expand wad blockmap into larger internal one,
      // by treating all offsets except -1 as unsigned and zero-extending
      // them. This potentially doubles the size of blockmaps allowed,
      // because Doom originally considered the offsets as always signed.

      blockmaplump[0] = LittleShort(wadblockmaplump[0]);
      blockmaplump[1] = LittleShort(wadblockmaplump[1]);
      blockmaplump[2] = (long)(LittleShort(wadblockmaplump[2])) & 0xffff;
      blockmaplump[3] = (long)(LittleShort(wadblockmaplump[3])) & 0xffff;

      for (i=4 ; i<count ; i++)
        {
          short t = LittleShort(wadblockmaplump[i]);          // killough 3/1/98
          blockmaplump[i] = t == -1 ? -1l : (long) t & 0xffff;
        }

      W_UnlockLumpNum(lump); // cph - unlock the lump

      bmaporgx = blockmaplump[0]<<FRACBITS;
      bmaporgy = blockmaplump[1]<<FRACBITS;
      bmapwidth = blockmaplump[2];
      bmapheight = blockmaplump[3];

      // haleyjd 03/04/10: check for blockmap problems
      // http://www.doomworld.com/idgames/index.php?id=12935
      if (!P_VerifyBlockMap(count))
      {
        lprintf(LO_INFO, "P_LoadBlockMap: erroneous BLOCKMAP lump may cause crashes.\n");
        lprintf(LO_INFO, "P_LoadBlockMap: use \"-blockmap\" command line switch for rebuilding\n");
      }
    }

  // clear out mobj chains - CPhipps - use calloc
  blocklinks = calloc_IfSameLevel(blocklinks, bmapwidth * bmapheight, sizeof(*blocklinks));
  blockmap = blockmaplump+4;

  // MAES: set blockmapxneg and blockmapyneg
  // E.g. for a full 512x512 map, they should be both
  // -1. For a 257*257, they should be both -255 etc.
  blockmapxneg = (bmapwidth > 255 ? bmapwidth - 512 : -257);
  blockmapyneg = (bmapheight > 255 ? bmapheight - 512 : -257);
  if (blockmapxneg != -257 || blockmapyneg != -257)
  {
    lprintf(LO_WARN,
      "P_LoadBlockMap: This map uses a large blockmap which may cause no-clipping bugs. "
      "Toggle the \"Fix clipping problems in large levels\" option "
      "in the \"Compatibility with common mapping errors\" menu in order to activate a fix. "
      "That fix won't be applied during demo playback or recording.\n");
  }
}

//
// P_LoadReject - load the reject table
//

static void P_LoadReject(int lumpnum, int totallines)
{
  // dump any old cached reject lump, then cache the new one
  if (rejectlump != -1)
    W_UnlockLumpNum(rejectlump);
  rejectlump = lumpnum + ML_REJECT;
  rejectmatrix = W_CacheLumpNum(rejectlump);

  //e6y: check for overflow
  RejectOverrun(rejectlump, &rejectmatrix, totallines);
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
// killough 5/3/98: reformatted, cleaned up
// cph 18/8/99: rewritten to avoid O(numlines * numsectors) section
// It makes things more complicated, but saves seconds on big levels
// figgi 09/18/00 -- adapted for gl-nodes

// cph - convenient sub-function
static void P_AddLineToSector(line_t* li, sector_t* sector)
{
  fixed_t *bbox = (void*)sector->blockbox;

  sector->lines[sector->linecount++] = li;
  M_AddToBox (bbox, li->v1->x, li->v1->y);
  M_AddToBox (bbox, li->v2->x, li->v2->y);
}

// modified to return totallines (needed by P_LoadReject)
static int P_GroupLines (void)
{
  register line_t *li;
  register sector_t *sector;
  int i,j, total = numlines;

  // figgi
  for (i=0 ; i<numsubsectors ; i++)
  {
    seg_t *seg = &segs[subsectors[i].firstline];
    subsectors[i].sector = NULL;
    for(j=0; j<subsectors[i].numlines; j++)
    {
      if(seg->sidedef)
      {
        subsectors[i].sector = seg->sidedef->sector;
        break;
      }
      seg++;
    }
    if(subsectors[i].sector == NULL)
      I_Error("P_GroupLines: Subsector a part of no sector!\n");
  }

  // count number of lines in each sector
  for (i=0,li=lines; i<numlines; i++, li++)
    {
      li->frontsector->linecount++;
      if (li->backsector && li->backsector != li->frontsector)
        {
          li->backsector->linecount++;
          total++;
        }
    }

  {  // allocate line tables for each sector
    line_t **linebuffer = Z_Malloc(total*sizeof(line_t *), PU_LEVEL, 0);
    // e6y: REJECT overrun emulation code
    // moved to P_LoadReject

    for (i=0, sector = sectors; i<numsectors; i++, sector++)
    {
      sector->lines = linebuffer;
      linebuffer += sector->linecount;
      sector->linecount = 0;
      M_ClearBox(sector->blockbox);
    }
  }

  // Enter those lines
  for (i=0,li=lines; i<numlines; i++, li++)
  {
    P_AddLineToSector(li, li->frontsector);
    if (li->backsector && li->backsector != li->frontsector)
      P_AddLineToSector(li, li->backsector);
  }

  for (i=0, sector = sectors; i<numsectors; i++, sector++)
  {
    fixed_t *bbox = (void*)sector->blockbox; // cph - For convenience, so
                                  // I can sue the old code unchanged
    int block;

    sector->bbox[0] = sector->blockbox[0] >> FRACTOMAPBITS;
    sector->bbox[1] = sector->blockbox[1] >> FRACTOMAPBITS;
    sector->bbox[2] = sector->blockbox[2] >> FRACTOMAPBITS;
    sector->bbox[3] = sector->blockbox[3] >> FRACTOMAPBITS;

    // set the degenmobj_t to the middle of the bounding box
    if (comp[comp_sound])
    {
      sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
      sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
    }
    else
    {
      //e6y: fix sound origin for large levels
      sector->soundorg.x = bbox[BOXRIGHT]/2+bbox[BOXLEFT]/2;
      sector->soundorg.y = bbox[BOXTOP]/2+bbox[BOXBOTTOM]/2;
    }

    // adjust bounding box to map blocks
    block = P_GetSafeBlockY(bbox[BOXTOP]-bmaporgy+MAXRADIUS);
    block = block >= bmapheight ? bmapheight-1 : block;
    sector->blockbox[BOXTOP]=block;

    block = P_GetSafeBlockY(bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS);
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXBOTTOM]=block;

    block = P_GetSafeBlockX(bbox[BOXRIGHT]-bmaporgx+MAXRADIUS);
    block = block >= bmapwidth ? bmapwidth-1 : block;
    sector->blockbox[BOXRIGHT]=block;

    block = P_GetSafeBlockX(bbox[BOXLEFT]-bmaporgx-MAXRADIUS);
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXLEFT]=block;
  }

  return total; // this value is needed by the reject overrun emulation code
}

//
// killough 10/98
//
// Remove slime trails.
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//      2        2                         2        2
//    dx  x0 + dy  x1 + dx dy (y0 - y1)  dy  y0 + dx  y1 + dx dy (x0 - x1)
//   {---------------------------------, ---------------------------------}
//                  2     2                            2     2
//                dx  + dy                           dx  + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

static void P_RemoveSlimeTrails(void)         // killough 10/98
{
  byte *hit = calloc(1, numvertexes);         // Hitlist for vertices
  int i;
  // Correction of desync on dv04-423.lmp/dv.wad
  // http://www.doomworld.com/vb/showthread.php?s=&postid=627257#post627257
  int apply_for_real_vertexes = (compatibility_level>=lxdoom_1_compatibility || prboom_comp[PC_REMOVE_SLIME_TRAILS].state);

  for (i=0; i<numvertexes; i++)
  {
    // [crispy] initialize pseudovertexes with actual vertex coordinates
    vertexes[i].px = vertexes[i].x;
    vertexes[i].py = vertexes[i].y;
  }

  for (i=0; i<numsegs; i++)                   // Go through each seg
  {
    const line_t *l;

    if (segs[i].miniseg == true)        //figgi -- skip minisegs
      break;                            //e6y: probably 'continue;'?

    l = segs[i].linedef;            // The parent linedef
    if (l->dx && l->dy)                     // We can ignore orthogonal lines
    {
    vertex_t *v = segs[i].v1;
    do
      if (!hit[v - vertexes])           // If we haven't processed vertex
        {
    hit[v - vertexes] = 1;        // Mark this vertex as processed
    if (v != l->v1 && v != l->v2) // Exclude endpoints of linedefs
      { // Project the vertex back onto the parent linedef
        int_64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
        int_64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
        int_64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
        int_64_t s = dx2 + dy2;
        int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;
        v->px = (int)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
        v->py = (int)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
        
        // [crispy] wait a minute... moved more than 8 map units?
        // maybe that's a linguortal then, back to the original coordinates
        // http://www.doomworld.com/vb/doom-editing/74354-stupid-bsp-tricks/
        if (!apply_for_real_vertexes && (D_abs(v->px - v->x) > 8*FRACUNIT || D_abs(v->py - v->y) > 8*FRACUNIT))
        {
          v->px = v->x;
          v->py = v->y;
        }

        if (apply_for_real_vertexes)
        {
          v->x = v->px;
          v->y = v->py;
        }
      }
        }  // Obsfucated C contest entry:   :)
    while ((v != segs[i].v2) && (v = segs[i].v2));
  }
    }
  free(hit);
}

static void R_CalcSegsLength(void)
{
  int i;
  for (i=0; i<numsegs; i++)
  {
    seg_t *li = segs+i;
    int_64_t dx = (int_64_t)li->v2->px - li->v1->px;
    int_64_t dy = (int_64_t)li->v2->py - li->v1->py;
    li->length = (int_64_t)sqrt((double)dx*dx + (double)dy*dy);
    // [crispy] re-calculate angle used for rendering
    li->pangle = R_PointToAngleEx2(li->v1->px, li->v1->py, li->v2->px, li->v2->py);
  }
}

//
// P_CheckLumpsForSameSource
//
// Are these lumps in the same wad file?
//

dboolean P_CheckLumpsForSameSource(int lump1, int lump2)
{
  int wad1_index, wad2_index;
  wadfile_info_t *wad1, *wad2;

  if (((unsigned)lump1 >= (unsigned)numlumps) || ((unsigned)lump2 >= (unsigned)numlumps))
    return false;
  
  wad1 = lumpinfo[lump1].wadfile;
  wad2 = lumpinfo[lump2].wadfile;
  
  if (!wad1 || !wad2)
    return false;

  wad1_index = (int)(wad1 - wadfiles);
  wad2_index = (int)(wad2 - wadfiles);

  if (wad1_index != wad2_index)
    return false;

  if ((wad1_index < 0) || ((size_t)wad1_index >= numwadfiles))
    return false;

  if ((wad2_index < 0) || ((size_t)wad2_index >= numwadfiles))
    return false;

  return true;
}

//
// P_CheckLevelFormat
//
// Checking for presence of necessary lumps
//

void P_CheckLevelWadStructure(const char *mapname)
{
  int i, lumpnum;

  static const char *ml_labels[] = {
    "ML_LABEL",             // A separator, name, ExMx or MAPxx
    "ML_THINGS",            // Monsters, items..
    "ML_LINEDEFS",          // LineDefs, from editing
    "ML_SIDEDEFS",          // SideDefs, from editing
    "ML_VERTEXES",          // Vertices, edited and BSP splits generated
    "ML_SEGS",              // LineSegs, from LineDefs split by BSP
    "ML_SSECTORS",          // SubSectors, list of LineSegs
    "ML_NODES",             // BSP nodes
    "ML_SECTORS",           // Sectors, from editing
    "ML_REJECT",            // LUT, sector-sector visibility
    "ML_BLOCKMAP",          // LUT, motion clipping, walls/grid element
  };

  if (!mapname)
  {
    I_Error("P_SetupLevel: Wrong map name");
  }

  lumpnum = W_CheckNumForName(mapname);

  if (lumpnum < 0)
  {
    I_Error("P_SetupLevel: There is no %s map.", mapname);
  }

  for (i = ML_THINGS + 1; i <= ML_SECTORS; i++)
  {
    if (!P_CheckLumpsForSameSource(lumpnum, lumpnum + i))
    {
      I_Error("P_SetupLevel: Level wad structure is incomplete. There is no %s lump.", ml_labels[i]);
    }
  }

  // refuse to load Hexen-format maps, avoid segfaults
  i = lumpnum + ML_BLOCKMAP + 1;
  if (P_CheckLumpsForSameSource(lumpnum, i))
  {
    if (!strncasecmp(lumpinfo[i].name, "BEHAVIOR", 8))
    {
      I_Error("P_SetupLevel: %s: Hexen format not supported", mapname);
    }
  }
}

void P_InitSubsectorsLines(void)
{
  int num, count;

  if (sslines_indexes)
  {
    free(sslines_indexes);
    sslines_indexes = NULL;
  }

  if (sslines)
  {
    free(sslines);
    sslines = NULL;
  }

  count = 0;
  sslines_indexes = malloc((numsubsectors + 1) * sizeof(sslines_indexes[0]));

  for (num = 0; num < numsubsectors; num++)
  {
    seg_t *seg;
    const seg_t *seg_last = segs + subsectors[num].firstline + subsectors[num].numlines;

    sslines_indexes[num] = count;

    for (seg = segs + subsectors[num].firstline; seg < seg_last; seg++)
    {
      if (!seg->linedef) continue;
      seg->linedef->validcount = 0;
    }

    for (seg = segs + subsectors[num].firstline; seg < seg_last; seg++)
    {
      if (!seg->linedef) continue;

      if (seg->linedef->validcount == 1)
        continue;

      seg->linedef->validcount = 1;
      count++;
    }
  }

  sslines_indexes[numsubsectors] = count;

  sslines = malloc(count * sizeof(sslines[0]));
  count = 0;

  for (num = 0; num < numsubsectors; num++)
  {
    seg_t *seg;
    const seg_t *seg_last = segs + subsectors[num].firstline + subsectors[num].numlines;

    for (seg = segs + subsectors[num].firstline; seg < seg_last; seg++)
    {
      if (!seg->linedef) continue;
      seg->linedef->validcount = 0;
    }

    for (seg = segs + subsectors[num].firstline; seg < seg_last; seg++)
    {
      ssline_t *ssline = &sslines[count];
      if (!seg->linedef) continue;

      if (seg->linedef->validcount == 1)
        continue;

      seg->linedef->validcount = 1;

      ssline->seg = seg;
      ssline->linedef = seg->linedef;

      ssline->x1 = seg->linedef->v1->x;
      ssline->y1 = seg->linedef->v1->y;
      ssline->x2 = seg->linedef->v2->x;
      ssline->y2 = seg->linedef->v2->y;
      ssline->bbox[0] = seg->linedef->bbox[0];
      ssline->bbox[1] = seg->linedef->bbox[1];
      ssline->bbox[2] = seg->linedef->bbox[2];
      ssline->bbox[3] = seg->linedef->bbox[3];

      count++;
    }
  }

  for (num = 0; num < numlines; num++)
  {
    lines[num].validcount = 0;
  }
}

//
// P_SetupLevel
//
// killough 5/3/98: reformatted, cleaned up

void P_SetupLevel(int episode, int map, int playermask, skill_t skill)
{
  int   i;
  char  lumpname[9];
  int   lumpnum;

  char  gl_lumpname[9];
  int   gl_lumpnum;

  //e6y
  totallive = 0;
  transparentpresent = false;

  R_StopAllInterpolations();

  totallive = totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
  wminfo.partime = 180;

  for (i=0; i<MAXPLAYERS; i++)
  {
    players[i].killcount = players[i].secretcount = players[i].itemcount = 0;
    players[i].resurectedkillcount = 0;//e6y
  }

  // Initial height of PointOfView will be set by player think.
  players[consoleplayer].viewz = 1;

  // Make sure all sounds are stopped before Z_FreeTags.
  S_Start();

  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);
  if (rejectlump != -1) { // cph - unlock the reject table
    W_UnlockLumpNum(rejectlump);
    rejectlump = -1;
  }
  
  P_InitThinkers();

  // if working with a devlopment map, reload it
  //    W_Reload ();     killough 1/31/98: W_Reload obsolete

  // find map name
  if (gamemode == commercial)
  {
    sprintf(lumpname, "map%02d", map);           // killough 1/24/98: simplify
    sprintf(gl_lumpname, "gl_map%02d", map);    // figgi
  }
  else
  {
    sprintf(lumpname, "E%dM%d", episode, map);   // killough 1/24/98: simplify
    sprintf(gl_lumpname, "GL_E%iM%i", episode, map); // figgi
  }

  lumpnum = W_GetNumForName(lumpname);
  gl_lumpnum = W_CheckNumForName(gl_lumpname); // figgi

  // e6y
  // Refuse to load a map with incomplete pwad structure.
  // Avoid segfaults on levels without nodes.
  P_CheckLevelWadStructure(lumpname);

  leveltime = 0; totallive = 0;

  // note: most of this ordering is important

  // killough 3/1/98: P_LoadBlockMap call moved down to below
  // killough 4/4/98: split load of sidedefs into two parts,
  // to allow texture names to be used in special linedefs

  // figgi 10/19/00 -- check for gl lumps and load them
  P_GetNodesVersion(lumpnum,gl_lumpnum);

  // e6y: speedup of level reloading
  // Most of level's structures now are allocated with PU_STATIC instead of PU_LEVEL
  // It is important for OpenGL, because in case of the same data in memory
  // we can skip recalculation of much stuff

  samelevel =
    (map == current_map) &&
    (episode == current_episode) &&
    (nodesVersion == current_nodesVersion);

  current_episode = episode;
  current_map = map;
  current_nodesVersion = nodesVersion;

  if (!samelevel)
  {
#ifdef GL_DOOM
    // proff 11/99: clean the memory from textures etc.
    gld_CleanMemory();
#endif

    free(segs);
    free(nodes);
    free(subsectors);
#ifdef GL_DOOM
    free(map_subsectors);
#endif

    free(blocklinks);
    free(blockmaplump);

    free(lines);
    free(sides);
    free(sectors);
    free(vertexes);
  }

  if (nodesVersion > 0)
    P_LoadVertexes2 (lumpnum+ML_VERTEXES,gl_lumpnum+ML_GL_VERTS);
  else
    P_LoadVertexes  (lumpnum+ML_VERTEXES);
  P_LoadSectors   (lumpnum+ML_SECTORS);
  P_LoadSideDefs  (lumpnum+ML_SIDEDEFS);
  P_LoadLineDefs  (lumpnum+ML_LINEDEFS);
  P_LoadSideDefs2 (lumpnum+ML_SIDEDEFS);
  P_LoadLineDefs2 (lumpnum+ML_LINEDEFS);

  // e6y: speedup of level reloading
  // Do not reload BlockMap for same level,
  // because in case of big level P_CreateBlockMap eats much time
  //
  // BlockMap should be reloaded after OVERFLOW_INTERCEPT,
  // because bmapwidth/bmapheight/bmaporgx/bmaporgy can be overwritten
  if (!samelevel || overflows[OVERFLOW_INTERCEPT].shit_happens)
  {
    P_LoadBlockMap  (lumpnum+ML_BLOCKMAP);
  }
  else
  {
    memset(blocklinks, 0, bmapwidth*bmapheight*sizeof(*blocklinks));
  }

  if (nodesVersion > 0)
  {
    P_LoadSubsectors(gl_lumpnum + ML_GL_SSECT);
    P_LoadNodes(gl_lumpnum + ML_GL_NODES);
    P_LoadGLSegs(gl_lumpnum + ML_GL_SEGS);
  }
  else
  {
    if (P_CheckForZDoomUncompressedNodes(lumpnum, gl_lumpnum))
    {
      P_LoadZNodes(lumpnum + ML_NODES, 0);
    }
    else if (P_CheckForDeePBSPv4Nodes(lumpnum, gl_lumpnum))
    {
      P_LoadSubsectors_V4(lumpnum + ML_SSECTORS);
      P_LoadNodes_V4(lumpnum + ML_NODES);
      P_LoadSegs_V4(lumpnum + ML_SEGS);
    }
    else
    {
      P_LoadSubsectors(lumpnum + ML_SSECTORS);
      P_LoadNodes(lumpnum + ML_NODES);
      P_LoadSegs(lumpnum + ML_SEGS);
    }
  }

  if (!samelevel)
  {
    P_InitSubsectorsLines();
  }

#ifdef GL_DOOM
  map_subsectors = calloc_IfSameLevel(map_subsectors,
    numsubsectors, sizeof(map_subsectors[0]));
#endif

  // reject loading and underflow padding separated out into new function
  // P_GroupLines modified to return a number the underflow padding needs
  P_LoadReject(lumpnum, P_GroupLines());

  P_RemoveSlimeTrails();    // killough 10/98: remove slime trails from wad

  // should be after P_RemoveSlimeTrails, because it changes vertexes
  R_CalcSegsLength();

  // Note: you don't need to clear player queue slots --
  // a much simpler fix is in g_game.c -- killough 10/98

  bodyqueslot = 0;

  /* cph - reset all multiplayer starts */
  memset(playerstarts,0,sizeof(playerstarts));
  deathmatch_p = deathmatchstarts;
  for (i = 0; i < MAXPLAYERS; i++)
    players[i].mo = NULL;
  TracerClearStarts();

  P_MapStart();

  P_LoadThings(lumpnum+ML_THINGS);

  // if deathmatch, randomly spawn the active players
  if (deathmatch)
  {
    for (i=0; i<MAXPLAYERS; i++)
      if (playeringame[i])
        {
          players[i].mo = NULL; // not needed? - done before P_LoadThings
          G_DeathMatchSpawnPlayer(i);
        }
  }
  else // if !deathmatch, check all necessary player starts actually exist
  {
    for (i=0; i<MAXPLAYERS; i++)
      if (playeringame[i] && !players[i].mo)
        I_Error("P_SetupLevel: missing player %d start\n", i+1);
  }

  if (players[consoleplayer].cheats & CF_FLY)
  {
    players[consoleplayer].mo->flags |= (MF_NOGRAVITY | MF_FLY);
  }

  // killough 3/26/98: Spawn icon landings:
  if (gamemode==commercial)
    P_SpawnBrainTargets();

  if (gamemode != shareware)
  {
    S_ParseMusInfo(lumpname);
  }

  // clear special respawning que
  iquehead = iquetail = 0;

  // set up world state
  P_SpawnSpecials();

  P_MapEnd();

  // preload graphics
  if (precache)
    R_PrecacheLevel();

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL)
  {
    // e6y
    // Do not preprocess GL data during skipping,
    // because it potentially will not be used.
    // But preprocessing must be called immediately after stop of skipping.
    if (!doSkip)
    {
      // proff 11/99: calculate all OpenGL specific tables etc.
      gld_PreprocessLevel();
    }
  }
#endif
  //e6y
  P_SyncWalkcam(true, true);
  R_SmoothPlaying_Reset(NULL);
}

//
// P_Init
//
void P_Init (void)
{
  P_InitSwitchList();
  P_InitPicAnims();
  R_InitSprites(sprnames);
}
