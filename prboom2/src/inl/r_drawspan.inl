
//---------------------------------------------------------------------------
#undef GETCOL8_DEPTH_POINT
#undef GETCOL8_DEPTH_LINEAR
#undef GETCOL8_DEPTH

#undef GETMAP_DEPTH_POINT
#undef GETMAP_DEPTH_LINEAR
#undef GETMAP_DEPTH

#undef GETCOL8_POINT
#undef GETCOL8_LINEAR
#undef GETCOL8

#undef GETCOL16_POINT
#undef GETCOL16_LINEAR
#undef GETCOL16

#undef GETCOL32_POINT
#undef GETCOL32_LINEAR
#undef GETCOL32

#undef TOPLEFT
#undef GETCOL
#undef R_DRAWSPAN_SCRNTYPE

//---------------------------------------------------------------------------
#define GETCOL8_DEPTH_POINT(col) dsvars.colormap[(col)]
#define GETCOL8_DEPTH_LINEAR(col) (colormapAndNextColormap[filter_getDitheredPixelLevel(dsvars.x1+count, dsvars.y, fracz)][(col)])

#define GETMAP_DEPTH_POINT dsvars.colormap
#define GETMAP_DEPTH_LINEAR (colormapAndNextColormap[filter_getDitheredPixelLevel(dsvars.x1+count, dsvars.y, fracz)])

#if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)  
  #define GETCOL8_DEPTH(col) GETCOL8_DEPTH_LINEAR(col)
  #define GETMAP_DEPTH GETMAP_DEPTH_LINEAR
#else
  #define GETCOL8_DEPTH(col) GETCOL8_DEPTH_POINT(col)
  #define GETMAP_DEPTH GETMAP_DEPTH_POINT
#endif
  
//---------------------------------------------------------------------------
#define GETCOL8_POINT(col) GETCOL8_DEPTH(col)
#define GETCOL8_LINEAR(col) GETCOL8_DEPTH(col)

#define GETCOL16_POINT(col) VID_SHORTPAL(GETCOL8_DEPTH(col), VID_COLORWEIGHTMASK)
#define GETCOL16_LINEAR(col) filter_getFilteredForSpan16(GETMAP_DEPTH, xfrac, yfrac);

#define GETCOL32_POINT(col) VID_INTPAL(GETCOL8_DEPTH(col), VID_COLORWEIGHTMASK)
#define GETCOL32_LINEAR(col) filter_getFilteredForSpan32(GETMAP_DEPTH, xfrac, yfrac);

#if (R_DRAWSPAN_PIPELINE & RDC_BILINEAR)  
  #define GETCOL8(col) GETCOL8_LINEAR(col)
  #define GETCOL16(col) GETCOL16_LINEAR(col)
  #define GETCOL32(col) GETCOL32_LINEAR(col)
#else
  #define GETCOL8(col) GETCOL8_POINT(col)
  #define GETCOL16(col) GETCOL16_POINT(col)
  #define GETCOL32(col) GETCOL32_POINT(col)
#endif

//---------------------------------------------------------------------------
// Set up the screen data type macros and the final pipeline bindings
//---------------------------------------------------------------------------
#if (R_DRAWSPAN_PIPELINE & RDC_8BITS)
  // 8 bit
  #define R_DRAWSPAN_SCRNTYPE byte
  #define GETCOL(col) GETCOL8(col)
  #define TOPLEFT rdrawvars.topleft_byte

#elif (R_DRAWSPAN_PIPELINE & RDC_16BITS)
  // 16 bit
  #define R_DRAWSPAN_SCRNTYPE short
  #define GETCOL(col) GETCOL16(col)
  #define TOPLEFT rdrawvars.topleft_short

#elif (R_DRAWSPAN_PIPELINE & RDC_32BITS)
  // 32 bit
  #define R_DRAWSPAN_SCRNTYPE int
  #define GETCOL(col) GETCOL32(col)
  #define TOPLEFT rdrawvars.topleft_int

#endif



//---------------------------------------------------------------------------
// R_DrawSpan
//---------------------------------------------------------------------------
void R_DRAWSPAN_FUNCNAME() {
  register unsigned int count,xfrac = dsvars.xfrac,yfrac = dsvars.yfrac;
  int x = dsvars.x1;
  R_DRAWSPAN_SCRNTYPE *dest;
#if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)
  int fracz;
  const byte *colormapAndNextColormap[2] = { dsvars.colormap, dsvars.nextcolormap };
#endif

#if (R_DRAWCOLUMN_PIPELINE & RDC_BILINEAR)
  // drop back to point filtering if we're minifying
  // 49152 = FRACUNIT * 0.75
  if (dsvars.xfrac > 49152 && dsvars.yfrac > 49152) {
    getPointFilteredUVFunc(R_DRAWCOLUMN_FUNCTYPE)();
    return;
  }
#endif

  dest = TOPLEFT + dsvars.y*SCREENWIDTH + dsvars.x1;
  count = dsvars.x2 - dsvars.x1 + 1;
  
#if (R_DRAWSPAN_PIPELINE & RDC_DITHERZ)
  fracz = dsvars.z >> 12;
  fracz &= 0xff;
  //fracz >>= (8-VID_COLORWEIGHTBITS);
#endif

  while (count) {
#if !(R_DRAWSPAN_PIPELINE & RDC_8BITS) && (R_DRAWSPAN_PIPELINE & RDC_BILINEAR)
    // truecolor bilinear filtered
    *dest++ = GETCOL(0); 

#else
    // 8 bit or truecolor point sampling
    register unsigned xtemp = xfrac >> 16;
    register unsigned ytemp = yfrac >> 10;
    register unsigned spot;

#if (R_DRAWSPAN_PIPELINE & RDC_BILINEAR)
    // 8 bit bilinear
    xtemp += (filter_getDitheredPixelLevel(dsvars.x1+count, dsvars.y, ((xfrac>>8)&0xff)));
    ytemp += 0x40*(filter_getDitheredPixelLevel(dsvars.x1+count, dsvars.y, ((yfrac>>8)&0xff)));
#endif
    ytemp &= 0xfc0;
    xtemp &= 0x3f;
    spot = xtemp | ytemp;    
    *dest++ = GETCOL(dsvars.source[spot]);
#endif

    xfrac += dsvars.xstep;
    yfrac += dsvars.ystep;
    count--;
  }
}

//---------------------------------------------------------------------------
// Prepare for the next include
#undef R_DRAWSPAN_PIPELINE
#undef R_DRAWSPAN_FUNCNAME
