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
 *-----------------------------------------------------------------------------*/

//
// R_DrawSpan
//

#if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)  
  #define GETCOL(col) (dither_colormaps[filter_getDitheredPixelLevel(x1, y, fracz)][(col)])
#else
  #define GETCOL(col) colormap[(col)]
#endif

static void R_DRAWSPAN_FUNCNAME(draw_span_vars_t *dsvars)
{
#if (R_DRAWSPAN_PIPELINE & (RDC_ROUNDED|RDC_BILINEAR))
  // drop back to point filtering if we're minifying
  // 49152 = FRACUNIT * 0.75
  if (dsvars->xfrac > drawvars.mag_threshold && dsvars->yfrac > drawvars.mag_threshold) {
    R_GetDrawSpanFunc(RDRAW_FILTER_POINT,
                      drawvars.filterz)(dsvars);
    return;
  }
#endif
  {
  unsigned count = dsvars->x2 - dsvars->x1 + 1;
  fixed_t xfrac = dsvars->xfrac;
  fixed_t yfrac = dsvars->yfrac;
  const fixed_t xstep = dsvars->xstep;
  const fixed_t ystep = dsvars->ystep;
  const byte *source = dsvars->source;
  const byte *colormap = dsvars->colormap;
  byte *dest = drawvars.topleft + dsvars->y*drawvars.pitch + dsvars->x1;
#if (R_DRAWSPAN_PIPELINE & (RDC_DITHERZ|RDC_BILINEAR))
  const int y = dsvars->y;
  int x1 = dsvars->x1;
#endif
#if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)
  const int fracz = (dsvars->z >> 12) & 255;
  const byte *dither_colormaps[2] = { dsvars->colormap, dsvars->nextcolormap };
#endif

  while (count) {
#if (R_DRAWSPAN_PIPELINE & RDC_ROUNDED)
    *dest++ = GETCOL(filter_getRoundedForSpan(xfrac, yfrac));
    xfrac += xstep;
    yfrac += ystep;
    count--;
  #if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)
    x1--;
  #endif
#else
  #if (R_DRAWSPAN_PIPELINE & RDC_BILINEAR)
    // 8 bit bilinear
    const fixed_t xtemp = ((xfrac >> 16) + (filter_getDitheredPixelLevel(x1, y, ((xfrac>>8)&0xff)))) & 63;
    const fixed_t ytemp = ((yfrac >> 10) + 0x40*(filter_getDitheredPixelLevel(x1, y, ((yfrac>>8)&0xff)))) & 4032;
  #else
    const fixed_t xtemp = (xfrac >> 16) & 63;
    const fixed_t ytemp = (yfrac >> 10) & 4032;
  #endif
    const fixed_t spot = xtemp | ytemp;
    xfrac += xstep;
    yfrac += ystep;
    *dest++ = GETCOL(source[spot]);
    count--;
  #if (R_DRAWSPAN_PIPELINE & (RDC_DITHERZ|RDC_BILINEAR))
    x1--;
  #endif
#endif
  }
  }
}

#undef GETCOL

#undef R_DRAWSPAN_PIPELINE
#undef R_DRAWSPAN_FUNCNAME
