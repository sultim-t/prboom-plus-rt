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
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <SDL.h>
#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "doomtype.h"
#include "i_video.h"
#include "m_argv.h"
#include "lprintf.h"

#ifndef HIBYTE
#define HIBYTE(W) (((W) >> 8) & 0xFF)
#endif

int useglgamma;
int gl_DeviceSupportsGamma = false;

static Uint16 gl_oldHardwareGamma[3][256];

static void CalculateGammaRamp (float gamma, Uint16 *ramp)
{
  int i;

  // 0.0 gamma is all black
  if (gamma <= 0.0f)
  {
    for (i = 0; i < 256; i++)
    {
      ramp[i] = 0;
    }
    return;
  }
  else
  {
    // 1.0 gamma is identity
    if (gamma == 1.0f)
    {
      for (i = 0; i < 256; i++)
      {
        ramp[i] = (i << 8) | i;
      }
      return;
    }
    else
    {
      // Calculate a real gamma ramp
      int value;
      gamma = 1.0f / gamma;
      for (i = 0; i < 256; i++)
      {
        value = (int)(pow((double)i / 256.0, gamma) * 65535.0 + 0.5);
        if (value > 65535)
        {
          value = 65535;
        }
        ramp[i] = (Uint16)value;
      }
    }
  }
}

//
// gld_CheckHardwareGamma
//
// Determines if the underlying hardware supports the Win32 gamma correction API.
//
void gld_CheckHardwareGamma(void)
{
  gl_DeviceSupportsGamma = (-1 != SDL_GetGammaRamp(gl_oldHardwareGamma[0], gl_oldHardwareGamma[1], gl_oldHardwareGamma[2]));

  if (gl_DeviceSupportsGamma)
  {
    //
    // do a sanity check on the gamma values
    //
    if (
      (HIBYTE(gl_oldHardwareGamma[0][255]) <= HIBYTE(gl_oldHardwareGamma[0][0])) ||
      (HIBYTE(gl_oldHardwareGamma[1][255]) <= HIBYTE(gl_oldHardwareGamma[1][0])) ||
      (HIBYTE(gl_oldHardwareGamma[2][255]) <= HIBYTE(gl_oldHardwareGamma[2][0])))
    {
      gl_DeviceSupportsGamma = false;
    }

    //
    // make sure that we didn't have a prior crash in the game, and if so we need to
    // restore the gamma values to at least a linear value
    //
    if ((HIBYTE(gl_oldHardwareGamma[0][181]) == 255))
    //if ((HIBYTE(gl_oldHardwareGamma[0][247]) == 255))
    {
      int g;

      lprintf(LO_WARN, "gld_CheckHardwareGamma: suspicious gamma tables, using linear ramp for restoration\n");

      for ( g = 0; g < 255; g++ )
      {
        gl_oldHardwareGamma[0][g] = g << 8;
        gl_oldHardwareGamma[1][g] = g << 8;
        gl_oldHardwareGamma[2][g] = g << 8;
      }
    }

  }

  if (!gl_DeviceSupportsGamma)
  {
    lprintf(LO_WARN, "gld_CheckHardwareGamma: device has broken gamma support\n");
  }
}

//
// gld_SetGammaRamp
//
// This routine should only be called if gl_DeviceSupportsGamma is TRUE
//
int gld_SetGammaRamp(int gamma)
{
  int succeeded = false;
  static int first = true;
  float g = (BETWEEN(0, MAX_GLGAMMA, gamma)) / 10.0f + 1.0f;
  Uint16 gammatable[256];

  if (!gl_DeviceSupportsGamma)
    return false;

  if (gamma == -1)
  {
    succeeded = (SDL_SetGammaRamp(gl_oldHardwareGamma[0], gl_oldHardwareGamma[1], gl_oldHardwareGamma[2]) != -1);
  }
  else
  {
    if (first && desired_fullscreen)
    {
      // From GZDoom:
      //
      // Fix for Radeon 9000, possibly other R200s: When the device is
      // reset, it resets the gamma ramp, but the driver apparently keeps a
      // cached copy of the ramp that it doesn't update, so when
      // SetGammaRamp is called later to handle the NeedGammaUpdate flag,
      // it doesn't do anything, because the gamma ramp is the same as the
      // one passed in the last call, even though the visible gamma ramp 
      // actually has changed.
      //
      // So here we force the gamma ramp to something absolutely horrible and
      // trust that we will be able to properly set the gamma later
      first = false;
      memset(gammatable, 0, sizeof(gammatable));
      SDL_SetGammaRamp(NULL, NULL, gammatable);
    }

    CalculateGammaRamp(g, gammatable);

    succeeded = (SDL_SetGammaRamp(gammatable, gammatable, gammatable) != -1);
    if (!succeeded)
    {
      lprintf(LO_WARN, "gld_SetGammaRamp: hardware gamma adjustment is not supported\n");
      gl_lightmode = gl_lightmode_glboom;
    }
  }

  return succeeded;
}

// gld_ResetGammaRamp
// Restoring the gamma values to a linear value and exit
void gld_ResetGammaRamp(void)
{
  if (M_CheckParm("-resetgamma"))
  {
    if (gld_SetGammaRamp(1))
    {
      lprintf(LO_WARN, "gld_ResetGammaRamp: suspicious gamma tables, using linear ramp for restoration\n");
      _exit(0);
    }
  }
}

void gld_ApplyGammaRamp(byte *buf, int pitch, int width, int height)
{
  if (gl_hardware_gamma)
  {
    int w, h;
    byte *pixel;
    Uint16 r[256], g[256], b[256];

    SDL_GetGammaRamp(&r[0], &g[0], &b[0]);

    for (h = 0; h < height; h++)
    {
      for (w = 0; w < width; w++)
      {
        pixel = buf + h * pitch + 3 * w;

        *(pixel + 0) = (byte)(r[*(pixel + 0)] >> 8);
        *(pixel + 1) = (byte)(g[*(pixel + 1)] >> 8);
        *(pixel + 2) = (byte)(b[*(pixel + 2)] >> 8);
      }
    }
  }
}
