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

#include "config.h"
#include "z_zone.h"
#include "gl_intern.h"
#include "gl_struct.h"

int gld_max_vertexes=0;
int gld_num_vertexes=0;
GLVertex *gld_vertexes=NULL;
GLTexcoord *gld_texcoords=NULL;

// this is the list for all sectors to the loops
GLSector *sectorloops=NULL;

void gld_SetVertexArrays(void)
{
  if (gld_texcoords)
    p_glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
  if (gld_vertexes)
    p_glVertexPointer(3,GL_FLOAT,0,gld_vertexes);
}

static void gld_AddGlobalVertexes(int count)
{
  if ((gld_num_vertexes+count)>=gld_max_vertexes)
  {
    gld_max_vertexes+=count+1024;
    gld_vertexes=realloc(gld_vertexes,gld_max_vertexes*sizeof(GLVertex));
    gld_texcoords=realloc(gld_texcoords,gld_max_vertexes*sizeof(GLTexcoord));
    gld_SetVertexArrays();
  }
}

static FILE *levelinfo;

/*****************************
 *
 * FLATS
 *
 *****************************/

/* proff - 05/15/2000
 * The idea and algorithm to compute the flats with nodes and subsectors is
 * originaly from JHexen. I have redone it.
 */

#define FIX2DBL(x)		((double)(x))
#define MAX_CC_SIDES	64

static boolean gld_PointOnSide(vertex_t *p, divline_t *d)
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
	unsigned char	sidelist[MAX_CC_SIDES];
	int				i, k, num = *numpoints;

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
			if(endIdx == num) endIdx = 0;	// Wrap-around.
			// Clipping will happen when the ends are on different sides.
			if(sidelist[startIdx] != sidelist[endIdx])
			{
				vertex_t newvert;

        gld_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

				// Add the new vertex. Also modify the sidelist.
        num++;
				points = (vertex_t*)realloc(points, num*sizeof(vertex_t));
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
	
  clippers=(divline_t*)malloc(numclippers*sizeof(divline_t));
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
	edgepoints = (vertex_t*)malloc(numedgepoints*sizeof(vertex_t));
	
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
		if (levelinfo) fprintf(levelinfo, "All carved away: subsector %i - sector %i\n", ssec-subsectors, ssec->sector->iSectorID);
	}
	else
	{
	  if(numedgepoints >= 3)
    {
      gld_AddGlobalVertexes(numedgepoints);
      if ((gld_vertexes) && (gld_texcoords))
      {
        int currentsector=ssec->sector->iSectorID;

        sectorloops[ currentsector ].loopcount++;
        sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_LEVEL, 0);
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=GL_TRIANGLE_FAN;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=numedgepoints;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexindex=gld_num_vertexes;

	      for(i = 0;  i < numedgepoints; i++)
	      {
		      gld_texcoords[gld_num_vertexes].u = ( (float)edgepoints[i].x/(float)FRACUNIT)/64.0f;
		      gld_texcoords[gld_num_vertexes].v = (-(float)edgepoints[i].y/(float)FRACUNIT)/64.0f;
          gld_vertexes[gld_num_vertexes].x = -(float)edgepoints[i].x/MAP_SCALE;
          gld_vertexes[gld_num_vertexes].y = 0.0f;
          gld_vertexes[gld_num_vertexes].z =  (float)edgepoints[i].y/MAP_SCALE;
          gld_num_vertexes++;
	      }
      }
    }
	}
	// We're done, free the edgepoints memory.
	free(edgepoints);
  free(clippers);
}

static void gld_CarveFlats(int bspnode, int numdivlines, divline_t *divlines, boolean *sectorclosed)
{
	node_t		*nod;
	divline_t	*childlist, *dl;
	int			childlistsize = numdivlines+1;
	
	// If this is a subsector we are dealing with, begin carving with the
	// given list.
	if(bspnode & NF_SUBSECTOR)
	{
		// We have arrived at a subsector. The divline list contains all
		// the partition lines that carve out the subsector.
		int ssidx = bspnode & (~NF_SUBSECTOR);
    if (!sectorclosed[subsectors[ssidx].sector->iSectorID])
  		gld_FlatConvexCarver(ssidx, numdivlines, divlines);
		return;
	}

	// Get a pointer to the node.
	nod = nodes + bspnode;

	// Allocate a new list for each child.
	childlist = (divline_t*)malloc(childlistsize*sizeof(divline_t));

	// Copy the previous lines.
	if(divlines) memcpy(childlist,divlines,numdivlines*sizeof(divline_t));

	dl = childlist + numdivlines;
	dl->x = nod->x;
	dl->y = nod->y;
	// The right child gets the original line (LEFT side clipped).
	dl->dx = nod->dx;
	dl->dy = nod->dy;
	gld_CarveFlats(nod->children[0],childlistsize,childlist,sectorclosed);

	// The left side. We must reverse the line, otherwise the wrong
	// side would get clipped.
	dl->dx = -nod->dx;
	dl->dy = -nod->dy;
	gld_CarveFlats(nod->children[1],childlistsize,childlist,sectorclosed);

	// We are finishing with this node, free the allocated list.
	free(childlist);
}

/********************************************
 * Name     : gld_GetSubSectorVertices	    *
 * created  : 08/13/00					            *
 * modified : 09/18/00, adapted for PrBoom  *
 * author   : figgi						              *
 * what		  : prepares subsectorvertices    *
 *            (glnodes only)			          *
 ********************************************/

void gld_GetSubSectorVertices(boolean *sectorclosed)
{
	int			 i, j;
  int			 numedgepoints;
	subsector_t* ssector;
		
	for(i = 0; i < numsubsectors; i++)
	{
		ssector = &subsectors[i];

    if (sectorclosed[ssector->sector->iSectorID])
      continue;

		numedgepoints  = ssector->numlines;

    gld_AddGlobalVertexes(numedgepoints);

    if ((gld_vertexes) && (gld_texcoords))
    {
			int currentsector = ssector->sector->iSectorID;

			sectorloops[currentsector].loopcount++;
			sectorloops[currentsector].loops = Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_LEVEL, 0);
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].mode		 = GL_TRIANGLE_FAN;
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].vertexcount = numedgepoints;
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].vertexindex = gld_num_vertexes;
      for(j = 0;  j < numedgepoints; j++)
			{
        gld_texcoords[gld_num_vertexes].u =( (float)(segs[ssector->firstline + j].v1->x)/FRACUNIT)/64.0f;
        gld_texcoords[gld_num_vertexes].v =(-(float)(segs[ssector->firstline + j].v1->y)/FRACUNIT)/64.0f;
        gld_vertexes[gld_num_vertexes].x = -(float)(segs[ssector->firstline + j].v1->x)/MAP_SCALE;
        gld_vertexes[gld_num_vertexes].y = 0.0f;
        gld_vertexes[gld_num_vertexes].z =  (float)(segs[ssector->firstline + j].v1->y)/MAP_SCALE;
        gld_num_vertexes++;
			}
	  }
  }
}

static void gld_PrepareSectorSpecialEffects(int num)
{
  int i;

  // the following is for specialeffects. see r_bsp.c in R_Subsector
  sectors[num].no_toptextures=true;
  sectors[num].no_bottomtextures=true;
  for (i=0; i<sectors[num].linecount; i++)
  {
    if ( (sectors[num].lines[i]->sidenum[0]>=0) &&
         (sectors[num].lines[i]->sidenum[1]>=0) )
    {
      if (sides[sectors[num].lines[i]->sidenum[0]].toptexture!=R_TextureNumForName("-"))
        sectors[num].no_toptextures=false;
      if (sides[sectors[num].lines[i]->sidenum[0]].bottomtexture!=R_TextureNumForName("-"))
        sectors[num].no_bottomtextures=false;
      if (sides[sectors[num].lines[i]->sidenum[1]].toptexture!=R_TextureNumForName("-"))
        sectors[num].no_toptextures=false;
      if (sides[sectors[num].lines[i]->sidenum[1]].bottomtexture!=R_TextureNumForName("-"))
        sectors[num].no_bottomtextures=false;
    }
    else
    {
      sectors[num].no_toptextures=false;
      sectors[num].no_bottomtextures=false;
    }
  }
#ifdef _DEBUG
  if (sectors[num].no_toptextures)
    lprintf(LO_INFO,"Sector %i has no toptextures\n",num);
  if (sectors[num].no_bottomtextures)
    lprintf(LO_INFO,"Sector %i has no bottomtextures\n",num);
#endif
}

void gld_PreprocessSectors(void)
{
  boolean *sectorclosed;
  int i;

#ifdef _DEBUG
  levelinfo=fopen("levelinfo.txt","a");
  if (levelinfo)
  {
    if (gamemode==commercial)
      fprintf(levelinfo,"MAP%02i\n",gamemap);
    else
      fprintf(levelinfo,"E%iM%i\n",gameepisode,gamemap);
  }
#endif

  sectorclosed=malloc(numsectors*sizeof(boolean));
  if (!sectorclosed)
    I_Error("gld_PreprocessSectors: Not enough memory for array sectorclosed");
  memset(sectorclosed, 0, sizeof(boolean)*numsectors);

  sectorloops=realloc(sectorloops, sizeof(GLSector)*numsectors);
  if (!sectorloops)
    I_Error("gld_PreprocessSectors: Not enough memory for array sectorloops");
  memset(sectorloops, 0, sizeof(GLSector)*numsectors);

  sectorrendered=realloc(sectorrendered,numsectors*sizeof(byte));
  if (!sectorrendered)
    I_Error("gld_PreprocessSectors: Not enough memory for array sectorrendered");
  memset(sectorrendered, 0, numsectors*sizeof(byte));

  segrendered=realloc(segrendered,numsegs*sizeof(byte));
  if (!segrendered)
    I_Error("gld_PreprocessSectors: Not enough memory for array segrendered");
  memset(segrendered, 0, numsegs*sizeof(byte));

  gld_max_vertexes=0;
  gld_num_vertexes=0;
  gld_AddGlobalVertexes(numvertexes*2);

  for (i=0; i<numsectors; i++)
    gld_PrepareSectorSpecialEffects(i);

  // figgi -- adapted for glnodes
  if (usingGLNodes == false)
	  gld_CarveFlats(numnodes-1, 0, 0, sectorclosed);
  else
	  gld_GetSubSectorVertices(sectorclosed);

  if (levelinfo) fclose(levelinfo);
  free(sectorclosed);
}

static void gld_PreprocessSegs(void)
{
  int i;

  gl_segs=realloc(gl_segs, numsegs*sizeof(GLSeg));
  for (i=0; i<numsegs; i++)
  {
    gl_segs[i].x1=-(float)segs[i].v1->x/(float)MAP_SCALE;
    gl_segs[i].z1= (float)segs[i].v1->y/(float)MAP_SCALE;
    gl_segs[i].x2=-(float)segs[i].v2->x/(float)MAP_SCALE;
    gl_segs[i].z2= (float)segs[i].v2->y/(float)MAP_SCALE;
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

extern void gld_Precache(void);

void gld_PreprocessLevel(void)
{
  if (gamestate != GS_LEVEL)
    return;

  if (precache)
    gld_Precache();
  gld_PreprocessSectors();
  gld_PreprocessSegs();
  memset(&gld_drawinfo,0,sizeof(GLDrawInfo));
}
