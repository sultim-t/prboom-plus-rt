/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: v_video_trans.c,v 1.1 2000/09/20 09:34:16 figgi Exp $
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
 *  Video mode translation & image enlarging
 *-----------------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: v_video_trans.c,v 1.1 2000/09/20 09:34:16 figgi Exp $";
#endif /* lint */

#ifdef HAVE_LINUX_BITOPS_H

// Use the standard ffz
#include <linux/bitops.h>

#else

// Portable equivalent
// Proff - added __inline for VisualC
/*
#ifdef _MSC_VER
__inline
#else
inline
#endif
*/
#ifndef _MSC_VER
inline
#endif
static const unsigned long ffz(unsigned long x)
{
  unsigned long r = 0;
  while (x & 1) {
    x >>= 1;
    r++;
  }
  return r;
}

#endif

#include "doomtype.h"
#include "v_video.h"
#include "i_video.h"
#include "lprintf.h"
#include "v_video_trans.h"
#include "z_zone.h"
#include "m_swap.h"

// Blocky mode, replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
int multiply=1;
int dest_bpp=8;

pval* pixelvals = NULL;
static boolean pv_changed; // Has the above changed since last time?

// CPhipps - quick flag to determine if we use separate rendering and image
//  buffers (e.g when screen multiplying or in highcolour)
boolean         expand_buffer;

//
// TrueColor palette builder API
//

boolean true_color;
colourshift_t redshift, greenshift, blueshift;

//
// I_SetColourShift
//
// Constructs the shifts necessary to set one element of an RGB colour value

void I_SetColourShift(unsigned long mask, colourshift_t* ps)
{
  // Find first 1 bit; pshift is the position of the 1 bit
  // Also add 1 to this bit; as bits are contiguous, this should 
  //  leave a single
  //  1 bit after the old position of the most significant 1 bit
  mask += 1 << (ps->pshift = ffz(~mask));
  
  // Find that 1 bit, and set rshift
  ps->rshift = 8 - (ffz(~mask) - ps->pshift);

  // Detect non-contigous bit patterns
  if ((mask ^ (1 << ffz(~mask))) != 0)
    I_Error("I_SetColourShift: non-contigous bit mask\n");
}

void I_SetPaletteTranslation(const byte* palette)
{
  // This builds a suitable palette for DirectColor and TrueColor modes
  register unsigned short int i = 256;
  register pval *ppnum;
  register const byte *const  gtable = gammatable[usegamma];

  pv_changed = true; // Flag the buffer copying routines that palette
  //  has changed. In case they maintain/derive their own palette
  // from pixelvals instead of using it directly.

  if (pixelvals == NULL)
    pixelvals = Z_Malloc(256 * sizeof(*pixelvals), PU_STATIC, NULL);
  
  ppnum = pixelvals;
  do
  {
    *ppnum++ = 
        (((pval)gtable[palette[0]] >> redshift.rshift) << redshift.pshift)
      | (((pval)gtable[palette[1]] >> greenshift.rshift) << greenshift.pshift)
      | (((pval)gtable[palette[2]] >> blueshift.rshift) << blueshift.pshift);

    palette+=3;
  } while (--i);
}

//
// Image expanding
//

static void I_Double8Buf(pval* out_buffer, const byte* src)
{
  register unsigned long *olineptr = out_buffer;
  register const unsigned long *ilineptr = (const unsigned long*)src;
  int y = SCREENHEIGHT;

  do {
    register int x = SCREENWIDTH/4;
    do {
      register unsigned int twoopixels;
      register unsigned int twomoreopixels;
      unsigned long* oline2ptr = olineptr + SCREENWIDTH*2 / sizeof(*olineptr);
#ifndef I386_ASM
      unsigned int fouripixels = *ilineptr++;
      twoopixels =	(fouripixels & 0xff000000)
	|	((fouripixels>>8) & 0xffff00)
	|	((fouripixels>>16) & 0xff);
      twomoreopixels =	((fouripixels<<16) & 0xff000000)
	|	((fouripixels<<8) & 0xffff00)
	|	(fouripixels & 0xff);
#else
# ifdef _MSC_VER
      unsigned int fouripixels = *ilineptr++;
      twoopixels =	(fouripixels & 0xff000000)
	|	((fouripixels>>8) & 0xffff00)
	|	((fouripixels>>16) & 0xff);
      twomoreopixels =	((fouripixels<<16) & 0xff000000)
	|	((fouripixels<<8) & 0xffff00)
	|	(fouripixels & 0xff);
# else /* _MSC_VER */
      asm( "movl %%eax, %%edx ; "
	       "shrl $16, %%eax ; "
	       "movb %%dh, %%cl ; "
	       "movb %%dh, %%ch ; "
	       "movb %%ah, %%bh ; "
	       "shll $16, %%ecx ; "
	       "movb %%ah, %%bl ; "
	       "shll $16, %%ebx ; "
	       "movb %%dl, %%ch ; "
	       "movb %%al, %%bh ; "
	       "movb %%dl, %%cl ; "
	       "movb %%al, %%bl ; "
	       : "=b" (twoopixels), "=c" (twomoreopixels)
	       : "a" (*ilineptr++) : "%cc", "%edx");
# endif /* _MSC_VER */
#endif
#ifdef WORDS_BIGENDIAN
      olineptr[0] = oline2ptr[0] = twoopixels;
      olineptr[1] = oline2ptr[1] = twomoreopixels;
#else
      olineptr[0] = oline2ptr[0] = twomoreopixels;
      olineptr[1] = oline2ptr[1] = twoopixels;
#endif
      olineptr+=2;
    } while (--x);

    olineptr += SCREENWIDTH * 2 / sizeof(*olineptr);
  } while (--y);
}

static void I_Triple8Buf(pval* out_buffer, const byte* src)
{
  register unsigned long *olineptr = out_buffer;
  const unsigned long *ilineptr = (const unsigned long*)src;
  int x, y;
  unsigned int fouropixels[3];
  unsigned int fouripixels;
  const int lineadd = SCREENWIDTH * 3 / sizeof(*olineptr);
  
  y = SCREENHEIGHT;
  while (y--) {
    x = SCREENWIDTH/4;
    do {
      fouripixels = *ilineptr++;
      fouropixels[0] = (fouripixels & 0xff000000)
	|	((fouripixels>>8) & 0xff0000)
	|	((fouripixels>>16) & 0xffff);
      fouropixels[1] = ((fouripixels<<8) & 0xff000000)
	|	(fouripixels & 0xffff00)
	|	((fouripixels>>8) & 0xff);
      fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
	|	((fouripixels<<8) & 0xff00)
	|	(fouripixels & 0xff);
#ifdef WORDS_BIGENDIAN
      olineptr[0] = fouropixels[0];
      olineptr[lineadd / sizeof (*olineptr)] = fouropixels[0];
      olineptr[2 * lineadd] = fouropixels[0];
      olineptr[1] = fouropixels[1];
      olineptr[1 + lineadd] = fouropixels[1];
      olineptr[1 + 2 * lineadd] = fouropixels[1];
      olineptr[2] = fouropixels[2];
      olineptr[2 + lineadd] = fouropixels[2];
      olineptr[2 + 2 * lineadd] = fouropixels[2];
#else
      olineptr[0] = fouropixels[2];
      olineptr[lineadd] = fouropixels[2];
      olineptr[2 * lineadd] = fouropixels[2];
      olineptr[1] = fouropixels[1];
      olineptr[1 + lineadd] = fouropixels[1];
      olineptr[1 + 2 * lineadd] = fouropixels[1];
      olineptr[2] = fouropixels[0];
      olineptr[2 + lineadd] = fouropixels[0];
      olineptr[2 + 2 * lineadd] = fouropixels[0];
#endif
      olineptr += 3;
    } while (--x);
    olineptr += 2*lineadd;
  }
}

static const void* fake; // Force gcc to include some asm of mine

static void I_Copyto16Buf(pval* out_buffer, const byte* src)
{
  register const byte* iptr = src;
  register unsigned short* optr = (unsigned short*)out_buffer;
  int count = SCREENHEIGHT * SCREENWIDTH / 4;

  if (pixelvals == NULL) return;

#ifndef I386_ASM
  do {
    optr[0] = (unsigned short)pixelvals[iptr[0]];
    optr[1] = (unsigned short)pixelvals[iptr[1]];
    optr[2] = (unsigned short)pixelvals[iptr[2]];
    optr[3] = (unsigned short)pixelvals[iptr[3]];

    iptr += 4; optr += 4;
  } while (--count);
#else
# ifdef _MSC_VER
  do {
    optr[0] = (unsigned short)pixelvals[iptr[0]];
    optr[1] = (unsigned short)pixelvals[iptr[1]];
    optr[2] = (unsigned short)pixelvals[iptr[2]];
    optr[3] = (unsigned short)pixelvals[iptr[3]];

    iptr += 4; optr += 4;
  } while (--count);
# else /* _MSC_VER */
  asm("xorl %%eax,%%eax ; "
      ".align 8 ; "
      "0:"
      "movb (%%esi),%%al ; "
      "addl $4,%%esi ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -3(%%esi),%%al ; "
      "movw %%dx,(%%edi) ; "
      "addl $8,%%edi ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -2(%%esi),%%al ; "
      "movw %%dx,-6(%%edi) ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -1(%%esi),%%al ; "
      "movw %%dx,-4(%%edi) ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movw %%dx,-2(%%edi) ; "
      "dec %%ecx ; jg 0b"
      :: "D" (optr), "S" (iptr), "b" (pixelvals), "c" (count)
      : "%eax", "%edx");
# endif /* _MSC_VER */
#endif
}

static void I_Double16Buf(pval* out_buffer, const byte* src)
{
  register const byte* iptr = src;
  register unsigned long* optr = out_buffer;
  int y = SCREENHEIGHT;
  static unsigned long* pixelpair = NULL;

  if (pixelvals == NULL) return;

  if (pv_changed || (pixelpair == NULL)) {
    int i;

    // Avoid all the shifts necessary to 'double' the 16bit pixel value
    // into 2 adjacent 16-bit pixels, by pre-doubling a lookup table.

    if (pixelpair == NULL)
      pixelpair = Z_Malloc(256 * sizeof(*pixelpair), PU_STATIC, NULL);

    for (i = 0; i<256; i++)
      pixelpair[i] = (pixelvals[i] << 16) | pixelvals[i];

    pv_changed = false;
  }

  do {
    int x = SCREENWIDTH/4;

    do {
      unsigned long* optr2 = optr + 4*SCREENWIDTH / sizeof(*optr);

      optr[0] = optr2[0] = pixelpair[iptr[0]];
      optr[1] = optr2[1] = pixelpair[iptr[1]];
      optr[2] = optr2[2] = pixelpair[iptr[2]];
      optr[3] = optr2[3] = pixelpair[iptr[3]];

      iptr+=4; optr+=4;
    } while (--x);

    optr+=4*SCREENWIDTH/sizeof(*optr);
  } while (--y);

  fake = iptr;
}

#ifdef I386
// Nasty cast needed for outputting 3-byte pixelvals
#define LONG_AT(p) *((unsigned long*)(p))

static byte buffor24bpp[4*3+1];

static void I_Copyto24Buf(pval* out_buffer, const byte* src)
{
#ifndef I386
#error Warning!! Non-portable
#endif
  register const byte* iptr = src;
  register unsigned char* optr = (unsigned char*)out_buffer;
  register int count = SCREENHEIGHT * SCREENWIDTH / 4;

  if (pixelvals == NULL) return;

  do {
    LONG_AT(buffor24bpp+9) = pixelvals[iptr[3]];
    LONG_AT(buffor24bpp+6) = pixelvals[iptr[2]];
    LONG_AT(buffor24bpp+3) = pixelvals[iptr[1]];
    LONG_AT(buffor24bpp) = pixelvals[iptr[0]];
    iptr += 4;
    memcpy(optr, buffor24bpp+1, 3*4);
    optr += 3*4; 
  } while (--count);
}

static void I_Double24Buf(pval* out_buffer, const byte* src)
{
  register const byte* iptr = src;
  register unsigned char* optr = (unsigned char*)out_buffer;
  register int y = SCREENHEIGHT;

  if (pixelvals == NULL) return;

  do {
    register int x = SCREENWIDTH / 4;

    do {
      unsigned int w;
      // Note - order is important in these overlapping writes
      LONG_AT(buffor24bpp+9) = w = pixelvals[iptr[1]];
      LONG_AT(buffor24bpp+6) = w;
      LONG_AT(buffor24bpp+3) = w = pixelvals[iptr[0]];
      LONG_AT(buffor24bpp) = w;
      iptr += 2;
      memcpy(optr, buffor24bpp+1, 3*4);
      memcpy(optr + (6*SCREENWIDTH / sizeof(*optr)), buffor24bpp+1, 3*4);
      optr += 3*4; 
    } while (--x);

    optr += 6*SCREENWIDTH / sizeof(*optr);
  } while (--y);
}

#undef LONG_AT
#endif

static void I_Copyto32Buf(pval* out_buffer, const byte* src)
{
  register const byte* iptr = src;
  register unsigned long* optr = (unsigned long*)out_buffer;
  int count = SCREENHEIGHT * SCREENWIDTH / 4;

  if (pixelvals == NULL) return;

#ifndef I386_ASM
  do {
    optr[0] = pixelvals[iptr[0]];
    optr[1] = pixelvals[iptr[1]];
    optr[2] = pixelvals[iptr[2]];
    optr[3] = pixelvals[iptr[3]];

    iptr += 4; optr += 4;
  } while (--count);
#else
# ifdef _MSC_VER
  do {
    optr[0] = pixelvals[iptr[0]];
    optr[1] = pixelvals[iptr[1]];
    optr[2] = pixelvals[iptr[2]];
    optr[3] = pixelvals[iptr[3]];

    iptr += 4; optr += 4;
  } while (--count);
# else /* _MSC_VER */
  asm("xorl %%eax,%%eax ; "
      ".align 8 ; " 
      "0:"
      "movb (%%esi),%%al ; "
      "addl $4,%%esi ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -3(%%esi),%%al ; "
      "movl %%edx,(%%edi) ; "
      "addl $16,%%edi ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -2(%%esi),%%al ; "
      "movl %%edx,-12(%%edi) ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movb -1(%%esi),%%al ; "
      "movl %%edx,-8(%%edi) ; "
      "movl (%%ebx,%%eax,4),%%edx ; "
      "movl %%edx,-4(%%edi) ; "
      "dec %%ecx ; jg 0b"
      :: "D" (optr), "S" (iptr), "b" (pixelvals), "c" (count)
      : "%eax", "%edx");
# endif /* _MSC_VER */
#endif
}

static void I_Double32Buf(pval* out_buffer, const byte* src)
{
  register const byte* iptr = src;
  register unsigned long* optr = (unsigned long*)out_buffer;
  int y = SCREENHEIGHT;

  if (pixelvals == NULL) return;

  do {
    int x = SCREENWIDTH/4;

    do {
      unsigned long* optr2 = optr + 4*SCREENWIDTH / sizeof(*optr);

      optr[0] = optr[1] = optr2[0] = optr2[1] = pixelvals[iptr[0]];
      optr[2] = optr[3] = optr2[2] = optr2[3] = pixelvals[iptr[1]];
      optr[4] = optr[5] = optr2[4] = optr2[5] = pixelvals[iptr[2]];
      optr[6] = optr[7] = optr2[6] = optr2[7] = pixelvals[iptr[3]];

      iptr+=4; optr+=8; // phew, 4 bytes to 8 dwords!
    } while (--x);

    optr+=8*SCREENWIDTH/sizeof(*optr);
  } while (--y);

  fake = iptr;
}

//
// Image expanding API
//

// Table of image expanding functions
//
#define MAX_BYTESPP 4
#define MAX_MULT 4

typedef void (*expand_func_t)(pval*, const byte*);

void (*I_ExpandImage)(pval* dest, const byte* src);

const expand_func_t pExpander[MAX_BYTESPP][MAX_MULT] = 
{ { NULL, I_Double8Buf, I_Triple8Buf, NULL }, // 8 bpp modes
  { I_Copyto16Buf, I_Double16Buf, NULL, NULL }, // 15/16 bpp modes
#ifdef I386
  { I_Copyto24Buf, I_Double24Buf, NULL, NULL }, // 24 bpp modes
#else
  { NULL, NULL, NULL, NULL }, // 24 bpp modes unsupported on other arch's
#endif
  { I_Copyto32Buf, I_Double32Buf, NULL, NULL }, // 32 bpp modes
};

boolean I_QueryImageTranslation(void)
{
  if (multiply > MAX_MULT   ) return false;
  if (BYTESPP > MAX_BYTESPP) return false;

  expand_buffer = (multiply == 1) && (dest_bpp == 8) ? false : true;
  true_color = (dest_bpp > 8) ? true : false;

  if (!expand_buffer) return true;
  return ((pExpander[BYTESPP -1][multiply-1] == NULL) ? false : true);
}

size_t I_InitImageTranslation(void)
{
  I_QueryImageTranslation(); // Set flags

  I_ExpandImage = pExpander[BYTESPP -1][multiply-1];
  
  if (expand_buffer)
    return (SCREENWIDTH*multiply*SCREENHEIGHT*multiply*BYTESPP);
  else
    return 0;
}

// 
// I_EndImageTranslation
//
// Cleanup I_InitImageTranslation stuff
void I_EndImageTranslation(void)
{
}
