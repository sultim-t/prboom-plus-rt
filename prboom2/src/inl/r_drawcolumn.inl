//---------------------------------------------------------------------------
// This file is included from r_draw.c and is a unified version of the 
// previous set of R_DrawColumn* functions
//
// Valid switches to set when including this are:
//
// R_DRAWCOLUMN_PIPELINE
// R_DRAWCOLUMN_FUNCNAME
//
// Note that there is no performance penalty for macro parameters
// that are not used.
//
// - POPE
//---------------------------------------------------------------------------
// set up the GETCOL macro that solves for the current color given 
// the screen x, y, the current texture V coord, and the next texture V coord
#undef GETBLENDED8_5050
#undef GETBLENDED16_5050
#undef GETBLENDED32_5050
#undef GETBLENDED8_5050
#undef GETBLENDED16_7525
#undef GETBLENDED32_7525
#undef GETBLENDED16_8812
#undef GETBLENDED32_8812
#undef GETBLENDED16_9406
#undef GETBLENDED32_9406
#undef SETDESTBLENDED8
#undef SETDESTBLENDED16
#undef SETDESTBLENDED32
#undef SETDESTBLENDED
#undef SETDESTCOL

#undef SETDESTFUZZED8
#undef SETDESTFUZZED16
#undef SETDESTFUZZED32
#undef SETDESTFUZZED

#undef GETCOL8_MAPPED_NORMAL
#undef GETCOL8_MAPPED_PLAYER
#undef GETCOL8_MAPPED

#undef GETCOL8_DEPTH_POINT
#undef GETCOL8_DEPTH_LINEAR
#undef GETCOL8_DEPTH

#undef GETMAP_DEPTH_POINT
#undef GETMAP_DEPTH_LINEAR 
#undef GETMAP_DEPTH

#undef GETTRUECOLORFILTERED16_NORMAL
#undef GETTRUECOLORFILTERED16_PLAYER
#undef GETTRUECOLORFILTERED32_NORMAL
#undef GETTRUECOLORFILTERED32_PLAYER
#undef GETTRUECOLORFILTERED16
#undef GETTRUECOLORFILTERED32

#undef GETCOL8_LINEAR
#undef GETCOL8_POINT
#undef GETCOL8
#undef GETCOL16_LINEAR
#undef GETCOL16_POINT
#undef GETCOL16
#undef GETCOL32_LINEAR
#undef GETCOL32_POINT
#undef GETCOL32

#undef TOPLEFT
#undef GETCOL
#undef R_DRAWCOLUMN_SCRNTYPE
#undef INCSCREENY
#undef INCFRAC

//---------------------------------------------------------------------------
// Blend macros
#define GETBLENDED8_5050(col1, col2) col1 = tranmap[(col1<<8) + col2]

#define GETBLENDED16_5050(col1, col2) \
  ((((col1&0xf800)+(col2&0xf800))>>1)&0xf800) | \
  ((((col1&0x07e0)+(col2&0x07e0))>>1)&0x07e0) | \
  ((((col1&0x001f)+(col2&0x001f))>>1)&0x001f)

#define GETBLENDED32_5050(col1, col2) \
  ((((col1&0xff0000)+(col2&0xff0000))>>1)&0xff0000) | \
  ((((col1&0x00ff00)+(col2&0x00ff00))>>1)&0x00ff00) | \
  ((((col1&0x0000ff)+(col2&0x0000ff))>>1)&0x0000ff)

#define GETBLENDED16_7525(col1, col2) \
  ((((col1&0xf800)*3+(col2&0xf800))>>2)&0xf800) | \
  ((((col1&0x07e0)*3+(col2&0x07e0))>>2)&0x07e0) | \
  ((((col1&0x001f)*3+(col2&0x001f))>>2)&0x001f)

#define GETBLENDED32_7525(col1, col2) \
  ((((col1&0xff0000)*3+(col2&0xff0000))>>2)&0xff0000) | \
  ((((col1&0x00ff00)*3+(col2&0x00ff00))>>2)&0x00ff00) | \
  ((((col1&0x0000ff)*3+(col2&0x0000ff))>>2)&0x0000ff)

#define GETBLENDED16_8812(col1, col2) \
  ((((col1&0xf800)*7+(col2&0xf800))>>3)&0xf800) | \
  ((((col1&0x07e0)*7+(col2&0x07e0))>>3)&0x07e0) | \
  ((((col1&0x001f)*7+(col2&0x001f))>>3)&0x001f)

#define GETBLENDED32_8812(col1, col2) \
  ((((col1&0xff0000)*7+(col2&0xff0000))>>3)&0xff0000) | \
  ((((col1&0x00ff00)*7+(col2&0x00ff00))>>3)&0x00ff00) | \
  ((((col1&0x0000ff)*7+(col2&0x0000ff))>>3)&0x0000ff)

#define GETBLENDED16_9406(col1, col2) \
  ((((col1&0xf800)*15+(col2&0xf800))>>4)&0xf800) | \
  ((((col1&0x07e0)*15+(col2&0x07e0))>>4)&0x07e0) | \
  ((((col1&0x001f)*15+(col2&0x001f))>>4)&0x001f)

#define GETBLENDED32_9406(col1, col2) \
  ((((col1&0xff0000)*15+(col2&0xff0000))>>4)&0xff0000) | \
  ((((col1&0x00ff00)*15+(col2&0x00ff00))>>4)&0x00ff00) | \
  ((((col1&0x0000ff)*15+(col2&0x0000ff))>>4)&0x0000ff)
  
// forced 50-50 blending on non-8bit modes for speed (>>1)
#define SETDESTBLENDED8(col) *dest = GETBLENDED8_5050(*dest, col)
#define SETDESTBLENDED16(col) srcColor = col; *dest = GETBLENDED16_5050(*dest, col)
#define SETDESTBLENDED32(col) srcColor = col; *dest = GETBLENDED32_5050(*dest, col)

//---------------------------------------------------------------------------
// Fuzz macros
#define SETDESTFUZZED8(col) *dest = fullcolormap[6*256+(col)]
#define SETDESTFUZZED16(col) srcColor = col; *dest = GETBLENDED16_9406(srcColor, 0)
#define SETDESTFUZZED32(col) srcColor = col; *dest = GETBLENDED32_9406(srcColor, 0)

//---------------------------------------------------------------------------
// Player color-translation macros
#define GETCOL8_MAPPED_NORMAL(col) (col)
#define GETCOL8_MAPPED_PLAYER(col) dcvars.translation[(col)]

#define GETTRUECOLORFILTERED16_NORMAL(colormap, texV, nextRowTexV) filter_getFilteredForColumn16(colormap, texV, nextRowTexV)
#define GETTRUECOLORFILTERED16_PLAYER(colormap, texV, nextRowTexV) filter_getFilteredForColumn16_Translated(dcvars.translation,colormap, texV, nextRowTexV) 

#define GETTRUECOLORFILTERED32_NORMAL(colormap, texV, nextRowTexV) filter_getFilteredForColumn32(colormap, texV, nextRowTexV)
#define GETTRUECOLORFILTERED32_PLAYER(colormap, texV, nextRowTexV) filter_getFilteredForColumn32_Translated(dcvars.translation,colormap, texV, nextRowTexV) 

#if (R_DRAWCOLUMN_PIPELINE & RDC_PLAYER)
  #define GETCOL8_MAPPED(col) GETCOL8_MAPPED_PLAYER(col)
  #define GETTRUECOLORFILTERED16(colormap, texV, nextRowTexV) GETTRUECOLORFILTERED16_PLAYER(colormap, texV, nextRowTexV)
  #define GETTRUECOLORFILTERED32(colormap, texV, nextRowTexV) GETTRUECOLORFILTERED32_PLAYER(colormap, texV, nextRowTexV)
#else
  #define GETCOL8_MAPPED(col) GETCOL8_MAPPED_NORMAL(col)
  #define GETTRUECOLORFILTERED16(colormap, texV, nextRowTexV) GETTRUECOLORFILTERED16_NORMAL(colormap, texV, nextRowTexV)
  #define GETTRUECOLORFILTERED32(colormap, texV, nextRowTexV) GETTRUECOLORFILTERED32_NORMAL(colormap, texV, nextRowTexV)
#endif
 
//---------------------------------------------------------------------------
// Dither-On-Z macros
#define GETCOL8_DEPTH_LINEAR(col) (colormapAndNextColormap[filter_getDitheredPixelLevel(dcvars.x, screenY, fracz)][GETCOL8_MAPPED(col)])
#define GETCOL8_DEPTH_POINT(col) dcvars.colormap[GETCOL8_MAPPED(col)]

#define GETMAP_DEPTH_POINT dcvars.colormap
#define GETMAP_DEPTH_LINEAR (colormapAndNextColormap[filter_getDitheredPixelLevel(dcvars.x, screenY, fracz)])

#if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ)
  #define GETCOL8_DEPTH(col) GETCOL8_DEPTH_LINEAR(col)
  #define GETMAP_DEPTH GETMAP_DEPTH_LINEAR
#else
  #define GETCOL8_DEPTH(col) GETCOL8_DEPTH_POINT(col)
  #define GETMAP_DEPTH GETMAP_DEPTH_POINT
#endif

//---------------------------------------------------------------------------
// UV filter macros
#define GETCOL8_LINEAR(texV, nextRowTexV) GETCOL8_DEPTH(filter_getDitheredForColumn(dcvars.x,screenY,texV,nextRowTexV))
#define GETCOL8_POINT(texV, nextRowTexV) GETCOL8_DEPTH(dcvars.source[(texV)>>FRACBITS])
#define GETCOL8_ROUNDED(texV, nextRowTexV) GETCOL8_DEPTH(filter_getRoundedForColumn(texV,nextRowTexV))

#define GETCOL16_LINEAR(texV, nextRowTexV) GETTRUECOLORFILTERED16(GETMAP_DEPTH, texV, nextRowTexV)
#define GETCOL16_POINT(texV, nextRowTexV) VID_SHORTPAL(GETCOL8_DEPTH(GETCOL8_MAPPED(dcvars.source[(texV)>>FRACBITS])), VID_COLORWEIGHTMASK)
#define GETCOL16_ROUNDED(texV, nextRowTexV) VID_SHORTPAL(GETCOL8_DEPTH(GETCOL8_MAPPED(filter_getRoundedForColumn(texV,nextRowTexV))), VID_COLORWEIGHTMASK)

#define GETCOL32_LINEAR(texV, nextRowTexV) GETTRUECOLORFILTERED32(GETMAP_DEPTH, texV, nextRowTexV)
#define GETCOL32_POINT(texV, nextRowTexV) VID_INTPAL(GETCOL8_DEPTH(GETCOL8_MAPPED(dcvars.source[(texV)>>FRACBITS])), VID_COLORWEIGHTMASK)
#define GETCOL32_ROUNDED(texV, nextRowTexV) VID_INTPAL(GETCOL8_DEPTH(GETCOL8_MAPPED(filter_getRoundedForColumn(texV,nextRowTexV))), VID_COLORWEIGHTMASK)

#if (R_DRAWCOLUMN_PIPELINE & RDC_BILINEAR)
  #define GETCOL8(texV, nextRowTexV) GETCOL8_LINEAR(texV, nextRowTexV)
  #define GETCOL16(texV, nextRowTexV) GETCOL16_LINEAR(texV, nextRowTexV)
  #define GETCOL32(texV, nextRowTexV) GETCOL32_LINEAR(texV, nextRowTexV)
#elif (R_DRAWCOLUMN_PIPELINE & RDC_ROUNDED)
  #define GETCOL8(texV, nextRowTexV) GETCOL8_ROUNDED(texV, nextRowTexV)
  #define GETCOL16(texV, nextRowTexV) GETCOL16_ROUNDED(texV, nextRowTexV)
  #define GETCOL32(texV, nextRowTexV) GETCOL32_ROUNDED(texV, nextRowTexV)
#else
  #define GETCOL8(texV, nextRowTexV) GETCOL8_POINT(texV, nextRowTexV)
  #define GETCOL16(texV, nextRowTexV) GETCOL16_POINT(texV, nextRowTexV)
  #define GETCOL32(texV, nextRowTexV) GETCOL32_POINT(texV, nextRowTexV)
#endif
  
//---------------------------------------------------------------------------
// Set up the screen data type macros and the final pipeline bindings
//---------------------------------------------------------------------------
#if (R_DRAWCOLUMN_PIPELINE & RDC_8BITS)
  // 8 bit
  #define R_DRAWCOLUMN_SCRNTYPE byte
  #define SETDESTBLENDED(col) SETDESTBLENDED8(col)
  #define GETCOL(texV, nextRowTexV) GETCOL8(texV, nextRowTexV)
  #define TOPLEFT rdrawvars.topleft_byte
  #define SETDESTFUZZED(col) SETDESTFUZZED8(col)
  
#elif (R_DRAWCOLUMN_PIPELINE & RDC_16BITS)
  // 16 bit
  #define R_DRAWCOLUMN_SCRNTYPE short
  #define SETDESTBLENDED(col) SETDESTBLENDED16(col)
  #define GETCOL(texV, nextRowTexV) GETCOL16(texV, nextRowTexV)
  #define TOPLEFT rdrawvars.topleft_short
  #define SETDESTFUZZED(col) SETDESTFUZZED16(col)
  
#elif (R_DRAWCOLUMN_PIPELINE & RDC_32BITS)
  // 32 bit
  #define R_DRAWCOLUMN_SCRNTYPE int
  #define SETDESTBLENDED(col) SETDESTBLENDED32(col)
  #define GETCOL(texV, nextRowTexV) GETCOL32(texV, nextRowTexV)
  #define TOPLEFT rdrawvars.topleft_int
  #define SETDESTFUZZED(col) SETDESTFUZZED32(col)
  
#endif

//---------------------------------------------------------------------------
// Translucent-or-opaque macros
#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
  #define SETDESTCOL(texV, nextRowTexV) SETDESTBLENDED(GETCOL(texV, nextRowTexV))
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
  #define SETDESTCOL(texV, nextRowTexV) SETDESTFUZZED(dest[fuzzoffset[ (++fuzzpos >= FUZZTABLE) ? fuzzpos = 0 : fuzzpos ]])
#else
  #define SETDESTCOL(texV, nextRowTexV) *dest = GETCOL(texV, nextRowTexV)
#endif

//---------------------------------------------------------------------------
// 8-bit filtering needs an incrementing screen Y coord
#if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ) || ((R_DRAWCOLUMN_PIPELINE & RDC_BILINEAR) && (R_DRAWCOLUMN_PIPELINE & RDC_8BITS))
  #define INCSCREENY screenY++;
#else
  #define INCSCREENY
#endif

//---------------------------------------------------------------------------
// R_DrawColumn*
//---------------------------------------------------------------------------
void R_DRAWCOLUMN_FUNCNAME() {
  int count;
  register R_DRAWCOLUMN_SCRNTYPE *dest;
  register fixed_t frac;
  int screenY;
  
#if (R_DRAWCOLUMN_PIPELINE & RDC_DITHERZ)
  const byte *colormapAndNextColormap[2] = { dcvars.colormap, dcvars.nextcolormap };
  int fracz = ((dcvars.z >> 6) & 0xff);
#endif

#if ((R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT) || (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)) && !(R_DRAWCOLUMN_PIPELINE & RDC_8BITS)
  R_DRAWCOLUMN_SCRNTYPE srcColor;
#endif

#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
  const byte *sourceAndNextSource[2] = { dcvars.source, dcvars.nextsource };

  // drop back to point filtering if we're minifying
  if (dcvars.iscale > rdrawvars.magThresh) {
    getPointFilteredUVFunc(R_DRAWCOLUMN_FUNCTYPE)();
    return;
  }
#endif
    
#if (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
  // Adjust borders
  if (!dcvars.yl) dcvars.yl = 1;
  if (dcvars.yh == viewheight-1) dcvars.yh = viewheight - 2;
#endif
  
  // leban 1/17/99:
  // removed the + 1 here, adjusted the if test, and added an increment
  // later.  this helps a compiler pipeline a bit better.  the x86
  // assembler also does this.
  count = dcvars.yh - dcvars.yl;

  // leban 1/17/99:
  // this case isn't executed too often.  depending on how many instructions
  // there are between here and the second if test below, this case could
  // be moved down and might save instructions overall.  since there are
  // probably different wads that favor one way or the other, i'll leave
  // this alone for now.
  if (count < 0) return; // Zero length, column does not exceed a pixel.

  count++;
  
  screenY = dcvars.yl;

#ifdef RANGECHECK
  if ((unsigned)dcvars.x >= (unsigned)dcvars.targetwidth
      || dcvars.yl < 0
      || dcvars.yh >= dcvars.targetheight)
    I_Error("R_DrawColumn: %i to %i at %i", dcvars.yl, dcvars.yh, dcvars.x);
#endif

  // Framebuffer destination address.
  dest = TOPLEFT + dcvars.yl*dcvars.targetwidth + dcvars.x;

  // Determine scaling, which is the only mapping to be done.
#define  fracstep dcvars.iscale
  frac = dcvars.texturemid + (dcvars.yl-centery)*fracstep;


#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
  #if ((R_DRAWCOLUMN_PIPELINE & RDC_ROUNDED) || (R_DRAWCOLUMN_PIPELINE & RDC_8BITS))
    // 8 bit dithering and rounded filtering uses filter_fracu: 0 -> 0xff
    filter_fracu = (dcvars.source == dcvars.nextsource) ? 0 : (dcvars.texu>>8) & 0xff;
  #else
    // true-color filtering uses filter_fracu: 0 -> 0xffff    
    filter_fracu = (dcvars.source == dcvars.nextsource) ? 0 : dcvars.texu & 0xffff;    
  #endif
#endif

  if (dcvars.drawingmasked && rdrawvars.maskedColumnEdgeType == RDRAW_MASKEDCOLUMNEDGE_SLOPED) {
    // slope the top and bottom column edge based on the fractional u coordinate
    // and dcvars.topslope and dcvars.bottomslope, which were set in R_DrawMaskedColumn
    // in r_things.c
    if (dcvars.yl != 0) {
      if (dcvars.topslope > 0) {
        // [/#]
        int shift = ((0xffff-(dcvars.texu&0xffff))/dcvars.iscale);
        dest += dcvars.targetwidth * shift;
        count -= shift;
        frac += 0xffff-(dcvars.texu&0xffff);
      }
      else if (dcvars.topslope < 0) {
        // [#\]
        int shift = ((dcvars.texu&0xffff)/dcvars.iscale);
        dest += dcvars.targetwidth * shift;
        count -= shift;
        frac += dcvars.texu&0xffff;
      }
    }
    if (dcvars.yh != viewheight-1) {
      if (dcvars.bottomslope > 0) {
        // [#/]
        count -= ((0xffff-(dcvars.texu&0xffff))/dcvars.iscale);
      }
      else if (dcvars.bottomslope < 0) {
        // [\#]
        count -= ((dcvars.texu&0xffff)/dcvars.iscale);
      }
    }
    if (count <= 0) return;  
  }
      
  // Inner loop that does the actual texture mapping,
  if (dcvars.texheight == 128) {
    #define FIXEDT_128MASK ((127<<FRACBITS)|0xffff)
    while (count--) {
      SETDESTCOL(frac&FIXEDT_128MASK, (frac+FRACUNIT)&FIXEDT_128MASK);
      INCSCREENY
      dest += dcvars.targetwidth;
      frac += fracstep;
    }
  } 
  else if (dcvars.texheight == 0) {
    // cph - another special case
    while (count--) {
      SETDESTCOL(frac, (frac+FRACUNIT));
      INCSCREENY
      dest += dcvars.targetwidth;
      frac += fracstep;
    }
  } 
  else {
    register unsigned heightmask = dcvars.texheight-1; // CPhipps - specify type
    if (! (dcvars.texheight & heightmask) ) {
      // texture height is a power of 2 -- killough
      while (count>0) {
        #define FIXEDT_HEIGHTMASK ((heightmask<<FRACBITS)|0xffff)
        SETDESTCOL(frac&FIXEDT_HEIGHTMASK, (frac+FRACUNIT)&FIXEDT_HEIGHTMASK);
        INCSCREENY
        dest += dcvars.targetwidth;
        frac += fracstep;
        count--;
      }
    }
    else {
      fixed_t nextfrac = 0;
      
      heightmask++;
      heightmask <<= FRACBITS;

      if (frac < 0) while ((frac += heightmask) <  0);
      else while (frac >= (int)heightmask) frac -= heightmask;

#define INCFRAC(f) if ((f += fracstep) >= (int)heightmask) f -= heightmask;
      
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
      nextfrac = frac + FRACUNIT;
      while (nextfrac >= (int)heightmask) nextfrac -= heightmask;
#endif
      
      while (count > 0) {
        // heightmask is the Tutti-Frutti fix -- killough
        SETDESTCOL(frac, nextfrac);
        INCSCREENY      
        dest += dcvars.targetwidth;
        INCFRAC(frac);
#if (R_DRAWCOLUMN_PIPELINE & (RDC_BILINEAR|RDC_ROUNDED))
        INCFRAC(nextfrac); 
#endif     
        count--;
      }
    }
  }
} 
#undef fracstep

//---------------------------------------------------------------------------
// Prepare for the next include
#undef R_DRAWCOLUMN_PIPELINE
#undef R_DRAWCOLUMN_FUNCNAME