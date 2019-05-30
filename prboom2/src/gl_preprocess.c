/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#include <math.h>

#include "gl_opengl.h"

#include "z_zone.h"
#include "doomstat.h"
#include "gl_intern.h"
#include "gl_struct.h"
#include "p_maputl.h"
#include "r_main.h"
#include "am_map.h"
#include "lprintf.h"

static FILE *levelinfo;

static int gld_max_vertexes = 0;
static int gld_num_vertexes = 0;

static int triangulate_subsectors = 0;

// this is the list for all sectors to the loops
GLSector *sectorloops;

// this is the list for all subsectors to the loops
// uses by textured automap
GLMapSubsector *subsectorloops;

static void gld_AddGlobalVertexes(int count)
{
  if ((gld_num_vertexes+count)>=gld_max_vertexes)
  {
    gld_max_vertexes+=count+1024;
    flats_vbo = Z_Realloc(flats_vbo, gld_max_vertexes * sizeof(flats_vbo[0]), PU_STATIC, 0);
  }
}

/*****************************
 *
 * FLATS
 *
 *****************************/

/* proff - 05/15/2000
 * The idea and algorithm to compute the flats with nodes and subsectors is
 * originaly from JHexen. I have redone it.
 */

#define FIX2DBL(x)    ((double)(x))
#define MAX_CC_SIDES  128

static dboolean gld_PointOnSide(vertex_t *p, divline_t *d)
{
  // We'll return false if the point c is on the left side.
  return ((FIX2DBL(d->y)-FIX2DBL(p->y))*FIX2DBL(d->dx)-(FIX2DBL(d->x)-FIX2DBL(p->x))*FIX2DBL(d->dy) >= 0);
}

// Lines start-end and fdiv must intersect.
static void gld_CalcIntersectionVertex(vertex_t *s, vertex_t *e, divline_t *d, vertex_t *i)
{
  double ax = FIX2DBL(s->x), ay = FIX2DBL(s->y), bx = FIX2DBL(e->x), by = FIX2DBL(e->y);
  double cx = FIX2DBL(d->x), cy = FIX2DBL(d->y), dx = cx+FIX2DBL(d->dx), dy = cy+FIX2DBL(d->dy);
  double r = ((ay-cy)*(dx-cx)-(ax-cx)*(dy-cy)) / ((bx-ax)*(dy-cy)-(by-ay)*(dx-cx));
  i->x = (fixed_t)((double)s->x + r*((double)e->x-(double)s->x));
  i->y = (fixed_t)((double)s->y + r*((double)e->y-(double)s->y));
}

#undef FIX2DBL

// Returns a pointer to the list of points. It must be used.
//
static vertex_t *gld_FlatEdgeClipper(int *numpoints, vertex_t *points, int numclippers, divline_t *clippers)
{
  unsigned char sidelist[MAX_CC_SIDES];
  int       i, k, num = *numpoints;

  // We'll clip the polygon with each of the divlines. The left side of
  // each divline is discarded.
  for(i=0; i<numclippers; i++)
  {
    divline_t *curclip = &clippers[i];

    // First we'll determine the side of each vertex. Points are allowed
    // to be on the line.
    for(k=0; k<num; k++)
      sidelist[k] = gld_PointOnSide(&points[k], curclip);

    for(k=0; k<num; k++)
    {
      int startIdx = k, endIdx = k+1;
      // Check the end index.
      if(endIdx == num) endIdx = 0; // Wrap-around.
      // Clipping will happen when the ends are on different sides.
      if(sidelist[startIdx] != sidelist[endIdx])
      {
        vertex_t newvert;

        gld_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

        // Add the new vertex. Also modify the sidelist.
        points = (vertex_t*)Z_Realloc(points,(++num)*sizeof(vertex_t),PU_LEVEL,0);
        if(num >= MAX_CC_SIDES)
          I_Error("gld_FlatEdgeClipper: Too many points in carver");

        // Make room for the new vertex.
        memmove(&points[endIdx+1], &points[endIdx],
          (num - endIdx-1)*sizeof(vertex_t));
        memcpy(&points[endIdx], &newvert, sizeof(newvert));

        memmove(&sidelist[endIdx+1], &sidelist[endIdx], num-endIdx-1);
        sidelist[endIdx] = 1;

        // Skip over the new vertex.
        k++;
      }
    }

    // Now we must discard the points that are on the wrong side.
    for(k=0; k<num; k++)
      if(!sidelist[k])
      {
        memmove(&points[k], &points[k+1], (num - k-1)*sizeof(vertex_t));
        memmove(&sidelist[k], &sidelist[k+1], num - k-1);
        num--;
        k--;
      }
  }
  // Screen out consecutive identical points.
  for(i=0; i<num; i++)
  {
    int previdx = i-1;
    if(previdx < 0) previdx = num - 1;
    if(points[i].x == points[previdx].x
      && points[i].y == points[previdx].y)
    {
      // This point (i) must be removed.
      memmove(&points[i], &points[i+1], sizeof(vertex_t)*(num-i-1));
      num--;
      i--;
    }
  }
  *numpoints = num;
  return points;
}

static void gld_FlatConvexCarver(int ssidx, int num, divline_t *list)
{
  subsector_t *ssec=&subsectors[ssidx];
  int numclippers = num+ssec->numlines;
  divline_t *clippers;
  int i, numedgepoints;
  vertex_t *edgepoints;

  clippers=(divline_t*)Z_Malloc(numclippers*sizeof(divline_t),PU_LEVEL,0);
  if (!clippers)
    return;
  for(i=0; i<num; i++)
  {
    clippers[i].x = list[num-i-1].x;
    clippers[i].y = list[num-i-1].y;
    clippers[i].dx = list[num-i-1].dx;
    clippers[i].dy = list[num-i-1].dy;
  }
  for(i=num; i<numclippers; i++)
  {
    seg_t *seg = &segs[ssec->firstline+i-num];
    clippers[i].x = seg->v1->x;
    clippers[i].y = seg->v1->y;
    clippers[i].dx = seg->v2->x-seg->v1->x;
    clippers[i].dy = seg->v2->y-seg->v1->y;
  }

  // Setup the 'worldwide' polygon.
  numedgepoints = 4;
  edgepoints = (vertex_t*)Z_Malloc(numedgepoints*sizeof(vertex_t),PU_LEVEL,0);

  edgepoints[0].x = INT_MIN;
  edgepoints[0].y = INT_MAX;

  edgepoints[1].x = INT_MAX;
  edgepoints[1].y = INT_MAX;

  edgepoints[2].x = INT_MAX;
  edgepoints[2].y = INT_MIN;

  edgepoints[3].x = INT_MIN;
  edgepoints[3].y = INT_MIN;

  // Do some clipping, <snip> <snip>
  edgepoints = gld_FlatEdgeClipper(&numedgepoints, edgepoints, numclippers, clippers);

  if(!numedgepoints)
  {
    if (levelinfo) fprintf(levelinfo, "All carved away: subsector %li - sector %i\n", ssec-subsectors, ssec->sector->iSectorID);
  }
  else
  {
    if(numedgepoints >= 3)
    {
      gld_AddGlobalVertexes(numedgepoints);
      if (flats_vbo)
      {
        int currentsector=ssec->sector->iSectorID;
        GLLoopDef **loop;
        int *loopcount;

        if (triangulate_subsectors)
        {
          loop = &subsectorloops[ ssidx ].loops;
          loopcount = &subsectorloops[ ssidx ].loopcount;
        }
        else
        {
          loop = &sectorloops[ currentsector ].loops;
          loopcount = &sectorloops[ currentsector ].loopcount;
        }

        (*loopcount)++;
        (*loop) = Z_Realloc((*loop), sizeof(GLLoopDef)*(*loopcount), PU_STATIC, 0);
        ((*loop)[(*loopcount) - 1]).index = ssidx;
        ((*loop)[(*loopcount) - 1]).mode = GL_TRIANGLE_FAN;
        ((*loop)[(*loopcount) - 1]).vertexcount = numedgepoints;
        ((*loop)[(*loopcount) - 1]).vertexindex = gld_num_vertexes;

        for(i = 0;  i < numedgepoints; i++)
        {
          flats_vbo[gld_num_vertexes].u = ( (float)edgepoints[i].x/(float)FRACUNIT)/64.0f;
          flats_vbo[gld_num_vertexes].v = (-(float)edgepoints[i].y/(float)FRACUNIT)/64.0f;
          flats_vbo[gld_num_vertexes].x = -(float)edgepoints[i].x/MAP_SCALE;
          flats_vbo[gld_num_vertexes].y = 0.0f;
          flats_vbo[gld_num_vertexes].z =  (float)edgepoints[i].y/MAP_SCALE;
          gld_num_vertexes++;
        }
      }
    }
  }
  // We're done, free the edgepoints memory.
  Z_Free(edgepoints);
  Z_Free(clippers);
}

static void gld_CarveFlats(int bspnode, int numdivlines, divline_t *divlines)
{
  node_t    *nod;
  divline_t *childlist, *dl;
  int     childlistsize = numdivlines+1;

  // If this is a subsector we are dealing with, begin carving with the
  // given list.
  if(bspnode & NF_SUBSECTOR)
  {
    // We have arrived at a subsector. The divline list contains all
    // the partition lines that carve out the subsector.
    // special case for trivial maps (no nodes, single subsector)
    int ssidx = (numnodes != 0) ? bspnode & (~NF_SUBSECTOR) : 0;

    if (!(subsectors[ssidx].sector->flags & SECTOR_IS_CLOSED) || triangulate_subsectors)
      gld_FlatConvexCarver(ssidx, numdivlines, divlines);
    return;
  }

  // Get a pointer to the node.
  nod = nodes + bspnode;

  // Allocate a new list for each child.
  childlist = (divline_t*)Z_Malloc(childlistsize*sizeof(divline_t),PU_LEVEL,0);

  // Copy the previous lines.
  if(divlines) memcpy(childlist,divlines,numdivlines*sizeof(divline_t));

  dl = childlist + numdivlines;
  dl->x = nod->x;
  dl->y = nod->y;
  // The right child gets the original line (LEFT side clipped).
  dl->dx = nod->dx;
  dl->dy = nod->dy;
  gld_CarveFlats(nod->children[0],childlistsize,childlist);

  // The left side. We must reverse the line, otherwise the wrong
  // side would get clipped.
  dl->dx = -nod->dx;
  dl->dy = -nod->dy;
  gld_CarveFlats(nod->children[1],childlistsize,childlist);

  // We are finishing with this node, free the allocated list.
  Z_Free(childlist);
}

#ifdef USE_GLU_TESS

static int currentsector; // the sector which is currently tesselated

// ntessBegin
//
// called when the tesselation of a new loop starts

static void CALLBACK ntessBegin( GLenum type )
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
  {
    if (type==GL_TRIANGLES)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLES\n");
    else
    if (type==GL_TRIANGLE_FAN)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_FAN\n");
    else
    if (type==GL_TRIANGLE_STRIP)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_STRIP\n");
    else
      fprintf(levelinfo, "\t\tBegin: unknown\n");
  }
#endif
  // increase loopcount for currentsector
  sectorloops[ currentsector ].loopcount++;
  // reallocate to get space for another loop
  // PU_LEVEL is used, so this gets freed before a new level is loaded
  sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_STATIC, 0);
  // set initial values for current loop
  // currentloop is -> sectorloops[currentsector].loopcount-1
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].index=-1;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=type;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=0;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexindex=gld_num_vertexes;
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

static void CALLBACK ntessCombine( GLdouble coords[3], vertex_t *vert[4], GLfloat w[4], void **dataOut )
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
  {
    fprintf(levelinfo, "\t\tVertexCombine Coords: x %10.5f, y %10.5f z %10.5f\n", coords[0], coords[1], coords[2]);
    if (vert[0]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[0]->x>>FRACBITS, vert[0]->y>>FRACBITS, vert[0]);
    if (vert[1]) fprintf(levelinfo, "\t\tVertexCombine Vert2 : x %10i, y %10i p %p\n", vert[1]->x>>FRACBITS, vert[1]->y>>FRACBITS, vert[1]);
    if (vert[2]) fprintf(levelinfo, "\t\tVertexCombine Vert3 : x %10i, y %10i p %p\n", vert[2]->x>>FRACBITS, vert[2]->y>>FRACBITS, vert[2]);
    if (vert[3]) fprintf(levelinfo, "\t\tVertexCombine Vert4 : x %10i, y %10i p %p\n", vert[3]->x>>FRACBITS, vert[3]->y>>FRACBITS, vert[3]);
  }
#endif
  // just return the first vertex, because all vertexes are on the same coordinate
  *dataOut = vert[0];
}

// ntessVertex
//
// called when a vertex is found

static void CALLBACK ntessVertex( vertex_t *vert )
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tVertex : x %10i, y %10i\n", vert->x>>FRACBITS, vert->y>>FRACBITS);
#endif
  // increase vertex count
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount++;

  // increase vertex count
  gld_AddGlobalVertexes(1);
  // add the new vertex (vert is the second argument of gluTessVertex)
  flats_vbo[gld_num_vertexes].u=( (float)vert->x/(float)FRACUNIT)/64.0f;
  flats_vbo[gld_num_vertexes].v=(-(float)vert->y/(float)FRACUNIT)/64.0f;
  flats_vbo[gld_num_vertexes].x=-(float)vert->x/MAP_SCALE;
  flats_vbo[gld_num_vertexes].y=0.0f;
  flats_vbo[gld_num_vertexes].z= (float)vert->y/MAP_SCALE;
  gld_num_vertexes++;
}

// ntessEnd
//
// called when the tesselation of a the current loop ends (DEBUG only)

static void CALLBACK ntessEnd( void )
{
#ifdef PRBOOM_DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tEnd loopcount %i vertexcount %i\n", sectorloops[currentsector].loopcount, sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount);
#endif
}

// gld_PrecalculateSector
//
// this calculates the loops for the sector "num"
//
// how does it work?
// first I have to credit Michael 'Kodak' Ryssen for the usage of the
// glu tesselation functions. the rest of this stuff is entirely done by me (proff).
// if there are any similarities, then they are implications of the algorithm.
//
// I'm starting with the first line of the current sector. I take it's ending vertex and
// add it to the tesselator. the current line is marked as used. then I'm searching for
// the next line which connects to the current line. if there is more than one line, I
// choose the one with the smallest angle to the current. if there is no next line, I
// start a new loop and take the first unused line in the sector. after all lines are
// processed, the polygon is tesselated.
//
// e6y
// The bug in algorithm of splitting of a sector into the closed contours was fixed.
// There is no more HOM at the starting area on MAP16 @ Eternal.wad
// I hope nothing was broken

static void gld_PrecalculateSector(int num)
{
  int i;
  dboolean *lineadded=NULL;
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
  double *v=NULL;
  int maxvertexnum;
  int vertexnum;

  currentsector=num;
  lineadded=Z_Malloc(sectors[num].linecount*sizeof(dboolean),PU_LEVEL,0);
  if (!lineadded)
  {
    if (levelinfo) fclose(levelinfo);
    return;
  }
  // init tesselator
  tess=gluNewTess();
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
  for (i=0; i<sectors[num].linecount; i++)
  {
    lineadded[i]=false;
    if (sectors[num].lines[i]->sidenum[0]!=NO_INDEX)
      if (sectors[num].lines[i]->sidenum[1]!=NO_INDEX)
        if (sides[sectors[num].lines[i]->sidenum[0]].sector
          ==sides[sectors[num].lines[i]->sidenum[1]].sector)
        {
          lineadded[i]=true;
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
  linecount=sectors[num].linecount;
  oldline=0;
  currentline=0;
  startvertex=sectors[num].lines[currentline]->v2;
  currentloop=0;
  vertexnum=0;
  maxvertexnum=0;
  // start tesselator
  if (levelinfo) fprintf(levelinfo, "gluTessBeginPolygon\n");
  gluTessBeginPolygon(tess, NULL);
  if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
  gluTessBeginContour(tess);
  while (linecount)
  {
    // if there is no connected line, then start new loop
    if ((oldline==currentline) || (startvertex==currentvertex))
    {
      currentline=-1;
      for (i=0; i<sectors[num].linecount; i++)
        if (!lineadded[i])
        {
          currentline=i;
          currentloop++;
          if ((sectors[num].lines[currentline]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
            startvertex=sectors[num].lines[currentline]->v1;
          else
            startvertex=sectors[num].lines[currentline]->v2;
          if (levelinfo) fprintf(levelinfo, "\tNew Loop %3i\n", currentloop);
          if (oldline!=0)
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
    if (currentline==-1)
      break;
    // add current line
    lineadded[currentline]=true;
    // check if currentsector is on the front side of the line ...
    if ((sectors[num].lines[currentline]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
    {
      // v2 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v2;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y,sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      
      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped false\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    else // ... or on the back side
    {
      // v1 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v1;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y,sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;

      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped true\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    if (vertexnum>=maxvertexnum)
    {
      maxvertexnum+=512;
      v=Z_Realloc(v,maxvertexnum*3*sizeof(double),PU_LEVEL,0);
    }
    // calculate coordinates for the glu tesselation functions
    v[vertexnum*3+0]=-(double)currentvertex->x/(double)MAP_SCALE;
    v[vertexnum*3+1]=0.0;
    v[vertexnum*3+2]= (double)currentvertex->y/(double)MAP_SCALE;
    // add the vertex to the tesselator, currentvertex is the pointer to the vertexlist of doom
    // v[vertexnum] is the GLdouble array of the current vertex
    if (levelinfo) fprintf(levelinfo, "\t\tgluTessVertex(%i, %i)\n",currentvertex->x>>FRACBITS,currentvertex->y>>FRACBITS);
    gluTessVertex(tess, &v[vertexnum*3], currentvertex);
    // increase vertexindex
    vertexnum++;
    // decrease linecount of current sector
    linecount--;
    // find the next line
    oldline=currentline; // if this isn't changed at the end of the search, a new loop will start
    bestline=-1; // set to start values
    bestlinecount=0;
    // set backsector if there is one
    /*if (sectors[num].lines[currentline]->sidenum[1]!=NO_INDEX)
      backsector=sides[sectors[num].lines[currentline]->sidenum[1]].sector;
    else
      backsector=NULL;*/
    // search through all lines of the current sector
    for (i=0; i<sectors[num].linecount; i++)
      if (!lineadded[i]) // if the line isn't already added ...
        // check if one of the vertexes is the same as the current vertex
        if ((sectors[num].lines[i]->v1==currentvertex) || (sectors[num].lines[i]->v2==currentvertex))
        {
          // calculate the angle of this best line candidate
          if ((sectors[num].lines[i]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
            angle = R_PointToAngle2(sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y,sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y);
          else
            angle = R_PointToAngle2(sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y,sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y);
          angle=(angle>>ANGLETOFINESHIFT)*360/8192;

          //e6y: direction of a line shouldn't be changed
          //if (angle>=180)
          //  angle=angle-360;

          // check if line is flipped ...
          if ((sectors[num].lines[i]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
          {
            // when the line is not flipped and startvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v1!=currentvertex)
              continue;
          }
          else
          {
            // when the line is flipped and endvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v2!=currentvertex)
              continue;
          }
          // set new best line candidate
          if (bestline==-1) // if this is the first one ...
          {
            bestline=i;
            bestangle=lineangle-angle;
            bestlinecount++;
          }
          else
            // check if the angle between the current line and this best line candidate is smaller then
            // the angle of the last candidate
            // e6y: for finding an angle between AB and BC vectors we should subtract
            // (BC - BA) == (BC - (180 - AB)) == (angle-(180-lineangle))
            if (D_abs(angle-(180-lineangle))<D_abs(bestangle))
            {
              bestline=i;
              bestangle=angle-(180-lineangle);
              bestlinecount++;
            }
        }
    if (bestline!=-1) // if a line is found, make it the current line
    {
      currentline=bestline;
      if (bestlinecount>1)
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

#endif /* USE_GLU_TESS */

/********************************************
 * Name     : gld_GetSubSectorVertices      *
 * created  : 08/13/00                      *
 * modified : 09/18/00, adapted for PrBoom  *
 * author   : figgi                         *
 * what     : prepares subsectorvertices    *
 *            (glnodes only)                *
 ********************************************/

static void gld_GetSubSectorVertices(void)
{
  int      i, j;
  int      numedgepoints;
  subsector_t* ssector;

  for(i = 0; i < numsubsectors; i++)
  {
    ssector = &subsectors[i];

    if ((ssector->sector->flags & SECTOR_IS_CLOSED) && !triangulate_subsectors)
      continue;

    numedgepoints  = ssector->numlines;

    gld_AddGlobalVertexes(numedgepoints);

    if (flats_vbo)
    {
      int currentsector = ssector->sector->iSectorID;
      GLLoopDef **loop;
      int *loopcount;

      if (triangulate_subsectors)
      {
        loop = &subsectorloops[ i ].loops;
        loopcount = &subsectorloops[ i ].loopcount;
      }
      else
      {
        loop = &sectorloops[ currentsector ].loops;
        loopcount = &sectorloops[ currentsector ].loopcount;
      }

      (*loopcount)++;
      (*loop) = Z_Realloc((*loop), sizeof(GLLoopDef)*(*loopcount), PU_STATIC, 0);
      ((*loop)[(*loopcount) - 1]).index = i;
      ((*loop)[(*loopcount) - 1]).mode = GL_TRIANGLE_FAN;
      ((*loop)[(*loopcount) - 1]).vertexcount = numedgepoints;
      ((*loop)[(*loopcount) - 1]).vertexindex = gld_num_vertexes;

      for(j = 0;  j < numedgepoints; j++)
      {
        flats_vbo[gld_num_vertexes].u =( (float)(segs[ssector->firstline + j].v1->x)/FRACUNIT)/64.0f;
        flats_vbo[gld_num_vertexes].v =(-(float)(segs[ssector->firstline + j].v1->y)/FRACUNIT)/64.0f;
        flats_vbo[gld_num_vertexes].x = -(float)(segs[ssector->firstline + j].v1->x)/MAP_SCALE;
        flats_vbo[gld_num_vertexes].y = 0.0f;
        flats_vbo[gld_num_vertexes].z =  (float)(segs[ssector->firstline + j].v1->y)/MAP_SCALE;
        gld_num_vertexes++;
      }
    }
  }
}

// gld_PreprocessLevel
//
// this checks all sectors if they are closed and calls gld_PrecalculateSector to
// calculate the loops for every sector
// the idea to check for closed sectors is from DEU. check next commentary
/*
      Note from RQ:
      This is a very simple idea, but it works!  The first test (above)
      checks that all Sectors are closed.  But if a closed set of LineDefs
      is moved out of a Sector and has all its "external" SideDefs pointing
      to that Sector instead of the new one, then we need a second test.
      That's why I check if the SideDefs facing each other are bound to
      the same Sector.

      Other note from RQ:
      Nowadays, what makes the power of a good editor is its automatic tests.
      So, if you are writing another Doom editor, you will probably want
      to do the same kind of tests in your program.  Fine, but if you use
      these ideas, don't forget to credit DEU...  Just a reminder... :-)
*/
// so I credited DEU

//
// e6y
// All sectors which are inside 64x64 square of map grid should be marked here.
// GL_CLAMP instead of GL_REPEAT should be used for them for avoiding seams
//

static void gld_MarkSectorsForClamp(void)
{
  int i;

  for (i = 0; i < numsectors; i++)
  {
    int loopnum; // current loop number
    GLLoopDef *currentloop; // the current loop
    GLfloat minu, maxu, minv, maxv;
    dboolean fail;

    minu = minv = 65535;
    maxu = maxv = -65535;
    fail = false;

    for (loopnum = 0; !fail && loopnum < sectorloops[i].loopcount; loopnum++)
    {
      int vertexnum;
      // set the current loop
      currentloop = &sectorloops[i].loops[loopnum];
      if (!currentloop)
        continue;
      for (vertexnum = currentloop->vertexindex; !fail && vertexnum<(currentloop->vertexindex+currentloop->vertexcount); vertexnum++)
      {
        vbo_xyz_uv_t *vbo;
        vbo = &flats_vbo[vertexnum];

        if (vbo->u < minu) minu = (float)floor(vbo->u);
        if (vbo->v < minv) minv = (float)floor(vbo->v);
        if (vbo->u > maxu) maxu = vbo->u;
        if (vbo->v > maxv) maxv = vbo->v;
        
        fail = (maxu - minu > 1.0f || maxv - minv > 1.0f);
      }
    }

    if (!fail)
    {
      sectorloops[i].flags = SECTOR_CLAMPXY;

      for (loopnum=0; loopnum<sectorloops[i].loopcount; loopnum++)
      {
        int vertexnum;
        // set the current loop
        currentloop = &sectorloops[i].loops[loopnum];
        if (!currentloop)
          continue;
        for (vertexnum = currentloop->vertexindex; vertexnum < (currentloop->vertexindex+currentloop->vertexcount); vertexnum++)
        {
          flats_vbo[vertexnum].u = flats_vbo[vertexnum].u - minu;
          flats_vbo[vertexnum].v = flats_vbo[vertexnum].v - minv;
        }
      }
    }
  }
}

static void gld_PreprocessSectors(void)
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
  levelinfo=fopen("levelinfo.txt","a");
  if (levelinfo)
  {
    if (gamemode==commercial)
      fprintf(levelinfo,"MAP%02i\n",gamemap);
    else
      fprintf(levelinfo,"E%iM%i\n",gameepisode,gamemap);
  }
#endif

  if (numsectors)
  {
    sectorloops=Z_Malloc(sizeof(GLSector)*numsectors,PU_STATIC,0);
    if (!sectorloops)
      I_Error("gld_PreprocessSectors: Not enough memory for array sectorloops");
    memset(sectorloops, 0, sizeof(GLSector)*numsectors);
  }

  if (numsubsectors)
  {
    subsectorloops=Z_Malloc(sizeof(GLMapSubsector)*numsubsectors,PU_STATIC,0);
    if (!subsectorloops)
      I_Error("gld_PreprocessSectors: Not enough memory for array subsectorloops");
    memset(subsectorloops, 0, sizeof(GLMapSubsector)*numsubsectors);
  }

  if (numsegs)
  {
    segrendered=calloc(numsegs, sizeof(byte));
    if (!segrendered)
      I_Error("gld_PreprocessSectors: Not enough memory for array segrendered");
  }

  if (numlines)
  {
    linerendered[0]=calloc(numlines, sizeof(byte));
    linerendered[1]=calloc(numlines, sizeof(byte));
    if (!linerendered[0] || !linerendered[1])
      I_Error("gld_PreprocessSectors: Not enough memory for array linerendered");
  }

  flats_vbo = NULL;
  gld_max_vertexes=0;
  gld_num_vertexes=0;
  if (numvertexes)
  {
    gld_AddGlobalVertexes(numvertexes*2);
  }

#ifdef USE_GLU_TESS
  if (numvertexes)
  {
    vertexcheck=malloc(numvertexes*sizeof(vertexcheck[0]));
    vertexcheck2=malloc(numvertexes*sizeof(vertexcheck2[0]));
    if (!vertexcheck || !vertexcheck2)
    {
      if (levelinfo) fclose(levelinfo);
      I_Error("gld_PreprocessSectors: Not enough memory for array vertexcheck");
      return;
    }
  }

  for (i=0; i<numsectors; i++)
  {
    memset(vertexcheck,0,numvertexes*sizeof(vertexcheck[0]));
    memset(vertexcheck2,0,numvertexes*sizeof(vertexcheck2[0]));
    for (j=0; j<sectors[i].linecount; j++)
    {
      v1num=((intptr_t)sectors[i].lines[j]->v1-(intptr_t)vertexes)/sizeof(vertex_t);
      v2num=((intptr_t)sectors[i].lines[j]->v2-(intptr_t)vertexes)/sizeof(vertex_t);
      if ((v1num>=numvertexes) || (v2num>=numvertexes))
        continue;

      // e6y: for correct handling of missing textures.
      // We do not need to apply some algos for isolated lines.
      vertexcheck2[v1num]++;
      vertexcheck2[v2num]++;

      if (sectors[i].lines[j]->sidenum[0]!=NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[0]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=1;
          vertexcheck[v2num]|=2;
        }
      if (sectors[i].lines[j]->sidenum[1]!=NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[1]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=2;
          vertexcheck[v2num]|=1;
        }
    }
    if (sectors[i].linecount<3)
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
      for (j=0; j<numvertexes; j++)
      {
        if ((vertexcheck[j]==1) || (vertexcheck[j]==2))
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
    for (j=0; j<sectors[i].linecount; j++)
    {
      v1num=((intptr_t)sectors[i].lines[j]->v1-(intptr_t)vertexes)/sizeof(vertex_t);
      v2num=((intptr_t)sectors[i].lines[j]->v2-(intptr_t)vertexes)/sizeof(vertex_t);
      if (vertexcheck2[v1num] < 2 && vertexcheck2[v2num] < 2)
      {
        sectors[i].lines[j]->r_flags |= RF_ISOLATED;
      }
    }
    
    // figgi -- adapted for glnodes
    if (sectors[i].flags & SECTOR_IS_CLOSED)
      gld_PrecalculateSector(i);
  }
  free(vertexcheck);
  free(vertexcheck2);
#endif /* USE_GLU_TESS */

  // figgi -- adapted for glnodes
  if (numnodes)
  {
    if (nodesVersion == 0)
      gld_CarveFlats(numnodes-1, 0, 0);
    else
      gld_GetSubSectorVertices();
  }

  gld_ProcessTexturedMap();

  if (levelinfo) fclose(levelinfo);

  //e6y: for seamless rendering
  gld_MarkSectorsForClamp();
}

static void gld_PreprocessSegs(void)
{
  int i;

  gl_segs=Z_Malloc(numsegs*sizeof(GLSeg),PU_STATIC,0);
  for (i=0; i<numsegs; i++)
  {
    gl_segs[i].x1=-(float)segs[i].v1->x/(float)MAP_SCALE;
    gl_segs[i].z1= (float)segs[i].v1->y/(float)MAP_SCALE;
    gl_segs[i].x2=-(float)segs[i].v2->x/(float)MAP_SCALE;
    gl_segs[i].z2= (float)segs[i].v2->y/(float)MAP_SCALE;
  }

  gl_lines=Z_Malloc(numlines*sizeof(GLSeg),PU_STATIC,0);
  for (i=0; i<numlines; i++)
  {
    gl_lines[i].x1=-(float)lines[i].v1->x/(float)MAP_SCALE;
    gl_lines[i].z1= (float)lines[i].v1->y/(float)MAP_SCALE;
    gl_lines[i].x2=-(float)lines[i].v2->x/(float)MAP_SCALE;
    gl_lines[i].z2= (float)lines[i].v2->y/(float)MAP_SCALE;
  }
}

void gld_PreprocessLevel(void)
{
  // e6y: speedup of level reloading
  // Do not preprocess GL data twice for same level
  if (!gl_preprocessed)
  {
    int i;
    static int numsectors_prev = 0;
    static int numsubsectors_prev = 0;

    free(gl_segs);
    free(gl_lines);

    free(flats_vbo);
    flats_vbo = NULL;

    free(segrendered);
    free(linerendered[0]);
    free(linerendered[1]);
    
    for (i = 0; i < numsectors_prev; i++)
    {
      free(sectorloops[i].loops);
    }
    free(sectorloops);
    for (i = 0; i < numsubsectors_prev; i++)
    {
      free(subsectorloops[i].loops);
    }
    free(subsectorloops);

    gld_Precache();
    gld_PreprocessSectors();
    gld_PreprocessFakeSectors();
    gld_PreprocessSegs();

    numsectors_prev = numsectors;
    numsubsectors_prev = numsubsectors;
  }
  else
  {
    gld_PreprocessFakeSectors();

    memset(segrendered, 0, numsegs*sizeof(segrendered[0]));
    memset(linerendered[0], 0, numlines*sizeof(linerendered[0][0]));
    memset(linerendered[1], 0, numlines*sizeof(linerendered[1][0]));
  }

  gld_ResetTexturedAutomap();

  gld_FreeDrawInfo();

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (!gl_preprocessed)
  {
    if (gl_ext_arb_vertex_buffer_object)
    {
      if (flats_vbo_id)
      {
        // delete VBO when already exists
        GLEXT_glDeleteBuffersARB(1, &flats_vbo_id);
      }
      // generate a new VBO and get the associated ID
      GLEXT_glGenBuffersARB(1, &flats_vbo_id);
      // bind VBO in order to use
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, flats_vbo_id);
      // upload data to VBO
      GLEXT_glBufferDataARB(GL_ARRAY_BUFFER,
        gld_num_vertexes * sizeof(flats_vbo[0]),
        flats_vbo, GL_STATIC_DRAW_ARB);

      free(flats_vbo);
      flats_vbo = NULL;

      // bind VBO in order to use
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, flats_vbo_id);
    }

    glVertexPointer(3, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_u);
  }
#endif

  //e6y
  gld_PreprocessDetail();
  gld_InitVertexData();

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (!gl_preprocessed)
  {
    if (gl_use_display_lists)
    {
      gld_InitDisplayLists();
    }
  }
#endif

  gl_preprocessed = true;
}

/*****************************
 *
 * AUTOMAP
 *
 *****************************/

void gld_ProcessTexturedMap(void)
{
  if (map_textured && subsectorloops && subsectorloops[0].loops == NULL)
  {
    triangulate_subsectors = 1;
    if (nodesVersion == 0)
      gld_CarveFlats(numnodes-1, 0, 0);
    else
      gld_GetSubSectorVertices();
    triangulate_subsectors = 0;
  }
}
