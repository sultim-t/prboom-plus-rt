/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_setup.c,v 1.3 2000/05/07 20:19:34 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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

static const char
rcsid[] = "$Id: p_setup.c,v 1.3 2000/05/07 20:19:34 proff_fs Exp $";

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
#include "lprintf.h" //jff 10/6/98 for debug outputs
#ifdef GL_DOOM
#include "gl_struct.h"
#endif

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
long      *blockmap;              // was short -- killough

// offsets in blockmap are from here
long      *blockmaplump;          // was short -- killough

fixed_t   bmaporgx, bmaporgy;     // origin of block map

mobj_t    **blocklinks;           // for thing chains

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

//
// P_LoadVertexes
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadVertexes (int lump)
{
  const byte *data; // cph - const
  int i;

  // Determine number of lumps:
  //  total lump length / vertex record length.
  numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

  // Allocate zone memory for buffer.
  vertexes = Z_Malloc(numvertexes*sizeof(vertex_t),PU_LEVEL,0);

  // Load data into cache. 
  data = W_CacheLumpNum(lump); // cph - wad handling updated

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i=0; i<numvertexes; i++)
    {
      vertexes[i].x = SHORT(((mapvertex_t *) data)[i].x)<<FRACBITS;
      vertexes[i].y = SHORT(((mapvertex_t *) data)[i].y)<<FRACBITS;
    }

  // Free buffer memory.
  W_UnlockLumpNum(lump);
}


//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSegs (int lump)
{
  int  i;
  const byte *data; // cph - const

  numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
  segs = Z_Calloc(numsegs,sizeof(seg_t),PU_LEVEL,0);
  data = W_CacheLumpNum(lump); // cph - wad lump handling updated

  for (i=0; i<numsegs; i++)
    {
      seg_t *li = segs+i;
      mapseg_t *ml = (mapseg_t *) data + i;

      int side, linedef;
      line_t *ldef;

      li->v1 = &vertexes[SHORT(ml->v1)];
      li->v2 = &vertexes[SHORT(ml->v2)];

      li->angle = (SHORT(ml->angle))<<16;
      li->offset = (SHORT(ml->offset))<<16;
      linedef = SHORT(ml->linedef);
      ldef = &lines[linedef];
      li->linedef = ldef;
      side = SHORT(ml->side);
      li->sidedef = &sides[ldef->sidenum[side]];
      li->frontsector = sides[ldef->sidenum[side]].sector;

      // killough 5/3/98: ignore 2s flag if second sidedef missing:
      if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side^1]!=-1)
        li->backsector = sides[ldef->sidenum[side^1]].sector;
      else
        li->backsector = 0;
    }

  W_UnlockLumpNum(lump); // cph - release the data
}


//
// P_LoadSubsectors
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSubsectors (int lump)
{
  const byte *data; // cph - const*
  int  i;

  numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
  subsectors = Z_Calloc(numsubsectors,sizeof(subsector_t),PU_LEVEL,0);
  data = W_CacheLumpNum(lump); // cph - wad lump handling updated

  for (i=0; i<numsubsectors; i++)
    {
      subsectors[i].numlines  = SHORT(((mapsubsector_t *) data)[i].numsegs );
      subsectors[i].firstline = SHORT(((mapsubsector_t *) data)[i].firstseg);
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
  sectors = Z_Calloc (numsectors,sizeof(sector_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  for (i=0; i<numsectors; i++)
    {
      sector_t *ss = sectors + i;
      const mapsector_t *ms = (mapsector_t *) data + i;

  		ss->iSectorID=i; // proff 04/05/2000: needed for OpenGL and used in debugmode by the HUD to draw sectornum
      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
      ss->floorpic = R_FlatNumForName(ms->floorpic);
      ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
      ss->lightlevel = SHORT(ms->lightlevel);
      ss->special = SHORT(ms->special);
      ss->oldspecial = SHORT(ms->special);
      ss->tag = SHORT(ms->tag);
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
  nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  for (i=0; i<numnodes; i++)
    {
      node_t *no = nodes + i;
      mapnode_t *mn = (mapnode_t *) data + i;
      int j;

      no->x = SHORT(mn->x)<<FRACBITS;
      no->y = SHORT(mn->y)<<FRACBITS;
      no->dx = SHORT(mn->dx)<<FRACBITS;
      no->dy = SHORT(mn->dy)<<FRACBITS;

      for (j=0 ; j<2 ; j++)
        {
          int k;
          no->children[j] = SHORT(mn->children[j]);
          for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }

  W_UnlockLumpNum(lump); // cph - release the data
}


//
// P_LoadThings
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadThings (int lump)
{
  int  i, numthings = W_LumpLength (lump) / sizeof(mapthing_t);
  const byte *data = W_CacheLumpNum (lump); // cph - wad lump handling updated, const*

  for (i=0; i<numthings; i++)
    {
      mapthing_t *mt = (mapthing_t *) data + i;

      // Do not spawn cool, new monsters if !commercial
      if (gamemode != commercial)
        switch(mt->type)
          {
          case 68:  // Arachnotron
          case 64:  // Archvile
          case 88:  // Boss Brain
          case 89:  // Boss Shooter
          case 69:  // Hell Knight
          case 67:  // Mancubus
          case 71:  // Pain Elemental
          case 65:  // Former Human Commando
          case 66:  // Revenant
          case 84:  // Wolf SS
            continue;
          }

      // Do spawn all other stuff.
      mt->x = SHORT(mt->x);
      mt->y = SHORT(mt->y);
      mt->angle = SHORT(mt->angle);
      mt->type = SHORT(mt->type);
      mt->options = SHORT(mt->options);

      P_SpawnMapThing (mt);
    }

  W_UnlockLumpNum(lump); // cph - release the data
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
  lines = Z_Calloc (numlines,sizeof(line_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump); // cph - wad lump handling updated

  for (i=0; i<numlines; i++)
    {
      maplinedef_t *mld = (maplinedef_t *) data + i;
      line_t *ld = lines+i;
      vertex_t *v1, *v2;

      ld->flags = SHORT(mld->flags);
      ld->special = SHORT(mld->special);
      ld->tag = SHORT(mld->tag);
      v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
      v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;

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

  		ld->iLineID=i; // proff 04/05/2000: needed for OpenGL
      ld->sidenum[0] = SHORT(mld->sidenum[0]);
      ld->sidenum[1] = SHORT(mld->sidenum[1]);

      // killough 4/4/98: support special sidedef interpretation below
      if (ld->sidenum[0] != -1 && ld->special)
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
      // CPhipps - compatibility selected
      if (compatibility_level >= lxdoom_1_compatibility) {
	// killough 11/98: fix common wad errors (missing sidedefs):
	
	if (ld->sidenum[0] == -1) {
	  ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side
	  // cph - print a warning about the bug
	  lprintf(LO_WARN, "P_LoadSegs: linedef %d missing first sidedef\n",numlines-i);
	}
	
	if ((ld->sidenum[1] == -1) && (ld->flags & ML_TWOSIDED)) {
	  ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
	  // cph - print a warning about the bug
	  lprintf(LO_WARN, "P_LoadSegs: linedef %d has two-sided flag set, but no second sidedef\n",numlines-i);
	}
      }

      ld->frontsector = ld->sidenum[0]!=-1 ? sides[ld->sidenum[0]].sector : 0;
      ld->backsector  = ld->sidenum[1]!=-1 ? sides[ld->sidenum[1]].sector : 0;
      switch (ld->special)
        {                       // killough 4/11/98: handle special types
          int lump, j;

        case 260:               // killough 4/11/98: translucent 2s textures
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
  sides = Z_Calloc(numsides,sizeof(side_t),PU_LEVEL,0);
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
      register mapsidedef_t *msd = (mapsidedef_t *) data + i;
      register side_t *sd = sides + i;
      register sector_t *sec;

      sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

      // killough 4/4/98: allow sidedef texture names to be overloaded
      // killough 4/11/98: refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.

      sd->sector = sec = &sectors[SHORT(msd->sector)];
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

        default:                        // normal cases
          sd->midtexture = R_TextureNumForName(msd->midtexture);
          sd->toptexture = R_TextureNumForName(msd->toptexture);
          sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
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

void P_CreateBlockMap()
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

  blockmaplump = Z_Malloc(sizeof(*blockmaplump) * (4+NBlocks+linetotal),
                          PU_LEVEL, 0);
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

  if (M_CheckParm("-blockmap") || (count = W_LumpLength(lump)/2) >= 0x10000)
    P_CreateBlockMap();
  else
    {
      long i;
      // cph - const*, wad lump handling updated
      const short *wadblockmaplump = W_CacheLumpNum(lump);
      blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

      // killough 3/1/98: Expand wad blockmap into larger internal one,
      // by treating all offsets except -1 as unsigned and zero-extending
      // them. This potentially doubles the size of blockmaps allowed,
      // because Doom originally considered the offsets as always signed.

      blockmaplump[0] = SHORT(wadblockmaplump[0]);
      blockmaplump[1] = SHORT(wadblockmaplump[1]);
      blockmaplump[2] = (long)(SHORT(wadblockmaplump[2])) & 0xffff;
      blockmaplump[3] = (long)(SHORT(wadblockmaplump[3])) & 0xffff;

      for (i=4 ; i<count ; i++)
        {
          short t = SHORT(wadblockmaplump[i]);          // killough 3/1/98
          blockmaplump[i] = t == -1 ? -1l : (long) t & 0xffff;
        }

      W_UnlockLumpNum(lump); // cph - unlock the lump

      bmaporgx = blockmaplump[0]<<FRACBITS;
      bmaporgy = blockmaplump[1]<<FRACBITS;
      bmapwidth = blockmaplump[2];
      bmapheight = blockmaplump[3];
    }

  // clear out mobj chains - CPhipps - use calloc
  blocklinks = Z_Calloc (bmapwidth*bmapheight,sizeof(*blocklinks),PU_LEVEL,0);
  blockmap = blockmaplump+4;
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
// killough 5/3/98: reformatted, cleaned up
// cph 18/8/99: rewritten to avoid O(numlines * numsectors) section
// It makes things more complicated, but saves seconds on big levels

// cph - convenient sub-function
static void P_AddLineToSector(line_t* li, sector_t* sector)
{
  fixed_t *bbox = (void*)sector->blockbox;
  
  sector->lines[sector->linecount++] = li;
  M_AddToBox (bbox, li->v1->x, li->v1->y);
  M_AddToBox (bbox, li->v2->x, li->v2->y);
}

void P_GroupLines (void)
{
  register line_t *li;
  register sector_t* sector;
  int i, total = numlines;

  // look up sector number for each subsector
  for (i=0; i<numsubsectors; i++)
    subsectors[i].sector = segs[subsectors[i].firstline].sidedef->sector;

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
    line_t **linebuffer = Z_Malloc(total*4, PU_LEVEL, 0);

    for (i=0, sector = sectors; i<numsectors; i++, sector++) {
      sector->lines = linebuffer;
      linebuffer += sector->linecount;
      sector->linecount = 0;
      M_ClearBox(sector->blockbox);
    }
  }

  // Enter those lines
  for (i=0,li=lines; i<numlines; i++, li++) {
    P_AddLineToSector(li, li->frontsector);
    if (li->backsector && li->backsector != li->frontsector)
      P_AddLineToSector(li, li->backsector);
  }
  
  for (i=0, sector = sectors; i<numsectors; i++, sector++) {
    fixed_t *bbox = (void*)sector->blockbox; // cph - For convenience, so
                                  // I can sue the old code unchanged
    int block;
    
    // set the degenmobj_t to the middle of the bounding box
    sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
    sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
    
    // adjust bounding box to map blocks
    block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block >= bmapheight ? bmapheight-1 : block;
    sector->blockbox[BOXTOP]=block;
    
    block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXBOTTOM]=block;
    
    block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block >= bmapwidth ? bmapwidth-1 : block;
    sector->blockbox[BOXRIGHT]=block;
    
    block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXLEFT]=block;
  }
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

void P_RemoveSlimeTrails(void)                // killough 10/98
{
  byte *hit = calloc(1, numvertexes);         // Hitlist for vertices
  int i;
  for (i=0; i<numsegs; i++)                   // Go through each seg
    {
      const line_t *l = segs[i].linedef;      // The parent linedef
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
		    v->x = (int)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
		    v->y = (int)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
		  }
	      }  // Obsfucated C contest entry:   :)
	  while ((v != segs[i].v2) && (v = segs[i].v2));
	}
    }
  free(hit);
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

  totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
  wminfo.partime = 180;
  for (i=0; i<MAXPLAYERS; i++)
    players[i].killcount = players[i].secretcount = players[i].itemcount = 0;

  // Initial height of PointOfView will be set by player think.
  players[consoleplayer].viewz = 1;

  // Make sure all sounds are stopped before Z_FreeTags.
  S_Start();

  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);
  if (rejectlump != -1) { // cph - unlock the reject table
    W_UnlockLumpNum(rejectlump);
    rejectlump = -1;
  }

#ifdef GL_DOOM
// proff 11/99: clean the memory from textures etc.
  gld_CleanMemory();
#endif	

  P_InitThinkers();

  // if working with a devlopment map, reload it
  //    W_Reload ();     killough 1/31/98: W_Reload obsolete

  // find map name
  if (gamemode == commercial)
    sprintf(lumpname, "map%02d", map);           // killough 1/24/98: simplify
  else
    sprintf(lumpname, "E%dM%d", episode, map);   // killough 1/24/98: simplify

  lumpnum = W_GetNumForName(lumpname);

  leveltime = 0;

  // note: most of this ordering is important

  // killough 3/1/98: P_LoadBlockMap call moved down to below
  // killough 4/4/98: split load of sidedefs into two parts,
  // to allow texture names to be used in special linedefs

  P_LoadVertexes  (lumpnum+ML_VERTEXES);
  P_LoadSectors   (lumpnum+ML_SECTORS);
  P_LoadSideDefs  (lumpnum+ML_SIDEDEFS);             // killough 4/4/98
  P_LoadLineDefs  (lumpnum+ML_LINEDEFS);             //       |
  P_LoadSideDefs2 (lumpnum+ML_SIDEDEFS);             //       |
  P_LoadLineDefs2 (lumpnum+ML_LINEDEFS);             // killough 4/4/98
  P_LoadBlockMap  (lumpnum+ML_BLOCKMAP);             // killough 3/1/98
  P_LoadSubsectors(lumpnum+ML_SSECTORS);
  P_LoadNodes     (lumpnum+ML_NODES);
  P_LoadSegs      (lumpnum+ML_SEGS);

  if (rejectlump != -1)
    W_UnlockLumpNum(rejectlump);
  rejectmatrix = W_CacheLumpNum(rejectlump = lumpnum+ML_REJECT);
  P_GroupLines();

  P_RemoveSlimeTrails();    // killough 10/98: remove slime trails from wad      
                                                                                    
  bodyqueslot = 0;

// phares 8/10/98: Clear body queue so the corpses from previous games are
// not assumed to be from this one. The mobj_t's belonging to these corpses
// are cleared in the normal freeing of zoned memory between maps, so all
// we have to do here is clear the pointers to them.

  if (bodyque)
    memset(bodyque, 0, bodyquesize * sizeof (*bodyque)); // CPhipps - use memset

  deathmatch_p = deathmatchstarts;
  P_LoadThings(lumpnum+ML_THINGS);

  // if deathmatch, randomly spawn the active players
  if (deathmatch)
    for (i=0; i<MAXPLAYERS; i++)
      if (playeringame[i])
        {
          players[i].mo = NULL;
          G_DeathMatchSpawnPlayer(i);
        }

  // killough 3/26/98: Spawn icon landings:
  if (gamemode==commercial)
    P_SpawnBrainTargets();

  // clear special respawning que
  iquehead = iquetail = 0;

  // set up world state
  P_SpawnSpecials();

  // preload graphics
  if (precache)
    R_PrecacheLevel();
#ifdef GL_DOOM
  // proff 11/99: calculate all OpenGL specific tables etc.
  gld_PreprocessLevel();
#endif	
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

//----------------------------------------------------------------------------
//
// $Log: p_setup.c,v $
// Revision 1.3  2000/05/07 20:19:34  proff_fs
// changed use of colormaps from pointers to numbers.
// That's needed for OpenGL.
// The OpenGL part is slightly better now.
// Added some typedefs to reduce warnings in VisualC.
// Messages are also scaled now, because at 800x600 and
// above you can't read them even on a 21" monitor.
//
// Revision 1.2  2000/05/04 16:40:00  proff_fs
// added OpenGL stuff. Not complete yet.
// Only the playerview is rendered.
// The normal output is displayed in a small window.
// The level is only drawn in debugmode to the window.
//
// Revision 1.1.1.1  2000/05/04 08:13:41  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.15  2000/05/01 14:39:44  Proff
// changed long long to int_64_t for portability
//
// Revision 1.14  1999/11/04 12:33:38  cphipps
// Switch from the Boom fireline-recuding code to the MBF fireline code.
//
// Revision 1.13  1999/10/12 13:01:13  cphipps
// Changed header to GPL
//
// Revision 1.12  1999/09/05 13:42:50  cphipps
// Print warnings when fixing errors in wad files
//
// Revision 1.11  1999/08/18 22:17:43  cphipps
// Partly rewrote P_GroupLines, to eliminate a bad algorithm causing
// big levels to take seconds to load.
//
// Revision 1.10  1999/02/02 09:18:37  cphipps
// Enhanced skies stuff from MBF
//
// Revision 1.9  1999/01/25 16:17:31  cphipps
// Include math.h
//
// Revision 1.8  1999/01/01 16:12:51  cphipps
// Fix erroneous release of lump 0 in first level load
//
// Revision 1.7  1998/12/31 20:53:04  cphipps
// New wad lump handling changes
// rejectmatrix made const*
//
// Revision 1.6  1998/12/27 15:19:17  cphipps
// Use memset to clear body queue
//
// Revision 1.5  1998/12/26 20:09:21  cphipps
// Compatibility option the 'fix common wad errors' code
//
// Revision 1.4  1998/12/23 16:16:11  cphipps
// Fix common wad errors - MBF code imported
// Made P_Load* funcs static
// Replaced a lot of malloc,memset pairs by calloc
//
// Revision 1.3  1998/10/27 18:36:01  cphipps
// Boom v2.02 updated source imported
//
// Revision 1.21  1998/10/13  03:19:21  jim
// Rand's segadjust chosen, Blockmap tweak
//
// Revision 1.18  1998/10/05  21:29:21  phares
// Fixed firelines
//
// Revision 1.17  1998/08/11  19:32:07  phares
// DM Weapon bug fix
//
// Revision 1.16  1998/05/07  00:56:49  killough
// Ignore translucency lumps that are not exactly 64K long
//
// Revision 1.15  1998/05/03  23:04:01  killough
// beautification
//
// Revision 1.14  1998/04/12  02:06:46  killough
// Improve 242 colomap handling, add translucent walls
//
// Revision 1.13  1998/04/06  04:47:05  killough
// Add support for overloading sidedefs for special uses
//
// Revision 1.12  1998/03/31  10:40:42  killough
// Remove blockmap limit
//
// Revision 1.11  1998/03/28  18:02:51  killough
// Fix boss spawner savegame crash bug
//
// Revision 1.10  1998/03/20  00:30:17  phares
// Changed friction to linedef control
//
// Revision 1.9  1998/03/16  12:35:36  killough
// Default floor light level is sector's
//
// Revision 1.8  1998/03/09  07:21:48  killough
// Remove use of FP for point/line queries and add new sector fields
//
// Revision 1.7  1998/03/02  11:46:10  killough
// Double blockmap limit, prepare for when it's unlimited
//
// Revision 1.6  1998/02/27  11:51:05  jim
// Fixes for stairs
//
// Revision 1.5  1998/02/17  22:58:35  jim
// Fixed bug of vanishinb secret sectors in automap
//
// Revision 1.4  1998/02/02  13:38:48  killough
// Comment out obsolete reload hack
//
// Revision 1.3  1998/01/26  19:24:22  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:02:21  killough
// Generalize and simplify level name generation
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
