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
 *      Rendering main loop and setup functions,
 *       utility functions (BSP, geometry, trigonometry).
 *      See tables.c, too.
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef USE_SDL
#include "SDL.h"
#endif
#include "doomstat.h"
#include "d_net.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_things.h"
#include "r_plane.h"
#include "r_bsp.h"
#include "r_draw.h"
#include "m_bbox.h"
#include "r_sky.h"
#include "v_video.h"
#include "lprintf.h"
#include "st_stuff.h"
#include "i_main.h"
#include "i_system.h"
#include "g_game.h"
#include "r_demo.h"
#include "r_fps.h"
#include <math.h>
#include "e6y.h"//e6y

int r_frame_count;

//e6y
render_precise_t render_precise;
const char *render_precises[] = {"Speed","Quality"};

int r_have_internal_hires = false;

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// killough: viewangleoffset is a legacy from the pre-v1.2 days, when Doom
// had Left/Mid/Right viewing. +/-ANG90 offsets were placed here on each
// node, by d_net.c, to set up a L/M/R session.

int viewangleoffset;
int validcount = 1;         // increment every time a check is made
const lighttable_t *fixedcolormap;
int      centerx, centery;
// e6y: wide-res
int wide_centerx;
int wide_ratio;
int wide_offsetx;
const base_ratio_t BaseRatioSizes[5] =
{
	{  960, 600, 0, 48 ,      RMUL*1.333333f }, // 4:3
	{ 1280, 450, 0, 48*3/4,   RMUL*1.777777f }, // 16:9
	{ 1152, 500, 0, 48*5/6,   RMUL*1.6f      }, // 16:10
  {  960, 600, 0, 48,       RMUL*1.333333f },
	{  960, 640, (int)(6.5*FRACUNIT), 48*15/16, RMUL*1.2f } // 5:4
};

fixed_t  centerxfrac, centeryfrac;
fixed_t  viewheightfrac; //e6y: for correct cliping of things
fixed_t  projection;
// proff 11/06/98: Added for high-res
fixed_t  projectiony;
fixed_t  viewx, viewy, viewz;
angle_t  viewangle;
fixed_t  viewcos, viewsin;
player_t *viewplayer;
// e6y: Added for more precise flats drawing
float viewfocratio;

extern lighttable_t **walllights;

static mobj_t *oviewer;

//
// precalculated math tables
//

angle_t clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.

int viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.

// e6y: resolution limitation is removed
angle_t *xtoviewangle;   // killough 2/8/98

// killough 3/20/98: Support dynamic colormaps, e.g. deep water
// killough 4/4/98: support dynamic number of them as well

//e6y
int scalelight_offset[LIGHTLEVELS][MAXLIGHTSCALE];

int numcolormaps;
const lighttable_t *(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];
const lighttable_t *(*zlight)[MAXLIGHTZ];
const lighttable_t *fullcolormap;
const lighttable_t **colormaps;

// killough 3/20/98, 4/4/98: end dynamic colormaps

//e6y: for Boom colormaps in OpenGL mode
dboolean use_boom_cm;
int boom_cm;         // current colormap
int last_boom_cm=-1; // previous colormap
int last_fixedcolormap=-1;
int frame_fixedcolormap = 0;

int extralight;                           // bumped light from gun blasts

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//

PUREFUNC int R_PointOnSide(fixed_t x, fixed_t y, const node_t *node)
{
  if (!node->dx)
    return x <= node->x ? node->dy > 0 : node->dy < 0;

  if (!node->dy)
    return y <= node->y ? node->dx < 0 : node->dx > 0;

  x -= node->x;
  y -= node->y;

  // Try to quickly decide by looking at sign bits.
  if ((node->dy ^ node->dx ^ x ^ y) < 0)
    return (node->dy ^ x) < 0;  // (left is negative)
  return FixedMul(y, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, x);
}

// killough 5/2/98: reformatted

PUREFUNC int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t *line)
{
  fixed_t lx = line->v1->x;
  fixed_t ly = line->v1->y;
  fixed_t ldx = line->v2->x - lx;
  fixed_t ldy = line->v2->y - ly;

  if (!ldx)
    return x <= lx ? ldy > 0 : ldy < 0;

  if (!ldy)
    return y <= ly ? ldx < 0 : ldx > 0;

  x -= lx;
  y -= ly;

  // Try to quickly decide by looking at sign bits.
  if ((ldy ^ ldx ^ x ^ y) < 0)
    return (ldy ^ x) < 0;          // (left is negative)
  return FixedMul(y, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, x);
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
  return (y -= viewy, (x -= viewx) || y) ?
    x >= 0 ?
      y >= 0 ?
        (x > y) ? tantoangle[SlopeDiv(y,x)] :                      // octant 0
                ANG90-1-tantoangle[SlopeDiv(x,y)] :                // octant 1
        x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                // octant 8
                       ANG270+tantoangle[SlopeDiv(x,y)] :          // octant 7
      y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv(y,x)] : // octant 3
                            ANG90 + tantoangle[SlopeDiv(x,y)] :    // octant 2
        (x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv(y,x)] :  // octant 4
                              ANG270-1-tantoangle[SlopeDiv(x,y)] : // octant 5
    0;
}

angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
  return (y -= viewy, (x -= viewx) || y) ?
    x >= 0 ?
      y >= 0 ?
        (x > y) ? tantoangle[SlopeDiv(y,x)] :                      // octant 0
                ANG90-1-tantoangle[SlopeDiv(x,y)] :                // octant 1
        x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                // octant 8
                       ANG270+tantoangle[SlopeDiv(x,y)] :          // octant 7
      y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv(y,x)] : // octant 3
                            ANG90 + tantoangle[SlopeDiv(x,y)] :    // octant 2
        (x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv(y,x)] :  // octant 4
                              ANG270-1-tantoangle[SlopeDiv(x,y)] : // octant 5
    0;
}

// e6y
// The precision of the code above is abysmal so use the CRT atan2 function instead!

// FIXME - use of this function should be disabled on architectures with
// poor floating point support! Imagine how slow this would be on ARM, say.

angle_t R_PointToAngleEx(fixed_t x, fixed_t y)
{
  static int old_y_viewy;
  static int old_x_viewx;
  static int old_result;

  int y_viewy = y - viewy;
  int x_viewx = x - viewx;

  if (!render_precise)
  {
    // e6y: here is where "slime trails" can SOMETIMES occur
#ifdef GL_DOOM
    if (V_GetMode() != VID_MODEGL)
#endif
      if (y_viewy < INT_MAX/4 && x_viewx < INT_MAX/4
          && y_viewy > -INT_MAX/4 && x_viewx > -INT_MAX/4)
        return R_PointToAngle(x, y);
  }

  if (old_y_viewy != y_viewy || old_x_viewx != x_viewx)
  {
    old_y_viewy = y_viewy;
    old_x_viewx = x_viewx;

    old_result = (int)((float)atan2(y_viewy, x_viewx) * (ANG180/M_PI));
  }
  return old_result;
}

// e6y: caching
angle_t R_GetVertexViewAngle(vertex_t *v)
{
  if (v->angletime != r_frame_count)
  {
    v->angletime = r_frame_count;
    v->viewangle = R_PointToAngleEx(v->x, v->y);
  }
  return v->viewangle;
}


//
// R_InitTextureMapping
//
// killough 5/2/98: reformatted

static void R_InitTextureMapping (void)
{
  register int i,x;
  fixed_t focallength;
  int fov = FIELDOFVIEW;

  // For widescreen displays, increase the FOV so that the middle part of the
  // screen that would be visible on a 4:3 display has the requested FOV.
  if (wide_centerx != centerx)
  { // wide_centerx is what centerx would be if the display was not widescreen
    fov = (int)(atan((double)centerx * tan((double)fov * M_PI / FINEANGLES) / (double)wide_centerx) * FINEANGLES / M_PI);
    if (fov > 130 * FINEANGLES / 360)
      fov = 130 * FINEANGLES / 360;
  }

  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  //
  // Calc focallength
  //  so FIELDOFVIEW angles covers SCREENWIDTH.

  focallength = FixedDiv(centerxfrac, finetangent[FINEANGLES/4 + fov/2]);

  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      int t;
      if (finetangent[i] > FRACUNIT*2)
        t = -1;
      else
        if (finetangent[i] < -FRACUNIT*2)
          t = viewwidth+1;
      else
        {
          t = FixedMul(finetangent[i], focallength);
          t = (centerxfrac - t + FRACUNIT-1) >> FRACBITS;
          if (t < -1)
            t = -1;
          else
            if (t > viewwidth+1)
              t = viewwidth+1;
        }
      viewangletox[i] = t;
    }

  // Scan viewangletox[] to generate xtoviewangle[]:
  //  xtoviewangle will give the smallest view angle
  //  that maps to x.

  for (x=0; x<=viewwidth; x++)
    {
      for (i=0; viewangletox[i] > x; i++)
        ;
      xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

  // Take out the fencepost cases from viewangletox.
  for (i=0; i<FINEANGLES/2; i++)
    if (viewangletox[i] == -1)
      viewangletox[i] = 0;
    else
      if (viewangletox[i] == viewwidth+1)
        viewangletox[i] = viewwidth;

  clipangle = xtoviewangle[0];
}

//
// R_InitLightTables
//

#define DISTMAP 2

static void R_InitLightTables (void)
{
  int i;

  // killough 4/4/98: dynamic colormaps
  c_zlight = malloc(sizeof(*c_zlight) * numcolormaps);

  // Calculate the light levels to use
  //  for each level / distance combination.
  for (i=0; i< LIGHTLEVELS; i++)
    {
      int j, startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0; j<MAXLIGHTZ; j++)
        {
    // CPhipps - use 320 here instead of SCREENWIDTH, otherwise hires is
    //           brighter than normal res
          int scale = FixedDiv ((320/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
          int t, level = startmap - (scale >>= LIGHTSCALESHIFT)/DISTMAP;

          if (level < 0)
            level = 0;
          else
            if (level >= NUMCOLORMAPS)
              level = NUMCOLORMAPS-1;

          // killough 3/20/98: Initialize multiple colormaps
          level *= 256;
          for (t=0; t<numcolormaps; t++)         // killough 4/4/98
            c_zlight[t][i][j] = colormaps[t] + level;
        }
    }
}

//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//

dboolean setsizeneeded;
int     setblocks;

void R_SetViewSize(int blocks)
{
  setsizeneeded = true;
  setblocks = blocks;
}

void R_SetupViewScaling(void)
{
  int i;
  fixed_t frac = 0, lastfrac;

  // SoM: ANYRES
  // Moved stuff, reformatted a bit
  // haleyjd 04/03/05: removed unnecessary FixedDiv calls

  video.xscale  = (SCREENWIDTH << FRACBITS) / 320;
  video.xstep   = ((320 << FRACBITS) / WIDE_SCREENWIDTH) + 1; // e6y: wide-res
  video.yscale  = (SCREENHEIGHT << FRACBITS) / 200;
  video.ystep   = ((200 << FRACBITS) / SCREENHEIGHT) + 1;

  // SoM: ok, assemble the realx1/x2 arrays differently. To start, we are using floats
  // to do the scaling which is 100 times more accurate, secondly, I realized that the
  // reason the old single arrays were causing problems was they was only calculating the 
  // top-left corner of the scaled pixels. Calculating widths through these arrays is wrong
  // because the scaling will change the final scaled widths depending on what their unscaled
  // screen coords were. Thusly, all rectangles should be converted to unscaled x1, y1, x2, y2
  // coords, scaled, and then converted back to x, y, w, h
  //
  // e6y: wide-res

  video.x1lookup[0] = 0;
  lastfrac = frac = 0;
  for(i = 0; i < WIDE_SCREENWIDTH; i++)
  {
    if(frac >> FRACBITS > lastfrac >> FRACBITS)
    {
      video.x1lookup[frac >> FRACBITS] = i;
      video.x2lookup[lastfrac >> FRACBITS] = i - 1;

      lastfrac = frac;
    }
    frac += video.xstep;
  }
  video.x2lookup[319] = SCREENWIDTH - 1;
  video.x1lookup[320] = video.x2lookup[320] = SCREENWIDTH;

  video.y1lookup[0] = 0;
  lastfrac = frac = 0;
  for(i = 0; i < SCREENHEIGHT; i++)
  {
    if(frac >> FRACBITS > lastfrac >> FRACBITS)
    {
      video.y1lookup[frac >> FRACBITS] = i;
      video.y2lookup[lastfrac >> FRACBITS] = i-1;
      lastfrac = frac;
    }

    frac += video.ystep;
  }
  video.y2lookup[199] = SCREENHEIGHT - 1;
  video.y1lookup[200] = video.y2lookup[200] = SCREENHEIGHT;

  // e6y: for fast calculation of patches offset
  memset(video.strech_offsetx, 0, sizeof(video.strech_offsetx));
  video.strech_offsetx[VPT_STRETCH] = wide_offsetx;
  video.strech_offsetx[VPT_STRETCH_RIGHT] = 2 * wide_offsetx;
}

//
// R_ExecuteSetViewSize
//

void R_ExecuteSetViewSize (void)
{
  int i;

  setsizeneeded = false;

  if (setblocks == 11)
    {
      scaledviewwidth = SCREENWIDTH;
      viewheight = SCREENHEIGHT;
    }
// proff 09/24/98: Added for high-res
  else if (setblocks == 10)
    {
      scaledviewwidth = SCREENWIDTH;
      viewheight = SCREENHEIGHT-ST_SCALED_HEIGHT;
    }
  else
    {
// proff 08/17/98: Changed for high-res
      scaledviewwidth = setblocks*SCREENWIDTH/10;
      viewheight = (setblocks*(SCREENHEIGHT-ST_SCALED_HEIGHT)/10) & ~7;
    }

  viewwidth = scaledviewwidth;

  viewheightfrac = viewheight<<FRACBITS;//e6y

  centery = viewheight/2;
  centerx = viewwidth/2;
  centerxfrac = centerx<<FRACBITS;
  centeryfrac = centery<<FRACBITS;

  // If the screen is approximately 16:9 or 16:10, consider it widescreen.
  CheckRatio(SCREENWIDTH, SCREENHEIGHT);

  // e6y: wide-res
  projection = wide_centerx<<FRACBITS;

// proff 11/06/98: Added for high-res
  projectiony = ((SCREENHEIGHT * centerx * 320) / 200) / SCREENWIDTH * FRACUNIT;
  // e6y: this is a precalculated value for more precise flats drawing (see R_MapPlane)
  viewfocratio = (1.6f * centerx / wide_centerx) / ((float)SCREENWIDTH / (float)SCREENHEIGHT);

  R_SetupViewScaling();

  R_InitBuffer (scaledviewwidth, viewheight);

  R_InitTextureMapping();

  // psprite scales
  // proff 08/17/98: Changed for high-res
  // proff 11/06/98: Added for high-res
  // e6y: wide-res
  pspritexscale = (wide_centerx << FRACBITS) / 160;
  pspriteyscale = (((SCREENHEIGHT*viewwidth)/SCREENWIDTH) << FRACBITS) / 200;
  pspriteiscale = FixedDiv (FRACUNIT, pspritexscale);

  //e6y: added for GL
  pspritexscale_f = (float)wide_centerx/160.0f;
  pspriteyscale_f = (((float)SCREENHEIGHT*viewwidth)/(float)SCREENWIDTH) / 200.0f;

  // thing clipping
  for (i=0 ; i<viewwidth ; i++)
    screenheightarray[i] = viewheight;

  // planes
  for (i=0 ; i<viewheight ; i++)
    {   // killough 5/2/98: reformatted
      fixed_t dy = D_abs(((i-viewheight/2)<<FRACBITS)+FRACUNIT/2);
// proff 08/17/98: Changed for high-res
      yslope[i] = FixedDiv(projectiony, dy);
    }

  for (i=0 ; i<viewwidth ; i++)
    {
      fixed_t cosadj = D_abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
      distscale[i] = FixedDiv(FRACUNIT,cosadj);
    }

  // e6y
  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0 ; i< LIGHTLEVELS ; i++)
  {
    int j, startmap, level;
    startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
    for (j=0 ; j<MAXLIGHTSCALE ; j++)
    {
      level = BETWEEN(0, NUMCOLORMAPS - 1, startmap - j*SCREENWIDTH/(viewwidth)/DISTMAP);
      scalelight_offset[i][j] = level*256;
    }
  }

}

//
// R_Init
//

extern int screenblocks;

void R_Init (void)
{
  // CPhipps - R_DrawColumn isn't constant anymore, so must
  //  initialise in code
  // current column draw function
  lprintf(LO_INFO, "\nR_LoadTrigTables: ");
  R_LoadTrigTables();
  lprintf(LO_INFO, "\nR_InitData: ");
  R_InitData();
  R_SetViewSize(screenblocks);
  lprintf(LO_INFO, "\nR_Init: R_InitPlanes ");
  R_InitPlanes();
  lprintf(LO_INFO, "R_InitLightTables ");
  R_InitLightTables();
  lprintf(LO_INFO, "R_InitSkyMap ");
  R_InitSkyMap();
  lprintf(LO_INFO, "R_InitTranslationsTables ");
  R_InitTranslationTables();
  lprintf(LO_INFO, "R_InitPatches ");
  R_InitPatches();
}

//
// R_PointInSubsector
//
// killough 5/2/98: reformatted, cleaned up

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
  int nodenum = numnodes-1;

  // special case for trivial maps (single subsector, no nodes)
  if (numnodes == 0)
    return subsectors;

  while (!(nodenum & NF_SUBSECTOR))
    nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];
  return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_SetupFrame
//

static void R_SetupFrame (player_t *player)
{
  int cm;
  dboolean NoInterpolate = paused || (menuactive && !demoplayback);

  viewplayer = player;

  if (player->mo != oviewer || NoInterpolate)
  {
    R_ResetViewInterpolation ();
    oviewer = player->mo;
  }
  tic_vars.frac = I_GetTimeFrac ();
  if (NoInterpolate)
    tic_vars.frac = FRACUNIT;
  R_InterpolateView (player, tic_vars.frac);

  extralight = player->extralight;

  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

  R_DoInterpolations(tic_vars.frac);

  // killough 3/20/98, 4/4/98: select colormap based on player status

  if (player->mo->subsector->sector->heightsec != -1)
    {
      const sector_t *s = player->mo->subsector->sector->heightsec + sectors;
      cm = viewz < s->floorheight ? s->bottommap : viewz > s->ceilingheight ?
        s->topmap : s->midmap;
      if (cm < 0 || cm > numcolormaps)
        cm = 0;
    }
  else
    cm = 0;

  //e6y: save previous and current colormap
  last_boom_cm = boom_cm;
  boom_cm = cm;

  fullcolormap = colormaps[cm];
  zlight = c_zlight[cm];

  //e6y
  frame_fixedcolormap = player->fixedcolormap;
  if (frame_fixedcolormap < 0 || frame_fixedcolormap > NUMCOLORMAPS)
  {
    I_Error("<fixedcolormap> value out of range: %d\n", player->fixedcolormap);
  }

  if (player->fixedcolormap)
    {
      fixedcolormap = fullcolormap   // killough 3/20/98: use fullcolormap
        + player->fixedcolormap*256*sizeof(lighttable_t);
    }
  else
    fixedcolormap = 0;

  R_SetClipPlanes();

  validcount++;
}

//
// R_ShowStats
//
int rendered_visplanes, rendered_segs, rendered_vissprites;
dboolean rendering_stats;
int renderer_fps = 0;

static void R_ShowStats(void)
{
  static unsigned int FPS_SavedTick = 0, FPS_FrameCount = 0;
  unsigned int tick = SDL_GetTicks();
  FPS_FrameCount++;
  if(tick >= FPS_SavedTick + 1000)
  {
    renderer_fps = 1000 * FPS_FrameCount / (tick - FPS_SavedTick);
    if (rendering_stats)
    {
      doom_printf((V_GetMode() == VID_MODEGL)
                  ?"Frame rate %d fps\nWalls %d, Flats %d, Sprites %d"
                  :"Frame rate %d fps\nSegs %d, Visplanes %d, Sprites %d",
      renderer_fps, rendered_segs, rendered_visplanes, rendered_vissprites);
    }
    FPS_SavedTick = tick;
    FPS_FrameCount = 0;
  }
}

//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
  r_frame_count++;

  R_SetupFrame (player);

  // Clear buffers.
  R_ClearClipSegs ();
  R_ClearDrawSegs ();
  R_ClearPlanes ();
  R_ClearSprites ();

  rendered_segs = rendered_visplanes = 0;
  if (V_GetMode() == VID_MODEGL)
  {
#ifdef GL_DOOM
    // proff 11/99: clear buffers
    gld_InitDrawScene();
    // proff 11/99: switch to perspective mode
    gld_StartDrawScene();
#endif
  } else {
    if (flashing_hom)
    { // killough 2/10/98: add flashing red HOM indicators
      unsigned char color=(gametic % 20) < 9 ? 0xb0 : 0;
      V_FillRect(0, viewwindowx, viewwindowy, viewwidth, viewheight, color);
      R_DrawViewBorder();
    }
  }

  // check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL) {
    {
      angle_t a1 = gld_FrustumAngle();
      gld_clipper_Clear();
      gld_clipper_SafeAddClipRange(viewangle + a1, viewangle - a1);
    }
  }
#endif

  // The head node is the last node output.
  R_RenderBSPNode (numnodes-1);
  R_ResetColumnBuffer();

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

  if (V_GetMode() != VID_MODEGL)
    R_DrawPlanes ();

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

  if (V_GetMode() != VID_MODEGL) {
    R_DrawMasked ();
    R_ResetColumnBuffer();
  }

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

  if (V_GetMode() == VID_MODEGL) {
#ifdef GL_DOOM
    // proff 11/99: draw the scene
    gld_DrawScene(player);
    // proff 11/99: finishing off
    gld_EndDrawScene();
#endif
  }

  //e6y if (rendering_stats) 
  R_ShowStats();

  R_RestoreInterpolations();
}
