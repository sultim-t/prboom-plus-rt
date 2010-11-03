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
 *  I_SavePNG: PNG version of SDL_SaveBMP
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "SDL.h"

#ifdef HAVE_LIBPNG
#include <png.h>

// Write the palette for 8bpp screenshots
static int write_png_palette(
    png_struct *png_ptr, png_info *info_ptr, SDL_Surface *scr)
{
  int result = -1;
  png_color *palette;

  palette = malloc(sizeof(*palette) * scr->format->palette->ncolors);

  if (palette)
  {
    int i;

    // Convert SDL palette to libpng
    for (i = 0; i < scr->format->palette->ncolors; i++) {
      palette[i].red   = scr->format->palette->colors[i].r;
      palette[i].green = scr->format->palette->colors[i].g;
      palette[i].blue  = scr->format->palette->colors[i].b;
    }

    png_set_PLTE(png_ptr, info_ptr,
        palette, scr->format->palette->ncolors);

    free(palette);
    result = 0;
  }
  return result;
}

// Transform SDL pixel format into png_color
static void write_png_rgb_transform(unsigned char *buffer, SDL_Surface *scr)
{
  SDL_PixelFormat *fmt = scr->format;
  png_color *pixel = (png_color *)buffer;
  unsigned char *source = scr->pixels;
  int y;

  for (y = scr->w * scr->h; y > 0; pixel++, source += fmt->BytesPerPixel, y--)
  {
    Uint32 p = *(Uint32 *)source;
    pixel->red   = (((p & fmt->Rmask)>>fmt->Rshift)<<fmt->Rloss);
    pixel->green = (((p & fmt->Gmask)>>fmt->Gshift)<<fmt->Gloss);
    pixel->blue  = (((p & fmt->Bmask)>>fmt->Bshift)<<fmt->Bloss);
  }
}

// I_SavePNG will have set up the write, then it calls this to do it
static int write_png(
    png_struct *png_ptr, png_info *info_ptr, SDL_Surface *scr)
{
  int result = -1;
  int rgb = (scr->format->palette == NULL);
  size_t pixel_size = rgb ? sizeof(png_color) : 1; // sizeof(char) == 1
  unsigned char *row_data = malloc(scr->w * pixel_size);

  if (row_data)
  {
    int lock_needed = SDL_MUSTLOCK(scr);

    if (!lock_needed || SDL_LockSurface(scr) >= 0)
    {
      int y;

      png_write_info(png_ptr, info_ptr);

      for (y = 0; y < scr->h; y++)
      {
        if (rgb)
          ; // FIXME write_png_rgb_transform(pixel_data, scr);
        else
          memcpy(row_data, scr->pixels + y * scr->pitch, scr->w);

        png_write_row(png_ptr, row_data);
      }

      png_write_end(png_ptr, info_ptr);

      if (lock_needed)
        SDL_UnlockSurface(scr);

      result = 0;
    }

    free(row_data);
  }
  return result;
}

#endif // HAVE_LIBPNG

// I_SavePNG: PNG version of SDL_SaveBMP
int I_SavePNG(SDL_Surface *scr, const char *fname)
{
  int result = -1;
#ifdef HAVE_LIBPNG
  FILE *fp = fopen(fname, "wb");

  if (fp)
  {
    png_struct *png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, png_error_ptr_NULL, NULL, NULL);

    if (png_ptr)
    {
      png_info *info_ptr = png_create_info_struct(png_ptr);

      if (info_ptr)
      {
        png_time ptime;

        png_set_compression_level(png_ptr, 2);
        png_init_io(png_ptr, fp);
        png_set_IHDR(
            png_ptr, info_ptr, scr->w, scr->h, 8,
            ((scr->format->palette == NULL)
             ? PNG_COLOR_TYPE_RGB
             : PNG_COLOR_TYPE_PALETTE),
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        png_convert_from_time_t(&ptime, time(NULL));
        png_set_tIME(png_ptr, info_ptr, &ptime);

        if (scr->format->palette == NULL
            || (write_png_palette(png_ptr, info_ptr, scr) >= 0))
          result = write_png(png_ptr, info_ptr, scr);
      }
      png_destroy_write_struct(&png_ptr, png_infopp_NULL);
    }
    fclose(fp);
  }
#endif // HAVE_LIBPNG
  return result;
}
