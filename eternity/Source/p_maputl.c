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
//      Movement/collision utility functions,
//      as used by function in p_map.c.
//      BLOCKMAP Iterator functions,
//      and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_maputl.c,v 1.13 1998/05/03 22:16:48 killough Exp $";

#include "doomstat.h"
#include "m_bbox.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"

// haleyjd
extern int tmfloorpic;

// SoM: monster 3D sides fix
extern boolean tmtouch3dside;

//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
   dx = D_abs(dx);
   dy = D_abs(dy);
   if(dx < dy)
      return dx+dy-(dx>>1);
   return dx+dy-(dy>>1);
}

//
// P_PointOnLineSide
// Returns 0 or 1
//
// killough 5/3/98: reformatted, cleaned up

int P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line)
{
   return
      !line->dx ? x <= line->v1->x ? line->dy > 0 : line->dy < 0 :
      !line->dy ? y <= line->v1->y ? line->dx < 0 : line->dx > 0 :
      FixedMul(y-line->v1->y, line->dx>>FRACBITS) >=
      FixedMul(line->dy>>FRACBITS, x-line->v1->x);
}

//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
// killough 5/3/98: reformatted, cleaned up

int P_BoxOnLineSide(fixed_t *tmbox, line_t *ld)
{
   int p;

   switch (ld->slopetype)
   {
   default: // shut up compiler warnings -- killough
   case ST_HORIZONTAL:
      return
      (tmbox[BOXBOTTOM] > ld->v1->y) == (p = tmbox[BOXTOP] > ld->v1->y) ?
        p ^ (ld->dx < 0) : -1;
   case ST_VERTICAL:
      return
        (tmbox[BOXLEFT] < ld->v1->x) == (p = tmbox[BOXRIGHT] < ld->v1->x) ?
        p ^ (ld->dy < 0) : -1;
   case ST_POSITIVE:
      return
        P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld) ==
        (p = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld)) ? p : -1;
   case ST_NEGATIVE:
      return
        (P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld)) ==
        (p = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld)) ? p : -1;
    }
}

//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
// killough 5/3/98: reformatted, cleaned up

int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line)
{
   return
      !line->dx ? x <= line->x ? line->dy > 0 : line->dy < 0 :
      !line->dy ? y <= line->y ? line->dx < 0 : line->dx > 0 :
      (line->dy^line->dx^(x -= line->x)^(y -= line->y)) < 0 ? (line->dy^x) < 0 :
      FixedMul(y>>8, line->dx>>8) >= FixedMul(line->dy>>8, x>>8);
}

//
// P_MakeDivline
//

void P_MakeDivline(line_t *li, divline_t *dl)
{
   dl->x = li->v1->x;
   dl->y = li->v1->y;
   dl->dx = li->dx;
   dl->dy = li->dy;
}

//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
// killough 5/3/98: reformatted, cleaned up

fixed_t P_InterceptVector(divline_t *v2, divline_t *v1)
{
   fixed_t den = FixedMul(v1->dy>>8, v2->dx) - FixedMul(v1->dx>>8, v2->dy);
   return den ? FixedDiv((FixedMul((v1->x-v2->x)>>8, v1->dy) +
                          FixedMul((v2->y-v1->y)>>8, v1->dx)), den) : 0;
}

//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//

fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t lowfloor;

#ifdef OVER_UNDER
// SoM 11/3/02: opensecfloor, opensecceil.
fixed_t opensecfloor;
fixed_t opensecceil;
#endif

// moved front and back outside P_LineOpening and changed    // phares 3/7/98
// them to these so we can pick up the new friction value
// in PIT_CheckLine()
sector_t *openfrontsector; // made global                    // phares
sector_t *openbacksector;  // made global

// haleyjd 10/16/02: floorsec
sector_t *openfloorsec;

void P_LineOpening(line_t *linedef, mobj_t *mo)
{
   if(linedef->sidenum[1] == -1)      // single sided line
   {
      openrange = 0;
      return;
   }
   
   openfrontsector = linedef->frontsector;
   openbacksector = linedef->backsector;
   
   if(openfrontsector->ceilingheight < openbacksector->ceilingheight)
      opentop = openfrontsector->ceilingheight;
   else
      opentop = openbacksector->ceilingheight;
   
   if(openfrontsector->floorheight > openbacksector->floorheight)
   {
      openbottom = openfrontsector->floorheight;
      lowfloor = openbacksector->floorheight;
      // haleyjd
      tmfloorpic = openfrontsector->floorpic;
      // haleyjd
      openfloorsec = openfrontsector;
      
      // haleyjd 11/11/04: 3DMidTex: we may no longer be on a 
      // 3DMidTex line
      tmtouch3dside = false;
   }
   else
   {
      openbottom = openbacksector->floorheight;
      lowfloor = openfrontsector->floorheight;
      // haleyjd
      tmfloorpic = openbacksector->floorpic;
      // haleyjd
      openfloorsec = openbacksector;

      // haleyjd 11/11/04: 3DMidTex: we may no longer be on a 
      // 3DMidTex line
      tmtouch3dside = false;
   }


   #ifdef OVER_UNDER
   opensecfloor = openbottom;
   opensecceil = opentop;
   #endif

   // SoM 9/02/02: Um... I know I told Quasar` I would do this after 
   // I got SDL_Mixer support and all, but I WANT THIS NOW hehe
   if(demo_version >= 331 && mo && (linedef->flags & ML_3DMIDTEX) && 
      sides[linedef->sidenum[0]].midtexture)
   {
      fixed_t textop, texbot, texmid;
      side_t *side = &sides[linedef->sidenum[0]];
      
      if(linedef->flags & ML_DONTPEGBOTTOM)
      {
         texbot = side->rowoffset + openbottom;
         textop = texbot + textureheight[side->midtexture];
      }
      else
      {
         textop = opentop + side->rowoffset;
         texbot = textop - textureheight[side->midtexture];
      }
      texmid = (textop + texbot)/2;

      // SoM 9/7/02: use monster blocking line to provide better
      // clipping
      if((linedef->flags & ML_BLOCKMONSTERS) && 
         !(mo->flags & (MF_FLOAT | MF_DROPOFF)) &&
         D_abs(mo->z - textop) <= 24*FRACUNIT)
      {
         opentop = openbottom;
         openrange = 0;
         return;
      }
      
      if(mo->z + (mo->info->height/2) < texmid)
      {
         if(texbot < opentop)
            opentop = texbot;
      }
      else
      {
         if(textop > openbottom)
            openbottom = textop;
 
         // haleyjd
         openfloorsec = NULL;
      }

      // SoM 09/07/02: let monsters walk over dropoffs
      
      // haleyjd 11/10/04: 3DMidTex: tmtouch3dside is now
      // used differently, see above and in PIT_CheckLine.
      // We may be standing on a 3dmidtex line now.
      tmtouch3dside = true;

   }

   openrange = opentop - openbottom;
}

//
// THING POSITION SETTING
//

//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//

void P_UnsetThingPosition (mobj_t *thing)
{
   if(!(thing->flags & MF_NOSECTOR))
   {
      // invisible things don't need to be in sector list
      // unlink from subsector
      
      // killough 8/11/98: simpler scheme using pointers-to-pointers for prev
      // pointers, allows head node pointers to be treated like everything else
      mobj_t **sprev = thing->sprev;
      mobj_t  *snext = thing->snext;
      if((*sprev = snext))  // unlink from sector list
         snext->sprev = sprev;

      // phares 3/14/98
      //
      // Save the sector list pointed to by touching_sectorlist.
      // In P_SetThingPosition, we'll keep any nodes that represent
      // sectors the Thing still touches. We'll add new ones then, and
      // delete any nodes for sectors the Thing has vacated. Then we'll
      // put it back into touching_sectorlist. It's done this way to
      // avoid a lot of deleting/creating for nodes, when most of the
      // time you just get back what you deleted anyway.
      //
      // If this Thing is being removed entirely, then the calling
      // routine will clear out the nodes in sector_list.
      
      sector_list = thing->touching_sectorlist;
      thing->touching_sectorlist = NULL; //to be restored by P_SetThingPosition
   }

   if(!(thing->flags & MF_NOBLOCKMAP))
   {
      // inert things don't need to be in blockmap
      
      // killough 8/11/98: simpler scheme using pointers-to-pointers for prev
      // pointers, allows head node pointers to be treated like everything else
      //
      // Also more robust, since it doesn't depend on current position for
      // unlinking. Old method required computing head node based on position
      // at time of unlinking, assuming it was the same position as during
      // linking.
      
      mobj_t *bnext, **bprev = thing->bprev;
      if(bprev && (*bprev = bnext = thing->bnext))  // unlink from block map
         bnext->bprev = bprev;
   }
}

//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
// killough 5/3/98: reformatted, cleaned up

void P_SetThingPosition(mobj_t *thing)
{                                                      // link into subsector
   subsector_t *ss = thing->subsector = 
      R_PointInSubsector(thing->x, thing->y);

   if(!(thing->flags & MF_NOSECTOR))
   {
      // invisible things don't go into the sector links
      
      // killough 8/11/98: simpler scheme using pointer-to-pointer prev
      // pointers, allows head nodes to be treated like everything else
      
      mobj_t **link = &ss->sector->thinglist;
      mobj_t *snext = *link;
      if((thing->snext = snext))
         snext->sprev = &thing->snext;
      thing->sprev = link;
      *link = thing;

      // phares 3/16/98
      //
      // If sector_list isn't NULL, it has a collection of sector
      // nodes that were just removed from this Thing.

      // Collect the sectors the object will live in by looking at
      // the existing sector_list and adding new nodes and deleting
      // obsolete ones.

      // When a node is deleted, its sector links (the links starting
      // at sector_t->touching_thinglist) are broken. When a node is
      // added, new sector links are created.

      P_CreateSecNodeList(thing, thing->x, thing->y);
      thing->touching_sectorlist = sector_list; // Attach to Thing's mobj_t
      sector_list = NULL; // clear for next time
   }

   // link into blockmap
   if(!(thing->flags & MF_NOBLOCKMAP))
   {
      // inert things don't need to be in blockmap
      int blockx = (thing->x - bmaporgx)>>MAPBLOCKSHIFT;
      int blocky = (thing->y - bmaporgy)>>MAPBLOCKSHIFT;
      
      if(blockx>=0 && blockx < bmapwidth && blocky>=0 && blocky < bmapheight)
      {
         // killough 8/11/98: simpler scheme using pointer-to-pointer prev
         // pointers, allows head nodes to be treated like everything else

         mobj_t **link = &blocklinks[blocky*bmapwidth+blockx];
         mobj_t *bnext = *link;
         if((thing->bnext = bnext))
            bnext->bprev = &thing->bnext;
         thing->bprev = link;
         *link = thing;
      }
      else        // thing is off the map
         thing->bnext = NULL, thing->bprev = NULL;
   }
}

// killough 3/15/98:
//
// A fast function for testing intersections between things and linedefs.
//
// haleyjd: note -- this is never called, and is, according to
// SoM, VERY inaccurate. I don't really know what its for or why
// its here, but I'm leaving it be.
//
boolean ThingIsOnLine(mobj_t *t, line_t *l)
{
   int dx = l->dx >> FRACBITS;                           // Linedef vector
   int dy = l->dy >> FRACBITS;
   int a = (l->v1->x >> FRACBITS) - (t->x >> FRACBITS);  // Thing-->v1 vector
   int b = (l->v1->y >> FRACBITS) - (t->y >> FRACBITS);
   int r = t->radius >> FRACBITS;                        // Thing radius

   // First make sure bounding boxes of linedef and thing intersect.
   // Leads to quick rejection using only shifts and adds/subs/compares.
   
   if(D_abs(a*2+dx)-D_abs(dx) > r*2 || D_abs(b*2+dy)-D_abs(dy) > r*2)
      return 0;

   // Next, make sure that at least one thing crosshair intersects linedef's
   // extension. Requires only 3-4 multiplications, the rest adds/subs/
   // shifts/xors (writing the steps out this way leads to better codegen).

   a *= dy;
   b *= dx;
   a -= b;
   b = dx + dy;
   b *= r;
   if(((a-b)^(a+b)) < 0)
      return 1;
   dy -= dx;
   dy *= r;
   b = a+dy;
   a -= dy;
   return (a^b) < 0;
}

//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//

//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
// killough 5/3/98: reformatted, cleaned up

boolean P_BlockLinesIterator(int x, int y, boolean func(line_t*))
{
   int        offset;
   const long *list;   // killough 3/1/98: for removal of blockmap limit
   
   if (x<0 || y<0 || x>=bmapwidth || y>=bmapheight)
      return true;
   offset = y*bmapwidth+x;
   offset = *(blockmap+offset);
   list = blockmaplump+offset;  // original was reading         // phares
                                // delimiting 0 as linedef 0    // phares

   // killough 1/31/98: for compatibility we need to use the old method.
   // Most demos go out of sync, and maybe other problems happen, if we
   // don't consider linedef 0. For safety this should be qualified.

   if(!demo_compatibility) // killough 2/22/98: demo_compatibility check
      list++;     // skip 0 starting delimiter                 // phares
   for( ; *list != -1 ; list++)                                // phares
   {
      line_t *ld = &lines[*list];
      if(ld->validcount == validcount)
         continue;       // line has already been checked
      ld->validcount = validcount;
      if(!func(ld))
         return false;
   }
   return true;  // everything was checked
}

//
// P_BlockThingsIterator
//
// killough 5/3/98: reformatted, cleaned up

boolean P_BlockThingsIterator(int x, int y, boolean func(mobj_t*))
{
   mobj_t *mobj;
   if(!(x<0 || y<0 || x>=bmapwidth || y>=bmapheight))
      for(mobj = blocklinks[y*bmapwidth+x]; mobj; mobj = mobj->bnext)
         if(!func(mobj))
            return false;
   return true;
}

//
// INTERCEPT ROUTINES
//

// 1/11/98 killough: Intercept limit removed
static intercept_t *intercepts, *intercept_p;

// Check for limit and double size if necessary -- killough
static void check_intercept(void)
{
   static size_t num_intercepts;
   size_t offset = intercept_p - intercepts;
   if(offset >= num_intercepts)
   {
      num_intercepts = num_intercepts ? num_intercepts*2 : 128;
      intercepts = realloc(intercepts, sizeof(*intercepts)*num_intercepts);
      intercept_p = intercepts + offset;
   }
}

divline_t trace;

// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
//
// killough 5/3/98: reformatted, cleaned up

boolean PIT_AddLineIntercepts(line_t *ld)
{
   int       s1;
   int       s2;
   fixed_t   frac;
   divline_t dl;

   // avoid precision problems with two routines
   if(trace.dx >  FRACUNIT*16 || trace.dy >  FRACUNIT*16 ||
      trace.dx < -FRACUNIT*16 || trace.dy < -FRACUNIT*16)
   {
      s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
      s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
   }
   else
   {
      s1 = P_PointOnLineSide (trace.x, trace.y, ld);
      s2 = P_PointOnLineSide (trace.x+trace.dx, trace.y+trace.dy, ld);
   }

   if(s1 == s2)
      return true;        // line isn't crossed
   
   // hit the line
   P_MakeDivline(ld, &dl);
   frac = P_InterceptVector(&trace, &dl);
   
   if(frac < 0)
      return true;        // behind source

   check_intercept();    // killough
   
   intercept_p->frac = frac;
   intercept_p->isaline = true;
   intercept_p->d.line = ld;
   intercept_p++;
   
   return true;  // continue
}

//
// PIT_AddThingIntercepts
//
// killough 5/3/98: reformatted, cleaned up

boolean PIT_AddThingIntercepts(mobj_t *thing)
{
   fixed_t   x1, y1;
   fixed_t   x2, y2;
   int       s1, s2;
   divline_t dl;
   fixed_t   frac;

   // check a corner to corner crossection for hit
   if((trace.dx ^ trace.dy) > 0)
   {
      x1 = thing->x - thing->radius;
      y1 = thing->y + thing->radius;
      x2 = thing->x + thing->radius;
      y2 = thing->y - thing->radius;
   }
   else
   {
      x1 = thing->x - thing->radius;
      y1 = thing->y - thing->radius;
      x2 = thing->x + thing->radius;
      y2 = thing->y + thing->radius;
   }

   s1 = P_PointOnDivlineSide (x1, y1, &trace);
   s2 = P_PointOnDivlineSide (x2, y2, &trace);
   
   if(s1 == s2)
      return true;                // line isn't crossed

   dl.x = x1;
   dl.y = y1;
   dl.dx = x2-x1;
   dl.dy = y2-y1;
   
   frac = P_InterceptVector (&trace, &dl);
   
   if (frac < 0)
      return true;                // behind source
   
   check_intercept();            // killough
   
   intercept_p->frac = frac;
   intercept_p->isaline = false;
   intercept_p->d.thing = thing;
   intercept_p++;
   
   return true;          // keep going
}

//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
// killough 5/3/98: reformatted, cleaned up

boolean P_TraverseIntercepts(traverser_t func, fixed_t maxfrac)
{
   intercept_t *in = NULL;
   int count = intercept_p - intercepts;
   while(count--)
   {
      fixed_t dist = D_MAXINT;
      intercept_t *scan;
      for(scan = intercepts; scan < intercept_p; scan++)
         if(scan->frac < dist)
            dist = (in=scan)->frac;
      if(dist > maxfrac)
         return true;    // checked everything in range
      if(!func(in))
         return false;           // don't bother going farther
      in->frac = D_MAXINT;
   }
   return true;                  // everything was traversed
}

//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
// killough 5/3/98: reformatted, cleaned up

boolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
                       int flags, boolean trav(intercept_t *))
{
   fixed_t xt1, yt1;
   fixed_t xt2, yt2;
   fixed_t xstep, ystep;
   fixed_t partial;
   fixed_t xintercept, yintercept;
   int     mapx, mapy;
   int     mapxstep, mapystep;
   int     count;

   validcount++;
   intercept_p = intercepts;
   
   if(!((x1-bmaporgx)&(MAPBLOCKSIZE-1)))
      x1 += FRACUNIT;     // don't side exactly on a line
   
   if(!((y1-bmaporgy)&(MAPBLOCKSIZE-1)))
      y1 += FRACUNIT;     // don't side exactly on a line

   trace.x = x1;
   trace.y = y1;
   trace.dx = x2 - x1;
   trace.dy = y2 - y1;
   
   x1 -= bmaporgx;
   y1 -= bmaporgy;
   xt1 = x1>>MAPBLOCKSHIFT;
   yt1 = y1>>MAPBLOCKSHIFT;
   
   x2 -= bmaporgx;
   y2 -= bmaporgy;
   xt2 = x2>>MAPBLOCKSHIFT;
   yt2 = y2>>MAPBLOCKSHIFT;

   if(xt2 > xt1)
   {
      mapxstep = 1;
      partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
      ystep = FixedDiv (y2-y1,D_abs(x2-x1));
   }
   else if(xt2 < xt1)
   {
      mapxstep = -1;
      partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
      ystep = FixedDiv (y2-y1,D_abs(x2-x1));
   }
   else
   {
      mapxstep = 0;
      partial = FRACUNIT;
      ystep = 256*FRACUNIT;
   }

   yintercept = (y1>>MAPBTOFRAC) + FixedMul(partial, ystep);
   
   if(yt2 > yt1)
   {
      mapystep = 1;
      partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
      xstep = FixedDiv (x2-x1,D_abs(y2-y1));
   }
   else if(yt2 < yt1)
   {
      mapystep = -1;
      partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
      xstep = FixedDiv (x2-x1,D_abs(y2-y1));
   }
   else
   {
      mapystep = 0;
      partial = FRACUNIT;
      xstep = 256*FRACUNIT;
   }

   xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);
   
   // Step through map blocks.
   // Count is present to prevent a round off error
   // from skipping the break.
   
   mapx = xt1;
   mapy = yt1;

   for(count = 0; count < 64; count++)
   {
      if(flags & PT_ADDLINES)
      {
         if(!P_BlockLinesIterator(mapx, mapy,PIT_AddLineIntercepts))
            return false; // early out
      }
      
      if(flags & PT_ADDTHINGS)
      {
         if(!P_BlockThingsIterator(mapx, mapy,PIT_AddThingIntercepts))
            return false; // early out
      }
      
      if(mapx == xt2 && mapy == yt2)
         break;
      
      if((yintercept >> FRACBITS) == mapy)
      {
         yintercept += ystep;
         mapx += mapxstep;
      }
      else if((xintercept >> FRACBITS) == mapx)
      {
         xintercept += xstep;
         mapy += mapystep;
      }
   }

   // go through the sorted list
   return P_TraverseIntercepts(trav, FRACUNIT);
}

//----------------------------------------------------------------------------
//
// $Log: p_maputl.c,v $
// Revision 1.13  1998/05/03  22:16:48  killough
// beautification
//
// Revision 1.12  1998/03/20  00:30:03  phares
// Changed friction to linedef control
//
// Revision 1.11  1998/03/19  14:37:12  killough
// Fix ThingIsOnLine()
//
// Revision 1.10  1998/03/19  00:40:52  killough
// Change ThingIsOnLine() comments
//
// Revision 1.9  1998/03/16  12:34:45  killough
// Add ThingIsOnLine() function
//
// Revision 1.8  1998/03/09  18:27:10  phares
// Fixed bug in neighboring variable friction sectors
//
// Revision 1.7  1998/03/09  07:19:26  killough
// Remove use of FP for point/line queries
//
// Revision 1.6  1998/03/02  12:03:43  killough
// Change blockmap offsets to 32-bit
//
// Revision 1.5  1998/02/23  04:45:24  killough
// Relax blockmap iterator to demo_compatibility
//
// Revision 1.4  1998/02/02  13:41:38  killough
// Fix demo sync programs caused by last change
//
// Revision 1.3  1998/01/30  23:13:10  phares
// Fixed delimiting 0 bug in P_BlockLinesIterator
//
// Revision 1.2  1998/01/26  19:24:11  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
