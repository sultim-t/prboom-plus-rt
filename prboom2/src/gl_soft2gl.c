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
 *  2D-in-3D video code
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gl_opengl.h"

#include "gl_intern.h"
#include "gl_struct.h"
#include "i_video.h"
#include "e6y.h"

void gld_Init8InGLMode(void)
{
  // clean up texture
  if (vid_8ingl.texid)
  {
    glDeleteTextures(1, &vid_8ingl.texid);
    vid_8ingl.texid = 0;
  }

  // clean up PBOs
  if (vid_8ingl.pboids[0])
  {
    GLEXT_glDeleteBuffersARB(2, vid_8ingl.pboids);
    memset(vid_8ingl.pboids, 0, sizeof(vid_8ingl.pboids));
  }
  
  // deallocate texture buffer
  if (vid_8ingl.buf)
  {
    free(vid_8ingl.buf);
    vid_8ingl.buf = NULL;
  }

  gld_InitOpenGL(gl_compatibility);
  gld_InitTextureParams();
  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glViewport(0, 0, REAL_SCREENWIDTH, REAL_SCREENHEIGHT);
  gld_Set2DMode();

  vid_8ingl.width = gld_GetTexDimension(REAL_SCREENWIDTH);
  vid_8ingl.height = gld_GetTexDimension(REAL_SCREENHEIGHT);
  vid_8ingl.size = vid_8ingl.width * vid_8ingl.height * 4;

  vid_8ingl.fU1 = 0.0f;
  vid_8ingl.fU2 = (float)REAL_SCREENWIDTH / (float)vid_8ingl.width;
  vid_8ingl.fV1 = 0.0f;
  vid_8ingl.fV2 = (float)REAL_SCREENHEIGHT / (float)vid_8ingl.height;

  glGenTextures(1, &vid_8ingl.texid);

  glBindTexture(GL_TEXTURE_2D, vid_8ingl.texid);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_format,
    vid_8ingl.width, vid_8ingl.height, 
    0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
  
  glBindTexture(GL_TEXTURE_2D, 0);

  // create 2 pixel buffer objects, you need to delete them when program exits.
  // glBufferDataARB with NULL pointer reserves only memory space.
  if (gl_arb_pixel_buffer_object)
  {
    GLEXT_glGenBuffersARB(2, vid_8ingl.pboids);
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.pboids[0]);
    GLEXT_glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.size, 0, GL_STREAM_DRAW_ARB);
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.pboids[1]);
    GLEXT_glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.size, 0, GL_STREAM_DRAW_ARB);
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }
  else
  {
    vid_8ingl.buf = malloc(vid_8ingl.size);
  }
}

void UpdatePixels(unsigned char* dst)
{
  int x, y;

  unsigned int *pal = (unsigned int*)(vid_8ingl.colours +
    256 * vid_8ingl.palette * 4);

  if (V_GetMode() == VID_MODE8)
  {
    for (y = 0; y < REAL_SCREENHEIGHT; y++)
    {
      byte *px = (((byte*)vid_8ingl.screen->pixels) + y * vid_8ingl.screen->pitch);
      int *py = ((int*)dst) + y * vid_8ingl.width;
      for (x = 0; x < REAL_SCREENWIDTH; x++)
      {
        *(int*)py = pal[*(byte*)px];
        px += 1;
        py += 1;
      }
    }
  } else if (V_GetMode() == VID_MODE15 || V_GetMode() == VID_MODE16)
  {
    unsigned int Rshift, Gshift, Bshift;
    unsigned int Rmask, Gmask, Bmask;
    SDL_PixelFormat *format = vid_8ingl.screen->format;

    Rmask = format->Rmask;
    Gmask = format->Gmask;
    Bmask = format->Bmask;
   
    Rshift = 16 - format->Rshift + format->Rloss;
    Gshift =  8 - format->Gshift + format->Gloss;
    Bshift =  0 - format->Bshift + format->Bloss;

    for (y = 0; y < REAL_SCREENHEIGHT; y++)
    {
      byte *px = (((byte*)vid_8ingl.screen->pixels) + y * vid_8ingl.screen->pitch);
      int *py = ((int*)dst) + y * vid_8ingl.width;
      for (x = 0; x < REAL_SCREENWIDTH; x++)
      {
        short color = *(short*)px;
        *(int*)py =
          ((color & Bmask) << Bshift) |
          ((color & Gmask) << Gshift) |
          ((color & Rmask) << Rshift);
        px += 2;
        py += 1;
      }
    }
  }
  else if (V_GetMode() == VID_MODE32)
  {
    for (y = 0; y < REAL_SCREENHEIGHT; y++)
    {
      byte *px = (((byte*)vid_8ingl.screen->pixels) + y * vid_8ingl.screen->pitch);
      int *py = ((int*)dst) + y * vid_8ingl.width;
      for (x = 0; x < REAL_SCREENWIDTH; x++)
      {
        *(int*)py = *(int*)px;
        px += 4;
        py += 1;
      }
    }
  }
}

void gld_Draw8InGL(void)
{
  if (gl_arb_pixel_buffer_object)
  {
    const int pboMode = 2;
    unsigned char *ptr;
    static int index = 0;
    int nextIndex = 0; // pbo index used for next frame

    // increment current index first then get the next index
    // "index" is used to copy pixels from a PBO to a texture object
    // "nextIndex" is used to update pixels in a PBO
    index = (index + 1) % 2;
    if (pboMode == 1)        // with 1 PBO
      nextIndex = index;
    else if (pboMode == 2)   // with 2 PBO
      nextIndex = (index + 1) % 2;

    // bind the texture and PBO
    glBindTexture(GL_TEXTURE_2D, vid_8ingl.texid);
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.pboids[index]);

    // copy pixels from PBO to texture object
    // Use offset instead of ponter.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
      vid_8ingl.width, vid_8ingl.height,
      GL_BGRA, GL_UNSIGNED_BYTE, 0);

    // bind PBO to update pixel values
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.pboids[nextIndex]);

    // map the buffer object into client's memory
    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // for GPU to finish its job. To avoid waiting (stall), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    GLEXT_glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, vid_8ingl.size, 0, GL_STREAM_DRAW_ARB);

    ptr = GLEXT_glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if (ptr)
    {
      // update data directly on the mapped buffer
      UpdatePixels(ptr);

      // release pointer to mapping buffer
      GLEXT_glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    }

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations behave normal ways.
    GLEXT_glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, vid_8ingl.texid);

    UpdatePixels(vid_8ingl.buf);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
      vid_8ingl.width, vid_8ingl.height,
      GL_BGRA, GL_UNSIGNED_BYTE, vid_8ingl.buf);
  }

  glBegin(GL_TRIANGLE_STRIP);
  {
    glTexCoord2f(vid_8ingl.fU1, vid_8ingl.fV1);
    glVertex2f(0.0f, 0.0f);
    
    glTexCoord2f(vid_8ingl.fU1, vid_8ingl.fV2);
    glVertex2f(0.0f, (float)REAL_SCREENHEIGHT);

    glTexCoord2f(vid_8ingl.fU2, vid_8ingl.fV1);
    glVertex2f((float)REAL_SCREENWIDTH, 0.0f);

    glTexCoord2f(vid_8ingl.fU2, vid_8ingl.fV2);
    glVertex2f((float)REAL_SCREENWIDTH, (float)REAL_SCREENHEIGHT);
  }
  glEnd();

  // unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);

  if (gl_finish)
  {
    glFinish();
  }
  SDL_GL_SwapBuffers();
}
