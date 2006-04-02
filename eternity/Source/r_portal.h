// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 Stephen McGranahan
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
//      Creating, managing and rendering portals.
//      SoM created 12/8/03
//
//-----------------------------------------------------------------------------

#ifndef R_PORTALS_H
#define R_PORTALS_H

typedef enum
{
   R_NONE,
   R_SKYBOX,
   R_ANCHORED,
   R_HORIZON,
   R_PLANE,
} rportaltype_e;

typedef struct
{
   mobj_t    *mobj;
   fixed_t   deltax, deltay, deltaz;
} cameraportal_t;

typedef struct
{
   short *floorpic, *ceilingpic;
   fixed_t *floorz, *ceilingz;
   short *floorlight, *ceilinglight;
   fixed_t *floorxoff, *flooryoff;
   fixed_t *ceilingxoff, *ceilingyoff;
} horizonportal_t;

typedef struct
{
   short *pic;
   fixed_t *delta;
   short *lightlevel;
   fixed_t *xoff, *yoff;
} skyplaneportal_t;

typedef struct rportal_s
{
   rportaltype_e type;

   union
   {
      cameraportal_t   camera;
      horizonportal_t  horizon;
      skyplaneportal_t plane;
   } data;

   short top[MAX_SCREENWIDTH];
   short bottom[MAX_SCREENWIDTH];
   int minx, maxx;

   fixed_t  vx, vy, vz;

   struct rportal_s *next, *child;
} rportal_t;

rportal_t *R_GetSkyBoxPortal(mobj_t *camera);
rportal_t *R_GetAnchoredPortal(fixed_t deltax, fixed_t deltay, fixed_t deltaz);
rportal_t *R_GetHorizonPortal(short *floorpic, short *ceilingpic, fixed_t *floorz, fixed_t *ceilingz, short *floorlight, short *ceilinglight, fixed_t *floorxoff, fixed_t *flooryoff, fixed_t *ceilingxoff, fixed_t *ceilingyoff);
rportal_t *R_GetPlanePortal(short *pic, fixed_t *delta, short *lightlevel, fixed_t *xoff, fixed_t *yoff);

void R_PortalAdd(rportal_t *portal, int x, int ytop, int ybottom);

void R_ClearPortals(void);
void R_RenderPortals(void);

// SoM 3/14/2004: flag if we are rendering portals.
extern boolean portalrender;
#endif

//----------------------------------------------------------------------------
//
// $Log: r_portals.h,v $
//
//----------------------------------------------------------------------------
