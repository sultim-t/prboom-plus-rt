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
//  Functions to manipulate linear blocks of graphics data.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "v_video.h"

//
// V_DrawBlock Implementors
//
// * V_BlockDrawer   -- unscaled
// * V_BlockDrawer2x -- fast 2x scaling
// * V_BlockDrawerS  -- general scaling

static void V_BlockDrawer(int x, int y, VBuffer *buffer, int width, int height, byte *src)
{
   byte *dest;

#ifdef RANGECHECK
   if(x < 0 || x + width > buffer->width ||
      y < 0 || y + height > buffer->height)
   {
      I_Error("V_BlockDrawer: block exceeds buffer boundaries.\n");
   }
#endif

   dest = buffer->data + y * buffer->pitch + x;
   
   while(height--)
   {
      memcpy(dest, src, width);
      src += width;
      dest += buffer->pitch;
   }
}

static void V_BlockDrawer2x(int x, int y, VBuffer *buffer, int width, int height, byte *src)
{
   byte *dest;

#ifdef RANGECHECK
   if(x < 0 || x + width > buffer->width/2 ||
      y < 0 || y + height > buffer->height/2)
   {
      I_Error("V_BlockDrawer2x: block exceeds buffer boundaries.\n");
   }
#endif

   dest = buffer->data + y * buffer->pitch * 2 + x * 2;
   
   if(width)
   {
      while(height--)
      {
         byte *d = dest;
         int t = width;
         do
         {
            d[buffer->pitch] 
               = d[buffer->pitch + 1] 
               = d[0] 
               = d[1] 
               = *src++;
         } while(d += 2, --t);
         
         dest += buffer->pitch * 2;
      }
   }
}

static void V_BlockDrawerS(int x, int y, VBuffer *buffer, int width, int height, byte *src)
{
   byte *dest;
   fixed_t xstep, ystep, xfrac, yfrac;
   int xtex, ytex, w, h, i, realx, realy;
      
   realx = buffer->xlookup[x];
   realy = buffer->ylookup[y];
   w     = buffer->xlookup[x + width] - realx;
   h     = buffer->ylookup[y + height] - realy;
   xstep = buffer->ixscale;
   ystep = buffer->iyscale;
   yfrac = 0;
   dest  = buffer->data + realy * buffer->pitch + realx;

#ifdef RANGECHECK
   if(realx < 0 || realx + w > buffer->width ||
      realy < 0 || realy + h > buffer->height)
   {
      I_Error("V_BlockDrawerS: block exceeds buffer boundaries.\n");
   }
#endif

   while(h--)
   {
      i = w;
      xfrac = 0;
      ytex = (yfrac >> FRACBITS) * width;
      
      while(i--)
      {
         xtex = (xfrac >> FRACBITS);
         *dest++ = src[ytex + xtex];
         xfrac += xstep;
      }
      
      yfrac += ystep;
   }
}

//
// V_TileFlat implementors
//

//
// Unscaled
//
// Works for any video mode.
//
static void V_TileBlock64(VBuffer *buffer, byte *src)
{
   int x, y;
   byte *back_dest = buffer->data;
   int wmod64      = buffer->width & 63;
   boolean ndiv64  = !!wmod64;
   int pwdiff      = buffer->pitch - buffer->width;

   for(y = 0; y < buffer->height; ++y)
   {
      for(x = 0; x < buffer->width/64; ++x)
      {
         memcpy(back_dest, src + ((y & 63) << 6), 64);
         back_dest += 64;
      }
      if(ndiv64)
      {
         memcpy(back_dest, src + ((y & 63) << 6), wmod64);
         back_dest += wmod64;
      }
      back_dest += pwdiff;
   }
}

//
//
// Scaled 2x
//
// NOTE: This version ONLY works for 640x400 buffers.
//
static void V_TileBlock64_2x(VBuffer *buffer, byte *src)
{
   int x, y;
   byte *back_dest = buffer->data;
   byte *back_src  = src;

   for(y = 0; y < SCREENHEIGHT; src = ((++y & 63) << 6) + back_src,
       back_dest += buffer->pitch)
   {
      for(x = 0; x < SCREENWIDTH/64; ++x)
      {
         int i = 63;

         do
         {
            back_dest[i * 2]
               = back_dest[i * 2 + SCREENWIDTH * 2]
               = back_dest[i * 2 + 1]
               = back_dest[i * 2 + SCREENWIDTH * 2 + 1]
               = src[i];
         } while(--i >= 0);
         back_dest += 128;
      }
   }
}

//
// General scaling
//
static void V_TileBlock64S(VBuffer *buffer, byte *src)
{
   byte *dest;
   fixed_t xstep, ystep, xfrac, yfrac = 0;
   int xtex, ytex, w, h, pwdiff;
   
   w = buffer->width;
   h = buffer->height;
   xstep = buffer->ixscale;
   ystep = buffer->iyscale;
   
   dest = buffer->data;

   pwdiff = buffer->pitch - buffer->width;

   while(h--)
   {
      int i = w;
      xfrac = 0;
      ytex = ((yfrac >> FRACBITS) & 63) * 64;
      
      while(i--)
      {
         xtex = (xfrac >> FRACBITS) & 63;
         *dest++ = src[ytex + xtex];
         xfrac += xstep;
      }
      
      yfrac += ystep;
      dest += pwdiff; // haleyjd 06/28/04: forgot this!
   }
}

void V_SetBlockFuncs(VBuffer *buffer, int drawtype)
{
   switch(drawtype)
   {
   case DRAWTYPE_UNSCALED:
      buffer->BlockDrawer = V_BlockDrawer;
      buffer->TileBlock64 = V_TileBlock64;
      break;
   case DRAWTYPE_2XSCALED:
      buffer->BlockDrawer = V_BlockDrawer2x;
      // 2x version of V_TileBlock only works for 640x400
      if(buffer->width == 640 && buffer->height == 400)
         buffer->TileBlock64 = V_TileBlock64_2x;
      else
         buffer->TileBlock64 = V_TileBlock64S;
      break;
   case DRAWTYPE_GENSCALED:
      buffer->BlockDrawer = V_BlockDrawerS;
      buffer->TileBlock64 = V_TileBlock64S;
      break;
   default:
      break;
   }
}

// EOF

