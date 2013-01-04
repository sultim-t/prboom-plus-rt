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
#include "gl_opengl.h"

#include "doomtype.h"
#include "lprintf.h"

#define isExtensionSupported(ext) strstr(extensions, ext)

int gl_version;

static dboolean gl_compatibility_mode;

int GLEXT_CLAMP_TO_EDGE = GL_CLAMP;
int gl_max_texture_size = 0;

SDL_PixelFormat RGBAFormat;

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
dboolean gl_arb_pixel_buffer_object = false;
dboolean gl_arb_shader_objects = false;

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
int gl_arb_pixel_buffer_object_default;
int gl_arb_shader_objects_default;

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

/* PBO */
PFNGLBUFFERSUBDATAARBPROC           GLEXT_glBufferSubDataARB           = NULL;
PFNGLGETBUFFERPARAMETERIVARBPROC    GLEXT_glGetBufferParameterivARB    = NULL;
PFNGLMAPBUFFERARBPROC               GLEXT_glMapBufferARB               = NULL;
PFNGLUNMAPBUFFERARBPROC             GLEXT_glUnmapBufferARB             = NULL;

/* GL_ARB_shader_objects */
#ifdef USE_SHADERS
PFNGLDELETEOBJECTARBPROC        GLEXT_glDeleteObjectARB = NULL;
PFNGLGETHANDLEARBPROC           GLEXT_glGetHandleARB = NULL;
PFNGLDETACHOBJECTARBPROC        GLEXT_glDetachObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC  GLEXT_glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC        GLEXT_glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC       GLEXT_glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC GLEXT_glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC        GLEXT_glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC         GLEXT_glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC    GLEXT_glUseProgramObjectARB = NULL;
PFNGLVALIDATEPROGRAMARBPROC     GLEXT_glValidateProgramARB = NULL;

PFNGLUNIFORM1FARBPROC           GLEXT_glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC           GLEXT_glUniform2fARB = NULL;
PFNGLUNIFORM1IARBPROC           GLEXT_glUniform1iARB = NULL;

PFNGLGETOBJECTPARAMETERFVARBPROC GLEXT_glGetObjectParameterfvARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC GLEXT_glGetObjectParameterivARB = NULL;
PFNGLGETINFOLOGARBPROC           GLEXT_glGetInfoLogARB = NULL;
PFNGLGETATTACHEDOBJECTSARBPROC   GLEXT_glGetAttachedObjectsARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC   GLEXT_glGetUniformLocationARB = NULL;
PFNGLGETACTIVEUNIFORMARBPROC     GLEXT_glGetActiveUniformARB = NULL;
PFNGLGETUNIFORMFVARBPROC         GLEXT_glGetUniformfvARB = NULL;
#endif

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

  gl_compatibility_mode = compatibility_mode;

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

  gl_arb_pixel_buffer_object = gl_arb_pixel_buffer_object_default &&
    isExtensionSupported("GL_ARB_pixel_buffer_object") != NULL;
  if (gl_arb_pixel_buffer_object)
  {
    GLEXT_glGenBuffersARB = SDL_GL_GetProcAddress("glGenBuffersARB");
    GLEXT_glBindBufferARB = SDL_GL_GetProcAddress("glBindBufferARB");
    GLEXT_glBufferDataARB = SDL_GL_GetProcAddress("glBufferDataARB");
    GLEXT_glBufferSubDataARB = SDL_GL_GetProcAddress("glBufferSubDataARB");
    GLEXT_glDeleteBuffersARB = SDL_GL_GetProcAddress("glDeleteBuffersARB");
    GLEXT_glGetBufferParameterivARB = SDL_GL_GetProcAddress("glGetBufferParameterivARB");
    GLEXT_glMapBufferARB = SDL_GL_GetProcAddress("glMapBufferARB");
    GLEXT_glUnmapBufferARB = SDL_GL_GetProcAddress("glUnmapBufferARB");

    if (!GLEXT_glGenBuffersARB || !GLEXT_glBindBufferARB ||
        !GLEXT_glBufferDataARB || !GLEXT_glBufferSubDataARB ||
        !GLEXT_glDeleteBuffersARB || !GLEXT_glGetBufferParameterivARB ||
        !GLEXT_glMapBufferARB || !GLEXT_glUnmapBufferARB)
      gl_arb_pixel_buffer_object = false;
  }
  if (gl_arb_pixel_buffer_object)
    lprintf(LO_INFO,"using GL_ARB_pixel_buffer_object\n");

  //
  // Stencil support
  //

  gl_use_stencil = gl_use_stencil_default;

  //
  // GL_ARB_shader_objects
  //
#ifdef USE_SHADERS
  gl_arb_shader_objects = gl_arb_shader_objects_default &&
    (gl_version >= OPENGL_VERSION_2_0) &&
    isExtensionSupported ("GL_ARB_shader_objects") &&
    isExtensionSupported ("GL_ARB_vertex_shader") &&
    isExtensionSupported ("GL_ARB_fragment_shader") &&
    isExtensionSupported ("GL_ARB_shading_language_100");
  if (gl_arb_shader_objects)
  {
		GLEXT_glDeleteObjectARB        = SDL_GL_GetProcAddress("glDeleteObjectARB");
		GLEXT_glGetHandleARB           = SDL_GL_GetProcAddress("glGetHandleARB");
		GLEXT_glDetachObjectARB        = SDL_GL_GetProcAddress("glDetachObjectARB");
		GLEXT_glCreateShaderObjectARB  = SDL_GL_GetProcAddress("glCreateShaderObjectARB");
		GLEXT_glShaderSourceARB        = SDL_GL_GetProcAddress("glShaderSourceARB");
		GLEXT_glCompileShaderARB       = SDL_GL_GetProcAddress("glCompileShaderARB");
		GLEXT_glCreateProgramObjectARB = SDL_GL_GetProcAddress("glCreateProgramObjectARB");
		GLEXT_glAttachObjectARB        = SDL_GL_GetProcAddress("glAttachObjectARB");
		GLEXT_glLinkProgramARB         = SDL_GL_GetProcAddress("glLinkProgramARB");
		GLEXT_glUseProgramObjectARB    = SDL_GL_GetProcAddress("glUseProgramObjectARB");
		GLEXT_glValidateProgramARB     = SDL_GL_GetProcAddress("glValidateProgramARB");

		GLEXT_glUniform1fARB = SDL_GL_GetProcAddress("glUniform1fARB");
		GLEXT_glUniform2fARB = SDL_GL_GetProcAddress("glUniform2fARB");
		GLEXT_glUniform1iARB = SDL_GL_GetProcAddress("glUniform1iARB");

		GLEXT_glGetObjectParameterfvARB = SDL_GL_GetProcAddress("glGetObjectParameterfvARB");
		GLEXT_glGetObjectParameterivARB = SDL_GL_GetProcAddress("glGetObjectParameterivARB");
		GLEXT_glGetInfoLogARB           = SDL_GL_GetProcAddress("glGetInfoLogARB");
		GLEXT_glGetAttachedObjectsARB   = SDL_GL_GetProcAddress("glGetAttachedObjectsARB");
		GLEXT_glGetUniformLocationARB   = SDL_GL_GetProcAddress("glGetUniformLocationARB");
		GLEXT_glGetActiveUniformARB     = SDL_GL_GetProcAddress("glGetActiveUniformARB");
		GLEXT_glGetUniformfvARB         = SDL_GL_GetProcAddress("glGetUniformfvARB");

    if (!GLEXT_glDeleteObjectARB || !GLEXT_glGetHandleARB ||
        !GLEXT_glDetachObjectARB || !GLEXT_glCreateShaderObjectARB ||
        !GLEXT_glShaderSourceARB || !GLEXT_glCompileShaderARB ||
        !GLEXT_glCreateProgramObjectARB || !GLEXT_glAttachObjectARB ||
        !GLEXT_glLinkProgramARB || !GLEXT_glUseProgramObjectARB ||
        !GLEXT_glValidateProgramARB ||
        !GLEXT_glUniform1fARB || !GLEXT_glUniform2fARB ||
        !GLEXT_glUniform1iARB ||
        !GLEXT_glGetObjectParameterfvARB || !GLEXT_glGetObjectParameterivARB ||
        !GLEXT_glGetInfoLogARB || !GLEXT_glGetAttachedObjectsARB ||
        !GLEXT_glGetUniformLocationARB || !GLEXT_glGetActiveUniformARB ||
        !GLEXT_glGetUniformfvARB)
      gl_arb_shader_objects = false;
  }
  if (gl_arb_shader_objects)
  {
    lprintf(LO_INFO,"using GL_ARB_shader_objects\n");
    lprintf(LO_INFO,"using GL_ARB_vertex_shader\n");
    lprintf(LO_INFO,"using GL_ARB_fragment_shader\n");
    lprintf(LO_INFO,"using GL_ARB_shading_language_100\n");
  }
#else
  gl_arb_shader_objects = false;
#endif

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
    gl_arb_pixel_buffer_object = false;
    gl_arb_shader_objects = false;
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

  //init global variables
  RGBAFormat.palette = 0;
#if !SDL_VERSION_ATLEAST(1, 3, 0)
  RGBAFormat.colorkey = 0;
  RGBAFormat.alpha = 0;
#endif
  RGBAFormat.BitsPerPixel = 32;
  RGBAFormat.BytesPerPixel = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  RGBAFormat.Rmask = 0xFF000000; RGBAFormat.Rshift = 0; RGBAFormat.Rloss = 0;
  RGBAFormat.Gmask = 0x00FF0000; RGBAFormat.Gshift = 8; RGBAFormat.Gloss = 0;
  RGBAFormat.Bmask = 0x0000FF00; RGBAFormat.Bshift = 16; RGBAFormat.Bloss = 0;
  RGBAFormat.Amask = 0x000000FF; RGBAFormat.Ashift = 24; RGBAFormat.Aloss = 0;
#else
  RGBAFormat.Rmask = 0x000000FF; RGBAFormat.Rshift = 24; RGBAFormat.Rloss = 0;
  RGBAFormat.Gmask = 0x0000FF00; RGBAFormat.Gshift = 16; RGBAFormat.Gloss = 0;
  RGBAFormat.Bmask = 0x00FF0000; RGBAFormat.Bshift = 8; RGBAFormat.Bloss = 0;
  RGBAFormat.Amask = 0xFF000000; RGBAFormat.Ashift = 0; RGBAFormat.Aloss = 0;
#endif
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

void SetTextureMode(tex_mode_e type)
{
  if (gl_compatibility_mode)
  {
    type = TM_MODULATE;
  }

  if (type == TM_MASK)
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE); 
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
  }
  else if (type == TM_OPAQUE)
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE); 
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  }
  else if (type == TM_INVERT)
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE); 
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
  }
  else if (type == TM_INVERTOPAQUE)
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE); 
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  }
  else // if (type == TM_MODULATE)
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }
}
