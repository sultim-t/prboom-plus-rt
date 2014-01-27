/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *      Screen multiply stuff. There are three algos:
 *      fast special methods for 2x and 4x and universal algo
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_screenmultiply.h"
#include "doomdef.h"
#include "lprintf.h"

int render_screen_multiply;
int screen_multiply;
int render_interlaced_scanning;
int interlaced_scanning_requires_clearing;

#ifdef WORDS_BIGENDIAN
#define INIT_DATA_DEST_2X unsigned int data_dest = (*(psrc + 1)) | ((*psrc) << 16);
#else
#define INIT_DATA_DEST_2X unsigned int data_dest = (*psrc) | ((*(psrc + 1)) << 16);
#endif

#define PROCESS_LINE_2X_1 \
{\
  x = SCREENWIDTH / 2;\
  while (x > 0)\
  {\
    INIT_DATA_DEST_2X;\
    data_dest |= data_dest << 8;\
    *pdest++ = data_dest;\
    psrc += 2;\
    x--;\
  }\
}\

#define PROCESS_LINE_2X_2 \
{\
  x = SCREENWIDTH;\
  while (x > 0)\
  {\
    INIT_DATA_DEST_2X;\
    *pdest++ = data_dest;\
    psrc++;\
    x--;\
  }\
}\

#define PROCESS_LINE_2X_4 \
{\
  x = SCREENWIDTH;\
  while (x > 0)\
  {\
    *pdest++ = *psrc;\
    *pdest++ = *psrc;\
    psrc++;\
    x--;\
  }\
}\

#define PROCESS_SCREEN_MULTIPLY_2X(SCREENTYPE, PIXELDEPTH) \
{\
  int x, y;\
  if (interlaced) {\
    for (y = ytop; y <= ybottom; y++)\
    {\
      unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 2));\
      SCREENTYPE *psrc = (SCREENTYPE *)(pixels_src + y * pitch_src);\
      PROCESS_LINE_2X_ ## PIXELDEPTH;\
    }\
  } else {\
    for (y = ytop; y <= ybottom; y++)\
    {\
      unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 2));\
      byte *pdest_saved = (byte *)pdest;\
      SCREENTYPE *psrc = (SCREENTYPE*)(pixels_src + y * pitch_src);\
      PROCESS_LINE_2X_ ## PIXELDEPTH;\
      memcpy(pdest_saved + pitch_dest, pdest_saved, pitch_dest);\
    }\
  }\
}\

static void R_ProcessScreenMultiplyBlock2x(byte* pixels_src, byte* pixels_dest,
  int pixel_depth, int pitch_src, int pitch_dest, int ytop, int ybottom, int interlaced)
{
  switch (pixel_depth)
  {
  case 1:
    PROCESS_SCREEN_MULTIPLY_2X(byte, 1);
    break;
  case 2:
    PROCESS_SCREEN_MULTIPLY_2X(unsigned short, 2);
    break;
  case 4:
    PROCESS_SCREEN_MULTIPLY_2X(unsigned int, 4);
    break;
  }
}

#define PROCESS_LINE_4X_1 \
{\
  x = SCREENWIDTH;\
  while (x > 0)\
  {\
    unsigned int data_dest = *psrc | (*psrc << 16);\
    data_dest |= data_dest << 8;\
    *pdest++ = data_dest;\
    psrc++;\
    x--;\
  }\
}\

#define PROCESS_LINE_4X_2 \
{\
  x = SCREENWIDTH;\
  while (x > 0)\
  {\
    unsigned int data_dest = *psrc | (*psrc << 16);\
    *pdest++ = data_dest;\
    *pdest++ = data_dest;\
    psrc++;\
    x--;\
  }\
}\

#define PROCESS_LINE_4X_4 \
{\
  x = SCREENWIDTH;\
  while (x > 0)\
  {\
    *pdest++ = *psrc;\
    *pdest++ = *psrc;\
    *pdest++ = *psrc;\
    *pdest++ = *psrc;\
    psrc++;\
    x--;\
  }\
}\

#define PROCESS_SCREEN_MULTIPLY_4X(SCREENTYPE, PIXELDEPTH) \
{\
  int i, x, y;\
  for (y = ytop; y <= ybottom; y++)\
  {\
    unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 4));\
    byte *pdest_saved = (byte *)pdest;\
    SCREENTYPE *psrc = (SCREENTYPE *)(pixels_src + y * pitch_src);\
    PROCESS_LINE_4X_ ## PIXELDEPTH;\
    if (!interlaced)\
    {\
      for (i = 1; i < screen_multiply; i++)\
        memcpy(pdest_saved + i * pitch_dest, pdest_saved, pitch_dest);\
    }\
  }\
}\

static void R_ProcessScreenMultiplyBlock4x(byte* pixels_src, byte* pixels_dest,
  int pixel_depth, int pitch_src, int pitch_dest, int ytop, int ybottom, int interlaced)
{
  switch (pixel_depth)
  {
  case 1:
    PROCESS_SCREEN_MULTIPLY_4X(byte, 1);
    break;
  case 2:
    PROCESS_SCREEN_MULTIPLY_4X(unsigned short, 2);
    break;
  case 4:
    PROCESS_SCREEN_MULTIPLY_4X(unsigned int, 4);
    break;
  }
}

#define PROCESS_SCREEN_MULTIPLY_UNI(SCREENTYPE) \
{\
  SCREENTYPE *pdest = (SCREENTYPE *)(pixels_dest + pitch_dest * (ybottom * screen_multiply));\
  byte *pdest_saved = (byte *)pdest;\
  SCREENTYPE *psrc  = (SCREENTYPE *)(pixels_src + pitch_src * ybottom);\
  SCREENTYPE *data_src;\
  for (y = ybottom; y >= ytop; y--)\
  {\
    data_src = psrc;\
    { /* GCC didn't like (byte *)psrc -= pitch_src; */ \
      byte *p = (byte *)psrc;\
      p -= pitch_src;\
      psrc = (SCREENTYPE *)p;\
    }\
    for (x = 0; x < SCREENWIDTH; x++, data_src++)\
    {\
      for (i = 0; i < screen_multiply; i++)\
      {\
        *pdest++ = *data_src;\
      }\
    }\
    if (!render_interlaced_scanning)\
    {\
      for (i = 1; i < screen_multiply; i++)\
        memcpy(pdest_saved + i * pitch_dest, pdest_saved, pitch_dest);\
    }\
    pdest = (SCREENTYPE *)(pdest_saved - pitch_dest * screen_multiply);\
    pdest_saved = (byte *)pdest;\
  }\
}\

static void R_ProcessScreenMultiplyBlock(byte* pixels_src, byte* pixels_dest,
  int pixel_depth, int pitch_src, int pitch_dest, int ytop, int ybottom, int interlaced)
{
  int x, y, i;

  switch (pixel_depth)
  {
  case 1:
    PROCESS_SCREEN_MULTIPLY_UNI(byte)
    break;
  case 2:
    PROCESS_SCREEN_MULTIPLY_UNI(unsigned short);
    break;
  case 4:
    PROCESS_SCREEN_MULTIPLY_UNI(unsigned int);
    break;
  }
}

void R_ProcessScreenMultiply(byte* pixels_src, byte* pixels_dest,
  int pixel_depth, int pitch_src, int pitch_dest)
{
  if (screen_multiply < 2)
    return;

  if (pixel_depth != 1 && pixel_depth != 2 && pixel_depth != 4)
    I_Error("R_ProcessScreenMultiply: unsupported pixel depth %d", pixel_depth);

  // there is no necessity to do it each tic
  if (interlaced_scanning_requires_clearing)
  {
    // needed for "directx" video driver with double buffering
    // two pages must be cleared
    interlaced_scanning_requires_clearing = (interlaced_scanning_requires_clearing + 1)%3;

    memset(pixels_dest, 0, pitch_dest * screen_multiply * SCREENHEIGHT);
  }

  switch (screen_multiply)
  {
  // special cases for 2x and 4x for max speed
  case 2:
    R_ProcessScreenMultiplyBlock2x(pixels_src, pixels_dest, pixel_depth,
      pitch_src, pitch_dest, 0, SCREENHEIGHT - 1, render_interlaced_scanning);
    break;
  case 4:
    R_ProcessScreenMultiplyBlock4x(pixels_src, pixels_dest, pixel_depth,
      pitch_src, pitch_dest, 0, SCREENHEIGHT - 1, render_interlaced_scanning);
    break;
  default:
    {
      // the following code works correctly even if pixels_src == pixels_dest
      dboolean same = (pixels_src == pixels_dest);
      R_ProcessScreenMultiplyBlock(pixels_src, pixels_dest, pixel_depth,
        pitch_src, pitch_dest, (same ? 1 : 0), SCREENHEIGHT - 1, render_interlaced_scanning);
      if (same)
      {
        // never happens after SDL_LockSurface()
        static byte *tmpbuf = NULL;
        if (!tmpbuf)
        {
          tmpbuf = malloc(pitch_src);
        }
        memcpy(tmpbuf, pixels_src, pitch_src);
        R_ProcessScreenMultiplyBlock(tmpbuf, pixels_dest, pixel_depth,
          pitch_src, pitch_dest, 0, 0, render_interlaced_scanning);
      }
      break;
    }
  }
}
