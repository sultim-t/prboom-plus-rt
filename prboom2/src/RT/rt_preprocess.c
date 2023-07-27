/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2022 by
 *  Sultim Tsyrendashiev
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
 *
 *---------------------------------------------------------------------
 */

 /* Based on gl_preprocess.c */

#include "rt_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#ifdef _WIN32
#include "WIN/win_fopen.h"
#endif

#include <GL/glu.h>

#include "doomtype.h"
#include "lprintf.h"
#include "p_maputl.h"
#include "rt_textures.h"
#include "r_defs.h"
#include "r_main.h"
#include "z_zone.h"
#include "rt_main.h"



typedef struct
{
  float x, y, z;
} RTPPosition;

typedef struct
{
  float u, v;
} RTPTexCoord;

typedef struct
{
  float x1, x2;
  float z1, z2;
  // dboolean fracleft, fracright;
} RTPSeg;

typedef struct
{
  RTPPosition *positions;
  RTPTexCoord *texCoords;
} RTPVbo;

typedef enum
{
  RTP_TRIANGLE_MODE_TRIANGLES,
  RTP_TRIANGLE_MODE_TRIANGLE_STRIP,
  RTP_TRIANGLE_MODE_TRIANGLE_FAN,
} RTPTriangleMode;

typedef struct
{
  int index;   // subsector index
  RTPTriangleMode mode;
  int vertexcount; // number of vertexes in this loop
  int vertexindex; // index into vertex list
} RTPLoopDef;

typedef struct
{
  int loopcount; // number of loops for this sector
  RTPLoopDef *loops; // the loops itself
  unsigned int flags;
} RTPSector;

typedef struct
{
  int loopcount; // number of loops for this sector
  RTPLoopDef *loops; // the loops itself
} RTPMapSubsector;



static FILE *levelinfo;

static int rtp_max_vertexes = 0;
static int rtp_num_vertexes = 0;
static RgBool32 rtp_triangulate_subsectors = false;

// this is the list for all sectors to the loops
static RTPSector *rtp_sectorloops;
// this is the list for all subsectors to the loops
// uses by textured automap
static RTPMapSubsector *rtp_subsectorloops;
static RTPVbo rtp_flats_vbo;

static int rtp_tess_currentsector; // the sector which is currently tesselated

static RTPSeg *rtp_segs = NULL;
static RTPSeg *rtp_lines = NULL;

const RgBool32 rtp_preprocessed = false;



extern int nodesVersion;



static void RTP_AddGlobalVertexes(int count)
{
  if ((rtp_num_vertexes + count) >= rtp_max_vertexes)
  {
    rtp_max_vertexes += count + 1024;
    rtp_flats_vbo.positions = Z_Realloc(rtp_flats_vbo.positions,(size_t)rtp_max_vertexes * sizeof(RTPPosition), PU_STATIC, 0);
    rtp_flats_vbo.texCoords = Z_Realloc(rtp_flats_vbo.texCoords,(size_t)rtp_max_vertexes * sizeof(RTPTexCoord), PU_STATIC, 0);
  }
}


#define FIX2DBL(x)    ((double)(x))
#define MAX_CC_SIDES  128
static dboolean RTP_PointOnSide(const vertex_t *p, const divline_t *d)
{
  // We'll return false if the point c is on the left side.
  return ((FIX2DBL(d->y) - FIX2DBL(p->y)) * FIX2DBL(d->dx) - (FIX2DBL(d->x) - FIX2DBL(p->x)) * FIX2DBL(d->dy) >= 0);
}
// Lines start-end and fdiv must intersect.
static void RTP_CalcIntersectionVertex(const vertex_t *s, const vertex_t *e, const divline_t *d, vertex_t *i)
{
  double ax = FIX2DBL(s->x), ay = FIX2DBL(s->y), bx = FIX2DBL(e->x), by = FIX2DBL(e->y);
  double cx = FIX2DBL(d->x), cy = FIX2DBL(d->y), dx = cx + FIX2DBL(d->dx), dy = cy + FIX2DBL(d->dy);
  double r = ((ay - cy) * (dx - cx) - (ax - cx) * (dy - cy)) / ((bx - ax) * (dy - cy) - (by - ay) * (dx - cx));
  i->x = (fixed_t)((double)s->x + r * ((double)e->x - (double)s->x));
  i->y = (fixed_t)((double)s->y + r * ((double)e->y - (double)s->y));
}
#undef FIX2DBL


// Returns a pointer to the list of points. It must be used.
static vertex_t *RTP_FlatEdgeClipper(int *numpoints, vertex_t *points, int numclippers, divline_t *clippers)
{
  unsigned char sidelist[MAX_CC_SIDES];
  int       i, k, num = *numpoints;

  // We'll clip the polygon with each of the divlines. The left side of
  // each divline is discarded.
  for (i = 0; i < numclippers; i++)
  {
    divline_t *curclip = &clippers[i];

    // First we'll determine the side of each vertex. Points are allowed
    // to be on the line.
    for (k = 0; k < num; k++)
      sidelist[k] = RTP_PointOnSide(&points[k], curclip);

    for (k = 0; k < num; k++)
    {
      int startIdx = k, endIdx = k + 1;
      // Check the end index.
      if (endIdx == num) endIdx = 0; // Wrap-around.
      // Clipping will happen when the ends are on different sides.
      if (sidelist[startIdx] != sidelist[endIdx])
      {
        vertex_t newvert;

        RTP_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

        // Add the new vertex. Also modify the sidelist.
        points = (vertex_t *)Z_Realloc(points, (++num) * sizeof(vertex_t), PU_LEVEL, 0);
        if (num >= MAX_CC_SIDES)
          I_Error("RTP_FlatEdgeClipper: Too many points in carver");

        // Make room for the new vertex.
        memmove(&points[endIdx + 1], &points[endIdx],
                (num - endIdx - 1) * sizeof(vertex_t));
        memcpy(&points[endIdx], &newvert, sizeof(newvert));

        memmove(&sidelist[endIdx + 1], &sidelist[endIdx], num - endIdx - 1);
        sidelist[endIdx] = 1;

        // Skip over the new vertex.
        k++;
      }
    }

    // Now we must discard the points that are on the wrong side.
    for (k = 0; k < num; k++)
      if (!sidelist[k])
      {
        memmove(&points[k], &points[k + 1], (num - k - 1) * sizeof(vertex_t));
        memmove(&sidelist[k], &sidelist[k + 1], num - k - 1);
        num--;
        k--;
      }
  }
  // Screen out consecutive identical points.
  for (i = 0; i < num; i++)
  {
    int previdx = i - 1;
    if (previdx < 0) previdx = num - 1;
    if (points[i].x == points[previdx].x
        && points[i].y == points[previdx].y)
    {
      // This point (i) must be removed.
      memmove(&points[i], &points[i + 1], sizeof(vertex_t) * (num - i - 1));
      num--;
      i--;
    }
  }
  *numpoints = num;
  return points;
}


static void RTP_FlatConvexCarver(int ssidx, int num, const divline_t *list)
{
  subsector_t *ssec = &subsectors[ssidx];
  int numclippers = num + ssec->numlines;
  int i, numedgepoints;

  divline_t *clippers = Z_Malloc(numclippers * sizeof(divline_t), PU_LEVEL, 0);
  if (!clippers)
    return;
  for (i = 0; i < num; i++)
  {
    clippers[i].x = list[num - i - 1].x;
    clippers[i].y = list[num - i - 1].y;
    clippers[i].dx = list[num - i - 1].dx;
    clippers[i].dy = list[num - i - 1].dy;
  }
  for (i = num; i < numclippers; i++)
  {
    seg_t *seg = &segs[ssec->firstline + i - num];
    clippers[i].x = seg->v1->x;
    clippers[i].y = seg->v1->y;
    clippers[i].dx = seg->v2->x - seg->v1->x;
    clippers[i].dy = seg->v2->y - seg->v1->y;
  }

  // Setup the 'worldwide' polygon.
  numedgepoints = 4;
  vertex_t *edgepoints = Z_Malloc(numedgepoints * sizeof(vertex_t), PU_LEVEL, 0);

  edgepoints[0].x = INT_MIN;
  edgepoints[0].y = INT_MAX;

  edgepoints[1].x = INT_MAX;
  edgepoints[1].y = INT_MAX;

  edgepoints[2].x = INT_MAX;
  edgepoints[2].y = INT_MIN;

  edgepoints[3].x = INT_MIN;
  edgepoints[3].y = INT_MIN;

  // Do some clipping, <snip> <snip>
  edgepoints = RTP_FlatEdgeClipper(&numedgepoints, edgepoints, numclippers, clippers);

  if (!numedgepoints)
  {
    if (levelinfo) fprintf(levelinfo, "All carved away: subsector %lld - sector %i\n", ssec - subsectors, ssec->sector->iSectorID);
  }
  else
  {
    if (numedgepoints >= 3)
    {
      RTP_AddGlobalVertexes(numedgepoints);
      if (rtp_flats_vbo.positions && rtp_flats_vbo.texCoords)
      {
        int currentsector = ssec->sector->iSectorID;
        RTPLoopDef **loop;
        int *loopcount;

        if (rtp_triangulate_subsectors)
        {
          loop = &rtp_subsectorloops[ssidx].loops;
          loopcount = &rtp_subsectorloops[ssidx].loopcount;
        }
        else
        {
          loop = &rtp_sectorloops[currentsector].loops;
          loopcount = &rtp_sectorloops[currentsector].loopcount;
        }

        (*loopcount)++;
        (*loop) = Z_Realloc((*loop), sizeof(RTPLoopDef) * (*loopcount), PU_STATIC, 0);
        ((*loop)[(*loopcount) - 1]).index = ssidx;
        ((*loop)[(*loopcount) - 1]).mode = RTP_TRIANGLE_MODE_TRIANGLE_FAN;
        ((*loop)[(*loopcount) - 1]).vertexcount = numedgepoints;
        ((*loop)[(*loopcount) - 1]).vertexindex = rtp_num_vertexes;

        for (i = 0; i < numedgepoints; i++)
        {
          RTPPosition *dst_p = &rtp_flats_vbo.positions[rtp_num_vertexes];
          RTPTexCoord *dst_t = &rtp_flats_vbo.texCoords[rtp_num_vertexes];

          dst_p->x = -(float)edgepoints[i].x / MAP_SCALE;
          dst_p->y = 0.0f;
          dst_p->z =  (float)edgepoints[i].y / MAP_SCALE;

          dst_t->u = ( (float)edgepoints[i].x / (float)FRACUNIT) / 64.0f;
          dst_t->v = (-(float)edgepoints[i].y / (float)FRACUNIT) / 64.0f;

          rtp_num_vertexes++;
        }
      }
    }
  }
  // We're done, free the edgepoints memory.
  Z_Free(edgepoints);
  Z_Free(clippers);
}


static void RTP_CarveFlats(int bspnode, int numdivlines, const divline_t *divlines)
{
  int childlistsize = numdivlines + 1;

  // If this is a subsector we are dealing with, begin carving with the
  // given list.
  if (bspnode & NF_SUBSECTOR)
  {
    // We have arrived at a subsector. The divline list contains all
    // the partition lines that carve out the subsector.
    // special case for trivial maps (no nodes, single subsector)
    int ssidx = (numnodes != 0) ? bspnode & (~NF_SUBSECTOR) : 0;

    if (!(subsectors[ssidx].sector->flags & SECTOR_IS_CLOSED) || rtp_triangulate_subsectors)
      RTP_FlatConvexCarver(ssidx, numdivlines, divlines);
    return;
  }

  // Get a pointer to the node.
  node_t *nod = nodes + bspnode;

  // Allocate a new list for each child.
  divline_t *childlist = (divline_t*)Z_Malloc(childlistsize * sizeof(divline_t), PU_LEVEL, 0);

  // Copy the previous lines.
  if (divlines) memcpy(childlist, divlines, numdivlines * sizeof(divline_t));

  divline_t *dl = childlist + numdivlines;
  dl->x = nod->x;
  dl->y = nod->y;
  // The right child gets the original line (LEFT side clipped).
  dl->dx = nod->dx;
  dl->dy = nod->dy;
  RTP_CarveFlats(nod->children[0], childlistsize, childlist);

  // The left side. We must reverse the line, otherwise the wrong
  // side would get clipped.
  dl->dx = -nod->dx;
  dl->dy = -nod->dy;
  RTP_CarveFlats(nod->children[1], childlistsize, childlist);

  // We are finishing with this node, free the allocated list.
  Z_Free(childlist);
}


#ifdef USE_GLU_TESS


static RTPTriangleMode GLenumToRTPTriangleMode(GLenum type)
{
  switch (type)
  {
    case GL_TRIANGLES:
      return RTP_TRIANGLE_MODE_TRIANGLES;
    case GL_TRIANGLE_STRIP:
      return RTP_TRIANGLE_MODE_TRIANGLE_STRIP;
    case GL_TRIANGLE_FAN:
      return RTP_TRIANGLE_MODE_TRIANGLE_FAN;
    default:
      assert_always("RTP: Incorrect GLenum");
      return 0;
  }
}


// ntessBegin
//
// called when the tesselation of a new loop starts

static void CALLBACK ntessBegin(GLenum gltype)
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
  {
    if (gltype == GL_TRIANGLES)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLES\n");
    else
      if (gltype == GL_TRIANGLE_FAN)
        fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_FAN\n");
      else
        if (gltype == GL_TRIANGLE_STRIP)
          fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_STRIP\n");
        else
          fprintf(levelinfo, "\t\tBegin: unknown\n");
  }
#endif
  RTPTriangleMode type = GLenumToRTPTriangleMode(gltype);
  RTPSector *s = &rtp_sectorloops[rtp_tess_currentsector];

  // increase loopcount for currentsector
  s->loopcount++;
  // reallocate to get space for another loop
  // PU_LEVEL is used, so this gets freed before a new level is loaded
  s->loops = Z_Realloc(s->loops, sizeof(RTPLoopDef) * s->loopcount, PU_STATIC, 0);
  // set initial values for current loop
  // currentloop is: (s->loopcount - 1)
  s->loops[s->loopcount - 1].index = -1;
  s->loops[s->loopcount - 1].mode = type;
  s->loops[s->loopcount - 1].vertexcount = 0;
  s->loops[s->loopcount - 1].vertexindex = rtp_num_vertexes;
}

// ntessError
//
// called when the tesselation failes (DEBUG only)

static void CALLBACK ntessError(GLenum error)
{
#ifdef PRBOOM_DEBUG
  const GLubyte *estring;
  estring = gluErrorString(error);
  fprintf(levelinfo, "\t\tTessellation Error: %s\n", estring);
#endif
}
// ntessCombine
//
// called when the two or more vertexes are on the same coordinate

static void CALLBACK ntessCombine(double coords[3], vertex_t *vert[4], float w[4], void **dataOut)
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
  {
    fprintf(levelinfo, "\t\tVertexCombine Coords: x %10.5f, y %10.5f z %10.5f\n", coords[0], coords[1], coords[2]);
    if (vert[0]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[0]->x >> FRACBITS, vert[0]->y >> FRACBITS, vert[0]);
    if (vert[1]) fprintf(levelinfo, "\t\tVertexCombine Vert2 : x %10i, y %10i p %p\n", vert[1]->x >> FRACBITS, vert[1]->y >> FRACBITS, vert[1]);
    if (vert[2]) fprintf(levelinfo, "\t\tVertexCombine Vert3 : x %10i, y %10i p %p\n", vert[2]->x >> FRACBITS, vert[2]->y >> FRACBITS, vert[2]);
    if (vert[3]) fprintf(levelinfo, "\t\tVertexCombine Vert4 : x %10i, y %10i p %p\n", vert[3]->x >> FRACBITS, vert[3]->y >> FRACBITS, vert[3]);
  }
#endif
  // just return the first vertex, because all vertexes are on the same coordinate
  *dataOut = vert[0];
}

// ntessVertex
//
// called when a vertex is found

static void CALLBACK ntessVertex(vertex_t *vert)
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tVertex : x %10i, y %10i\n", vert->x >> FRACBITS, vert->y >> FRACBITS);
#endif
  // increase vertex count
  rtp_sectorloops[rtp_tess_currentsector].loops[rtp_sectorloops[rtp_tess_currentsector].loopcount - 1].vertexcount++;

  // increase vertex count
  RTP_AddGlobalVertexes(1);
  // add the new vertex (vert is the second argument of gluTessVertex)
  RTPPosition *dst_p = &rtp_flats_vbo.positions[rtp_num_vertexes];
  RTPTexCoord *dst_t = &rtp_flats_vbo.texCoords[rtp_num_vertexes];

  dst_p->x = -(float)vert->x / MAP_SCALE;
  dst_p->y = 0.0f;
  dst_p->z =  (float)vert->y / MAP_SCALE;

  dst_t->u = ( (float)vert->x / (float)FRACUNIT) / 64.0f;
  dst_t->v = (-(float)vert->y / (float)FRACUNIT) / 64.0f;

  rtp_num_vertexes++;
}

// ntessEnd
//
// called when the tesselation of a the current loop ends (DEBUG only)

static void CALLBACK ntessEnd(void)
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tEnd loopcount %i vertexcount %i\n", rtp_sectorloops[currentsector].loopcount, rtp_sectorloops[currentsector].loops[rtp_sectorloops[currentsector].loopcount - 1].vertexcount);
#endif
}

static void RTP_PrecalculateSector(int num)
{
  int i;
  dboolean *lineadded = NULL;
  int linecount;
  int currentline;
  int oldline;
  int currentloop;
  int bestline;
  int bestlinecount;
  vertex_t *startvertex;
  vertex_t *currentvertex = NULL; //e6y: fix use of uninitialized local variable below
  angle_t lineangle;
  angle_t angle;
  angle_t bestangle;
  GLUtesselator *tess;
  double *v = NULL;
  int maxvertexnum;
  int vertexnum;

  rtp_tess_currentsector = num;
  lineadded = Z_Malloc(sectors[num].linecount * sizeof(dboolean), PU_LEVEL, 0);
  if (!lineadded)
  {
    if (levelinfo) fclose(levelinfo);
    return;
  }
  // init tesselator
  tess = gluNewTess();
  if (!tess)
  {
    if (levelinfo) fclose(levelinfo);
    Z_Free(lineadded);
    return;
  }
  // set callbacks
  gluTessCallback(tess, GLU_TESS_BEGIN, ntessBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, ntessVertex);
  gluTessCallback(tess, GLU_TESS_ERROR, ntessError);
  gluTessCallback(tess, GLU_TESS_COMBINE, ntessCombine);
  gluTessCallback(tess, GLU_TESS_END, ntessEnd);
  if (levelinfo) fprintf(levelinfo, "sector %i, %i lines in sector\n", num, sectors[num].linecount);
  // remove any line which has both sides in the same sector (i.e. Doom2 Map01 Sector 1)
  for (i = 0; i < sectors[num].linecount; i++)
  {
    lineadded[i] = false;
    if (sectors[num].lines[i]->sidenum[0] != NO_INDEX)
      if (sectors[num].lines[i]->sidenum[1] != NO_INDEX)
        if (sides[sectors[num].lines[i]->sidenum[0]].sector
            == sides[sectors[num].lines[i]->sidenum[1]].sector)
        {
          lineadded[i] = true;
          if (levelinfo) fprintf(levelinfo, "line %4i (iLineID %4i) has both sides in same sector (removed)\n", i, sectors[num].lines[i]->iLineID);
        }
  }
  // e6y
  // Remove any line which has a clone with the same vertexes and orientation
  // (i.e. MM.WAD Map22 lines 1298 and 2397)
  // There is no more HOM on Memento Mori MAP22 sector 299
  for (i = 0; i < sectors[num].linecount - 1; i++)
  {
    int j;
    for (j = i + 1; j < sectors[num].linecount; j++)
    {
      if (sectors[num].lines[i]->v1 == sectors[num].lines[j]->v1 &&
          sectors[num].lines[i]->v2 == sectors[num].lines[j]->v2 &&
          sectors[num].lines[i]->frontsector == sectors[num].lines[j]->frontsector &&
          sectors[num].lines[i]->backsector == sectors[num].lines[j]->backsector &&
          lineadded[i] == false && lineadded[j] == false)
      {
        lineadded[i] = true;
      }
    }
  }

  // initialize variables
  linecount = sectors[num].linecount;
  oldline = 0;
  currentline = 0;
  startvertex = sectors[num].lines[currentline]->v2;
  currentloop = 0;
  vertexnum = 0;
  maxvertexnum = 0;
  // start tesselator
  if (levelinfo) fprintf(levelinfo, "gluTessBeginPolygon\n");
  gluTessBeginPolygon(tess, NULL);
  if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
  gluTessBeginContour(tess);
  while (linecount)
  {
    // if there is no connected line, then start new loop
    if ((oldline == currentline) || (startvertex == currentvertex))
    {
      currentline = -1;
      for (i = 0; i < sectors[num].linecount; i++)
        if (!lineadded[i])
        {
          currentline = i;
          currentloop++;
          if ((sectors[num].lines[currentline]->sidenum[0] != NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector == &sectors[num]) : false)
            startvertex = sectors[num].lines[currentline]->v1;
          else
            startvertex = sectors[num].lines[currentline]->v2;
          if (levelinfo) fprintf(levelinfo, "\tNew Loop %3i\n", currentloop);
          if (oldline != 0)
          {
            if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
            gluTessEndContour(tess);
            //            if (levelinfo) fprintf(levelinfo, "\tgluNextContour\n");
            //            gluNextContour(tess, GLU_CW);
            if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
            gluTessBeginContour(tess);
          }
          break;
        }
    }
    if (currentline == -1)
      break;
    // add current line
    lineadded[currentline] = true;
    // check if currentsector is on the front side of the line ...
    if ((sectors[num].lines[currentline]->sidenum[0] != NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector == &sectors[num]) : false)
    {
      // v2 is ending vertex
      currentvertex = sectors[num].lines[currentline]->v2;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v1->x, sectors[num].lines[currentline]->v1->y, sectors[num].lines[currentline]->v2->x, sectors[num].lines[currentline]->v2->y);
      lineangle = (lineangle >> ANGLETOFINESHIFT) * 360 / 8192;

      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped false\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    else // ... or on the back side
    {
      // v1 is ending vertex
      currentvertex = sectors[num].lines[currentline]->v1;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v2->x, sectors[num].lines[currentline]->v2->y, sectors[num].lines[currentline]->v1->x, sectors[num].lines[currentline]->v1->y);
      lineangle = (lineangle >> ANGLETOFINESHIFT) * 360 / 8192;

      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped true\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    if (vertexnum >= maxvertexnum)
    {
      maxvertexnum += 512;
      v = Z_Realloc(v, maxvertexnum * 3 * sizeof(double), PU_LEVEL, 0);
    }
    // calculate coordinates for the glu tesselation functions
    v[vertexnum * 3 + 0] = -(double)currentvertex->x / (double)MAP_SCALE;
    v[vertexnum * 3 + 1] = 0.0;
    v[vertexnum * 3 + 2] = (double)currentvertex->y / (double)MAP_SCALE;
    // add the vertex to the tesselator, currentvertex is the pointer to the vertexlist of doom
    // v[vertexnum] is the GLdouble array of the current vertex
    if (levelinfo) fprintf(levelinfo, "\t\tgluTessVertex(%i, %i)\n", currentvertex->x >> FRACBITS, currentvertex->y >> FRACBITS);
    gluTessVertex(tess, &v[vertexnum * 3], currentvertex);
    // increase vertexindex
    vertexnum++;
    // decrease linecount of current sector
    linecount--;
    // find the next line
    oldline = currentline; // if this isn't changed at the end of the search, a new loop will start
    bestline = -1; // set to start values
    bestlinecount = 0;
    // set backsector if there is one
    /*if (sectors[num].lines[currentline]->sidenum[1]!=NO_INDEX)
      backsector=sides[sectors[num].lines[currentline]->sidenum[1]].sector;
    else
      backsector=NULL;*/
      // search through all lines of the current sector
    for (i = 0; i < sectors[num].linecount; i++)
      if (!lineadded[i]) // if the line isn't already added ...
        // check if one of the vertexes is the same as the current vertex
        if ((sectors[num].lines[i]->v1 == currentvertex) || (sectors[num].lines[i]->v2 == currentvertex))
        {
          // calculate the angle of this best line candidate
          if ((sectors[num].lines[i]->sidenum[0] != NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector == &sectors[num]) : false)
            angle = R_PointToAngle2(sectors[num].lines[i]->v1->x, sectors[num].lines[i]->v1->y, sectors[num].lines[i]->v2->x, sectors[num].lines[i]->v2->y);
          else
            angle = R_PointToAngle2(sectors[num].lines[i]->v2->x, sectors[num].lines[i]->v2->y, sectors[num].lines[i]->v1->x, sectors[num].lines[i]->v1->y);
          angle = (angle >> ANGLETOFINESHIFT) * 360 / 8192;

          //e6y: direction of a line shouldn't be changed
          //if (angle>=180)
          //  angle=angle-360;

          // check if line is flipped ...
          if ((sectors[num].lines[i]->sidenum[0] != NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector == &sectors[num]) : false)
          {
            // when the line is not flipped and startvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v1 != currentvertex)
              continue;
          }
          else
          {
            // when the line is flipped and endvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v2 != currentvertex)
              continue;
          }
          // set new best line candidate
          if (bestline == -1) // if this is the first one ...
          {
            bestline = i;
            bestangle = lineangle - angle;
            bestlinecount++;
          }
          else
            // check if the angle between the current line and this best line candidate is smaller then
            // the angle of the last candidate
            // e6y: for finding an angle between AB and BC vectors we should subtract
            // (BC - BA) == (BC - (180 - AB)) == (angle-(180-lineangle))
            if (D_abs(angle - (180 - lineangle)) < D_abs(bestangle))
            {
              bestline = i;
              bestangle = angle - (180 - lineangle);
              bestlinecount++;
            }
        }
    if (bestline != -1) // if a line is found, make it the current line
    {
      currentline = bestline;
      if (bestlinecount > 1)
        if (levelinfo) fprintf(levelinfo, "\t\tBestlinecount: %4i\n", bestlinecount);
    }
  }
  // let the tesselator calculate the loops
  if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
  gluTessEndContour(tess);
  if (levelinfo) fprintf(levelinfo, "gluTessEndPolygon\n");
  gluTessEndPolygon(tess);
  // clean memory
  gluDeleteTess(tess);
  Z_Free(v);
  Z_Free(lineadded);
}
#endif // USE_GLU_TESS


static void RTP_GetSubSectorVertices(void)
{
  int      i, j;
  int      numedgepoints;
  subsector_t *ssector;

  for (i = 0; i < numsubsectors; i++)
  {
    ssector = &subsectors[i];

    if ((ssector->sector->flags & SECTOR_IS_CLOSED) && !rtp_triangulate_subsectors)
      continue;

    numedgepoints = ssector->numlines;

    RTP_AddGlobalVertexes(numedgepoints);

    if (rtp_flats_vbo.positions && rtp_flats_vbo.texCoords)
    {
      int currentsector = ssector->sector->iSectorID;
      RTPLoopDef **loop;
      int *loopcount;

      if (rtp_triangulate_subsectors)
      {
        loop = &rtp_subsectorloops[i].loops;
        loopcount = &rtp_subsectorloops[i].loopcount;
      }
      else
      {
        loop = &rtp_sectorloops[currentsector].loops;
        loopcount = &rtp_sectorloops[currentsector].loopcount;
      }

      (*loopcount)++;
      (*loop) = Z_Realloc((*loop), sizeof(RTPLoopDef) * (*loopcount), PU_STATIC, 0);
      ((*loop)[(*loopcount) - 1]).index = i;
      ((*loop)[(*loopcount) - 1]).mode = RTP_TRIANGLE_MODE_TRIANGLE_FAN;
      ((*loop)[(*loopcount) - 1]).vertexcount = numedgepoints;
      ((*loop)[(*loopcount) - 1]).vertexindex = rtp_num_vertexes;

      for (j = 0; j < numedgepoints; j++)
      {
        RTPPosition *dst_p = &rtp_flats_vbo.positions[rtp_num_vertexes];
        RTPTexCoord *dst_t = &rtp_flats_vbo.texCoords[rtp_num_vertexes];

        dst_p->x = -(float)(segs[ssector->firstline + j].v1->x) / MAP_SCALE;
        dst_p->y = 0.0f;
        dst_p->z =  (float)(segs[ssector->firstline + j].v1->y) / MAP_SCALE;

        dst_t->u = ( (float)(segs[ssector->firstline + j].v1->x) / FRACUNIT) / 64.0f;
        dst_t->v = (-(float)(segs[ssector->firstline + j].v1->y) / FRACUNIT) / 64.0f;

        rtp_num_vertexes++;
      }
    }
  }
}


static void RTP_MarkSectorsForClamp(void)
{
  // TODO RT: impl this function, if seams are visible:
  // gl_preprocess: All sectors which are inside 64x64 square of map grid should be marked here.
  //                GL_CLAMP instead of GL_REPEAT should be used for them for avoiding seams
}


static void RTP_ResetTexturedAutomap(void)
{
  // gld_ResetTexturedAutomap
}
static void RTP_ProcessTexturedMap(void)
{
  // RT: let always map_textured=false for AutoMap
}


static void RTP_PreprocessSectors(void)
{
#ifdef USE_GLU_TESS // figgi
  char *vertexcheck = NULL;
  char *vertexcheck2 = NULL;
  int v1num;
  int v2num;
  int i;
  int j;
#endif

#ifdef PRBOOM_DEBUG
  levelinfo = fopen("levelinfo.txt", "a");
  if (levelinfo)
  {
    if (gamemode == commercial)
      fprintf(levelinfo, "MAP%02i\n", gamemap);
    else
      fprintf(levelinfo, "E%iM%i\n", gameepisode, gamemap);
  }
#endif

  if (numsectors)
  {
    rtp_sectorloops = Z_Malloc(sizeof(RTPSector) * numsectors, PU_STATIC, 0);
    if (!rtp_sectorloops)
      I_Error("RTP_PreprocessSectors: Not enough memory for array sectorloops");
    memset(rtp_sectorloops, 0, sizeof(RTPSector) * numsectors);
  }

  if (numsubsectors)
  {
    rtp_subsectorloops = Z_Malloc(sizeof(RTPMapSubsector) * numsubsectors, PU_STATIC, 0);
    if (!rtp_subsectorloops)
      I_Error("RTP_PreprocessSectors: Not enough memory for array subsectorloops");
    memset(rtp_subsectorloops, 0, sizeof(RTPMapSubsector) * numsubsectors);
  }

  if (numsegs)
  {
    //segrendered = calloc(numsegs, sizeof(byte));
    //if (!segrendered)
    //  I_Error("RTP_PreprocessSectors: Not enough memory for array segrendered");
  }

  if (numlines)
  {
    //linerendered[0] = calloc(numlines, sizeof(byte));
    //linerendered[1] = calloc(numlines, sizeof(byte));
    //if (!linerendered[0] || !linerendered[1])
    //  I_Error("RTP_PreprocessSectors: Not enough memory for array linerendered");
  }

  rtp_flats_vbo.positions = NULL;
  rtp_flats_vbo.texCoords = NULL;
  rtp_max_vertexes = 0;
  rtp_num_vertexes = 0;
  if (numvertexes)
  {
    RTP_AddGlobalVertexes(numvertexes * 2);
  }

#ifdef USE_GLU_TESS
  if (numvertexes)
  {
    vertexcheck = malloc(numvertexes * sizeof(vertexcheck[0]));
    vertexcheck2 = malloc(numvertexes * sizeof(vertexcheck2[0]));
    if (!vertexcheck || !vertexcheck2)
    {
      if (levelinfo) fclose(levelinfo);
      I_Error("RTP_PreprocessSectors: Not enough memory for array vertexcheck");
      return;
    }
  }

  for (i = 0; i < numsectors; i++)
  {
    memset(vertexcheck, 0, numvertexes * sizeof(vertexcheck[0]));
    memset(vertexcheck2, 0, numvertexes * sizeof(vertexcheck2[0]));
    for (j = 0; j < sectors[i].linecount; j++)
    {
      v1num = ((intptr_t)sectors[i].lines[j]->v1 - (intptr_t)vertexes) / sizeof(vertex_t);
      v2num = ((intptr_t)sectors[i].lines[j]->v2 - (intptr_t)vertexes) / sizeof(vertex_t);
      if ((v1num >= numvertexes) || (v2num >= numvertexes))
        continue;

      // e6y: for correct handling of missing textures.
      // We do not need to apply some algos for isolated lines.
      vertexcheck2[v1num]++;
      vertexcheck2[v2num]++;

      if (sectors[i].lines[j]->sidenum[0] != NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[0]].sector == &sectors[i])
        {
          vertexcheck[v1num] |= 1;
          vertexcheck[v2num] |= 2;
        }
      if (sectors[i].lines[j]->sidenum[1] != NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[1]].sector == &sectors[i])
        {
          vertexcheck[v1num] |= 2;
          vertexcheck[v2num] |= 1;
        }
    }
    if (sectors[i].linecount < 3)
    {
    #ifdef PRBOOM_DEBUG
      lprintf(LO_ERROR, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
    #endif
      if (levelinfo) fprintf(levelinfo, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      sectors[i].flags &= ~SECTOR_IS_CLOSED;
    }
    else
    {
      sectors[i].flags |= SECTOR_IS_CLOSED;
      for (j = 0; j < numvertexes; j++)
      {
        if ((vertexcheck[j] == 1) || (vertexcheck[j] == 2))
        {
        #ifdef PRBOOM_DEBUG
          lprintf(LO_ERROR, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
        #endif
          if (levelinfo) fprintf(levelinfo, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
          sectors[i].flags &= ~SECTOR_IS_CLOSED;
        }
      }
    }

    // e6y: marking all the isolated lines
    for (j = 0; j < sectors[i].linecount; j++)
    {
      v1num = ((intptr_t)sectors[i].lines[j]->v1 - (intptr_t)vertexes) / sizeof(vertex_t);
      v2num = ((intptr_t)sectors[i].lines[j]->v2 - (intptr_t)vertexes) / sizeof(vertex_t);
      if (vertexcheck2[v1num] < 2 && vertexcheck2[v2num] < 2)
      {
        sectors[i].lines[j]->r_flags |= RF_ISOLATED;
      }
    }

    // figgi -- adapted for glnodes
    if (sectors[i].flags & SECTOR_IS_CLOSED)
      RTP_PrecalculateSector(i);
  }
  free(vertexcheck);
  free(vertexcheck2);
#endif /* USE_GLU_TESS */

  // figgi -- adapted for glnodes
  if (numnodes)
  {
    if (nodesVersion == 0)
      RTP_CarveFlats(numnodes - 1, 0, 0);
    else
      RTP_GetSubSectorVertices();
  }

  RTP_ProcessTexturedMap();

  if (levelinfo) fclose(levelinfo);

  //e6y: for seamless rendering
  RTP_MarkSectorsForClamp();
}


static void RTP_PreprocessSegs(void)
{
  int i;

  rtp_segs = Z_Malloc(numsegs * sizeof(RTPSeg), PU_STATIC, 0);
  for (i = 0; i < numsegs; i++)
  {
    rtp_segs[i].x1 = -(float)segs[i].v1->x / (float)MAP_SCALE;
    rtp_segs[i].z1 =  (float)segs[i].v1->y / (float)MAP_SCALE;
    rtp_segs[i].x2 = -(float)segs[i].v2->x / (float)MAP_SCALE;
    rtp_segs[i].z2 =  (float)segs[i].v2->y / (float)MAP_SCALE;
  }

  rtp_lines = Z_Malloc(numlines * sizeof(RTPSeg), PU_STATIC, 0);
  for (i = 0; i < numlines; i++)
  {
    rtp_lines[i].x1 = -(float)lines[i].v1->x / (float)MAP_SCALE;
    rtp_lines[i].z1 =  (float)lines[i].v1->y / (float)MAP_SCALE;
    rtp_lines[i].x2 = -(float)lines[i].v2->x / (float)MAP_SCALE;
    rtp_lines[i].z2 =  (float)lines[i].v2->y / (float)MAP_SCALE;
  }
}


static void FreeSectors(int sectors_count, int subsectors_count)
{
  for (int i = 0; i < sectors_count; i++)
  {
    free(rtp_sectorloops[i].loops);
  }
  free(rtp_sectorloops);
  rtp_sectorloops = NULL;

  for (int i = 0; i < subsectors_count; i++)
  {
    free(rtp_subsectorloops[i].loops);
  }
  free(rtp_subsectorloops);
  rtp_subsectorloops = NULL;
}


static void RTP_PreprocesSectorGeometryData(void);


void RT_PreprocessLevel(void)
{
  // RT: here, these arrays are kept only while preprocessing
  assert(rtp_sectorloops == NULL);
  assert(rtp_subsectorloops == NULL);

  // speedup of level reloading
  // Do not preprocess GL data twice for same level
  if (!rtp_preprocessed)
  {
    int i;

    free(rtp_segs);
    free(rtp_lines);

    free(rtp_flats_vbo.positions);
    free(rtp_flats_vbo.texCoords);
    memset(&rtp_flats_vbo, 0, sizeof(rtp_flats_vbo));

    //free(segrendered);
    //free(linerendered[0]);
    //free(linerendered[1]);


    RT_Texture_PrecacheTextures();
    RTP_PreprocessSectors();
    //gld_PreprocessFakeSectors();
    RTP_PreprocessSegs();
    RTP_PreprocesSectorGeometryData();


    FreeSectors(numsectors, numsubsectors);
  }
  else
  {
    //gld_PreprocessFakeSectors();

    //memset(segrendered, 0, numsegs * sizeof(segrendered[0]));
    //memset(linerendered[0], 0, numlines * sizeof(linerendered[0][0]));
    //memset(linerendered[1], 0, numlines * sizeof(linerendered[1][0]));
  }

  RTP_ResetTexturedAutomap();

  //gld_FreeDrawInfo();

  if (!rtp_preprocessed)
  {
    // RT: don't free anything, keep for RT_Preprocess_Get* functions

    //free(rtp_flats_vbo.positions);
    //free(rtp_flats_vbo.texCoords);
    //memset(&rtp_flats_vbo, 0, sizeof(rtp_flats_vbo));
  }

  //gld_PreprocessDetail();
  //gld_InitVertexData();

  // rtp_preprocessed = true;
}


static const RTPPosition* GetPreprocessedPositions(int vertex_index)
{
  assert(rtp_flats_vbo.positions != NULL);
  assert(vertex_index < rtp_num_vertexes);

  return &rtp_flats_vbo.positions[vertex_index];
}


static const RTPTexCoord *GetPreprocessedTexCoords(int vertex_index)
{
  assert(rtp_flats_vbo.texCoords != NULL);
  assert(vertex_index < rtp_num_vertexes);

  return &rtp_flats_vbo.texCoords[vertex_index];
}


static int GetTriangleCount(RTPTriangleMode mode, int vertex_count)
{
  switch (mode)
  {
    case RTP_TRIANGLE_MODE_TRIANGLES:
    {
      assert(vertex_count % 3 == 0);
      return vertex_count / 3;
    }
    case RTP_TRIANGLE_MODE_TRIANGLE_STRIP:
    case RTP_TRIANGLE_MODE_TRIANGLE_FAN:
    {
      return i_max(0, vertex_count - 2);
    }
    default:
    {
      assert(0);
      return 0;
    }
  }
}


typedef struct
{
  int                vertex_count;
  RgPrimitiveVertex* vertices;
  int                index_count;
  uint32_t*          indices;
} rttempdata_t;


static int RTP_GetVertexCount(int sectornum)
{
  int vertex_count = 0;

  for (int loopnum = 0; loopnum < rtp_sectorloops[sectornum].loopcount; loopnum++)
  {
    const RTPLoopDef *loop = &rtp_sectorloops[sectornum].loops[loopnum];

    vertex_count += loop->vertexcount;
  }

  return vertex_count;
}


static int RTP_GetIndexCount(int sectornum)
{
  int index_count = 0;

  for (int loopnum = 0; loopnum < rtp_sectorloops[sectornum].loopcount; loopnum++)
  {
    const RTPLoopDef *loop = &rtp_sectorloops[sectornum].loops[loopnum];

    index_count += 3 * GetTriangleCount(loop->mode, loop->vertexcount);
  }

  return index_count;
}


static rttempdata_t RTP_CreateSectorGeometryData(int sectornum)
{
  rttempdata_t result = { 0 };
  result.vertex_count = RTP_GetVertexCount(sectornum);
  result.index_count = RTP_GetIndexCount(sectornum);

  result.vertices = calloc(sizeof(RgPrimitiveVertex), result.vertex_count);
  result.indices = calloc(sizeof(uint32_t), result.index_count);

  int vertex_iter = 0;
  int index_iter = 0;

  for (int loopnum = 0; loopnum < rtp_sectorloops[sectornum].loopcount; loopnum++)
  {
    const RTPLoopDef *loop = &rtp_sectorloops[sectornum].loops[loopnum];

    {
      {
        const RTPPosition* pos = GetPreprocessedPositions(loop->vertexindex);
        const RTPTexCoord* tex = GetPreprocessedTexCoords(loop->vertexindex);

        for (int i = 0; i < loop->vertexcount; i++)
        {
          RgPrimitiveVertex v = {
              .position = { pos[i].x, pos[i].y, pos[i].z },
              .normal   = { 0, 1, 0 },
              .tangent  = { 1, 0, 0, 1 },
              .texCoord = { tex[i].u, tex[i].v },
              .color    = RG_PACKED_COLOR_WHITE,
          };

          result.vertices[vertex_iter + i] = v;
        } 
      }
      
      switch (loop->mode)
      {
        case RTP_TRIANGLE_MODE_TRIANGLES:
        {
          for (int t = 0; t < GetTriangleCount(loop->mode, loop->vertexcount); t++)
          {
            result.indices[index_iter] = vertex_iter + t * 3 + 0;       index_iter++;
            result.indices[index_iter] = vertex_iter + t * 3 + 1;       index_iter++;
            result.indices[index_iter] = vertex_iter + t * 3 + 2;       index_iter++;
          }
          break;
        }
        case RTP_TRIANGLE_MODE_TRIANGLE_STRIP:
        {
          for (int t = 0; t < GetTriangleCount(loop->mode, loop->vertexcount); t++)
          {
            result.indices[index_iter] = vertex_iter + t;               index_iter++;
            result.indices[index_iter] = vertex_iter + t + (1 + t % 2); index_iter++;
            result.indices[index_iter] = vertex_iter + t + (2 - t % 2); index_iter++;
          }
          break;
        }
        case RTP_TRIANGLE_MODE_TRIANGLE_FAN:
        {
          for (int t = 0; t < GetTriangleCount(loop->mode, loop->vertexcount); t++)
          {
            result.indices[index_iter] = vertex_iter + t + 1;           index_iter++;
            result.indices[index_iter] = vertex_iter + t + 2;           index_iter++;
            result.indices[index_iter] = vertex_iter + 0;               index_iter++;
          }
          break;
        }
        default: assert(0); break;
      }
    }

    vertex_iter += loop->vertexcount;
  }

  return result;
}


static void RTP_DestroySectorGeometryData(const rttempdata_t *data)
{
  free(data->vertices);
  free(data->indices);
}


static struct
{
  RgPrimitiveVertex* vertices;
  uint32_t*          indices;
} rtp_all_buffer = { 0 };

static rtsectordata_t *rtp_all_sectorinfos = NULL;


static void RTP_PreprocesSectorGeometryData(void)
{
  free(rtp_all_buffer.vertices);
  free(rtp_all_buffer.indices);
  free(rtp_all_sectorinfos);


  int all_vertcount = 0, all_indexcount = 0;
  for (int i = 0; i < numsectors; i++)
  {
    all_vertcount += RTP_GetVertexCount(i);
    all_indexcount += RTP_GetIndexCount(i);
  }

  rtp_all_buffer.vertices         = calloc(all_vertcount, sizeof(*rtp_all_buffer.vertices));
  rtp_all_buffer.indices          = calloc(all_indexcount, sizeof(*rtp_all_buffer.indices));
  rtp_all_sectorinfos             = calloc(numsectors, sizeof(*rtp_all_sectorinfos));


  RgPrimitiveVertex* vertices_iter = rtp_all_buffer.vertices;
  uint32_t*          indices_iter  = rtp_all_buffer.indices;

  for (int i = 0; i < numsectors; i++)
  {
    const rttempdata_t data = RTP_CreateSectorGeometryData(i);

    memcpy(vertices_iter, data.vertices, data.vertex_count * sizeof(*data.vertices));
    memcpy(indices_iter, data.indices, data.index_count * sizeof(*data.indices));


    rtp_all_sectorinfos[i].vertex_count = data.vertex_count;
    rtp_all_sectorinfos[i].vertices     = vertices_iter;
    rtp_all_sectorinfos[i].index_count  = data.index_count;
    rtp_all_sectorinfos[i].indices      = indices_iter;


    vertices_iter += data.vertex_count;
    indices_iter += data.index_count;

    RTP_DestroySectorGeometryData(&data);
  }


  int max_vertcount = 0;
  for (int i = 0; i < numsectors; i++)
  {
    max_vertcount = i_max(max_vertcount, RTP_GetVertexCount(i));
  }
}


rtsectordata_t RT_GetSectorGeometryData(int sectornum, dboolean is_ceiling)
{
  return rtp_all_sectorinfos[sectornum];
}


void RT_GetLineInfo(int lineid, float *out_x1, float *out_z1, float *out_x2, float *out_z2)
{
  assert(lineid >= 0 && lineid < numlines);

  const RTPSeg *line = &rtp_lines[lineid];

  *out_x1 = line->x1;
  *out_z1 = line->z1;
  *out_x2 = line->x2;
  *out_z2 = line->z2;
}
