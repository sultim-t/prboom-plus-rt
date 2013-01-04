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

#ifndef _GL_OPENGL_H
#define _GL_OPENGL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define USE_VERTEX_ARRAYS
//#define USE_VBO

#include <SDL.h>
#include <SDL_opengl.h>

#if SDL_VERSION_ATLEAST(1, 3, 0)
#if defined(__MACOSX__)
#include <OpenGL/gl.h>	/* Header File For The OpenGL Library */
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#elif defined(__MACOS__)
#include <gl.h>		/* Header File For The OpenGL Library */
#include <glu.h>	/* Header File For The GLU Library */
#else
#include <GL/gl.h>	/* Header File For The OpenGL Library */
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif
#endif

#include "doomtype.h"

#if !defined(GL_DEPTH_STENCIL_EXT)
#define GL_DEPTH_STENCIL_EXT              0x84F9
#endif

#define isExtensionSupported(ext) strstr(extensions, ext)

//e6y: OpenGL version
typedef enum {
  OPENGL_VERSION_1_0,
  OPENGL_VERSION_1_1,
  OPENGL_VERSION_1_2,
  OPENGL_VERSION_1_3,
  OPENGL_VERSION_1_4,
  OPENGL_VERSION_1_5,
  OPENGL_VERSION_2_0,
  OPENGL_VERSION_2_1,
} glversion_t;

extern int gl_version;

extern int GLEXT_CLAMP_TO_EDGE;
extern int gl_max_texture_size;

extern SDL_PixelFormat RGBAFormat;

// obsolete?
extern int gl_use_paletted_texture;
extern int gl_use_shared_texture_palette;
extern int gl_paletted_texture;
extern int gl_shared_texture_palette;

extern dboolean gl_ext_texture_filter_anisotropic;
extern dboolean gl_arb_texture_non_power_of_two;
extern dboolean gl_arb_multitexture;
extern dboolean gl_arb_texture_compression;
extern dboolean gl_ext_framebuffer_object;
extern dboolean gl_ext_packed_depth_stencil;
extern dboolean gl_ext_blend_color;
extern dboolean gl_use_stencil;
extern dboolean gl_ext_arb_vertex_buffer_object;
extern dboolean gl_arb_pixel_buffer_object;
extern dboolean gl_arb_shader_objects;

// obsolete?
extern PFNGLCOLORTABLEEXTPROC              GLEXT_glColorTableEXT;

extern PFNGLBINDFRAMEBUFFEREXTPROC         GLEXT_glBindFramebufferEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC         GLEXT_glGenFramebuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC        GLEXT_glGenRenderbuffersEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC        GLEXT_glBindRenderbufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC     GLEXT_glRenderbufferStorageEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC GLEXT_glFramebufferRenderbufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    GLEXT_glFramebufferTexture2DEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  GLEXT_glCheckFramebufferStatusEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC      GLEXT_glDeleteFramebuffersEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC     GLEXT_glDeleteRenderbuffersEXT;

/* ARB_multitexture command function pointers */
extern PFNGLACTIVETEXTUREARBPROC           GLEXT_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC     GLEXT_glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC         GLEXT_glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD2FVARBPROC        GLEXT_glMultiTexCoord2fvARB;

extern PFNGLBLENDCOLOREXTPROC              GLEXT_glBlendColorEXT;

/* ARB_texture_compression */
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    GLEXT_glCompressedTexImage2DARB;

/* VBO */
extern PFNGLGENBUFFERSARBPROC              GLEXT_glGenBuffersARB;
extern PFNGLDELETEBUFFERSARBPROC           GLEXT_glDeleteBuffersARB;
extern PFNGLBINDBUFFERARBPROC              GLEXT_glBindBufferARB;
extern PFNGLBUFFERDATAARBPROC              GLEXT_glBufferDataARB;

/* PBO */
extern PFNGLBUFFERSUBDATAARBPROC           GLEXT_glBufferSubDataARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC    GLEXT_glGetBufferParameterivARB;
extern PFNGLMAPBUFFERARBPROC               GLEXT_glMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC             GLEXT_glUnmapBufferARB;

/* GL_ARB_shader_objects */
#ifdef USE_SHADERS
extern PFNGLDELETEOBJECTARBPROC            GLEXT_glDeleteObjectARB;
extern PFNGLGETHANDLEARBPROC               GLEXT_glGetHandleARB;
extern PFNGLDETACHOBJECTARBPROC            GLEXT_glDetachObjectARB;
extern PFNGLCREATESHADEROBJECTARBPROC      GLEXT_glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC            GLEXT_glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC           GLEXT_glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC     GLEXT_glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC            GLEXT_glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC             GLEXT_glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC        GLEXT_glUseProgramObjectARB;
extern PFNGLVALIDATEPROGRAMARBPROC         GLEXT_glValidateProgramARB;

extern PFNGLUNIFORM1FARBPROC               GLEXT_glUniform1fARB;
extern PFNGLUNIFORM2FARBPROC               GLEXT_glUniform2fARB;
extern PFNGLUNIFORM1IARBPROC               GLEXT_glUniform1iARB;

extern PFNGLGETOBJECTPARAMETERFVARBPROC     GLEXT_glGetObjectParameterfvARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC     GLEXT_glGetObjectParameterivARB;
extern PFNGLGETINFOLOGARBPROC               GLEXT_glGetInfoLogARB;
extern PFNGLGETATTACHEDOBJECTSARBPROC       GLEXT_glGetAttachedObjectsARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC       GLEXT_glGetUniformLocationARB;
extern PFNGLGETACTIVEUNIFORMARBPROC         GLEXT_glGetActiveUniformARB;
extern PFNGLGETUNIFORMFVARBPROC             GLEXT_glGetUniformfvARB;
#endif
  
void gld_InitOpenGL(dboolean compatibility_mode);

//states
void gld_EnableTexture2D(GLenum texture, int enable);
void gld_EnableClientCoordArray(GLenum texture, int enable);
void gld_EnableMultisample(int enable);

typedef enum
{
  TMF_MASKBIT = 1,
  TMF_OPAQUEBIT = 2,
  TMF_INVERTBIT = 4,

  TM_MODULATE = 0,
  TM_MASK = TMF_MASKBIT,
  TM_OPAQUE = TMF_OPAQUEBIT,
  TM_INVERT = TMF_INVERTBIT,
  //TM_INVERTMASK = TMF_MASKBIT | TMF_INVERTBIT
  TM_INVERTOPAQUE = TMF_INVERTBIT | TMF_OPAQUEBIT,
} tex_mode_e;
void SetTextureMode(tex_mode_e type);

#endif // _GL_OPENGL_H
