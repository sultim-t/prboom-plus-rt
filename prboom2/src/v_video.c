/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
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

#include "doomdef.h"
#include "r_main.h"
#include "r_draw.h"
#include "m_bbox.h"
#include "w_wad.h"   /* needed for color translation lump lookup */
#include "v_video.h"
#include "i_video.h"
#include "lprintf.h"

// Each screen is [SCREENWIDTH*SCREENHEIGHT];
byte *screens[6];

/* jff 4/24/98 initialize this at runtime */
const byte *colrngs[CR_LIMIT];

int usegamma;

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

// killough 5/2/98: tiny engine driven by table above
void V_InitColorTranslation(void)
{
  register const crdef_t *p;
  for (p=crdefs; p->name; p++)
    *p->map = W_CacheLumpName(p->name);
}

//
// V_CopyRect
//
// Copies a source rectangle in a screen buffer to a destination
// rectangle in another screen buffer. Source origin in srcx,srcy,
// destination origin in destx,desty, common size in width and height.
// Source buffer specfified by srcscrn, destination buffer by destscrn.
//
// Marks the destination rectangle on the screen dirty.
//
// No return.
//
#ifndef GL_DOOM
void V_CopyRect(int srcx, int srcy, int srcscrn, int width,
                int height, int destx, int desty, int destscrn,
                enum patch_translation_e flags)
{
  byte *src;
  byte *dest;

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

  src = screens[srcscrn]+SCREENWIDTH*srcy+srcx;
  dest = screens[destscrn]+SCREENWIDTH*desty+destx;

  for ( ; height>0 ; height--)
    {
      memcpy (dest, src, width);
      src += SCREENWIDTH;
      dest += SCREENWIDTH;
    }
}
#endif /* GL_DOOM */

/*
 * V_DrawBackground tiles a 64x64 patch over the entire screen, providing the
 * background for the Help and Setup screens, and plot text betwen levels.
 * cphipps - used to have M_DrawBackground, but that was used the framebuffer
 * directly, so this is my code from the equivalent function in f_finale.c
 */
#ifndef GL_DOOM
void V_DrawBackground(const char* flatname, int scrn)
{
  /* erase the entire screen to a tiled background */
  const byte *src;
  byte       *dest;
  int         x,y;
  int         width,height;
  int         lump;

  // killough 4/17/98:
  src = W_CacheLumpNum(lump = firstflat + R_FlatNumForName(flatname));

  /* V_DrawBlock(0, 0, scrn, 64, 64, src, 0); */
  width = height = 64;
  dest = screens[scrn];

  while (height--) {
    memcpy (dest, src, width);
    src += width;
    dest += SCREENWIDTH;
  }
  /* end V_DrawBlock */

  for (y=0 ; y<SCREENHEIGHT ; y+=64)
    for (x=y ? 0 : 64; x<SCREENWIDTH ; x+=64)
      V_CopyRect(0, 0, scrn, ((SCREENWIDTH-x) < 64) ? (SCREENWIDTH-x) : 64,
     ((SCREENHEIGHT-y) < 64) ? (SCREENHEIGHT-y) : 64, x, y, scrn, VPT_NONE);
  W_UnlockLumpNum(lump);
}
#endif

//
// V_Init
//
// Allocates the 4 full screen buffers in low DOS memory
// No return
//

void V_Init (void)
{
  int  i;
  // CPhipps - allocate only 2 screens all the time, the rest can be allocated as and when needed
#define PREALLOCED_SCREENS 2

  // CPhipps - no point in "stick these in low dos memory on PCs" anymore
  // Allocate the screens individually, so I_InitGraphics can release screens[0]
  //  if e.g. it wants a MitSHM buffer instead

  for (i=0 ; i<PREALLOCED_SCREENS ; i++)
    screens[i] = calloc(SCREENWIDTH*SCREENHEIGHT, 1);
  for (; i<4; i++) // Clear the rest (paranoia)
    screens[i] = NULL;
}

//
// V_DrawMemPatch
//
// CPhipps - unifying patch drawing routine, handles all cases and combinations
//  of stretching, flipping and translating
//
// This function is big, hopefully not too big that gcc can't optimise it well.
// In fact it packs pretty well, there is no big performance lose for all this merging;
// the inner loops themselves are just the same as they always were
// (indeed, laziness of the people who wrote the 'clones' of the original V_DrawPatch
//  means that their inner loops weren't so well optimised, so merging code may even speed them).
//
#ifndef GL_DOOM
void V_DrawMemPatch(int x, int y, int scrn, const patch_t *patch,
        int cm, enum patch_translation_e flags)
{
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

  if (x<0
      ||x+SHORT(patch->width) > ((flags & VPT_STRETCH) ? 320 : SCREENWIDTH)
      || y<0
      || y+SHORT(patch->height) > ((flags & VPT_STRETCH) ? 200 :  SCREENHEIGHT))
    // killough 1/19/98: improved error message:
    I_Error("V_DrawMemPatch: Patch (%d,%d)-(%d,%d) exceeds LFB"
            "Bad V_DrawMemPatch (flags=%u)", x, y, x+SHORT(patch->width), y+SHORT(patch->height), flags);

  if (!(flags & VPT_STRETCH)) {
    int             col;
    const column_t *column;
    byte           *desttop = screens[scrn]+y*SCREENWIDTH+x;
    unsigned int    w = SHORT(patch->width);

    w--; // CPhipps - note: w = width-1 now, speeds up flipping

    for (col=0 ; (unsigned int)col<=w ; desttop++, col++) {
      column = (column_t *)((byte *)patch +
          LONG(patch->columnofs[(flags & VPT_FLIP) ? w-col : col]));

  // step through the posts in a column
      while (column->topdelta != 0xff ) {
  // killough 2/21/98: Unrolled and performance-tuned

  register const byte *source = (byte *)column + 3;
  register byte *dest = desttop + column->topdelta*SCREENWIDTH;
  register int count = column->length;

  if (!(flags & VPT_TRANS)) {
    if ((count-=4)>=0)
      do {
        register byte s0,s1;
        s0 = source[0];
        s1 = source[1];
        dest[0] = s0;
        dest[SCREENWIDTH] = s1;
        dest += SCREENWIDTH*2;
        s0 = source[2];
        s1 = source[3];
        source += 4;
        dest[0] = s0;
        dest[SCREENWIDTH] = s1;
        dest += SCREENWIDTH*2;
      } while ((count-=4)>=0);
    if (count+=4)
      do {
        *dest = *source++;
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
        dest[0] = s0;
        dest[SCREENWIDTH] = s1;
        dest += SCREENWIDTH*2;
        s0 = source[2];
        s1 = source[3];
        s0 = trans[s0];
        s1 = trans[s1];
        source += 4;
        dest[0] = s0;
        dest[SCREENWIDTH] = s1;
        dest += SCREENWIDTH*2;
      } while ((count-=4)>=0);
    if (count+=4)
      do {
        *dest = trans[*source++];
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

    byte *desttop;
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

    desttop = screens[scrn] + stretchy * SCREENWIDTH +  stretchx;

    for ( col = 0; col <= w; x++, col+=DXI, desttop++ ) {
      const column_t *column;
      {
  unsigned int d = patch->columnofs[(flags & VPT_FLIP) ? ((w - col)>>16): (col>>16)];
  column = (column_t*)((byte*)patch + LONG(d));
      }

      while ( column->topdelta != 0xff ) {
  register const byte *source = ( byte* ) column + 3;
  register byte       *dest = desttop + (( column->topdelta * DY ) >> 16 ) * SCREENWIDTH;
  register int         count  = ( column->length * DY ) >> 16;
  register int         srccol = 0;

  if (flags & VPT_TRANS)
    while (count--) {
      *dest  =  trans[source[srccol>>16]];
      dest  +=  SCREENWIDTH;
      srccol+=  DYI;
    }
  else
    while (count--) {
      *dest  =  source[srccol>>16];
      dest  +=  SCREENWIDTH;
      srccol+=  DYI;
    }
  column = ( column_t* ) (( byte* ) column + ( column->length ) + 4 );
      }
    }
  }
}
#endif // GL_DOOM

// CPhipps - some simple, useful wrappers for that function, for drawing patches from wads

// CPhipps - GNU C only suppresses generating a copy of a function if it is
// static inline; other compilers have different behaviour.
// This inline is _only_ for the function below

#ifndef GL_DOOM
#ifdef __GNUC__
inline
#endif
void V_DrawNumPatch(int x, int y, int scrn, int lump,
         int cm, enum patch_translation_e flags)
{
  V_DrawMemPatch(x, y, scrn, (const patch_t*)W_CacheLumpNum(lump),
     cm, flags);
  W_UnlockLumpNum(lump);
}
#endif // GL_DOOM

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
int V_NamePatchWidth(const char* name)
{
  int lump = W_GetNumForName(name);
  int w;

  w = SHORT(((const patch_t*)W_CacheLumpNum(lump))->width);
  W_UnlockLumpNum(lump);
  return w;
}

int V_NamePatchHeight(const char* name)
{
  int lump = W_GetNumForName(name);
  int w;

  w = SHORT(((const patch_t*)W_CacheLumpNum(lump))->height);
  W_UnlockLumpNum(lump);
  return w;
}

//
// V_SetPalette
//
// CPhipps - New function to set the palette to palette number pal.
// Handles loading of PLAYPAL and calls I_SetPalette

void V_SetPalette(int pal)
{
#ifndef GL_DOOM
  I_SetPalette(pal);
#else
  // proff 11/99: update the palette
  gld_SetPalette(pal);
#endif
}

//
// V_FillRect
//
// CPhipps - New function to fill a rectangle with a given colour
#ifndef GL_DOOM
void V_FillRect(int scrn, int x, int y, int width, int height, byte colour)
{
  byte* dest = screens[scrn] + x + y*SCREENWIDTH;
  while (height--) {
    memset(dest, colour, width);
    dest += SCREENWIDTH;
  }
}
#endif
