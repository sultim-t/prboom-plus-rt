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
#else
  // 8 bit
  #define V_VIDEO_SCRNTYPE byte
  #define V_VIDEO_GETCOL(col) col
  #define FUNC_V_CopyRect       V_CopyRect8
  #define FUNC_V_FillRect       V_FillRect8
  #define FUNC_V_DrawMemPatch   V_DrawMemPatch8
  #define FUNC_V_DrawNumPatch   V_DrawNumPatch8
  #define FUNC_V_DrawBlock      V_DrawBlock8
  #define FUNC_V_DrawBackground V_DrawBackground8
  #define FUNC_V_PlotPixel      V_PlotPixel8
#endif


//---------------------------------------------------------------------------
// V_DrawMemPatch
//---------------------------------------------------------------------------
// CPhipps - unifying patch drawing routine, handles all cases and combinations
//  of stretching, flipping and translating
//
// This function is big, hopefully not too big that gcc can't optimise it well.
// In fact it packs pretty well, there is no big performance lose for all this merging;
// the inner loops themselves are just the same as they always were
// (indeed, laziness of the people who wrote the 'clones' of the original V_DrawPatch
//  means that their inner loops weren't so well optimised, so merging code may even speed them).
//---------------------------------------------------------------------------
void FUNC_V_DrawMemPatch(int x, int y, int scrn, const patch_t *patch, int cm, enum patch_translation_e flags) {
  const byte *trans;
  
  if (cm<CR_LIMIT)
    trans=colrngs[cm];
  else
    trans=translationtables + 256*((cm-CR_LIMIT)-1);
  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  // CPhipps - auto-no-stretch if not high-res
  if (flags & VPT_STRETCH)
    if ((SCREENWIDTH==320) && (SCREENHEIGHT==200))
      flags &= ~VPT_STRETCH;

  // CPhipps - null translation pointer => no translation
  if (!trans)
    flags &= ~VPT_TRANS;

#ifdef RANGECHECK
  if (x<0
      ||x+SHORT(patch->width) > ((flags & VPT_STRETCH) ? 320 : SCREENWIDTH)
      || y<0
      || y+SHORT(patch->height) > ((flags & VPT_STRETCH) ? 200 :  SCREENHEIGHT))
    // killough 1/19/98: improved error message:
    I_Error("V_DrawMemPatch: Patch origin %d,%d exceeds LFB"
            "Bad V_DrawMemPatch (flags=%u)", x, y, flags);
#endif

  if (!(flags & VPT_STRETCH)) {
    int             col;
    const column_t *column;
    V_VIDEO_SCRNTYPE      *desttop = (V_VIDEO_SCRNTYPE*)(screens[scrn])+y*SCREENWIDTH+x;
    unsigned int    w = SHORT(patch->width);

    //if (!scrn) V_MarkRect (x, y, w, SHORT(patch->height));

    w--; // CPhipps - note: w = width-1 now, speeds up flipping

    for (col=0 ; (unsigned int)col<=w ; desttop++, col++) {
      column = (column_t *)((byte *)patch +
          LONG(patch->columnofs[(flags & VPT_FLIP) ? w-col : col]));

  // step through the posts in a column
      while (column->topdelta != 0xff ) {
  // killough 2/21/98: Unrolled and performance-tuned

  register const byte *source = (byte *)column + 3;
  register V_VIDEO_SCRNTYPE *dest = desttop + column->topdelta*SCREENWIDTH;
  register int count = column->length;

  if (!(flags & VPT_TRANS)) {
    if ((count-=4)>=0)
      do {
        register byte s0,s1;
        s0 = source[0];
        s1 = source[1];
        dest[0] = V_VIDEO_GETCOL(s0);
        dest[SCREENWIDTH] = V_VIDEO_GETCOL(s1);
        dest += SCREENWIDTH*2;
        s0 = source[2];
        s1 = source[3];
        source += 4;
        dest[0] = V_VIDEO_GETCOL(s0);
        dest[SCREENWIDTH] = V_VIDEO_GETCOL(s1);
        dest += SCREENWIDTH*2;
      } while ((count-=4)>=0);
    if (count+=4)
      do {
        *dest = V_VIDEO_GETCOL(*source++);
        dest += SCREENWIDTH;
      } while (--count);
    column = (column_t *)(source+1); //killough 2/21/98 even faster
  } else {
    // CPhipps - merged translation code here
    if ((count-=4)>=0)
      do {
        register byte s0,s1;
        s0 = source[0];
        s1 = source[1];
        s0 = trans[s0];
        s1 = trans[s1];
        dest[0] = V_VIDEO_GETCOL(s0);
        dest[SCREENWIDTH] = V_VIDEO_GETCOL(s1);
        dest += SCREENWIDTH*2;
        s0 = source[2];
        s1 = source[3];
        s0 = trans[s0];
        s1 = trans[s1];
        source += 4;
        dest[0] = V_VIDEO_GETCOL(s0);
        dest[SCREENWIDTH] = V_VIDEO_GETCOL(s1);
        dest += SCREENWIDTH*2;
      } while ((count-=4)>=0);
    if (count+=4)
      do {
        *dest = V_VIDEO_GETCOL(trans[*source++]);
        dest += SCREENWIDTH;
      } while (--count);
    column = (column_t *)(source+1);
  }
      }
    }
  }
  else {
    // CPhipps - move stretched patch drawing code here
    //         - reformat initialisers, move variables into inner blocks

    V_VIDEO_SCRNTYPE *desttop;
    int   col;
    int   w = (SHORT( patch->width ) << 16) - 1; // CPhipps - -1 for faster flipping
    int   stretchx, stretchy;
    int   DX  = (SCREENWIDTH<<16)  / 320;
    int   DXI = (320<<16)          / SCREENWIDTH;
    int   DY  = (SCREENHEIGHT<<16) / 200;
    register int DYI = (200<<16)   / SCREENHEIGHT;
    int   DY2, DYI2;

    stretchx = ( x * DX ) >> 16;
    stretchy = ( y * DY ) >> 16;
    DY2  = DY / 2;
    DYI2 = DYI* 2;

    //if (!scrn) V_MarkRect ( stretchx, stretchy, (SHORT( patch->width ) * DX ) >> 16, (SHORT( patch->height) * DY ) >> 16 );

    desttop = (V_VIDEO_SCRNTYPE*)(screens[scrn]) + stretchy * SCREENWIDTH +  stretchx;

    for ( col = 0; col <= w; x++, col+=DXI, desttop++ ) {
      const column_t *column;
      {
  unsigned int d = patch->columnofs[(flags & VPT_FLIP) ? ((w - col)>>16): (col>>16)];
  column = (column_t*)((byte*)patch + LONG(d));
      }

      while ( column->topdelta != 0xff ) {
  register const byte *source = ( byte* ) column + 3;
  register V_VIDEO_SCRNTYPE  *dest = desttop + (( column->topdelta * DY ) >> 16 ) * SCREENWIDTH;
  register int         count  = ( column->length * DY ) >> 16;
  register int         srccol = 0x8000;

  if (flags & VPT_TRANS)
    while (count--) {
      *dest  =  V_VIDEO_GETCOL(trans[source[srccol>>16]]);
      dest  +=  SCREENWIDTH;
      srccol+=  DYI;
    }
  else
    while (count--) {
      *dest  =  V_VIDEO_GETCOL(source[srccol>>16]);
      dest  +=  SCREENWIDTH;
      srccol+=  DYI;
    }
  column = ( column_t* ) (( byte* ) column + ( column->length ) + 4 );
      }
    }
  }
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
// V_PlotPixel
//---------------------------------------------------------------------------
void FUNC_V_PlotPixel(int scrn, int x, int y, byte color) {
  ((V_VIDEO_SCRNTYPE*)screens[scrn])[x+SCREENWIDTH*y] = V_VIDEO_GETCOL(color);
}

//---------------------------------------------------------------------------
// V_FillRect
//---------------------------------------------------------------------------
void FUNC_V_FillRect(int scrn, int x, int y, int width, int height, byte colour) {
  V_VIDEO_SCRNTYPE *dest = (V_VIDEO_SCRNTYPE*)(screens[scrn]) + x + y*SCREENWIDTH;

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

  src = (V_VIDEO_SCRNTYPE*)(screens[srcscrn])+SCREENWIDTH*srcy+srcx;
  dest = (V_VIDEO_SCRNTYPE*)(screens[destscrn])+SCREENWIDTH*desty+destx;

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

    dest = (V_VIDEO_SCRNTYPE*)(screens[scrn]) + y*SCREENWIDTH+x;
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

    dest = (V_VIDEO_SCRNTYPE*)(screens[scrn]) + y*SCREENWIDTH+x;

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
// Prepare for the next include
#undef V_VIDEO_BITS
