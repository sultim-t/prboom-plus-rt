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
 *   Thanks Roman "Vortex" Marchenko
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include "gl_opengl.h"

#include "doomtype.h"
#include "lprintf.h"

#define isExtensionSupported(ext) strstr(extensions, ext)

int gl_version;

int GLEXT_CLAMP_TO_EDGE = GL_CLAMP;
int gl_max_texture_size = 0;

// obsolete?
int gl_use_paletted_texture = 0;
int gl_use_shared_texture_palette = 0;
int gl_paletted_texture = 0;
int gl_shared_texture_palette = 0;

dboolean gl_ext_texture_filter_anisotropic = false;
dboolean gl_arb_texture_non_power_of_two = false;
dboolean gl_arb_multitexture = false;
dboolean gl_arb_texture_compression = false;
dboolean gl_ext_framebuffer_object = false;
dboolean gl_ext_packed_depth_stencil = false;
dboolean gl_ext_blend_color = false;
dboolean gl_use_stencil = false;
dboolean gl_ext_arb_vertex_buffer_object = false;

// cfg values
int gl_ext_texture_filter_anisotropic_default;
int gl_arb_texture_non_power_of_two_default;
int gl_arb_multitexture_default;
int gl_arb_texture_compression_default;
int gl_ext_framebuffer_object_default;
int gl_ext_packed_depth_stencil_default;
int gl_ext_blend_color_default;
int gl_use_stencil_default;
int gl_ext_arb_vertex_buffer_object_default;

int active_texture_enabled[32];
int clieant_active_texture_enabled[32];

// obsolete?
PFNGLCOLORTABLEEXTPROC              GLEXT_glColorTableEXT              = NULL;

/* EXT_framebuffer_object */
PFNGLBINDFRAMEBUFFEREXTPROC         GLEXT_glBindFramebufferEXT         = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC         GLEXT_glGenFramebuffersEXT         = NULL;
PFNGLGENRENDERBUFFERSEXTPROC        GLEXT_glGenRenderbuffersEXT        = NULL;
PFNGLBINDRENDERBUFFEREXTPROC        GLEXT_glBindRenderbufferEXT        = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC     GLEXT_glRenderbufferStorageEXT     = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC GLEXT_glFramebufferRenderbufferEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    GLEXT_glFramebufferTexture2DEXT    = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  GLEXT_glCheckFramebufferStatusEXT  = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC      GLEXT_glDeleteFramebuffersEXT      = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC     GLEXT_glDeleteRenderbuffersEXT     = NULL;

/* ARB_multitexture command function pointers */
PFNGLACTIVETEXTUREARBPROC        GLEXT_glActiveTextureARB              = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC  GLEXT_glClientActiveTextureARB        = NULL;
PFNGLMULTITEXCOORD2FARBPROC      GLEXT_glMultiTexCoord2fARB            = NULL;
PFNGLMULTITEXCOORD2FVARBPROC     GLEXT_glMultiTexCoord2fvARB           = NULL;

/* ARB_texture_compression */
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC GLEXT_glCompressedTexImage2DARB       = NULL;

PFNGLBLENDCOLOREXTPROC              GLEXT_glBlendColorEXT              = NULL;

/* VBO */
PFNGLGENBUFFERSARBPROC              GLEXT_glGenBuffersARB              = NULL;
PFNGLDELETEBUFFERSARBPROC           GLEXT_glDeleteBuffersARB           = NULL;
PFNGLBINDBUFFERARBPROC              GLEXT_glBindBufferARB              = NULL;
PFNGLBUFFERDATAARBPROC              GLEXT_glBufferDataARB              = NULL;

void gld_InitOpenGLVersion(void)
{
  int MajorVersion, MinorVersion;
  gl_version = OPENGL_VERSION_1_0;
  if (sscanf((const char*)glGetString(GL_VERSION), "%d.%d", &MajorVersion, &MinorVersion) == 2)
  {
    if (MajorVersion > 1)
    {
      gl_version = OPENGL_VERSION_2_0;
      if (MinorVersion > 0) gl_version = OPENGL_VERSION_2_1;
    }
    else
    {
      gl_version = OPENGL_VERSION_1_0;
      if (MinorVersion > 0) gl_version = OPENGL_VERSION_1_1;
      if (MinorVersion > 1) gl_version = OPENGL_VERSION_1_2;
      if (MinorVersion > 2) gl_version = OPENGL_VERSION_1_3;
      if (MinorVersion > 3) gl_version = OPENGL_VERSION_1_4;
      if (MinorVersion > 4) gl_version = OPENGL_VERSION_1_5;
    }
  }
}

void gld_InitOpenGL(dboolean compatibility_mode)
{
  GLenum texture;
  const char *extensions = (const char*)glGetString(GL_EXTENSIONS);

  gld_InitOpenGLVersion();

  gl_ext_texture_filter_anisotropic = gl_ext_texture_filter_anisotropic_default &&
    isExtensionSupported("GL_EXT_texture_filter_anisotropic") != NULL;
  if (gl_ext_texture_filter_anisotropic)
    lprintf(LO_INFO, "using GL_EXT_texture_filter_anisotropic\n");

  // Any textures sizes are allowed
  gl_arb_texture_non_power_of_two = gl_arb_texture_non_power_of_two_default &&
    isExtensionSupported("GL_ARB_texture_non_power_of_two") != NULL;
  if (gl_arb_texture_non_power_of_two)
    lprintf(LO_INFO, "using GL_ARB_texture_non_power_of_two\n");

  // Paletted textures
  if (isExtensionSupported("GL_EXT_paletted_texture") != NULL)
  {
    if (gl_use_paletted_texture)
    {
      gl_paletted_texture = true;
      GLEXT_glColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
      if (GLEXT_glColorTableEXT == NULL)
        gl_paletted_texture = false;
      else
        lprintf(LO_INFO,"using GL_EXT_paletted_texture\n");
    }
  }
  else if (isExtensionSupported("GL_EXT_shared_texture_palette") != NULL)
  {
    if (gl_use_shared_texture_palette)
    {
      gl_shared_texture_palette = true;
      GLEXT_glColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
      if (GLEXT_glColorTableEXT == NULL)
        gl_shared_texture_palette = false;
      else
        lprintf(LO_INFO,"using GL_EXT_shared_texture_palette\n");
    }
  }

  //
  // ARB_multitexture command function pointers
  //

  gl_arb_multitexture = gl_arb_multitexture_default &&
    isExtensionSupported("GL_ARB_multitexture") != NULL;
  if (gl_arb_multitexture)
  {
    GLEXT_glActiveTextureARB        = SDL_GL_GetProcAddress("glActiveTextureARB");
    GLEXT_glClientActiveTextureARB  = SDL_GL_GetProcAddress("glClientActiveTextureARB");
    GLEXT_glMultiTexCoord2fARB      = SDL_GL_GetProcAddress("glMultiTexCoord2fARB");
    GLEXT_glMultiTexCoord2fvARB     = SDL_GL_GetProcAddress("glMultiTexCoord2fvARB");

    if (!GLEXT_glActiveTextureARB   || !GLEXT_glClientActiveTextureARB ||
        !GLEXT_glMultiTexCoord2fARB || !GLEXT_glMultiTexCoord2fvARB)
      gl_arb_multitexture = false;
  }
  if (gl_arb_multitexture)
    lprintf(LO_INFO,"using GL_ARB_multitexture\n");

  //
  // ARB_texture_compression
  //

  gl_arb_texture_compression = gl_arb_texture_compression_default &&
    isExtensionSupported("GL_ARB_texture_compression") != NULL;
  if (gl_arb_texture_compression)
  {
    GLEXT_glCompressedTexImage2DARB = SDL_GL_GetProcAddress("glCompressedTexImage2DARB");

    if (!GLEXT_glCompressedTexImage2DARB)
      gl_arb_texture_compression = false;
  }
  if (gl_arb_texture_compression)
    lprintf(LO_INFO,"using GL_ARB_texture_compression\n");

  //
  // EXT_framebuffer_object
  //
  gl_ext_framebuffer_object = gl_ext_framebuffer_object_default &&
    isExtensionSupported("GL_EXT_framebuffer_object") != NULL;
  if (gl_ext_framebuffer_object)
  {
    GLEXT_glGenFramebuffersEXT         = SDL_GL_GetProcAddress("glGenFramebuffersEXT");
    GLEXT_glBindFramebufferEXT         = SDL_GL_GetProcAddress("glBindFramebufferEXT");
    GLEXT_glGenRenderbuffersEXT        = SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
    GLEXT_glBindRenderbufferEXT        = SDL_GL_GetProcAddress("glBindRenderbufferEXT");
    GLEXT_glRenderbufferStorageEXT     = SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
    GLEXT_glFramebufferRenderbufferEXT = SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
    GLEXT_glFramebufferTexture2DEXT    = SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
    GLEXT_glCheckFramebufferStatusEXT  = SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
    GLEXT_glDeleteFramebuffersEXT      = SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
    GLEXT_glDeleteRenderbuffersEXT     = SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");

    if (!GLEXT_glGenFramebuffersEXT || !GLEXT_glBindFramebufferEXT ||
        !GLEXT_glGenRenderbuffersEXT || !GLEXT_glBindRenderbufferEXT ||
        !GLEXT_glRenderbufferStorageEXT || !GLEXT_glFramebufferRenderbufferEXT ||
        !GLEXT_glFramebufferTexture2DEXT || !GLEXT_glCheckFramebufferStatusEXT ||
        !GLEXT_glDeleteFramebuffersEXT || !GLEXT_glDeleteRenderbuffersEXT)
      gl_ext_framebuffer_object = false;
  }
  if (gl_ext_framebuffer_object)
    lprintf(LO_INFO,"using GL_EXT_framebuffer_object\n");

  gl_ext_packed_depth_stencil = gl_ext_packed_depth_stencil_default &&
    isExtensionSupported("GL_EXT_packed_depth_stencil") != NULL;
  if (gl_ext_packed_depth_stencil)
    lprintf(LO_INFO,"using GL_EXT_packed_depth_stencil\n");

  //
  // Blending
  //

  gl_ext_blend_color = gl_ext_blend_color_default &&
    isExtensionSupported("GL_EXT_blend_color") != NULL;
  if (gl_ext_blend_color)
  {
    GLEXT_glBlendColorEXT = SDL_GL_GetProcAddress("glBlendColorEXT");

    if (!GLEXT_glBlendColorEXT)
      gl_ext_blend_color = false;
  }
  if (gl_ext_blend_color)
    lprintf(LO_INFO,"using GL_EXT_blend_color\n");

  // VBO
#ifdef USE_VBO
  gl_ext_arb_vertex_buffer_object = gl_ext_arb_vertex_buffer_object_default &&
    isExtensionSupported("GL_ARB_vertex_buffer_object") != NULL;
  if (gl_ext_arb_vertex_buffer_object)
  {
    GLEXT_glGenBuffersARB = SDL_GL_GetProcAddress("glGenBuffersARB");
    GLEXT_glDeleteBuffersARB = SDL_GL_GetProcAddress("glDeleteBuffersARB");
    GLEXT_glBindBufferARB = SDL_GL_GetProcAddress("glBindBufferARB");
    GLEXT_glBufferDataARB = SDL_GL_GetProcAddress("glBufferDataARB");

    if (!GLEXT_glGenBuffersARB || !GLEXT_glDeleteBuffersARB ||
        !GLEXT_glBindBufferARB || !GLEXT_glBufferDataARB)
      gl_ext_arb_vertex_buffer_object = false;
  }
  if (gl_ext_arb_vertex_buffer_object)
    lprintf(LO_INFO,"using GL_ARB_vertex_buffer_object\n");
#else
  gl_ext_arb_vertex_buffer_object = false;
#endif

  //
  // Stencil support
  //

  gl_use_stencil = gl_use_stencil_default;

  // GL_CLAMP_TO_EDGE
  GLEXT_CLAMP_TO_EDGE = (gl_version >= OPENGL_VERSION_1_2 ? GL_CLAMP_TO_EDGE : GL_CLAMP);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
  lprintf(LO_INFO,"GL_MAX_TEXTURE_SIZE=%i\n", gl_max_texture_size);

  // Additional checks
  if (gl_version < OPENGL_VERSION_1_3)
  {
    gl_ext_framebuffer_object = false;
    gl_ext_blend_color = false;
  }

  if ((compatibility_mode) || (gl_version <= OPENGL_VERSION_1_1))
  {
    lprintf(LO_INFO, "gld_InitOpenGL: Compatibility mode is used.\n");
    gl_arb_texture_non_power_of_two = false;
    gl_arb_multitexture = false;
    gl_arb_texture_compression = false;
    gl_ext_framebuffer_object = false;
    gl_ext_packed_depth_stencil = false;
    gl_ext_blend_color = false;
    gl_use_stencil = false;
    gl_ext_arb_vertex_buffer_object = false;
    GLEXT_CLAMP_TO_EDGE = GL_CLAMP;
    gl_version = OPENGL_VERSION_1_1;
  }

  //init states manager
  gld_EnableMultisample(true);
  gld_EnableMultisample(false);

  for (texture = GL_TEXTURE0_ARB; texture <= GL_TEXTURE31_ARB; texture++)
  {
    gld_EnableTexture2D(texture, true);
    gld_EnableTexture2D(texture, false);

    gld_EnableClientCoordArray(texture, true);
    gld_EnableClientCoordArray(texture, false);
  }
}

void gld_EnableTexture2D(GLenum texture, int enable)
{
  int arb;

  if (!gl_arb_multitexture && texture != GL_TEXTURE0_ARB)
    return;

  arb = texture - GL_TEXTURE0_ARB;

#ifdef RANGECHECK
  if (arb < 0 || arb > 31)
    I_Error("gld_EnableTexture2D: wronge ARB texture unit %d", arb);
#endif

  if (enable)
  {
    if (!active_texture_enabled[arb])
    {
      if (arb != 0)
      {
        GLEXT_glActiveTextureARB(texture);
        glEnable(GL_TEXTURE_2D);
        GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
      }
      else
      {
        glEnable(GL_TEXTURE_2D);
      }
      active_texture_enabled[arb] = enable;
    }
  }
  else
  {
    if (active_texture_enabled[arb])
    {
      if (arb != 0)
      {
        GLEXT_glActiveTextureARB(texture);
        glDisable(GL_TEXTURE_2D);
        GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
      }
      else
      {
        glDisable(GL_TEXTURE_2D);
      }
      active_texture_enabled[arb] = enable;
    }
  }
}

void gld_EnableClientCoordArray(GLenum texture, int enable)
{
#ifdef USE_VERTEX_ARRAYS
  int arb;

  if (!gl_arb_multitexture)
    return;

  arb = texture - GL_TEXTURE0_ARB;

#ifdef RANGECHECK
  if (arb < 0 || arb > 31)
    I_Error("gld_EnableTexture2D: wronge ARB texture unit %d", arb);
#endif

  if (enable)
  {
    if (!clieant_active_texture_enabled[arb])
    {
      GLEXT_glClientActiveTextureARB(texture);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      GLEXT_glClientActiveTextureARB(GL_TEXTURE0_ARB);

      clieant_active_texture_enabled[arb] = enable;
    }
  }
  else
  {
    if (clieant_active_texture_enabled[arb])
    {
      GLEXT_glClientActiveTextureARB(texture);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      GLEXT_glClientActiveTextureARB(GL_TEXTURE0_ARB);

      clieant_active_texture_enabled[arb] = enable;
    }
  }
#endif
}

void gld_EnableMultisample(int enable)
{
  static int multisample_is_enabled = 0;
  if (enable)
  {
    if (!multisample_is_enabled)
    {
      glEnable(GL_MULTISAMPLE_ARB);

      multisample_is_enabled = enable;
    }
  }
  else
  {
    if (multisample_is_enabled)
    {
      glDisable(GL_MULTISAMPLE_ARB);

      multisample_is_enabled = enable;
    }
  }
}
