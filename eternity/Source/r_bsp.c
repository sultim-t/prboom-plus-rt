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
//      BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_bsp.c,v 1.17 1998/05/03 22:47:33 killough Exp $";

#include "doomstat.h"
#include "m_bbox.h"
#include "i_system.h"
#include "r_main.h"
#include "r_segs.h"
#include "r_plane.h"
#include "r_things.h"

seg_t     *curline;
side_t    *sidedef;
line_t    *linedef;
sector_t  *frontsector;
sector_t  *backsector;
drawseg_t *ds_p;

// killough 4/7/98: indicates doors closed wrt automap bugfix:
int      doorclosed;

// killough: New code which removes 2s linedef limit
drawseg_t *drawsegs;
unsigned  maxdrawsegs;
// drawseg_t drawsegs[MAXDRAWSEGS];       // old code -- killough

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs(void)
{
   ds_p = drawsegs;
}

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
// 1/11/98 killough: Since a type "short" is sufficient, we
// should use it, since smaller arrays fit better in cache.
//

typedef struct 
{
  short first, last;      // killough
} cliprange_t;

// 1/11/98: Lee Killough
//
// This fixes many strange venetian blinds crashes, which occurred when a scan
// line had too many "posts" of alternating non-transparent and transparent
// regions. Using a doubly-linked list to represent the posts is one way to
// do it, but it has increased overhead and poor spatial locality, which hurts
// cache performance on modern machines. Since the maximum number of posts
// theoretically possible is a function of screen width, a static limit is
// okay in this case. It used to be 32, which was way too small.
//
// This limit was frequently mistaken for the visplane limit in some Doom
// editing FAQs, where visplanes were said to "double" if a pillar or other
// object split the view's space into two pieces horizontally. That did not
// have anything to do with visplanes, but it had everything to do with these
// clip posts.

#define MAXSEGS (MAX_SCREENWIDTH/2+1)   /* killough 1/11/98, 2/8/98 */

// newend is one past the last valid seg
static cliprange_t *newend;
static cliprange_t solidsegs[MAXSEGS];

//
// R_ClipSolidWallSegment
//
// Handles solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
//
static void R_ClipSolidWallSegment(int first, int last)
{
   cliprange_t *next, *start;
   
   // Find the first range that touches the range
   // (adjacent pixels are touching).
   
   start = solidsegs;
   while(start->last < first - 1)
      ++start;

   if(first < start->first)
   {
      if(last < start->first - 1)
      {
         // Post is entirely visible (above start), so insert a new clippost.
         R_StoreWallRange(first, last);
         
         // 1/11/98 killough: performance tuning using fast memmove
         memmove(start + 1, start, (++newend - start) * sizeof(*start));
         start->first = first;
         start->last = last;
         return;
      }

      // There is a fragment above *start.
      R_StoreWallRange(first, start->first - 1);
      
      // Now adjust the clip size.
      start->first = first;
   }

   // Bottom contained in start?
   if(last <= start->last)
      return;

   next = start;
   while(last >= (next + 1)->first - 1)
   {      // There is a fragment between two posts.
      R_StoreWallRange(next->last + 1, (next + 1)->first - 1);
      ++next;
      if(last <= next->last)
      {  
         // Bottom is contained in next. Adjust the clip size.
         start->last = next->last;
         goto crunch;
      }
   }

   // There is a fragment after *next.
   R_StoreWallRange(next->last + 1, last);
   
   // Adjust the clip size.
   start->last = last;
   
   // Remove start+1 to next from the clip list,
   // because start now covers their area.
crunch:
   if(next == start) // Post just extended past the bottom of one post.
      return;
   
   while(next++ != newend)      // Remove a post.
      *++start = *next;
   
   newend = start + 1;
}

//
// R_ClipPassWallSegment
//
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
static void R_ClipPassWallSegment(int first, int last)
{
   cliprange_t *start = solidsegs;
   
   // Find the first range that touches the range
   //  (adjacent pixels are touching).
   while(start->last < first - 1)
      ++start;

   if(first < start->first)
   {
      if(last < start->first - 1)
      {
         // Post is entirely visible (above start).
         R_StoreWallRange(first, last);
         return;
      }

      // There is a fragment above *start.
      R_StoreWallRange(first, start->first - 1);
   }

   // Bottom contained in start?
   if(last <= start->last)
      return;

   while(last >= (start + 1)->first - 1)
   {
      // There is a fragment between two posts.
      R_StoreWallRange(start->last + 1, (start + 1)->first - 1);
      ++start;
      
      if(last <= start->last)
         return;
   }
   
   // There is a fragment after *next.
   R_StoreWallRange(start->last + 1, last);
}

//
// R_ClearClipSegs
//
void R_ClearClipSegs(void)
{
   solidsegs[0].first = -0x7fff; // ffff;    new short limit --  killough
   solidsegs[0].last = -1;
   solidsegs[1].first = viewwidth;
   solidsegs[1].last = 0x7fff; // ffff;      new short limit --  killough
   newend = solidsegs+2;
}

#ifdef R_PORTALS
boolean R_SetupPortalClipsegs(short *top, short *bottom)
{
   int i = 0;
   cliprange_t *seg;
   
   R_ClearClipSegs();
   
   // extend first solidseg to one column left of first open post
   while(i < viewwidth && top[i] + 1 >= bottom[i]) 
      ++i;
   
   // first open post found, set last closed post to last closed post (i - 1);
   solidsegs[0].last = i - 1;
   
   // the entire thing is closed?
   if(i == viewwidth)
      return false;
   
   seg = solidsegs + 1;
   
   while(1)
   {
      //find the first closed post.
      while(i < viewwidth && top[i] + 1 < bottom[i]) 
         ++i;
      
      if(i == viewwidth)
         goto endopen;
      
      // set the solidsegs
      seg->first = i;
      
      // find the first open post
      while(i < viewwidth && top[i] + 1 >= bottom[i]) i++;
      if(i == viewwidth)
         goto endclosed;
      
      seg->last = i - 1;
      seg++;
   }
   
endopen:
   seg->first = viewwidth;
   seg->last = 0x7fff;
   newend = seg + 1;
   return true;
   
endclosed:
   seg->last = 0x7fff;
   newend = seg + 1;
   return true;
}
#endif

// killough 1/18/98 -- This function is used to fix the automap bug which
// showed lines behind closed doors simply because the door had a dropoff.
//
// It assumes that Doom has already ruled out a door being closed because
// of front-back closure (e.g. front floor is taller than back ceiling).

int R_DoorClosed(void)
{
   return

     // if door is closed because back is shut:
     backsector->ceilingheight <= backsector->floorheight

     // preserve a kind of transparent door/lift special effect:
     && (backsector->ceilingheight >= frontsector->ceilingheight ||
      curline->sidedef->toptexture)

     && (backsector->floorheight <= frontsector->floorheight ||
      curline->sidedef->bottomtexture)

     // properly render skies (consider door "open" if both ceilings are sky):
     && ((backsector->ceilingpic != skyflatnum && 
          backsector->ceilingpic != sky2flatnum) ||
         (frontsector->ceilingpic != skyflatnum &&
          frontsector->ceilingpic != sky2flatnum));
}

//
// killough 3/7/98: Hack floor/ceiling heights for deep water etc.
//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
// killough 4/11/98, 4/13/98: fix bugs, add 'back' parameter
//

extern camera_t *camera; // haleyjd

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
                     int *floorlightlevel, int *ceilinglightlevel,
                     boolean back)
{
   if(floorlightlevel)
   {
      *floorlightlevel = sec->floorlightsec == -1 ?
         sec->lightlevel : sectors[sec->floorlightsec].lightlevel;
   }

   if(ceilinglightlevel)
   {
      *ceilinglightlevel = sec->ceilinglightsec == -1 ? // killough 4/11/98
         sec->lightlevel : sectors[sec->ceilinglightsec].lightlevel;
   }


   if(sec->heightsec != -1)
   {
      int underwater; // haleyjd: restructured
      
      int heightsec = -1;
      
      const sector_t *s = &sectors[sec->heightsec];
      
      // haleyjd: Lee assumed that only players would ever be
      // involved in LOS calculations for deep water -- must be
      // fixed for cameras -- thanks to Julian for finding the
      // solution to this old problem!

      heightsec = camera ? camera->heightsec
                         : viewplayer->mo->subsector->sector->heightsec;
            
      underwater = 
	 (heightsec!=-1 && viewz<=sectors[heightsec].floorheight);

      // Replace sector being drawn, with a copy to be hacked
      *tempsec = *sec;

      // Replace floor and ceiling height with other sector's heights.
      tempsec->floorheight   = s->floorheight;
      tempsec->ceilingheight = s->ceilingheight;

      // killough 11/98: prevent sudden light changes from non-water sectors:
      if(underwater && (tempsec->floorheight   = sec->floorheight,
                        tempsec->ceilingheight = s->floorheight-1, !back))
      {
         // head-below-floor hack
         tempsec->floorpic    = s->floorpic;
         tempsec->floor_xoffs = s->floor_xoffs;
         tempsec->floor_yoffs = s->floor_yoffs;

         // haleyjd 03/13/05: removed redundant if(underwater) check
         if(s->ceilingpic == skyflatnum || s->ceilingpic == sky2flatnum)
         {
            tempsec->floorheight   = tempsec->ceilingheight+1;
            tempsec->ceilingpic    = tempsec->floorpic;
            tempsec->ceiling_xoffs = tempsec->floor_xoffs;
            tempsec->ceiling_yoffs = tempsec->floor_yoffs;
         }
         else
         {
            tempsec->ceilingpic    = s->ceilingpic;
            tempsec->ceiling_xoffs = s->ceiling_xoffs;
            tempsec->ceiling_yoffs = s->ceiling_yoffs;
         }

         tempsec->lightlevel  = s->lightlevel;
         
         if(floorlightlevel)
         {
            *floorlightlevel = s->floorlightsec == -1 ? s->lightlevel :
               sectors[s->floorlightsec].lightlevel; // killough 3/16/98
         }

         if (ceilinglightlevel)
         {
            *ceilinglightlevel = s->ceilinglightsec == -1 ? s->lightlevel :
               sectors[s->ceilinglightsec].lightlevel; // killough 4/11/98
         }
      }
      else if(heightsec != -1 && 
              viewz >= sectors[heightsec].ceilingheight &&
              sec->ceilingheight > s->ceilingheight)
      {   
         // Above-ceiling hack
         tempsec->ceilingheight = s->ceilingheight;
         tempsec->floorheight   = s->ceilingheight + 1;

         tempsec->floorpic    = tempsec->ceilingpic    = s->ceilingpic;
         tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
         tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;

         if(s->floorpic != skyflatnum && s->floorpic != sky2flatnum)
         {
            tempsec->ceilingheight = sec->ceilingheight;
            tempsec->floorpic      = s->floorpic;
            tempsec->floor_xoffs   = s->floor_xoffs;
            tempsec->floor_yoffs   = s->floor_yoffs;
         }
         
         tempsec->lightlevel  = s->lightlevel;
         
         if(floorlightlevel)
         {
            *floorlightlevel = s->floorlightsec == -1 ? s->lightlevel :
               sectors[s->floorlightsec].lightlevel; // killough 3/16/98
         }

         if(ceilinglightlevel)
         {
            *ceilinglightlevel = s->ceilinglightsec == -1 ? s->lightlevel :
               sectors[s->ceilinglightsec].lightlevel; // killough 4/11/98
         }
      }
      sec = tempsec;               // Use other sector
   }
   return sec;
}

//
// R_AddLine
//
// Clips the given segment
// and adds any visible pieces to the line list.
//
static void R_AddLine(seg_t *line)
{
   int      x1;
   int      x2;
   angle_t  angle1;
   angle_t  angle2;
   angle_t  span;
   angle_t  tspan;
   static sector_t tempsec;     // killough 3/8/98: ceiling/water hack

   curline = line;
   
   angle1 = R_PointToAngle(line->v1->x, line->v1->y);
   angle2 = R_PointToAngle(line->v2->x, line->v2->y);

   // Clip to view edges.
   span = angle1 - angle2;
   
   // Back side, i.e. backface culling
   if(span >= ANG180)
      return;
   
   // Global angle needed by segcalc.
   rw_angle1 = angle1;
   angle1 -= viewangle;
   angle2 -= viewangle;

   tspan = angle1 + clipangle;
   if(tspan > 2 * clipangle)
   {
      tspan -= (2 * clipangle);
      
      // Totally off the left edge?
      if(tspan >= span)
         return;
      
      angle1 = clipangle;
   }

   tspan = clipangle - angle2;
   if(tspan > 2 * clipangle)
   {
      tspan -= (2 * clipangle);
      
      // Totally off the left edge?
      if(tspan >= span)
         return;
      angle2 = -clipangle;
   }

   // The seg is in the view range,
   // but not necessarily visible.
   
   angle1 = (angle1 + ANG90) >> ANGLETOFINESHIFT;
   angle2 = (angle2 + ANG90) >> ANGLETOFINESHIFT;
   
   // killough 1/31/98: Here is where "slime trails" can SOMETIMES occur:
   // SoM: This is also where all the line rendering bugs come from.
   // bouncy wall bugs are caused by the left vertex being WAAAY of the screen and the LUT
   // at that point becomes unstable and highly inaccurate. 
   x1 = viewangletox[angle1];
   x2 = viewangletox[angle2];

   // Does not cross a pixel?
   if(x1 >= x2)       // killough 1/31/98 -- change == to >= for robustness
      return;

   backsector = line->backsector;
   
   // Single sided line?
   if(!backsector)
      goto clipsolid;

   // killough 3/8/98, 4/4/98: hack for invisible ceilings / deep water
   backsector = R_FakeFlat(backsector, &tempsec, NULL, NULL, true);
   
   doorclosed = 0;       // killough 4/16/98
   
   // Closed door.

   if(backsector->ceilingheight <= frontsector->floorheight || 
      backsector->floorheight   >= frontsector->ceilingheight)
      goto clipsolid;

   // This fixes the automap floor height bug -- killough 1/18/98:
   // killough 4/7/98: optimize: save result in doorclosed for use in r_segs.c
   if((doorclosed = R_DoorClosed()))
      goto clipsolid;

   // Window.
   if(backsector->ceilingheight != frontsector->ceilingheight || 
      backsector->floorheight   != frontsector->floorheight)
      goto clippass;

   // Reject empty lines used for triggers
   //  and special events.
   // Identical floor and ceiling on both sides,
   // identical light levels on both sides,
   // and no middle texture.
   if(backsector->ceilingpic == frontsector->ceilingpic 
      && backsector->floorpic == frontsector->floorpic
      && backsector->lightlevel == frontsector->lightlevel 
      && curline->sidedef->midtexture == 0
      
      // killough 3/7/98: Take flats offsets into account:
      && backsector->floor_xoffs == frontsector->floor_xoffs
      && backsector->floor_yoffs == frontsector->floor_yoffs
      && backsector->ceiling_xoffs == frontsector->ceiling_xoffs
      && backsector->ceiling_yoffs == frontsector->ceiling_yoffs
      
      // killough 4/16/98: consider altered lighting
      && backsector->floorlightsec == frontsector->floorlightsec
      && backsector->ceilinglightsec == frontsector->ceilinglightsec
      
      // sf: coloured lighting
      && backsector->heightsec == frontsector->heightsec
#ifdef R_PORTALS
      // SoM 12/10/03: PORTALS
      && backsector->c_portal == frontsector->c_portal
      && backsector->f_portal == frontsector->f_portal
#endif
      )
      return;

clippass:
#ifdef R_PORTALS
   x2 -= 1;
   // SoM 3/14/2005: If we are rendering a portal area, check the seg against the portal view
   // and possibly reject it.
   if(portalrender && !R_ClipSeg(&x1, &x2))
      return;
   R_ClipPassWallSegment(x1, x2);
#else
   R_ClipPassWallSegment(x1, x2 - 1);
#endif
   return;
   
clipsolid:
#ifdef R_PORTALS
   x2 -= 1;
   // SoM 3/14/2005: If we are rendering a portal area, check the seg against the portal view
   // and possibly reject it.
   if(portalrender && !R_ClipSeg(&x1, &x2))
      return;
   R_ClipSolidWallSegment(x1, x2);
#else
   R_ClipSolidWallSegment(x1, x2 - 1);
#endif
}

static const int checkcoord[12][4] = // killough -- static const
{
   {3,0,2,1},
   {3,0,2,0},
   {3,1,2,0},
   {0},
   {2,0,2,1},
   {0,0,0,0},
   {3,1,3,0},
   {0},
   {2,0,3,1},
   {2,1,3,1},
   {2,1,3,0}
};

//
// R_CheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
static boolean R_CheckBBox(fixed_t *bspcoord) // killough 1/28/98: static
{
   int     boxpos, boxx, boxy;
   fixed_t x1, x2, y1, y2;
   angle_t angle1, angle2, span, tspan;
   int     sx1, sx2;
   cliprange_t *start;

   // Find the corners of the box
   // that define the edges from current viewpoint.
   boxx = viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT ] ? 1 : 2;
   boxy = viewy >= bspcoord[BOXTOP ] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 1 : 2;

   boxpos = (boxy << 2) + boxx;
   if(boxpos == 5)
      return true;

   x1 = bspcoord[checkcoord[boxpos][0]];
   y1 = bspcoord[checkcoord[boxpos][1]];
   x2 = bspcoord[checkcoord[boxpos][2]];
   y2 = bspcoord[checkcoord[boxpos][3]];

   // check clip list for an open space
   angle1 = R_PointToAngle (x1, y1) - viewangle;
   angle2 = R_PointToAngle (x2, y2) - viewangle;
   
   span = angle1 - angle2;
   
   // Sitting on a line?
   if(span >= ANG180)
      return true;

   tspan = angle1 + clipangle;
   if(tspan > 2 * clipangle)
   {
      tspan -= (2 * clipangle);
      
      // Totally off the left edge?
      if(tspan >= span)
         return false;
      
      angle1 = clipangle;
   }

   tspan = clipangle - angle2;
   if(tspan > 2 * clipangle)
   {
      tspan -= (2 * clipangle);
      
      // Totally off the left edge?
      if(tspan >= span)
         return false;
      
      angle2 = -clipangle;
   }

   // Find the first clippost
   //  that touches the source post
   //  (adjacent pixels are touching).
   angle1 = (angle1 + ANG90) >> ANGLETOFINESHIFT;
   angle2 = (angle2 + ANG90) >> ANGLETOFINESHIFT;
   sx1 = viewangletox[angle1];
   sx2 = viewangletox[angle2];
   
   // Does not cross a pixel.
   if(sx1 == sx2)
      return false;
   --sx2;

   start = solidsegs;
   while(start->last < sx2)
      ++start;
   
   if(sx1 >= start->first && sx2 <= start->last)
      return false;      // The clippost contains the new span.
   
   return true;
}

//
// R_Subsector
//
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
// killough 1/31/98 -- made static, polished
//
static void R_Subsector(int num)
{
   int         count;
   seg_t       *line;
   subsector_t *sub;
   sector_t    tempsec;              // killough 3/7/98: deep water hack
   int         floorlightlevel;      // killough 3/16/98: set floor lightlevel
   int         ceilinglightlevel;    // killough 4/11/98
   
#ifdef RANGECHECK
   if(num >= numsubsectors)
      I_Error("R_Subsector: ss %i with numss = %i", num, numsubsectors);
#endif

   sub = &subsectors[num];
   frontsector = sub->sector;
   count = sub->numlines;
   line = &segs[sub->firstline];
   
   R_SectorColormap(frontsector);
   
   // killough 3/8/98, 4/4/98: Deep water / fake ceiling effect
   frontsector = R_FakeFlat(frontsector, &tempsec, &floorlightlevel,
                            &ceilinglightlevel, false);   // killough 4/11/98

   // killough 3/7/98: Add (x,y) offsets to flats, add deep water check
   // killough 3/16/98: add floorlightlevel
   // killough 10/98: add support for skies transferred from sidedefs

   floorplane = frontsector->floorheight < viewz || // killough 3/7/98
     (frontsector->heightsec != -1 &&
      (sectors[frontsector->heightsec].ceilingpic == skyflatnum ||
       sectors[frontsector->heightsec].ceilingpic == sky2flatnum)) ?
     R_FindPlane(frontsector->floorheight, 
                 (frontsector->floorpic == skyflatnum ||
                  frontsector->floorpic == sky2flatnum) &&  // kilough 10/98
                 frontsector->sky & PL_SKYFLAT ? frontsector->sky :
                 frontsector->floorpic,
                 floorlightlevel,                // killough 3/16/98
                 frontsector->floor_xoffs,       // killough 3/7/98
                 frontsector->floor_yoffs) : NULL;

   ceilingplane = frontsector->ceilingheight > viewz ||
     (frontsector->ceilingpic == skyflatnum ||
      frontsector->ceilingpic == sky2flatnum) ||
     (frontsector->heightsec != -1 &&
      (sectors[frontsector->heightsec].floorpic == skyflatnum ||
       sectors[frontsector->heightsec].floorpic == sky2flatnum)) ?
     R_FindPlane(frontsector->ceilingheight,     // killough 3/8/98
                 (frontsector->ceilingpic == skyflatnum ||
                  frontsector->ceilingpic == sky2flatnum) &&  // kilough 10/98
                 frontsector->sky & PL_SKYFLAT ? frontsector->sky :
                 frontsector->ceilingpic,
                 ceilinglightlevel,              // killough 4/11/98
                 frontsector->ceiling_xoffs,     // killough 3/7/98
                 frontsector->ceiling_yoffs) : NULL;
  
   // killough 9/18/98: Fix underwater slowdown, by passing real sector 
   // instead of fake one. Improve sprite lighting by basing sprite
   // lightlevels on floor & ceiling lightlevels in the surrounding area.
   //
   // 10/98 killough:
   //
   // NOTE: TeamTNT fixed this bug incorrectly, messing up sprite lighting!!!
   // That is part of the 242 effect!!!  If you simply pass sub->sector to
   // the old code you will not get correct lighting for underwater sprites!!!
   // Either you must pass the fake sector and handle validcount here, on the
   // real sector, or you must account for the lighting in some other way, 
   // like passing it as an argument.

   R_AddSprites(sub->sector, (floorlightlevel+ceilinglightlevel)/2);

   while(count--)
      R_AddLine(line++);
}

//
// R_RenderBSPNode
//
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
//
// killough 5/2/98: reformatted, removed tail recursion
//
void R_RenderBSPNode(int bspnum)
{
   while(!(bspnum & NF_SUBSECTOR))  // Found a subsector?
   {
      node_t *bsp = &nodes[bspnum];
      
      // Decide which side the view point is on.
      int side = R_PointOnSide(viewx, viewy, bsp);
      
      // Recursively divide front space.
      R_RenderBSPNode(bsp->children[side]);
      
      // Possibly divide back space.
      
      if(!R_CheckBBox(bsp->bbox[side^=1]))
         return;
      
      bspnum = bsp->children[side];
   }
   R_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

//----------------------------------------------------------------------------
//
// $Log: r_bsp.c,v $
// Revision 1.17  1998/05/03  22:47:33  killough
// beautification
//
// Revision 1.16  1998/04/23  12:19:50  killough
// Testing untabify feature
//
// Revision 1.15  1998/04/17  10:22:22  killough
// Fix 213, 261 (floor/ceiling lighting)
//
// Revision 1.14  1998/04/14  08:15:55  killough
// Fix light levels on 2s textures
//
// Revision 1.13  1998/04/13  09:44:40  killough
// Fix head-over ceiling effects
//
// Revision 1.12  1998/04/12  01:57:18  killough
// Fix deep water effects
//
// Revision 1.11  1998/04/07  06:41:14  killough
// Fix disappearing things, AASHITTY sky wall HOM, remove obsolete HOM detector
//
// Revision 1.10  1998/04/06  04:37:48  killough
// Make deep water / fake ceiling handling more consistent
//
// Revision 1.9  1998/03/28  18:14:27  killough
// Improve underwater support
//
// Revision 1.8  1998/03/16  12:40:11  killough
// Fix underwater effects, floor light levels from other sectors
//
// Revision 1.7  1998/03/09  07:22:41  killough
// Add primitive underwater support
//
// Revision 1.6  1998/03/02  11:50:53  killough
// Add support for scrolling flats
//
// Revision 1.5  1998/02/17  06:21:57  killough
// Change commented-out code to #if'ed out code
//
// Revision 1.4  1998/02/09  03:14:55  killough
// Make HOM detector under control of TNTHOM cheat
//
// Revision 1.3  1998/02/02  13:31:23  killough
// Performance tuning, add HOM detector
//
// Revision 1.2  1998/01/26  19:24:36  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:02  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
