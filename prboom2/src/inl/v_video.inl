//---------------------------------------------------------------------------
// This file contains the inline, bit-depth variant functions that used
// to reside in v_video.c. Valid switches when including this function are
// 
// #define V_VIDEO_BITS 8  -OR-
// #define V_VIDEO_BITS 16 -OR-
// #define V_VIDEO_BITS 32
//---------------------------------------------------------------------------
#undef V_VIDEO_SCRNTYPE
#undef V_VIDEO_GETCOL
#undef FUNC_V_CopyRect
#undef FUNC_V_FillRect
#undef FUNC_V_DrawMemPatch
#undef FUNC_V_DrawNumPatch
#undef FUNC_V_DrawBlock
#undef FUNC_V_DrawBackground
#undef FUNC_V_PlotPixel
#undef FUNC_V_PlotPatch
#undef FUNC_V_PlotTexture
#undef FUNC_V_PlotPatch
#undef FUNC_V_PlotPatchNum
#undef FUNC_V_PlotTextureNum
#undef FUNC_V_GetPlottedPatch
#undef FUNC_V_GetPlottedTexture
  
//---------------------------------------------------------------------------
#if (V_VIDEO_BITS == 32)
  // 32 bit
  #define V_VIDEO_SCRNTYPE int
  #define V_VIDEO_GETCOL(col) VID_INTPAL((col), VID_COLORWEIGHTMASK)
  #define FUNC_V_CopyRect       V_CopyRect32
  #define FUNC_V_FillRect       V_FillRect32
  #define FUNC_V_DrawMemPatch   V_DrawMemPatch32
  #define FUNC_V_DrawNumPatch   V_DrawNumPatch32
  #define FUNC_V_DrawBlock      V_DrawBlock32
  #define FUNC_V_DrawBackground V_DrawBackground32
  #define FUNC_V_PlotPixel      V_PlotPixel32
  #define FUNC_V_PlotPatch      V_PlotPatch32
  #define FUNC_V_PlotPatchNum   V_PlotPatchNum32
  #define FUNC_V_PlotTextureNum V_PlotTextureNum32
  #define FUNC_V_GetPlottedPatch    V_GetPlottedPatch32
  #define FUNC_V_GetPlottedTexture  V_GetPlottedTexture32
#elif  (V_VIDEO_BITS == 16)
  // 16 bit
  #define V_VIDEO_SCRNTYPE short
  #define V_VIDEO_GETCOL(col) VID_SHORTPAL((col), VID_COLORWEIGHTMASK)
  #define FUNC_V_CopyRect       V_CopyRect16
  #define FUNC_V_FillRect       V_FillRect16
  #define FUNC_V_DrawMemPatch   V_DrawMemPatch16
  #define FUNC_V_DrawNumPatch   V_DrawNumPatch16
  #define FUNC_V_DrawBlock      V_DrawBlock16
  #define FUNC_V_DrawBackground V_DrawBackground16
  #define FUNC_V_PlotPixel      V_PlotPixel16
  #define FUNC_V_PlotPatch      V_PlotPatch16
  #define FUNC_V_PlotPatchNum   V_PlotPatchNum16
  #define FUNC_V_PlotTextureNum V_PlotTextureNum16
  #define FUNC_V_GetPlottedPatch    V_GetPlottedPatch16
  #define FUNC_V_GetPlottedTexture  V_GetPlottedTexture16
#else
  // 8 bit
  #define V_VIDEO_BITS 8
  #define V_VIDEO_SCRNTYPE byte
  #define V_VIDEO_GETCOL(col) col
  #define FUNC_V_CopyRect       V_CopyRect8
  #define FUNC_V_FillRect       V_FillRect8
  #define FUNC_V_DrawMemPatch   V_DrawMemPatch8
  #define FUNC_V_DrawNumPatch   V_DrawNumPatch8
  #define FUNC_V_DrawBlock      V_DrawBlock8
  #define FUNC_V_DrawBackground V_DrawBackground8
  #define FUNC_V_PlotPixel      V_PlotPixel8
  #define FUNC_V_PlotPatch      V_PlotPatch8
  #define FUNC_V_PlotPatchNum   V_PlotPatchNum8
  #define FUNC_V_PlotTextureNum V_PlotTextureNum8
  #define FUNC_V_GetPlottedPatch    V_GetPlottedPatch8
  #define FUNC_V_GetPlottedTexture  V_GetPlottedTexture8
#endif


//---------------------------------------------------------------------------
// V_PlotPixel
//---------------------------------------------------------------------------
void FUNC_V_PlotPixel(int scrn, int x, int y, byte color) {
  ((V_VIDEO_SCRNTYPE*)screens[scrn].data)[x+SCREENWIDTH*y] = V_VIDEO_GETCOL(color);
}

//---------------------------------------------------------------------------
// V_FillRect
//---------------------------------------------------------------------------
void FUNC_V_FillRect(int scrn, int x, int y, int width, int height, byte colour) {
  V_VIDEO_SCRNTYPE *dest = (V_VIDEO_SCRNTYPE*)(screens[scrn].data) + x + y*SCREENWIDTH;

#if (V_VIDEO_BITS == 8)
  // 8 bit optimized
  while (height--) {
    memset(dest, colour, width);
    dest += SCREENWIDTH;
  }
#else
  // true color
  while (height--) {
    int i;
    V_VIDEO_SCRNTYPE trueColor = V_VIDEO_GETCOL(colour);
    for (i=0; i<width; i++) {
      dest[i] = trueColor;
    }
    dest += SCREENWIDTH;
  }
#endif

}

//---------------------------------------------------------------------------
// V_CopyRect
//---------------------------------------------------------------------------
// Copies a source rectangle in a screen buffer to a destination
// rectangle in another screen buffer. Source origin in srcx,srcy,
// destination origin in destx,desty, common size in width and height.
// Source buffer specfified by srcscrn, destination buffer by destscrn.
//
// Marks the destination rectangle on the screen dirty.
//---------------------------------------------------------------------------
void FUNC_V_CopyRect(int srcx, int srcy, int srcscrn, int width,
                int height, int destx, int desty, int destscrn,
                enum patch_translation_e flags)
{
  V_VIDEO_SCRNTYPE *src;
  V_VIDEO_SCRNTYPE *dest;

  if (flags & VPT_STRETCH)
  {
    srcx=srcx*SCREENWIDTH/320;
    srcy=srcy*SCREENHEIGHT/200;
    width=width*SCREENWIDTH/320;
    height=height*SCREENHEIGHT/200;
    destx=destx*SCREENWIDTH/320;
    desty=desty*SCREENHEIGHT/200;
  }

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width >SCREENWIDTH
      || srcy<0
      || srcy+height>SCREENHEIGHT
      ||destx<0||destx+width >SCREENWIDTH
      || desty<0
      || desty+height>SCREENHEIGHT)
    I_Error ("V_CopyRect: Bad arguments");
#endif

  //V_MarkRect (destx, desty, width, height);

  src = (V_VIDEO_SCRNTYPE*)(screens[srcscrn].data)+SCREENWIDTH*srcy+srcx;
  dest = (V_VIDEO_SCRNTYPE*)(screens[destscrn].data)+SCREENWIDTH*desty+destx;

  for ( ; height>0 ; height--)
    {
      memcpy (dest, src, width*sizeof(V_VIDEO_SCRNTYPE));
      src += SCREENWIDTH;
      dest += SCREENWIDTH;
    }
}

//---------------------------------------------------------------------------
// V_DrawBlock
//---------------------------------------------------------------------------
// Draw a linear block of pixels into the view buffer.
//
// The bytes at src are copied in linear order to the screen rectangle
// at x,y in screenbuffer scrn, with size width by height.
//
// The destination rectangle is marked dirty.
//
// No return.
//
// CPhipps - modified  to take the patch translation flags. For now, only stretching is
//  implemented, to support highres in the menus
//---------------------------------------------------------------------------
void FUNC_V_DrawBlock(int x, int y, int scrn, int width, int height,
     const byte *src, enum patch_translation_e flags)
{
  V_VIDEO_SCRNTYPE *dest;
  
#ifdef RANGECHECK
  if (x<0
      ||x+width >((flags & VPT_STRETCH) ? 320 : SCREENWIDTH)
      || y<0
      || y+height>((flags & VPT_STRETCH) ? 200 : SCREENHEIGHT))
    I_Error ("V_DrawBlock: Bad V_DrawBlock");

  if (flags & (VPT_TRANS | VPT_FLIP))
    I_Error("V_DrawBlock: Unsupported flags (%u)", flags);
#endif

  if (flags & VPT_STRETCH) {
    int     s_width;
    fixed_t dx = (320 << FRACBITS) / SCREENWIDTH;

    x = (x * SCREENWIDTH) / 320; y = (y * SCREENHEIGHT) / 200;
    s_width = (width * SCREENWIDTH) / 320; height = (height * SCREENHEIGHT) / 200;

    //if (!scrn) V_MarkRect (x, y, width, height);

    dest = (V_VIDEO_SCRNTYPE*)(screens[scrn].data) + y*SCREENWIDTH+x;
    // x & y no longer needed

    while (height--) {
      const byte *const src_row = src + width * ((height * 200) / SCREENHEIGHT);
      V_VIDEO_SCRNTYPE  *const dst_row = dest + SCREENWIDTH * height;
      fixed_t           tx;

      for (x=0, tx=0; x<s_width; x++, tx+=dx)
        dst_row[x] = V_VIDEO_GETCOL(src_row[tx >> FRACBITS]);
    }
  } else {
    // V_MarkRect (x, y, width, height);

    dest = (V_VIDEO_SCRNTYPE*)(screens[scrn].data) + y*SCREENWIDTH+x;

#if (V_VIDEO_BITS == 8)
    // 8 bit optimized
    while (height--) {
      memcpy (dest, src, width);
      src += width;
      dest += SCREENWIDTH;
    } 
#else
    while (height--) {
      // true color
      int i;
      for (i=0; i<width; i++) {
        dest[i] = V_VIDEO_GETCOL(src[i]);
      }
      src += width;
      dest += SCREENWIDTH;
    }
#endif
  }
}

//---------------------------------------------------------------------------
// V_DrawBackground
//---------------------------------------------------------------------------
// V_DrawBackground tiles a 64x64 patch over the entire screen, providing the
// background for the Help and Setup screens, and plot text betwen levels.
// cphipps - used to have M_DrawBackground, but that was used the framebuffer
// directly, so this is my code from the equivalent function in f_finale.c
//
//---------------------------------------------------------------------------
void FUNC_V_DrawBackground(const char* flatname, int scrn)
{
  /* erase the entire screen to a tiled background */
  const byte *src;
  int         x,y;
  int         lump;

  // killough 4/17/98:
  src = W_CacheLumpNum(lump = firstflat + R_FlatNumForName(flatname));

  FUNC_V_DrawBlock(0, 0, scrn, 64, 64, src, 0);

  for (y=0 ; y<SCREENHEIGHT ; y+=64)
    for (x=y ? 0 : 64; x<SCREENWIDTH ; x+=64)
      V_CopyRect(0, 0, scrn, ((SCREENWIDTH-x) < 64) ? (SCREENWIDTH-x) : 64,
     ((SCREENHEIGHT-y) < 64) ? (SCREENHEIGHT-y) : 64, x, y, scrn, VPT_NONE);
  W_UnlockLumpNum(lump);
}

//---------------------------------------------------------------------------
// V_PlotPatch
//---------------------------------------------------------------------------
void FUNC_V_PlotPatch(
  const TPatch *patch, TPlotRect destRect, const TPlotRect clampRect,
  TRDrawFilterType filter, TRDrawColumnMaskedEdgeType slope, 
  const byte *colorTranslationTable, boolean translucent,
  byte *destBuffer, int bufferWidth, int bufferHeight
) {
  const TPatchColumn *column, *nextColumn, *prevColumn;
  fixed_t yfrac, xfrac, srcX, srcYShift;
  int srcColumnIndex, dx, postIndex;
  TRDrawPipelineType type;
  void (*columnFunc)();
  
  // choose the right R_DrawColumn pipeline  
  if (translucent) {
    type = RDRAW_PIPELINE_COL_TRANSLUCENT;
	tranmap = main_tranmap;
  } else {
    if (colorTranslationTable)
      type = RDRAW_PIPELINE_COL_TRANSLATED;
    else
      type = RDRAW_PIPELINE_COL_STANDARD;
  }
  columnFunc = R_GetExactDrawFunc(type, V_VIDEO_BITS, filter, RDRAW_FILTER_POINT);
  
  // calc the fractional stepping  
  xfrac = FixedDiv((destRect.right-destRect.left)<<FRACBITS, patch->width<<FRACBITS);
  yfrac = FixedDiv((destRect.bottom-destRect.top)<<FRACBITS, patch->height<<FRACBITS);
  
  // setup R_DrawColumn globals for our needs
  repointDrawColumnGlobals(
    destBuffer, bufferWidth, bufferHeight, 
    FixedDiv(FRACUNIT, yfrac), colorTranslationTable, 
    patch->isNotTileable, slope
  );
  
  if (filter == RDRAW_FILTER_LINEAR) {
    // bias the texture u coordinate
    if (patch->isNotTileable) srcX = (FRACUNIT>>1);
    else srcX = (patch->width<<FRACBITS)-(FRACUNIT>>1);
  }
  else {
    srcX = 0;
  }
  
  for (dx=destRect.left; dx<destRect.right; dx++) {
    dcvars.x = dx;
    if (dcvars.x >= clampRect.right) break;
    
    dcvars.texu = srcX % (patch->width<<FRACBITS);
    srcColumnIndex = (srcX>>FRACBITS) % patch->width;
    srcX += FixedDiv(FRACUNIT, xfrac);

    // ignore this column if it's to the left of our clampRect
    // only after we've updated our source X coord
    if (dcvars.x < clampRect.left) continue;
    
    column = R_GetPatchColumn(patch, srcColumnIndex);
    prevColumn = R_GetPatchColumn(patch, srcColumnIndex-1);
    nextColumn = R_GetPatchColumn(patch, srcColumnIndex+1);
      
    for (postIndex=0; postIndex<column->numPosts; postIndex++) {
      const TPatchPost *post = &column->posts[postIndex];
      
      dcvars.yl = destRect.top + ((FixedMul(post->startY<<FRACBITS, yfrac))>>FRACBITS);
      // -FRACUNIT/2 == rounding factor
      dcvars.yh = destRect.top + ((FixedMul((post->startY+post->length)<<FRACBITS, yfrac)-FRACUNIT/2)>>FRACBITS);
      dcvars.edgeSlope = post->edgeSloping;
      
      // clamp and set up our sloping
      if (dcvars.yh >= clampRect.bottom) {
        dcvars.yh = clampRect.bottom-1;
        dcvars.edgeSlope &= ~RDRAW_EDGESLOPE_BOT_MASK;
      }
      if (dcvars.yl < clampRect.top) {
        srcYShift = FixedDiv((clampRect.top - dcvars.yl)<<FRACBITS, yfrac);
        dcvars.yl = clampRect.top;
        dcvars.edgeSlope &= ~RDRAW_EDGESLOPE_TOP_MASK;
      }
      else {
        srcYShift = 0;
      }
            
      dcvars.texheight = patch->height;
      dcvars.source = column->pixels + post->startY;
      dcvars.nextsource = nextColumn ? nextColumn->pixels + post->startY : dcvars.source;
      dcvars.prevsource = prevColumn ? prevColumn->pixels + post->startY : dcvars.source;
      
      if (filter == RDRAW_FILTER_LINEAR) {
        // (FRACUNIT>>1) = empirical shift
        dcvars.texturemid = srcYShift - (FRACUNIT>>1) - (dcvars.yl-centery)*dcvars.iscale;
      }
      else {
        dcvars.texturemid = (srcYShift - (dcvars.yl-centery)*dcvars.iscale);
      }
      
      columnFunc();
    }
  }  
 
  revertDrawColumnGlobals();
}

//---------------------------------------------------------------------------
// FUNC_V_PlotPatchNum
//---------------------------------------------------------------------------
void FUNC_V_PlotPatchNum(
  int patchNum, TPlotRect destRect, const TPlotRect clampRect,
  TRDrawFilterType filter, TRDrawColumnMaskedEdgeType slope, 
  const byte *colorTranslationTable, boolean translucent,
  byte *destBuffer, int bufferWidth, int bufferHeight
) {
  const TPatch *patch = R_GetPatch(patchNum);
  FUNC_V_PlotPatch(patch, destRect, clampRect, filter, slope, colorTranslationTable, translucent, destBuffer, bufferWidth, bufferHeight);
}

//---------------------------------------------------------------------------
// V_PlotTexture
//---------------------------------------------------------------------------
void FUNC_V_PlotTextureNum(
  int textureNum, int x, int y, int width, int height, TRDrawFilterType filter,
  TRDrawColumnMaskedEdgeType slope, byte *destBuffer, int bufferWidth, int bufferHeight
) {
  texture_t *texture = textures[textureNum];
  int p, patchWidth, patchHeight, patchNum;
  const TPatch *patch;
  fixed_t xfrac, yfrac;
  TPlotRect destRect, clampRect;
  
  xfrac = FixedDiv(texture->width<<FRACBITS, width<<FRACBITS);
  yfrac = FixedDiv(texture->height<<FRACBITS, height<<FRACBITS);
  
  clampRect.left = max(0, x);
  clampRect.right = min(bufferWidth, x+width);
  clampRect.top = max(0,y);
  clampRect.bottom = min(bufferHeight, y+height);
  
  for (p=0; p<texture->patchcount; p++) {
    patchNum = texture->patches[p].patch;
    patch = R_GetPatch(patchNum);
    patchWidth = patch->width;
    patchHeight = patch->height;
    
    destRect.left = x + ((FixedDiv(texture->patches[p].originx<<FRACBITS, xfrac)+(FRACUNIT>>1))>>FRACBITS);
    destRect.right = destRect.left + ((FixedDiv(patchWidth<<FRACBITS, xfrac)+(FRACUNIT>>1))>>FRACBITS);
    
    destRect.top = y + ((FixedDiv(texture->patches[p].originy<<FRACBITS, yfrac)+(FRACUNIT>>1))>>FRACBITS);
    destRect.bottom = destRect.top + ((FixedDiv(patchHeight<<FRACBITS, yfrac)+(FRACUNIT>>1))>>FRACBITS);
    
    FUNC_V_PlotPatch(
      patch, destRect, clampRect, filter, slope, 0, false,
      destBuffer, bufferWidth, bufferHeight
    );
  }
}


//---------------------------------------------------------------------------
byte *FUNC_V_GetPlottedPatch(
  int patchNum, int plotWidth, int plotHeight, 
  int bufferWidth, int bufferHeight, TRDrawFilterType filter, 
  TRDrawColumnMaskedEdgeType slope, const byte *colorTranslationTable    
#if V_VIDEO_BITS == 8  
, byte clearColor
#elif V_VIDEO_BITS == 32
, int convertToBGRA
#endif
) {
  byte *destBuffer;
  int bufferSize;
  TPlotRect destRect;
  TPlotRect clampRect = { 0, 0, bufferWidth, bufferHeight };
  
  if (plotWidth > bufferWidth) plotWidth = bufferWidth;
  if (plotHeight > bufferHeight) plotHeight = bufferHeight;
  
  destRect.left = destRect.top = 0;
  destRect.right = plotWidth;
  destRect.bottom = plotHeight;
  
  bufferSize = bufferWidth*bufferHeight*sizeof(V_VIDEO_SCRNTYPE);
  destBuffer = malloc(bufferSize);
  
#if V_VIDEO_BITS == 8  
  memset(destBuffer, clearColor, bufferSize);  
#else
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
#endif
  
  FUNC_V_PlotPatchNum(patchNum, destRect, clampRect, filter, slope, colorTranslationTable, false, destBuffer, bufferWidth, bufferHeight);  
  
#if V_VIDEO_BITS == 32
  finalizeTrueColorBuffer(destBuffer, bufferWidth*bufferHeight, convertToBGRA);
#endif

  return destBuffer;
}


//---------------------------------------------------------------------------
byte *FUNC_V_GetPlottedTexture(
  int textureNum, int plotWidth, int plotHeight, 
  int bufferWidth, int bufferHeight, 
  TRDrawFilterType filter, TRDrawColumnMaskedEdgeType slope
#if V_VIDEO_BITS == 8  
, byte clearColor
#elif V_VIDEO_BITS == 32
, int convertToBGRA
#endif
) {
  byte *destBuffer;
  int bufferSize;

  if (plotWidth > bufferWidth) plotWidth = bufferWidth;
  if (plotHeight > bufferHeight) plotHeight = bufferHeight;
  
  bufferSize = bufferWidth*bufferHeight*sizeof(V_VIDEO_SCRNTYPE);
  destBuffer = malloc(bufferSize);
  
#if V_VIDEO_BITS == 8  
  memset(destBuffer, clearColor, bufferSize);  
#else
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
#endif
  
  FUNC_V_PlotTextureNum(textureNum, 0, 0, plotWidth, plotHeight, filter, slope, destBuffer, bufferWidth, bufferHeight);

#if V_VIDEO_BITS == 32
  finalizeTrueColorBuffer(destBuffer, bufferWidth*bufferHeight, convertToBGRA);
#endif
    
  return destBuffer;
}

//---------------------------------------------------------------------------
// V_DrawNumPatch
//---------------------------------------------------------------------------
void FUNC_V_DrawNumPatch(int x, int y, int scrn, int lump,
         int cm, enum patch_translation_e flags) {

  const TPatch *patch = R_GetPatch(lump);
  int width, height;
  const byte *trans;  
  boolean translucent = false;
  TPlotRect destRect;
  TPlotRect clampRect = { 0, 0, SCREENWIDTH, SCREENHEIGHT };

  width = patch->width;
  height = patch->height;
  
  x -= patch->leftOffset;
  y -= patch->topOffset;
  
  destRect.left = x;
  destRect.right = x+width;
  destRect.top = y;
  destRect.bottom = y+height;
  
  if (flags & VPT_STRETCH) {
    destRect.left = (destRect.left * SCREENWIDTH + 160) / 320;
    destRect.right = (destRect.right * SCREENWIDTH + 160) / 320;
    destRect.top = (destRect.top * SCREENHEIGHT + 100) / 200;
    destRect.bottom = (destRect.bottom * SCREENHEIGHT + 100) / 200;
  }
  
  if (flags & VPT_TRANS) {
    if (cm<CR_LIMIT) trans = colrngs[cm];
    else trans = translationtables + 256*((cm-CR_LIMIT)-1);
  }
  else {
    trans = 0;
  }

  if (flags & VPT_TRANSLUCENT)
	translucent = true;

  FUNC_V_PlotPatch(
    patch, destRect, clampRect, 
    vid_drawPatchFilterType, vid_drawPatchSlopeType, 
    trans, translucent, screens[scrn].data, SCREENWIDTH, SCREENHEIGHT
  );
}

//---------------------------------------------------------------------------
// Prepare for the next include
#undef V_VIDEO_BITS
