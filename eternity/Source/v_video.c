// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Gamma correction LUT stuff.
//  Color range translation support
//  Functions to draw patches (by post) directly to screen.
//  Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: v_video.c,v 1.10 1998/05/06 11:12:48 jim Exp $";

#include "c_io.h"
#include "doomdef.h"
#include "doomstat.h"
#include "r_main.h"
#include "m_bbox.h"
#include "r_draw.h"
#include "w_wad.h"   /* needed for color translation lump lookup */
#include "v_video.h"
#include "v_patch.h" // haleyjd
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
  {"CRBRICK",  &cr_brick,   &colrngs[CR_BRICK ]},
  {"CRTAN",    &cr_tan,     &colrngs[CR_TAN   ]},
  {"CRGRAY",   &cr_gray,    &colrngs[CR_GRAY  ]},
  {"CRGREEN",  &cr_green,   &colrngs[CR_GREEN ]},
  {"CRBROWN",  &cr_brown,   &colrngs[CR_BROWN ]},
  {"CRGOLD",   &cr_gold,    &colrngs[CR_GOLD  ]},
  {"CRRED",    &cr_red,     &colrngs[CR_RED   ]},
  {"CRBLUE",   &cr_blue,    &colrngs[CR_BLUE  ]},
  {"CRORANGE", &cr_orange,  &colrngs[CR_ORANGE]},
  {"CRYELLOW", &cr_yellow,  &colrngs[CR_YELLOW]},
  {"CRBLUE2",  &cr_blue_status, &cr_blue_status},
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
// V_MarkRect
//
// Marks a rectangular portion of the screen specified by
// upper left origin and height and width dirty to minimize
// the amount of screen update necessary. No return value.
//
// killough 11/98: commented out, macroized to no-op, since it's unused now

#if 0
void V_MarkRect(int x, int y, int width, int height)
{
  M_AddToBox(dirtybox, x, y);
  M_AddToBox(dirtybox, x+width-1, y+height-1);
}
#endif

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

void V_CopyRect(int srcx, int srcy, int srcscrn, int width,
		int height, int destx, int desty, int destscrn )
{
  byte *src;
  byte *dest;

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width > SCREENWIDTH
      || srcy<0
      || srcy+height > SCREENHEIGHT
      ||destx<0||destx+width > SCREENWIDTH
      || desty<0
      || desty+height > SCREENHEIGHT
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    I_Error ("Bad V_CopyRect");
#endif

  V_MarkRect (destx, desty, width, height);

   // SoM 1-30-04: ANYRES
   if(globalyscale > FRACUNIT)
   {
      int realx, realy, realw, realh;

      realx = realxarray[srcx];
      realy = realyarray[srcy];
      src = screens[srcscrn] + v_width * realy + realx;

      realx = realxarray[destx];
      realy = realyarray[desty];
      dest = screens[destscrn] + v_width * realy + realx;

      // I HOPE this will not extend the array bounds HEHE
      realw = realxarray[width + destx] - realx;
      realh = realyarray[height + desty] - realy;

      while(realh--)
      {
         memcpy(dest, src, realw);
         src += v_width;
         dest += v_width;
      }
   }
   else
   {
      src = screens[srcscrn]+SCREENWIDTH*srcy+srcx;
      dest = screens[destscrn]+SCREENWIDTH*desty+destx;

      for ( ; height>0 ; height--)
      {
         memcpy (dest, src, width);
         src += SCREENWIDTH;
         dest += SCREENWIDTH;
      }
   }
}

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
// haleyjd  04/04: rewritten to use new ANYRES patch system
//
void V_DrawPatchGeneral(int x, int y, VBuffer *buffer, patch_t *patch,
			boolean flipped)
{
   PatchInfo pi;

   pi.x = x;
   pi.y = y;
   pi.patch = patch;
   pi.flipped = flipped;
   pi.drawstyle = PSTYLE_NORMAL;

   buffer->PatchWrapper(&pi, buffer);
}

//
// V_DrawPatchUnscaled
//
// sf: drawpatch but not scaled like drawpatch is
// haleyjd 04/11/03: removed useless code for lowres drawing
//
// SoM: So I'm guessing that this is really scaled to 640x400 and not truly unscaled?
// or at least, is that how I should handle it?
//
// FIXME/TODO: supporting this in general resolutions requires
// more work.
//
void V_DrawPatchUnscaled(int x, int y, int scrn, patch_t *patch)
{
   byte *desttop;
   int  w = SHORT(patch->width), col = 0, colstop = w, colstep = 1;
   
#ifdef RANGECHECK
   if(v_width == SCREENWIDTH || v_height == SCREENHEIGHT)
      I_Error("V_DrawPatchUnscaled called in 320x200 video mode\n");
#endif
   
   y -= SHORT(patch->topoffset);
   x -= SHORT(patch->leftoffset);
   
   // haleyjd: fix for hires
   // haleyjd 05/18/02: no message, just return
   // SoM: This makes more sense I guess
   if((unsigned)x + w > (unsigned)v_width || 
      (unsigned)y + SHORT(patch->height) > (unsigned)v_height || 
      (unsigned)scrn>4)
   {
      return;      // killough 1/19/98: commented out printfs
   }

#if 0
   if (!scrn)
      V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));
#endif
   
   desttop = screens[scrn]+y*(SCREENWIDTH*2)+x;
   
   for(; col != colstop; col += colstep, desttop++)
   {
      const column_t *column = 
         (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      // step through the posts in a column
      while(column->topdelta != 0xff)
      {
         // killough 2/21/98: Unrolled and performance-tuned
         
         register const byte *source = (byte *) column + 3;
         register byte *dest = desttop + column->topdelta*SCREENWIDTH*2;
         register int count = column->length;

         if((count-=4)>=0)
         {
            do
            {
               register byte s0,s1;
               s0 = source[0];
               s1 = source[1];
               dest[0] = s0;
               dest[SCREENWIDTH*2] = s1;
               dest += SCREENWIDTH*4;
               s0 = source[2];
               s1 = source[3];
               source += 4;
               dest[0] = s0;
               dest[SCREENWIDTH*2] = s1;
               dest += SCREENWIDTH*4;
            }
            while((count-=4)>=0);
         }
         if(count+=4)
         {
            do
            {
               *dest = *source++;
               dest += SCREENWIDTH*2;
            }
            while(--count);
         }
         column = (column_t *)(source+1); //killough 2/21/98 even faster
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
// haleyjd 04/03/04: rewritten for ANYRES patch system
//
void V_DrawPatchTranslated(int x, int y, VBuffer *buffer, patch_t *patch,
                           char *outr, boolean flipped)
{
   PatchInfo pi;
   
   pi.x = x;
   pi.y = y;
   pi.patch = patch;
   pi.flipped = flipped;

   // is the patch really translated?
   if(outr)
   {
      pi.drawstyle = PSTYLE_TLATED;   
      V_SetPatchColrng(outr);
   }
   else
      pi.drawstyle = PSTYLE_NORMAL;

   buffer->PatchWrapper(&pi, buffer);
}

//
// V_DrawPatchTL
//
// Masks a column based masked pic to the screen with translucency.
// Also translates colors from one palette range to another using
// the color translation lumps loaded in V_InitColorTranslation.
//
// haleyjd 04/03/04: rewritten for ANYRES patch system
// 
void V_DrawPatchTL(int x, int y, VBuffer *buffer, patch_t *patch,
                   unsigned char *outr, int tl)
{
   PatchInfo pi;

   // if translucency is off, fall back to translated
   if(!general_translucency)
   {
      V_DrawPatchTranslated(x, y, buffer, patch, outr, false);
      return;
   }

   pi.x = x;
   pi.y = y;
   pi.patch = patch;
   pi.flipped = false; // TODO: these could be flipped too now

   // is the patch translated as well as translucent?
   if(outr)
   {
      pi.drawstyle = PSTYLE_TLTRANSLUC;
      V_SetPatchColrng(outr);
   }
   else
      pi.drawstyle = PSTYLE_TRANSLUC;

   // figure out the RGB tables to use for the tran level
   {
      fixed_t fglevel, bglevel;
      fglevel = tl & ~0x3ff;
      bglevel = FRACUNIT - fglevel;
      V_SetPatchTL(Col2RGB[fglevel >> 10], Col2RGB[bglevel >> 10]);
   }

   buffer->PatchWrapper(&pi, buffer);
}


#if 0
                // code to produce 100% accurate results in hires
      for ( ; col<w ; col++, desttop+=2)
	{
	  const column_t *column =
	    (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

	  // step through the posts in a column
	  while (column->topdelta != 0xff)
	    {
	      // killough 2/21/98: Unrolled and performance-tuned

              register const byte *source = (byte *) column + 3;
              register byte *dest = desttop + column->topdelta*SCREENWIDTH*4;
	      register int count = column->length;

	      if ((count-=4)>=0)
		do
		  {
		    register byte s0,s1;
		    s0 = source[0];
		    s1 = source[1];
		    s0 = outr[s0];
		    s1 = outr[s1];
                    *dest = main_tranmap[(*dest<<8) + s0];
                    dest[SCREENWIDTH*4] = main_tranmap[(dest[SCREENWIDTH*4]<<8) + s1];
                    dest[SCREENWIDTH*2] = main_tranmap[(dest[SCREENWIDTH*2]<<8) + s0];
                    dest[SCREENWIDTH*6] = main_tranmap[(dest[SCREENWIDTH*6]<<8) + s1];
                    dest[1] = main_tranmap[(dest[1]<<8) + s0];
                    dest[SCREENWIDTH*4+1] = main_tranmap[(dest[SCREENWIDTH*4+1]<<8) + s1];
                    dest[SCREENWIDTH*2+1] = main_tranmap[(dest[SCREENWIDTH*2+1]<<8) + s0];
                    dest[SCREENWIDTH*6+1] = main_tranmap[(dest[SCREENWIDTH*6+1]<<8) + s1];
		    dest += SCREENWIDTH*8;
		    s0 = source[2];
		    s1 = source[3];
		    s0 = outr[s0];
		    s1 = outr[s1];
		    source += 4;
                    *dest = main_tranmap[(*dest<<8) + s0];
                    dest[SCREENWIDTH*4] = main_tranmap[(dest[SCREENWIDTH*4]<<8) + s1];
                    dest[SCREENWIDTH*2] = main_tranmap[(dest[SCREENWIDTH*2]<<8) + s0];
                    dest[SCREENWIDTH*6] = main_tranmap[(dest[SCREENWIDTH*6]<<8) + s1];
                    dest[1] = main_tranmap[(dest[1]<<8) + s0];
                    dest[SCREENWIDTH*4+1] = main_tranmap[(dest[SCREENWIDTH*4+1]<<8) + s1];
                    dest[SCREENWIDTH*2+1] = main_tranmap[(dest[SCREENWIDTH*2+1]<<8) + s0];
                    dest[SCREENWIDTH*6+1] = main_tranmap[(dest[SCREENWIDTH*6+1]<<8) + s1];
		    dest += SCREENWIDTH*8;
		  }
		while ((count-=4)>=0);
	      if (count+=4)
		do
		  {
                    register byte s;
                                // sf : some changes here for tranlucency
                    s = outr[*source];
                    *dest = main_tranmap[(*dest<<8) + s];
                    dest[SCREENWIDTH*2] = main_tranmap[(dest[SCREENWIDTH*2]<<8) + s];
                    dest[1] = main_tranmap[(dest[1]<<8) + s];
                    dest[SCREENWIDTH*2+1] = main_tranmap[(dest[SCREENWIDTH*2+1]<<8) + s];
                    source++; dest += SCREENWIDTH*4;
		  }
		while (--count);
	      column = (column_t *)(source+1);

	    }
	}

#endif

// haleyjd 04/03/04: removed V_DrawPatchFlexTL, see above function.

//
// V_DrawBlock
//
// Draw a linear block of pixels into the view buffer. 
//
// The bytes at src are copied in linear order to the screen rectangle
// at x,y in screenbuffer scrn, with size width by height.
//
// No return value.
//
// haleyjd 04/08/03: rewritten for ANYRES system -- see v_block.c
// 
void V_DrawBlock(int x, int y, VBuffer *buffer, int width, int height, byte *src)
{
   buffer->BlockDrawer(x, y, buffer, width, height, src);
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
      ||x+width > SCREENWIDTH
      || y<0
      || y+height > SCREENHEIGHT
      || (unsigned)scrn>4 )
    I_Error ("Bad V_GetBlock");
#endif

   // SoM 1-30-04: ANYRES
   if(globalyscale > FRACUNIT)
   {
      x = realxarray[x];
      height = (height * globalyscale) >> FRACBITS;

      y = realyarray[y];
      width = (width * globalxscale) >> FRACBITS;
   }

   src = screens[scrn] + y * v_width + x;
   while (height--)
   {
      memcpy (dest, src, width);
      src += v_width;
      dest += width;
   }
}

// 
// V_FindBestColor
//
// Adapted from zdoom -- thanks to Randy Heit.
//
// This always assumes a 256-color palette;
// its intended for use in startup functions to match hard-coded
// color values to the best fit in the game's palette (allows
// cross-game usage among other things).
//
byte V_FindBestColor(const byte *palette, int r, int g, int b)
{
   int i, dr, dg, db;
   int bestcolor, bestdistortion, distortion;

   // use color 0 as a worst-case match for any color
   bestdistortion = 256*256*4;
   bestcolor = 0;

   for(i = 0; i < 256; i++)
   {
      dr = r - *palette++;
      dg = g - *palette++;
      db = b - *palette++;
      
      distortion = dr*dr + dg*dg + db*db;

      if(distortion < bestdistortion)
      {
	 // exact match
	 if(!distortion)
	    return i;

	 bestdistortion = distortion;
	 bestcolor = i;
      }
   }

   return bestcolor;
}

// haleyjd: DOSDoom-style single-lut translucency table
// generation code. This code has a 24k footprint but allows
// a much wider range of translucency effects than BOOM-style
// translucency. This will be used for particles, for variable
// mapthing trans levels, and for screen patches.

#define RPART(c)         (((c)>>16)&0xff)
#define GPART(c)         (((c)>>8)&0xff)
#define BPART(c)         ((c)&0xff)
#define MAKERGB(r, g, b) (((r)<<16)|((g)<<8)|(b))

boolean flexTranInit = false;
unsigned int Col2RGB[65][256];
byte RGB8k[16][32][16];

void V_InitFlexTranTable(const byte *palette)
{
   int i, r, g, b, x, y;
   unsigned int *tempRGBpal;
   const byte   *palRover;

   // mark that we've initialized the flex tran table
   flexTranInit = true;
   
   tempRGBpal = Z_Malloc(256*sizeof(*tempRGBpal), PU_STATIC, NULL);
   
   for(i = 0, palRover = palette; i < 256; i++, palRover += 3)
   {
      tempRGBpal[i] = MAKERGB(palRover[0], palRover[1], palRover[2]);
   }
   
   // build RGB table
   for(r = 0; r < 16; r++)
   {
      for(g = 0; g < 32; g++)
      {
         for(b = 0; b < 16; b++)
            RGB8k[r][g][b] = 
               V_FindBestColor(palette, r*16, g*8, b *16);
      }
   }
   
   // build lookup table
   for(x = 0; x < 65; x++)
   {
      for(y = 0; y < 256; y++)
      {
         Col2RGB[x][y] = (((RPART(tempRGBpal[y])*x)>>5)<<9)  |
                         (((GPART(tempRGBpal[y])*x)>>4)<<18) |
                          ((BPART(tempRGBpal[y])*x)>>5);
      }
   }
   
   Z_Free(tempRGBpal);
}

//
// V_CacheBlock
//
// haleyjd 12/22/02: 
// Copies a linear block to a memory buffer as if to a 
// low-res screen
//
void V_CacheBlock(int x, int y, int width, int height, byte *src,
                  byte *bdest)
{
   byte *dest = bdest + y*SCREENWIDTH + x;
   
   while(height--)
   {
      memcpy(dest, src, width);
      src += width;
      dest += SCREENWIDTH;
   }
}


//----------------------------------------------------------------------------
//
// $Log: v_video.c,v $
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

