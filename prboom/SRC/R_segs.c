// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: R_segs.c,v 1.1 2000/04/09 18:13:55 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------
//
// 4/25/98, 5/2/98 killough: reformatted, beautified

static const char
rcsid[] = "$Id: R_segs.c,v 1.1 2000/04/09 18:13:55 proff_fs Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_things.h"
#include "r_draw.h"
#include "w_wad.h"
#ifdef GL_DOOM
#include "gl_struct.h"
#endif

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

// True if any of the segs textures might be visible.
static H_boolean  segtextured;
static H_boolean  markfloor;      // False if the back side is the same plane.
static H_boolean  markceiling;
static H_boolean  maskedtexture;
static int      toptexture;
static int      bottomtexture;
static int      midtexture;

angle_t         rw_normalangle; // angle to line origin
int             rw_angle1;
fixed_t         rw_distance;
lighttable_t    **walllights;

//
// regular wall
//
static int      rw_x;
static int      rw_stopx;
static angle_t  rw_centerangle;
static fixed_t  rw_offset;
static fixed_t  rw_scale;
static fixed_t  rw_scalestep;
static fixed_t  rw_midtexturemid;
static fixed_t  rw_toptexturemid;
static fixed_t  rw_bottomtexturemid;
static int      worldtop;
static int      worldbottom;
static int      worldhigh;
static int      worldlow;
static fixed_t  pixhigh;
static fixed_t  pixlow;
static fixed_t  pixhighstep;
static fixed_t  pixlowstep;
static fixed_t  topfrac;
static fixed_t  topstep;
static fixed_t  bottomfrac;
static fixed_t  bottomstep;
static short    *maskedtexturecol;

//
// R_RenderMaskedSegRange
//

void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2)
{
  column_t *col;
  int      lightnum;
  int      texnum;
  sector_t tempsec;      // killough 4/13/98

  // Calculate light table.
  // Use different light tables
  //   for horizontal / vertical / diagonal. Diagonal?

  curline = ds->curline;  // OPTIMIZE: get rid of LIGHTSEGSHIFT globally

  // killough 4/11/98: draw translucent 2s normal textures

  colfunc = R_DrawColumn;
  if (curline->linedef->tranlump >= 0 && general_translucency)
    {
      colfunc = R_DrawTLColumn;
      // proff 11/19/98: Changed for 2 new filters
      tranmap = main_tranmap+TRANMAP_POS_2;
      //tranmap = main_tranmap;
      if (curline->linedef->tranlump > 0)
        tranmap = W_CacheLumpNum(curline->linedef->tranlump-1, PU_STATIC);
    }
  // killough 4/11/98: end translucent 2s normal code

  frontsector = curline->frontsector;
  backsector = curline->backsector;

  texnum = texturetranslation[curline->sidedef->midtexture];

  // killough 4/13/98: get correct lightlevel for 2s normal textures
  lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)
              ->lightlevel >> LIGHTSEGSHIFT)+extralight;

  if (curline->v1->y == curline->v2->y)
    lightnum--;
  else
    if (curline->v1->x == curline->v2->x)
      lightnum++;

  walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
    lightnum <  0           ? scalelight[0] : scalelight[lightnum];

  maskedtexturecol = ds->maskedtexturecol;

  rw_scalestep = ds->scalestep;
  spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
  mfloorclip = ds->sprbottomclip;
  mceilingclip = ds->sprtopclip;

  // find positioning
  if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
      dc_texturemid = frontsector->floorheight > backsector->floorheight
        ? frontsector->floorheight : backsector->floorheight;
      dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
  else
    {
      dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
        ? frontsector->ceilingheight : backsector->ceilingheight;
      dc_texturemid = dc_texturemid - viewz;
    }

  dc_texturemid += curline->sidedef->rowoffset;

  if (fixedcolormap)
    dc_colormap = fixedcolormap;

  // draw the columns
  for (dc_x = x1 ; dc_x <= x2 ; dc_x++, spryscale += rw_scalestep)
    if (maskedtexturecol[dc_x] != MAXSHORT)
      {
        if (!fixedcolormap)      // calculate lighting
          {
            unsigned index = spryscale>>LIGHTSCALESHIFT;

            if (index >=  MAXLIGHTSCALE )
              index = MAXLIGHTSCALE-1;

            dc_colormap = walllights[index];
          }

        // killough 3/2/98:
        //
        // This calculation used to overflow and cause crashes in Doom:
        //
        // sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
        //
        // This code fixes it, by using double-precision intermediate
        // arithmetic and by skipping the drawing of 2s normals whose
        // mapping to screen coordinates is totally out of range:

        {
          longlong t = ((longlong) centeryfrac << FRACBITS) -
            (longlong) dc_texturemid * spryscale;
          if (t + (longlong) textureheight[texnum] * spryscale < 0 ||
              t > (longlong) MAX_SCREENHEIGHT << FRACBITS*2)
            continue;        // skip if the texture is out of screen's range
          sprtopscreen = (long)(t >> FRACBITS);
        }

        dc_iscale = 0xffffffffu / (unsigned) spryscale;

        // killough 1/25/98: here's where Medusa came in, because
        // it implicitly assumed that the column was all one patch.
        // Originally, Doom did not construct complete columns for
        // multipatched textures, so there were no header or trailer
        // bytes in the column referred to below, which explains
        // the Medusa effect. The fix is to construct true columns
        // when forming multipatched textures (see r_data.c).

        // draw the texture
        col = (column_t *)((byte *)
                           R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);
        R_DrawMaskedColumn (col);
        maskedtexturecol[dc_x] = MAXSHORT;
      }

  // Except for main_tranmap, mark others purgable at this point
  if (curline->linedef->tranlump > 0 && general_translucency)
    Z_ChangeTag(tranmap, PU_CACHE); // killough 4/11/98
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling textures.
// CALLED: CORE LOOPING ROUTINE.
//

#define HEIGHTBITS 12
#define HEIGHTUNIT (1<<HEIGHTBITS)

static void R_RenderSegLoop (void)
{
  fixed_t  texturecolumn = 0;   // shut up compiler warning
  for ( ; rw_x < rw_stopx ; rw_x++)
    {
      // mark floor / ceiling areas
      int yh,yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

      // no space above wall?
      int bottom, top = ceilingclip[rw_x]+1;

      if (yl < top)
        yl = top;

      if (markceiling)
        {
          bottom = yl-1;

          if (bottom >= floorclip[rw_x])
            bottom = floorclip[rw_x]-1;

          if (top <= bottom)
            {
              ceilingplane->top[rw_x] = top;
              ceilingplane->bottom[rw_x] = bottom;
            }
        }

      yh = bottomfrac>>HEIGHTBITS;

      bottom = floorclip[rw_x]-1;
      if (yh > bottom)
        yh = bottom;

      if (markfloor)
        {
          top  = yh < ceilingclip[rw_x] ? ceilingclip[rw_x] : yh;
          if (++top <= bottom)
            {
              floorplane->top[rw_x] = top;
              floorplane->bottom[rw_x] = bottom;
            }
        }

      // texturecolumn and lighting are independent of wall tiers
      if (segtextured)
        {
          unsigned index;

          // calculate texture offset
          angle_t angle =(rw_centerangle+xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
          texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
          texturecolumn >>= FRACBITS;
          // calculate lighting
          index = rw_scale>>LIGHTSCALESHIFT;

          if (index >=  MAXLIGHTSCALE )
            index = MAXLIGHTSCALE-1;
          dc_colormap = walllights[index];
          dc_x = rw_x;
          dc_iscale = 0xffffffffu / (unsigned)rw_scale;
        }

      // draw the wall tiers
      if (midtexture)
        {
          dc_yl = yl;     // single sided line
          dc_yh = yh;
          dc_texturemid = rw_midtexturemid;
          dc_source = R_GetColumn(midtexture, texturecolumn);
          dc_texheight = textureheight[midtexture]>>FRACBITS; // killough
          colfunc ();
          ceilingclip[rw_x] = viewheight;
          floorclip[rw_x] = -1;
        }
      else
        {
          // two sided line
          if (toptexture)
            {
              // top wall
              int mid = pixhigh>>HEIGHTBITS;
              pixhigh += pixhighstep;

              if (mid >= floorclip[rw_x])
                mid = floorclip[rw_x]-1;

              if (mid >= yl)
                {
                  dc_yl = yl;
                  dc_yh = mid;
                  dc_texturemid = rw_toptexturemid;
                  dc_source = R_GetColumn(toptexture,texturecolumn);
                  dc_texheight = textureheight[toptexture]>>FRACBITS;//killough
                  colfunc ();
                  ceilingclip[rw_x] = mid;
                }
              else
                ceilingclip[rw_x] = yl-1;
            }
          else  // no top wall
            if (markceiling)
              ceilingclip[rw_x] = yl-1;

          if (bottomtexture)          // bottom wall
            {
              int mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
              pixlow += pixlowstep;

              // no space above wall?
              if (mid <= ceilingclip[rw_x])
                mid = ceilingclip[rw_x]+1;

              if (mid <= yh)
                {
                  dc_yl = mid;
                  dc_yh = yh;
                  dc_texturemid = rw_bottomtexturemid;
                  dc_source = R_GetColumn(bottomtexture,
                                          texturecolumn);
                  dc_texheight = textureheight[bottomtexture]>>FRACBITS; // killough
                  colfunc ();
                  floorclip[rw_x] = mid;
                }
              else
                floorclip[rw_x] = yh+1;
            }
          else        // no bottom wall
            if (markfloor)
              floorclip[rw_x] = yh+1;

          // save texturecol for backdrawing of masked mid texture
          if (maskedtexture)
            maskedtexturecol[rw_x] = texturecolumn;
        }

      rw_scale += rw_scalestep;
      topfrac += topstep;
      bottomfrac += bottomstep;
    }
}

// killough 5/2/98: move from r_main.c, made static, simplified

static fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
  // proff: Changed abs to D_abs (see m_fixed.h)
  fixed_t dx = D_abs(x - viewx);
  fixed_t dy = D_abs(y - viewy);

  if (dy > dx)
    {
      fixed_t t = dx;
      dx = dy;
      dy = t;
    }

  return dx ? FixedDiv(dx, finesine[(tantoangle[FixedDiv(dy,dx) >> DBITS]
              + ANG90) >> ANGLETOFINESHIFT]) : 0;
}

//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(const int start, const int stop)
{
  fixed_t hyp;
  fixed_t sineval;
  angle_t distangle, offsetangle;

  if (ds_p == drawsegs+maxdrawsegs)   // killough 1/98 -- fix 2s line HOM
    {
      unsigned pos = ds_p - drawsegs; // jff 8/9/98 fix from ZDOOM1.14a
      unsigned newmax = maxdrawsegs ? maxdrawsegs*2 : 128; // killough
      drawsegs = realloc(drawsegs,newmax*sizeof(*drawsegs));
//      ds_p = drawsegs+maxdrawsegs;
      ds_p = drawsegs + pos;          // jff 8/9/98 fix from ZDOOM1.14a
      maxdrawsegs = newmax;
    }

#ifdef GL_DOOM
  // proff 11/99: the rest of the calculations is not needed for OpenGL
  ds_p->curline = curline;
  ds_p++;
  curline->linedef->flags |= ML_MAPPED;
  return;
#endif

#ifdef RANGECHECK
  if (start >=viewwidth || start > stop)
    I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif

  sidedef = curline->sidedef;
  linedef = curline->linedef;

  // mark the segment as visible for auto map
  linedef->flags |= ML_MAPPED;

  // calculate rw_distance for scale calculation
  rw_normalangle = curline->angle + ANG90;
  // proff: Changed abs to D_abs (see m_fixed.h)
  offsetangle = D_abs(rw_normalangle-rw_angle1);

  if (offsetangle > ANG90)
    offsetangle = ANG90;

  distangle = ANG90 - offsetangle;
  hyp = R_PointToDist (curline->v1->x, curline->v1->y);
  sineval = finesine[distangle>>ANGLETOFINESHIFT];
  rw_distance = FixedMul(hyp, sineval);

  ds_p->x1 = rw_x = start;
  ds_p->x2 = stop;
  ds_p->curline = curline;
  rw_stopx = stop+1;

  // killough 1/6/98, 2/1/98: remove limit on openings
  // killough 8/1/98: Replaced code with a static limit
  // guaranteed to be big enough

  // calculate scale at both ends and step
  ds_p->scale1 = rw_scale =
    R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);

  if (stop > start)
    {
      ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
      ds_p->scalestep = rw_scalestep = (ds_p->scale2-rw_scale) / (stop-start);
    }
  else
    ds_p->scale2 = ds_p->scale1;

  // calculate texture boundaries
  //  and decide if floor / ceiling marks are needed
  worldtop = frontsector->ceilingheight - viewz;
  worldbottom = frontsector->floorheight - viewz;

  midtexture = toptexture = bottomtexture = maskedtexture = 0;
  ds_p->maskedtexturecol = NULL;

  if (!backsector)
    {
      // single sided line
      midtexture = texturetranslation[sidedef->midtexture];

      // a single sided line is terminal, so it must mark ends
      markfloor = markceiling = true;

      if (linedef->flags & ML_DONTPEGBOTTOM)
        {         // bottom of texture at bottom
          fixed_t vtop = frontsector->floorheight +
            textureheight[sidedef->midtexture];
          rw_midtexturemid = vtop - viewz;
        }
      else        // top of texture at top
        rw_midtexturemid = worldtop;

      rw_midtexturemid += sidedef->rowoffset;

      {      // killough 3/27/98: reduce offset
        fixed_t h = textureheight[sidedef->midtexture];
        if (h & (h-FRACUNIT))
          rw_midtexturemid %= h;
      }

      ds_p->silhouette = SIL_BOTH;
      ds_p->sprtopclip = screenheightarray;
      ds_p->sprbottomclip = negonearray;
      ds_p->bsilheight = MAXINT;
      ds_p->tsilheight = MININT;
    }
  else      // two sided line
    {
      ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
      ds_p->silhouette = 0;

      if (frontsector->floorheight > backsector->floorheight)
        {
          ds_p->silhouette = SIL_BOTTOM;
          ds_p->bsilheight = frontsector->floorheight;
        }
      else
        if (backsector->floorheight > viewz)
          {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = MAXINT;
          }

      if (frontsector->ceilingheight < backsector->ceilingheight)
        {
          ds_p->silhouette |= SIL_TOP;
          ds_p->tsilheight = frontsector->ceilingheight;
        }
      else
        if (backsector->ceilingheight < viewz)
          {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = MININT;
          }

      // killough 1/17/98: this test is required if the fix
      // for the automap bug (r_bsp.c) is used, or else some
      // sprites will be displayed behind closed doors. That
      // fix prevents lines behind closed doors with dropoffs
      // from being displayed on the automap.
      //
      // killough 4/7/98: make doorclosed external variable

      {
        extern int doorclosed;    // killough 1/17/98, 2/8/98, 4/7/98
        if (doorclosed || backsector->ceilingheight<=frontsector->floorheight)
          {
            ds_p->sprbottomclip = negonearray;
            ds_p->bsilheight = MAXINT;
            ds_p->silhouette |= SIL_BOTTOM;
          }
        if (doorclosed || backsector->floorheight>=frontsector->ceilingheight)
          {                   // killough 1/17/98, 2/8/98
            ds_p->sprtopclip = screenheightarray;
            ds_p->tsilheight = MININT;
            ds_p->silhouette |= SIL_TOP;
          }
      }

      worldhigh = backsector->ceilingheight - viewz;
      worldlow = backsector->floorheight - viewz;

      // hack to allow height changes in outdoor areas
      if (frontsector->ceilingpic == skyflatnum
          && backsector->ceilingpic == skyflatnum)
        worldtop = worldhigh;

      markfloor = worldlow != worldbottom
        || backsector->floorpic != frontsector->floorpic
        || backsector->lightlevel != frontsector->lightlevel

        // killough 3/7/98: Add checks for (x,y) offsets
        || backsector->floor_xoffs != frontsector->floor_xoffs
        || backsector->floor_yoffs != frontsector->floor_yoffs

        // killough 4/15/98: prevent 2s normals
        // from bleeding through deep water
        || frontsector->heightsec != -1

        // killough 4/17/98: draw floors if different light levels
        || backsector->floorlightsec != frontsector->floorlightsec
        ;

      markceiling = worldhigh != worldtop
        || backsector->ceilingpic != frontsector->ceilingpic
        || backsector->lightlevel != frontsector->lightlevel

        // killough 3/7/98: Add checks for (x,y) offsets
        || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
        || backsector->ceiling_yoffs != frontsector->ceiling_yoffs

        // killough 4/15/98: prevent 2s normals
        // from bleeding through fake ceilings
        || (frontsector->heightsec != -1 &&
            frontsector->ceilingpic!=skyflatnum)

        // killough 4/17/98: draw ceilings if different light levels
        || backsector->ceilinglightsec != frontsector->ceilinglightsec
        ;

      if (backsector->ceilingheight <= frontsector->floorheight
          || backsector->floorheight >= frontsector->ceilingheight)
        markceiling = markfloor = true;   // closed door

      if (worldhigh < worldtop)   // top texture
        {
          toptexture = texturetranslation[sidedef->toptexture];
          rw_toptexturemid = linedef->flags & ML_DONTPEGTOP ? worldtop :
            backsector->ceilingheight+textureheight[sidedef->toptexture]-viewz;
        }

      if (worldlow > worldbottom) // bottom texture
        {
          bottomtexture = texturetranslation[sidedef->bottomtexture];
          rw_bottomtexturemid = linedef->flags & ML_DONTPEGBOTTOM ? worldtop :
            worldlow;
        }
      rw_toptexturemid += sidedef->rowoffset;

      // killough 3/27/98: reduce offset
      {
        fixed_t h = textureheight[sidedef->toptexture];
        if (h & (h-FRACUNIT))
          rw_toptexturemid %= h;
      }

      rw_bottomtexturemid += sidedef->rowoffset;

      // killough 3/27/98: reduce offset
      {
        fixed_t h;
        h = textureheight[sidedef->bottomtexture];
        if (h & (h-FRACUNIT))
          rw_bottomtexturemid %= h;
      }

      // allocate space for masked texture tables
      if (sidedef->midtexture)    // masked midtexture
        {
          maskedtexture = true;
          ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
          lastopening += rw_stopx - rw_x;
        }
    }

  // calculate rw_offset (only needed for textured lines)
  segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

  if (segtextured)
    {
      offsetangle = rw_normalangle-rw_angle1;

      if (offsetangle > ANG180)
        offsetangle = -offsetangle;

      if (offsetangle > ANG90)
        offsetangle = ANG90;

      sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
      rw_offset = FixedMul (hyp, sineval);

      if (rw_normalangle-rw_angle1 < ANG180)
        rw_offset = -rw_offset;

      rw_offset += sidedef->textureoffset + curline->offset;

      rw_centerangle = ANG90 + viewangle - rw_normalangle;

      // calculate light table
      //  use different light tables
      //  for horizontal / vertical / diagonal
      // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
      if (!fixedcolormap)
        {
          int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

          if (curline->v1->y == curline->v2->y)
            lightnum--;
          else if (curline->v1->x == curline->v2->x)
            lightnum++;

          if (lightnum < 0)
            walllights = scalelight[0];
          else if (lightnum >= LIGHTLEVELS)
            walllights = scalelight[LIGHTLEVELS-1];
          else
            walllights = scalelight[lightnum];
        }
    }

  // if a floor / ceiling plane is on the wrong side of the view
  // plane, it is definitely invisible and doesn't need to be marked.

  // killough 3/7/98: add deep water check
  if (frontsector->heightsec == -1)
    {
      if (frontsector->floorheight >= viewz)       // above view plane
        markfloor = false;
      if (frontsector->ceilingheight <= viewz &&
          frontsector->ceilingpic != skyflatnum)   // below view plane
        markceiling = false;
    }

  // calculate incremental stepping values for texture edges
  worldtop >>= 4;
  worldbottom >>= 4;

  topstep = -FixedMul (rw_scalestep, worldtop);
  topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

  bottomstep = -FixedMul (rw_scalestep,worldbottom);
  bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);

  if (backsector)
    {
      worldhigh >>= 4;
      worldlow >>= 4;

      if (worldhigh < worldtop)
        {
          pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
          pixhighstep = -FixedMul (rw_scalestep,worldhigh);
        }
      if (worldlow > worldbottom)
        {
          pixlow = (centeryfrac>>4) - FixedMul (worldlow, rw_scale);
          pixlowstep = -FixedMul (rw_scalestep,worldlow);
        }
    }

  // render it
  if (markceiling)
    if (ceilingplane)   // killough 4/11/98: add NULL ptr checks
      ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
    else
      markceiling = 0;

  if (markfloor)
    if (floorplane)     // killough 4/11/98: add NULL ptr checks
      floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
    else
      markfloor = 0;

  R_RenderSegLoop();

  // save sprite clipping info
  if ((ds_p->silhouette & SIL_TOP || maskedtexture) && !ds_p->sprtopclip)
    {
      memcpy (lastopening, ceilingclip+start, 2*(rw_stopx-start));
      ds_p->sprtopclip = lastopening - start;
      lastopening += rw_stopx - start;
    }
  if ((ds_p->silhouette & SIL_BOTTOM || maskedtexture) && !ds_p->sprbottomclip)
    {
      memcpy (lastopening, floorclip+start, 2*(rw_stopx-start));
      ds_p->sprbottomclip = lastopening - start;
      lastopening += rw_stopx - start;
    }
  if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
    {
      ds_p->silhouette |= SIL_TOP;
      ds_p->tsilheight = MININT;
    }
  if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
    {
      ds_p->silhouette |= SIL_BOTTOM;
      ds_p->bsilheight = MAXINT;
    }
  ds_p++;
}

//----------------------------------------------------------------------------
//
// $Log: R_segs.c,v $
// Revision 1.1  2000/04/09 18:13:55  proff_fs
// Initial revision
//
// Revision 1.20  1998/10/05  21:46:31  phares
// Cleanup fireline checkin
//
// Revision 1.19  1998/10/05  21:29:32  phares
// Fixed firelines
//
// Revision 1.18  1998/09/11  16:19:17  jim
// Fixed startup on vertex segviol
//
// Revision 1.17  1998/08/11  07:58:58  jim
// Added ZDoom's fix to opening limit removal
//
// Revision 1.16  1998/05/03  23:02:01  killough
// Move R_PointToDist from r_main.c, fix #includes
//
// Revision 1.15  1998/04/27  01:48:37  killough
// Program beautification
//
// Revision 1.14  1998/04/17  10:40:31  killough
// Fix 213, 261 (floor/ceiling lighting)
//
// Revision 1.13  1998/04/16  06:24:20  killough
// Prevent 2s sectors from bleeding across deep water or fake floors
//
// Revision 1.12  1998/04/14  08:17:16  killough
// Fix light levels on 2s textures
//
// Revision 1.11  1998/04/12  02:01:41  killough
// Add translucent walls, add insurance against SIGSEGV
//
// Revision 1.10  1998/04/07  06:43:05  killough
// Optimize: use external doorclosed variable
//
// Revision 1.9  1998/03/28  18:04:31  killough
// Reduce texture offsets vertically
//
// Revision 1.8  1998/03/16  12:41:09  killough
// Fix underwater / dual ceiling support
//
// Revision 1.7  1998/03/09  07:30:25  killough
// Add primitive underwater support, fix scrolling flats
//
// Revision 1.6  1998/03/02  11:52:58  killough
// Fix texturemapping overflow, add scrolling walls
//
// Revision 1.5  1998/02/09  03:17:13  killough
// Make closed door clipping more consistent
//
// Revision 1.4  1998/02/02  13:27:02  killough
// fix openings bug
//
// Revision 1.3  1998/01/26  19:24:47  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  06:10:42  killough
// Discard old Medusa hack -- fixed in r_data.c now
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
