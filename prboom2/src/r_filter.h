#ifndef R_FILTER_H
#define R_FILTER_H
//---------------------------------------------------------------------------
// Functions/Macros for filtering support - POPE
//---------------------------------------------------------------------------
extern unsigned int filter_fracu;
extern unsigned int filter_tempColor;

//---------------------------------------------------------------------------
// Select the dither matrix. See r_filter.c for more detail. Most of these
// are either very similar or too wonky for real use.
#define DITHER_TYPE 1

//---------------------------------------------------------------------------
#if (DITHER_TYPE == -1)
  #define DITHER_DIM 8
#elif (DITHER_TYPE == 0)
  #define DITHER_DIM 4
#elif (DITHER_TYPE == 1) 
  // standard smooth box matrix, looks the best
  #define DITHER_DIM 4
#elif (DITHER_TYPE == 2)
  #define DITHER_DIM 8
#elif (DITHER_TYPE == 3)
  #define DITHER_DIM 8
#elif (DITHER_TYPE == 6)
  #define DITHER_DIM 8
#elif (DITHER_TYPE == 7)
  #define DITHER_DIM 16
#elif (DITHER_TYPE == 10)
  // stylized posterizing matrix
  #define DITHER_DIM 16
#endif


//---------------------------------------------------------------------------
extern byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM];

//---------------------------------------------------------------------------
// These should really be functions, but C doesn't support inline functions
// so they're nice confusing macros instead.
//---------------------------------------------------------------------------
// Use the dither matrix to determine whether a pixel is on or off based
// on the overall intensity we're trying to simulate
#define filter_getDitheredPixelLevel(x, y, intensity) \
  ((filter_ditherMatrix[(y)&(DITHER_DIM-1)][(x)&(DITHER_DIM-1)] < (intensity)) ? 1 : 0)

// Dither macro for 3 levels of intensity
#define filter_getDitheredPixelLevel2(x, y, intensity) \
  ((filter_ditherMatrix[(y)&(DITHER_DIM-1)][(x)&(DITHER_DIM-1)] < (((intensity)*2)&0xff)) ? \
  (((intensity)&0x80) ? 1 : 0) + 1 : \
  (((intensity)&0x80) ? 1 : 0) + 0)

//---------------------------------------------------------------------------
// Choose current pixel or next pixel down based on dither of the fractional
// texture V coord. texV is not a true fractional texture coord, so it
// has to be converted using ((texV) - dcvars.yl) >> 8), which was empirically
// derived. the "-dcvars.yl" is apparently required to offset some minor
// shaking in coordinate y-axis and prevents dithering seams
#define FILTER_GETV(x,y,texV,nextRowTexV) \
  (filter_getDitheredPixelLevel(x, y, (((texV) - dcvars.yl) >> 8)&0xff) ? ((nextRowTexV)>>FRACBITS) : ((texV)>>FRACBITS))

//---------------------------------------------------------------------------
// Choose current column or next column to the right based on dither of the 
// fractional texture U coord
#define filter_getDitheredForColumn(x, y, texV, nextRowTexV) \
  sourceAndNextSource[(filter_getDitheredPixelLevel(x, y, filter_fracu))][FILTER_GETV(x,y,texV,nextRowTexV)]
  
//---------------------------------------------------------------------------
// This is the horrendous macro version of the function commented out of 
// r_filter.c. It does a bilinear blend on the four source texels for a 
// given u and v
#define filter_getFilteredForColumn32(colormap, texV, nextRowTexV) ( \
  VID_INTPAL( colormap[ dcvars.nextsource[(nextRowTexV)>>FRACBITS] ],   (filter_fracu*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ dcvars.source[(nextRowTexV)>>FRACBITS] ],       ((0xffff-filter_fracu)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ dcvars.source[(texV)>>FRACBITS] ],              ((0xffff-filter_fracu)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ dcvars.nextsource[(texV)>>FRACBITS] ],          (filter_fracu*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ))

//---------------------------------------------------------------------------
// The 16 bit method of the filtering doesn't really maintain enough 
// accuracy for discerning viewers, but the alternative requires converting
// from 32 bit, which is slow and requires both the intPalette and the 
// shortPalette to be in memory at the same time.
#define filter_getFilteredForColumn16(colormap, texV, nextRowTexV) ( \
  VID_SHORTPAL( colormap[ dcvars.nextsource[(nextRowTexV)>>FRACBITS ] ],  (filter_fracu*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ dcvars.source[(nextRowTexV)>>FRACBITS] ],       ((0xffff-filter_fracu)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ dcvars.source[(texV)>>FRACBITS] ],              ((0xffff-filter_fracu)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ dcvars.nextsource[(texV)>>FRACBITS] ],          (filter_fracu*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ))

//---------------------------------------------------------------------------
// Same as for column, only using the dsvars.* globals
#define filter_getFilteredForSpan32(colormap, texU, texV) ( \
  VID_INTPAL( colormap[ dsvars.source[ ((((texU)+FRACUNIT)>>16)&0x3f) | ((((texV)+FRACUNIT)>>10)&0xfc0) ] ],  (((texU)&0xffff)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_INTPAL( colormap[ dsvars.source[ (((texU)>>16)&0x3f) | ((((texV)+FRACUNIT)>>10)&0xfc0) ] ],             ((0xffff-((texU)&0xffff))*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_INTPAL( colormap[ dsvars.source[ (((texU)>>16)&0x3f) | (((texV)>>10)&0xfc0) ] ],                        ((0xffff-((texU)&0xffff))*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_INTPAL( colormap[ dsvars.source[ ((((texU)+FRACUNIT)>>16)&0x3f) | (((texV)>>10)&0xfc0) ] ],             (((texU)&0xffff)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS)))

//---------------------------------------------------------------------------
// Use 16 bit addition here since it's a little faster and the defects from
// such low-accuracy blending are less visible on spans
#define filter_getFilteredForSpan16(colormap, texU, texV) ( \
  VID_SHORTPAL( colormap[ dsvars.source[ ((((texU)+FRACUNIT)>>16)&0x3f) | ((((texV)+FRACUNIT)>>10)&0xfc0) ] ],  (((texU)&0xffff)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_SHORTPAL( colormap[ dsvars.source[ (((texU)>>16)&0x3f) | ((((texV)+FRACUNIT)>>10)&0xfc0) ] ],             ((0xffff-((texU)&0xffff))*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_SHORTPAL( colormap[ dsvars.source[ (((texU)>>16)&0x3f) | (((texV)>>10)&0xfc0) ] ],                        ((0xffff-((texU)&0xffff))*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS)) + \
  VID_SHORTPAL( colormap[ dsvars.source[ ((((texU)+FRACUNIT)>>16)&0x3f) | (((texV)>>10)&0xfc0) ] ],             (((texU)&0xffff)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS)))


//---------------------------------------------------------------------------
// Now for translated...
#define filter_getFilteredForColumn32_Translated(transmap, colormap, texV, nextRowTexV) ( \
  VID_INTPAL( colormap[ transmap[dcvars.nextsource[(nextRowTexV)>>FRACBITS]] ],   (filter_fracu*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ transmap[dcvars.source[(nextRowTexV)>>FRACBITS]] ],       ((0xffff-filter_fracu)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ transmap[dcvars.source[(texV)>>FRACBITS]] ],              ((0xffff-filter_fracu)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_INTPAL( colormap[ transmap[dcvars.nextsource[(texV)>>FRACBITS]] ],          (filter_fracu*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ))

//---------------------------------------------------------------------------
#define filter_getFilteredForColumn16_Translated(transmap, colormap, texV, nextRowTexV) ( \
  VID_SHORTPAL( colormap[ transmap[dcvars.nextsource[(nextRowTexV)>>FRACBITS ]] ],  (filter_fracu*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ transmap[dcvars.source[(nextRowTexV)>>FRACBITS]] ],       ((0xffff-filter_fracu)*((texV)&0xffff))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ transmap[dcvars.source[(texV)>>FRACBITS]] ],              ((0xffff-filter_fracu)*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ) + \
  VID_SHORTPAL( colormap[ transmap[dcvars.nextsource[(texV)>>FRACBITS]] ],          (filter_fracu*(0xffff-((texV)&0xffff)))>>(32-VID_COLORWEIGHTBITS) ))

//---------------------------------------------------------------------------
// The above macros were implemented originally as functions, then compressed
// to macros for speed concerns. The function definitions are left commented
// in this header and r_filter.c for legible reference.
//---------------------------------------------------------------------------
// short filter_getFilteredForColumn16(const byte *colormap, fixed_t texV, fixed_t nextRowTexV);
// int filter_getFilteredForColumn32(const byte *colormap, fixed_t texV, fixed_t nextRowTexV);
// short filter_getFilteredForSpan16(const byte *colormap, unsigned int texU, unsigned int texV);
// int filter_getFilteredForSpan32(const byte *colormap, unsigned int texU, unsigned int texV);

//---------------------------------------------------------------------------

#endif