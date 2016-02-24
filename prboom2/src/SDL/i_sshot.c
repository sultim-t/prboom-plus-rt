/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
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
 *  Screenshot functions, moved out of i_video.c
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "SDL.h"

#ifdef HAVE_LIBSDL2_IMAGE
#include <SDL_image.h>
#endif

#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"

//
// I_ScreenShot
//

int I_ScreenShot(const char *fname)
{
  int result = -1;
  unsigned char *pixels = I_GrabScreen();
  SDL_Surface *screenshot = SDL_CreateRGBSurfaceFrom(pixels, REAL_SCREENWIDTH, REAL_SCREENHEIGHT, 24,
    REAL_SCREENWIDTH * 3, 0x000000ff, 0x0000ff00, 0x00ff0000, 0);

  if (screenshot)
  {
#ifdef HAVE_LIBSDL2_IMAGE
    result = IMG_SavePNG(screenshot, fname);
#else
    result = SDL_SaveBMP(screenshot, fname);
#endif
    SDL_FreeSurface(screenshot);
  }
  return result;
}

// NSM
// returns current screern contents as RGB24 (raw)
// returned pointer should be freed when done

unsigned char *I_GrabScreen(void)
{
  static unsigned char *pixels = NULL;
  static int pixels_size = 0;
  int size;

  #ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL)
  {
    return gld_ReadScreen();
  }
  #endif

  size = REAL_SCREENHEIGHT * REAL_SCREENWIDTH * 3;
  if (!pixels || size > pixels_size)
  {
    pixels_size = size;
    pixels = realloc(pixels, size);
  }

  if (pixels)
  {
    SDL_RenderReadPixels(sdl_renderer, NULL, SDL_PIXELFORMAT_RGB24, pixels, REAL_SCREENWIDTH * 3);
  }

  return pixels;
}
