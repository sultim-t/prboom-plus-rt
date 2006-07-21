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
 *      Rendering main loop and setup functions,
 *       utility functions (BSP, geometry, trigonometry).
 *      See tables.c, too.
 *
 *-----------------------------------------------------------------------------*/


#include "doomstat.h"
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
#ifdef GL_DOOM
#include "gl_struct.h"
#endif
#include "e6y.h"//e6y

void R_LoadTrigTables(void);

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// killough: viewangleoffset is a legacy from the pre-v1.2 days, when Doom
// had Left/Mid/Right viewing. +/-ANG90 offsets were placed here on each
// node, by d_net.c, to set up a L/M/R session.

int viewangleoffset;
int validcount = 1;         // increment every time a check is made
lighttable_t *fixedcolormap;
int      centerx, centery;
fixed_t  centerxfrac, centeryfrac;
fixed_t  projection;
// proff 11/06/98: Added for high-res
fixed_t  projectiony;
fixed_t  viewx, viewy, viewz;
angle_t  viewangle;
fixed_t  viewcos, viewsin;
player_t *viewplayer;
extern lighttable_t **walllights;

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

angle_t xtoviewangle[MAX_SCREENWIDTH+1];   // killough 2/8/98

// killough 3/20/98: Support dynamic colormaps, e.g. deep water
// killough 4/4/98: support dynamic number of them as well

int numcolormaps;
lighttable_t *(*c_scalelight)[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];
lighttable_t *(*scalelight)[MAXLIGHTSCALE];
lighttable_t *(*zlight)[MAXLIGHTZ];
lighttable_t *fullcolormap;
lighttable_t **colormaps;

// killough 3/20/98, 4/4/98: end dynamic colormaps

int extralight;                           // bumped light from gun blasts

void (*colfunc)(void);

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//

CONSTFUNC int R_PointOnSide(fixed_t x, fixed_t y, const node_t *node)
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

CONSTFUNC int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t *line)
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

//
// R_InitTextureMapping
//
// killough 5/2/98: reformatted

static void R_InitTextureMapping (void)
{
  register int i,x;
  fixed_t focallength;

  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  //
  // Calc focallength
  //  so FIELDOFVIEW angles covers SCREENWIDTH.

  focallength = FixedDiv(centerxfrac, finetangent[FINEANGLES/4+FIELDOFVIEW/2]);

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
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//

#define DISTMAP 2

void R_InitLightTables (void)
{
  int i;

  // killough 4/4/98: dynamic colormaps
  c_zlight = malloc(sizeof(*c_zlight) * numcolormaps);
  c_scalelight = malloc(sizeof(*c_scalelight) * numcolormaps);

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

boolean setsizeneeded;
int     setblocks;

void R_SetViewSize(int blocks)
{
  setsizeneeded = true;
  setblocks = blocks;
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

  centery = viewheight/2;
  centerx = viewwidth/2;
  centerxfrac = centerx<<FRACBITS;
  centeryfrac = centery<<FRACBITS;
  projection = centerxfrac;
// proff 11/06/98: Added for high-res
  projectiony = ((SCREENHEIGHT * centerx * 320) / 200) / SCREENWIDTH * FRACUNIT;

  R_InitBuffer (scaledviewwidth, viewheight);

  R_InitTextureMapping();

  // psprite scales
// proff 08/17/98: Changed for high-res
  pspritescale = FRACUNIT*viewwidth/320;
  pspriteiscale = FRACUNIT*320/viewwidth;
// proff 11/06/98: Added for high-res
  pspriteyscale = (((SCREENHEIGHT*viewwidth)/SCREENWIDTH) << FRACBITS) / 200;

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

  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0; i<LIGHTLEVELS; i++)
    {
      int j, startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
          int t, level = startmap - j*320/viewwidth/DISTMAP;

          if (level < 0)
            level = 0;

          if (level >= NUMCOLORMAPS)
            level = NUMCOLORMAPS-1;

          // killough 3/20/98: initialize multiple colormaps
          level *= 256;

          for (t=0; t<numcolormaps; t++)     // killough 4/4/98
            c_scalelight[t][i][j] = colormaps[t] + level;
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
  colfunc = R_DrawColumn;     // current column draw function
  if (SCREENWIDTH<320)
    I_Error("R_Init: Screenwidth(%d) < 320",SCREENWIDTH);
#if defined TABLES_AS_LUMPS && defined NO_PREDEFINED_LUMPS
  lprintf(LO_INFO, "\nR_LoadTrigTables: ");
  R_LoadTrigTables();
#endif
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
}

//
// R_PointInSubsector
//
// killough 5/2/98: reformatted, cleaned up

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
  int nodenum = numnodes-1;
  while (!(nodenum & NF_SUBSECTOR))
    nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];
  return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_SetupFrame
//

void R_SetupFrame (player_t *player)
{
  int i, cm;

  viewplayer = player;
//e6y  viewx = player->mo->x;
//e6y  viewy = player->mo->y;
//e6y  viewangle = player->mo->angle + viewangleoffset;
//e6y
  if (player->mo != oviewer || r_NoInterpolate)
  {
    R_ResetViewInterpolation ();
    oviewer = player->mo;
  }
  r_TicFrac = I_GetTimeFrac ();
  if (r_NoInterpolate)
    r_TicFrac = FRACUNIT;
  R_InterpolateView (player, r_TicFrac);

  extralight = player->extralight;

//e6y  viewz = player->viewz;

  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

  dointerpolations(r_TicFrac);//e6y

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

  fullcolormap = colormaps[cm];
  zlight = c_zlight[cm];
  scalelight = c_scalelight[cm];

  if (player->fixedcolormap)
    {
      // killough 3/20/98: localize scalelightfixed (readability/optimization)
      static lighttable_t *scalelightfixed[MAXLIGHTSCALE];

      fixedcolormap = fullcolormap   // killough 3/20/98: use fullcolormap
        + player->fixedcolormap*256*sizeof(lighttable_t);

      walllights = scalelightfixed;

      for (i=0 ; i<MAXLIGHTSCALE ; i++)
        scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

  validcount++;
}

int autodetect_hom = 0;       // killough 2/7/98: HOM autodetection flag

//
// R_ShowStats
//
int rendered_visplanes, rendered_segs, rendered_vissprites;
boolean rendering_stats;

static void R_ShowStats(void)
{
#define KEEPTIMES 10
  static int keeptime[KEEPTIMES], showtime;
  int now = I_GetTime();

  if (now - showtime > 35) {
#ifdef GL_DOOM
    doom_printf("Frame rate %d fps\nWalls %d, Flats %d, Sprites %d",
#else
    doom_printf("Frame rate %d fps\nSegs %d, Visplanes %d, Sprites %d",
#endif
    (35*KEEPTIMES)/(now - keeptime[0]), rendered_segs,
    rendered_visplanes, rendered_vissprites);
    showtime = now;
  }
  memmove(keeptime, keeptime+1, sizeof(keeptime[0]) * (KEEPTIMES-1));
  keeptime[KEEPTIMES-1] = now;
}

//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
  R_SetupFrame (player);

  // Clear buffers.
  R_ClearClipSegs ();
  R_ClearDrawSegs ();
  R_ClearPlanes ();
  R_ClearSprites ();

  rendered_segs = rendered_visplanes = 0;
#ifdef GL_DOOM
  // proff 11/99: clear buffers
  gld_InitDrawScene();
#else /* not GL_DOOM */
  if (autodetect_hom)
    { // killough 2/10/98: add flashing red HOM indicators
      char c[47*47];
      extern int lastshottic;
      int i,color=(gametic % 20) < 9 ? 0xb0 : 0;
      memset(*screens+viewwindowy*SCREENWIDTH,color,viewheight*SCREENWIDTH);
      for (i=0;i<47*47;i++)
        {
          char t =
"/////////////////////////////////////////////////////////////////////////////"
"/////////////////////////////////////////////////////////////////////////////"
"///////jkkkkklk////////////////////////////////////hkllklklkklkj/////////////"
"///////////////////jkkkkklklklkkkll//////////////////////////////kkkkkklklklk"
"lkkkklk//////////////////////////jllkkkkklklklklklkkklk//////////////////////"
"//klkkllklklklkllklklkkklh//////////////////////kkkkkjkjjkkj\3\205\214\3lllkk"
"lkllh////////////////////kllkige\211\210\207\206\205\204\203\203\203\205`\206"
"\234\234\234\234kkllg//////////////////klkkjhfe\210\206\203\203\203\202\202"
"\202\202\202\202\203\205`\207\211eikkk//////////////////kkkk\3g\211\207\206"
"\204\203\202\201\201\200\200\200\200\200\201\201\202\204b\210\211\3lkh///////"
"//////////lklki\213\210b\206\203\201\201\200\200\200\200\200Z\200\200\200\202"
"\203\204\205\210\211jll/////////////////lkkk\3\212\210b\205\202\201\200\200"
"\200XW\200\200\200\200\200\200\202\203\204\206\207eklj////////////////lkkjg"
"\211b\206\204\202\200\200\200YWWX\200Z\200\200\200\202\203\203\205bdjkk//////"
"//////////llkig\211a\205\203\202\200\200\200YXWX\200\200\200\200\200\201\202"
"\203\203\206\207ekk////////////////lkki\3\211\206\204\202\201\200\200XXWWWXX"
"\200\200\200\200\202\202\204\206\207ekk////////////////lkkj\3e\206\206\204\\"
"\200\200XWVVWWWXX\200\200\200\\\203\205\207\231kk////////////////lkkjjgcccfd"
"\207\203WVUVW\200\200\202\202\204\204\205\204\206\210gkk////////////////kkkkj"
"e``\210hjjgb\200W\200\205\206fhghcbdcdfkk////////////////jkkj\3\207ab\211e"
"\213j\3g\204XX\207\213jii\212\207\203\204\210gfkj///////////////j\211lkjf\210"
"\214\3\3kj\213\213\211\205X\200\205\212\210\213\213\213\211\210\203\205gelj//"
"////////////hf\211\213kh\212\212i\212gkh\202\203\210\210\202\201\206\207\206"
"\\kkhf\210aabkk//////////////je\210\210\3g\210\207\210e\210c\205\204\202\210"
"\207\203\202\210\205\203\203fjbe\213\210bbieW/////////////ke\207\206ie\206"
"\203\203\203\205\205\204\203\210\211\207\202\202\206\210\203\204\206\207\210"
"\211\231\206\206`\206\206]/////////////kf\\\202ig\204\203\202\201\\\202\202"
"\205\207\210\207\203\202\206\206\206\205\203\203\203\202\202\203\204b\206\204"
"Z/////////////i\3\\\204j\212\204\202\201\200\202\202\202\203\206\211\210\203"
"\203c\205\202\201\201\201\200\200\201\202\204a\204\201W/////////////j\3\207"
"\210jh\206\202\200\200\200\200\200\202\206\211\205\202\202bb\201\200\200\200"
"\200\200\200\202\203b\\WW/////////////jke\206jic\203\201\200\200\200\200\202"
"\211\211\201\200\200\204\210\201\200\200W\200\200\200\201\204c\\\200]////////"
"//////kd\210\3\3e\205\202\200\200W\200\202\211\210\210\201\202\207\210\203"
"\200WWW\200\200\202\205d\\\202///////////////kkdhigb\203\201\200\200\200\202"
"\206\210\210\205\210\211\206\203\200WWW\200\201\203ce\203\205////////////////"
"ijkig\211\203\201\200\200\202\206\207\207\205\206\207\210\206\203\200\200WW"
"\200\203\206ce\202_//////////////////jig\210\203\202\200\201\206\210\210\205"
"\204\204\205\206\206\204\202\200\200\200\200\203bcd////////////////////hjgc"
"\205\202\201\203\206\210\206\204\204\202\202\204\205\206\204\200\200\200\201"
"\206\207c//////////////////////j\3\207\204\203\202\202\211c\204\201W\200\200"
"\203\205\206\203\200\200\200\203\206b///////////////////////ihd\204\203\202"
"\201\207f\205VTVTW\202\210\206Z\200\200\203aa////////////////////////jg\204"
"\204\203\201\202\210\211\211c\206\205\210d\210\200\200\200\202\204ac/////////"
"///////////////j\3b\203\203\202\202\205\207\206\205\207\207\206\206\202\200"
"\201\202\203ac/////////////////////////iid\206\204\203\202\204\205\377\205"
"\204\205\204\203\201\200\202\203\203bc//////////////////////////ej\207\205"
"\203\201\202\202\203\207\204\203\202\202\201\201\203\203bd///////////////////"
"////////ee\3a\204\201\200\201\202\205\203\201\200\200\201\202\204\205cc//////"
"//////////////////////c\3ec\203\201\200\200\201\202\201\200\200\202\203\206cc"
"//////////////////////////////c\3f\206\203\201\200\200\200\200\200\201\203bdc"
"////////////////////////////////g\3\211\206\202\\\201\200\201\202\203dde/////"
"/////////////////////////////\234\3db\203\203\203\203adec////////////////////"
"/////////////////hffed\211de////////////////////"[i];
          c[i] = t=='/' ? color : t;
        }
      if (gametic-lastshottic < TICRATE*2 && gametic-lastshottic > TICRATE/8)
        V_DrawBlock(viewwindowx +  viewwidth/2 - 24,
                    viewwindowy + viewheight/2 - 24, 0, 47, 47, c, VPT_NONE);
      R_DrawViewBorder();
    }
#endif /* not GL_DOOM */

#ifdef GL_DOOM
  // proff 11/99: switch to perspective mode
  gld_StartDrawScene();
#endif

  // check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

  // The head node is the last node output.
#ifndef GL_DOOM //e6y
  R_RenderBSPNode (numnodes-1);

  //e6y
#else
  if ((render_fov > FOV90) || (GetMouseLook() && viewpitch))
  {
    unsigned int oldviewangle;
    oldviewangle = viewangle;
    if (render_fov > FOV90 || transparentpresent)//FIXME!!!
    {
      R_RenderBSPNode (numnodes-1);
      viewangle += ANG90;
      R_ClearClipSegs();
      R_RenderBSPNode(numnodes - 1);
      viewangle += ANG90;
      if (((int) viewpitch > ANG45 || (int) viewpitch < -ANG45))
      {
        R_ClearClipSegs();
        R_RenderBSPNode(numnodes - 1);
      }
      viewangle += ANG90;
      R_ClearClipSegs();
      R_RenderBSPNode(numnodes - 1);
    }
    else
    {
      viewangle -= ANG45;
      R_RenderBSPNode(numnodes - 1);
      viewangle += ANG90;
      R_ClearClipSegs();
      R_RenderBSPNode(numnodes - 1);
      if (((int) viewpitch > ANG45 || (int) viewpitch < -ANG45))
      {
        viewangle += ANG90;
        R_ClearClipSegs();
        R_RenderBSPNode(numnodes - 1);
        viewangle += ANG90;
        R_ClearClipSegs();
        R_RenderBSPNode(numnodes - 1);
      }
    }
    viewangle = oldviewangle;
  } 
  else
  {
    R_RenderBSPNode (numnodes-1);
  } 
#endif

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

#ifndef GL_DOOM
  R_DrawPlanes ();
#endif

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

#ifndef GL_DOOM
  R_DrawMasked ();
#endif

  // Check for new console commands.
#ifdef HAVE_NET
  NetUpdate ();
#endif

#ifdef GL_DOOM
  // proff 11/99: draw the scene
  gld_DrawScene(player);
  // proff 11/99: finishing off
  gld_EndDrawScene();
#endif
  if (rendering_stats) R_ShowStats();
  
  restoreinterpolations ();//e6y
}
