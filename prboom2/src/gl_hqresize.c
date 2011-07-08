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

/*
** gl_hqresize.cpp
** Contains high quality upsampling functions.
** So far Scale2x/3x/4x as described in http://scale2x.sourceforge.net/
** are implemented.
**
**---------------------------------------------------------------------------
** Copyright 2008 Benjamin Berkels
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>

#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"

int gl_texture_hqresize;
const char *gl_hqresizemodes[hq_scale_max] = {
  "Off", "Scale2x", "Scale3x", "Scale4x"};

int gl_texture_hqresize;
int gl_texture_hqresize_maxinputsize = 512;
int gl_texture_hqresize_textures;
int gl_texture_hqresize_sprites;
int gl_texture_hqresize_patches;

static void scale2x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight, int seamlessWidth, int seamlessHeight )
{
  int i, j;
  const int width = 2* inWidth;
  //const int height = 2 * inHeight;

  for ( i = 0; i < inWidth; ++i )
  {
    // [JB] when the current index is at an edge and seamlessWidth is true, 
    // the opposite edge's index will be used for iMinus and iPlus 
    // when the current index is at an edge and seamlessWidth is false,
    // the current index will be used for iMinus and iPlus
    // otherwise iMinus and iPlus are equal to i-1 and i+1 respectively
    const int iMinus = ((i == 0) ? (seamlessWidth ? inWidth - 1 : 0) : (i - 1));
    const int iPlus = ((i == inWidth - 1) ? (seamlessWidth ? 0 : i) : (i + 1));
    for ( j = 0; j < inHeight; ++j )
    {
      const int jMinus = ((j == 0) ? (seamlessHeight ? inHeight - 1 : 0) : (j - 1));
      const int jPlus = ((j == inHeight - 1) ? (seamlessHeight ? 0 : j) : (j + 1));
      //const unsigned int A = inputBuffer[ iMinus +inWidth*jMinus];
      const unsigned int B = inputBuffer[ iMinus +inWidth*j    ];
      //const unsigned int C = inputBuffer[ iMinus +inWidth*jPlus];
      const unsigned int D = inputBuffer[ i     +inWidth*jMinus];
      const unsigned int E = inputBuffer[ i     +inWidth*j    ];
      const unsigned int F = inputBuffer[ i     +inWidth*jPlus];
      //const unsigned int G = inputBuffer[ iPlus +inWidth*jMinus];
      const unsigned int H = inputBuffer[ iPlus +inWidth*j    ];
      //const unsigned int I = inputBuffer[ iPlus +inWidth*jPlus];
      if (B != H && D != F) {
        outputBuffer[2*i   + width*2*j    ] = D == B ? D : E;
        outputBuffer[2*i   + width*(2*j+1)] = B == F ? F : E;
        outputBuffer[2*i+1 + width*2*j    ] = D == H ? D : E;
        outputBuffer[2*i+1 + width*(2*j+1)] = H == F ? F : E;
      } else {
        outputBuffer[2*i   + width*2*j    ] = E;
        outputBuffer[2*i   + width*(2*j+1)] = E;
        outputBuffer[2*i+1 + width*2*j    ] = E;
        outputBuffer[2*i+1 + width*(2*j+1)] = E;
      }
    }
  }
}

static void scale3x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight, int seamlessWidth, int seamlessHeight )
{
  int i, j;
  const int width = 3* inWidth;
  //const int height = 3 * inHeight;

  for ( i = 0; i < inWidth; ++i )
  {
    const int iMinus = ((i == 0) ? (seamlessWidth ? inWidth - 1 : 0) : (i - 1));
    const int iPlus = ((i == inWidth - 1) ? (seamlessWidth ? 0 : i) : (i + 1));
    for ( j = 0; j < inHeight; ++j )
    {
      const int jMinus = ((j == 0) ? (seamlessHeight ? inHeight - 1 : 0) : (j - 1));
      const int jPlus = ((j == inHeight - 1) ? (seamlessHeight ? 0 : j) : (j + 1));
      const unsigned int A = inputBuffer[ iMinus +inWidth*jMinus];
      const unsigned int B = inputBuffer[ iMinus +inWidth*j    ];
      const unsigned int C = inputBuffer[ iMinus +inWidth*jPlus];
      const unsigned int D = inputBuffer[ i     +inWidth*jMinus];
      const unsigned int E = inputBuffer[ i     +inWidth*j    ];
      const unsigned int F = inputBuffer[ i     +inWidth*jPlus];
      const unsigned int G = inputBuffer[ iPlus +inWidth*jMinus];
      const unsigned int H = inputBuffer[ iPlus +inWidth*j    ];
      const unsigned int I = inputBuffer[ iPlus +inWidth*jPlus];
      if (B != H && D != F) {
        outputBuffer[3*i   + width*3*j    ] = D == B ? D : E;
        outputBuffer[3*i   + width*(3*j+1)] = (D == B && E != C) || (B == F && E != A) ? B : E;
        outputBuffer[3*i   + width*(3*j+2)] = B == F ? F : E;
        outputBuffer[3*i+1 + width*3*j    ] = (D == B && E != G) || (D == H && E != A) ? D : E;
        outputBuffer[3*i+1 + width*(3*j+1)] = E;
        outputBuffer[3*i+1 + width*(3*j+2)] = (B == F && E != I) || (H == F && E != C) ? F : E;
        outputBuffer[3*i+2 + width*3*j    ] = D == H ? D : E;
        outputBuffer[3*i+2 + width*(3*j+1)] = (D == H && E != I) || (H == F && E != G) ? H : E;
        outputBuffer[3*i+2 + width*(3*j+2)] = H == F ? F : E;
      } else {
        outputBuffer[3*i   + width*3*j    ] = E;
        outputBuffer[3*i   + width*(3*j+1)] = E;
        outputBuffer[3*i   + width*(3*j+2)] = E;
        outputBuffer[3*i+1 + width*3*j    ] = E;
        outputBuffer[3*i+1 + width*(3*j+1)] = E;
        outputBuffer[3*i+1 + width*(3*j+2)] = E;
        outputBuffer[3*i+2 + width*3*j    ] = E;
        outputBuffer[3*i+2 + width*(3*j+1)] = E;
        outputBuffer[3*i+2 + width*(3*j+2)] = E;
      }
    }
  }
}

static void scale4x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight, int seamlessWidth, int seamlessHeight )
{
  unsigned int * buffer2x = malloc((2 * inWidth) * (2 * inHeight) * sizeof(unsigned int));
  scale2x (inputBuffer, buffer2x, inWidth, inHeight, seamlessWidth, seamlessHeight);
  scale2x (buffer2x, outputBuffer, 2 * inWidth, 2 * inHeight, seamlessWidth, seamlessHeight);
  free(buffer2x);
}


static unsigned char *HQScaleHelper( void (*scaleNxFunction) ( unsigned int* , unsigned int* , int , int, int, int),
                                    const int N,
                                    unsigned char *inputBuffer,
                                    const int inWidth,
                                    const int inHeight,
                                    int *outWidth,
                                    int *outHeight,
                                    int seamlessWidth,
                                    int seamlessHeight )
{
  unsigned char * newBuffer;

  (*outWidth) = N * inWidth;
  (*outHeight) = N *inHeight;
  newBuffer = malloc((*outWidth) * (*outHeight) * 4 * sizeof(unsigned char));
                                                   
  scaleNxFunction ( (unsigned int*)inputBuffer, (unsigned int*)newBuffer, inWidth, inHeight, seamlessWidth, seamlessHeight );
  free(inputBuffer);
  inputBuffer = NULL;
  return newBuffer;
}

//===========================================================================
// 
// [BB] Upsamples the texture in inputBuffer, frees inputBuffer and returns
//  the upsampled buffer.
//
//===========================================================================
unsigned char* gld_HQResize(GLTexture *gltexture, unsigned char *inputBuffer, int inWidth, int inHeight, int *outWidth, int *outHeight)
{
  int w = inWidth;
  int h = inHeight;
  unsigned char *result = inputBuffer;
  gl_hqresizemode_t scale_mode = hq_scale_none;
  int sw = 0;
  int sh = 0;

  if (outWidth) *outWidth = inWidth;
  if (outHeight) *outHeight = inHeight;

  if (!gl_texture_hqresize || !gltexture || !inputBuffer)
    return result;

  // [BB] Don't resample if the width or height of the input texture is bigger than gl_texture_hqresize_maxinputsize.
  if ((inWidth > gl_texture_hqresize_maxinputsize) ||
      (inHeight > gl_texture_hqresize_maxinputsize))
    return result;

  // [BB] The hqnx upsampling (not the scaleN one) destroys partial transparency, don't upsamle textures using it.
  //if (gltexture->bIsTransparent == 1)
  //  return inputBuffer;

  switch (gltexture->textype)
  {
  case GLDT_PATCH:
    sw = sh = 0;
    if (gltexture->flags & GLTEXTURE_SPRITE)
      scale_mode = gl_texture_hqresize_sprites;
    else
      scale_mode = gl_texture_hqresize_patches;
    break;

  case GLDT_FLAT:
    sw = sh = 1;
    scale_mode = gl_texture_hqresize_textures;
    break;
    
  case GLDT_TEXTURE:
    //sw = gltexture->flags & GLTEXTURE_SKY;
    //sh = 0;
    sw = sh = 1;
    scale_mode = gl_texture_hqresize_textures;
    break;
  }

  switch (scale_mode)
  {
  case hq_scale_2x:
    result = HQScaleHelper(&scale2x, 2, inputBuffer, inWidth, inHeight, &w, &h, sw, sh);
    break;
  case hq_scale_3x:
    result = HQScaleHelper(&scale3x, 3, inputBuffer, inWidth, inHeight, &w, &h, sw, sh);
    break;
  case hq_scale_4x:
    result = HQScaleHelper(&scale4x, 4, inputBuffer, inWidth, inHeight, &w, &h, sw, sh);
    break;
  }

  if (result != inputBuffer)
  {
    gltexture->tex_width = w;
    gltexture->tex_height = h;

    if (outWidth) *outWidth = w;
    if (outHeight) *outHeight = h;
  }

  return result;
}
