// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley
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
//  Functions to draw patches (by post)
//
//-----------------------------------------------------------------------------

#include "v_video.h"

//
// Module globals needed by patch drawing functions.
// These have to be set before the patch wrapper function
// is invoked. See the V_DrawPatch* functions in v_video.c
// for an example.
//

// color range translation table
static byte *v_colrng;

// translucency lookups
static unsigned int *v_fg2rgb;
static unsigned int *v_bg2rgb;

void V_SetPatchColrng(byte *colrng)
{
   v_colrng = colrng;
}

void V_SetPatchTL(unsigned int *fg, unsigned int *bg)
{
   v_fg2rgb = fg;
   v_bg2rgb = bg;
}

//
// Column Drawers
//
// TR = Translated
// TL = Translucent
// 2x = Scaled 2x (hard-coded 4-pixel writes, fast)
// S  = Scaled, general (slower but works for all resolutions)
//

//
// Unscaled functions
//

//
// Normal
//
static void V_PatchColumn(byte *source, byte *dest, int count, int pitch)
{
   do
   {
      *dest = *source++;
      dest += pitch;
   } while(--count);
}

//
// Translated
//
static void V_PatchColumnTR(byte *source, byte *dest, int count, int pitch)
{
   do
   {
      *dest = v_colrng[*source++];
      dest += pitch;
   } while(--count);
}

//
// Translucent
//
static void V_PatchColumnTL(byte *source, byte *dest, int count, int pitch)
{
   register unsigned int color;

   do
   {
      color = (v_fg2rgb[*source++] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(color >> 5) & (color >> 19)];
      dest += pitch;
   } while(--count);
}

//
// Translated Translucent
//
static void V_PatchColumnTRTL(byte *source, byte *dest, int count, int pitch)
{
   register unsigned int color;

   do
   {
      color = (v_fg2rgb[v_colrng[*source++]] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(color >> 5) & (color >> 19)];
      dest += pitch;
   } while(--count);
}

//
// Array of unscaled column function pointers
//

typedef void (*unscaled_func_t)(byte *, byte *, int, int);

static unscaled_func_t unscaledfuncs[] =
{
   V_PatchColumn,
   V_PatchColumnTR,
   V_PatchColumnTL,
   V_PatchColumnTRTL
};

//
// 2x Scaled Functions
//

//
// Normal
//
static void V_PatchColumn2x(byte *source, byte *dest, int count, int pitch)
{
   do
   {
      dest[0] 
         = dest[pitch] 
         = dest[1] 
         = dest[pitch + 1] 
         = *source++;

      dest += pitch * 2;
   } while(--count);
}

//
// Translated
//
static void V_PatchColumnTR2x(byte *source, byte *dest, int count, int pitch)
{
   do
   {
      dest[0] 
         = dest[pitch] 
         = dest[1] 
         = dest[pitch + 1] 
         = v_colrng[*source++];
      
      dest += pitch * 2;
   } while(--count);
}

//
// Translucent
//
// In interest of maximum benefit from 2x scaling, this is not 100%
// accurate.  The background pixel is the upper left one in each
// group of four.  This looks good enough most of the time.
//
static void V_PatchColumnTL2x(byte *source, byte *dest, int count, int pitch)
{
   register unsigned int color;

   do
   {
      color = (v_fg2rgb[*source++] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      
      dest[0] 
         = dest[pitch]
         = dest[1]
         = dest[pitch+1]
         = RGB8k[0][0][(color >> 5) & (color >> 19)];
            
      dest += pitch * 2;
   } while(--count);
}

//
// Translated Translucent
//
// In interest of maximum benefit from 2x scaling, this is not 100%
// accurate.  The background pixel is the upper left one in each
// group of four.  This looks good enough most of the time.
//
static void V_PatchColumnTRTL2x(byte *source, byte *dest, int count, int pitch)
{
   register unsigned int color;

   do
   {
      color = (v_fg2rgb[v_colrng[*source++]] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      
      dest[0]
         = dest[pitch]
         = dest[1]
         = dest[pitch+1]
         = RGB8k[0][0][(color >> 5) & (color >> 19)];
      
      dest += pitch * 2;
   } while(--count);
}

//
// Array of 2x column function pointers
//
// (Same prototype as unscaled)
//

static unscaled_func_t scale2xfuncs[] =
{
   V_PatchColumn2x,
   V_PatchColumnTR2x,
   V_PatchColumnTL2x,
   V_PatchColumnTRTL2x
};

//
// General Scaled Functions
//

//
// Normal
//
static void V_PatchColumnS(byte *source, byte *dest, int count, int pitch, fixed_t yfrac, fixed_t yinc)
{
   while(count--)
   {
      *dest = source[yfrac >> FRACBITS];
      dest += pitch;
      yfrac += yinc;
   }
}

//
// Translated
//
static void V_PatchColumnTRS(byte *source, byte *dest, int count, int pitch, fixed_t yfrac, fixed_t yinc)
{
   while(count--)
   {
      *dest = v_colrng[source[yfrac >> FRACBITS]];
      dest += pitch;
      yfrac += yinc;
   }
}

//
// Translucent
//
static void V_PatchColumnTLS(byte *source, byte *dest, int count, int pitch, fixed_t yfrac, fixed_t yinc)
{
   register unsigned int color;

   while(count--)
   {
      color = (v_fg2rgb[source[yfrac >> FRACBITS]] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(color >> 5) & (color >> 19)];
      dest += pitch;
      yfrac += yinc;
   }
}

//
// Translated Translucent
//
static void V_PatchColumnTRTLS(byte *source, byte *dest, int count, int pitch, fixed_t yfrac, fixed_t yinc)
{
   register unsigned int color;

   while(count--)
   {
      color = (v_fg2rgb[v_colrng[source[yfrac >> FRACBITS]]] + v_bg2rgb[*dest]) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(color >> 5) & (color >> 19)];
      dest += pitch;
      yfrac += yinc;
   }
}

//
// Array of general scaled column function pointers
//

typedef void (*scaled_func_t)(byte *, byte *, int, int, fixed_t, fixed_t);

static scaled_func_t scaledfuncs[] =
{
   V_PatchColumnS,
   V_PatchColumnTRS,
   V_PatchColumnTLS,
   V_PatchColumnTRTLS,
};

//
// Wrapper Functions
//
// These functions draw a patch in the appropriate manner,
// using the column drawers above.
//

void V_PatchWrapper(PatchInfo *pi, VBuffer *buffer)
{
   patch_t *patch = pi->patch;
   int w = SHORT(patch->width);
   int col = 0, colstop = w, colstep = 1;
   int x = pi->x, y = pi->y;
   byte *desttop;
   unscaled_func_t colfunc;
   
   if(pi->flipped)
      col = w - 1, colstop = -1, colstep = -1;

   y -= SHORT(patch->topoffset);
   x -= SHORT(patch->leftoffset);
   
   if(x < 0 || x + w > buffer->width || 
      y < 0 || y + SHORT(patch->height) > buffer->height)
      return;

   colfunc = unscaledfuncs[pi->drawstyle];
   desttop = buffer->data + y * buffer->pitch + x;

   for(; col != colstop; col += colstep, desttop++)
   {
      const column_t *column = 
         (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      // step through the posts in a column
      while(column->topdelta != 0xff)
      {
         if(column->length)
         {
            colfunc((byte *)column + 3,
                    desttop + column->topdelta * buffer->pitch,
                    column->length,
                    buffer->pitch);
         }
         column = (column_t *)((byte *)column + column->length + 4);
      }
   }
}

void V_PatchWrapper2x(PatchInfo *pi, VBuffer *buffer)
{
   patch_t *patch = pi->patch;
   int w = SHORT(patch->width);
   int col = 0, colstop = w, colstep = 1;
   int x = pi->x, y = pi->y;
   byte *desttop;
   unscaled_func_t colfunc;
   
   if(pi->flipped)
      col = w - 1, colstop = -1, colstep = -1;

   y -= SHORT(patch->topoffset);
   x -= SHORT(patch->leftoffset);
   
   if(x < 0 || x + w > buffer->width/2 || 
      y < 0 || y + SHORT(patch->height) > buffer->height/2)
      return;

   colfunc = scale2xfuncs[pi->drawstyle];
   desttop = buffer->data + y * 2 * buffer->pitch + x * 2;

   for(; col != colstop; col += colstep, desttop += 2)
   {
      const column_t *column = 
         (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      // step through the posts in a column
      while(column->topdelta != 0xff)
      {
         if(column->length)
         {
            colfunc((byte *)column + 3,
                    desttop + column->topdelta * 2 * buffer->pitch,
                    column->length,
                    buffer->pitch);
         }
         column = (column_t *)((byte *)column + column->length + 4);
      }
   }
}

void V_PatchWrapperS(PatchInfo *pi, VBuffer *buffer)
{
   int x, y;
   patch_t *patch = pi->patch;
   scaled_func_t colfunc;
   
   byte *desttop;
   fixed_t xfrac;
   fixed_t xstep, ystep, ybottom, ybottomfrac;
   int realx, realy, realw, realh;
   int *xlookup = buffer->xlookup;
   int *ylookup = buffer->ylookup;

   int w = SHORT(patch->width), col = 0, colstop = w, colstep = 1;
   
   if(pi->flipped)
      col = w-1, colstop = -1, colstep = -1;

   x = pi->x;
   y = pi->y;

   y -= SHORT(patch->topoffset);
   x -= SHORT(patch->leftoffset);

   realx = xlookup[x];
   realw = xlookup[x + w] - realx;
   realy = ylookup[y];
   realh = ylookup[y + SHORT(patch->height)] - realy;

   if(realx < 0 || realx + realw > buffer->width || 
      realy < 0 || realy + realh > buffer->height)
      return;
   
   colfunc = scaledfuncs[pi->drawstyle];
   desttop = buffer->data + realy * buffer->pitch + realx;
   
   xstep = buffer->ixscale * colstep;
   ystep = buffer->iyscale;
   
   ybottom = patch->height + y;
   ybottomfrac = ybottom << FRACBITS;
   
   xfrac = col << FRACBITS;
   
   x = realw;

   for( ; x-- && col != colstop; desttop++)
   {
      const column_t *column = 
         (const column_t *)((byte *)patch + LONG(patch->columnofs[col]));
      
      // step through the posts in a column
      while(column->topdelta != 0xff)
      {
         // haleyjd 04/08/04: this code is apparently sensitive to
         // posts that go off the destination buffer (bad patches??),
         // so it needs to ensure that each post doesn't exceed the
         // normal unscaled vertical boundary of 200. Any post which
         // does must be ignored.

         if(y + column->topdelta + column->length <= 200)
         {         
            colfunc((byte *)column + 3, 
               desttop + ylookup[column->topdelta] * buffer->pitch, 
               ylookup[y + column->topdelta + column->length] - ylookup[y + column->topdelta],
               buffer->pitch, 0, ystep);
         }
         column = (column_t *)((byte *)column + column->length + 4);
      }
      
      xfrac += xstep;
      col = xfrac >> FRACBITS;
   }
}

void V_SetBlockFuncs(VBuffer *, int);

//
// V_SetupBufferFuncs
//
// VBuffer setup function
//
// Call to determine the type of drawing your VBuffer object will have
//
void V_SetupBufferFuncs(VBuffer *buffer, int drawtype)
{
   // call other setting functions
   V_SetBlockFuncs(buffer, drawtype);

   // set patch wrapper function here
   switch(drawtype)
   {
   case DRAWTYPE_UNSCALED:
      buffer->PatchWrapper = V_PatchWrapper;
      break;
   case DRAWTYPE_2XSCALED:
      buffer->PatchWrapper = V_PatchWrapper2x;
      break;
   case DRAWTYPE_GENSCALED:
      buffer->PatchWrapper = V_PatchWrapperS;
      break;
   default:
      break;
   }
}

// EOF

