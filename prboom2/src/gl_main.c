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
 *  Copyright 2007 by
 *  Andrey Budko, Roman Marchenko
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

#include "z_zone.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "doomtype.h"
#include "w_wad.h"
#include "m_argv.h"
#include "d_event.h"
#include "v_video.h"
#include "doomstat.h"
#include "r_bsp.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_data.h"
#include "r_things.h"
#include "r_fps.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "lprintf.h"
#include "gl_intern.h"
#include "gl_struct.h"
#include "p_spec.h"
#include "i_system.h"
#include "m_argv.h"
#include "i_video.h"
#include "i_main.h"
#include "e6y.h"//e6y

int gl_preprocessed = false;

//e6y: all OpenGL extentions will be disabled with TRUE
int gl_compatibility = 0;

// Vortex: Frame buffer object related
#ifdef USE_FBO_TECHNIQUE
GLint glSceneImageFBOTexID = 0;
GLuint glDepthBufferFBOTexID = 0;
GLuint glSceneImageTextureFBOTexID = 0;
boolean gld_CreateScreenSizeFBO(void);
void gld_FreeScreenSizeFBO(void);
void gld_InitMotionBlur(void);
#endif

unsigned int invul_method;
float bw_red = 0.3f;
float bw_green = 0.59f;
float bw_blue = 0.11f;

//e6y
int SceneInTexture = false;

//e6y: motion bloor
int gl_motionblur;
int gl_use_motionblur = false;
char *gl_motionblur_minspeed;
char *gl_motionblur_att_a;
char *gl_motionblur_att_b;
char *gl_motionblur_att_c;
int MotionBlurOn;
int gl_motionblur_minspeed_pow2 = 0x32 * 0x32 + 0x28 * 0x28;
float gl_motionblur_a = 55.0f;
float gl_motionblur_b = 1.8f;
float gl_motionblur_c = 0.9f;

//e6y
int gl_invul_bw_method;
static boolean SkyDrawed;

gl_lightmode_t gl_lightmode;
const char *gl_lightmodes[] = {"glboom", "gzdoom", "mixed"};
int gl_light_ambient;

boolean gl_arb_texture_non_power_of_two = false;
boolean gl_arb_multitexture = false;
boolean gl_arb_texture_compression = false;
boolean gl_ext_framebuffer_object = false;
boolean gl_ext_blend_color = false;

boolean gl_use_stencil = false;
boolean gl_use_FBO = false;

/* ARB_multitexture command function pointers */
PFNGLACTIVETEXTUREARBPROC        GLEXT_glActiveTextureARB        = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC  GLEXT_glClientActiveTextureARB  = NULL;
PFNGLMULTITEXCOORD2FARBPROC      GLEXT_glMultiTexCoord2fARB      = NULL;
PFNGLMULTITEXCOORD2FVARBPROC     GLEXT_glMultiTexCoord2fvARB     = NULL;

/* EXT_framebuffer_object */
#ifdef USE_FBO_TECHNIQUE
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
#endif

PFNGLBLENDCOLOREXTPROC              GLEXT_glBlendColorEXT = NULL;

/* ARB_texture_compression */
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC GLEXT_glCompressedTexImage2DARB = NULL;

int imageformats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

extern int tran_filter_pct;

#define USE_VERTEX_ARRAYS

boolean use_fog=false;

int gl_vsync = true;

int gl_nearclip=5;
int gl_texture_filter;
int gl_tex_filter;
int gl_mipmap_filter;
int gl_sortsprites=true;
int gl_texture_filter_anisotropic = 0;
int gl_use_texture_filter_anisotropic = 0;
int gl_use_paletted_texture = 0;
int gl_use_shared_texture_palette = 0;
int gl_paletted_texture = 0;
int gl_shared_texture_palette = 0;
//e6y: moved to globals
//int gl_sprite_offset;	// item out of floor offset Mead 8/13/03

GLuint gld_DisplayList=0;
int fog_density=200;
static float extra_red=0.0f;
static float extra_green=0.0f;
static float extra_blue=0.0f;
static float extra_alpha=0.0f;

GLfloat gl_whitecolor[4]={1.0f,1.0f,1.0f,1.0f};

static void gld_InitExtensions(const char *_extensions)
{
  char *extensions;
  char *extension;
  char *p;

  if (!_extensions)
    return;

  extensions = malloc(strlen(_extensions) + 1);
  if (!extensions)
    return;
  memcpy(extensions, _extensions, strlen(_extensions) + 1);

  p = extensions;
  extension = p;

  do {
    while ((*p != ' ') && (*p != '\0'))
      p++;
    if (*p != '\0')
      *p++ = '\0';
    while (*p == ' ')
      p++;

    if (strcasecmp(extension, "GL_EXT_texture_filter_anisotropic") == 0)
      gl_use_texture_filter_anisotropic = true;
    else if (strcasecmp(extension, "GL_EXT_paletted_texture") == 0) {
      if (gl_use_paletted_texture) {
        gl_paletted_texture = true;
        gld_ColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
	if (gld_ColorTableEXT == NULL)
	  gl_paletted_texture = false;
	else
          lprintf(LO_INFO,"using GL_EXT_paletted_texture\n");
      }
    }
    else if (strcasecmp(extension, "GL_EXT_shared_texture_palette") == 0)
      if (gl_use_shared_texture_palette) {
        gl_shared_texture_palette = true;
        gld_ColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
	if (gld_ColorTableEXT == NULL)
	  gl_shared_texture_palette = false;
	else
          lprintf(LO_INFO,"using GL_EXT_shared_texture_palette\n");
      }

    extension = p;
  } while (*extension != '\0');

  free(extensions);
}

void gld_InitExtensionsEx(void)
{
#define isExtensionSupported(ext) strstr(extensions, ext)

  extern int gl_tex_filter;
  extern int gl_mipmap_filter;
  extern int gl_tex_format;

  const GLubyte *extensions = glGetString(GL_EXTENSIONS);

  if (gl_compatibility)
  {
    lprintf(LO_INFO, "gld_InitExtensionsEx: Compatibility mode is used.\n");
    gl_arb_texture_non_power_of_two = false;
    gl_arb_multitexture = false;
    gl_arb_texture_compression = false;
    gl_ext_framebuffer_object = false;
    gl_ext_blend_color = false;
    gl_use_stencil = false;
    glversion = OPENGL_VERSION_1_1;
    return;
  }

  gl_arb_texture_non_power_of_two = isExtensionSupported("GL_ARB_texture_non_power_of_two") != NULL;
  if (gl_arb_texture_non_power_of_two)
    lprintf(LO_INFO,"using GL_ARB_texture_non_power_of_two\n");

  gl_arb_multitexture = isExtensionSupported("GL_ARB_multitexture") != NULL;

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

  gld_InitDetail();

  gl_arb_texture_compression = isExtensionSupported("GL_ARB_texture_compression") != NULL;

  if (gl_arb_texture_compression)
  {
    GLEXT_glCompressedTexImage2DARB = SDL_GL_GetProcAddress("glCompressedTexImage2DARB");

    if (!GLEXT_glCompressedTexImage2DARB)
      gl_arb_texture_compression = false;
  }

  if (gl_arb_texture_compression)
    lprintf(LO_INFO,"using GL_ARB_texture_compression\n");

#ifdef USE_FBO_TECHNIQUE
  gl_ext_framebuffer_object = isExtensionSupported("GL_EXT_framebuffer_object") != NULL;

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
#endif

  gl_ext_blend_color = isExtensionSupported("GL_EXT_blend_color") != NULL;

  if (gl_ext_blend_color)
  {
    GLEXT_glBlendColorEXT = SDL_GL_GetProcAddress("glBlendColorEXT");

    if (!GLEXT_glBlendColorEXT)
      gl_ext_blend_color = false;
  }

  if (gl_ext_blend_color)
    lprintf(LO_INFO,"using GL_EXT_blend_color\n");

  if (glversion < OPENGL_VERSION_1_3)
  {
    gl_ext_framebuffer_object = false;
    gl_ext_blend_color = false;
  }

  gl_use_stencil = true;
}

//e6y
void gld_InitGLVersion(void)
{
  int MajorVersion, MinorVersion;
  glversion = OPENGL_VERSION_1_0;
  if (sscanf(glGetString(GL_VERSION), "%d.%d", &MajorVersion, &MinorVersion) == 2)
  {
    if (MajorVersion > 1)
    {
      glversion = OPENGL_VERSION_2_0;
      if (MinorVersion > 0) glversion = OPENGL_VERSION_2_1;
    }
    else
    {
      glversion = OPENGL_VERSION_1_0;
      if (MinorVersion > 0) glversion = OPENGL_VERSION_1_1;
      if (MinorVersion > 1) glversion = OPENGL_VERSION_1_2;
      if (MinorVersion > 2) glversion = OPENGL_VERSION_1_3;
      if (MinorVersion > 3) glversion = OPENGL_VERSION_1_4;
      if (MinorVersion > 4) glversion = OPENGL_VERSION_1_5;
    }
  }
}

void gld_InitTextureParams(void)
{
  typedef struct tex_filter_s
  {
    boolean mipmap;
    int tex_filter;
    int mipmap_filter;
    char *tex_filter_name;
    char *mipmap_filter_name;
  } tex_filter_t;

  tex_filter_t params[filter_count] = {
    {false, GL_NEAREST, GL_NEAREST,                "GL_NEAREST", "GL_NEAREST"},
    {true,  GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, "GL_NEAREST", "GL_NEAREST_MIPMAP_NEAREST"},
    {true,  GL_LINEAR,  GL_LINEAR,                 "GL_LINEAR",  "GL_LINEAR"},
    {true,  GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,  "GL_LINEAR",  "GL_LINEAR_MIPMAP_NEAREST"},
    {true,  GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR,   "GL_LINEAR",  "GL_LINEAR_MIPMAP_LINEAR"},
  };

  use_mipmapping   = params[gl_texture_filter].mipmap;
  gl_tex_filter    = params[gl_texture_filter].tex_filter;
  gl_mipmap_filter = params[gl_texture_filter].mipmap_filter;
  lprintf(LO_INFO, "Using %s for normal textures.\n", params[gl_texture_filter].tex_filter_name);
  lprintf(LO_INFO, "Using %s for mipmap textures.\n", params[gl_texture_filter].mipmap_filter_name);

  if (use_mipmapping)
  {
    gl_shared_texture_palette = false;
  }

#ifndef USE_GLU_MIPMAP
  use_mipmapping = false;
#endif

  if (gl_color_mip_levels)
  {
    gl_tex_format=GL_RGBA;
  }
  else
  if (!strcasecmp(gl_tex_format_string,"GL_RGBA8"))
  {
    gl_tex_format=GL_RGBA8;
    lprintf(LO_INFO,"Using texture format GL_RGBA8.\n");
  }
  else
  if (!strcasecmp(gl_tex_format_string,"GL_RGB5_A1"))
  {
    gl_tex_format=GL_RGB5_A1;
    lprintf(LO_INFO,"Using texture format GL_RGB5_A1.\n");
  }
  else
  if (!strcasecmp(gl_tex_format_string,"GL_RGBA4"))
  {
    gl_tex_format=GL_RGBA4;
    lprintf(LO_INFO,"Using texture format GL_RGBA4.\n");
  }
  else
  if (!strcasecmp(gl_tex_format_string,"GL_RGBA2"))
  {
    gl_tex_format=GL_RGBA2;
    lprintf(LO_INFO,"Using texture format GL_RGBA2.\n");
  }
  else
  {
    gl_tex_format=GL_RGBA;
    lprintf(LO_INFO,"Using texture format GL_RGBA.\n");
  }
}

void gld_Init(int width, int height)
{
  GLfloat params[4]={0.0f,0.0f,1.0f,0.0f};

  lprintf(LO_INFO,"GL_VENDOR: %s\n",glGetString(GL_VENDOR));
  lprintf(LO_INFO,"GL_RENDERER: %s\n",glGetString(GL_RENDERER));
  lprintf(LO_INFO,"GL_VERSION: %s\n",glGetString(GL_VERSION));
  lprintf(LO_INFO,"GL_EXTENSIONS:\n");
  {
    char ext_name[256];
    const char *extensions = glGetString(GL_EXTENSIONS);
    const char *rover = extensions;
    const char *p = rover;

    while (*rover)
    {
      p = rover;
      while (*p && *p != ' ')
        p++;
      if (*p)
      {
        int len = MIN(p-rover, sizeof(ext_name)-1);
        memset(ext_name, 0, sizeof(ext_name));
        strncpy(ext_name, rover, len);
        lprintf(LO_INFO,"\t%s\n", ext_name);
      }
      rover = p;
      while (*rover && *rover == ' ')
        rover++;
    }
  }

  gld_InitExtensions(glGetString(GL_EXTENSIONS));
  //gl_shared_texture_palette = false;
  gld_InitPalettedTextures();

  glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);

  glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
  glClearDepth(1.0f);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gld_max_texturesize);
  //gld_max_texturesize=16;
  lprintf(LO_INFO,"GL_MAX_TEXTURE_SIZE=%i\n",gld_max_texturesize);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // proff_dis
//  glShadeModel(GL_FLAT);
  glEnable(GL_TEXTURE_2D);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL,0.5f);
  glDisable(GL_CULL_FACE);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

  glTexGenfv(GL_Q,GL_EYE_PLANE,params);
  glTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);

  //e6y
  gld_InitGLVersion();
  gld_InitExtensionsEx();
  gld_InitTextureParams();
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  gld_Finish();
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.5f, 0.5f, 1.0f);

  // e6y
  // if you have a prior crash in the game,
  // you can restore the gamma values to at least a linear value
  // with -resetgamma command-line switch
  gld_ResetGammaRamp();

  gld_InitLightTable();
  M_ChangeLightMode();
  M_ChangeAllowFog();

#ifdef HAVE_LIBSDL_IMAGE
  gld_InitHiRes();
#endif

  // Vortex: Create FBO object and associated render targets
#ifdef USE_FBO_TECHNIQUE
  gld_InitFBO();
  atexit(gld_FreeScreenSizeFBO);
#endif

  //e6y: is can be slow
  //atexit(gld_CleanMemory); //e6y
}

void gld_InitCommandLine(void)
{
}

void gld_DrawTriangleStrip(GLWall *wall, gl_strip_coords_t *c)
{
  glBegin(GL_TRIANGLE_STRIP);
  
  glTexCoord2fv((const GLfloat*)&c->t[0]);
  glVertex3fv((const GLfloat*)&c->v[0]);

  glTexCoord2fv((const GLfloat*)&c->t[1]);
  glVertex3fv((const GLfloat*)&c->v[1]);

  glTexCoord2fv((const GLfloat*)&c->t[2]);
  glVertex3fv((const GLfloat*)&c->v[2]);

  glTexCoord2fv((const GLfloat*)&c->t[3]);
  glVertex3fv((const GLfloat*)&c->v[3]);

  glEnd();
}

#define SCALE_X(x)    ((flags & VPT_STRETCH)?((float)x)*(float)SCREENWIDTH/320.0f:(float)x)
#define SCALE_Y(y)    ((flags & VPT_STRETCH)?((float)y)*(float)SCREENHEIGHT/200.0f:(float)y)

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

  //e6y
  boolean bFakeColormap;
  static float cm2RGB[CR_LIMIT + 1][4] = {
    {0.50f ,0.00f, 0.00f, 1.00f}, //CR_BRICK
    {1.00f ,1.00f, 1.00f, 1.00f}, //CR_TAN
    {1.00f ,1.00f, 1.00f, 1.00f}, //CR_GRAY
    {0.00f ,1.00f, 0.00f, 1.00f}, //CR_GREEN
    {0.50f ,0.20f, 1.00f, 1.00f}, //CR_BROWN
    {1.00f ,1.00f, 0.00f, 1.00f}, //CR_GOLD
    {1.00f ,0.00f, 0.00f, 1.00f}, //CR_RED
    {0.80f ,0.80f, 1.00f, 1.00f}, //CR_BLUE
    {1.00f ,0.50f, 0.25f, 1.00f}, //CR_ORANGE
    {1.00f ,1.00f, 0.00f, 1.00f}, //CR_YELLOW
    {0.50f ,0.50f, 1.00f, 1.00f}, //CR_BLUE2
    {1.00f ,1.00f, 1.00f, 1.00f}, //CR_LIMIT
  };

  if (flags & VPT_TRANS)
  {
    gltexture=gld_RegisterPatch(lump,cm);
    gld_BindPatch(gltexture, cm);
  }
  else
  {
    gltexture=gld_RegisterPatch(lump,CR_DEFAULT);
    gld_BindPatch(gltexture, CR_DEFAULT);
  }
  if (!gltexture)
    return;
  fV1=0.0f;
  fV2=gltexture->scaleyfac;
  if (flags & VPT_FLIP)
  {
    fU1=gltexture->scalexfac;
    fU2=0.0f;
  }
  else
  {
    fU1=0.0f;
    fU2=gltexture->scalexfac;
  }
  xpos=SCALE_X(x-gltexture->leftoffset);
  ypos=SCALE_Y(y-gltexture->topoffset);
  width=SCALE_X(gltexture->realtexwidth);
  height=SCALE_Y(gltexture->realtexheight);

  bFakeColormap =
    (gltexture->flags & GLTEXTURE_HIRES) && 
    (lumpinfo[lump].flags & LUMP_CM2RGB);
  if (bFakeColormap)
  {
    if (!(flags & VPT_TRANS) && (cm != CR_GRAY))
    {
      cm = CR_RED;
    }
    glColor3f(cm2RGB[cm][0], cm2RGB[cm][1], cm2RGB[cm][2]);//, cm2RGB[cm][3]);
  }
  else
  {
    // e6y
    // This is a workaround for some on-board Intel video cards.
    // Do you know more elegant solution?
    glColor3f(1.0f, 1.0f, 1.0f);
  }

  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
    glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
    glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
    glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
  glEnd();
  
  if (bFakeColormap)
  {
    glColor3f(1.0f,1.0f,1.0f);
  }
}

#undef SCALE_X
#undef SCALE_Y

void gld_DrawBackground(const char* name)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int width,height;

  //e6y: Boom colormap should not be applied for background
  int saved_boom_cm = boom_cm;
  boom_cm = 0;

  gltexture=gld_RegisterFlat(R_FlatNumForName(name), false);
  gld_BindFlat(gltexture);

  //e6y
  boom_cm = saved_boom_cm;

  if (!gltexture)
    return;
  fU1=0;
  fV1=0;
  fU2=(float)SCREENWIDTH/(float)gltexture->realtexwidth;
  fV2=(float)SCREENHEIGHT/(float)gltexture->realtexheight;
  width=SCREENWIDTH;
  height=SCREENHEIGHT;
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((float)(0),(float)(0));
    glTexCoord2f(fU1, fV2); glVertex2f((float)(0),(float)(0+height));
    glTexCoord2f(fU2, fV1); glVertex2f((float)(0+width),(float)(0));
    glTexCoord2f(fU2, fV2); glVertex2f((float)(0+width),(float)(0+height));
  glEnd();
}

void gld_DrawLine(int x0, int y0, int x1, int y1, int BaseColor)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

  glBindTexture(GL_TEXTURE_2D, 0);
  last_gltexture = NULL;
  last_cm = -1;
  glColor3f((float)playpal[3*BaseColor]/255.0f,
            (float)playpal[3*BaseColor+1]/255.0f,
            (float)playpal[3*BaseColor+2]/255.0f);
  glBegin(GL_LINES);
    glVertex2i( x0, y0 );
    glVertex2i( x1, y1 );
  glEnd();
  W_UnlockLumpName("PLAYPAL");
}

void gld_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int x1,y1,x2,y2;
  float light;

  gltexture=gld_RegisterPatch(firstspritelump+weaponlump, CR_DEFAULT);
  if (!gltexture)
    return;
  gld_BindPatch(gltexture, CR_DEFAULT);
  fU1=0;
  fV1=0;
  fU2=gltexture->scalexfac;
  fV2=gltexture->scaleyfac;
  // e6y
  // More precise weapon drawing:
  // Shotgun from DSV3_War looks correctly now. Especially during movement.
  // There is no more line of graphics under certain weapons.
  x1=viewwindowx+vis->x1;
  x2=x1+(int)((float)gltexture->realtexwidth*pspritexscale_f);
  y1=viewwindowy+centery-(int)(((float)vis->texturemid/(float)FRACUNIT)*pspriteyscale_f);
  y2=y1+(int)((float)gltexture->realtexheight*pspriteyscale_f)+1;
  light=gld_CalcLightLevel(lightlevel);

  // e6y
  // Fix of no warning (flashes between shadowed and solid)
  // when invisibility is about to go
  if (/*(viewplayer->mo->flags & MF_SHADOW) && */!vis->colormap)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.1f);
    //glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
    glColor4f(0.2f,0.2f,0.2f,0.33f);
  }
  else
  {
    if (viewplayer->mo->flags & MF_TRANSLUCENT)
      gld_StaticLightAlpha(light,(float)tran_filter_pct/100.0f);
    else
      gld_StaticLight(light);
  }
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((float)(x1),(float)(y1));
    glTexCoord2f(fU1, fV2); glVertex2f((float)(x1),(float)(y2));
    glTexCoord2f(fU2, fV1); glVertex2f((float)(x2),(float)(y1));
    glTexCoord2f(fU2, fV2); glVertex2f((float)(x2),(float)(y2));
  glEnd();
  if(!vis->colormap)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.5f);
  }
  glColor3f(1.0f,1.0f,1.0f);
}

void gld_FillBlock(int x, int y, int width, int height, int col)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

  glBindTexture(GL_TEXTURE_2D, 0);
  last_gltexture = NULL;
  last_cm = -1;
  glColor3f((float)playpal[3*col]/255.0f,
            (float)playpal[3*col+1]/255.0f,
            (float)playpal[3*col+2]/255.0f);
  glBegin(GL_TRIANGLE_STRIP);
    glVertex2i( x, y );
    glVertex2i( x, y+height );
    glVertex2i( x+width, y );
    glVertex2i( x+width, y+height );
  glEnd();
  glColor3f(1.0f,1.0f,1.0f);
  W_UnlockLumpName("PLAYPAL");
}

void gld_SetPalette(int palette)
{
  static int last_palette = 0;
  extra_red=0.0f;
  extra_green=0.0f;
  extra_blue=0.0f;
  extra_alpha=0.0f;
  if (palette < 0)
    palette = last_palette;
  last_palette = palette;
  if (gl_shared_texture_palette) {
    const unsigned char *playpal;
    unsigned char pal[1024];
    int i;

    playpal = W_CacheLumpName("PLAYPAL");
    playpal += (768*palette);
    for (i=0; i<256; i++) {
      int col;

      if (fixedcolormap)
        col = fixedcolormap[i];
      else if (fullcolormap)
        col = fullcolormap[i];
      else
        col = i;
      pal[i*4+0] = playpal[col*3+0];
      pal[i*4+1] = playpal[col*3+1];
      pal[i*4+2] = playpal[col*3+2];
      pal[i*4+3] = 255;
    }
    pal[transparent_pal_index*4+0]=0;
    pal[transparent_pal_index*4+1]=0;
    pal[transparent_pal_index*4+2]=0;
    pal[transparent_pal_index*4+3]=0;
    gld_ColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, pal);
    W_UnlockLumpName("PLAYPAL");
  } else {
    if (palette>0)
    {
      if (palette<=8)
      {
        extra_red=(float)palette/2.0f;
        extra_green=0.0f;
        extra_blue=0.0f;
        extra_alpha=(float)palette/10.0f;
      }
      else
        if (palette<=12)
        {
          palette=palette-8;
          extra_red=(float)palette*1.0f;
          extra_green=(float)palette*0.8f;
          extra_blue=(float)palette*0.1f;
          extra_alpha=(float)palette/11.0f;
        }
        else
          if (palette==13)
          {
            extra_red=0.4f;
            extra_green=1.0f;
            extra_blue=0.0f;
            extra_alpha=0.2f;
          }
    }
    if (extra_red>1.0f)
      extra_red=1.0f;
    if (extra_green>1.0f)
      extra_green=1.0f;
    if (extra_blue>1.0f)
      extra_blue=1.0f;
    if (extra_alpha>1.0f)
      extra_alpha=1.0f;
  }
}

void gld_ReadScreen (byte* scr)
{
  glReadPixels(0,0,SCREENWIDTH,SCREENHEIGHT,GL_RGB,GL_UNSIGNED_BYTE,scr);
}

GLvoid gld_Set2DMode(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(
    (GLdouble) 0,
    (GLdouble) SCREENWIDTH,
    (GLdouble) SCREENHEIGHT,
    (GLdouble) 0,
    (GLdouble) -1.0,
    (GLdouble) 1.0
  );
  glDisable(GL_DEPTH_TEST);
}

void gld_InitDrawScene(void)
{
}

void gld_Finish(void)
{
  gld_Set2DMode();
  glFinish();
  SDL_GL_SwapBuffers();
}

int gld_max_vertexes=0;
int gld_num_vertexes=0;
GLVertex *gld_vertexes=NULL;
GLTexcoord *gld_texcoords=NULL;

static void gld_AddGlobalVertexes(int count)
{
  if ((gld_num_vertexes+count)>=gld_max_vertexes)
  {
    gld_max_vertexes+=count+1024;
    gld_vertexes=Z_Realloc(gld_vertexes,gld_max_vertexes*sizeof(GLVertex),PU_STATIC,0);
    gld_texcoords=Z_Realloc(gld_texcoords,gld_max_vertexes*sizeof(GLTexcoord),PU_STATIC,0);
  }
}

GLSeg *gl_segs=NULL;

// e6y
// New memory manager for GL data.
// It is more universal and much easier in use.
GLDrawInfo gld_drawinfo;
static void gld_FreeDrawInfo(void);
static void gld_ResetDrawInfo(void);
static void gld_AddDrawRange(int size);
static void gld_AddDrawItem(GLDrawItemType itemtype, void *itemdata);

// this is the list for all sectors to the loops
GLSector *sectorloops;

byte rendermarker=0;
static byte *sectorrendered; // true if sector rendered (only here for malloc)
static byte *segrendered; // true if sector rendered (only here for malloc)

static FILE *levelinfo;

/*****************************
 *
 * FLATS
 *
 *****************************/

/* proff - 05/15/2000
 * The idea and algorithm to compute the flats with nodes and subsectors is
 * originaly from JHexen. I have redone it.
 */

#define FIX2DBL(x)    ((double)(x))
#define MAX_CC_SIDES  64

static boolean gld_PointOnSide(vertex_t *p, divline_t *d)
{
  // We'll return false if the point c is on the left side.
  return ((FIX2DBL(d->y)-FIX2DBL(p->y))*FIX2DBL(d->dx)-(FIX2DBL(d->x)-FIX2DBL(p->x))*FIX2DBL(d->dy) >= 0);
}

// Lines start-end and fdiv must intersect.
static void gld_CalcIntersectionVertex(vertex_t *s, vertex_t *e, divline_t *d, vertex_t *i)
{
  double ax = FIX2DBL(s->x), ay = FIX2DBL(s->y), bx = FIX2DBL(e->x), by = FIX2DBL(e->y);
  double cx = FIX2DBL(d->x), cy = FIX2DBL(d->y), dx = cx+FIX2DBL(d->dx), dy = cy+FIX2DBL(d->dy);
  double r = ((ay-cy)*(dx-cx)-(ax-cx)*(dy-cy)) / ((bx-ax)*(dy-cy)-(by-ay)*(dx-cx));
  i->x = (fixed_t)((double)s->x + r*((double)e->x-(double)s->x));
  i->y = (fixed_t)((double)s->y + r*((double)e->y-(double)s->y));
}

#undef FIX2DBL

// Returns a pointer to the list of points. It must be used.
//
static vertex_t *gld_FlatEdgeClipper(int *numpoints, vertex_t *points, int numclippers, divline_t *clippers)
{
  unsigned char sidelist[MAX_CC_SIDES];
  int       i, k, num = *numpoints;

  // We'll clip the polygon with each of the divlines. The left side of
  // each divline is discarded.
  for(i=0; i<numclippers; i++)
  {
    divline_t *curclip = &clippers[i];

    // First we'll determine the side of each vertex. Points are allowed
    // to be on the line.
    for(k=0; k<num; k++)
      sidelist[k] = gld_PointOnSide(&points[k], curclip);

    for(k=0; k<num; k++)
    {
      int startIdx = k, endIdx = k+1;
      // Check the end index.
      if(endIdx == num) endIdx = 0; // Wrap-around.
      // Clipping will happen when the ends are on different sides.
      if(sidelist[startIdx] != sidelist[endIdx])
      {
        vertex_t newvert;

        gld_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

        // Add the new vertex. Also modify the sidelist.
        points = (vertex_t*)Z_Realloc(points,(++num)*sizeof(vertex_t),PU_LEVEL,0);
        if(num >= MAX_CC_SIDES)
          I_Error("gld_FlatEdgeClipper: Too many points in carver");

        // Make room for the new vertex.
        memmove(&points[endIdx+1], &points[endIdx],
          (num - endIdx-1)*sizeof(vertex_t));
        memcpy(&points[endIdx], &newvert, sizeof(newvert));

        memmove(&sidelist[endIdx+1], &sidelist[endIdx], num-endIdx-1);
        sidelist[endIdx] = 1;

        // Skip over the new vertex.
        k++;
      }
    }

    // Now we must discard the points that are on the wrong side.
    for(k=0; k<num; k++)
      if(!sidelist[k])
      {
        memmove(&points[k], &points[k+1], (num - k-1)*sizeof(vertex_t));
        memmove(&sidelist[k], &sidelist[k+1], num - k-1);
        num--;
        k--;
      }
  }
  // Screen out consecutive identical points.
  for(i=0; i<num; i++)
  {
    int previdx = i-1;
    if(previdx < 0) previdx = num - 1;
    if(points[i].x == points[previdx].x
      && points[i].y == points[previdx].y)
    {
      // This point (i) must be removed.
      memmove(&points[i], &points[i+1], sizeof(vertex_t)*(num-i-1));
      num--;
      i--;
    }
  }
  *numpoints = num;
  return points;
}

static void gld_FlatConvexCarver(int ssidx, int num, divline_t *list)
{
  subsector_t *ssec=&subsectors[ssidx];
  int numclippers = num+ssec->numlines;
  divline_t *clippers;
  int i, numedgepoints;
  vertex_t *edgepoints;

  clippers=(divline_t*)Z_Malloc(numclippers*sizeof(divline_t),PU_LEVEL,0);
  if (!clippers)
    return;
  for(i=0; i<num; i++)
  {
    clippers[i].x = list[num-i-1].x;
    clippers[i].y = list[num-i-1].y;
    clippers[i].dx = list[num-i-1].dx;
    clippers[i].dy = list[num-i-1].dy;
  }
  for(i=num; i<numclippers; i++)
  {
    seg_t *seg = &segs[ssec->firstline+i-num];
    clippers[i].x = seg->v1->x;
    clippers[i].y = seg->v1->y;
    clippers[i].dx = seg->v2->x-seg->v1->x;
    clippers[i].dy = seg->v2->y-seg->v1->y;
  }

  // Setup the 'worldwide' polygon.
  numedgepoints = 4;
  edgepoints = (vertex_t*)Z_Malloc(numedgepoints*sizeof(vertex_t),PU_LEVEL,0);

  edgepoints[0].x = INT_MIN;
  edgepoints[0].y = INT_MAX;

  edgepoints[1].x = INT_MAX;
  edgepoints[1].y = INT_MAX;

  edgepoints[2].x = INT_MAX;
  edgepoints[2].y = INT_MIN;

  edgepoints[3].x = INT_MIN;
  edgepoints[3].y = INT_MIN;

  // Do some clipping, <snip> <snip>
  edgepoints = gld_FlatEdgeClipper(&numedgepoints, edgepoints, numclippers, clippers);

  if(!numedgepoints)
  {
    if (levelinfo) fprintf(levelinfo, "All carved away: subsector %i - sector %i\n", ssec-subsectors, ssec->sector->iSectorID);
  }
  else
  {
    if(numedgepoints >= 3)
    {
      gld_AddGlobalVertexes(numedgepoints);
      if ((gld_vertexes) && (gld_texcoords))
      {
        int currentsector=ssec->sector->iSectorID;

        sectorloops[ currentsector ].loopcount++;
        sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_STATIC, 0);
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=GL_TRIANGLE_FAN;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=numedgepoints;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexindex=gld_num_vertexes;

        for(i = 0;  i < numedgepoints; i++)
        {
          gld_texcoords[gld_num_vertexes].u = ( (float)edgepoints[i].x/(float)FRACUNIT)/64.0f;
          gld_texcoords[gld_num_vertexes].v = (-(float)edgepoints[i].y/(float)FRACUNIT)/64.0f;
          gld_vertexes[gld_num_vertexes].x = -(float)edgepoints[i].x/MAP_SCALE;
          gld_vertexes[gld_num_vertexes].y = 0.0f;
          gld_vertexes[gld_num_vertexes].z =  (float)edgepoints[i].y/MAP_SCALE;
          gld_num_vertexes++;
        }
      }
    }
  }
  // We're done, free the edgepoints memory.
  Z_Free(edgepoints);
  Z_Free(clippers);
}

static void gld_CarveFlats(int bspnode, int numdivlines, divline_t *divlines)
{
  node_t    *nod;
  divline_t *childlist, *dl;
  int     childlistsize = numdivlines+1;

  // If this is a subsector we are dealing with, begin carving with the
  // given list.
  if(bspnode & NF_SUBSECTOR)
  {
    // We have arrived at a subsector. The divline list contains all
    // the partition lines that carve out the subsector.
    // special case for trivial maps (no nodes, single subsector)
    int ssidx = (numnodes != 0) ? bspnode & (~NF_SUBSECTOR) : 0;

    if (!(subsectors[ssidx].sector->flags & SECTOR_IS_CLOSED))
      gld_FlatConvexCarver(ssidx, numdivlines, divlines);
    return;
  }

  // Get a pointer to the node.
  nod = nodes + bspnode;

  // Allocate a new list for each child.
  childlist = (divline_t*)Z_Malloc(childlistsize*sizeof(divline_t),PU_LEVEL,0);

  // Copy the previous lines.
  if(divlines) memcpy(childlist,divlines,numdivlines*sizeof(divline_t));

  dl = childlist + numdivlines;
  dl->x = nod->x;
  dl->y = nod->y;
  // The right child gets the original line (LEFT side clipped).
  dl->dx = nod->dx;
  dl->dy = nod->dy;
  gld_CarveFlats(nod->children[0],childlistsize,childlist);

  // The left side. We must reverse the line, otherwise the wrong
  // side would get clipped.
  dl->dx = -nod->dx;
  dl->dy = -nod->dy;
  gld_CarveFlats(nod->children[1],childlistsize,childlist);

  // We are finishing with this node, free the allocated list.
  Z_Free(childlist);
}

#ifdef USE_GLU_TESS

static int currentsector; // the sector which is currently tesselated

// ntessBegin
//
// called when the tesselation of a new loop starts

static void CALLBACK ntessBegin( GLenum type )
{
#ifdef _DEBUG
  if (levelinfo)
  {
    if (type==GL_TRIANGLES)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLES\n");
    else
    if (type==GL_TRIANGLE_FAN)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_FAN\n");
    else
    if (type==GL_TRIANGLE_STRIP)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_STRIP\n");
    else
      fprintf(levelinfo, "\t\tBegin: unknown\n");
  }
#endif
  // increase loopcount for currentsector
  sectorloops[ currentsector ].loopcount++;
  // reallocate to get space for another loop
  // PU_LEVEL is used, so this gets freed before a new level is loaded
  sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_STATIC, 0);
  // set initial values for current loop
  // currentloop is -> sectorloops[currentsector].loopcount-1
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=type;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=0;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexindex=gld_num_vertexes;
}

// ntessError
//
// called when the tesselation failes (DEBUG only)

static void CALLBACK ntessError(GLenum error)
{
#ifdef _DEBUG
  const GLubyte *estring;
  estring = gluErrorString(error);
  fprintf(levelinfo, "\t\tTessellation Error: %s\n", estring);
#endif
}

// ntessCombine
//
// called when the two or more vertexes are on the same coordinate

static void CALLBACK ntessCombine( GLdouble coords[3], vertex_t *vert[4], GLfloat w[4], void **dataOut )
{
#ifdef _DEBUG
  if (levelinfo)
  {
    fprintf(levelinfo, "\t\tVertexCombine Coords: x %10.5f, y %10.5f z %10.5f\n", coords[0], coords[1], coords[2]);
    if (vert[0]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[0]->x>>FRACBITS, vert[0]->y>>FRACBITS, vert[0]);
    if (vert[1]) fprintf(levelinfo, "\t\tVertexCombine Vert2 : x %10i, y %10i p %p\n", vert[1]->x>>FRACBITS, vert[1]->y>>FRACBITS, vert[1]);
    if (vert[2]) fprintf(levelinfo, "\t\tVertexCombine Vert3 : x %10i, y %10i p %p\n", vert[2]->x>>FRACBITS, vert[2]->y>>FRACBITS, vert[2]);
    if (vert[3]) fprintf(levelinfo, "\t\tVertexCombine Vert4 : x %10i, y %10i p %p\n", vert[3]->x>>FRACBITS, vert[3]->y>>FRACBITS, vert[3]);
  }
#endif
  // just return the first vertex, because all vertexes are on the same coordinate
  *dataOut = vert[0];
}

// ntessVertex
//
// called when a vertex is found

static void CALLBACK ntessVertex( vertex_t *vert )
{
#ifdef _DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tVertex : x %10i, y %10i\n", vert->x>>FRACBITS, vert->y>>FRACBITS);
#endif
  // increase vertex count
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount++;

  // increase vertex count
  gld_AddGlobalVertexes(1);
  // add the new vertex (vert is the second argument of gluTessVertex)
  gld_texcoords[gld_num_vertexes].u=( (float)vert->x/(float)FRACUNIT)/64.0f;
  gld_texcoords[gld_num_vertexes].v=(-(float)vert->y/(float)FRACUNIT)/64.0f;
  gld_vertexes[gld_num_vertexes].x=-(float)vert->x/MAP_SCALE;
  gld_vertexes[gld_num_vertexes].y=0.0f;
  gld_vertexes[gld_num_vertexes].z= (float)vert->y/MAP_SCALE;
  gld_num_vertexes++;
}

// ntessEnd
//
// called when the tesselation of a the current loop ends (DEBUG only)

static void CALLBACK ntessEnd( void )
{
#ifdef _DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tEnd loopcount %i vertexcount %i\n", sectorloops[currentsector].loopcount, sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount);
#endif
}

// gld_PrecalculateSector
//
// this calculates the loops for the sector "num"
//
// how does it work?
// first I have to credit Michael 'Kodak' Ryssen for the usage of the
// glu tesselation functions. the rest of this stuff is entirely done by me (proff).
// if there are any similarities, then they are implications of the algorithm.
//
// I'm starting with the first line of the current sector. I take it's ending vertex and
// add it to the tesselator. the current line is marked as used. then I'm searching for
// the next line which connects to the current line. if there is more than one line, I
// choose the one with the smallest angle to the current. if there is no next line, I
// start a new loop and take the first unused line in the sector. after all lines are
// processed, the polygon is tesselated.
//
// e6y
// The bug in algorithm of splitting of a sector into the closed contours was fixed.
// There is no more HOM at the starting area on MAP16 @ Eternal.wad
// I hope nothing was broken

static void gld_PrecalculateSector(int num)
{
  int i;
  boolean *lineadded=NULL;
  int linecount;
  int currentline;
  int oldline;
  int currentloop;
  int bestline;
  int bestlinecount;
  vertex_t *startvertex;
  vertex_t *currentvertex = NULL; //e6y: fix use of uninitialized local variable below
  angle_t lineangle;
  angle_t angle;
  angle_t bestangle;
  sector_t *backsector;
  GLUtesselator *tess;
  double *v=NULL;
  int maxvertexnum;
  int vertexnum;

  currentsector=num;
  lineadded=Z_Malloc(sectors[num].linecount*sizeof(boolean),PU_LEVEL,0);
  if (!lineadded)
  {
    if (levelinfo) fclose(levelinfo);
    return;
  }
  // init tesselator
  tess=gluNewTess();
  if (!tess)
  {
    if (levelinfo) fclose(levelinfo);
    Z_Free(lineadded);
    return;
  }
  // set callbacks
  gluTessCallback(tess, GLU_TESS_BEGIN, ntessBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, ntessVertex);
  gluTessCallback(tess, GLU_TESS_ERROR, ntessError);
  gluTessCallback(tess, GLU_TESS_COMBINE, ntessCombine);
  gluTessCallback(tess, GLU_TESS_END, ntessEnd);
  if (levelinfo) fprintf(levelinfo, "sector %i, %i lines in sector\n", num, sectors[num].linecount);
  // remove any line which has both sides in the same sector (i.e. Doom2 Map01 Sector 1)
  for (i=0; i<sectors[num].linecount; i++)
  {
    lineadded[i]=false;
    if (sectors[num].lines[i]->sidenum[0]!=NO_INDEX)
      if (sectors[num].lines[i]->sidenum[1]!=NO_INDEX)
        if (sides[sectors[num].lines[i]->sidenum[0]].sector
          ==sides[sectors[num].lines[i]->sidenum[1]].sector)
        {
          lineadded[i]=true;
          if (levelinfo) fprintf(levelinfo, "line %4i (iLineID %4i) has both sides in same sector (removed)\n", i, sectors[num].lines[i]->iLineID);
        }
  }
  // e6y
  // Remove any line which has a clone with the same vertexes and orientation
  // (i.e. MM.WAD Map22 lines 1298 and 2397)
  // There is no more HOM on Memento Mori MAP22 sector 299
  for (i = 0; i < sectors[num].linecount - 1; i++)
  {
    int j;
    for (j = i + 1; j < sectors[num].linecount; j++)
    {
      if (sectors[num].lines[i]->v1 == sectors[num].lines[j]->v1 &&
          sectors[num].lines[i]->v2 == sectors[num].lines[j]->v2 &&
          sectors[num].lines[i]->frontsector == sectors[num].lines[j]->frontsector &&
          sectors[num].lines[i]->backsector == sectors[num].lines[j]->backsector &&
          lineadded[i] == false && lineadded[j] == false)
      {
        lineadded[i] = true;
      }
    }
  }

  // initialize variables
  linecount=sectors[num].linecount;
  oldline=0;
  currentline=0;
  startvertex=sectors[num].lines[currentline]->v2;
  currentloop=0;
  vertexnum=0;
  maxvertexnum=0;
  // start tesselator
  if (levelinfo) fprintf(levelinfo, "gluTessBeginPolygon\n");
  gluTessBeginPolygon(tess, NULL);
  if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
  gluTessBeginContour(tess);
  while (linecount)
  {
    // if there is no connected line, then start new loop
    if ((oldline==currentline) || (startvertex==currentvertex))
    {
      currentline=-1;
      for (i=0; i<sectors[num].linecount; i++)
        if (!lineadded[i])
        {
          currentline=i;
          currentloop++;
          if ((sectors[num].lines[currentline]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
            startvertex=sectors[num].lines[currentline]->v1;
          else
            startvertex=sectors[num].lines[currentline]->v2;
          if (levelinfo) fprintf(levelinfo, "\tNew Loop %3i\n", currentloop);
          if (oldline!=0)
          {
            if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
            gluTessEndContour(tess);
//            if (levelinfo) fprintf(levelinfo, "\tgluNextContour\n");
//            gluNextContour(tess, GLU_CW);
            if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
            gluTessBeginContour(tess);
          }
          break;
        }
    }
    if (currentline==-1)
      break;
    // add current line
    lineadded[currentline]=true;
    // check if currentsector is on the front side of the line ...
    if ((sectors[num].lines[currentline]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
    {
      // v2 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v2;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y,sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      
      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped false\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    else // ... or on the back side
    {
      // v1 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v1;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y,sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;

      //e6y: direction of a line shouldn't be changed
      //if (lineangle>=180)
      //  lineangle=lineangle-360;

      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped true\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    if (vertexnum>=maxvertexnum)
    {
      maxvertexnum+=512;
      v=Z_Realloc(v,maxvertexnum*3*sizeof(double),PU_LEVEL,0);
    }
    // calculate coordinates for the glu tesselation functions
    v[vertexnum*3+0]=-(double)currentvertex->x/(double)MAP_SCALE;
    v[vertexnum*3+1]=0.0;
    v[vertexnum*3+2]= (double)currentvertex->y/(double)MAP_SCALE;
    // add the vertex to the tesselator, currentvertex is the pointer to the vertexlist of doom
    // v[vertexnum] is the GLdouble array of the current vertex
    if (levelinfo) fprintf(levelinfo, "\t\tgluTessVertex(%i, %i)\n",currentvertex->x>>FRACBITS,currentvertex->y>>FRACBITS);
    gluTessVertex(tess, &v[vertexnum*3], currentvertex);
    // increase vertexindex
    vertexnum++;
    // decrease linecount of current sector
    linecount--;
    // find the next line
    oldline=currentline; // if this isn't changed at the end of the search, a new loop will start
    bestline=-1; // set to start values
    bestlinecount=0;
    // set backsector if there is one
    if (sectors[num].lines[currentline]->sidenum[1]!=NO_INDEX)
      backsector=sides[sectors[num].lines[currentline]->sidenum[1]].sector;
    else
      backsector=NULL;
    // search through all lines of the current sector
    for (i=0; i<sectors[num].linecount; i++)
      if (!lineadded[i]) // if the line isn't already added ...
        // check if one of the vertexes is the same as the current vertex
        if ((sectors[num].lines[i]->v1==currentvertex) || (sectors[num].lines[i]->v2==currentvertex))
        {
          // calculate the angle of this best line candidate
          if ((sectors[num].lines[i]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
            angle = R_PointToAngle2(sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y,sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y);
          else
            angle = R_PointToAngle2(sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y,sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y);
          angle=(angle>>ANGLETOFINESHIFT)*360/8192;

          //e6y: direction of a line shouldn't be changed
          //if (angle>=180)
          //  angle=angle-360;

          // check if line is flipped ...
          if ((sectors[num].lines[i]->sidenum[0]!=NO_INDEX) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
          {
            // when the line is not flipped and startvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v1!=currentvertex)
              continue;
          }
          else
          {
            // when the line is flipped and endvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v2!=currentvertex)
              continue;
          }
          // set new best line candidate
          if (bestline==-1) // if this is the first one ...
          {
            bestline=i;
            bestangle=lineangle-angle;
            bestlinecount++;
          }
          else
            // check if the angle between the current line and this best line candidate is smaller then
            // the angle of the last candidate
            // e6y: for finding an angle between AB and BC vectors we should subtract
            // (BC - BA) == (BC - (180 - AB)) == (angle-(180-lineangle))
            if (D_abs(angle-(180-lineangle))<D_abs(bestangle))
            {
              bestline=i;
              bestangle=angle-(180-lineangle);
              bestlinecount++;
            }
        }
    if (bestline!=-1) // if a line is found, make it the current line
    {
      currentline=bestline;
      if (bestlinecount>1)
        if (levelinfo) fprintf(levelinfo, "\t\tBestlinecount: %4i\n", bestlinecount);
    }
  }
  // let the tesselator calculate the loops
  if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
  gluTessEndContour(tess);
  if (levelinfo) fprintf(levelinfo, "gluTessEndPolygon\n");
  gluTessEndPolygon(tess);
  // clean memory
  gluDeleteTess(tess);
  Z_Free(v);
  Z_Free(lineadded);
}

#endif /* USE_GLU_TESS */

/********************************************
 * Name     : gld_GetSubSectorVertices      *
 * created  : 08/13/00                      *
 * modified : 09/18/00, adapted for PrBoom  *
 * author   : figgi                         *
 * what     : prepares subsectorvertices    *
 *            (glnodes only)                *
 ********************************************/

void gld_GetSubSectorVertices(void)
{
  int      i, j;
  int      numedgepoints;
  subsector_t* ssector;

  for(i = 0; i < numsubsectors; i++)
  {
    ssector = &subsectors[i];

    if (ssector->sector->flags & SECTOR_IS_CLOSED)
      continue;

    numedgepoints  = ssector->numlines;

    gld_AddGlobalVertexes(numedgepoints);

    if ((gld_vertexes) && (gld_texcoords))
    {
      int currentsector = ssector->sector->iSectorID;

      sectorloops[currentsector].loopcount++;
      sectorloops[currentsector].loops = Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_STATIC, 0);
      sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].mode    = GL_TRIANGLE_FAN;
      sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].vertexcount = numedgepoints;
      sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].vertexindex = gld_num_vertexes;
      for(j = 0;  j < numedgepoints; j++)
      {
        gld_texcoords[gld_num_vertexes].u =( (float)(segs[ssector->firstline + j].v1->x)/FRACUNIT)/64.0f;
        gld_texcoords[gld_num_vertexes].v =(-(float)(segs[ssector->firstline + j].v1->y)/FRACUNIT)/64.0f;
        gld_vertexes[gld_num_vertexes].x = -(float)(segs[ssector->firstline + j].v1->x)/MAP_SCALE;
        gld_vertexes[gld_num_vertexes].y = 0.0f;
        gld_vertexes[gld_num_vertexes].z =  (float)(segs[ssector->firstline + j].v1->y)/MAP_SCALE;
        gld_num_vertexes++;
      }
    }
  }
}

// gld_PreprocessLevel
//
// this checks all sectors if they are closed and calls gld_PrecalculateSector to
// calculate the loops for every sector
// the idea to check for closed sectors is from DEU. check next commentary
/*
      Note from RQ:
      This is a very simple idea, but it works!  The first test (above)
      checks that all Sectors are closed.  But if a closed set of LineDefs
      is moved out of a Sector and has all its "external" SideDefs pointing
      to that Sector instead of the new one, then we need a second test.
      That's why I check if the SideDefs facing each other are bound to
      the same Sector.

      Other note from RQ:
      Nowadays, what makes the power of a good editor is its automatic tests.
      So, if you are writing another Doom editor, you will probably want
      to do the same kind of tests in your program.  Fine, but if you use
      these ideas, don't forget to credit DEU...  Just a reminder... :-)
*/
// so I credited DEU

void gld_PreprocessSectors(void)
{
  int i;
#ifdef USE_GLU_TESS // figgi
  char *vertexcheck;
  char *vertexcheck2;
  int v1num;
  int v2num;
  int j;
#endif

#ifdef _DEBUG
  levelinfo=fopen("levelinfo.txt","a");
  if (levelinfo)
  {
    if (gamemode==commercial)
      fprintf(levelinfo,"MAP%02i\n",gamemap);
    else
      fprintf(levelinfo,"E%iM%i\n",gameepisode,gamemap);
  }
#endif

  sectorloops=Z_Malloc(sizeof(GLSector)*numsectors,PU_STATIC,0);
  if (!sectorloops)
    I_Error("gld_PreprocessSectors: Not enough memory for array sectorloops");
  memset(sectorloops, 0, sizeof(GLSector)*numsectors);

  sectorrendered=Z_Malloc(numsectors*sizeof(byte),PU_STATIC,0);
  if (!sectorrendered)
    I_Error("gld_PreprocessSectors: Not enough memory for array sectorrendered");
  memset(sectorrendered, 0, numsectors*sizeof(byte));

  segrendered=Z_Malloc(numsegs*sizeof(byte),PU_STATIC,0);
  if (!segrendered)
    I_Error("gld_PreprocessSectors: Not enough memory for array segrendered");
  memset(segrendered, 0, numsegs*sizeof(byte));

  gld_vertexes=NULL;
  gld_texcoords=NULL;
  gld_max_vertexes=0;
  gld_num_vertexes=0;
  gld_AddGlobalVertexes(numvertexes*2);

#ifdef USE_GLU_TESS
  vertexcheck=malloc(numvertexes*sizeof(vertexcheck[0]));
  vertexcheck2=malloc(numvertexes*sizeof(vertexcheck2[0]));
  if (!vertexcheck || !vertexcheck2)
  {
    if (levelinfo) fclose(levelinfo);
    I_Error("gld_PreprocessSectors: Not enough memory for array vertexcheck");
    return;
  }

  for (i=0; i<numsectors; i++)
  {
    memset(vertexcheck,0,numvertexes*sizeof(vertexcheck[0]));
    memset(vertexcheck2,0,numvertexes*sizeof(vertexcheck2[0]));
    for (j=0; j<sectors[i].linecount; j++)
    {
      v1num=((int)sectors[i].lines[j]->v1-(int)vertexes)/sizeof(vertex_t);
      v2num=((int)sectors[i].lines[j]->v2-(int)vertexes)/sizeof(vertex_t);
      if ((v1num>=numvertexes) || (v2num>=numvertexes))
        continue;

      // e6y: for correct handling of missing textures.
      // We do not need to apply some algos for isolated lines.
      vertexcheck2[v1num]++;
      vertexcheck2[v2num]++;

      if (sectors[i].lines[j]->sidenum[0]!=NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[0]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=1;
          vertexcheck[v2num]|=2;
        }
      if (sectors[i].lines[j]->sidenum[1]!=NO_INDEX)
        if (sides[sectors[i].lines[j]->sidenum[1]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=2;
          vertexcheck[v2num]|=1;
        }
    }
    if (sectors[i].linecount<3)
    {
#ifdef _DEBUG
      lprintf(LO_ERROR, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
#endif
      if (levelinfo) fprintf(levelinfo, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      sectors[i].flags &= ~SECTOR_IS_CLOSED;
    }
    else
    {
      sectors[i].flags |= SECTOR_IS_CLOSED;
      for (j=0; j<numvertexes; j++)
      {
        if ((vertexcheck[j]==1) || (vertexcheck[j]==2))
        {
#ifdef _DEBUG
          lprintf(LO_ERROR, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
#endif
          if (levelinfo) fprintf(levelinfo, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
          sectors[i].flags &= ~SECTOR_IS_CLOSED;
        }
      }
    }

    // e6y: marking all the isolated lines
    for (j=0; j<sectors[i].linecount; j++)
    {
      v1num=((int)sectors[i].lines[j]->v1-(int)vertexes)/sizeof(vertex_t);
      v2num=((int)sectors[i].lines[j]->v2-(int)vertexes)/sizeof(vertex_t);
      if (vertexcheck2[v1num] < 2 && vertexcheck2[v2num] < 2)
      {
        sectors[i].lines[j]->r_flags |= RF_ISOLATED;
      }
    }
    
    // figgi -- adapted for glnodes
    if (sectors[i].flags & SECTOR_IS_CLOSED)
      gld_PrecalculateSector(i);
  }
  free(vertexcheck);
  free(vertexcheck2);
#endif /* USE_GLU_TESS */

  // figgi -- adapted for glnodes
  if (nodesVersion == 0)
    gld_CarveFlats(numnodes-1, 0, 0);
  else
    gld_GetSubSectorVertices();

  if (levelinfo) fclose(levelinfo);
}

float roll     = 0.0f;
float yaw      = 0.0f;
float inv_yaw  = 0.0f;
float pitch    = 0.0f;

#define __glPi 3.14159265358979323846

static void infinitePerspective(GLdouble fovy, GLdouble aspect, GLdouble znear)
{
	GLdouble left, right, bottom, top;
	GLdouble m[16];

	top = znear * tan(fovy * __glPi / 360.0);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;

	//qglFrustum(left, right, bottom, top, znear, zfar);

	m[ 0] = (2 * znear) / (right - left);
	m[ 4] = 0;
	m[ 8] = (right + left) / (right - left);
	m[12] = 0;

	m[ 1] = 0;
	m[ 5] = (2 * znear) / (top - bottom);
	m[ 9] = (top + bottom) / (top - bottom);
	m[13] = 0;

	m[ 2] = 0;
	m[ 6] = 0;
	//m[10] = - (zfar + znear) / (zfar - znear);
	//m[14] = - (2 * zfar * znear) / (zfar - znear);
	m[10] = -1;
	m[14] = -2 * znear;

	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = -1;
	m[15] = 0;

	glMultMatrixd(m);
}

void gld_StartDrawScene(void)
{
  extern int screenblocks;
  int height;

  if (gl_shared_texture_palette)
    glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
  gld_SetPalette(-1);

  if (screenblocks == 11)
    height = SCREENHEIGHT;
  else if (screenblocks == 10)
    height = SCREENHEIGHT;
  else
    height = (screenblocks*SCREENHEIGHT/10) & ~7;

  glViewport(viewwindowx, SCREENHEIGHT-(height+viewwindowy-((height-viewheight)/2)), viewwidth, height);
  glScissor(viewwindowx, SCREENHEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
  glEnable(GL_SCISSOR_TEST);
  // Player coordinates
  xCamera=-(float)viewx/MAP_SCALE;
  yCamera=(float)viewy/MAP_SCALE;
  zCamera=(float)viewz/MAP_SCALE;

  yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
  inv_yaw=180.0f-yaw;

  //e6y: fog in frame
  gl_use_fog = !gl_compatibility && gl_fog && !frame_fixedcolormap && !boom_cm;

//e6y
//  viewMaxY = viewz;
  mlook_or_fov = GetMouseLook() || (render_fov != FOV90);
  if(!mlook_or_fov)
  {
    pitch=0.0f;
    paperitems_pitch = 0.0f;
    skyXShift = -2.0f*((yaw+90.0f)/90.0f);
    skyYShift = 200.0f/319.5f*((100.0f)/100.0f);
  }
  else
  {
    pitch=(float)(float)(viewpitch>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
    paperitems_pitch = ((pitch > 87.0f && pitch <= 90.0f) ? 87.0f : pitch);
    viewPitch = (pitch>180 ? pitch-360 : pitch);
    skyXShift = -2.0f*((yaw+90.0f)/90.0f/fovscale);
    skyYShift = viewPitch<skyUpAngle ? skyUpShift : (float)sin(viewPitch*__glPi/180.0f)-0.2f;
  }
  
  invul_method = 0;
  if (players[displayplayer].fixedcolormap == 32)
  {
    if (gl_boom_colormaps)
    {
      invul_method = INVUL_CM;
    }
    else
    {
      if (glversion >= OPENGL_VERSION_1_3)
      {
        invul_method = INVUL_BW;
      }
      else
      {
        invul_method = INVUL_INV;
      }
    }
  }

#ifdef USE_FBO_TECHNIQUE
  if (gl_use_motionblur)
  {
    ticcmd_t *cmd = &players[displayplayer].cmd;
    int camera_speed = cmd->forwardmove * cmd->forwardmove + cmd->sidemove * cmd->sidemove;
    MotionBlurOn = camera_speed > gl_motionblur_minspeed_pow2;
  }

  SceneInTexture = (gl_ext_framebuffer_object) &&
    ((invul_method & INVUL_BW) || (MotionBlurOn));

  // Vortex: Set FBO object
  if (SceneInTexture)
  {
    GLEXT_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glSceneImageFBOTexID);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  }
  else
#endif
  if (invul_method & INVUL_BW)
  {
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_DOT3_RGB);
    glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
  }

#ifdef _DEBUG
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#else
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif

  if (gl_drawskys == 2)
    gld_DrawScreenSkybox();

  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

//e6y  infinitePerspective(64.0f, 320.0f/200.0f, (float)gl_nearclip/100.0f);
  infinitePerspective(internal_render_fov,render_aspect_ratio, (float)gl_nearclip/100.0f);//e6y

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(roll,  0.0f, 0.0f, 1.0f);
  glRotatef(pitch, 1.0f, 0.0f, 0.0f);
  glRotatef(yaw,   0.0f, 1.0f, 0.0f);
  glTranslatef(-xCamera, -zCamera, -yCamera);

  rendermarker++;
  gld_ResetDrawInfo();
}

//e6y
static void gld_ProcessExtraAlpha(void)
{
  if (extra_alpha>0.0f)
  {
    glDisable(GL_ALPHA_TEST);
    glColor4f(extra_red, extra_green, extra_blue, extra_alpha);
    glBindTexture(GL_TEXTURE_2D, 0);
    last_gltexture = NULL;
    last_cm = -1;
    glBegin(GL_TRIANGLE_STRIP);
      glVertex2f( 0.0f, 0.0f);
      glVertex2f( 0.0f, (float)SCREENHEIGHT);
      glVertex2f( (float)SCREENWIDTH, 0.0f);
      glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
    glEnable(GL_ALPHA_TEST);
  }
}

//e6y
static void gld_InvertScene(void)
{
  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
  glColor4f(1,1,1,1);
  glBindTexture(GL_TEXTURE_2D, 0);
  last_gltexture = NULL;
  last_cm = -1;
  glBegin(GL_TRIANGLE_STRIP);
    glVertex2f( 0.0f, 0.0f);
    glVertex2f( 0.0f, (float)SCREENHEIGHT);
    glVertex2f( (float)SCREENWIDTH, 0.0f);
    glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
  glEnd();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gld_EndDrawScene(void)
{
  glDisable(GL_POLYGON_SMOOTH);

  glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
  glDisable(GL_FOG);
  gld_Set2DMode();

  if (viewangleoffset <= 1024<<ANGLETOFINESHIFT ||
    viewangleoffset >=-1024<<ANGLETOFINESHIFT)
  { // don't draw on side views
    R_DrawPlayerSprites();
  }

  // e6y
  // Effect of invulnerability uses a colormap instead of hard-coding now
  // See nuts.wad
  // http://www.doomworld.com/idgames/index.php?id=11402

#ifdef USE_FBO_TECHNIQUE
  // Vortex: Black and white effect
  if (SceneInTexture)
  {
    // below if scene is in texture
    if (!invul_method)
    {
      gld_ProcessExtraAlpha();
    }

    // Vortex: Restore original RT
    GLEXT_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glBindTexture(GL_TEXTURE_2D, glSceneImageTextureFBOTexID);

    // Setup blender
    if (invul_method & INVUL_BW)
    {
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
      glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_DOT3_RGB);
      glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_PRIMARY_COLOR);
      glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
      glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_TEXTURE);
      glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);

      if (gl_invul_bw_method == 0)
        glColor3f(0.3f, 0.3f, 0.4f);
      else
        glColor3f(bw_red, bw_green, bw_blue);
    }
    else
    {
      glColor3f(1.0f, 1.0f, 1.0f);
    }

    //e6y: motion bloor effect for strafe50
    if (MotionBlurOn)
    {
      extern int renderer_fps;
      static float prev_alpha = 0;
      static float motionblur_alpha = 1.0f;

      if (realframe)
      {
        motionblur_alpha = (float)((atan(-renderer_fps/gl_motionblur_a))/gl_motionblur_b)+gl_motionblur_c;
      }

      glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
      GLEXT_glBlendColorEXT(1.0f, 1.0f, 1.0f, motionblur_alpha);
    }
  
    glBegin(GL_TRIANGLE_STRIP);
    {
      glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, (float)SCREENHEIGHT);
      glTexCoord2f(1.0f, 1.0f); glVertex2f((float)SCREENWIDTH, 0.0f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f((float)SCREENWIDTH, (float)SCREENHEIGHT);
    }
    glEnd();

    
    if (MotionBlurOn)
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  }
  else
#endif
  {
    if (invul_method & INVUL_INV)
    {
      gld_InvertScene();
    }
    if (invul_method & INVUL_BW)
    {
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    }

    if (!invul_method)
    {
      gld_ProcessExtraAlpha();
    }
  }

  glColor3f(1.0f,1.0f,1.0f);
  glDisable(GL_SCISSOR_TEST);
  if (gl_shared_texture_palette)
    glDisable(GL_SHARED_TEXTURE_PALETTE_EXT);
}

//
// gld_FreeDrawInfo
//
static void gld_FreeDrawInfo(void)
{
  int i;

  for (i = 0; i < gld_drawinfo.maxsize; i++)
  {
    if (gld_drawinfo.data[i].data)
    {
      free(gld_drawinfo.data[i].data);
      gld_drawinfo.data[i].data = 0;
    }
  }
  free(gld_drawinfo.data);
  gld_drawinfo.data = 0;

  for (i = 0; i < GLDIT_TYPES; i++)
  {
    if (gld_drawinfo.items[i])
    {
      free(gld_drawinfo.items[i]);
      gld_drawinfo.items[i] = 0;
    }
  }

  memset(&gld_drawinfo, 0, sizeof(GLDrawInfo));
}

//
// gld_ResetDrawInfo
//
// Should be used between frames (in gld_StartDrawScene)
//
static void gld_ResetDrawInfo(void)
{
  int i;

  for (i = 0; i < gld_drawinfo.maxsize; i++)
  {
    gld_drawinfo.data[i].size = 0;
  }
  gld_drawinfo.size = 0;

  for (i = 0; i < GLDIT_TYPES; i++)
  {
    gld_drawinfo.num_items[i] = 0;
  }
}

//
// gld_AddDrawRange
//
static void gld_AddDrawRange(int size)
{
  gld_drawinfo.maxsize++;
  gld_drawinfo.data = realloc(gld_drawinfo.data, 
    gld_drawinfo.maxsize * sizeof(gld_drawinfo.data[0]));

  gld_drawinfo.data[gld_drawinfo.size].maxsize = size;
  gld_drawinfo.data[gld_drawinfo.size].data = malloc(size);
  gld_drawinfo.data[gld_drawinfo.size].size = 0;
}

//
// gld_AddDrawItem
//
#define NEWSIZE (MAX(64 * 1024, itemsize))
static void gld_AddDrawItem(GLDrawItemType itemtype, void *itemdata)
{
  int itemsize = 0;
  byte *item_p = NULL;

  static itemsizes[GLDIT_TYPES] = {
    0,
    sizeof(GLWall), sizeof(GLWall), sizeof(GLWall), sizeof(GLWall),
    sizeof(GLFlat), sizeof(GLFlat),
    sizeof(GLSprite), sizeof(GLSprite),
  };

  itemsize = itemsizes[itemtype];
  if (itemsize == 0)
  {
    I_Error("gld_AddDrawItem: unknown GLDrawItemType %d", itemtype);
  }

  if (gld_drawinfo.maxsize == 0)
  {
    gld_AddDrawRange(NEWSIZE);
  }

  if (gld_drawinfo.data[gld_drawinfo.size].size + itemsize >=
    gld_drawinfo.data[gld_drawinfo.size].maxsize)
  {
    gld_drawinfo.size++;
    if (gld_drawinfo.size >= gld_drawinfo.maxsize)
    {
      gld_AddDrawRange(NEWSIZE);
    }
  }

  item_p = gld_drawinfo.data[gld_drawinfo.size].data +
    gld_drawinfo.data[gld_drawinfo.size].size;

  memcpy(item_p, itemdata, itemsize);

  gld_drawinfo.data[gld_drawinfo.size].size += itemsize;

  if (gld_drawinfo.num_items[itemtype] >= gld_drawinfo.max_items[itemtype])
  {
    gld_drawinfo.max_items[itemtype] += 64;
    gld_drawinfo.items[itemtype] = realloc(
      gld_drawinfo.items[itemtype],
      gld_drawinfo.max_items[itemtype] * sizeof(gld_drawinfo.items[0]));
  }

  gld_drawinfo.items[itemtype][gld_drawinfo.num_items[itemtype]].item.item = item_p;
  gld_drawinfo.num_items[itemtype]++;
}
#undef NEWSIZE

/*****************
 *               *
 * Walls         *
 *               *
 *****************/

static void gld_DrawWall(GLWall *wall)
{
  rendered_segs++;

  if ( (!gl_drawskys) && (wall->flag>=GLDWF_SKY) )
    wall->gltexture=NULL;
  gld_BindTexture(wall->gltexture);
  if (!wall->gltexture)
  {
#ifdef _DEBUG
    glColor4f(1.0f,0.0f,0.0f,1.0f);
#endif
  }
  if (wall->flag>=GLDWF_SKY)
  {
    if ( wall->gltexture )
    {
      float sx, sy;

      glMatrixMode(GL_TEXTURE);
      glPushMatrix();
      
      if (!mlook_or_fov)
      {
        if ((wall->flag & GLDWF_SKYFLIP) == GLDWF_SKYFLIP)
          sx = -128.0f / (float)wall->gltexture->buffer_width;
        else
          sx = +128.0f / (float)wall->gltexture->buffer_width;

        //sy = 200.0f / 320.0f * (256 / wall->gltexture->buffer_height);
        sy = 160.0f / (float)wall->gltexture->buffer_height;

        glScalef(sx, sy, 1.0f);
        glTranslatef(wall->skyyaw, wall->skyymid, 0.0f);
      }
      else 
      {
        if ((wall->flag & GLDWF_SKYFLIP) == GLDWF_SKYFLIP)
          sx = (wall->gltexture->buffer_width == 256 ? -64.0f : -128.0f) *
            fovscale / (float)wall->gltexture->buffer_width;
        else
          sx = (wall->gltexture->buffer_width == 256 ? 64.0f : 128.0f) *
            fovscale / (float)wall->gltexture->buffer_width;

        sy = 200.0f / 320.0f * fovscale;

        glScalef(sx, sy, 1.0f);
        glTranslatef(wall->skyyaw, wall->skyymid, 0.0f);
      }

      if (!SkyDrawed)
      {             
        float maxcoord = 255.0f;
        boolean mlook = GetMouseLook();
        SkyDrawed = true;
        if (mlook)
        {
          glBegin(GL_TRIANGLE_STRIP);
          glVertex3f(-maxcoord,+maxcoord,+maxcoord);
          glVertex3f(+maxcoord,+maxcoord,+maxcoord);
          glVertex3f(-maxcoord,+maxcoord,-maxcoord);
          glVertex3f(+maxcoord,+maxcoord,-maxcoord);
          glEnd();
          
          glBegin(GL_TRIANGLE_STRIP);
          glVertex3f(-maxcoord,-maxcoord,+maxcoord);
          glVertex3f(+maxcoord,-maxcoord,+maxcoord);
          glVertex3f(-maxcoord,-maxcoord,-maxcoord);
          glVertex3f(+maxcoord,-maxcoord,-maxcoord);
          glEnd();
        }
      }

    }
    glBegin(GL_TRIANGLE_STRIP);
      glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);
      glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);
      glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);
      glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);
    glEnd();
    if ( wall->gltexture )
    {
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
    }
  }
  else
  {
    //e6y
    if (gl_arb_multitexture && render_detailedwalls &&
      distance2piece(xCamera, yCamera, 
      wall->glseg->x1, wall->glseg->z1,
      wall->glseg->x2, wall->glseg->z2) < DETAIL_DISTANCE)
    {
      gld_DrawWallWithDetail(wall);
    }
    else
    {
      if ((wall->flag == GLDWF_TOPFLUD) || (wall->flag == GLDWF_BOTFLUD))
      {
        gl_strip_coords_t c;

        gld_BindFlat(wall->gltexture);

        gld_SetupFloodStencil(wall);
        gld_SetupFloodedPlaneCoords(wall, &c);
        gld_SetupFloodedPlaneLight(wall);
        gld_DrawTriangleStrip(wall, &c);

        gld_ClearFloodStencil(wall);
      }
      else
      {
        gld_StaticLightAlpha(wall->light, wall->alpha);

        glBegin(GL_TRIANGLE_FAN);

        // lower left corner
        glTexCoord2f(wall->ul,wall->vb); glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);

        // split left edge of wall
        if (gl_seamless && !wall->glseg->fracleft)
          gld_SplitLeftEdge(wall, false, 0.0f, 0.0f);

        // upper left corner
        glTexCoord2f(wall->ul,wall->vt); glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);

        // upper right corner
        glTexCoord2f(wall->ur,wall->vt); glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);

        // split right edge of wall
        if (gl_seamless && !wall->glseg->fracright)
          gld_SplitRightEdge(wall, false, 0.0f, 0.0f);

        // lower right corner
        glTexCoord2f(wall->ur,wall->vb); glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);

        glEnd();
      }
    }
  }
}

#define LINE seg->linedef
#define CALC_Y_VALUES(w, lineheight, floor_height, ceiling_height)\
  (w).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;\
  (w).ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;\
  lineheight=((float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT)))

#define OU(w,seg) (((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width)
#define OV(w,seg) (((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height)
#define OV_PEG(w,seg,v_offset) (OV((w),(seg))-(((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height))

#define CALC_TEX_VALUES_TOP(w, seg, peg, linelength, lineheight)\
  (w).flag=GLDWF_TOP;\
  (w).ul=OU((w),(seg))+(0.0f);\
  (w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
  (peg)?\
  (\
    (w).vb=OV((w),(seg))+((w).gltexture->scaleyfac),\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
  ):(\
    (w).vt=OV((w),(seg))+(0.0f),\
    (w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
  )

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, peg, linelength, lineheight)\
  (w).flag=GLDWF_M1S;\
  (w).ul=OU((w),(seg))+(0.0f);\
  (w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
  (peg)?\
  (\
    (w).vb=OV((w),(seg))+((w).gltexture->scaleyfac),\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
  ):(\
    (w).vt=OV((w),(seg))+(0.0f),\
    (w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
  )

#define CALC_TEX_VALUES_MIDDLE2S(w, seg, peg, linelength, lineheight)\
  (w).flag=GLDWF_M2S;\
  (w).ul=OU((w),(seg))+(0.0f);\
  (w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->buffer_width);\
  (peg)?\
  (\
    (w).vb=((w).gltexture->scaleyfac),\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
  ):(\
    (w).vt=(0.0f),\
    (w).vb=((float)(lineheight)/(float)(w).gltexture->buffer_height)\
  )

#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, linelength, lineheight, v_offset)\
  (w).flag=GLDWF_BOT;\
  (w).ul=OU((w),(seg))+(0.0f);\
  (w).ur=OU((w),(seg))+((linelength)/(float)(w).gltexture->realtexwidth);\
  (peg)?\
  (\
    (w).vb=OV_PEG((w),(seg),(v_offset))+((w).gltexture->scaleyfac),\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height))\
  ):(\
    (w).vt=OV((w),(seg))+(0.0f),\
    (w).vb=OV((w),(seg))+((float)(lineheight)/(float)(w).gltexture->buffer_height)\
  )

// e6y
// Sky textures with a zero index should be forced
// See third episode of requiem.wad
static void gld_AddSkyTexture(GLWall *wall, int sky1, int sky2)
{
  line_t *l = NULL;
  wall->gltexture = NULL;

  if ((sky1) & PL_SKYFLAT)
  {
    l = &lines[sky1 & ~PL_SKYFLAT];
  }
  else
  {
    if ((sky2) & PL_SKYFLAT)
    {
      l = &lines[sky2 & ~PL_SKYFLAT];
    }
  }
  
  if (l)
  {
    side_t *s = *l->sidenum + sides;
    wall->gltexture = gld_RegisterTexture(texturetranslation[s->toptexture], false,
      texturetranslation[s->toptexture] == skytexture || l->special == 271 || l->special == 272);
    if (wall->gltexture)
    {
      if (!mlook_or_fov)
      {
        wall->skyyaw  = -2.0f*((-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES)/90.0f);
        wall->skyymid = 200.0f/319.5f*(((float)s->rowoffset/(float)FRACUNIT - 28.0f)/100.0f);
      }
      else
      {
        wall->skyyaw  = -2.0f*(((270.0f-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES)+90.0f)/90.0f/fovscale);
        wall->skyymid = skyYShift+(((float)s->rowoffset/(float)FRACUNIT)/100.0f);
      }
      wall->flag = (l->special == 272 ? GLDWF_SKY : GLDWF_SKYFLIP);
    }
  }
  else
  {
    wall->gltexture = gld_RegisterTexture(skytexture, false, true);
    if (wall->gltexture)
    {
      wall->skyyaw  = skyXShift;
      wall->skyymid = skyYShift;
      wall->flag = GLDWF_SKY;
    }
  }

  if (wall->gltexture)
  {
    wall->gltexture->flags |= GLTEXTURE_SKY;
    gld_AddDrawItem(GLDIT_SWALL, wall);
  }
}

void gld_AddWall(seg_t *seg)
{
  GLWall wall;
  GLTexture *temptex;
  sector_t *frontsector;
  sector_t *backsector;
  sector_t ftempsec; // needed for R_FakeFlat
  sector_t btempsec; // needed for R_FakeFlat
  float lineheight;
  int rellight = 0;

  if (!segrendered)
    return;
  if (segrendered[seg->iSegID]==rendermarker)
    return;
  segrendered[seg->iSegID]=rendermarker;
  if (!seg->frontsector)
    return;
  frontsector=R_FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
  if (!frontsector)
    return;
  wall.glseg=&gl_segs[seg->iSegID];

  // e6y: fake contrast stuff
  // Original doom added/removed one light level ((1<<LIGHTSEGSHIFT) == 16) 
  // for walls exactly vertical/horizontal on the map
  if (gl_lightmode == gl_lightmode_gzdoom)
    rellight = seg->linedef->dx==0? +8 : seg->linedef->dy==0 ? -8 : 0;
  else
    rellight = seg->linedef->dx==0? +(1<<LIGHTSEGSHIFT) : seg->linedef->dy==0 ? -(1<<LIGHTSEGSHIFT) : 0;
  wall.light=gld_CalcLightLevel(frontsector->lightlevel+rellight+(extralight<<5));
  wall.fogdensity = gld_CalcFogDensity(frontsector, frontsector->lightlevel);
  wall.alpha=1.0f;
  wall.gltexture=NULL;
  wall.seg = seg; //e6y

  if (!seg->backsector) /* onesided */
  {
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      wall.ybottom=(float)frontsector->ceilingheight/MAP_SCALE;
      gld_AddSkyTexture(&wall, frontsector->sky, frontsector->sky);
    }
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ytop=(float)frontsector->floorheight/MAP_SCALE;
      wall.ybottom=-255.0f;
      gld_AddSkyTexture(&wall, frontsector->sky, frontsector->sky);
    }
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true, false);
    if (temptex)
    {
      wall.gltexture=temptex;
      CALC_Y_VALUES(wall, lineheight, frontsector->floorheight, frontsector->ceilingheight);
      CALC_TEX_VALUES_MIDDLE1S(
        wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
        segs[seg->iSegID].length, lineheight
      );
      gld_AddDrawItem((wall.alpha == 1.0f ? GLDIT_WALL : GLDIT_TWALL), &wall);
    }
  }
  else /* twosided */
  {
    int floor_height,ceiling_height;

    backsector=R_FakeFlat(seg->backsector, &btempsec, NULL, NULL, true); // for boom effects
    if (!backsector)
      return;
    /* toptexture */
    ceiling_height=frontsector->ceilingheight;
    floor_height=backsector->ceilingheight;
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      if (
          // e6y
          // There is no more HOM in the starting area on Memento Mori map29 and on map30.
          // Old code:
          // (backsector->ceilingheight==backsector->floorheight) &&
          // (backsector->ceilingpic==skyflatnum)
          (backsector->ceilingpic==skyflatnum) &&
          ((backsector->ceilingheight<=backsector->floorheight)||
           (backsector->ceilingheight<=frontsector->floorheight))
         )
      {
        // e6y
        // There is no more visual glitches with sky on Icarus map14 sector 187
        // Old code: wall.ybottom=(float)backsector->floorheight/MAP_SCALE;
        wall.ybottom=((float)(backsector->floorheight +
          (seg->sidedef->rowoffset > 0 ? seg->sidedef->rowoffset : 0)))/MAP_SCALE;
        gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
      }
      else
      {
        if ( (texturetranslation[seg->sidedef->toptexture]!=NO_TEXTURE) )
        {
          // e6y
          // It corrects some problem with sky, but I do not remember which one
          // old code: wall.ybottom=(float)frontsector->ceilingheight/MAP_SCALE;
          wall.ybottom=(float)MAX(frontsector->ceilingheight,backsector->ceilingheight)/MAP_SCALE;

          gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
        }
        else
          if ( (backsector->ceilingheight <= frontsector->floorheight) ||
               (backsector->ceilingpic != skyflatnum) )
          {
            wall.ybottom=(float)backsector->ceilingheight/MAP_SCALE;
            gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
          }
      }
    }
    if (floor_height<ceiling_height)
    {
      if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
      {
        temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->toptexture], true, false);
        if (!temptex && gl_use_stencil && backsector &&
          !(seg->linedef->r_flags & RF_ISOLATED) &&
          frontsector->ceilingpic != skyflatnum && backsector->ceilingpic != skyflatnum)
        {
          wall.ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;
          wall.ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;
          if (wall.ybottom >= zCamera)
          {
            wall.flag=GLDWF_TOPFLUD;
            temptex=gld_RegisterFlat(flattranslation[seg->backsector->ceilingpic], true);
            wall.gltexture=temptex;
            gld_AddDrawItem(GLDIT_FWALL, &wall);
          }
        }
        else
        if (temptex)
        {
          wall.gltexture=temptex;
          CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
          CALC_TEX_VALUES_TOP(
            wall, seg, (LINE->flags & (/*e6y ML_DONTPEGBOTTOM | */ML_DONTPEGTOP))==0,
            segs[seg->iSegID].length, lineheight
          );
          gld_AddDrawItem((wall.alpha == 1.0f ? GLDIT_WALL : GLDIT_TWALL), &wall);
        }
      }
    }

    /* midtexture */
    //e6y
    if (comp[comp_maskedanim])
      temptex=gld_RegisterTexture(seg->sidedef->midtexture, true, false);
    else

    // e6y
    // Animated middle textures with a zero index should be forced
    // See spacelab.wad (http://www.doomworld.com/idgames/index.php?id=6826)
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true, true);
    if (temptex && seg->sidedef->midtexture != NO_TEXTURE)
    {
      wall.gltexture=temptex;
      if ( (LINE->flags & ML_DONTPEGBOTTOM) >0)
      {
        if (backsector->ceilingheight<=frontsector->floorheight)
          goto bottomtexture;
        floor_height=MAX(seg->frontsector->floorheight,seg->backsector->floorheight)+(seg->sidedef->rowoffset);
        ceiling_height=floor_height+(wall.gltexture->realtexheight<<FRACBITS);
      }
      else
      {
        if (backsector->ceilingheight<=frontsector->floorheight)
          goto bottomtexture;
        ceiling_height=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight)+(seg->sidedef->rowoffset);
        floor_height=ceiling_height-(wall.gltexture->realtexheight<<FRACBITS);
      }
      // e6y
      // The fix for wrong middle texture drawing
      // if it exceeds the boundaries of its floor and ceiling
      
      /*CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
      CALC_TEX_VALUES_MIDDLE2S(
        wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
        segs[seg->iSegID].length, lineheight
      );*/
      {
        int floormax, ceilingmin, linelen;
        float mip;
        mip = (float)wall.gltexture->realtexheight/(float)wall.gltexture->buffer_height;
//        if ( (texturetranslation[seg->sidedef->bottomtexture]!=R_TextureNumForName("-")) )
        if (seg->sidedef->bottomtexture)
          floormax=MAX(seg->frontsector->floorheight,seg->backsector->floorheight);
        else
          floormax=floor_height;
        if (seg->sidedef->toptexture)
          ceilingmin=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight);
        else
          ceilingmin=ceiling_height;
        linelen=abs(ceiling_height-floor_height);
        wall.ytop=((float)MIN(ceilingmin, ceiling_height)/(float)MAP_SCALE);
        wall.ybottom=((float)MAX(floormax, floor_height)/(float)MAP_SCALE);

        // e6y: z-fighting
        // The supersecret "COOL!" area of Kama Sutra map15 as example
        if (wall.ytop < wall.ybottom)
          goto bottomtexture;

        wall.flag=GLDWF_M2S;
        wall.ul=OU((wall),(seg))+(0.0f);
        wall.ur=OU(wall,(seg))+((segs[seg->iSegID].length)/(float)wall.gltexture->buffer_width);
        if (floormax<=floor_height)
#ifdef USE_GLU_IMAGESCALE
          wall.vb=1.0f;
#else  // USE_GLU_IMAGESCALE
          wall.vb=mip*1.0f;
#endif // USE_GLU_IMAGESCALE
        else
          wall.vb=mip*((float)(ceiling_height - floormax))/linelen;
        if (ceilingmin>=ceiling_height)
          wall.vt=0.0f;
        else
          wall.vt=mip*((float)(ceiling_height - ceilingmin))/linelen;
      }

      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.alpha=(float)tran_filter_pct/100.0f;
      gld_AddDrawItem((wall.alpha == 1.0f ? GLDIT_WALL : GLDIT_TWALL), &wall);
      wall.alpha=1.0f;
    }
bottomtexture:
    /* bottomtexture */
    ceiling_height=backsector->floorheight;
    floor_height=frontsector->floorheight;
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ybottom=-255.0f;
      if (
          (backsector->ceilingheight==backsector->floorheight) &&
          (backsector->floorpic==skyflatnum)
         )
      {
        wall.ytop=(float)backsector->floorheight/MAP_SCALE;
        gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
      }
      else
      {
        if ( (texturetranslation[seg->sidedef->bottomtexture]!=NO_TEXTURE) )
        {
          wall.ytop=(float)frontsector->floorheight/MAP_SCALE;
          gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
        }
        else
          if ( (backsector->floorheight >= frontsector->ceilingheight) ||
               (backsector->floorpic != skyflatnum) )
          {
            wall.ytop=(float)backsector->floorheight/MAP_SCALE;
            gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky);
          }
      }
    }
    if (floor_height<ceiling_height)
    {
      temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->bottomtexture], true, false);
      if (!temptex && gl_use_stencil && backsector &&
        !(seg->linedef->r_flags & RF_ISOLATED) &&
        frontsector->floorpic != skyflatnum && backsector->floorpic != skyflatnum)
      {
        wall.ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;
        wall.ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;
        if (wall.ytop <= zCamera)
        {
          wall.flag = GLDWF_BOTFLUD;
          temptex=gld_RegisterFlat(flattranslation[seg->backsector->floorpic], true);
          wall.gltexture=temptex;
          gld_AddDrawItem(GLDIT_FWALL, &wall);
        }
      }
      else
      if (temptex)
      {
        wall.gltexture=temptex;
        CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
        CALC_TEX_VALUES_BOTTOM(
          wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
          segs[seg->iSegID].length, lineheight,
          floor_height-frontsector->ceilingheight
        );
        gld_AddDrawItem((wall.alpha == 1.0f ? GLDIT_WALL : GLDIT_TWALL), &wall);
      }
    }
  }
}

#undef LINE
#undef CALC_Y_VALUES
#undef OU
#undef OV
#undef OV_PEG
#undef CALC_TEX_VALUES_TOP
#undef CALC_TEX_VALUES_MIDDLE1S
#undef CALC_TEX_VALUES_MIDDLE2S
#undef CALC_TEX_VALUES_BOTTOM
#undef ADDWALL

static void gld_PreprocessSegs(void)
{
  int i;

  gl_segs=Z_Malloc(numsegs*sizeof(GLSeg),PU_STATIC,0);
  for (i=0; i<numsegs; i++)
  {
    gl_segs[i].x1=-(float)segs[i].v1->x/(float)MAP_SCALE;
    gl_segs[i].z1= (float)segs[i].v1->y/(float)MAP_SCALE;
    gl_segs[i].x2=-(float)segs[i].v2->x/(float)MAP_SCALE;
    gl_segs[i].z2= (float)segs[i].v2->y/(float)MAP_SCALE;
  }
}

/*****************
 *               *
 * Flats         *
 *               *
 *****************/

static void gld_DrawFlat(GLFlat *flat)
{
  int loopnum; // current loop number
  GLLoopDef *currentloop; // the current loop

  rendered_visplanes++;

  gld_BindFlat(flat->gltexture);
  gld_StaticLight(flat->light);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,flat->z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(flat->uoffs/64.0f,flat->voffs/64.0f,0.0f);
  
//e6y
  if (gl_arb_multitexture && render_detailedflats)
  {
    float s;
    TAnimItemParam *animitem = &anim_flats[flat->gltexture->index - firstflat];

    GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    gld_StaticLight(flat->light);
    
    if (!animitem->anim)
    {
      s = 0.0f;
    }
    else
    {
      s = 1.0f / animitem->anim->numpics * animitem->index;
      if (s < 0.001) s = 0.0f;
    }

    glPushMatrix();
    glTranslatef(s + flat->uoffs/16.0f,flat->voffs/16.0f,0.0f);
    glScalef(4.0f, 4.0f, 1.0f);
    //      glTranslatef(0.0f,flat->z,0.0f);
  }

  if (flat->sectornum>=0)
  {
    // go through all loops of this sector
#ifndef USE_VERTEX_ARRAYS
    for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
    {
      int vertexnum;
      // set the current loop
      currentloop=&sectorloops[flat->sectornum].loops[loopnum];
      if (!currentloop)
        continue;
      // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
      glBegin(currentloop->mode);
      // go through all vertexes of this loop
      for (vertexnum=currentloop->vertexindex; vertexnum<(currentloop->vertexindex+currentloop->vertexcount); vertexnum++)
      {
        // set texture coordinate of this vertex
        if (gl_arb_multitexture && render_detailedflats)
        {
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, (GLfloat*)&gld_texcoords[vertexnum]);
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, (GLfloat*)&gld_texcoords[vertexnum]);
        }
        else
        {
          glTexCoord2fv((GLfloat*)&gld_texcoords[vertexnum]);
        }
        // set vertex coordinate
        glVertex3fv((GLfloat*)&gld_vertexes[vertexnum]);
      }
      // end of loop
      glEnd();
    }
#else
    for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
    {
      // set the current loop
      currentloop=&sectorloops[flat->sectornum].loops[loopnum];
      glDrawArrays(currentloop->mode,currentloop->vertexindex,currentloop->vertexcount);
    }
#endif
  }

  //e6y
  if (gl_arb_multitexture && render_detailedflats)
  {
    //glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
  }

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

// gld_AddFlat
//
// This draws on flat for the sector "num"
// The ceiling boolean indicates if the flat is a floor(false) or a ceiling(true)

static void gld_AddFlat(int sectornum, boolean ceiling, visplane_t *plane)
{
  sector_t *sector; // the sector we want to draw
  sector_t tempsec; // needed for R_FakeFlat
  int floorlightlevel;      // killough 3/16/98: set floor lightlevel
  int ceilinglightlevel;    // killough 4/11/98
  GLFlat flat;

  if (sectornum<0)
    return;
  flat.sectornum=sectornum;
  sector=&sectors[sectornum]; // get the sector
  sector=R_FakeFlat(sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false); // for boom effects
  flat.ceiling=ceiling;
  if (!ceiling) // if it is a floor ...
  {
    if (sector->floorpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.gltexture=gld_RegisterFlat(flattranslation[plane->picnum], true);
    if (!flat.gltexture)
      return;
    // get the lightlevel from floorlightlevel
    flat.light=gld_CalcLightLevel(plane->lightlevel+(extralight<<5));
    flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel);
    // calculate texture offsets
    flat.uoffs=(float)sector->floor_xoffs/(float)FRACUNIT;
    flat.voffs=(float)sector->floor_yoffs/(float)FRACUNIT;
  }
  else // if it is a ceiling ...
  {
    if (sector->ceilingpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.gltexture=gld_RegisterFlat(flattranslation[plane->picnum], true);
    if (!flat.gltexture)
      return;
    // get the lightlevel from ceilinglightlevel
    flat.light=gld_CalcLightLevel(plane->lightlevel+(extralight<<5));
    flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel);
    // calculate texture offsets
    flat.uoffs=(float)sector->ceiling_xoffs/(float)FRACUNIT;
    flat.voffs=(float)sector->ceiling_yoffs/(float)FRACUNIT;
  }

  // get height from plane
  flat.z=(float)plane->height/MAP_SCALE;

  gld_AddDrawItem((flat.ceiling ? GLDIT_CEILING : GLDIT_FLOOR), &flat);
}

void gld_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling)
{
  subsector_t *subsector;

  // check if all arrays are allocated
  if (!sectorrendered)
    return;

  subsector = &subsectors[subsectornum];
  if (!subsector)
    return;
  if (sectorrendered[subsector->sector->iSectorID]!=rendermarker) // if not already rendered
  {
    // render the floor
    if (floor && floor->height < viewz)
      gld_AddFlat(subsector->sector->iSectorID, false, floor);
    // render the ceiling
    if (ceiling && ceiling->height > viewz)
      gld_AddFlat(subsector->sector->iSectorID, true, ceiling);
    // set rendered true
    sectorrendered[subsector->sector->iSectorID]=rendermarker;
  }
}

/*****************
 *               *
 * Sprites       *
 *               *
 *****************/

static void gld_DrawSprite(GLSprite *sprite)
{
  float offsety;

  rendered_vissprites++;

  gld_BindPatch(sprite->gltexture,sprite->cm);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  // Bring items up out of floor by configurable amount times .01 Mead 8/13/03
  // e6y: adjust sprite clipping
  offsety = (gl_spriteclip != spriteclip_const ? sprite->y : sprite->y + (.01f * (float)gl_sprite_offset));
  if (!render_paperitems && !(sprite->thing->flags & (MF_SOLID | MF_SPAWNCEILING)))
  {
    float xcenter = (float)fabs((sprite->x1 + sprite->x2) * 0.5f);
    float ycenter = (float)fabs((sprite->y1 + sprite->y2) * 0.5f);
    glTranslatef(sprite->x + xcenter, offsety + ycenter, sprite->z);
    glRotatef(inv_yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(paperitems_pitch, 1.0f, 0.0f, 0.0f);
    glTranslatef(-xcenter, -ycenter, 0);
  }
  else
  {
    glTranslatef(sprite->x, offsety, sprite->z);

    glRotatef(inv_yaw,0.0f,1.0f,0.0f);
  }

  if(sprite->shadow)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    //glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
    glAlphaFunc(GL_GEQUAL,0.1f);
    glColor4f(0.2f,0.2f,0.2f,0.33f);
  }
  else
  {
    if(sprite->trans)
      gld_StaticLightAlpha(sprite->light,(float)tran_filter_pct/100.0f);
    else
      gld_StaticLight(sprite->light);
  }
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(sprite->ul, sprite->vt); glVertex3f(sprite->x1, sprite->y1, 0.0f);
    glTexCoord2f(sprite->ur, sprite->vt); glVertex3f(sprite->x2, sprite->y1, 0.0f);
    glTexCoord2f(sprite->ul, sprite->vb); glVertex3f(sprite->x1, sprite->y2, 0.0f);
    glTexCoord2f(sprite->ur, sprite->vb); glVertex3f(sprite->x2, sprite->y2, 0.0f);
  glEnd();

  glPopMatrix();

  if(sprite->shadow)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.5f);
  }
}

void gld_AddSprite(vissprite_t *vspr)
{
  mobj_t *pSpr=vspr->thing;
  GLSprite sprite;
  float voff,hoff;

  sprite.scale=vspr->scale;
  if (pSpr->frame & FF_FULLBRIGHT)
    sprite.light = 1.0f;
  else
    sprite.light = gld_CalcLightLevel(pSpr->subsector->sector->lightlevel+(extralight<<5));
  sprite.cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  sprite.gltexture=gld_RegisterPatch(vspr->patch+firstspritelump,sprite.cm);
  if (!sprite.gltexture)
    return;
  sprite.shadow = (pSpr->flags & MF_SHADOW) != 0;
  sprite.trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
  sprite.thing = vspr->thing;//e6y
  if (movement_smooth)
  {
    sprite.x = (float)(-pSpr->PrevX + FixedMul (tic_vars.frac, -pSpr->x - (-pSpr->PrevX)))/MAP_SCALE;
    sprite.y = (float)(pSpr->PrevZ + FixedMul (tic_vars.frac, pSpr->z - pSpr->PrevZ))/MAP_SCALE;
    sprite.z = (float)(pSpr->PrevY + FixedMul (tic_vars.frac, pSpr->y - pSpr->PrevY))/MAP_SCALE;
  }
  else
  {
    sprite.x=-(float)pSpr->x/MAP_SCALE;
    sprite.y= (float)pSpr->z/MAP_SCALE;
    sprite.z= (float)pSpr->y/MAP_SCALE;
  }

  sprite.vt=0.0f;
  sprite.vb=sprite.gltexture->scaleyfac;
  if (vspr->flip)
  {
    sprite.ul=0.0f;
    sprite.ur=sprite.gltexture->scalexfac;
  }
  else
  {
    sprite.ul=sprite.gltexture->scalexfac;
    sprite.ur=0.0f;
  }
  hoff=(float)sprite.gltexture->leftoffset/(float)(MAP_COEFF);
  voff=(float)sprite.gltexture->topoffset/(float)(MAP_COEFF);
  sprite.x1=hoff-((float)sprite.gltexture->realtexwidth/(float)(MAP_COEFF));
  sprite.x2=hoff;
  sprite.y1=voff;
  sprite.y2=voff-((float)sprite.gltexture->realtexheight/(float)(MAP_COEFF));
  
  // e6y
  // if the sprite is below the floor, and it's not a hanger/floater/missile, 
  // and it's not a fully dead corpse, move it up
  if (gl_spriteclip != spriteclip_const)
  {
    uint_64_t flags = vspr->thing->flags;
    if (sprite.y2 < 0 && !(flags & (MF_SPAWNCEILING|MF_FLOAT|MF_MISSILE|MF_NOGRAVITY)) && 
      ((gl_spriteclip == spriteclip_always) || !(flags & MF_CORPSE && vspr->thing->tics == -1)))
    {
      sprite.y1 -= sprite.y2;
      sprite.y2 = 0.0f;
    }
  }

  //e6y: support for transparent sprites
  gld_AddDrawItem((sprite.trans || sprite.shadow ? GLDIT_TSPRITE : GLDIT_SPRITE), &sprite);
}

/*****************
 *               *
 * Draw          *
 *               *
 *****************/

//e6y
static inline gl_SetAlphaBlend(boolean on)
{
  static boolean gl_alpha_blend_enabled;

  if (on)
  {
    if (!gl_alpha_blend_enabled) 
    {
      glEnable(GL_ALPHA_TEST);
      glEnable(GL_BLEND);
    }
  }
  else 
  {
    if (gl_alpha_blend_enabled) 
    {
      glDisable(GL_ALPHA_TEST);
      glDisable(GL_BLEND);
    }
  }

  gl_alpha_blend_enabled = on;
}

//e6y
void gld_ProcessWall(GLWall *wall)
{
  gl_SetAlphaBlend(wall->seg->backsector || !(wall->gltexture->flags&GLTEXTURE_HASHOLES));

  // e6y
  // The ultimate 'ATI sucks' fix: Some of ATIs graphics cards are so unprecise when 
  // rendering geometry that each and every border between polygons must be seamless, 
  // otherwise there are rendering artifacts worse than anything that could be seen 
  // on Geforce 2's! Made this a menu option because the speed impact is quite severe
  // and this special handling is not necessary on modern NVidia cards.
  if (gl_seamless)
  {
    seg_t *seg = wall->seg;
    vertex_t *v1, *v2;
    if (seg->sidedef == &sides[seg->linedef->sidenum[0]])
    {
      v1 = seg->linedef->v1;
      v2 = seg->linedef->v2;
    }
    else
    {
      v1 = seg->linedef->v2;
      v2 = seg->linedef->v1;
    }

    wall->glseg->fracleft  = (seg->v1->x != v1->x) || (seg->v1->y != v1->y);
    wall->glseg->fracright = (seg->v2->x != v2->x) || (seg->v2->y != v2->y);

    gld_RecalcVertexHeights(seg->v1);
    gld_RecalcVertexHeights(seg->v2);
  }

  gld_DrawWall(wall);
}

void gld_DrawScene(player_t *player)
{
  int i,j,k;
  fixed_t max_scale;

  //e6y
  SkyDrawed = false;

  //e6y: must call it twice for correct initialisation
  gl_SetAlphaBlend(false);
  gl_SetAlphaBlend(true);

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  rendered_visplanes = rendered_segs = rendered_vissprites = 0;

  if (gl_drawskys == 4)
    gld_DrawDomeSkyBox();

  // enable backside removing
  glEnable(GL_CULL_FACE);

  // floors
  glCullFace(GL_FRONT);
  for (i = gld_drawinfo.num_items[GLDIT_FLOOR] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_FLOOR][i].item.flat->fogdensity);
    gld_DrawFlat(gld_drawinfo.items[GLDIT_FLOOR][i].item.flat);
  }

  // ceilings
  glCullFace(GL_BACK);
  for (i = gld_drawinfo.num_items[GLDIT_CEILING] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_CEILING][i].item.flat->fogdensity);
    gld_DrawFlat(gld_drawinfo.items[GLDIT_CEILING][i].item.flat);
  }

  // disable backside removing
  glDisable(GL_CULL_FACE);

  // opaque walls
  for (i = gld_drawinfo.num_items[GLDIT_WALL] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_WALL][i].item.wall->fogdensity);
    gld_ProcessWall(gld_drawinfo.items[GLDIT_WALL][i].item.wall);
  }

  gl_EnableFog(false);

  // projected walls
  if (gl_use_stencil && gld_drawinfo.num_items[GLDIT_FWALL] > 0)
  {
    // Push bleeding floor/ceiling textures back a little in the z-buffer
    // so they don't interfere with overlapping mid textures.
    glPolygonOffset(1.0f, 128.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_STENCIL_TEST);
    for (i = gld_drawinfo.num_items[GLDIT_FWALL] - 1; i >= 0; i--)
    {
      GLWall *wall = gld_drawinfo.items[GLDIT_FWALL][i].item.wall;
      
      if (gl_use_fog)
      {
        // calculation of fog density for flooded walls
        if (((wall->flag == GLDWF_TOPFLUD) || (wall->flag == GLDWF_BOTFLUD)) && (wall->seg->backsector))
        {
          wall->fogdensity = gld_CalcFogDensity(frontsector, wall->seg->backsector->lightlevel);
        }

        gld_SetFog(wall->fogdensity);
      }

      gld_ProcessWall(wall);
    }
    glDisable(GL_STENCIL_TEST);

    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
  }

  gl_SetAlphaBlend(true);

  if (gl_drawskys == 1) // skybox is already applied if gl_drawskys == 2
  {
    if ( (gl_drawskys) )
    {
      if (comp[comp_skymap] && gl_shared_texture_palette)
        glDisable(GL_SHARED_TEXTURE_PALETTE_EXT);

      if (comp[comp_skymap] && (invul_method & INVUL_BW))
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_Q);
      if (comp[comp_skymap] || !(invul_method & INVUL_BW))
        glColor4fv(gl_whitecolor);
    }

    // skies
    for (i = gld_drawinfo.num_items[GLDIT_SWALL] - 1; i >= 0; i--)
    {
      gld_DrawWall(gld_drawinfo.items[GLDIT_SWALL][i].item.wall);
    }

    if (gl_drawskys)
    {
      glDisable(GL_TEXTURE_GEN_Q);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_S);

      if (comp[comp_skymap] && (invul_method & INVUL_BW))
        glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_COMBINE);

      if (comp[comp_skymap] && gl_shared_texture_palette)
        glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
    }
  }

  // opaque sprites
  for (i = gld_drawinfo.num_items[GLDIT_SPRITE] - 1; i >= 0; i--)
  {
    gld_DrawSprite(gld_drawinfo.items[GLDIT_SPRITE][i].item.sprite);
  }

  // transparent stuff
  if (gld_drawinfo.num_items[GLDIT_TWALL] > 0 || gld_drawinfo.num_items[GLDIT_TSPRITE] > 0)
  {
    // if translucency percentage is less than 50,
    // then all translucent textures and sprites disappear completely
    // without this line
    glAlphaFunc(GL_GREATER, 0.0f);

    // transparent walls
    for (i = gld_drawinfo.num_items[GLDIT_TWALL] - 1; i >= 0; i--)
    {
      gld_SetFog(gld_drawinfo.items[GLDIT_TWALL][i].item.wall->fogdensity);
      gld_ProcessWall(gld_drawinfo.items[GLDIT_TWALL][i].item.wall);
    }

    gl_SetAlphaBlend(true);

    // transparent sprites
    for (i = gld_drawinfo.num_items[GLDIT_TSPRITE] - 1; i >= 0; i--)
    {
      // sorting is necessary only for transparent sprites.
      // from back to front
      do
      {
        max_scale = INT_MAX;
        k = -1;
        for (j = gld_drawinfo.num_items[GLDIT_TSPRITE] - 1; j >= 0; j--)
          if (gld_drawinfo.items[GLDIT_TSPRITE][j].item.sprite->scale < max_scale)
          {
            max_scale = gld_drawinfo.items[GLDIT_TSPRITE][j].item.sprite->scale;
            k = j;
          }
          if (k >= 0)
          {
            gld_DrawSprite(gld_drawinfo.items[GLDIT_TSPRITE][k].item.sprite);
            gld_drawinfo.items[GLDIT_TSPRITE][k].item.sprite->scale=INT_MAX;
          }
      } while (max_scale!=INT_MAX);
    }

    // restoration of an original condition
    glAlphaFunc(GL_GEQUAL, 0.5f);
  }

  // e6y: detail
  if (!gl_arb_multitexture && render_usedetail)
    gld_DrawDetail_NoARB();

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  if (gl_drawskys == 3)
    gld_DrawScreenSkybox();

  if (gl_drawskys == 5)
    gld_DrawDomeSkyBox();
}

void gld_PreprocessLevel(void)
{
  // e6y: speedup of level reloading
  // Do not preprocess GL data twice for same level
  if (!gl_preprocessed)
  {
    int i;
    static int numsectors_prev = 0;

    free(gl_segs);

    free(gld_texcoords);
    free(gld_vertexes);

    free(segrendered);
    free(sectorrendered);
    
    for (i = 0; i < numsectors_prev; i++)
    {
      free(sectorloops[i].loops);
    }
    free(sectorloops);

    gld_Precache();
    gld_PreprocessSectors();
    gld_PreprocessFakeSectors();
    gld_PreprocessSegs();

    numsectors_prev = numsectors;
  }
  else
  {
    gld_PreprocessFakeSectors();

    memset(sectorrendered, 0, numsectors*sizeof(sectorrendered[0]));
    memset(segrendered, 0, numsegs*sizeof(segrendered[0]));
  }

  gld_FreeDrawInfo();

  glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
  glVertexPointer(3,GL_FLOAT,0,gld_vertexes);

  //e6y
  gld_PreprocessDetail();
  gld_InitVertexData();

  gl_preprocessed = true;
}

// Vortex: some FBO stuff
#ifdef USE_FBO_TECHNIQUE

void gld_InitFBO(void)
{
  gld_FreeScreenSizeFBO();

  gl_use_motionblur = gl_ext_framebuffer_object && gl_motionblur && gl_ext_blend_color;

  gl_use_FBO = gl_ext_framebuffer_object && (gl_use_motionblur || !gl_boom_colormaps);

  if (gl_use_FBO)
  {
    if (gld_CreateScreenSizeFBO())
    {
      // motion blur setup
      gld_InitMotionBlur();
    }
    else
    {
      gl_use_FBO = false;
      gl_ext_framebuffer_object = false;
    }
  }
}

boolean gld_CreateScreenSizeFBO(void)
{
  int status = 0;

  if (!gl_ext_framebuffer_object)
    return false;

  GLEXT_glGenFramebuffersEXT(1, &glSceneImageFBOTexID);
  GLEXT_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glSceneImageFBOTexID);

  GLEXT_glGenRenderbuffersEXT(1, &glDepthBufferFBOTexID);
  GLEXT_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, glDepthBufferFBOTexID);

  GLEXT_glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, SCREENWIDTH, SCREENHEIGHT);
  GLEXT_glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, glDepthBufferFBOTexID);
  
  glGenTextures(1, &glSceneImageTextureFBOTexID);
  glBindTexture(GL_TEXTURE_2D, glSceneImageTextureFBOTexID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREENWIDTH, SCREENHEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  
  // e6y
  // Some ATI’s drivers have a bug whereby adding the depth renderbuffer
  // and then a texture causes the application to crash.
  // This should be kept in mind when doing any FBO related work and
  // tested for as it is possible it could be fixed in a future driver revision
  // thus rendering the problem non-existent.
  PRBOOM_TRY(EXEPTION_glFramebufferTexture2DEXT)
  {
    GLEXT_glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, glSceneImageTextureFBOTexID, 0);
    status = GLEXT_glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  }
  PRBOOM_EXCEPT(EXEPTION_glFramebufferTexture2DEXT)

  if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    GLEXT_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }
  else
  {
    lprintf(LO_ERROR, "gld_CreateScreenSizeFBO: Cannot create framebuffer object (error code: %d)\n", status);
    gl_ext_framebuffer_object = false;
  }

  return (status == GL_FRAMEBUFFER_COMPLETE_EXT);
}

void gld_FreeScreenSizeFBO(void)
{
  if (!gl_ext_framebuffer_object)
    return;

  GLEXT_glDeleteFramebuffersEXT(1, &glSceneImageFBOTexID);
  glSceneImageFBOTexID = 0;

  GLEXT_glDeleteRenderbuffersEXT(1, &glDepthBufferFBOTexID);
  glDepthBufferFBOTexID = 0;

  glDeleteTextures(1, &glSceneImageTextureFBOTexID);
  glSceneImageTextureFBOTexID = 0;
}

void gld_InitMotionBlur(void)
{
  if (gl_use_motionblur)
  {
    float f;

    sscanf(gl_motionblur_minspeed, "%f", &f);
    sscanf(gl_motionblur_att_a, "%f", &gl_motionblur_a);
    sscanf(gl_motionblur_att_b, "%f", &gl_motionblur_b);
    sscanf(gl_motionblur_att_c, "%f", &gl_motionblur_c);

    gl_motionblur_minspeed_pow2 = (int)(f * f);
  }
}
#endif
