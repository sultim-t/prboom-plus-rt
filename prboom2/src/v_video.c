/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 * $Id: v_video.c,v 1.35 2002/11/23 22:55:51 proff_fs Exp $
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
 *  Gamma correction LUT stuff.
 *  Color range translation support
 *  Functions to draw patches (by post) directly to screen.
 *  Functions to blit a block to the screen.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: v_video.c,v 1.35 2002/11/23 22:55:51 proff_fs Exp $";

#include "doomdef.h"
#include "hu_stuff.h"
#include "r_main.h"
#include "r_draw.h"
#include "m_bbox.h"
#include "w_wad.h"   /* needed for color translation lump lookup */
#include "v_video.h"
#include "i_video.h"
#include "lprintf.h"
#include "r_things.h" // R_GetColumnEdgeSlope POPE 
#include "r_filter.h"

// Each screen is [SCREENWIDTH*SCREENHEIGHT];
TScreenVars screens[6];

/* jff 4/24/98 initialize this at runtime */
const byte *colrngs[CR_LIMIT];

int usegamma = 0;

/*
 * V_InitColorTranslation
 *
 * Loads the color translation tables from predefined lumps at game start
 * No return
 *
 * Used for translating text colors from the red palette range
 * to other colors. The first nine entries can be used to dynamically
 * switch the output of text color thru the HUlib_drawText routine
 * by embedding ESCn in the text to obtain color n. Symbols for n are
 * provided in v_video.h.
 *
 * cphipps - constness of crdef_t stuff fixed
 */

typedef struct {
  const char *name;
  const byte **map;
} crdef_t;

// killough 5/2/98: table-driven approach
static const crdef_t crdefs[] = {
  {"CRBRICK",  &colrngs[CR_BRICK ]},
  {"CRTAN",    &colrngs[CR_TAN   ]},
  {"CRGRAY",   &colrngs[CR_GRAY  ]},
  {"CRGREEN",  &colrngs[CR_GREEN ]},
  {"CRBROWN",  &colrngs[CR_BROWN ]},
  {"CRGOLD",   &colrngs[CR_GOLD  ]},
  {"CRRED",    &colrngs[CR_RED   ]},
  {"CRBLUE",   &colrngs[CR_BLUE  ]},
  {"CRORANGE", &colrngs[CR_ORANGE]},
  {"CRYELLOW", &colrngs[CR_YELLOW]},
  {"CRBLUE2",  &colrngs[CR_BLUE2]},
  {NULL}
};

//---------------------------------------------------------------------------
// killough 5/2/98: tiny engine driven by table above
void V_InitColorTranslation(void) {
  register const crdef_t *p;
  for (p=crdefs; p->name; p++)
    *p->map = W_CacheLumpName(p->name);
}

//---------------------------------------------------------------------------
#ifdef GL_DOOM
static TVidMode vidMode = VID_MODEGL;
#else
static TVidMode vidMode = VID_MODE32;
#endif

//---------------------------------------------------------------------------
TVidMode vid_getMode() { return vidMode; }
int vid_getNumBits() { return vid_getModePixelDepth(vidMode) * 8; }
int vid_getDepth() { return vid_getModePixelDepth(vidMode); }

//---------------------------------------------------------------------------
int vid_getModePixelDepth(TVidMode mode) {
  switch (mode) {
    case VID_MODE8: return 1;
    case VID_MODE16: return 2;
    case VID_MODE32: return 4;
  }
  return 0;
}

//---------------------------------------------------------------------------
TVidMode vid_getModeForNumBits(int numBits) {
  switch (numBits) {
    case 8: return VID_MODE8;
    case 16: return VID_MODE16;
    case 32: return VID_MODE32;
  }
  return 0;
}

//---------------------------------------------------------------------------
int *vid_intPalette = 0;
short *vid_shortPalette = 0;

//---------------------------------------------------------------------------
// Small util funcs to save the R_DrawColumn state - POPE
// (Welcome to the state-machine)
//---------------------------------------------------------------------------
void pushOrPopRDrawState(int push) {
  static TRDrawColumnVars dcvars_saved;
  static TRDrawVars rdrawvars_saved;
    
  if (push) {
    dcvars_saved = dcvars;
    rdrawvars_saved = rdrawvars;
  }
  else {
    rdrawvars = rdrawvars_saved;
    dcvars = dcvars_saved;
  }
}

//---------------------------------------------------------------------------
void repointDrawColumnGlobals(byte *topleft, int targetwidth, int targetheight, fixed_t iscale, const byte *translation, int drawingmasked) {
  pushOrPopRDrawState(1);
  rdrawvars.topleft_byte = topleft;
  rdrawvars.topleft_int = (int*)topleft;
  rdrawvars.topleft_short = (short*)topleft;  
  dcvars.targetwidth = targetwidth;
  dcvars.targetheight = targetheight;
  dcvars.drawingmasked = drawingmasked;
  dcvars.colormap = dcvars.nextcolormap = colormaps[0];  
  dcvars.iscale = iscale;
  dcvars.translation = translation;  
  dcvars.z = 0;
}

//---------------------------------------------------------------------------
void revertDrawColumnGlobals() {
  pushOrPopRDrawState(0);
}

//---------------------------------------------------------------------------
int getPatchHasAHoleOrIsNonRectangular(const patch_t *patch) {
  int x=0, numPosts, lastColumnDelta = 0;
  const column_t *column;
  
  for (x=0; x<patch->width; x++) {
    column = (const column_t *)((const byte *)patch + patch->columnofs[x]);
    if (!x) lastColumnDelta = column->topdelta;
    else if (lastColumnDelta != column->topdelta) return 1;
    
    numPosts = 0;
    while (column->topdelta != 0xff) {
      if (numPosts++) return 1;
      column = (const column_t *)((byte *)column + column->length + 4);
    }
  }
  return 0;    
}

//---------------------------------------------------------------------------
void finalizeTrueColorBuffer(byte *destBuffer, int numPixels, int convertToBGRA) {
  byte r,g,b,a;
  unsigned int color;
  int i;
  unsigned int *destBufferAsInt = (int*)destBuffer;   

  for (i=0; i<numPixels; i++) {
    color = destBufferAsInt[i];
    r = color & (0x000000ff);
    g = (color & (0x0000ff00)) >> 8;
    b = (color & (0x00ff0000)) >> 16;
    a = (color & (0xff000000)) >> 24;
    
    // if alpha is 0xff, then nothing was plotted to this pixel
    if (a) {
      destBufferAsInt[i] = 0; 
      continue;
    }
    
    // alpha was zero, which means R_DrawColumn plotted a pixel
    a = 0xff;
    if (convertToBGRA)
      destBufferAsInt[i] = ((a<<24) | (r<<16) | (g<<8) | b);
    else
      destBufferAsInt[i] = ((a<<24) | (b<<16) | (g<<8) | r);
  }
}

void WRAP_V_DrawLine(fline_t* fl, int color);

//---------------------------------------------------------------------------
// GL wrapping funcs so the parameters match up to the function pointer
// prototypes
//---------------------------------------------------------------------------
#ifdef GL_DOOM
void WRAP_gld_FillRect(int scrn, int x, int y, int width, int height, byte colour)
{
  gld_FillBlock(x,y,width,height,colour);
}
void WRAP_gld_CopyRect(int srcx, int srcy, int srcscrn, int width, int height, int destx, int desty, int destscrn, enum patch_translation_e flags)
{
}
void WRAP_gld_DrawBackground(const char *flatname, int n)
{
  gld_DrawBackground(flatname);
}
void WRAP_gld_DrawPatchFromMem(int x, int y, int scrn, const patch_t *patch, int cm, enum patch_translation_e flags)
{
  gld_DrawPatchFromMem(x,y,patch,cm,flags);
}
void WRAP_gld_DrawNumPatch(int x, int y, int scrn, int lump, int cm, enum patch_translation_e flags)
{
  gld_DrawNumPatch(x,y,lump,cm,flags);
}
void V_PlotPixelGL(int scrn, int x, int y, byte color) {
  gld_DrawLine(x-1, y, x+1, y, color);
  gld_DrawLine(x, y-1, x, y+1, color);
}
void WRAP_gld_DrawBlock(int x, int y, int scrn, int width, int height, const byte *src, enum patch_translation_e flags)
{
}
void WRAP_gld_DrawLine(fline_t* fl, int color)
{
  gld_DrawLine(fl->a.x, fl->a.y, fl->b.x, fl->b.y, color);
}
#endif

//---------------------------------------------------------------------------
// Generate the V_Video functions
//---------------------------------------------------------------------------
#define V_VIDEO_BITS 8
#include "inl/v_video.inl"
#define V_VIDEO_BITS 16
#include "inl/v_video.inl"
#define V_VIDEO_BITS 32
#include "inl/v_video.inl"

//---------------------------------------------------------------------------
// Function pointers for normal access to any of the bit-depth versions
TFunc_V_CopyRect        V_CopyRect;
TFunc_V_FillRect        V_FillRect;
TFunc_V_DrawMemPatch    V_DrawMemPatch;
TFunc_V_DrawNumPatch    V_DrawNumPatch;
TFunc_V_DrawBlock       V_DrawBlock;
TFunc_V_DrawBackground  V_DrawBackground;
TFunc_V_PlotPixel       V_PlotPixel;
TFunc_V_PlotPatch       V_PlotPatch;
TFunc_V_PlotPatchNum    V_PlotPatchNum;
TFunc_V_PlotTextureNum  V_PlotTextureNum;
TFunc_V_DrawLine        V_DrawLine;

//---------------------------------------------------------------------------
// Set Function Pointers
void vid_initMode(TVidMode vd) {
#ifndef GL_DOOM
  vidMode = vd;
#else // GL_DOOM
  vidMode = VID_MODEGL;
#endif
  if (vidMode == VID_MODE8) {
    V_FillRect = V_FillRect8;
    V_CopyRect = V_CopyRect8;
    V_DrawMemPatch = V_DrawMemPatch8;
    V_DrawNumPatch = V_DrawNumPatch8;
    V_DrawBlock = V_DrawBlock8;
    V_PlotPixel = V_PlotPixel8;
    V_DrawBackground = V_DrawBackground8; 
    V_PlotPatch = V_PlotPatch8;
    V_PlotPatchNum = V_PlotPatchNum8;
    V_PlotTextureNum = V_PlotTextureNum8;
    V_DrawLine = WRAP_V_DrawLine;
  }
  else if (vidMode == VID_MODE16) {
    V_FillRect = V_FillRect16;
    V_CopyRect = V_CopyRect16;
    V_DrawMemPatch = V_DrawMemPatch16;
    V_DrawNumPatch = V_DrawNumPatch16;
    V_DrawBlock = V_DrawBlock16;
    V_PlotPixel = V_PlotPixel16;
    V_DrawBackground = V_DrawBackground16;
    V_PlotPatch = V_PlotPatch16;
    V_PlotPatchNum = V_PlotPatchNum16;
    V_PlotTextureNum = V_PlotTextureNum16;
    V_DrawLine = WRAP_V_DrawLine;
  }
  else if (vidMode == VID_MODE32) {
    V_FillRect = V_FillRect32;
    V_CopyRect = V_CopyRect32;
    V_DrawMemPatch = V_DrawMemPatch32;
    V_DrawNumPatch = V_DrawNumPatch32;
    V_DrawBlock = V_DrawBlock32;
    V_PlotPixel = V_PlotPixel32;
    V_DrawBackground = V_DrawBackground32;
    V_PlotPatch = V_PlotPatch32;
    V_PlotPatchNum = V_PlotPatchNum32;
    V_PlotTextureNum = V_PlotTextureNum32;
    V_DrawLine = WRAP_V_DrawLine;
  }
#ifdef GL_DOOM
  else if (vidMode == VID_MODEGL) {
    V_FillRect = WRAP_gld_FillRect;
    V_CopyRect = WRAP_gld_CopyRect;
    V_DrawMemPatch = WRAP_gld_DrawPatchFromMem;
    V_DrawNumPatch = WRAP_gld_DrawNumPatch;
    V_DrawBlock = WRAP_gld_DrawBlock;
    V_PlotPixel = V_PlotPixelGL;
    V_DrawBackground = WRAP_gld_DrawBackground;
    V_DrawLine = WRAP_gld_DrawLine;
  }
#endif
}

//---------------------------------------------------------------------------
void V_AllocScreen(TScreenVars *scrn) {
  if (!scrn->not_on_heap)
    if ((scrn->width * scrn->height) > 0)
      scrn->data = malloc(scrn->width*scrn->height*vid_getDepth());
}

//---------------------------------------------------------------------------
void V_AllocScreens() {
  int i;

  for (i=0; i<NUM_SCREENS; i++)
    V_AllocScreen(&screens[i]);
}

//---------------------------------------------------------------------------
void V_FreeScreen(TScreenVars *scrn) {
  if (!scrn->not_on_heap) {
    free(scrn->data);
    scrn->data = NULL;
  }
}

//---------------------------------------------------------------------------
void V_FreeScreens() {
  int i;

  for (i=0; i<NUM_SCREENS; i++)
    V_FreeScreen(&screens[i]);
}

//---------------------------------------------------------------------------
// byte -> int/short palettes
//---------------------------------------------------------------------------
int *intPalettes = 0;
short *shortPalettes = 0;
static int usegammaOnLastPaletteGeneration = -1;
static int currentPaletteIndex = 0;

//---------------------------------------------------------------------------
// V_UpdateTrueColorPalette
//---------------------------------------------------------------------------
void V_UpdateTrueColorPalette(TVidMode mode) {
  int i, w, p;
  byte r,g,b;
  int nr,ng,nb;
  float t;
  int paletteNum = currentPaletteIndex;
  
  int pplump = W_GetNumForName("PLAYPAL");
  int gtlump = (W_CheckNumForName)("GAMMATBL",ns_prboom);
  register const byte *pal = W_CacheLumpNum(pplump);
  // opengl doesn't use the gamma
  register const byte *const gtable = 
    (const byte *)W_CacheLumpNum(gtlump) + 
    (vid_getMode() == VID_MODEGL ? 0 : 256*(usegamma))
  ;

  int numPals = W_LumpLength(pplump) / (3*256);
  const float dontRoundAbove = 220;
  float roundUpR, roundUpG, roundUpB;
  
  if (usegammaOnLastPaletteGeneration != usegamma) {
    if (intPalettes) free(intPalettes);
    if (shortPalettes) free(shortPalettes);
    intPalettes = 0;
    shortPalettes = 0;
    usegammaOnLastPaletteGeneration = usegamma;      
  }
  
  if (mode == VID_MODE32) {
    if (!intPalettes) {
      // set int palette
      intPalettes = (int*)malloc(numPals*256*sizeof(int)*VID_NUMCOLORWEIGHTS);
      for (p=0; p<numPals; p++) {
        for (i=0; i<256; i++) {
          r = gtable[pal[(256*p+i)*3+0]];
          g = gtable[pal[(256*p+i)*3+1]];
          b = gtable[pal[(256*p+i)*3+2]];
          
          // ideally, we should always round up, but very bright colors
          // overflow the blending adds, so they don't get rounded.
          roundUpR = (r > dontRoundAbove) ? 0 : 0.5f;
          roundUpG = (g > dontRoundAbove) ? 0 : 0.5f;
          roundUpB = (b > dontRoundAbove) ? 0 : 0.5f;
                  
          for (w=0; w<VID_NUMCOLORWEIGHTS; w++) {
            t = (float)(w)/(float)(VID_NUMCOLORWEIGHTS-1);
            nr = (int)(r*t+roundUpR);
            ng = (int)(g*t+roundUpG);
            nb = (int)(b*t+roundUpB);
            intPalettes[((p*256+i)*VID_NUMCOLORWEIGHTS)+w] = (
              (nr<<16) | (ng<<8) | nb
            );
          }
        }
      }
    }
    vid_intPalette = intPalettes + paletteNum*256*VID_NUMCOLORWEIGHTS;
  }
  else if (mode == VID_MODE16) {
    if (!shortPalettes) {
      // set short palette
      shortPalettes = (short*)malloc(numPals*256*sizeof(short)*VID_NUMCOLORWEIGHTS);
      for (p=0; p<numPals; p++) {
        for (i=0; i<256; i++) {
          r = gtable[pal[(256*p+i)*3+0]];
          g = gtable[pal[(256*p+i)*3+1]];
          b = gtable[pal[(256*p+i)*3+2]];
          
          // ideally, we should always round up, but very bright colors
          // overflow the blending adds, so they don't get rounded.
          roundUpR = (r > dontRoundAbove) ? 0 : 0.5f;
          roundUpG = (g > dontRoundAbove) ? 0 : 0.5f;
          roundUpB = (b > dontRoundAbove) ? 0 : 0.5f;
                   
          for (w=0; w<VID_NUMCOLORWEIGHTS; w++) {
            t = (float)(w)/(float)(VID_NUMCOLORWEIGHTS-1);
            nr = (int)((r>>3)*t+roundUpR);
            ng = (int)((g>>2)*t+roundUpG);
            nb = (int)((b>>3)*t+roundUpB);
            shortPalettes[((p*256+i)*VID_NUMCOLORWEIGHTS)+w] = (
              (nr<<11) | (ng<<5) | nb
            );
          }
        }
      }
    }
    vid_shortPalette = shortPalettes + paletteNum*256*VID_NUMCOLORWEIGHTS;
  }       
   
  W_UnlockLumpNum(pplump);
  W_UnlockLumpNum(gtlump);
}


//---------------------------------------------------------------------------
// V_DestroyTrueColorPalette
//---------------------------------------------------------------------------
void V_DestroyTrueColorPalette(TVidMode mode) {
  if (mode == VID_MODE16) {
    if (shortPalettes) free(shortPalettes);
    shortPalettes = 0;
    vid_shortPalette = 0;
  }
  if (mode == VID_MODE32) {
    if (intPalettes) free(intPalettes);
    intPalettes = 0;
    vid_intPalette = 0;
  }
}

//---------------------------------------------------------------------------
void V_DestroyUnusedTrueColorPalettes() {
  if (vid_getMode() != VID_MODE16) V_DestroyTrueColorPalette(VID_MODE16);
  if (vid_getMode() != VID_MODE32) V_DestroyTrueColorPalette(VID_MODE32);  
}

//---------------------------------------------------------------------------
// V_SetPalette
//---------------------------------------------------------------------------
void V_SetPalette(int pal) {
  currentPaletteIndex = pal;
  
#ifndef GL_DOOM
  if (vidMode == VID_MODE8) {
    I_SetPalette(pal);
  }
  else if (vidMode == VID_MODE16 || vidMode == VID_MODE32) {
    I_SetPalette(pal);
    
    // V_SetPalette can be called as part of the gamma setting before
    // we've loaded any wads, which prevents us from reading the palette - POPE
    if (W_CheckNumForName("PLAYPAL") >= 0) {
      V_UpdateTrueColorPalette(vidMode);
    }
  }
  else if (vidMode == VID_MODEGL) {
  }
#else
  gld_SetPalette(pal);
#endif
}

//---------------------------------------------------------------------------
// V_Init
// Allocates the 4 full screen buffers in low DOS memory
//---------------------------------------------------------------------------
void V_Init (void)
{
  int i;

  vid_initMode(vidMode);
  filter_init();

  // reset the all
  for (i = 0; i<NUM_SCREENS; i++) {
    screens[i].data = NULL;
    screens[i].not_on_heap = false;
    screens[i].width = 0;
    screens[i].height = 0;
  }
}

//---------------------------------------------------------------------------
/* cph -
 * V_NamePatchWidth - returns width of a patch.
 * V_NamePatchHeight- returns height of a patch.
 *
 * Doesn't really belong here, but is often used in conjunction with
 *  this code.
 * This is needed to reduce the number of patches being held locked
 *  in memory, since a lot of code was locking and holding pointers
 *  to graphics in order to get this info easily. Also, we do endian
 *  correction here, which reduces the chance of other code forgetting
 *  this.
 */
//---------------------------------------------------------------------------
int V_NumPatchWidth(int lump) {
  int w;
  w = SHORT(((const patch_t*)W_CacheLumpNum(lump))->width);
  W_UnlockLumpNum(lump);
  return w;
}

//---------------------------------------------------------------------------
int V_NumPatchHeight(int lump) {
  int w;
  w = SHORT(((const patch_t*)W_CacheLumpNum(lump))->height);
  W_UnlockLumpNum(lump);
  return w;
}

//---------------------------------------------------------------------------
// V_GetBlock
//---------------------------------------------------------------------------
// Gets a linear block of pixels from the view buffer.
//
// The pixels in the rectangle at x,y in screenbuffer scrn with size
// width by height are linearly packed into the buffer dest.
//---------------------------------------------------------------------------
void V_GetBlock8(int x, int y, int scrn, int width, int height, byte *dest) {
  byte *src;
#ifdef RANGECHECK
  if (x<0 ||x+width >SCREENWIDTH || y<0 || y+height>SCREENHEIGHT) I_Error ("V_GetBlock: Bad arguments");
#endif
  src = (byte*)screens[scrn].data + y*SCREENWIDTH+x;
  while (height--) {
    memcpy (dest, src, width);
    src += SCREENWIDTH;
    dest += width;
  }
}

//---------------------------------------------------------------------------
// CPhipps -
// V_PatchToBlock
//
// Returns a simple bitmap which contains the patch. See-through parts of the
// patch will be undefined (in fact black for now)
//---------------------------------------------------------------------------
// This always draws to and returns an 8-bit byte* buffer - POPE
//---------------------------------------------------------------------------
byte *V_PatchToBlock(const char* name, int cm, enum patch_translation_e flags, unsigned short* width, unsigned short* height) {
  TScreenVars    oldscr = screens[1];
  byte          *block;
  const patch_t *patch;

  if (vidMode == VID_MODEGL) return 0;
  
  screens[1].data = calloc(SCREENWIDTH*SCREENHEIGHT, 1);

  patch = W_CacheLumpName(name);
  V_DrawMemPatch8(SHORT(patch->leftoffset), SHORT(patch->topoffset),1, patch, cm, flags);

#ifdef RANGECHECK
  if (flags & VPT_STRETCH)
    I_Error("V_PatchToBlock: Stretching not supported");
#endif

  *width = SHORT(patch->width); *height = SHORT(patch->height);

  W_UnlockLumpName(name);

  V_GetBlock8(0, 0, 1, *width, *height,
       block = malloc((long)(*width) * (*height)));
  
  free(screens[1].data);
  screens[1] = oldscr;
  return block;
}

//
// V_drawLine()
//
// Draw a line in the frame buffer.
// Classic Bresenham w/ whatever optimizations needed for speed
//
// Passed the frame coordinates of line, and the color to be drawn
// Returns nothing
//
void WRAP_V_DrawLine(fline_t* fl, int color)
{
  register int x;
  register int y;
  register int dx;
  register int dy;
  register int sx;
  register int sy;
  register int ax;
  register int ay;
  register int d;

#ifdef RANGECHECK         // killough 2/22/98    
  static int fuck = 0;

  // For debugging only
  if
  (
       fl->a.x < 0 || fl->a.x >= SCREENWIDTH
    || fl->a.y < 0 || fl->a.y >= SCREENHEIGHT
    || fl->b.x < 0 || fl->b.x >= SCREENWIDTH
    || fl->b.y < 0 || fl->b.y >= SCREENHEIGHT
  )
  {
    //jff 8/3/98 use logical output routine
    lprintf(LO_DEBUG, "fuck %d \r", fuck++);
    return;
  }
#endif

#define PUTDOT(xx,yy,cc) V_PlotPixel(0,xx,yy,(byte)cc)

  dx = fl->b.x - fl->a.x;
  ax = 2 * (dx<0 ? -dx : dx);
  sx = dx<0 ? -1 : 1;

  dy = fl->b.y - fl->a.y;
  ay = 2 * (dy<0 ? -dy : dy);
  sy = dy<0 ? -1 : 1;

  x = fl->a.x;
  y = fl->a.y;

  if (ax > ay)
  {
    d = ay - ax/2;
    while (1)
    {
      PUTDOT(x,y,color);
      if (x == fl->b.x) return;
      if (d>=0)
      {
        y += sy;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {
    d = ax - ay/2;
    while (1)
    {
      PUTDOT(x, y, color);
      if (y == fl->b.y) return;
      if (d >= 0)
      {
        x += sx;
        d -= ay;
      }
      y += sy;
      d += ax;
    }
  }
}

/*
//---------------------------------------------------------------------------
byte *V_GetPlottedPatch8(int patchNum, int width, int height, byte clearColor) {
  byte *destBuffer;
  int bufferSize = width*height;
  TPlotRect rect = { 0, 0, width, height };
  
  destBuffer = malloc(bufferSize);
  memset(destBuffer, clearColor, bufferSize);  
  V_PlotPatch(patchNum, rect, rect, destBuffer, VID_MODE8, width, height);  
  return destBuffer;
}

//---------------------------------------------------------------------------
byte *V_GetPlottedPatch32(int patchNum, int width, int height) {
  byte *destBuffer;
  int bufferSize = width*height*4, i;
  TPlotRect rect = { 0, 0, width, height };
  
  destBuffer = malloc(bufferSize);
  
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
  
  V_PlotPatch(patchNum, rect, rect, destBuffer, VID_MODE32, width, height);  
  
  // invert alphas (assumes ARGB or ABGR)
  for (i=3; i<bufferSize; i+=4) destBuffer[i] = 0xff-destBuffer[i];
  
  return destBuffer;
}


//---------------------------------------------------------------------------
byte *V_GetPlottedTexture8(int textureNum, int width, int height, TRDrawFilterType filter, byte clearColor) {
  byte *destBuffer;
  int bufferSize = width*height;
  
  destBuffer = malloc(bufferSize);  
  memset(destBuffer, clearColor, bufferSize);  
  
  V_PlotTexture(textureNum, 0, 0, width, height, destBuffer, VID_MODE8, width, height);
  
  return destBuffer;
}

//---------------------------------------------------------------------------
byte *V_GetPlottedTexture32(int textureNum, int width, int height, TRDrawFilterType filter) {
  byte *destBuffer;
  int bufferSize = width*height*4, i;
  
  destBuffer = malloc(bufferSize);  
  
  // when plotting, alpha's will be cleared
  memset(destBuffer, 0xff, bufferSize);
  
  V_PlotTexture32(textureNum, 0, 0, width, height, destBuffer, width, height);
  
  // invert alphas (assumes ARGB or ABGR)
  for (i=3; i<bufferSize; i+=4) destBuffer[i] = 0xff-destBuffer[i];
  
  return destBuffer;
}
*/

//---------------------------------------------------------------------------
// Font
//---------------------------------------------------------------------------
extern patchnum_t hu_font[HU_FONTSIZE];

void V_WriteText(unsigned char *s, int x, int y, int gap)
{
  int   w, h;
  unsigned char* ch;
  int colour = CR_DEFAULT;
  unsigned int c;
  int   cx;
  int   cy;

  ch = s;
  cx = x;
  cy = y;
  
  while(1)
  {
    c = *ch++;
    if (!c)
	    break;
    if (c >= FC_BASEVALUE)     // new colour
    {
      colour = c - FC_BASEVALUE;
      continue;
    }
    if (c == '\t')
    {
      cx = (cx/40)+1;
      cx = cx*40;
    }
    if (c == '\n')
	  {
	    cx = x;
      cy += 8;
	    continue;
	  }

    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }

    // haleyjd: was no cx<0 check

    w = SHORT(hu_font[c].width);
    if(cx < 0 || cx+w > 320)
	    break;

    // haleyjd: was no y checking at all!

    h = SHORT(hu_font[c].height);
    if(cy < 0 || cy+h > 200)
	    break;

    V_DrawNumPatch(cx, cy, 0, hu_font[c].lumpnum, colour, VPT_STRETCH | VPT_TRANS);
    //V_DrawPatchTranslated(cx, cy, 0, patch, colour, 0);

    cx+=w+gap;
  }
}

// write text in a particular colour

void V_WriteTextColoured(unsigned char *s, int colour, int x, int y, int gap) {
  char *tempstr = malloc(strlen(s)+3);
  sprintf(tempstr, "%c%s", FC_BASEVALUE+colour, s);
  V_WriteText(tempstr, x, y, gap);
  free(tempstr);
}

// find height(in pixels) of a string
int V_StringHeight(unsigned char *s) {
  int height = 8;  // always at least 8
  // add an extra 8 for each newline found
  while(*s) {
    if (*s == '\n') height += 8;
    s++;
  }
  return height;
}

int V_StringWidth(unsigned char *s, int gap) {
  int length = 0; // current line width
  int longest_width = 0; // line with longest width so far
  unsigned char c;

  for(; *s; s++) {
    c = *s;
    if(c >= FC_BASEVALUE)         // colour
	    continue;
    if(c == '\n')        // newline
  	{
	    if(length > longest_width) longest_width = length;
	    length = 0; // next line;
	    continue;
	  }
    c = toupper(c) - HU_FONTSTART;
    length += (c >= HU_FONTSIZE) ? 4 : (SHORT(hu_font[c].width)+gap);
  }

  if(length > longest_width) longest_width = length; // check last line

  return longest_width;
}
