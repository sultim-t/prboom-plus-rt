// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: V_video.c,v 1.1 2000/04/09 18:18:49 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//  Gamma correction LUT stuff.
//  Color range translation support
//  Functions to draw patches (by post) directly to screen.
//  Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: V_video.c,v 1.1 2000/04/09 18:18:49 proff_fs Exp $";

#include "doomdef.h"
#include "r_main.h"
#include "m_bbox.h"
#include "w_wad.h"   /* needed for color translation lump lookup */
#include "v_video.h"
#include "i_video.h"

// Each screen is [SCREENWIDTH*SCREENHEIGHT];
byte *screens[5];
int  dirtybox[4];

//jff 2/18/98 palette color ranges for translation
//jff 4/24/98 now pointers set to predefined lumps to allow overloading

char *cr_brick;
char *cr_tan;
char *cr_gray;
char *cr_green;
char *cr_brown;
char *cr_gold;
char *cr_red;
char *cr_blue;
char *cr_blue_status;
char *cr_orange;
char *cr_yellow;

//jff 4/24/98 initialize this at runtime
char *colrngs[10];

// Now where did these came from?
byte gammatable[5][256] =
{
  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
   17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
   49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
   65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
   81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
   97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
   113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
   128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
   144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
   160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
   176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
   192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
   208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
   224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
   240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},

  {2,4,5,7,8,10,11,12,14,15,16,18,19,20,21,23,24,25,26,27,29,30,31,
   32,33,34,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,52,54,55,
   56,57,58,59,60,61,62,63,64,65,66,67,69,70,71,72,73,74,75,76,77,
   78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,
   99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,
   115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,129,
   130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,
   146,147,148,148,149,150,151,152,153,154,155,156,157,158,159,160,
   161,162,163,163,164,165,166,167,168,169,170,171,172,173,174,175,
   175,176,177,178,179,180,181,182,183,184,185,186,186,187,188,189,
   190,191,192,193,194,195,196,196,197,198,199,200,201,202,203,204,
   205,205,206,207,208,209,210,211,212,213,214,214,215,216,217,218,
   219,220,221,222,222,223,224,225,226,227,228,229,230,230,231,232,
   233,234,235,236,237,237,238,239,240,241,242,243,244,245,245,246,
   247,248,249,250,251,252,252,253,254,255},

  {4,7,9,11,13,15,17,19,21,22,24,26,27,29,30,32,33,35,36,38,39,40,42,
   43,45,46,47,48,50,51,52,54,55,56,57,59,60,61,62,63,65,66,67,68,69,
   70,72,73,74,75,76,77,78,79,80,82,83,84,85,86,87,88,89,90,91,92,93,
   94,95,96,97,98,100,101,102,103,104,105,106,107,108,109,110,111,112,
   113,114,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
   129,130,131,132,133,133,134,135,136,137,138,139,140,141,142,143,144,
   144,145,146,147,148,149,150,151,152,153,153,154,155,156,157,158,159,
   160,160,161,162,163,164,165,166,166,167,168,169,170,171,172,172,173,
   174,175,176,177,178,178,179,180,181,182,183,183,184,185,186,187,188,
   188,189,190,191,192,193,193,194,195,196,197,197,198,199,200,201,201,
   202,203,204,205,206,206,207,208,209,210,210,211,212,213,213,214,215,
   216,217,217,218,219,220,221,221,222,223,224,224,225,226,227,228,228,
   229,230,231,231,232,233,234,235,235,236,237,238,238,239,240,241,241,
   242,243,244,244,245,246,247,247,248,249,250,251,251,252,253,254,254,
   255},

  {8,12,16,19,22,24,27,29,31,34,36,38,40,41,43,45,47,49,50,52,53,55,
   57,58,60,61,63,64,65,67,68,70,71,72,74,75,76,77,79,80,81,82,84,85,
   86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,103,104,105,106,107,
   108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,
   125,126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,
   141,142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,
   155,156,157,158,159,160,160,161,162,163,164,165,165,166,167,168,169,
   169,170,171,172,173,173,174,175,176,176,177,178,179,180,180,181,182,
   183,183,184,185,186,186,187,188,189,189,190,191,192,192,193,194,195,
   195,196,197,197,198,199,200,200,201,202,202,203,204,205,205,206,207,
   207,208,209,210,210,211,212,212,213,214,214,215,216,216,217,218,219,
   219,220,221,221,222,223,223,224,225,225,226,227,227,228,229,229,230,
   231,231,232,233,233,234,235,235,236,237,237,238,238,239,240,240,241,
   242,242,243,244,244,245,246,246,247,247,248,249,249,250,251,251,252,
   253,253,254,254,255},

  {16,23,28,32,36,39,42,45,48,50,53,55,57,60,62,64,66,68,69,71,73,75,76,
   78,80,81,83,84,86,87,89,90,92,93,94,96,97,98,100,101,102,103,105,106,
   107,108,109,110,112,113,114,115,116,117,118,119,120,121,122,123,124,
   125,126,128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,
   142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,155,
   156,157,158,159,159,160,161,162,163,163,164,165,166,166,167,168,169,
   169,170,171,172,172,173,174,175,175,176,177,177,178,179,180,180,181,
   182,182,183,184,184,185,186,187,187,188,189,189,190,191,191,192,193,
   193,194,195,195,196,196,197,198,198,199,200,200,201,202,202,203,203,
   204,205,205,206,207,207,208,208,209,210,210,211,211,212,213,213,214,
   214,215,216,216,217,217,218,219,219,220,220,221,221,222,223,223,224,
   224,225,225,226,227,227,228,228,229,229,230,230,231,232,232,233,233,
   234,234,235,235,236,236,237,237,238,239,239,240,240,241,241,242,242,
   243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,
   251,252,252,253,254,254,255,255}
};

int usegamma;

//
// V_InitColorTranslation
//
// Loads the color translation tables from predefined lumps at game start
// No return value
//
// Used for translating text colors from the red palette range
// to other colors. The first nine entries can be used to dynamically
// switch the output of text color thru the HUlib_drawText routine
// by embedding ESCn in the text to obtain color n. Symbols for n are
// provided in v_video.h.
//

typedef struct {
  const char *name;
  char **map1, **map2;
} crdef_t;

// killough 5/2/98: table-driven approach
static const crdef_t crdefs[] = {
  {"CRBRICK",  &cr_brick,       &colrngs[CR_BRICK ]},
  {"CRTAN",    &cr_tan,         &colrngs[CR_TAN   ]},
  {"CRGRAY",   &cr_gray,        &colrngs[CR_GRAY  ]},
  {"CRGREEN",  &cr_green,       &colrngs[CR_GREEN ]},
  {"CRBROWN",  &cr_brown,       &colrngs[CR_BROWN ]},
  {"CRGOLD",   &cr_gold,        &colrngs[CR_GOLD  ]},
  {"CRRED",    &cr_red,         &colrngs[CR_RED   ]},
  {"CRBLUE",   &cr_blue,        &colrngs[CR_BLUE  ]},
  {"CRORANGE", &cr_orange,      &colrngs[CR_ORANGE]},
  {"CRYELLOW", &cr_yellow,      &colrngs[CR_YELLOW]},
  {"CRBLUE2",  &cr_blue_status, &colrngs[CR_BLUE2]},
  {NULL}
};

// killough 5/2/98: tiny engine driven by table above
void V_InitColorTranslation(void)
{
  register const crdef_t *p;
  for (p=crdefs; p->name; p++)
    *p->map1 = *p->map2 = W_CacheLumpName(p->name, PU_STATIC);
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
// No return value.

#ifndef GL_DOOM
// proff 11/99: not needed in OpenGL
void V_CopyRect(int srcx, int srcy, int srcscrn, int width,
                int height, int destx, int desty, int destscrn )
{
  byte *src;
  byte *dest;

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width >SCREENWIDTH
      || srcy<0
      || srcy+height>SCREENHEIGHT
      ||destx<0||destx+width >SCREENWIDTH
      || desty<0
      || desty+height>SCREENHEIGHT
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    I_Error ("Bad V_CopyRect");
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

void V_CopyRectStretched(int srcx, int srcy, int srcscrn, int width,
                int height, int destx, int desty, int destscrn )
{
  byte *src;
  byte *dest;

  srcx=srcx*SCREENWIDTH/320;
  srcy=srcy*SCREENHEIGHT/200;
  width=width*SCREENWIDTH/320;
  height=height*SCREENHEIGHT/200;
  destx=destx*SCREENWIDTH/320;
  desty=desty*SCREENHEIGHT/200;

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width >SCREENWIDTH
      || srcy<0
      || srcy+height>SCREENHEIGHT
      ||destx<0||destx+width >SCREENWIDTH
      || desty<0
      || desty+height>SCREENHEIGHT
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    I_Error ("Bad V_CopyRect");
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
#endif // GL_DOOM

//
// V_DrawPatch
//
// Masks a column based masked pic to the screen.
//
// The patch is drawn at x,y in the buffer selected by scrn
// No return value
//
// V_DrawPatchFlipped
//
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
// Patch is drawn at x,y in screenbuffer scrn.
// No return value
//
// killough 11/98: Consolidated V_DrawPatch and V_DrawPatchFlipped into one
//

void V_DrawPatchGeneral(int x, int y, int scrn, patch_t *patch,
			H_boolean flipped)
{
  int  w = SHORT(patch->width), col = w-1, colstop = -1, colstep = -1;
  
  if (!flipped)
    col = 0, colstop = w, colstep = 1;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
  if (x<0
      ||x+SHORT(patch->width) >SCREENWIDTH
      || y<0
      || y+SHORT(patch->height)>SCREENHEIGHT
      || (unsigned)scrn>4)
      return;      // killough 1/19/98: commented out printfs
#endif

    {
      byte *desttop = screens[scrn]+y*SCREENWIDTH+x;

      for ( ; col != colstop ; col += colstep, desttop++)
	{
	  const column_t *column = 
	    (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

	  // step through the posts in a column
	  while (column->topdelta != 0xff )
	    {
	      // killough 2/21/98: Unrolled and performance-tuned

	      register const byte *source = (byte *)column + 3;
	      register byte *dest = desttop + column->topdelta*SCREENWIDTH;
	      register int count = column->length;

	      if ((count-=4)>=0)
		do
		  {
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
		  }
		while ((count-=4)>=0);
	      if (count+=4)
		do
		  {
		    *dest = *source++;
		    dest += SCREENWIDTH;
		  }
		while (--count);
	      column = (column_t *)(source+1); //killough 2/21/98 even faster
	    }
	}
    }
}

//
// V_DrawPatchTranslated
//
// Masks a column based masked pic to the screen.
// Also translates colors from one palette range to another using
// the color translation lumps loaded in V_InitColorTranslation
//
// The patch is drawn at x,y in the screen buffer scrn. Color translation
// is performed thru the table pointed to by outr. cm is not used.
//
// jff 1/15/98 new routine to translate patch colors
//
static void V_DrawPatchTranslated(int x, int y, int scrn, patch_t *patch, int cm)
{
  int col, w;
  unsigned char *outr=colrngs[cm];

  //jff 2/18/98 if translation not needed, just use the old routine
  if (cm==CR_RED)
    {
      V_DrawPatchGeneral(x,y,scrn,patch,false);
      return;                            // killough 2/21/98: add return
    }

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
  if (x<0
      ||x+SHORT(patch->width) >SCREENWIDTH
      || y<0
      || y+SHORT(patch->height)>SCREENHEIGHT
      || (unsigned)scrn>4)
    return;    // killough 1/19/98: commented out printfs
#endif

  col = 0;
  w = SHORT(patch->width);

    {
      byte *desttop = screens[scrn]+y*SCREENWIDTH+x;

      for ( ; col<w ; col++, desttop++)
	{
	  const column_t *column =
	    (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

	  // step through the posts in a column
	  while (column->topdelta != 0xff )
	    {
	      // killough 2/21/98: Unrolled and performance-tuned

	      register const byte *source = (byte *)column + 3;
	      register byte *dest = desttop + column->topdelta*SCREENWIDTH;
	      register int count = column->length;

	      if ((count-=4)>=0)
		do
		  {
		    register byte s0,s1;
		    s0 = source[0];
		    s1 = source[1];

		    //jff 2/18/98 apply red->range color translation
		    //2/18/98 don't brightness map for speed

		    s0 = outr[s0];
		    s1 = outr[s1];
		    dest[0] = s0;
		    dest[SCREENWIDTH] = s1;
		    dest += SCREENWIDTH*2;
		    s0 = source[2];
		    s1 = source[3];
		    s0 = outr[s0];
		    s1 = outr[s1];
		    source += 4;
		    dest[0] = s0;
		    dest[SCREENWIDTH] = s1;
		    dest += SCREENWIDTH*2;
		  }
		while ((count-=4)>=0);
	      if (count+=4)
		do
		  {
		    *dest = outr[*source++];
		    dest += SCREENWIDTH;
		  }
		while (--count);
	      column = (column_t *)(source+1);
	    }
	}

    }
}

//
// V_DrawBlock
//
// Draw a linear block of pixels into the view buffer. 
//
// The bytes at src are copied in linear order to the screen rectangle
// at x,y in screenbuffer scrn, with size width by height.
//
// The destination rectangle is marked dirty.
//
// No return value.
// 

void V_DrawBlock(int x, int y, int scrn, int width, int height, byte *src)
{
#ifdef RANGECHECK
  if (x<0
      ||x+width >SCREENWIDTH
      || y<0
      || y+height>SCREENHEIGHT
      || (unsigned)scrn>4 )
    I_Error ("Bad V_DrawBlock");
#endif

    {
      byte *dest = screens[scrn] + y*SCREENWIDTH+x;

      while (height--)
	{
	  memcpy (dest, src, width);
	  src += width;
	  dest += SCREENWIDTH;
	}
    }
}

//
// V_GetBlock
//
// Gets a linear block of pixels from the view buffer.
//
// The pixels in the rectangle at x,y in screenbuffer scrn with size
// width by height are linearly packed into the buffer dest.
// No return value
//

void V_GetBlock(int x, int y, int scrn, int width, int height, byte *dest)
{
  byte *src;

#ifdef RANGECHECK
  if (x<0
      ||x+width >SCREENWIDTH
      || y<0
      || y+height>SCREENHEIGHT
      || (unsigned)scrn>4 )
    I_Error ("Bad V_DrawBlock");
#endif

  src = screens[scrn] + y*SCREENWIDTH+x;

  while (height--)
    {
      memcpy (dest, src, width);
      src += SCREENWIDTH;
      dest += width;
    }
}

//
// V_FillBlock
//
// Fills a linear block of pixels with col. 
//
// The destination rectangle is marked dirty.
//
// No return value.
// 

#ifndef GL_DOOM
// proff 11/99: not needed in OpenGL
void V_FillBlock(int x, int y, int scrn, int width, int height, int col)
{
#ifdef RANGECHECK
  if (x<0
      ||x+width >SCREENWIDTH
      || y<0
      || y+height>SCREENHEIGHT
      || (unsigned)scrn>4 )
    I_Error ("Bad V_DrawBlock");
#endif

  {
    byte *dest = screens[scrn] + y*SCREENWIDTH+x;

    while (height--)
	  {
	    memset (dest, col, width);
	    dest += SCREENWIDTH;
	  }
  }
}
#endif // GL_DOOM

//
// V_Init
//
// Allocates the 4 full screen buffers in low DOS memory
// No return
//

void V_Init (void)
{
  int  i;
  static byte *base=NULL;

  // stick these in low dos memory on PCs

#ifdef GL_DOOM
  return;
// proff 11/99: not needed in OpenGL
#endif

  if (base)
    free(base);

  base = calloc(SCREENWIDTH*SCREENHEIGHT*4,1);

  screens[0]=base;

  for (i=1 ; i<4 ; i++)
    screens[i] = base + i*SCREENWIDTH*SCREENHEIGHT;
}

// proff/nicolas 09/20/98: Added for stretching patches in high-res

void V_DrawPatchStretchedGeneral( int x, int y, int scrn, patch_t* patch,
                                  H_boolean flipped)
{
  column_t*	column;
  byte*	desttop;
  register byte* source;
  register byte* dest;

  int col, w, count;
  int stretchx, stretchy;
  register int srccol;

  int DX;
  int DXI;
  int DY;
  register int DYI;
  int DY2;
  int DYI2;
 
  if ((SCREENWIDTH==320) && (SCREENHEIGHT==200))
  {
    V_DrawPatchGeneral(x,y,scrn,patch,false);
    return;
  }

  DX  = (SCREENWIDTH<<16)  / 320;
  DXI = (320<<16)          / SCREENWIDTH;
  DY  = (SCREENHEIGHT<<16) / 200;
  DYI = (200<<16)          / SCREENHEIGHT;
  DY2 = DY                 / 2;
  DYI2= DYI                * 2;

  x	-=	SHORT( patch->leftoffset );
  y	-=	SHORT( patch->topoffset  );

  
#ifdef RANGECHECK

  if ( x < 0 || x + SHORT(patch->width) > SCREENWIDTH
             || y < 0
             || y + SHORT(patch->height) > SCREENHEIGHT
             || ( unsigned ) scrn > 4 
	 ) return;

#endif

  stretchx = ( x * DX ) >> 16;
  stretchy = ( y * DY ) >> 16;

  desttop	= screens[scrn] + stretchy * SCREENWIDTH +  stretchx;
	 
  w = ( patch->width ) << 16;

  if (flipped)
  for ( col = 0; col < w; x++, col+=DXI, desttop++ )
  {
    column = ( column_t* ) (( byte* ) patch + LONG( patch->columnofs[(w-1-col)>>16] ));
 
    while ( column->topdelta != 0xff )
    {
	    source = ( byte* ) column + 3;
      dest   = desttop + (( column->topdelta * DY ) >> 16 ) * SCREENWIDTH;
      count  = ( column->length * DY ) >> 16;
   	  srccol = 0;
	 
	    while (count--)
      {
	      *dest  =  source[srccol>>16];
	      dest  +=  SCREENWIDTH;
        srccol+=  DYI;
	    }
	    column = ( column_t* ) (( byte* ) column + ( column->length ) + 4 );
    }
  }
  else
  for ( col = 0; col < w; x++, col+=DXI, desttop++ )
  {
    column = ( column_t* ) (( byte* ) patch + LONG( patch->columnofs[col>>16] ));
 
    while ( column->topdelta != 0xff )
    {
	    source = ( byte* ) column + 3;
      dest   = desttop + (( column->topdelta * DY ) >> 16 ) * SCREENWIDTH;
      count  = ( column->length * DY ) >> 16;
   	  srccol = 0;
	 
	    while (count--)
      {
	      *dest  =  source[srccol>>16];
	      dest  +=  SCREENWIDTH;
        srccol+=  DYI;
	    }
	    column = ( column_t* ) (( byte* ) column + ( column->length ) + 4 );
    }
  }
}

void V_DrawPatchTransStretched( int x, int y, int scrn, patch_t* patch, int cm )
{
  column_t*	column;
  byte*	desttop;
  register byte* source;
  register byte* dest;
  unsigned char *outr=colrngs[cm];
  
  int col, w, count;
  int stretchx, stretchy;
  register int srccol;

  int DX;
  int DXI;
  int DY;
  register int DYI;
  int DY2;
  int DYI2;
 
  if ((SCREENWIDTH==320) && (SCREENHEIGHT==200))
  {
    V_DrawPatchTranslated(x,y,scrn,patch,cm);
    return;
  }

  DX  = (SCREENWIDTH<<16)  / 320;
  DXI = (320<<16)          / SCREENWIDTH;
  DY  = (SCREENHEIGHT<<16) / 200;
  DYI = (200<<16)          / SCREENHEIGHT;
  DY2 = DY                 / 2;
  DYI2= DYI                * 2;

  x	-=	SHORT( patch->leftoffset );
  y	-=	SHORT( patch->topoffset  );

  
#ifdef RANGECHECK

  if ( x < 0 || x + SHORT(patch->width) > SCREENWIDTH
             || y < 0
             || y + SHORT(patch->height) > SCREENHEIGHT
             || ( unsigned ) scrn > 4 
	 ) return;

#endif

  stretchx = ( x * DX ) >> 16;
  stretchy = ( y * DY ) >> 16;

  desttop	= screens[scrn] + stretchy * SCREENWIDTH +  stretchx;
	 
  w = ( patch->width ) << 16;

  for ( col = 0; col < w; x++, col+=DXI, desttop++ )
  {
    column = ( column_t* ) (( byte* ) patch + LONG( patch->columnofs[col>>16] ));
 
    while ( column->topdelta != 0xff )
    {
	    source = ( byte* ) column + 3;
      dest   = desttop + (( column->topdelta * DY ) >> 16 ) * SCREENWIDTH;
      count  = ( column->length * DY ) >> 16;
   	  srccol = 0;
	 
	    while (count--)
      {
	      *dest  =  outr[source[srccol>>16]];
	      dest  +=  SCREENWIDTH;
        srccol+=  DYI;
	    }
	    column = ( column_t* ) (( byte* ) column + ( column->length ) + 4 );
    }
  }
}

// proff/nicolas 09/20/98: End of additions

//----------------------------------------------------------------------------
//
// $Log: V_video.c,v $
// Revision 1.1  2000/04/09 18:18:49  proff_fs
// Initial revision
//
// Revision 1.10  1998/05/06  11:12:48  jim
// Formattted v_video.*
//
// Revision 1.9  1998/05/03  22:53:16  killough
// beautification, simplify translation lookup
//
// Revision 1.8  1998/04/24  08:09:39  jim
// Make text translate tables lumps
//
// Revision 1.7  1998/03/02  11:41:58  killough
// Add cr_blue_status for blue statusbar numbers
//
// Revision 1.6  1998/02/24  01:40:12  jim
// Tuned HUD font
//
// Revision 1.5  1998/02/23  04:58:17  killough
// Fix performance problems
//
// Revision 1.4  1998/02/19  16:55:00  jim
// Optimized HUD and made more configurable
//
// Revision 1.3  1998/02/17  23:00:36  jim
// Added color translation machinery and data
//
// Revision 1.2  1998/01/26  19:25:08  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:05  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------

