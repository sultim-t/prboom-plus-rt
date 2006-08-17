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


#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
#define GETDESTCOLOR(col) (tranmap[(*dest<<8)+(col)])
#else
#define GETDESTCOLOR(col) (col)
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLATED)
#define GETCOL8_MAPPED(col) (translation[(col)])
#else
#define GETCOL8_MAPPED(col) (col)
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_NOCOLMAP)
  #if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ)  
    #define GETCOL8_DEPTH(col) (filter_getDitheredPixelLevel(x, y, fracz)][GETCOL8_MAPPED(col))
  #else
    #define GETCOL8_DEPTH(col) GETCOL8_MAPPED(col)
  #endif
#else
  #if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ)  
    #define GETCOL8_DEPTH(col) (dither_colormaps[filter_getDitheredPixelLevel(x, y, fracz)][GETCOL8_MAPPED(col)])
  #else
    #define GETCOL8_DEPTH(col) colormap[GETCOL8_MAPPED(col)]
  #endif
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_BILINEAR)
 #define GETCOL(frac, nextfrac) GETCOL8_DEPTH(filter_getDitheredForColumn(x,y,frac,nextfrac))
#elif (R_DRAWCOLUMN_PIPELINE & RDC_ROUNDED)
 #define GETCOL(frac, nextfrac) GETCOL8_DEPTH(filter_getRoundedForColumn(frac,nextfrac))
#else
 #define GETCOL(frac, nextfrac) GETCOL8_DEPTH(source[(frac)>>FRACBITS])
#endif

#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED|RDC_DITHERZ))
  #define INCY(y) (y++)
#else
  #define INCY(y)
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
#define COLTYPE (COL_TRANS)
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
#define COLTYPE (COL_FUZZ)
#else
#define COLTYPE (COL_OPAQUE)
#endif

static void R_DRAWCOLUMN_FUNCNAME(draw_column_vars_t *dcvars)
{
  int              count;
  byte             *dest;            // killough
  fixed_t          frac;
  const fixed_t    fracstep = dcvars->iscale;

  // drop back to point filtering if we're minifying
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
  if (dcvars->iscale > drawvars.mag_threshold) {
    R_GetDrawColumnFunc(R_DRAWCOLUMN_PIPELINE_TYPE,
                        RDRAW_FILTER_POINT,
                        drawvars.filterz)(dcvars);
    return;
  }
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
  // Adjust borders. Low...
  if (!dcvars->yl)
    dcvars->yl = 1;

  // .. and high.
  if (dcvars->yh == viewheight-1)
    dcvars->yh = viewheight - 2;
#endif

  // leban 1/17/99:
  // removed the + 1 here, adjusted the if test, and added an increment
  // later.  this helps a compiler pipeline a bit better.  the x86
  // assembler also does this.

  count = dcvars->yh - dcvars->yl;

  // leban 1/17/99:
  // this case isn't executed too often.  depending on how many instructions
  // there are between here and the second if test below, this case could
  // be moved down and might save instructions overall.  since there are
  // probably different wads that favor one way or the other, i'll leave
  // this alone for now.
  if (count < 0)    // Zero length, column does not exceed a pixel.
    return;

#ifdef RANGECHECK
  if (dcvars->x >= SCREENWIDTH
      || dcvars->yl < 0
      || dcvars->yh >= SCREENHEIGHT)
    I_Error("R_DrawColumn: %i to %i at %i", dcvars->yl, dcvars->yh, dcvars->x);
#endif

  // Determine scaling, which is the only mapping to be done.
  frac = dcvars->texturemid + (dcvars->yl-centery)*fracstep;

  if (dcvars->drawingmasked && dcvars->edgetype == RDRAW_MASKEDCOLUMNEDGE_SLOPED) {
    // slope the top and bottom column edge based on the fractional u coordinate
    // and dcvars->edgeslope, which were set in R_DrawMaskedColumn
    // in r_things.c
    if (dcvars->yl != 0) {
      if (dcvars->edgeslope & RDRAW_EDGESLOPE_TOP_UP) {
        // [/#]
        int shift = ((0xffff-(dcvars->texu & 0xffff))/dcvars->iscale);
        dcvars->yl += shift;
        count -= shift;
        frac += 0xffff-(dcvars->texu & 0xffff);
      }
      else if (dcvars->edgeslope & RDRAW_EDGESLOPE_TOP_DOWN) {
        // [#\]
        int shift = ((dcvars->texu & 0xffff)/dcvars->iscale);
        dcvars->yl += shift;
        count -= shift;
        frac += dcvars->texu & 0xffff;
      }
    }
    if (dcvars->yh != viewheight-1) {
      if (dcvars->edgeslope & RDRAW_EDGESLOPE_BOT_UP) {
        // [#/]
        int shift = ((0xffff-(dcvars->texu & 0xffff))/dcvars->iscale);
        dcvars->yh -= shift;
        count -= shift;
      }
      else if (dcvars->edgeslope & RDRAW_EDGESLOPE_BOT_DOWN) {
        // [\#]
        int shift = ((dcvars->texu & 0xffff)/dcvars->iscale);
        dcvars->yh -= shift;
        count -= shift;
      }
    }
    if (count <= 0) return;  
  }

  // Framebuffer destination address.
   // SoM: MAGIC
   {
      // haleyjd: reordered predicates
      if(temp_x == 4 ||
         (temp_x && (temptype != COLTYPE || temp_x + startx != dcvars->x)))
         R_FlushColumns();

      if(!temp_x)
      {
         ++temp_x;
         startx = dcvars->x;
         *tempyl = commontop = dcvars->yl;
         *tempyh = commonbot = dcvars->yh;
         temptype = COLTYPE;
#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
         temptranmap = tranmap;
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
         tempfuzzmap = fullcolormap; // SoM 7-28-04: Fix the fuzz problem.
#endif
         R_FlushWholeColumns = R_FLUSHWHOLE_FUNCNAME;
         R_FlushHTColumns    = R_FLUSHHEADTAIL_FUNCNAME;
         R_FlushQuadColumn   = R_FLUSHQUAD_FUNCNAME;
         dest = &tempbuf[dcvars->yl << 2];
      } else {
         tempyl[temp_x] = dcvars->yl;
         tempyh[temp_x] = dcvars->yh;
   
         if(dcvars->yl > commontop)
            commontop = dcvars->yl;
         if(dcvars->yh < commonbot)
            commonbot = dcvars->yh;
      
         dest = &tempbuf[(dcvars->yl << 2) + temp_x++];
      }
   }

// do nothing else when drawin fuzz columns
#if (!(R_DRAWCOLUMN_PIPELINE & RDC_FUZZ))
  {
    const byte          *source = dcvars->source;
    const lighttable_t  *colormap = dcvars->colormap;
    const byte          *translation = dcvars->translation;
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED|RDC_DITHERZ))
    int y = dcvars->yl;
    const int x = dcvars->x;
#endif
#if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ)
    const int fracz = (dcvars->z >> 6) & 255;
    const byte *dither_colormaps[2] = { dcvars->colormap, dcvars->nextcolormap };
#endif
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
    const unsigned int filter_fracu = (dcvars->source == dcvars->nextsource) ? 0 : (dcvars->texu>>8) & 0xff;
#endif
#if (R_DRAWCOLUMN_PIPELINE & RDC_BILINEAR)
    const int yl = dcvars->yl;
    const byte *dither_sources[2] = { dcvars->source, dcvars->nextsource };
#endif
#if (R_DRAWCOLUMN_PIPELINE & RDC_ROUNDED)
    const byte          *prevsource = dcvars->prevsource;
    const byte          *nextsource = dcvars->nextsource;
#endif

    count++;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.       (Yeah, right!!! -- killough)
    //
    // killough 2/1/98: more performance tuning

    if (dcvars->texheight == 128) {
      #define FIXEDT_128MASK ((127<<FRACBITS)|0xffff)
      while(count--) {
        *dest = GETDESTCOLOR(GETCOL(frac & FIXEDT_128MASK, (frac+FRACUNIT) & FIXEDT_128MASK));
        INCY(y);
        dest += 4;
        frac += fracstep;
      }
    } else if (dcvars->texheight == 0) {
      /* cph - another special case */
      while (count--) {
        *dest = GETDESTCOLOR(GETCOL(frac, (frac+FRACUNIT)));
        INCY(y);
        dest += 4;
        frac += fracstep;
      }
    } else {
      unsigned heightmask = dcvars->texheight-1; // CPhipps - specify type
      if (! (dcvars->texheight & heightmask) ) { // power of 2 -- killough
        fixed_t fixedt_heightmask = (heightmask<<FRACBITS)|0xffff;
        while ((count-=2)>=0) { // texture height is a power of 2 -- killough
          *dest = GETDESTCOLOR(GETCOL(frac & fixedt_heightmask, (frac+FRACUNIT) & fixedt_heightmask));
          INCY(y);
          dest += 4;
          frac += fracstep;
          *dest = GETDESTCOLOR(GETCOL(frac & fixedt_heightmask, (frac+FRACUNIT) & fixedt_heightmask));
          INCY(y);
          dest += 4;
          frac += fracstep;
        }
        if (count & 1)
          *dest = GETDESTCOLOR(GETCOL(frac & fixedt_heightmask, (frac+FRACUNIT) & fixedt_heightmask));
          INCY(y);
      } else {
        fixed_t nextfrac = 0;

        heightmask++;
        heightmask <<= FRACBITS;

        if (frac < 0)
          while ((frac += heightmask) <  0);
        else
          while (frac >= (int)heightmask)
            frac -= heightmask;

#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
        nextfrac = frac + FRACUNIT;
        while (nextfrac >= (int)heightmask)
          nextfrac -= heightmask;
#endif
      
#define INCFRAC(f) if ((f += fracstep) >= (int)heightmask) f -= heightmask;

        while (count--) {
          // Re-map color indices from wall texture column
          //  using a lighting/special effects LUT.

          // heightmask is the Tutti-Frutti fix -- killough

          *dest = GETDESTCOLOR(GETCOL(frac, nextfrac));
          INCY(y);
          dest += 4;
          INCFRAC(frac);
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
          INCFRAC(nextfrac); 
#endif
        }
      }
    }
  }
#endif // (!(R_DRAWCOLUMN_PIPELINE & RDC_FUZZ))
}

#undef GETDESTCOLOR
#undef GETCOL8_MAPPED
#undef GETCOL8_DEPTH
#undef GETCOL
#undef INCY
#undef INCFRAC
#undef COLTYPE

#undef R_DRAWCOLUMN_FUNCNAME
#undef R_DRAWCOLUMN_PIPELINE
#undef R_FLUSHWHOLE_FUNCNAME
#undef R_FLUSHHEADTAIL_FUNCNAME
#undef R_FLUSHQUAD_FUNCNAME
