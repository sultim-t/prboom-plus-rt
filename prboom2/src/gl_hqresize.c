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
#include <SDL_opengl.h>

#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"

int gl_texture_hqresize;
const char *gl_hqresizemodes[] = {"Off","Scale2x", "Scale3x","Scale4x"};

int gl_texture_hqresize;
int gl_texture_hqresize_maxinputsize = 512;
int gl_texture_hqresize_textures;
int gl_texture_hqresize_sprites;
int gl_texture_hqresize_patches;

static void scale2x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight )
{
  int i, j;
  const int width = 2* inWidth;
  const int height = 2 * inHeight;

  for ( i = 0; i < inWidth; ++i )
  {
    const int iMinus = (i > 0) ? (i-1) : 0;
    const int iPlus = (i < inWidth - 1 ) ? (i+1) : i;
    for ( j = 0; j < inHeight; ++j )
    {
      const int jMinus = (j > 0) ? (j-1) : 0;
      const int jPlus = (j < inHeight - 1 ) ? (j+1) : j;
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

static void scale3x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight )
{
  int i, j;
  const int width = 3* inWidth;
  const int height = 3 * inHeight;

  for ( i = 0; i < inWidth; ++i )
  {
    const int iMinus = (i > 0) ? (i-1) : 0;
    const int iPlus = (i < inWidth - 1 ) ? (i+1) : i;
    for ( j = 0; j < inHeight; ++j )
    {
      const int jMinus = (j > 0) ? (j-1) : 0;
      const int jPlus = (j < inHeight - 1 ) ? (j+1) : j;
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

static void scale4x ( unsigned int* inputBuffer, unsigned int* outputBuffer, int inWidth, int inHeight )
{
  int width = 2* inWidth;
  int height = 2 * inHeight;
  unsigned int * buffer2x = malloc(width*height * sizeof(unsigned int));

  scale2x ( inputBuffer, buffer2x, inWidth, inHeight );
  width *= 2;
  height *= 2;
  scale2x ( buffer2x, outputBuffer, 2*inWidth, 2*inHeight );
  free(buffer2x);
}


static unsigned char *scaleNxHelper( void (*scaleNxFunction) ( unsigned int* , unsigned int* , int , int),
                                    const int N,
                                    unsigned char *inputBuffer,
                                    const int inWidth,
                                    const int inHeight,
                                    int *outWidth,
                                    int *outHeight )
{
  unsigned char * newBuffer;

  (*outWidth) = N * inWidth;
  (*outHeight) = N *inHeight;
  newBuffer = malloc((*outWidth) * (*outHeight) * 4 * sizeof(unsigned char));
                                                   
  scaleNxFunction ( (unsigned int*)inputBuffer, (unsigned int*)newBuffer, inWidth, inHeight );
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
  unsigned char *result = inputBuffer;

  *outWidth = inWidth;
  *outHeight = inHeight;

  // [BB] Don't resample if the width or height of the input texture is bigger than gl_texture_hqresize_maxinputsize.
  if ( ( inWidth > gl_texture_hqresize_maxinputsize ) || ( inHeight > gl_texture_hqresize_maxinputsize ) )
    return result;

  // [BB] The hqnx upsampling (not the scaleN one) destroys partial transparency, don't upsamle textures using it.
  //if ( gltexture->bIsTransparent == 1 )
//    return inputBuffer;

  switch (gltexture->textype)
  {
  case GLDT_PATCH:
    if (gltexture->flags & GLTEXTURE_SPRITE)
    {
      if (!gl_texture_hqresize_sprites)
        return result;
    }
    else
    {
      if (!gl_texture_hqresize_patches)
        return result;
    }
    break;

  case GLDT_TEXTURE:
  case GLDT_FLAT:
    if (!gl_texture_hqresize_textures)
      return result;
    break;

  default:
    return result;
    break;
  }

  if (inputBuffer)
  {
    int type = gl_texture_hqresize;
    *outWidth = inWidth;
    *outHeight = inHeight;
    switch (type)
    {
    case 1:
      result = scaleNxHelper( &scale2x, 2, inputBuffer, inWidth, inHeight, outWidth, outHeight );
      break;
    case 2:
      result = scaleNxHelper( &scale3x, 3, inputBuffer, inWidth, inHeight, outWidth, outHeight );
      break;
    case 3:
      result = scaleNxHelper( &scale4x, 4, inputBuffer, inWidth, inHeight, outWidth, outHeight );
      break;
    }
  }

  if (inWidth != *outWidth)
  {
    gltexture->tex_width = *outWidth;
    gltexture->tex_height = *outHeight;

    //gltexture->buffer_width = outWidth;
    //gltexture->buffer_height = outHeight;
    //gltexture->buffer_size = outWidth * outHeight * 4;
  }

  return result;
}
