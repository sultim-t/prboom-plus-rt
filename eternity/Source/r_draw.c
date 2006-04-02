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
//      The actual span/column drawing functions.
//      Here find the main potential for optimization,
//       e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_draw.c,v 1.16 1998/05/03 22:41:46 killough Exp $";

#include "doomstat.h"
#include "w_wad.h"
#include "r_draw.h"
#include "r_main.h"
#include "v_video.h"
#include "mn_engin.h"
#include "d_gi.h"

#define MAXWIDTH  MAX_SCREENWIDTH          /* kilough 2/8/98 */
#define MAXHEIGHT MAX_SCREENHEIGHT

#ifdef DJGPP
#define USEASM /* sf: changed #ifdef DJGPP to #ifdef USEASM */
#endif

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

byte *viewimage; 
int  viewwidth;
int  scaledviewwidth;
int  scaledviewheight;        // killough 11/98
int  viewheight;
int  viewwindowx;
int  viewwindowy; 
// SoM: ANYRES
int  scaledwindowx;
int  scaledwindowy;

byte *ylookup[MAXHEIGHT]; 
int  columnofs[MAXWIDTH]; 
int  linesize = SCREENWIDTH;  // killough 11/98

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
 
byte *tranmap;          // translucency filter maps 256x256   // phares 
byte *main_tranmap;     // killough 4/11/98

//
// R_DrawColumn
// Source is the top of the column to scale.
//

lighttable_t *dc_colormap; 
int     dc_x; 
int     dc_yl; 
int     dc_yh; 
fixed_t dc_iscale; 
fixed_t dc_texturemid;
int     dc_texheight;    // killough
byte    *dc_source;      // first pixel in a column (possibly virtual) 
fixed_t dc_translevel;   // haleyjd: level for zdoom translucency


// SoM: OPTIMIZE for ANYRES
typedef enum
{
   COL_NONE,
   COL_OPAQUE,
   COL_TRANS,
   COL_FLEXTRANS,
   COL_FUZZ
} columntype_e;

static int    temp_x = 0;
static int    tempyl[4], tempyh[4];
static byte   tempbuf[MAX_SCREENHEIGHT * 4];
static int    startx = 0;
static int    temptype = COL_NONE;
static int    commontop, commonbot;
static byte   *temptranmap = NULL;
static fixed_t temptranslevel;
// haleyjd 09/12/04: optimization -- precalculate flex tran lookups
static unsigned int *temp_fg2rgb;
static unsigned int *temp_bg2rgb;
// SoM 7-28-04: Fix the fuzz problem.
static byte   *tempfuzzmap;



// Fuzz stuffs
#define FUZZTABLE 50 
#define FUZZOFF (SCREENWIDTH)

static const int fuzzoffset[FUZZTABLE] = 
{
  1,0,1,0,1,1,0,
  1,1,0,1,1,1,0,
  1,1,1,0,0,0,0,
  1,0,0,1,1,1,1,0,
  1,0,1,1,0,0,1,
  1,0,0,0,0,1,1,
  1,1,0,1,1,0,1 
}; 

static int fuzzpos = 0; 

//
// Error functions that will abort if R_FlushColumns tries to flush 
// columns without a column type.
//

static void R_FlushError(int columnnumber, int yl, int yh)
{
   I_Error("R_FlushSingleColumn called without being initialized.\n");
}

static void R_QuadFlushError(void)
{
   I_Error("R_FlushQuadColumn called without being initialized.\n");
}

// Begin: Single column flushing functions.
static void R_FlushSingleOpaque(int columnnumber, int yl, int yh)
{
   register byte *source = tempbuf + columnnumber + (yl << 2);
   register byte *dest = ylookup[yl] + columnofs[startx + columnnumber];
   register int count;

   count = yh - yl + 1;

   while(--count >= 0)
   {
      *dest = *source;
      source += 4;
      dest += linesize;
   }
}

static void R_FlushSingleTL(int columnnumber, int yl, int yh)
{
   register byte *source = tempbuf + columnnumber + (yl << 2);
   register byte *dest = ylookup[yl] + columnofs[startx + columnnumber];
   register int count;

   count = yh - yl + 1;

   while(--count >= 0)
   {
      // haleyjd 09/11/04: use temptranmap here
      *dest = temptranmap[(*dest<<8) + *source];
      source += 4;
      dest += linesize;
   }
}

static void R_FlushSingleFuzz(int columnnumber, int yl, int yh)
{
   register byte *source = tempbuf + columnnumber + (yl << 2);
   register byte *dest = ylookup[yl] + columnofs[startx + columnnumber];
   register int count;

   count = yh - yl + 1;

   while(--count >= 0)
   {
      // SoM 7-28-04: Fix the fuzz problem.
      *dest = tempfuzzmap[6*256+dest[fuzzoffset[fuzzpos] ? v_width: -v_width]];

      // Clamp table lookup index.
      if (++fuzzpos == FUZZTABLE) 
        fuzzpos = 0;

      source += 4;
      dest += linesize;
   }
}

static void R_FlushSingleFlex(int columnnumber, int yl, int yh)
{
   register byte *source = tempbuf + columnnumber + (yl << 2);
   register byte *dest = ylookup[yl] + columnofs[startx + columnnumber];
   register int count;
   unsigned int fg, bg;

   count = yh - yl + 1;

   while(--count >= 0)
   {
      // haleyjd 09/12/04: use precalculated lookups
      fg = temp_fg2rgb[*source];
      bg = temp_bg2rgb[*dest];
      fg = (fg+bg) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(fg>>5) & (fg>>19)];

      source += 4;
      dest += linesize;
   }
}

static void (*R_FlushSingleColumn)(int columnnumber, int yl, int yh) = R_FlushError;

// Begin: Quad column flushing functions.
static void R_FlushQuadOpaque(void)
{
   register int *source = (int *)(tempbuf + (commontop << 2));
   register int *dest = (int *)(ylookup[commontop] + columnofs[startx]);
   register int count;
   register int deststep = linesize / 4;

   count = commonbot - commontop + 1;

   while(--count >= 0)
   {
      *dest = *source++;
      dest += deststep;
   }
}

static void R_FlushQuadTL(void)
{
   register byte *source = tempbuf + (commontop << 2);
   register byte *dest = ylookup[commontop] + columnofs[startx];
   register int count;

   count = commonbot - commontop + 1;

   while(--count >= 0)
   {
      *dest   = temptranmap[(*dest<<8) + *source];
      dest[1] = temptranmap[(dest[1]<<8) + source[1]];
      dest[2] = temptranmap[(dest[2]<<8) + source[2]];
      dest[3] = temptranmap[(dest[3]<<8) + source[3]];
      source += 4;
      dest += linesize;
   }
}

static void R_FlushQuadFuzz(void)
{
   register byte *source = tempbuf + (commontop << 2);
   register byte *dest = ylookup[commontop] + columnofs[startx];
   register int count;
   int fuzz1, fuzz2, fuzz3, fuzz4;
   fuzz1 = fuzzpos;
   fuzz2 = (fuzz1 + MAX_SCREENHEIGHT) % FUZZTABLE;
   fuzz3 = (fuzz2 + MAX_SCREENHEIGHT) % FUZZTABLE;
   fuzz4 = (fuzz3 + MAX_SCREENHEIGHT) % FUZZTABLE;

   count = commonbot - commontop + 1;

   while(--count >= 0)
   {
      // SoM 7-28-04: Fix the fuzz problem.
      *dest = tempfuzzmap[6*256+dest[fuzzoffset[fuzz1] ? v_width: -v_width]];
      if(++fuzz1 == FUZZTABLE) fuzz1 = 0;
      dest[1] = tempfuzzmap[6*256+dest[1 + (fuzzoffset[fuzz2] ? v_width: -v_width)]];
      if(++fuzz2 == FUZZTABLE) fuzz2 = 0;
      dest[2] = tempfuzzmap[6*256+dest[2 + (fuzzoffset[fuzz3] ? v_width: -v_width)]];
      if(++fuzz3 == FUZZTABLE) fuzz3 = 0;
      dest[3] = tempfuzzmap[6*256+dest[3 + (fuzzoffset[fuzz4] ? v_width: -v_width)]];
      if(++fuzz4 == FUZZTABLE) fuzz4 = 0;

      source += 4;
      dest += linesize;
   }

   fuzzpos = fuzz4;
}

static void R_FlushQuadFlex(void)
{
   register byte *source = tempbuf + (commontop << 2);
   register byte *dest = ylookup[commontop] + columnofs[startx];
   register int count;
   unsigned int fg, bg;

   count = commonbot - commontop + 1;

   while(--count >= 0)
   {
      // haleyjd 09/12/04: use precalculated lookups
      fg = temp_fg2rgb[*source];
      bg = temp_bg2rgb[*dest];
      fg = (fg+bg) | 0xf07c3e1f;
      *dest = RGB8k[0][0][(fg>>5) & (fg>>19)];

      fg = temp_fg2rgb[source[1]];
      bg = temp_bg2rgb[dest[1]];
      fg = (fg+bg) | 0xf07c3e1f;
      dest[1] = RGB8k[0][0][(fg>>5) & (fg>>19)];

      fg = temp_fg2rgb[source[2]];
      bg = temp_bg2rgb[dest[2]];
      fg = (fg+bg) | 0xf07c3e1f;
      dest[2] = RGB8k[0][0][(fg>>5) & (fg>>19)];

      fg = temp_fg2rgb[source[3]];
      bg = temp_bg2rgb[dest[3]];
      fg = (fg+bg) | 0xf07c3e1f;
      dest[3] = RGB8k[0][0][(fg>>5) & (fg>>19)];

      source += 4;
      dest += linesize;
   }
}

static void (*R_FlushQuadColumn)(void) = R_QuadFlushError;

static void R_FlushColumns(void)
{
   if(temp_x != 4 || commontop >= commonbot)
   {
      while(--temp_x >= 0)
         R_FlushSingleColumn(temp_x, tempyl[temp_x], tempyh[temp_x]);
   }
   else
   {
      // haleyjd 09/11/04: unrolled loop, reordered
      if(tempyl[0] < commontop)
         R_FlushSingleColumn(0, tempyl[0], commontop);
      if(tempyl[1] < commontop)
         R_FlushSingleColumn(1, tempyl[1], commontop);
      if(tempyl[2] < commontop)
         R_FlushSingleColumn(2, tempyl[2], commontop);
      if(tempyl[3] < commontop)
         R_FlushSingleColumn(3, tempyl[3], commontop);
      if(tempyh[0] > commonbot)
         R_FlushSingleColumn(0, commonbot, tempyh[0]);
      if(tempyh[1] > commonbot)
         R_FlushSingleColumn(1, commonbot, tempyh[1]);
      if(tempyh[2] > commonbot)
         R_FlushSingleColumn(2, commonbot, tempyh[2]);
      if(tempyh[3] > commonbot)
         R_FlushSingleColumn(3, commonbot, tempyh[3]);

      R_FlushQuadColumn();
   }

   temp_x = 0;
   return;
}

//
// R_ResetColumnBuffer
//
// haleyjd 09/13/04: new function to call from main rendering loop
// which gets rid of the unnecessary reset of various variables during
// column drawing.
//
void R_ResetColumnBuffer(void)
{
   R_FlushColumns();
   temptype = COL_NONE;
   R_FlushSingleColumn = R_FlushError;
   R_FlushQuadColumn   = R_QuadFlushError;
}

// haleyjd 09/12/04: split up R_GetBuffer into various different
// functions to minimize the number of branches and take advantage
// of as much precalculated information as possible.

static byte *R_GetBufferOpaque(void)
{
   // haleyjd: reordered predicates
   if(temp_x == 4 ||
      (temp_x && (temptype != COL_OPAQUE || temp_x + startx != dc_x)))
      R_FlushColumns();

   if(!temp_x)
   {
      ++temp_x;
      startx = dc_x;
      *tempyl = commontop = dc_yl;
      *tempyh = commonbot = dc_yh;
      temptype = COL_OPAQUE;
      R_FlushSingleColumn = R_FlushSingleOpaque;
      R_FlushQuadColumn   = R_FlushQuadOpaque;
      return tempbuf + (dc_yl << 2);
   }

   tempyl[temp_x] = dc_yl;
   tempyh[temp_x] = dc_yh;
   
   if(dc_yl > commontop)
      commontop = dc_yl;
   if(dc_yh < commonbot)
      commonbot = dc_yh;
      
   return tempbuf + (dc_yl << 2) + temp_x++;
}

static byte *R_GetBufferTrans(void)
{
   // haleyjd: reordered predicates
   if(temp_x == 4 || tranmap != temptranmap ||
      (temp_x && (temptype != COL_TRANS || temp_x + startx != dc_x)))
      R_FlushColumns();

   if(!temp_x)
   {
      ++temp_x;
      startx = dc_x;
      *tempyl = commontop = dc_yl;
      *tempyh = commonbot = dc_yh;
      temptype = COL_TRANS;
      temptranmap = tranmap;
      R_FlushSingleColumn = R_FlushSingleTL;
      R_FlushQuadColumn   = R_FlushQuadTL;
      return tempbuf + (dc_yl << 2);
   }

   tempyl[temp_x] = dc_yl;
   tempyh[temp_x] = dc_yh;
   
   if(dc_yl > commontop)
      commontop = dc_yl;
   if(dc_yh < commonbot)
      commonbot = dc_yh;
      
   return tempbuf + (dc_yl << 2) + temp_x++;
}

static byte *R_GetBufferFlexTrans(void)
{
   // haleyjd: reordered predicates
   if(temp_x == 4 || temptranslevel != dc_translevel ||
      (temp_x && (temptype != COL_FLEXTRANS || temp_x + startx != dc_x)))
      R_FlushColumns();

   if(!temp_x)
   {
      ++temp_x;
      startx = dc_x;
      *tempyl = commontop = dc_yl;
      *tempyh = commonbot = dc_yh;
      temptype = COL_FLEXTRANS;
      temptranslevel = dc_translevel;
      
      // haleyjd 09/12/04: optimization -- calculate flex tran lookups
      // here instead of every time a column is flushed.
      {
         fixed_t fglevel, bglevel;
         
         fglevel = temptranslevel & ~0x3ff;
         bglevel = FRACUNIT - fglevel;
         temp_fg2rgb  = Col2RGB[fglevel >> 10];
         temp_bg2rgb  = Col2RGB[bglevel >> 10];
      }

      R_FlushSingleColumn = R_FlushSingleFlex;
      R_FlushQuadColumn   = R_FlushQuadFlex;
      return tempbuf + (dc_yl << 2);
   }

   tempyl[temp_x] = dc_yl;
   tempyh[temp_x] = dc_yh;
   
   if(dc_yl > commontop)
      commontop = dc_yl;
   if(dc_yh < commonbot)
      commonbot = dc_yh;
      
   return tempbuf + (dc_yl << 2) + temp_x++;
}

static byte *R_GetBufferFuzz(void)
{
   // haleyjd: reordered predicates
   if(temp_x == 4 ||
      (temp_x && (temptype != COL_FUZZ || temp_x + startx != dc_x)))
      R_FlushColumns();

   if(!temp_x)
   {
      ++temp_x;
      startx = dc_x;
      *tempyl = commontop = dc_yl;
      *tempyh = commonbot = dc_yh;
      temptype = COL_FUZZ;
      tempfuzzmap = dc_colormap; // SoM 7-28-04: Fix the fuzz problem.
      R_FlushSingleColumn = R_FlushSingleFuzz;
      R_FlushQuadColumn   = R_FlushQuadFuzz;
      return tempbuf + (dc_yl << 2);
   }

   tempyl[temp_x] = dc_yl;
   tempyh[temp_x] = dc_yh;
   
   if(dc_yl > commontop)
      commontop = dc_yl;
   if(dc_yh < commonbot)
      commonbot = dc_yh;
      
   return tempbuf + (dc_yl << 2) + temp_x++;
}

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
// 

// haleyjd 04/10/04: FIXME -- ASM version of R_DrawColumn is out
// of sync currently.

//#ifndef USEASM     // killough 2/15/98

void R_DrawColumn (void) 
{ 
   int              count; 
   register byte    *dest;            // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;     

   count = dc_yh - dc_yl + 1; 

   if (count <= 0)    // Zero length, column does not exceed a pixel.
      return;

#ifdef RANGECHECK 
   if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
      I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);    
#endif 

   // Framebuffer destination address.
   // Use ylookup LUT to avoid multiply with ScreenWidth.
   // Use columnofs LUT for subwindows? 

   // SoM: MAGIC
   dest = R_GetBufferOpaque();

   // Determine scaling, which is the only mapping to be done.

   fracstep = dc_iscale; 
   frac = dc_texturemid + (dc_yl-centery)*fracstep; 

   // Inner loop that does the actual texture mapping,
   //  e.g. a DDA-lile scaling.
   // This is as fast as it gets.       (Yeah, right!!! -- killough)
   //
   // killough 2/1/98: more performance tuning

   {
      register const byte *source = dc_source;            
      register const lighttable_t *colormap = dc_colormap; 
      register unsigned heightmask = dc_texheight-1;
      if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
         heightmask++;
         heightmask <<= FRACBITS;
          
         if (frac < 0)
            while ((frac += heightmask) <  0);
         else
            while (frac >= (int)heightmask)
               frac -= heightmask;
          
         do
         {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac>>FRACBITS]];
            dest += 4; //SoM: Oh, Oh it's MAGIC! You know...
            if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
         } 
         while (--count);
      }
      else
      {
         while ((count-=2)>=0)   // texture height is a power of 2 -- killough
         {
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4; //SoM: MAGIC 
            frac += fracstep;
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4;
            frac += fracstep;
         }
         if (count & 1)
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
   }
} 

// haleyjd 04/10/04: FIXME -- ASM version of R_DrawColumn is out
// of sync currently.

//#endif

// Here is the version of R_DrawColumn that deals with translucent  // phares
// textures and sprites. It's identical to R_DrawColumn except      //    |
// for the spot where the color index is stuffed into *dest. At     //    V
// that point, the existing color index and the new color index
// are mapped through the TRANMAP lump filters to get a new color
// index whose RGB values are the average of the existing and new
// colors.
//
// Since we're concerned about performance, the 'translucent or
// opaque' decision is made outside this routine, not down where the
// actual code differences are.

// haleyjd 04/10/04: FIXME -- ASM version of R_DrawTLColumn is out
// of sync currently.

//#ifndef USEASM                       // killough 2/21/98: converted to x86 asm

void R_DrawTLColumn (void)                                           
{ 
  int              count; 
  register byte    *dest;           // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;

  count = dc_yh - dc_yl + 1; 

  // Zero length, column does not exceed a pixel.
  if (count <= 0)
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  // SoM: MAGIC
  dest = R_GetBufferTrans();
  
  // Determine scaling,
  //  which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98, 2/21/98: more performance tuning
  
   {
      register const byte *source = dc_source;            
      register const lighttable_t *colormap = dc_colormap; 
      register unsigned heightmask = dc_texheight-1;
      if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
         heightmask++;
         heightmask <<= FRACBITS;
          
         if (frac < 0)
            while ((frac += heightmask) <  0);
         else
            while (frac >= (int)heightmask)
               frac -= heightmask;
          
         do
         {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac>>FRACBITS]];
            dest += 4; //SoM: Oh, Oh it's MAGIC! You know...
            if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
         } 
         while (--count);
      }
      else
      {
         while ((count-=2)>=0)   // texture height is a power of 2 -- killough
         {
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4; //SoM: MAGIC 
            frac += fracstep;
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4;
            frac += fracstep;
         }
         if (count & 1)
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
   }
} 

// haleyjd 04/10/04: FIXME -- ASM version of R_DrawTLColumn is out
// of sync currently.

//#endif  // killough 2/21/98: converted to x86 asm

//
// Spectre/Invisibility.
//


// SoM: Fuzz Stuff moved to beginning of the file
//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//

// sf: restored original fuzz effect (changed in mbf)
// sf: changed to use vis->colormap not fullcolormap
//     for coloured lighting and SHADOW now done with
//     flags not NULL colormap

void R_DrawFuzzColumn(void) 
{ 
  int      count; 

  // Adjust borders. Low...
  if (!dc_yl) 
    dc_yl = 1;

  // .. and high.
  if (dc_yh == viewheight-1) 
    dc_yh = viewheight - 2; 

  count = dc_yh - dc_yl;

  // Zero length.
  if (count < 0) 
    return; 
    
#ifdef RANGECHECK 
  // haleyjd: these should apparently be adjusted for hires
  // SoM: DONE
  if ((unsigned) dc_x >= v_width
      || dc_yl < 0 
      || dc_yh >= v_height)
    I_Error ("R_DrawFuzzColumn: %i to %i at %i",
             dc_yl, dc_yh, dc_x);
#endif

  // Keep till detailshift bug in blocky mode fixed,
  //  or blocky mode removed.

  // Does not work with blocky mode.
  // SoM: MAGIC
  R_GetBufferFuzz();
  // REAL MAGIC... you ready for this?
  return; // DONE
}

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//

// haleyjd: changed translationtables to byte **
byte *dc_translation, **translationtables = NULL;

// haleyjd: new stuff
int firsttranslationlump, lasttranslationlump;
int numtranslations = 0;

void R_DrawTranslatedColumn(void) 
{ 
  int      count; 
  byte     *dest; 
  fixed_t  frac;
  fixed_t  fracstep;     
 
  count = dc_yh - dc_yl; 
  if (count < 0) 
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT)
    I_Error ( "R_DrawColumn: %i to %i at %i",
              dc_yl, dc_yh, dc_x);
#endif 

  // FIXME. As above.
  // SoM: MAGIC
  dest = R_GetBufferOpaque();

  // Looks familiar.
  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  count++;        // killough 1/99: minor tuning

   // Here we do an additional index re-mapping.
   {
      register const byte *source = dc_source;            
      register const lighttable_t *colormap = dc_colormap; 
      register unsigned heightmask = dc_texheight-1;
      if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
         heightmask++;
         heightmask <<= FRACBITS;
          
         if (frac < 0)
            while ((frac += heightmask) <  0);
         else
            while (frac >= (int)heightmask)
               frac -= heightmask;
          
         do
         {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[dc_translation[source[frac>>FRACBITS]]];
            dest += 4; //SoM: Oh, Oh it's MAGIC! You know...
            if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
         } 
         while (--count);
      }
      else
      {
         while ((count-=2)>=0)   // texture height is a power of 2 -- killough
         {
            *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
            dest += 4; //SoM: MAGIC 
            frac += fracstep;
            *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
            dest += 4;
            frac += fracstep;
         }
         if (count & 1)
            *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
      }
   }
} 

//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//

typedef struct
{
  int start;      // start of the sequence of colours
  int number;     // number of colours
} translat_t;

translat_t translations[TRANSLATIONCOLOURS] =
{
    {96,  16},     // indigo
    {64,  16},     // brown
    {32,  16},     // red
  
  /////////////////////////
  // New colours
  
    {176, 16},     // tomato
    {128, 16},     // dirt
    {200, 8},      // blue
    {160, 8},      // gold
    {152, 8},      // felt?
    {0,   1},      // bleeacckk!!
    {250, 5},      // purple
  //  {168, 8}, // bright pink, kinda
    {216, 8},      // vomit yellow
    {16,  16},     // pink
    {56,  8},      // cream
    {88,  8},      // white
};

// 
// R_InitTranslationTables
//
// haleyjd 01/12/04: rewritten to support translation lumps
//
void R_InitTranslationTables(void)
{
   int numlumps, i, c;
   
   // don't leak the allocation
   if(translationtables)
   {
      for(i = 0; i < numtranslations; ++i)
         Z_Free(translationtables[i]);

      Z_Free(translationtables);

      // SoM: let's try... this.
      translationtables = NULL;
   }

   // count number of lumps
   firsttranslationlump = W_CheckNumForName("T_START");
   lasttranslationlump  = W_CheckNumForName("T_END");

   if(firsttranslationlump == -1 || lasttranslationlump == -1)
      numlumps = 0;
   else
      numlumps = (lasttranslationlump - firsttranslationlump) - 1;

   // set numtranslations
   numtranslations = TRANSLATIONCOLOURS + numlumps;

   // allocate the array of pointers
   translationtables = Z_Malloc(sizeof(byte *) * numtranslations, PU_STATIC, 0);
   
   // build the internal player translations
   for(i = 0; i < TRANSLATIONCOLOURS; ++i)
   {
      byte *transtbl;

      transtbl = translationtables[i] = Z_Malloc(256, PU_STATIC, 0);

      for(c = 0; c < 256; ++c)
      {
         transtbl[c] =
            (c < 0x70 || c > 0x7f) ? c : translations[i].start +
             ((c & 0xf) * (translations[i].number-1))/15;
      }
   }

   // read in the lumps, if any
   for(i = TRANSLATIONCOLOURS; i < numtranslations; ++i)
   {
      int lumpnum = (i - TRANSLATIONCOLOURS) + firsttranslationlump + 1;

      translationtables[i] = W_CacheLumpNum(lumpnum, PU_STATIC);
   }
}

//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//

int  ds_y; 
int  ds_x1; 
int  ds_x2;

lighttable_t *ds_colormap; 

fixed_t ds_xfrac; 
fixed_t ds_yfrac; 
fixed_t ds_xstep; 
fixed_t ds_ystep;

// start of a 64*64 tile image 
byte *ds_source;        

#ifndef USEASM      // killough 2/15/98

void R_DrawSpan (void) 
{ 
  register unsigned position;
  unsigned step;

  byte *source;
  byte *colormap;
  byte *dest;
    
  unsigned count;
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;
                
  position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
  step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
                
  source = ds_source;
  colormap = ds_colormap;
  dest = ylookup[ds_y] + columnofs[ds_x1];       
  count = ds_x2 - ds_x1 + 1; 
        
  while (count >= 4)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[0] = colormap[source[spot]]; 

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[1] = colormap[source[spot]];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[2] = colormap[source[spot]];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[3] = colormap[source[spot]]; 
                
      dest += 4;
      count -= 4;
    } 

  while (count)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      *dest++ = colormap[source[spot]]; 
      count--;
    } 
} 

#endif

//
// R_InitBuffer 
// Creates lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//


// SoM: ANYRES
void R_InitBuffer(int width, int height)
{ 
  int i; 
  int st_height;

  linesize = v_width;    // killough 11/98

  // Handle resize,
  //  e.g. smaller view windows
  //  with border and/or status bar.
  viewwindowx = (v_width-viewwidth) >> 1;
  scaledwindowx = (SCREENWIDTH - width) >> 1;
  // Column offset. For windows.
  for (i = viewwidth ; i--; )   // killough 11/98
    columnofs[i] = viewwindowx + i;
    
  // Same with base row offset.
  st_height = gameModeInfo->StatusBar->height;

  if(viewwidth == v_width)
     viewwindowy = scaledwindowy = 0;
  else
  {
     viewwindowy = (v_height - ((st_height * globalyscale) >> FRACBITS) - viewheight) >> 1;
     scaledwindowy = (SCREENHEIGHT - st_height - height) >> 1;
  }
  
  // Precalculate all row offsets.

  for (i = viewheight; i--; )
    ylookup[i] = screens[0] + (i + viewwindowy) * linesize; // killough 11/98
} 

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//

void R_FillBackScreen (void) 
{ 
  // killough 11/98: trick to shadow variables
  // SoM: ANYRES use scaledwindowx and scaledwindowy instead
  int x, y; 
  patch_t *patch;

  giborder_t *border = gameModeInfo->border;

  int offset = border->offset;
  int size   = border->size;

  if(scaledviewwidth == 320)
    return;

  // haleyjd 08/16/02: some restructuring to use gameModeInfo

  // killough 11/98: use the function in m_menu.c
  V_DrawBackground(gameModeInfo->borderFlat, &backscreen1);
        
  patch = W_CacheLumpName(border->top, PU_CACHE);

  for(x = 0; x < scaledviewwidth; x += size)
    V_DrawPatch(scaledwindowx+x,scaledwindowy-offset,&backscreen1,patch);

  patch = W_CacheLumpName(border->bottom, PU_CACHE);

  for(x = 0; x < scaledviewwidth; x += size)   // killough 11/98:
    V_DrawPatch(scaledwindowx+x,scaledwindowy+scaledviewheight,&backscreen1,patch);

  patch = W_CacheLumpName(border->left, PU_CACHE);

  for(y = 0; y < scaledviewheight; y += size)  // killough 11/98
    V_DrawPatch(scaledwindowx-offset,scaledwindowy+y,&backscreen1,patch);
  
  patch = W_CacheLumpName(border->right, PU_CACHE);

  for(y = 0; y < scaledviewheight; y += size)  // killough 11/98
    V_DrawPatch(scaledwindowx+scaledviewwidth,scaledwindowy+y,&backscreen1,patch);

  // Draw beveled edge. 
  V_DrawPatch(scaledwindowx-offset,
              scaledwindowy-offset,
              &backscreen1,
              W_CacheLumpName(border->c_tl, PU_CACHE));
    
  V_DrawPatch(scaledwindowx+scaledviewwidth,
              scaledwindowy-offset,
              &backscreen1,
              W_CacheLumpName(border->c_tr, PU_CACHE));
    
  V_DrawPatch(scaledwindowx-offset,
              scaledwindowy+scaledviewheight,             // killough 11/98
              &backscreen1,
              W_CacheLumpName(border->c_bl, PU_CACHE));
    
  V_DrawPatch(scaledwindowx+scaledviewwidth,
              scaledwindowy+scaledviewheight,             // killough 11/98
              &backscreen1,
              W_CacheLumpName(border->c_br, PU_CACHE));
} 

//
// Copy a screen buffer.
//
// SoM: why the hell was this written to only take an offset and size parameter?
// this is a much nicer solution which fixes scaling issues in highres modes that aren't
// perfectly 4/3
void R_VideoErase(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{ 
   unsigned int ofs;

   // SoM: ANYRES
   // This recieves scaled offsets.
   if(v_width != SCREENWIDTH || v_height != SCREENHEIGHT)
   {
      w = realxarray[x + w] - realxarray[x];
      h = realyarray[y + h] - realyarray[y];
      x = realxarray[x];
      y = realyarray[y];
   }
         
   ofs = x + y * v_width;

   if(x == 0 && w == (unsigned int)v_width)
   {
      memcpy(screens[0]+ofs, screens[1]+ofs, w * h);   // LFB copy.
      return;
   }

   ofs += (h - 1) * v_width;

   while(h-- > 0)
   {
      memcpy(screens[0] + ofs, screens[1] + ofs, w);
      ofs -= v_width;
   }
} 

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
// SoM: Removed old killough hack and reformatted to use new R_VideoErase
//
void R_DrawViewBorder(void) 
{ 
   int side, st_height;
   
   if(scaledviewwidth == SCREENWIDTH) 
      return;

   // copy top
   // SoM: ANYRES
   R_VideoErase(0, 0, SCREENWIDTH, scaledwindowy);

   // copy sides
   side = scaledwindowx;
   R_VideoErase(0, scaledwindowy, side, scaledviewheight);
   R_VideoErase(SCREENWIDTH - side, scaledwindowy, side, scaledviewheight);

   // copy bottom 
   R_VideoErase(0, scaledwindowy + scaledviewheight, SCREENWIDTH, scaledwindowy);

   st_height = gameModeInfo->StatusBar->height;
   
   V_MarkRect(0,0,SCREENWIDTH, SCREENHEIGHT-st_height); 
} 

// haleyjd: experimental column drawer for masked sky textures
void R_DrawNewSkyColumn(void) 
{ 
  int              count; 
  register byte    *dest;            // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;     

  count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawNewSkyColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  dest = ylookup[dc_yl] + columnofs[dc_x];  

  // Determine scaling, which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98: more performance tuning

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register int heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
        heightmask++;
        heightmask <<= FRACBITS;
          
        if (frac < 0)
          while ((frac += heightmask) <  0);
        else
          while (frac >= heightmask)
            frac -= heightmask;
          
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough

            // haleyjd
            if(source[frac>>FRACBITS])
              *dest = colormap[source[frac>>FRACBITS]];
            dest += linesize;                     // killough 11/98
            if ((frac += fracstep) >= heightmask)
              frac -= heightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            if(source[(frac>>FRACBITS) & heightmask])
              *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
            if(source[(frac>>FRACBITS) & heightmask])
              *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
          }
        if ((count & 1) && source[(frac>>FRACBITS) & heightmask])
          *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
  }
} 

//
// R_DrawFlexTLColumn
//
// haleyjd 09/01/02: zdoom-style translucency
//
void R_DrawFlexTLColumn(void)
{ 
   int              count; 
   register byte    *dest;           // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;
   
   count = dc_yh - dc_yl + 1; 

   // Zero length, column does not exceed a pixel.
   if(count <= 0)
      return; 
                                 
#ifdef RANGECHECK 
   if((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT)
      I_Error ("R_DrawFlexTLColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 
   
   // SoM: MAGIC
   dest = R_GetBufferFlexTrans();
  
   fracstep = dc_iscale; 
   frac = dc_texturemid + (dc_yl-centery)*fracstep; 

   {
      register const byte *source = dc_source;            
      register const lighttable_t *colormap = dc_colormap; 
      register unsigned heightmask = dc_texheight-1;
      if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
         heightmask++;
         heightmask <<= FRACBITS;
          
         if (frac < 0)
            while ((frac += heightmask) <  0);
         else
            while (frac >= (int)heightmask)
               frac -= heightmask;
          
         do
         {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac>>FRACBITS]];
            dest += 4; //SoM: Oh, Oh it's MAGIC! You know...
            if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
         } 
         while (--count);
      }
      else
      {
         while ((count-=2)>=0)   // texture height is a power of 2 -- killough
         {
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4; //SoM: MAGIC 
            frac += fracstep;
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += 4;
            frac += fracstep;
         }
         if (count & 1)
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
   }
}

//
// R_DrawFlexTLColumn
//
// haleyjd 11/05/02: zdoom-style translucency w/translation, for
// player sprites
//
void R_DrawFlexTlatedColumn(void) 
{ 
   int      count; 
   byte     *dest; 
   fixed_t  frac;
   fixed_t  fracstep;     
   
   count = dc_yh - dc_yl; 
   if (count < 0) 
      return; 
   
#ifdef RANGECHECK 
   if((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT)
      I_Error ( "R_DrawColumn: %i to %i at %i",
      dc_yl, dc_yh, dc_x);
#endif 

   // MAGIC
   dest = R_GetBufferFlexTrans();
   
   // Looks familiar.
   fracstep = dc_iscale; 
   frac = dc_texturemid + (dc_yl-centery)*fracstep; 
   
   count++;        // killough 1/99: minor tuning
   
   // Here we do an additional index re-mapping.
   {
      register const byte *source = dc_source;            
      register const lighttable_t *colormap = dc_colormap; 
      register unsigned heightmask = dc_texheight-1;
      if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
         heightmask++;
         heightmask <<= FRACBITS;
          
         if (frac < 0)
            while ((frac += heightmask) <  0);
         else
            while (frac >= (int)heightmask)
               frac -= heightmask;
          
         do
         {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[dc_translation[source[frac>>FRACBITS]]];
            dest += 4; //SoM: Oh, Oh it's MAGIC! You know...
            if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
         } 
         while (--count);
      }
      else
      {
         while ((count-=2)>=0)   // texture height is a power of 2 -- killough
         {
            *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
            dest += 4; //SoM: MAGIC 
            frac += fracstep;
            *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
            dest += 4;
            frac += fracstep;
         }
         if (count & 1)
         *dest = colormap[dc_translation[source[(frac>>FRACBITS) & heightmask]]];
      }
   }
} 

//----------------------------------------------------------------------------
//
// $Log: r_draw.c,v $
// Revision 1.16  1998/05/03  22:41:46  killough
// beautification
//
// Revision 1.15  1998/04/19  01:16:48  killough
// Tidy up last fix's code
//
// Revision 1.14  1998/04/17  15:26:55  killough
// fix showstopper
//
// Revision 1.13  1998/04/12  01:57:51  killough
// Add main_tranmap
//
// Revision 1.12  1998/03/23  03:36:28  killough
// Use new 'fullcolormap' for fuzzy columns
//
// Revision 1.11  1998/02/23  04:54:59  killough
// #ifdef out translucency code since its in asm
//
// Revision 1.10  1998/02/20  21:57:04  phares
// Preliminarey sprite translucency
//
// Revision 1.9  1998/02/17  06:23:40  killough
// #ifdef out code duplicated in asm for djgpp targets
//
// Revision 1.8  1998/02/09  03:18:02  killough
// Change MAXWIDTH, MAXHEIGHT defintions
//
// Revision 1.7  1998/02/02  13:17:55  killough
// performance tuning
//
// Revision 1.6  1998/01/27  16:33:59  phares
// more testing
//
// Revision 1.5  1998/01/27  16:32:24  phares
// testing
//
// Revision 1.4  1998/01/27  15:56:58  phares
// Comment about invisibility
//
// Revision 1.3  1998/01/26  19:24:40  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:05:55  killough
// Use unrolled version of R_DrawSpan
//
// Revision 1.1.1.1  1998/01/19  14:03:02  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
