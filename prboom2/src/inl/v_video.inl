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
  const patch_t *patch, TPlotRect destRect, const TPlotRect clampRect,
  TRDrawFilterType filter, const byte *colorTranslationTable,
  byte *destBuffer, int bufferWidth, int bufferHeight
) {
  const column_t *column, *nextcolumn, *prevcolumn;
  fixed_t yfrac, xfrac, srcX;
  int srcColumnIndex, dx;
  void (*columnFunc)();
  int patchHasHolesOrIsNonRectangular;
  
  // choose the right R_DrawColumn pipeline  
  columnFunc = R_GetExactDrawFunc(
    colorTranslationTable ? RDRAW_PIPELINE_COL_TRANSLATED : RDRAW_PIPELINE_COL_STANDARD, 
    V_VIDEO_BITS, filter, RDRAW_FILTER_POINT
  );
  
  patchHasHolesOrIsNonRectangular = getPatchHasAHoleOrIsNonRectangular(patch);

  // calc the fractional stepping  
  xfrac = FixedDiv((destRect.right-destRect.left)<<FRACBITS, patch->width<<FRACBITS);
  yfrac = FixedDiv((destRect.bottom-destRect.top)<<FRACBITS, patch->height<<FRACBITS);
  
  // setup R_DrawColumn globals for our needs
  repointDrawColumnGlobals(
    destBuffer, bufferWidth, bufferHeight, 
    FixedDiv(FRACUNIT, yfrac), colorTranslationTable, 
    patchHasHolesOrIsNonRectangular && filter == RDRAW_FILTER_LINEAR
  );
  
  if (false && filter == RDRAW_FILTER_LINEAR) {
    // bias the texture u coordinate
    if (patchHasHolesOrIsNonRectangular) srcX = (FRACUNIT>>1);
    else srcX = (patch->width<<FRACBITS)-(FRACUNIT>>1);
  }
  else {
    srcX = 0;
  }
  
  for (dx=destRect.left; dx<destRect.right; dx++) {
    dcvars.x = dx;
    if (dcvars.x < clampRect.left) continue;
    if (dcvars.x >= clampRect.right) break;
    
    dcvars.texu = srcX % (patch->width<<FRACBITS);
    srcColumnIndex = (srcX>>FRACBITS) % patch->width;
    srcX += FixedDiv(FRACUNIT, xfrac);

    column = (const column_t *)((const byte *)patch + patch->columnofs[srcColumnIndex]);
    prevcolumn = ((srcColumnIndex-1) < 0) ? 0 : (const column_t*)((const byte*)patch + LONG(patch->columnofs[srcColumnIndex-1]));
    
    if ((srcColumnIndex+1) < patch->width) {
      // no loop for nextcolumn
      nextcolumn = (const column_t*)((const byte*)patch + LONG(patch->columnofs[srcColumnIndex+1]));
    }
    else {
      // nextcolumn would loop around or if we're nonrectangular, there is no nextcolumn
      nextcolumn = patchHasHolesOrIsNonRectangular ? 0 : (const column_t*)((const byte*)patch + LONG(patch->columnofs[0]));
    }
      
    while (column->topdelta != 0xff) {
      dcvars.yl = destRect.top + ((FixedMul(column->topdelta<<FRACBITS, yfrac))>>FRACBITS);
      dcvars.yh = destRect.top + ((FixedMul((column->topdelta+column->length)<<FRACBITS, yfrac))>>FRACBITS);

      // clamp
      if (dcvars.yh >= clampRect.bottom) dcvars.yh = clampRect.bottom-1;
      if (dcvars.yl < clampRect.top) dcvars.yl = clampRect.top;
      
      // set up our top and bottom sloping
      dcvars.topslope = R_GetColumnEdgeSlope(prevcolumn, nextcolumn, column->topdelta);
      dcvars.bottomslope = R_GetColumnEdgeSlope(prevcolumn, nextcolumn, column->topdelta+column->length);
      
      dcvars.texheight = column->length;
      dcvars.source = (const byte*)column + 3;
      if (!patchHasHolesOrIsNonRectangular && nextcolumn) dcvars.nextsource = (const byte*)nextcolumn + 3;
      else dcvars.nextsource = dcvars.source;
      
      if (filter == RDRAW_FILTER_LINEAR) {
        // (FRACUNIT>>1) = empirical shift
        dcvars.texturemid = - (FRACUNIT>>1) - (dcvars.yl-centery)*dcvars.iscale;
      }
      else {
        dcvars.texturemid = -(dcvars.yl-centery)*dcvars.iscale;
      }
      
      columnFunc();
      
      // move to next post
      column = (const column_t *)((byte *)column + column->length + 4);  
    }
  }  
 
  revertDrawColumnGlobals();
}

//---------------------------------------------------------------------------
// FUNC_V_PlotPatchNum
//---------------------------------------------------------------------------
void FUNC_V_PlotPatchNum(
  int patchNum, TPlotRect destRect, const TPlotRect clampRect,
  TRDrawFilterType filter, const byte *colorTranslationTable,
  byte *destBuffer, int bufferWidth, int bufferHeight
) {
  const patch_t *patch = (const patch_t*)W_CacheLumpNum(patchNum);
  FUNC_V_PlotPatch(patch, destRect, clampRect, filter, colorTranslationTable, destBuffer, bufferWidth, bufferHeight);
  W_UnlockLumpNum(patchNum);  
}

//---------------------------------------------------------------------------
// V_PlotTexture
//---------------------------------------------------------------------------
void FUNC_V_PlotTextureNum(
  int textureNum, int x, int y, int width, int height, TRDrawFilterType filter,
  byte *destBuffer, int bufferWidth, int bufferHeight
) {
  texture_t *texture = textures[textureNum];
  int p, patchWidth, patchHeight, patchNum;
  const patch_t *patch;
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
    patch = (const patch_t*)W_CacheLumpNum(patchNum);
    patchWidth = patch->width;
    patchHeight = patch->height;
    W_UnlockLumpNum(patchNum);
    
    destRect.left = x + (FixedDiv(texture->patches[p].originx<<FRACBITS, xfrac)>>FRACBITS);
    destRect.right = destRect.left + (FixedDiv(patchWidth<<FRACBITS, xfrac)>>FRACBITS);
    
    destRect.top = y + (FixedDiv(texture->patches[p].originy<<FRACBITS, yfrac)>>FRACBITS);
    destRect.bottom = destRect.top + (FixedDiv(patchHeight<<FRACBITS, yfrac)>>FRACBITS);
    
    FUNC_V_PlotPatchNum(
      texture->patches[p].patch, destRect, clampRect, filter, 0,
      destBuffer, bufferWidth, bufferHeight
    );
  }
}


//---------------------------------------------------------------------------
byte *FUNC_V_GetPlottedPatch(int patchNum, int width, int height, TRDrawFilterType filter, const byte *colorTranslationTable
#if V_VIDEO_BITS == 8  
, byte clearColor) {
#else
) {
#endif
  byte *destBuffer;
  int bufferSize = width*height*sizeof(V_VIDEO_SCRNTYPE);
  TPlotRect rect = { 0, 0, width, height };
  
  destBuffer = malloc(bufferSize);
  
#if V_VIDEO_BITS == 8  
  memset(destBuffer, clearColor, bufferSize);  
#else
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
#endif
  
  FUNC_V_PlotPatchNum(patchNum, rect, rect, filter, colorTranslationTable, destBuffer, width, height);  
  
#if V_VIDEO_BITS == 32
  {
    int i;
    // invert alphas (assumes ARGB or ABGR)
    for (i=3; i<bufferSize; i+=4) destBuffer[i] = 0xff-destBuffer[i];
  }
#endif

  return destBuffer;
}


//---------------------------------------------------------------------------
byte *FUNC_V_GetPlottedTexture(int textureNum, int width, int height, TRDrawFilterType filter
#if V_VIDEO_BITS == 8  
, byte clearColor) {
#else
) {
#endif
  byte *destBuffer;
  int bufferSize = width*height*sizeof(V_VIDEO_SCRNTYPE);

  destBuffer = malloc(bufferSize);
  
#if V_VIDEO_BITS == 8  
  memset(destBuffer, clearColor, bufferSize);  
#else
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
#endif
  
  FUNC_V_PlotTextureNum(textureNum, 0, 0, width, height, filter, destBuffer, width, height);  

#if V_VIDEO_BITS == 32
  {
    int i;
    // invert alphas (assumes ARGB or ABGR)
    for (i=3; i<bufferSize; i+=4) destBuffer[i] = 0xff-destBuffer[i];
  }
#endif
    
  return destBuffer;
}

//---------------------------------------------------------------------------
// V_DrawMemPatch
//---------------------------------------------------------------------------
void FUNC_V_DrawMemPatch(int x, int y, int scrn, const patch_t *patch, int cm, enum patch_translation_e flags) {
  int width, height;
  const byte *trans;  
  TPlotRect destRect;
  TPlotRect clampRect = { 0, 0, SCREENWIDTH, SCREENHEIGHT };

  width = patch->width;
  height = patch->height;
  
  x -= patch->leftoffset;
  y -= patch->topoffset;
  destRect.left = (x) * SCREENWIDTH / 320;
  destRect.right = (x+width) * SCREENWIDTH / 320;
  destRect.top = (y) * SCREENHEIGHT / 200;
  destRect.bottom = (y+height) * SCREENHEIGHT / 200;
  
  if (flags & VPT_TRANS) {
    if (cm<CR_LIMIT) trans = colrngs[cm];
    else trans = translationtables + 256*((cm-CR_LIMIT)-1);
  }
  else {
    trans = 0;
  }

  FUNC_V_PlotPatch(patch, destRect, clampRect, RDRAW_FILTER_POINT, trans, screens[scrn].data, SCREENWIDTH, SCREENHEIGHT);
}

//---------------------------------------------------------------------------
// V_DrawNumPatch
//---------------------------------------------------------------------------
void FUNC_V_DrawNumPatch(int x, int y, int scrn, int lump,
         int cm, enum patch_translation_e flags) {
  
  FUNC_V_DrawMemPatch(x, y, scrn, (const patch_t*)W_CacheLumpNum(lump), cm, flags);
  W_UnlockLumpNum(lump); 
}

//---------------------------------------------------------------------------
// Prepare for the next include
#undef V_VIDEO_BITS
