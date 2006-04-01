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
 *      The actual span/column drawing functions.
 *      Here find the main potential for optimization,
 *       e.g. inline assembly, different algorithms.
 *
 *-----------------------------------------------------------------------------*/

#include "doomstat.h"
#include "w_wad.h"
#include "r_main.h"
#include "v_video.h"
#include "st_stuff.h"
#include "g_game.h"
#include "am_map.h"
#include "lprintf.h"

#define MAXWIDTH  MAX_SCREENWIDTH          /* kilough 2/8/98 */
#define MAXHEIGHT MAX_SCREENHEIGHT

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
int  viewheight;
int  viewwindowx;
int  viewwindowy;

// leban 1/17/99:
//
// these next two are pre-calculated to help the speed of the inner
// loops.  however, they're not a win on a powerpc, and probably not
// on any other modern cpu that isn't afraid of multiplies.
//
// consider ylookup.  below, it's initialized in a loop as
//    columnofs[i] = viewwindowx + i;
// that's one addition.  indexing into an array is one addition.
// but since columnofs is an array with global scope, loading usually
// is another instruction.  on powerpc, the value is stored in the
// TOC instead of the address.  i think x86 does something similar,
// as someone added a bunch of local copies of similar variables below.
// that tactic can move an extra load out of an inner loop.
//
// but wait, there's more, as an array offset must be converted into
// array units, which in this case is most likely a shift left.  that's
// one more instruction wasted per array index.
//
// there's also an extra benefit on powerpc:  the number of registers
// used in R_DrawColumn is reduced, and a stack frame is no longer
// needed.  there's another two instructions saved.
//
// i'll leave these two in for now, but they could eventually get
// removed.  columnofs[] is actually referenced elsewhere.  topleft
// isn't static to work around a metrowerks compiler bug.
//
// XXX
//
// CPhipps - also to use it in the i386 asm I need it global

byte *ylookup[MAXHEIGHT];
//int  columnofs[MAXWIDTH];
byte *topleft;

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//

// CPhipps - made const*'s
const byte *tranmap;          // translucency filter maps 256x256   // phares
const byte *main_tranmap;     // killough 4/11/98

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

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//

void R_DrawColumn (void)
{
  int              count;
  register byte    *dest;            // killough
  register fixed_t frac;            // killough

  // leban 1/17/99:
  // removed the + 1 here, adjusted the if test, and added an increment
  // later.  this helps a compiler pipeline a bit better.  the x86
  // assembler also does this.

  count = dc_yh - dc_yl;

  // leban 1/17/99:
  // this case isn't executed too often.  depending on how many instructions
  // there are between here and the second if test below, this case could
  // be moved down and might save instructions overall.  since there are
  // probably different wads that favor one way or the other, i'll leave
  // this alone for now.
  if (count < 0)    // Zero length, column does not exceed a pixel.
    return;

  count++;

#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= SCREENHEIGHT)
    I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // Framebuffer destination address.
  dest = topleft + dc_yl*SCREENWIDTH + dc_x;

  // Determine scaling, which is the only mapping to be done.
#define  fracstep dc_iscale
  frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98: more performance tuning

    if (dc_texheight == 128) {
        while(count--)
        {
                *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
                dest += SCREENWIDTH;
                frac += fracstep;
        }
    } else if (dc_texheight == 0) {
  /* cph - another special case */
  while (count--) {
    *dest = dc_colormap[dc_source[frac>>FRACBITS]];
    dest += SCREENWIDTH;
    frac += fracstep;
  }
    } else {
     register unsigned heightmask = dc_texheight-1; // CPhipps - specify type
     if (! (dc_texheight & heightmask) )   // power of 2 -- killough
     {
         while (count>0)   // texture height is a power of 2 -- killough
           {
             *dest = dc_colormap[dc_source[(frac>>FRACBITS) & heightmask]];
             dest += SCREENWIDTH;
             frac += fracstep;
            count--;
           }
     }
     else
     {
         heightmask++;
         heightmask <<= FRACBITS;

         if (frac < 0)
           while ((frac += heightmask) <  0);
         else
           while (frac >= (int)heightmask)
             frac -= heightmask;

         while(count>0)
           {
             // Re-map color indices from wall texture column
             //  using a lighting/special effects LUT.

             // heightmask is the Tutti-Frutti fix -- killough

             *dest = dc_colormap[dc_source[frac>>FRACBITS]];
             dest += SCREENWIDTH;
             if ((frac += fracstep) >= (int)heightmask)
               frac -= heightmask;
            count--;
           }
     }
    }
}
#undef fracstep

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

void R_DrawTLColumn (void)
{
  int              count;
  register byte    *dest;           // killough
  register fixed_t frac;            // killough

  count = dc_yh - dc_yl + 1;

  // Zero length, column does not exceed a pixel.
  if (count <= 0)
    return;

#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= SCREENHEIGHT)
    I_Error("R_DrawTLColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // Framebuffer destination address.
  dest = topleft + dc_yl*SCREENWIDTH + dc_x;

  // Determine scaling,
  //  which is the only mapping to be done.
#define  fracstep dc_iscale
  frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98, 2/21/98: more performance tuning

  {
    register const byte *source = dc_source;
    register const lighttable_t *colormap = dc_colormap;
    register unsigned heightmask = dc_texheight-1; // CPhipps - specify type
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

            *dest = tranmap[(*dest<<8)+colormap[source[frac>>FRACBITS]]]; // phares
            dest += SCREENWIDTH;
            if ((frac += fracstep) >= (int)heightmask)
              frac -= heightmask;
          }
        while (--count);
      }
    else
      {
	if (heightmask == -1 && frac < 0) frac = 0;
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
            dest += SCREENWIDTH;
            frac += fracstep;
            *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
            dest += SCREENWIDTH;
            frac += fracstep;
          }
        if (count & 1)
          *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
      }
  }
}
#undef fracstep

//
// Spectre/Invisibility.
//

#define FUZZTABLE 50
// proff 08/17/98: Changed for high-res
//#define FUZZOFF (SCREENWIDTH)
#define FUZZOFF 1

static const int fuzzoffset_org[FUZZTABLE] = {
  FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
  FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
  FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
  FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

static int fuzzoffset[FUZZTABLE];

static int fuzzpos = 0;

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//

void R_DrawFuzzColumn(void)
{
  int      count;
  byte     *dest;
  fixed_t  frac;
  fixed_t  fracstep;

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
  if ((unsigned) dc_x >= (unsigned)SCREENWIDTH
      || dc_yl < 0
      || (unsigned)dc_yh >= (unsigned)SCREENHEIGHT)
    I_Error("R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // Keep till detailshift bug in blocky mode fixed,
  //  or blocky mode removed.

  // Does not work with blocky mode.
  dest = topleft + dc_yl*SCREENWIDTH + dc_x;

  // Looks familiar.
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Looks like an attempt at dithering,
  // using the colormap #6 (of 0-31, a bit brighter than average).

  do
    {
      // Lookup framebuffer, and retrieve
      //  a pixel that is either one column
      //  left or right of the current one.
      // Add index from colormap to index.
      // killough 3/20/98: use fullcolormap instead of colormaps

      *dest = fullcolormap[6*256+dest[fuzzoffset[fuzzpos]]];

// Some varying invisibility effects can be gotten by playing // phares
// with this logic. For example, try                          // phares
//                                                            // phares
//    *dest = fullcolormap[0*256+dest[FUZZOFF]];              // phares

      // Clamp table lookup index.
      if (++fuzzpos == FUZZTABLE)
        fuzzpos = 0;

      dest += SCREENWIDTH;

      frac += fracstep;
    } while (count--);
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

byte *dc_translation, *translationtables;

void R_DrawTranslatedColumn (void)
{
  int      count;
  byte     *dest;
  fixed_t  frac;
  fixed_t  fracstep;

  count = dc_yh - dc_yl;
  if (count < 0)
    return;

#ifdef RANGECHECK
  if ((unsigned)dc_x >= (unsigned)SCREENWIDTH
      || dc_yl < 0
      || (unsigned)dc_yh >= (unsigned)SCREENHEIGHT)
    I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

  // FIXME. As above.
  dest = topleft + dc_yl*SCREENWIDTH + dc_x;

  // Looks familiar.
  fracstep = dc_iscale;
  frac = dc_texturemid + (dc_yl-centery)*fracstep;

  // Here we do an additional index re-mapping.
  do
    {
      // Translation tables are used
      //  to map certain colorramps to other ones,
      //  used with PLAY sprites.
      // Thus the "green" ramp of the player 0 sprite
      //  is mapped to gray, red, black/indigo.

      *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
      dest += SCREENWIDTH;

      frac += fracstep;
    }
  while (count--);
}

//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//

byte playernumtotrans[MAXPLAYERS];
extern lighttable_t *(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];

void R_InitTranslationTables (void)
{
  int i, j;
#define MAXTRANS 3
  byte transtocolour[MAXTRANS];

  // killough 5/2/98:
  // Remove dependency of colormaps aligned on 256-byte boundary

  if (translationtables == NULL) // CPhipps - allow multiple calls
    translationtables = Z_Malloc(256*MAXTRANS, PU_STATIC, 0);

  for (i=0; i<MAXTRANS; i++) transtocolour[i] = 255;

  for (i=0; i<MAXPLAYERS; i++) {
    byte wantcolour = mapcolor_plyr[i];
    playernumtotrans[i] = 0;
    if (wantcolour != 0x70) // Not green, would like translation
      for (j=0; j<MAXTRANS; j++)
  if (transtocolour[j] == 255) {
    transtocolour[j] = wantcolour; playernumtotrans[i] = j+1; break;
  }
  }

  // translate just the 16 green colors
  for (i=0; i<256; i++)
    if (i >= 0x70 && i<= 0x7f)
      {
  // CPhipps - configurable player colours
        translationtables[i] = colormaps[0][((i&0xf)<<9) + transtocolour[0]];
        translationtables[i+256] = colormaps[0][((i&0xf)<<9) + transtocolour[1]];
        translationtables[i+512] = colormaps[0][((i&0xf)<<9) + transtocolour[2]];
      }
    else  // Keep all other colors as is.
      translationtables[i]=translationtables[i+256]=translationtables[i+512]=i;
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

void R_DrawSpan (void)
{
  register unsigned count,xfrac = ds_xfrac,yfrac = ds_yfrac;

  byte *source;
  byte *colormap;
  byte *dest;

  source = ds_source;
  colormap = ds_colormap;
  dest = topleft + ds_y*SCREENWIDTH + ds_x1;
  count = ds_x2 - ds_x1 + 1;

  while (count)
    {
      register unsigned xtemp = xfrac >> 16;
      register unsigned ytemp = yfrac >> 10;
      register unsigned spot;
      ytemp &= 4032;
      xtemp &= 63;
      spot = xtemp | ytemp;
      xfrac += ds_xstep;
      yfrac += ds_ystep;
      *dest++ = colormap[source[spot]];
      count--;
    }
}

//
// R_InitBuffer
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//

void R_InitBuffer(int width, int height)
{
  int i=0;
  // Handle resize,
  //  e.g. smaller view windows
  //  with border and/or status bar.

  viewwindowx = (SCREENWIDTH-width) >> 1;

  // Same with base row offset.

  viewwindowy = width==SCREENWIDTH ? 0 : (SCREENHEIGHT-(ST_SCALED_HEIGHT-1)-height)>>1;

  topleft = screens[0] + viewwindowy*SCREENWIDTH + viewwindowx;

  // Preclaculate all row offsets.
  // CPhipps - merge viewwindowx into here

  for (i=0 ; i<height ; i++)
    ylookup[i] = screens[0] + (i+viewwindowy)*SCREENWIDTH + viewwindowx;

  for (i=0; i<FUZZTABLE; i++)
    fuzzoffset[i] = fuzzoffset_org[i]*SCREENWIDTH;
}

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
// CPhipps - patch drawing updated

void R_FillBackScreen (void)
{
  int     x,y;

  if (scaledviewwidth == SCREENWIDTH)
    return;

  V_DrawBackground(gamemode == commercial ? "GRNROCK" : "FLOOR7_2", 1);

  for (x=0; x<scaledviewwidth; x+=8)
    V_DrawNamePatch(viewwindowx+x,viewwindowy-8,1,"brdr_t", CR_DEFAULT, VPT_NONE);

  for (x=0; x<scaledviewwidth; x+=8)
    V_DrawNamePatch(viewwindowx+x,viewwindowy+viewheight,1,"brdr_b", CR_DEFAULT, VPT_NONE);

  for (y=0; y<viewheight; y+=8)
    V_DrawNamePatch(viewwindowx-8,viewwindowy+y,1,"brdr_l", CR_DEFAULT, VPT_NONE);

  for (y=0; y<viewheight; y+=8)
    V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy+y,1,"brdr_r", CR_DEFAULT, VPT_NONE);

  // Draw beveled edge.
  V_DrawNamePatch(viewwindowx-8,viewwindowy-8,1,"brdr_tl", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy-8,1,"brdr_tr", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx-8,viewwindowy+viewheight,1,"brdr_bl", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy+viewheight,1,"brdr_br", CR_DEFAULT, VPT_NONE);
}

//
// Copy a screen buffer.
//

void R_VideoErase(unsigned ofs, int count)
{
#ifndef GL_DOOM
  memcpy(screens[0]+ofs, screens[1]+ofs, count);   // LFB copy.
#endif
}

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//

void R_DrawViewBorder(void)
{
#ifdef GL_DOOM
  // proff 11/99: we don't have a backscreen in OpenGL from where we can copy this
  R_FillBackScreen();
#else

  int top, side, ofs, i;
// proff/nicolas 09/20/98: Added for high-res (inspired by DosDOOM)
  int side2;

// proff/nicolas 09/20/98: Removed for high-res
  //  if (scaledviewwidth == SCREENWIDTH)
  //  return;

// proff/nicolas 09/20/98: Added for high-res (inspired by DosDOOM)
  if ((SCREENHEIGHT != viewheight) ||
      ((automapmode & am_active) && ! (automapmode & am_overlay)))
  {
    ofs = ( SCREENHEIGHT - ST_SCALED_HEIGHT ) * SCREENWIDTH;
    side= ( SCREENWIDTH - ST_SCALED_WIDTH ) / 2;
    side2 = side * 2;

    R_VideoErase ( ofs, side );

    ofs += ( SCREENWIDTH - side );
    for ( i = 1; i < ST_SCALED_HEIGHT; i++ )
    {
      R_VideoErase ( ofs, side2 );
      ofs += SCREENWIDTH;
    }

    R_VideoErase ( ofs, side );
  }

  if ( viewheight >= ( SCREENHEIGHT - ST_SCALED_HEIGHT ))
    return; // if high-res, don´t go any further!

  top = ((SCREENHEIGHT-ST_SCALED_HEIGHT)-viewheight)/2;
  side = (SCREENWIDTH-scaledviewwidth)/2;

  // copy top and one line of left side
  R_VideoErase (0, top*SCREENWIDTH+side);

  // copy one line of right side and bottom
  ofs = (viewheight+top)*SCREENWIDTH-side;
  R_VideoErase (ofs, top*SCREENWIDTH+side);

  // copy sides using wraparound
  ofs = top*SCREENWIDTH + SCREENWIDTH-side;
  side <<= 1;

  for (i=1 ; i<viewheight ; i++)
    {
      R_VideoErase (ofs, side);
      ofs += SCREENWIDTH;
    }
#endif
}
