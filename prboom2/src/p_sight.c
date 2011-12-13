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
 *      LineOfSight/Visibility checks, uses REJECT Lookup Table.
 *
 *-----------------------------------------------------------------------------*/

#include "doomstat.h"
#include "doomtype.h"
#include "r_main.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "m_bbox.h"
#include "lprintf.h"
#include "g_overflow.h"
#include "e6y.h" //e6y


/*
==============================================================================

P_CheckSight
This uses specialized forms of the maputils routines for optimized performance

==============================================================================
*/

fixed_t sightzstart;            // eye z of looker
fixed_t topslope, bottomslope;  // slopes to top and bottom of target
int sightcounts[3];

CrossSubsectorFunc P_CrossSubsector;

/*
==============
=
= PTR_SightTraverse
=
==============
*/

dboolean PTR_SightTraverse(intercept_t *in)
{
  line_t *li;
  fixed_t slope;

  li = in->d.line;

  //
  // crosses a two sided line
  //
  P_LineOpening(li);

  if (openbottom >= opentop)  // quick test for totally closed doors
    return false;  // stop

  if (li->frontsector->floorheight != li->backsector->floorheight)
  {
    slope = FixedDiv(openbottom - sightzstart , in->frac);
    if (slope > bottomslope)
      bottomslope = slope;
  }

  if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
  {
    slope = FixedDiv(opentop - sightzstart, in->frac);
    if (slope < topslope)
      topslope = slope;
  }

  if (topslope <= bottomslope)
    return false;  // stop

  return true;  // keep going
}



/*
==================
=
= P_SightBlockLinesIterator
=
===================
*/

dboolean P_SightBlockLinesIterator(int x, int y)
{
  int offset;
  int *list;
  line_t *ld;
  int s1, s2;
  divline_t dl;

  offset = y*bmapwidth+x;

  offset = *(blockmap+offset);

  for (list = blockmaplump+offset; *list != -1; list++)
  {
    ld = &lines[*list];
    if (ld->validcount == validcount)
      continue;    // line has already been checked
    ld->validcount = validcount;

    s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
    s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
    if (s1 == s2)
      continue;    // line isn't crossed
    P_MakeDivline (ld, &dl);
    s1 = P_PointOnDivlineSide(trace.x, trace.y, &dl);
    s2 = P_PointOnDivlineSide(trace.x+trace.dx, trace.y+trace.dy, &dl);
    if (s1 == s2)
      continue; // line isn't crossed

    // try to early out the check
    if (!ld->backsector)
      return false; // stop checking

    check_intercept();    // killough

    // store the line for later intersection testing
    intercept_p->d.line = ld;
    intercept_p++;

  }

  return true; // everything was checked
}

/*
====================
=
= P_SightTraverseIntercepts
=
= Returns true if the traverser function returns true for all lines
====================
*/

dboolean P_SightTraverseIntercepts(void)
{
  int count;
  fixed_t dist;
  intercept_t *scan, *in;
  divline_t dl;

  count = intercept_p - intercepts;
  //
  // calculate intercept distance
  //
  for (scan = intercepts; scan<intercept_p; scan++)
  {
    P_MakeDivline(scan->d.line, &dl);
    scan->frac = P_InterceptVector(&trace, &dl);    
  }

  //
  // go through in order
  //  
  in = 0; // shut up compiler warning

  while (count--)
  {
    dist = INT_MAX;
    for (scan = intercepts ; scan<intercept_p ; scan++)
      if (scan->frac < dist)
      {
        dist = scan->frac;
        in = scan;
      }

      if (!PTR_SightTraverse(in))
        return false;      // don't bother going farther
      in->frac = INT_MAX;
  }

  return true;    // everything was traversed
}



/*
==================
=
= P_SightPathTraverse
=
= Traces a line from x1,y1 to x2,y2, calling the traverser function for each
= Returns true if the traverser function returns true for all lines
==================
*/

dboolean P_SightPathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
  fixed_t xt1,yt1,xt2,yt2;
  fixed_t xstep,ystep;
  fixed_t partial;
  fixed_t xintercept, yintercept;
  int mapx, mapy, mapxstep, mapystep;
  int count;

  validcount++;
  intercept_p = intercepts;

  if (((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
    x1 += FRACUNIT;        // don't side exactly on a line
  if (((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
    y1 += FRACUNIT;        // don't side exactly on a line
  trace.x = x1;
  trace.y = y1;
  trace.dx = x2 - x1;
  trace.dy = y2 - y1;

  x1 -= bmaporgx;
  y1 -= bmaporgy;
  xt1 = P_GetSafeBlockX(x1);
  yt1 = P_GetSafeBlockY(y1);

  x2 -= bmaporgx;
  y2 -= bmaporgy;
  xt2 = P_GetSafeBlockX(x2);
  yt2 = P_GetSafeBlockY(y2);

  // points should never be out of bounds, but check once instead of
  // each block
  if (xt1<0 || yt1<0 || xt1>=bmapwidth || yt1>=bmapheight
    || xt2<0 || yt2<0 || xt2>=bmapwidth || yt2>=bmapheight)
    return false;

  if (xt2 > xt1)
  {
    mapxstep = 1;
    partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
    ystep = FixedDiv (y2-y1,abs(x2-x1));
  }
  else if (xt2 < xt1)
  {
    mapxstep = -1;
    partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
    ystep = FixedDiv (y2-y1,abs(x2-x1));
  }
  else
  {
    mapxstep = 0;
    partial = FRACUNIT;
    ystep = 256*FRACUNIT;
  }  
  yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


  if (yt2 > yt1)
  {
    mapystep = 1;
    partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
    xstep = FixedDiv (x2-x1,abs(y2-y1));
  }
  else if (yt2 < yt1)
  {
    mapystep = -1;
    partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
    xstep = FixedDiv (x2-x1,abs(y2-y1));
  }
  else
  {
    mapystep = 0;
    partial = FRACUNIT;
    xstep = 256*FRACUNIT;
  }  
  xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);


  //
  // step through map blocks
  // Count is present to prevent a round off error from skipping the break
  mapx = xt1;
  mapy = yt1;


  for (count = 0; count < 64; count++)
  {
    if (!P_SightBlockLinesIterator(mapx, mapy))
    {
      sightcounts[1]++;
      return false;  // early out
    }

    if (mapx == xt2 && mapy == yt2)
      break;

    if ((yintercept >> FRACBITS) == mapy)
    {
      yintercept += ystep;
      mapx += mapxstep;
    }
    else if ((xintercept >> FRACBITS) == mapx)
    {
      xintercept += xstep;
      mapy += mapystep;
    }

  }


  //
  // couldn't early out, so go through the sorted list
  //
  sightcounts[2]++;

  return P_SightTraverseIntercepts();
}



/*
=====================
=
= P_CheckSight
=
= Returns true if a straight line between t1 and t2 is unobstructed
= look from eyes of t1 to any part of t2
=
=====================
*/

dboolean P_CheckSight_12(mobj_t *t1, mobj_t *t2)
{
  int s1, s2;
  int pnum, bytenum, bitnum;

  //
  // check for trivial rejection
  //
  s1 = (t1->subsector->sector->iSectorID);
  s2 = (t2->subsector->sector->iSectorID);
  pnum = s1*numsectors + s2;
  bytenum = pnum>>3;
  bitnum = 1 << (pnum&7);

  if (rejectmatrix[bytenum]&bitnum)
  {
    sightcounts[0]++;
    return false;    // can't possibly be connected
  }

  //
  // check precisely
  //    
  sightzstart = t1->z + t1->height - (t1->height>>2);
  topslope = (t2->z+t2->height) - sightzstart;
  bottomslope = (t2->z) - sightzstart;

  return P_SightPathTraverse (t1->x, t1->y, t2->x, t2->y);
}

//
// P_CheckSight
//
// killough 4/19/98:
// Convert LOS info to struct for reentrancy and efficiency of data locality

typedef struct {
  fixed_t sightzstart, t2x, t2y;   // eye z of looker
  divline_t strace;                // from t1 to t2
  fixed_t topslope, bottomslope;   // slopes to top and bottom of target
  fixed_t bbox[4];
  fixed_t maxz,minz;               // cph - z optimisations for 2sided lines
} los_t;

static los_t los; // cph - made static

//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//
// killough 4/19/98: made static, cleaned up

INLINE static int P_DivlineSide(fixed_t x, fixed_t y, const divline_t *node)
{
  fixed_t left, right;
  return
    !node->dx ? x == node->x ? 2 : x <= node->x ? node->dy > 0 : node->dy < 0 :
    !node->dy ? ( compatibility_level < prboom_4_compatibility ? x : y) == node->y ? 2 : y <= node->y ? node->dx < 0 : node->dx > 0 :
    (right = ((y - node->y) >> FRACBITS) * (node->dx >> FRACBITS)) <
    (left  = ((x - node->x) >> FRACBITS) * (node->dy >> FRACBITS)) ? 0 :
    right == left ? 2 : 1;
}

INLINE static int P_DivlineCrossed(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, divline_t *node)
{
#if 1
   return (P_DivlineSide(x1, y1, node) == P_DivlineSide(x2, y2, node));
#else
  if (!node->dx)
  {
    if (x1 == node->x)
      return (x2 == node->x);
    if (x2 == node->x)
      return (x1 == node->x);
    if (x1 < node->x)
      return (x2 < node->x);
    return (x2 > node->x);
  }

  if (!node->dy)
  {
    fixed_t _y1, _y2;
    if (compatibility_level < prboom_4_compatibility)
    {
      _y1 = x1;
      _y2 = x2;
    }
    else
    {
      _y1 = y1;
      _y2 = y2;
    }
    //if ((compatibility_level < prboom_4_compatibility ? x1 : y1) == node->y)
    //  return (x2 == node->y);
    if (_y1 == node->y)
      return (_y2 == node->y);
    if (_y2 == node->y)
      return (_y1 == node->y);
    if (y1 <= node->y)
      return (y2 < node->y);
    return (y2 > node->y);
  }

  {
    fixed_t node_dx = (node->dx >> FRACBITS);
    fixed_t node_dy = (node->dy >> FRACBITS);

    fixed_t left1  = node_dy * ((x1 - node->x) >> FRACBITS);
    fixed_t right1 = ((y1 - node->y) >> FRACBITS) * node_dx;
    fixed_t left2  = node_dy * ((x2 - node->x) >> FRACBITS);
    fixed_t right2 = ((y2 - node->y) >> FRACBITS) * node_dx;

    if (right1 < left1)
      return (right2 < left2);

    if (left1 == right1)
      return (left2 == right2);

    return (right2 > left2);
  }
#endif
}


//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//
// killough 4/19/98: made static and cleaned up

dboolean P_CrossSubsector_PrBoom(int num)
{
  ssline_t *ssline = &sslines[sslines_indexes[num]];
  const ssline_t *ssline_last = &sslines[sslines_indexes[num + 1]];
  fixed_t opentop = 0, openbottom = 0;
  const sector_t *front = NULL, *back = NULL;

#ifdef RANGECHECK
  if (num >= numsubsectors)
    I_Error("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  // check lines
  for (; ssline < ssline_last; ssline++)
  {
    divline_t divl;

    /* OPTIMIZE: killough 4/20/98: Added quick bounding-box rejection test
     * cph - this is causing demo desyncs on original Doom demos.
     *  Who knows why. Exclude test for those.
     */
    if (ssline->bbox[BOXLEFT  ] > los.bbox[BOXRIGHT ] ||
        ssline->bbox[BOXRIGHT ] < los.bbox[BOXLEFT  ] ||
        ssline->bbox[BOXBOTTOM] > los.bbox[BOXTOP   ] ||
        ssline->bbox[BOXTOP]    < los.bbox[BOXBOTTOM])
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    // Forget this line if it doesn't cross the line of sight
    if (P_DivlineCrossed(ssline->x1, ssline->y1, ssline->x2, ssline->y2, &los.strace))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    divl.dx = ssline->x2 - (divl.x = ssline->x1);
    divl.dy = ssline->y2 - (divl.y = ssline->y1);

    // line isn't crossed?
    if (P_DivlineCrossed(los.strace.x, los.strace.y, los.t2x, los.t2y, &divl))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    // allready checked other side?
    if (ssline->linedef->validcount == validcount)
      continue;

    ssline->linedef->validcount = validcount;

    // cph - do what we can before forced to check intersection
    if (ssline->linedef->flags & ML_TWOSIDED)
    {
      // crosses a two sided line
      front = ssline->seg->frontsector;
      back = ssline->seg->backsector;

      // no wall to block sight with?
      if (front->floorheight == back->floorheight
        && front->ceilingheight == back->ceilingheight)
        continue;	

      // possible occluder
      // because of ceiling height differences
      opentop = MIN(front->ceilingheight, back->ceilingheight);

      // because of floor height differences
      openbottom = MAX(front->floorheight, back->floorheight);

      // cph - reject if does not intrude in the z-space of the possible LOS
      if ((opentop >= los.maxz) && (openbottom <= los.minz))
        continue;
    }

    // cph - if bottom >= top or top < minz or bottom > maxz then it must be
    // solid wrt this LOS
    if (!(ssline->linedef->flags & ML_TWOSIDED) || (openbottom >= opentop) ||
  (prboom_comp[PC_FORCE_LXDOOM_DEMO_COMPATIBILITY].state ?
  (opentop <= los.minz) || (openbottom >= los.maxz) :
  (opentop < los.minz) || (openbottom > los.maxz)))
  return false;

    { // crosses a two sided line
      /* cph 2006/07/15 - oops, we missed this in 2.4.0 & .1;
       *  use P_InterceptVector2 for those compat levels only. */ 
      fixed_t frac = (compatibility_level == prboom_5_compatibility || compatibility_level == prboom_6_compatibility) ?
		      P_InterceptVector2(&los.strace, &divl) : 
		      P_InterceptVector(&los.strace, &divl);

      if (front->floorheight != back->floorheight)
      {
        fixed_t slope = FixedDiv(openbottom - los.sightzstart, frac);
        if (slope > los.bottomslope)
          los.bottomslope = slope;
      }

      if (front->ceilingheight != back->ceilingheight)
      {
        fixed_t slope = FixedDiv(opentop - los.sightzstart, frac);
        if (slope < los.topslope)
          los.topslope = slope;
      }

      if (los.topslope <= los.bottomslope)
        return false;               // stop
    }
  }
  // passed the subsector ok
  return true;
}

dboolean P_CrossSubsector_Doom(int num)
{
  ssline_t *ssline = &sslines[sslines_indexes[num]];
  const ssline_t *ssline_last = &sslines[sslines_indexes[num + 1]];

#ifdef RANGECHECK
  if (num >= numsubsectors)
    I_Error("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  for (; ssline < ssline_last; ssline++)
  {
    divline_t divl;
    fixed_t opentop, openbottom;
    const sector_t *front, *back;
    fixed_t frac;

    // line isn't crossed?
    if (P_DivlineCrossed(ssline->x1, ssline->y1, ssline->x2, ssline->y2, &los.strace))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    divl.dx = ssline->x2 - (divl.x = ssline->x1);
    divl.dy = ssline->y2 - (divl.y = ssline->y1);

    // line isn't crossed?
    if (P_DivlineCrossed(los.strace.x, los.strace.y, los.t2x, los.t2y, &divl))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    // allready checked other side?
    if (ssline->linedef->validcount == validcount)
      continue;

    ssline->linedef->validcount = validcount;

    // stop because it is not two sided anyway
    if (!(ssline->linedef->flags & ML_TWOSIDED))
      return false;

    // crosses a two sided line
    front = ssline->seg->frontsector;
    back = ssline->seg->backsector;

    // missed back side on two-sided lines.
    if (!back)
    {
      back = GetSectorAtNullAddress();
    }

    // no wall to block sight with?
    if (front->floorheight == back->floorheight
      && front->ceilingheight == back->ceilingheight)
      continue;	

    // possible occluder
    // because of ceiling height differences
    opentop = MIN(front->ceilingheight, back->ceilingheight);

    // because of floor height differences
    openbottom = MAX(front->floorheight, back->floorheight);

    // quick test for totally closed doors
    if (openbottom >= opentop)
      return false;               // stop

    frac = P_InterceptVector2(&los.strace, &divl);

    if (front->floorheight != back->floorheight)
    {
      fixed_t slope = FixedDiv(openbottom - los.sightzstart, frac);
      if (slope > los.bottomslope)
        los.bottomslope = slope;
    }

    if (front->ceilingheight != back->ceilingheight)
    {
      fixed_t slope = FixedDiv(opentop - los.sightzstart, frac);
      if (slope < los.topslope)
        los.topslope = slope;
    }

    if (los.topslope <= los.bottomslope)
      return false;               // stop
  }
  // passed the subsector ok
  return true;
}

dboolean P_CrossSubsector_Boom(int num)
{
  ssline_t *ssline = &sslines[sslines_indexes[num]];
  const ssline_t *ssline_last = &sslines[sslines_indexes[num + 1]];

#ifdef RANGECHECK
  if (num >= numsubsectors)
    I_Error("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  for (; ssline < ssline_last; ssline++)
  {
    divline_t divl;
    fixed_t opentop, openbottom;
    const sector_t *front, *back;
    fixed_t frac;

    // OPTIMIZE: killough 4/20/98: Added quick bounding-box rejection test

    if (ssline->bbox[BOXLEFT  ] > los.bbox[BOXRIGHT ] ||
        ssline->bbox[BOXRIGHT ] < los.bbox[BOXLEFT  ] ||
        ssline->bbox[BOXBOTTOM] > los.bbox[BOXTOP   ] ||
        ssline->bbox[BOXTOP]    < los.bbox[BOXBOTTOM])
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    // line isn't crossed?
    if (P_DivlineCrossed(ssline->x1, ssline->y1, ssline->x2, ssline->y2, &los.strace))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    divl.dx = ssline->x2 - (divl.x = ssline->x1);
    divl.dy = ssline->y2 - (divl.y = ssline->y1);

    // line isn't crossed?
    if (P_DivlineCrossed(los.strace.x, los.strace.y, los.t2x, los.t2y, &divl))
    {
      ssline->linedef->validcount = validcount;
      continue;
    }

    // allready checked other side?
    if (ssline->linedef->validcount == validcount)
      continue;

    ssline->linedef->validcount = validcount;

    // stop because it is not two sided anyway
    if (!(ssline->linedef->flags & ML_TWOSIDED))
      return false;

    // crosses a two sided line
    front = ssline->seg->frontsector;
    back = ssline->seg->backsector;

    // no wall to block sight with?
    if (front->floorheight == back->floorheight
      && front->ceilingheight == back->ceilingheight)
      continue;

    // possible occluder
    // because of ceiling height differences
    opentop = MIN(front->ceilingheight, back->ceilingheight);

    // because of floor height differences
    openbottom = MAX(front->floorheight, back->floorheight);

    // quick test for totally closed doors
    if (openbottom >= opentop)
      return false;               // stop

    frac = P_InterceptVector2(&los.strace, &divl);

    if (front->floorheight != back->floorheight)
    {
      fixed_t slope = FixedDiv(openbottom - los.sightzstart, frac);
      if (slope > los.bottomslope)
        los.bottomslope = slope;
    }

    if (front->ceilingheight != back->ceilingheight)
    {
      fixed_t slope = FixedDiv(opentop - los.sightzstart, frac);
      if (slope < los.topslope)
        los.topslope = slope;
    }

    if (los.topslope <= los.bottomslope)
      return false;               // stop
  }
  // passed the subsector ok
  return true;
}

//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
// killough 4/20/98: rewritten to remove tail recursion, clean up, and optimize
// cph - Made to use R_PointOnSide instead of P_DivlineSide, since the latter
//  could return 2 which was ambigous, and the former is
//  better optimised; also removes two casts :-)

static dboolean P_CrossBSPNode_LxDoom(int bspnum)
{
  while (!(bspnum & NF_SUBSECTOR))
    {
      register const node_t *bsp = nodes + bspnum;
      int side,side2;
      side = R_PointOnSide(los.strace.x, los.strace.y, bsp);
      side2 = R_PointOnSide(los.t2x, los.t2y, bsp);
      if (side == side2)
         bspnum = bsp->children[side]; // doesn't touch the other side
      else         // the partition plane is crossed here
        if (!P_CrossBSPNode_LxDoom(bsp->children[side]))
          return 0;  // cross the starting side
        else
          bspnum = bsp->children[side^1];  // cross the ending side
    }
  return P_CrossSubsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

static dboolean P_CrossBSPNode_PrBoom(int bspnum)
{
  while (!(bspnum & NF_SUBSECTOR))
    {
      register const node_t *bsp = nodes + bspnum;
      int side,side2;
      side = P_DivlineSide(los.strace.x,los.strace.y,(const divline_t *)bsp)&1;
      side2= P_DivlineSide(los.t2x, los.t2y, (const divline_t *) bsp);
      if (side == side2)
         bspnum = bsp->children[side]; // doesn't touch the other side
      else         // the partition plane is crossed here
        if (!P_CrossBSPNode_PrBoom(bsp->children[side]))
          return 0;  // cross the starting side
        else
          bspnum = bsp->children[side^1];  // cross the ending side
    }
  return P_CrossSubsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

/* proff - Moved the compatibility check outside the functions
 * this gives a slight speedup
 */
static dboolean P_CrossBSPNode(int bspnum)
{
  /* cph - LxDoom used some R_* funcs here */
  if (compatibility_level == lxdoom_1_compatibility || prboom_comp[PC_FORCE_LXDOOM_DEMO_COMPATIBILITY].state)
    return P_CrossBSPNode_LxDoom(bspnum);
  else
    return P_CrossBSPNode_PrBoom(bspnum);
}

//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
// killough 4/20/98: cleaned up, made to use new LOS struct

dboolean P_CheckSight(mobj_t *t1, mobj_t *t2)
{
  const sector_t *s1, *s2;
  int pnum;

  if (compatibility_level == doom_12_compatibility)
  {
    return P_CheckSight_12(t1, t2);
  }

  s1 = t1->subsector->sector;
  s2 = t2->subsector->sector;
  pnum = (s1->iSectorID)*numsectors + (s2->iSectorID);

  // First check for trivial rejection.
  // Determine subsector entries in REJECT table.
  //
  // Check in REJECT table.

  if (rejectmatrix[pnum>>3] & (1 << (pnum&7)))   // can't possibly be connected
    return false;

  // killough 4/19/98: make fake floors and ceilings block monster view

  if ((s1->heightsec != -1 &&
       ((t1->z + t1->height <= sectors[s1->heightsec].floorheight &&
         t2->z >= sectors[s1->heightsec].floorheight) ||
        (t1->z >= sectors[s1->heightsec].ceilingheight &&
         t2->z + t1->height <= sectors[s1->heightsec].ceilingheight)))
      ||
      (s2->heightsec != -1 &&
       ((t2->z + t2->height <= sectors[s2->heightsec].floorheight &&
         t1->z >= sectors[s2->heightsec].floorheight) ||
        (t2->z >= sectors[s2->heightsec].ceilingheight &&
         t1->z + t2->height <= sectors[s2->heightsec].ceilingheight))))
    return false;

  /* killough 11/98: shortcut for melee situations
   * same subsector? obviously visible
   * cph - compatibility optioned for demo sync, cf HR06-UV.LMP */
  if ((t1->subsector == t2->subsector) &&
      (compatibility_level >= mbf_compatibility))
    return true;

  // An unobstructed LOS is possible.
  // Now look from eyes of t1 to any part of t2.

  validcount++;

  los.topslope = (los.bottomslope = t2->z - (los.sightzstart =
                                             t1->z + t1->height -
                                             (t1->height>>2))) + t2->height;
  los.strace.dx = (los.t2x = t2->x) - (los.strace.x = t1->x);
  los.strace.dy = (los.t2y = t2->y) - (los.strace.y = t1->y);

  if (t1->x > t2->x)
    los.bbox[BOXRIGHT] = t1->x, los.bbox[BOXLEFT] = t2->x;
  else
    los.bbox[BOXRIGHT] = t2->x, los.bbox[BOXLEFT] = t1->x;

  if (t1->y > t2->y)
    los.bbox[BOXTOP] = t1->y, los.bbox[BOXBOTTOM] = t2->y;
  else
    los.bbox[BOXTOP] = t2->y, los.bbox[BOXBOTTOM] = t1->y;

  /* cph - calculate min and max z of the potential line of sight
   * For old demos, we disable this optimisation by setting them to
   * the extremes */
  if (compatibility_level == lxdoom_1_compatibility || prboom_comp[PC_FORCE_LXDOOM_DEMO_COMPATIBILITY].state)
  {
    if (los.sightzstart < t2->z) {
      los.maxz = t2->z + t2->height; los.minz = los.sightzstart;
    } else if (los.sightzstart > t2->z + t2->height) {
      los.maxz = los.sightzstart; los.minz = t2->z;
    } else {
      los.maxz = t2->z + t2->height; los.minz = t2->z;
    }
  }
  else
  {
    los.maxz = INT_MAX; los.minz = INT_MIN;
  }

  // the head node is the last node output
  return P_CrossBSPNode(numnodes-1);
}
