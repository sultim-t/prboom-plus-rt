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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gl_opengl.h"

#include "z_zone.h"
#include <math.h>
#include <SDL.h>
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
#include "am_map.h"
#include "sc_man.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "e6y.h"//e6y

// All OpenGL extentions will be disabled in gl_compatibility mode
int gl_compatibility = 0;

int gl_vsync = true;
int gl_clear;

int gl_preprocessed = false;

int gl_spriteindex;
int scene_has_overlapped_sprites;

int gl_blend_animations;

int gl_use_display_lists;
int flats_display_list;
int flats_display_list_size = 0;
int flats_detail_display_list;
int flats_detail_display_list_size = 0;

int gl_finish = 1;

// e6y
// This variables toggles the use of a trick to prevent the clearning of the 
// z-buffer between frames. When this variable is set to "1", the game will not 
// clear the z-buffer between frames. This will result in increased performance
// only on very old hardware and might cause problems for some display hardware.
int gl_ztrick;
int gl_ztrickframe = 0;
float gldepthmin, gldepthmax;

unsigned int invul_method;
float bw_red = 0.3f;
float bw_green = 0.59f;
float bw_blue = 0.11f;

extern int tran_filter_pct;

dboolean use_fog=false;

int gl_nearclip=5;
int gl_texture_filter;
int gl_sprite_filter;
int gl_patch_filter;
int gl_texture_filter_anisotropic = 0;

//sprites
spriteclipmode_t gl_spriteclip;
const char *gl_spriteclipmodes[] = {"constant", "full", "smart"};
int gl_spriteclip_threshold;
float gl_spriteclip_threshold_f;
int gl_sprites_frustum_culling;
int gl_sprite_offset_default;	// item out of floor offset Mead 8/13/03
float gl_sprite_offset;       // precalcilated float value for gl_sprite_offset_default
int gl_sprite_blend;  // e6y: smooth sprite edges
int gl_mask_sprite_threshold;
float gl_mask_sprite_threshold_f;

GLuint gld_DisplayList=0;
int fog_density=200;
static float extra_red=0.0f;
static float extra_green=0.0f;
static float extra_blue=0.0f;
static float extra_alpha=0.0f;

GLfloat gl_whitecolor[4]={1.0f,1.0f,1.0f,1.0f};

GLfloat cm2RGB[CR_LIMIT + 1][4] =
{
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

void SetFrameTextureMode(void)
{
#ifdef USE_FBO_TECHNIQUE
  if (SceneInTexture)
  {
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

  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE); 
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
}

void gld_InitTextureParams(void)
{
  typedef struct tex_filter_s
  {
    dboolean mipmap;
    int tex_filter;
    int mipmap_filter;
    const char *tex_filter_name;
    const char *mipmap_filter_name;
  } tex_filter_t;

  typedef struct tex_format_s
  {
    int tex_format;
    const char *tex_format_name;
  } tex_format_t;

  tex_filter_t params[filter_count] = {
    {false, GL_NEAREST, GL_NEAREST,                "GL_NEAREST", "GL_NEAREST"},
    {false, GL_LINEAR,  GL_LINEAR,                 "GL_LINEAR",  "GL_LINEAR"},
    {true,  GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, "GL_NEAREST", "GL_NEAREST_MIPMAP_NEAREST"},
    {true,  GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR,  "GL_NEAREST", "GL_NEAREST_MIPMAP_LINEAR"},
    {true,  GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,  "GL_LINEAR",  "GL_LINEAR_MIPMAP_NEAREST"},
    {true,  GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR,   "GL_LINEAR",  "GL_LINEAR_MIPMAP_LINEAR"},
  };

  tex_format_t tex_formats[] = {
    {GL_RGBA2,   "GL_RGBA2"},
    {GL_RGBA4,   "GL_RGBA4"},
    {GL_RGB5_A1, "GL_RGB5_A1"},
    {GL_RGBA8,   "GL_RGBA8"},
    {GL_RGBA,    "GL_RGBA"},
    {0, NULL}
  };

  int i;
  int *var[MIP_COUNT] = {&gl_texture_filter, &gl_sprite_filter, &gl_patch_filter};

  for (i = 0; i < MIP_COUNT; i++)
  {
#ifdef USE_GLU_MIPMAP
    tex_filter[i].mipmap     = params[*var[i]].mipmap;
#else
    tex_filter[i].mipmap     = false;
#endif
    tex_filter[i].mag_filter = params[*var[i]].tex_filter;
    tex_filter[i].min_filter = params[*var[i]].mipmap_filter;
  }

  if (tex_filter[MIP_TEXTURE].mipmap)
  {
    gl_shared_texture_palette = false;
  }

  i = 0;
  while (tex_formats[i].tex_format_name)
  {
    if (!strcasecmp(gl_tex_format_string, tex_formats[i].tex_format_name))
    {
      gl_tex_format = tex_formats[i].tex_format;
      lprintf(LO_INFO,"Using texture format %s.\n", tex_formats[i].tex_format_name);
      break;
    }
    i++;
  }
}

void gld_MultisamplingInit(void)
{
  if (render_multisampling)
  {
    extern int gl_colorbuffer_bits;
    extern int gl_depthbuffer_bits;
    
    gl_colorbuffer_bits = 32;
    SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, gl_colorbuffer_bits );
  
    if (gl_depthbuffer_bits!=8 && gl_depthbuffer_bits!=16 && gl_depthbuffer_bits!=24)
      gl_depthbuffer_bits = 16;
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, gl_depthbuffer_bits );

    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, render_multisampling );
    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 1 );
  }
}

void gld_MultisamplingCheck(void)
{
  if (render_multisampling)
  {
    int test = -1;
    SDL_GL_GetAttribute (SDL_GL_MULTISAMPLESAMPLES, &test);
    if (test!=render_multisampling)
    {
      void M_SaveDefaults (void);
      int i=render_multisampling;
      render_multisampling = 0;
      M_SaveDefaults ();
      I_Error("Couldn't set %dX multisamples for %dx%d video mode", i, SCREENWIDTH, SCREENHEIGHT);
    }
  }
}

void gld_MultisamplingSet(void)
{
  if (render_multisampling)
  {
    int use_multisampling = map_use_multisamling ||
      (!(automapmode & am_active) || (automapmode & am_overlay));

    gld_EnableMultisample(use_multisampling);
  }
}

int gld_LoadGLDefs(const char * defsLump)
{
  typedef enum
  {
    TAG_SKYBOX,
    TAG_DETAIL,

    TAG_MAX
  } gldef_type_e;

  // these are the core types available in the *DEFS lump
  static const char *CoreKeywords[TAG_MAX + 1] =
  {
    "skybox",
    "detail",

    NULL
  };

  int result = false;

  if (W_CheckNumForName(defsLump) != -1)
  {
    SC_OpenLump(defsLump);

    // Get actor class name.
    while (SC_GetString())
    {
      switch (SC_MatchString(CoreKeywords))
      {
      case TAG_SKYBOX:
        result = true;
        gld_ParseSkybox();
        break;
      case TAG_DETAIL:
        result = true;
        gld_ParseDetail();
        break;
      }
    }

    SC_Close();
  }

  return result;
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
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    const char *rover = extensions;
    const char *p = rover;

    while (*rover)
    {
      size_t len = 0;
      p = rover;
      while (*p && *p != ' ')
      {
        p++;
        len++;
      }
      if (*p)
      {
        len = MIN(len, sizeof(ext_name)-1);
        memset(ext_name, 0, sizeof(ext_name));
        strncpy(ext_name, rover, len);
        lprintf(LO_INFO,"\t%s\n", ext_name);
      }
      rover = p;
      while (*rover && *rover == ' ')
        rover++;
    }
  }

  gld_InitOpenGL(gl_compatibility);
  gld_InitPalettedTextures();
  gld_InitTextureParams();

  glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);

  glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
  glClearDepth(1.0f);

  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_CLAMP_NV);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // proff_dis
  glShadeModel(GL_FLAT);
  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
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
  gld_InitSky();
  M_ChangeLightMode();
  M_ChangeAllowFog();

  gld_InitDetail();
  gld_InitShadows();

#ifdef HAVE_LIBSDL_IMAGE
  gld_InitMapPics();
  gld_InitHiRes();
#endif

  // Create FBO object and associated render targets
#ifdef USE_FBO_TECHNIQUE
  gld_InitFBO();
  atexit(gld_FreeScreenSizeFBO);
#endif

  if(!gld_LoadGLDefs("GLBDEFS"))
  {
    gld_LoadGLDefs("GLDEFS");
  }

  gld_ResetLastTexture();

  atexit(gld_CleanMemory); //e6y
}

void gld_InitCommandLine(void)
{
}

//
// Textured automap
//

static int C_DECL dicmp_visible_subsectors_by_pic(const void *a, const void *b)
{
  return (*((const subsector_t *const *)b))->sector->floorpic -
         (*((const subsector_t *const *)a))->sector->floorpic;
}

static int visible_subsectors_count_prev = -1;
void gld_ResetTexturedAutomap(void)
{
  visible_subsectors_count_prev = -1;
}

void gld_MapDrawSubsectors(player_t *plr, int fx, int fy, fixed_t mx, fixed_t my, int fw, int fh, fixed_t scale)
{
  extern int ddt_cheating;
  
  static subsector_t **visible_subsectors = NULL;
  static int visible_subsectors_size = 0;
  int visible_subsectors_count;

  int i;
  float alpha;
  float coord_scale;
  GLTexture *gltexture;

  alpha = (float)((automapmode & am_overlay) ? map_textured_overlay_trans : map_textured_trans) / 100.0f;
  if (alpha == 0)
    return;

  if (numsubsectors > visible_subsectors_size)
  {
    visible_subsectors_size = numsubsectors;
    visible_subsectors = realloc(visible_subsectors, visible_subsectors_size * sizeof(visible_subsectors[0]));
  }

  visible_subsectors_count = 0;
  if (ddt_cheating)
  {
    visible_subsectors_count = numsubsectors;
  }
  else
  {
    for (i = 0; i < numsubsectors; i++)
    {
      visible_subsectors_count += map_subsectors[i];
    }
  }

  // Do not sort static visible_subsectors array at all
  // if there are no new visible subsectors.
  if (visible_subsectors_count != visible_subsectors_count_prev)
  {
    visible_subsectors_count_prev = visible_subsectors_count;

    visible_subsectors_count = 0;
    for (i = 0; i < numsubsectors; i++)
    {
      if (map_subsectors[i] || ddt_cheating)
      {
        visible_subsectors[visible_subsectors_count++] = &subsectors[i];
      }
    }

    // sort subsectors by texture
    qsort(visible_subsectors, visible_subsectors_count,
      sizeof(visible_subsectors[0]), dicmp_visible_subsectors_by_pic);
  }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glScissor(fx, SCREENHEIGHT - (fy + fh), fw, fh);
  glEnable(GL_SCISSOR_TEST);

  if (automapmode & am_rotate)
  {
    float pivotx = (float)(fx + fw / 2);
    float pivoty = (float)(fy + fh / 2);

    float rot = -(float)(ANG90 - viewangle) / (float)(1u << 31) * 180.0f;

    // Apply the automap's rotation to the vertexes.
    glTranslatef(pivotx, pivoty, 0.0f);
    glRotatef(rot, 0.0f, 0.0f, 1.0f);
    glTranslatef(-pivotx, -pivoty, 0.0f);
  }

  glTranslatef(
    (float)fx - (float)mx / (float)FRACUNIT * (float)scale / (float)FRACUNIT, 
    (float)fy + (float)fh + (float)my / (float)FRACUNIT * (float)scale / (float)FRACUNIT,
    0);
  coord_scale = (float)scale / (float)(1<<FRACTOMAPBITS) / (float)FRACUNIT * MAP_COEFF;
  glScalef(-coord_scale, -coord_scale, 1.0f);
  
  for (i = 0; i < visible_subsectors_count; i++)
  {
    subsector_t *sub = visible_subsectors[i];
    int ssidx = sub - subsectors;

    if (sub->sector->bbox[BOXLEFT] > am_frame.bbox[BOXRIGHT] ||
      sub->sector->bbox[BOXRIGHT] < am_frame.bbox[BOXLEFT] ||
      sub->sector->bbox[BOXBOTTOM] > am_frame.bbox[BOXTOP] ||
      sub->sector->bbox[BOXTOP] < am_frame.bbox[BOXBOTTOM])
    {
      continue;
    }

    gltexture = gld_RegisterFlat(flattranslation[sub->sector->floorpic], true);
    if (gltexture)
    {
      sector_t tempsec;
      int floorlight;
      float light;
      float floor_uoffs, floor_voffs;
      int loopnum;

      // For lighting and texture determination
      sector_t *sec = R_FakeFlat(sub->sector, &tempsec, &floorlight, NULL, false);

      gld_BindFlat(gltexture, 0);
      light = gld_Calc2DLightLevel(floorlight);
      gld_StaticLightAlpha(light, alpha);

      // Find texture origin.
      floor_uoffs = (float)sec->floor_xoffs/(float)(FRACUNIT*64);
      floor_voffs = (float)sec->floor_yoffs/(float)(FRACUNIT*64);

      for (loopnum = 0; loopnum < subsectorloops[ssidx].loopcount; loopnum++)
      {
        int vertexnum;
        GLLoopDef *currentloop = &subsectorloops[ssidx].loops[loopnum];

        if (!currentloop)
          continue;

        // set the mode (GL_TRIANGLE_FAN)
        glBegin(currentloop->mode);
        // go through all vertexes of this loop
        for (vertexnum = currentloop->vertexindex; vertexnum < (currentloop->vertexindex + currentloop->vertexcount); vertexnum++)
        {
          glTexCoord2f(flats_vbo[vertexnum].u + floor_uoffs, flats_vbo[vertexnum].v + floor_voffs);
          glVertex3f(flats_vbo[vertexnum].x, flats_vbo[vertexnum].z, 0);
        }
        glEnd();
      }
    }
  }

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glDisable(GL_SCISSOR_TEST);
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

void gld_DrawNumPatch_f(float x, float y, int lump, int cm, enum patch_translation_e flags)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;
  int cmap;
  int leftoffset, topoffset;

  //e6y
  dboolean bFakeColormap;

  cmap = ((flags & VPT_TRANS) ? cm : CR_DEFAULT);
  gltexture=gld_RegisterPatch(lump, cmap);
  gld_BindPatch(gltexture, cmap);

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

  if (flags & VPT_NOOFFSET)
  {
    leftoffset = 0;
    topoffset = 0;
  }
  else
  {
    leftoffset = gltexture->leftoffset;
    topoffset = gltexture->topoffset;
  }

  if (flags & VPT_STRETCH_MASK)
  {
    stretch_param_t *params = &stretch_params[flags & VPT_ALIGN_MASK];

    xpos   = (float)((x - leftoffset) * params->video->width)  / 320.0f + params->deltax1;
    ypos   = (float)((y - topoffset)  * params->video->height) / 200.0f + params->deltay1;
    width  = (float)(gltexture->realtexwidth     * params->video->width)  / 320.0f;
    height = (float)(gltexture->realtexheight    * params->video->height) / 200.0f;
  }
  else
  {
    xpos   = (float)(x - leftoffset);
    ypos   = (float)(y - topoffset);
    width  = (float)(gltexture->realtexwidth);
    height = (float)(gltexture->realtexheight);
  }

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

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags)
{
  gld_DrawNumPatch_f((float)x, (float)y, lump, cm, flags);
}

void gld_FillFlat(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
  GLTexture *gltexture;
  float fU1, fU2, fV1, fV2;

  //e6y: Boom colormap should not be applied for background
  int saved_boom_cm = boom_cm;
  boom_cm = 0;

  gltexture = gld_RegisterFlat(lump, false);
  gld_BindFlat(gltexture, 0);

  //e6y
  boom_cm = saved_boom_cm;

  if (!gltexture)
    return;

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  fU1 = 0;
  fV1 = 0;
  fU2 = (float)width / (float)gltexture->realtexwidth;
  fV2 = (float)height / (float)gltexture->realtexheight;

  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((float)(x),(float)(y));
    glTexCoord2f(fU1, fV2); glVertex2f((float)(x),(float)(y + height));
    glTexCoord2f(fU2, fV1); glVertex2f((float)(x + width),(float)(y));
    glTexCoord2f(fU2, fV2); glVertex2f((float)(x + width),(float)(y + height));
  glEnd();
}

void gld_FillPatch(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
  GLTexture *gltexture;
  float fU1, fU2, fV1, fV2;

  //e6y: Boom colormap should not be applied for background
  int saved_boom_cm = boom_cm;
  boom_cm = 0;

  gltexture = gld_RegisterPatch(lump, false);
  gld_BindPatch(gltexture, CR_DEFAULT);

  if (!gltexture)
    return;

  x = x - gltexture->leftoffset;
  y = y - gltexture->topoffset;

  //e6y
  boom_cm = saved_boom_cm;

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  fU1 = 0;
  fV1 = 0;
  fU2 = (float)width / (float)gltexture->realtexwidth;
  fV2 = (float)height / (float)gltexture->realtexheight;

  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fU1, fV1); glVertex2f((float)(x),(float)(y));
    glTexCoord2f(fU1, fV2); glVertex2f((float)(x),(float)(y + height));
    glTexCoord2f(fU2, fV1); glVertex2f((float)(x + width),(float)(y));
    glTexCoord2f(fU2, fV2); glVertex2f((float)(x + width),(float)(y + height));
  glEnd();
}

void gld_DrawLine_f(float x0, float y0, float x1, float y1, int BaseColor)
{
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  const unsigned char *playpal = V_GetPlaypal();
  unsigned char r, g, b, a;
  map_line_t *line;
  
  a = ((automapmode & am_overlay) ? map_lines_overlay_trans * 255 / 100 : 255);
  if (a == 0)
    return;

  r = playpal[3 * BaseColor + 0];
  g = playpal[3 * BaseColor + 1];
  b = playpal[3 * BaseColor + 2];

  line = M_ArrayGetNewItem(&map_lines, sizeof(line[0]));

  line->point[0].x = x0;
  line->point[0].y = y0;
  line->point[0].r = r;
  line->point[0].g = g;
  line->point[0].b = b;
  line->point[0].a = a;

  line->point[1].x = x1;
  line->point[1].y = y1;
  line->point[1].r = r;
  line->point[1].g = g;
  line->point[1].b = b;
  line->point[1].a = a;
#else
  const unsigned char *playpal = V_GetPlaypal();
  
  float alpha = ((automapmode & am_overlay) ? map_lines_overlay_trans / 100.0f : 1.0f);
  if (alpha == 0)
    return;

  glColor4f((float)playpal[3*BaseColor]/255.0f,
            (float)playpal[3*BaseColor+1]/255.0f,
            (float)playpal[3*BaseColor+2]/255.0f,
            alpha);
  glBegin(GL_LINES);
    glVertex2f( x0, y0 );
    glVertex2f( x1, y1 );
  glEnd();
#endif
}

void gld_DrawLine(int x0, int y0, int x1, int y1, int BaseColor)
{
  gld_DrawLine_f((float)x0, (float)y0, (float)x1, (float)y1, BaseColor);
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
  // e6y: don't do the gamma table correction on the lighting
  light = (float)lightlevel / 255.0f;

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
  const unsigned char *playpal = V_GetPlaypal();

  gld_EnableTexture2D(GL_TEXTURE0_ARB, false);
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
  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
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

    playpal = V_GetPlaypal();
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
    GLEXT_glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, pal);
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

unsigned char *gld_ReadScreen(void)
{ // NSM convert to static
  static unsigned char *scr = NULL;
  static unsigned char *buffer = NULL;
  static int scr_size = 0;
  static int buffer_size = 0;

  int i, size;

  size = REAL_SCREENWIDTH * 3;
  if (!buffer || size > buffer_size)
  {
    buffer_size = size;
    buffer = realloc (buffer, size);
  }
  size = REAL_SCREENWIDTH * REAL_SCREENHEIGHT * 3;
  if (!scr || size > scr_size)
  {
    scr_size = size;
    scr = realloc (scr, size);
  }

  if (buffer && scr)
  {
      GLint pack_aligment;
      glGetIntegerv(GL_PACK_ALIGNMENT, &pack_aligment);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      
      glFlush();
      glReadPixels(0, 0, REAL_SCREENWIDTH, REAL_SCREENHEIGHT, GL_RGB, GL_UNSIGNED_BYTE, scr);
      
      glPixelStorei(GL_PACK_ALIGNMENT, pack_aligment);

      gld_ApplyGammaRamp(scr, REAL_SCREENWIDTH * 3, REAL_SCREENWIDTH, REAL_SCREENHEIGHT);

      for (i=0; i<REAL_SCREENHEIGHT/2; i++)
      {
        memcpy(buffer, &scr[i*REAL_SCREENWIDTH*3], REAL_SCREENWIDTH*3);
        memcpy(&scr[i*REAL_SCREENWIDTH*3],
          &scr[(REAL_SCREENHEIGHT-(i+1))*REAL_SCREENWIDTH*3], REAL_SCREENWIDTH*3);
        memcpy(&scr[(REAL_SCREENHEIGHT-(i+1))*REAL_SCREENWIDTH*3], buffer, REAL_SCREENWIDTH*3);
      }
    }

  return scr;
}

GLvoid gld_Set2DMode(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(
    (GLdouble) 0,
    (GLdouble) REAL_SCREENWIDTH,
    (GLdouble) REAL_SCREENHEIGHT,
    (GLdouble) 0,
    (GLdouble) -1.0,
    (GLdouble) 1.0
  );
  glDisable(GL_DEPTH_TEST);
}

void gld_InitDrawScene(void)
{
  gld_ResetDrawInfo();
}

void gld_Finish(void)
{
  gld_Set2DMode();
  if (gl_finish)
  {
    glFinish();
  }
  SDL_GL_SwapBuffers();
}

GLuint flats_vbo_id = 0; // ID of VBO

vbo_xyz_uv_t *flats_vbo = NULL;

GLSeg *gl_segs=NULL;
GLSeg *gl_lines=NULL;

byte rendermarker=0;
byte *segrendered; // true if sector rendered (only here for malloc)
byte *linerendered[2]; // true if linedef rendered (only here for malloc)

float roll     = 0.0f;
float yaw      = 0.0f;
float inv_yaw  = 0.0f;
float pitch    = 0.0f;
float cos_inv_yaw, sin_inv_yaw;
float paperitems_pitch;
float cos_paperitems_pitch, sin_paperitems_pitch;

#define __glPi 3.14159265358979323846

void gld_Clear(void)
{
  int clearbits = 0;

#ifndef PRBOOM_DEBUG
  if (gl_clear)
#endif
    clearbits |= GL_COLOR_BUFFER_BIT;

  // flashing red HOM indicators
  if (flashing_hom)
  {
    clearbits |= GL_COLOR_BUFFER_BIT;
    glClearColor (gametic % 20 < 9 ? 1.0f : 0.0f, 0.0f, 0.0f, 1.0f);
  }

  if (gl_use_stencil)
    clearbits |= GL_STENCIL_BUFFER_BIT;

  if (!gl_ztrick)
    clearbits |= GL_DEPTH_BUFFER_BIT;

  if (clearbits)
    glClear(clearbits);

  if (gl_ztrick)
  {
    gl_ztrickframe = !gl_ztrickframe;
    if (gl_ztrickframe)
    {
      gldepthmin = 0.0f;
      gldepthmax = 0.49999f;
      glDepthFunc(GL_LEQUAL);
    }
    else
    {
      gldepthmin = 1.0f;
      gldepthmax = 0.5f;
      glDepthFunc(GL_GEQUAL);
    }
    glDepthRange(gldepthmin, gldepthmax);
  }
}

void gld_StartDrawScene(void)
{
  extern int screenblocks;

  gld_MultisamplingSet();

  if (gl_shared_texture_palette)
    glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
  gld_SetPalette(-1);

  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  glScissor(viewwindowx, SCREENHEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
  glEnable(GL_SCISSOR_TEST);
  // Player coordinates
  xCamera=-(float)viewx/MAP_SCALE;
  yCamera=(float)viewy/MAP_SCALE;
  zCamera=(float)viewz/MAP_SCALE;

  yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
  inv_yaw=180.0f-yaw;
  cos_inv_yaw = (float)cos(inv_yaw * M_PI / 180.f);
  sin_inv_yaw = (float)sin(inv_yaw * M_PI / 180.f);

  gl_spriteindex = 0;

  //e6y: fog in frame
  gl_use_fog = !gl_compatibility &&
    (gl_fog || gl_lightmode == gl_lightmode_fogbased) &&
    !frame_fixedcolormap && !boom_cm;

//e6y
  mlook_or_fov = GetMouseLook() || (render_fov != FOV90);
  if(!mlook_or_fov)
  {
    pitch = 0.0f;
    paperitems_pitch = 0.0f;

    skyXShift = -2.0f * ((yaw + 90.0f) / 90.0f);
    skyYShift = 200.0f / 319.5f;
  }
  else
  {
    float f = viewPitch * 2 + 50 / skyscale;
    f = BETWEEN(0, 127, f);
    skyXShift = -2.0f * ((yaw + 90.0f) / 90.0f / skyscale);
    skyYShift = f / 128.0f + 200.0f / 320.0f / skyscale;

    pitch = (float)(viewpitch>>ANGLETOFINESHIFT) * 360.0f / FINEANGLES;
    paperitems_pitch = ((pitch > 87.0f && pitch <= 90.0f) ? 87.0f : pitch);
    viewPitch = (pitch > 180 ? pitch - 360 : pitch);
  }
  cos_paperitems_pitch = (float)cos(paperitems_pitch * M_PI / 180.f);
  sin_paperitems_pitch = (float)sin(paperitems_pitch * M_PI / 180.f);
  gl_mask_sprite_threshold_f = (gl_sprite_blend ? (float)gl_mask_sprite_threshold / 100.0f : 0.5f);

  gld_InitFrameSky();
  
  invul_method = 0;
  if (players[displayplayer].fixedcolormap == 32)
  {
    if (gl_boom_colormaps && !gl_has_hires)
    {
      invul_method = INVUL_CM;
    }
    else
    {
      if (gl_version >= OPENGL_VERSION_1_3)
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
  motion_blur.enabled = gl_use_motionblur &&
    ((motion_blur.curr_speed_pow2 > motion_blur.minspeed_pow2) ||
    (abs(players[displayplayer].cmd.angleturn) > motion_blur.minangle));

  SceneInTexture = (gl_ext_framebuffer_object) &&
    ((invul_method & INVUL_BW) || (motion_blur.enabled));

  // Vortex: Set FBO object
  if (SceneInTexture)
  {
    GLEXT_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glSceneImageFBOTexID);
  }
#endif

  SetFrameTextureMode();

  gld_Clear();

  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(projMatrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(modelMatrix);

  rendermarker++;
  scene_has_overlapped_sprites = false;
  scene_has_wall_details = 0;
  scene_has_flat_details = 0;
}

//e6y
static void gld_ProcessExtraAlpha(void)
{
  if (extra_alpha>0.0f)
  {
    glDisable(GL_ALPHA_TEST);
    glColor4f(extra_red, extra_green, extra_blue, extra_alpha);
    gld_EnableTexture2D(GL_TEXTURE0_ARB, false);
    glBegin(GL_TRIANGLE_STRIP);
      glVertex2f( 0.0f, 0.0f);
      glVertex2f( 0.0f, (float)SCREENHEIGHT);
      glVertex2f( (float)SCREENWIDTH, 0.0f);
      glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
    gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
    glEnable(GL_ALPHA_TEST);
  }
}

//e6y
static void gld_InvertScene(void)
{
  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
  glColor4f(1,1,1,1);
  gld_EnableTexture2D(GL_TEXTURE0_ARB, false);
  glBegin(GL_TRIANGLE_STRIP);
    glVertex2f( 0.0f, 0.0f);
    glVertex2f( 0.0f, (float)SCREENHEIGHT);
    glVertex2f( (float)SCREENWIDTH, 0.0f);
    glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
  glEnd();
  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gld_EndDrawScene(void)
{
  glDisable(GL_POLYGON_SMOOTH);

  glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
  gl_EnableFog(false);
  gld_Set2DMode();

  if (viewangleoffset <= 1024<<ANGLETOFINESHIFT ||
    viewangleoffset >=-1024<<ANGLETOFINESHIFT)
  { // don't draw on side views
    glsl_SetActiveShader(sh_main);
    R_DrawPlayerSprites();
    glsl_SetActiveShader(NULL);
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

      glColor3f(0.3f, 0.3f, 0.4f);
    }
    else
    {
      glColor3f(1.0f, 1.0f, 1.0f);
    }

    //e6y: motion bloor effect for strafe50
    if (motion_blur.enabled)
    {
      extern int renderer_fps;
      static float motionblur_alpha = 1.0f;

      if (realframe)
      {
        motionblur_alpha = (float)((atan(-renderer_fps / motion_blur.att_a)) / motion_blur.att_b) + motion_blur.att_c;
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

    
    if (motion_blur.enabled)
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
  glDisable(GL_ALPHA_TEST);
  if (gl_shared_texture_palette)
    glDisable(GL_SHARED_TEXTURE_PALETTE_EXT);
}

static void gld_AddDrawWallItem(GLDrawItemType itemtype, void *itemdata)
{
  if (gl_blend_animations)
  {
    anim_t *anim;
    int currpic, nextpic;
    GLWall *wall = (GLWall*)itemdata;
    float oldalpha = wall->alpha;

    switch (itemtype)
    {
    case GLDIT_FWALL:
      anim = anim_flats[wall->gltexture->index - firstflat].anim;
      if (anim)
      {
        wall->alpha = 1.0f - ((float)tic_vars.frac + ((leveltime - 1) % anim->speed) * 65536.0f) / (65536.0f * anim->speed);
        gld_AddDrawItem(GLDIT_FAWALL, itemdata);

        currpic = wall->gltexture->index - firstflat - anim->basepic;
        nextpic = anim->basepic + (currpic + 1) % anim->numpics;
        wall->alpha = oldalpha;
        wall->gltexture = gld_RegisterFlat(nextpic, true);
      }
      break;
    case GLDIT_WALL:
    case GLDIT_MWALL:
      anim = anim_textures[wall->gltexture->index].anim;
      if (anim)
      {
        if (itemtype == GLDIT_WALL || itemtype == GLDIT_MWALL)
        {
          wall->alpha = 1.0f - ((float)tic_vars.frac + ((leveltime - 1) % anim->speed) * 65536.0f) / (65536.0f * anim->speed);
          gld_AddDrawItem(GLDIT_AWALL, itemdata);

          currpic = wall->gltexture->index - anim->basepic;
          nextpic = anim->basepic + (currpic + 1) % anim->numpics;
          wall->alpha = oldalpha;
          wall->gltexture = gld_RegisterTexture(nextpic, true, false);
        }
      }
      break;
    }
  }

  if (((GLWall*)itemdata)->gltexture->detail)
    scene_has_wall_details++;

  gld_AddDrawItem(itemtype, itemdata);
}

/*****************
 *               *
 * Walls         *
 *               *
 *****************/

static void gld_DrawWall(GLWall *wall)
{
  int has_detail;
  unsigned int flags;

  rendered_segs++;

  has_detail =
    scene_has_details &&
    gl_arb_multitexture &&
    (wall->flag < GLDWF_SKY) &&
    (wall->gltexture->detail) &&
    gld_IsDetailVisible(xCamera, yCamera, 
      wall->glseg->x1, wall->glseg->z1,
      wall->glseg->x2, wall->glseg->z2);

  // Do not repeat middle texture vertically
  // to avoid visual glitches for textures with holes
  if ((wall->flag == GLDWF_M2S) && (wall->flag < GLDWF_SKY))
    flags = GLTEXTURE_CLAMPY;
  else
    flags = 0;

  gld_BindTexture(wall->gltexture, flags);
  gld_BindDetailARB(wall->gltexture, has_detail);

  if (!wall->gltexture)
  {
    glColor4f(1.0f,0.0f,0.0f,1.0f);
  }

  if (has_detail)
  {
    gld_DrawWallWithDetail(wall);
  }
  else
  {
    if ((wall->flag == GLDWF_TOPFLUD) || (wall->flag == GLDWF_BOTFLUD))
    {
      gl_strip_coords_t c;

      gld_BindFlat(wall->gltexture, 0);

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
      glTexCoord2f(wall->ul,wall->vb);
      glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);

      // split left edge of wall
      if (!wall->glseg->fracleft)
        gld_SplitLeftEdge(wall, false);

      // upper left corner
      glTexCoord2f(wall->ul,wall->vt);
      glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);

      // upper right corner
      glTexCoord2f(wall->ur,wall->vt);
      glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);

      // split right edge of wall
      if (!wall->glseg->fracright)
        gld_SplitRightEdge(wall, false);

      // lower right corner
      glTexCoord2f(wall->ur,wall->vb);
      glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);

      glEnd();
    }
  }
}

#define LINE seg->linedef
#define CALC_Y_VALUES(w, lineheight, floor_height, ceiling_height)\
  (w).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;\
  (w).ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;\
  lineheight=((float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT)))

#define OU(w,seg) (((float)((seg)->sidedef->textureoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width)
#define OV(w,seg) (((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height)
#define OV_PEG(w,seg,v_offset) (OV((w),(seg))-(((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height))
#define URUL(w, seg, backseg, linelength)\
  if (backseg){\
    (w).ur=OU((w),(seg));\
    (w).ul=(w).ur+((linelength)/(float)(w).gltexture->buffer_width);\
  }else{\
    (w).ul=OU((w),(seg));\
    (w).ur=(w).ul+((linelength)/(float)(w).gltexture->buffer_width);\
  }

#define CALC_TEX_VALUES_TOP(w, seg, backseg, peg, linelength, lineheight)\
  (w).flag=GLDWF_TOP;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV((w),(seg))+((w).gltexture->scaleyfac);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).gltexture->buffer_height);\
  }

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, backseg, peg, linelength, lineheight)\
  (w).flag=GLDWF_M1S;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV((w),(seg))+((w).gltexture->scaleyfac);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).gltexture->buffer_height);\
  }

#define CALC_TEX_VALUES_BOTTOM(w, seg, backseg, peg, linelength, lineheight, v_offset)\
  (w).flag=GLDWF_BOT;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV_PEG((w),(seg),(v_offset))+((w).gltexture->scaleyfac);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).gltexture->buffer_height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).gltexture->buffer_height);\
  }

void gld_AddWall(seg_t *seg)
{
  GLWall wall;
  GLTexture *temptex;
  sector_t *frontsector;
  sector_t *backsector;
  sector_t ftempsec; // needed for R_FakeFlat
  sector_t btempsec; // needed for R_FakeFlat
  float lineheight, linelength;
  int rellight = 0;
  int backseg;

  int side = (seg->sidedef == &sides[seg->linedef->sidenum[0]] ? 0 : 1);
  if (linerendered[side][seg->linedef->iLineID] == rendermarker)
    return;
  linerendered[side][seg->linedef->iLineID] = rendermarker;
  linelength = lines[seg->linedef->iLineID].texel_length;
  wall.glseg=&gl_lines[seg->linedef->iLineID];
  backseg = seg->sidedef != &sides[seg->linedef->sidenum[0]];

  if (!seg->frontsector)
    return;
  frontsector=R_FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
  if (!frontsector)
    return;

  // e6y: fake contrast stuff
  // Original doom added/removed one light level ((1<<LIGHTSEGSHIFT) == 16) 
  // for walls exactly vertical/horizontal on the map
  if (fake_contrast)
  {
    rellight = seg->linedef->dx == 0 ? +gl_rellight : seg->linedef->dy==0 ? -gl_rellight : 0;
  }
  wall.light=gld_CalcLightLevel(frontsector->lightlevel+rellight+(extralight<<5));
  wall.fogdensity = gld_CalcFogDensity(frontsector,
    frontsector->lightlevel + (gl_lightmode == gl_lightmode_fogbased ? rellight : 0),
    GLDIT_WALL);
  wall.alpha=1.0f;
  wall.gltexture=NULL;
  wall.seg = seg; //e6y

  if (!seg->backsector) /* onesided */
  {
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=MAXCOORD;
      wall.ybottom=(float)frontsector->ceilingheight/MAP_SCALE;
      gld_AddSkyTexture(&wall, frontsector->sky, frontsector->sky, SKY_CEILING);
    }
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ytop=(float)frontsector->floorheight/MAP_SCALE;
      wall.ybottom=-MAXCOORD;
      gld_AddSkyTexture(&wall, frontsector->sky, frontsector->sky, SKY_FLOOR);
    }
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true, false);
    if (temptex && frontsector->ceilingheight > frontsector->floorheight)
    {
      wall.gltexture=temptex;
      CALC_Y_VALUES(wall, lineheight, frontsector->floorheight, frontsector->ceilingheight);
      CALC_TEX_VALUES_MIDDLE1S(
        wall, seg, backseg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
        linelength, lineheight
      );
      gld_AddDrawWallItem(GLDIT_WALL, &wall);
    }
  }
  else /* twosided */
  {
    sector_t *fs, *bs;
    int toptexture, midtexture, bottomtexture;
    fixed_t floor_height,ceiling_height;
    fixed_t max_floor, min_floor;
    fixed_t max_ceiling, min_ceiling;
    //fixed_t max_floor_tex, min_ceiling_tex;

    backsector=R_FakeFlat(seg->backsector, &btempsec, NULL, NULL, true); // for boom effects
    if (!backsector)
      return;

    if (frontsector->floorheight > backsector->floorheight)
    {
      max_floor = frontsector->floorheight;
      min_floor = backsector->floorheight;
    }
    else
    {
      max_floor = backsector->floorheight;
      min_floor = frontsector->floorheight;
    }

    if (frontsector->ceilingheight > backsector->ceilingheight)
    {
      max_ceiling = frontsector->ceilingheight;
      min_ceiling = backsector->ceilingheight;
    }
    else
    {
      max_ceiling = backsector->ceilingheight;
      min_ceiling = frontsector->ceilingheight;
    }

    //max_floor_tex = max_floor + seg->sidedef->rowoffset;
    //min_ceiling_tex = min_ceiling + seg->sidedef->rowoffset;

    if (backseg)
    {
      fs = backsector;
      bs = frontsector;
    }
    else
    {
      fs = frontsector;
      bs = backsector;
    }

    toptexture = texturetranslation[seg->sidedef->toptexture];
    midtexture = texturetranslation[seg->sidedef->midtexture];
    bottomtexture = texturetranslation[seg->sidedef->bottomtexture];

    /* toptexture */
    ceiling_height=frontsector->ceilingheight;
    floor_height=backsector->ceilingheight;
    if (frontsector->ceilingpic==skyflatnum)// || backsector->ceilingpic==skyflatnum)
    {
      wall.ytop= MAXCOORD;
      if (
          // e6y
          // There is no more HOM in the starting area on Memento Mori map29 and on map30.
          // Old code:
          // (backsector->ceilingheight==backsector->floorheight) &&
          // (backsector->ceilingpic==skyflatnum)
          (backsector->ceilingpic==skyflatnum) &&
          (backsector->ceilingheight<=backsector->floorheight)
         )
      {
        // e6y
        // There is no more visual glitches with sky on Icarus map14 sector 187
        // Old code: wall.ybottom=(float)backsector->floorheight/MAP_SCALE;
        wall.ybottom=((float)(backsector->floorheight +
          (seg->sidedef->rowoffset > 0 ? seg->sidedef->rowoffset : 0)))/MAP_SCALE;
        gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
      }
      else
      {
        if (bs->ceilingpic == skyflatnum && fs->ceilingpic != skyflatnum &&
          toptexture == NO_TEXTURE && midtexture == NO_TEXTURE)
        {
          wall.ybottom=(float)min_ceiling/MAP_SCALE;
          gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
        }
        else
        {
          if ((toptexture != NO_TEXTURE && midtexture == NO_TEXTURE) ||
            backsector->ceilingpic != skyflatnum ||
            backsector->ceilingheight <= frontsector->floorheight)
          {
            wall.ybottom=(float)max_ceiling/MAP_SCALE;
            gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
          }
        }
      }
    }
    if (floor_height<ceiling_height)
    {
      if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
      {
        temptex=gld_RegisterTexture(toptexture, true, false);
        if (!temptex && gl_use_stencil && backsector &&
          !(seg->linedef->r_flags & RF_ISOLATED) &&
          /*frontsector->ceilingpic != skyflatnum && */backsector->ceilingpic != skyflatnum &&
          !(backsector->flags & NULL_SECTOR))
        {
          wall.ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;
          wall.ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;
          if (wall.ybottom >= zCamera)
          {
            wall.flag=GLDWF_TOPFLUD;
            temptex=gld_RegisterFlat(flattranslation[seg->backsector->ceilingpic], true);
            if (temptex)
            {
              wall.gltexture=temptex;
              gld_AddDrawWallItem(GLDIT_FWALL, &wall);
            }
          }
        }
        else
        if (temptex)
        {
          wall.gltexture=temptex;
          CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
          CALC_TEX_VALUES_TOP(
            wall, seg, backseg, (LINE->flags & (/*e6y ML_DONTPEGBOTTOM | */ML_DONTPEGTOP))==0,
            linelength, lineheight
          );
          gld_AddDrawWallItem(GLDIT_WALL, &wall);
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
    temptex=gld_RegisterTexture(midtexture, true, true);
    if (temptex && seg->sidedef->midtexture != NO_TEXTURE && backsector->ceilingheight>frontsector->floorheight)
    {
      int top, bottom;
      wall.gltexture=temptex;

      if ( (LINE->flags & ML_DONTPEGBOTTOM) >0)
      {
        //floor_height=max_floor_tex;
        floor_height=MAX(seg->frontsector->floorheight, seg->backsector->floorheight)+(seg->sidedef->rowoffset);
        ceiling_height=floor_height+(wall.gltexture->realtexheight<<FRACBITS);
      }
      else
      {
        //ceiling_height=min_ceiling_tex;
        ceiling_height=MIN(seg->frontsector->ceilingheight, seg->backsector->ceilingheight)+(seg->sidedef->rowoffset);
        floor_height=ceiling_height-(wall.gltexture->realtexheight<<FRACBITS);
      }

      // Depending on missing textures and possible plane intersections
      // decide which planes to use for the polygon
      if (seg->frontsector != seg->backsector ||
        seg->frontsector->heightsec != -1)
      {
        sector_t *f, *b;

        f = (seg->frontsector->heightsec == -1 ? seg->frontsector : &ftempsec);
        b = (seg->backsector->heightsec == -1 ? seg->backsector : &btempsec);

        // Set up the top
        if (frontsector->ceilingpic != skyflatnum ||
            backsector->ceilingpic != skyflatnum)
        {
          if (toptexture == NO_TEXTURE)
            // texture is missing - use the higher plane
            top = MAX(f->ceilingheight, b->ceilingheight);
          else
            top = MIN(f->ceilingheight, b->ceilingheight);
        }
        else
          top = ceiling_height;

        // Set up the bottom
        if (frontsector->floorpic != skyflatnum ||
            backsector->floorpic != skyflatnum ||
            frontsector->floorheight != backsector->floorheight)
        {
          if (seg->sidedef->bottomtexture == NO_TEXTURE)
            // texture is missing - use the lower plane
            bottom = MIN(f->floorheight, b->floorheight);
          else
            // normal case - use the higher plane
            bottom = MAX(f->floorheight, b->floorheight);
        }
        else
        {
          bottom = floor_height;
        }

        //let's clip away some unnecessary parts of the polygon
        if (ceiling_height < top)
          top = ceiling_height;
        if (floor_height > bottom)
          bottom = floor_height;
      }
      else
      {
        // both sides of the line are in the same sector
        top = ceiling_height;
        bottom = floor_height;
      }

      if (top <= bottom)
        goto bottomtexture;

      wall.ytop = (float)top/(float)MAP_SCALE;
      wall.ybottom = (float)bottom/(float)MAP_SCALE;

      wall.flag = GLDWF_M2S;
      URUL(wall, seg, backseg, linelength);

      wall.vt = (float)((-top + ceiling_height) >> FRACBITS)/(float)wall.gltexture->realtexheight;
      wall.vb = (float)((-bottom + ceiling_height) >> FRACBITS)/(float)wall.gltexture->realtexheight;

      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.alpha=(float)tran_filter_pct/100.0f;
      gld_AddDrawWallItem((wall.alpha == 1.0f ? GLDIT_MWALL : GLDIT_TWALL), &wall);
      wall.alpha=1.0f;
    }
bottomtexture:
    /* bottomtexture */
    ceiling_height=backsector->floorheight;
    floor_height=frontsector->floorheight;
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ybottom=-MAXCOORD;
      if (
          (backsector->ceilingheight==backsector->floorheight) &&
          (backsector->floorpic==skyflatnum)
         )
      {
        wall.ytop=(float)backsector->floorheight/MAP_SCALE;
        gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_FLOOR);
      }
      else
      {
        if (bs->floorpic == skyflatnum &&// fs->floorpic != skyflatnum &&
          bottomtexture == NO_TEXTURE && midtexture == NO_TEXTURE)
        {
          wall.ytop=(float)max_floor/MAP_SCALE;
          gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
        }
        else
        {
          if ((bottomtexture != NO_TEXTURE && midtexture == NO_TEXTURE) ||
            backsector->floorpic != skyflatnum ||
            backsector->floorheight >= frontsector->ceilingheight)
          {
            wall.ytop=(float)min_floor/MAP_SCALE;
            gld_AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_FLOOR);
          }
        }
      }
    }
    if (floor_height<ceiling_height)
    {
      temptex=gld_RegisterTexture(bottomtexture, true, false);
      if (!temptex && gl_use_stencil && backsector &&
        !(seg->linedef->r_flags & RF_ISOLATED) &&
        /*frontsector->floorpic != skyflatnum && */backsector->floorpic != skyflatnum &&
        !(backsector->flags & NULL_SECTOR))
      {
        wall.ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;
        wall.ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;
        if (wall.ytop <= zCamera)
        {
          wall.flag = GLDWF_BOTFLUD;
          temptex=gld_RegisterFlat(flattranslation[seg->backsector->floorpic], true);
          if (temptex)
          {
            wall.gltexture=temptex;
            gld_AddDrawWallItem(GLDIT_FWALL, &wall);
          }
        }
      }
      else
      if (temptex)
      {
        wall.gltexture=temptex;
        CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
        CALC_TEX_VALUES_BOTTOM(
          wall, seg, backseg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
          linelength, lineheight,
          floor_height-frontsector->ceilingheight
        );
        gld_AddDrawWallItem(GLDIT_WALL, &wall);
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
#undef CALC_TEX_VALUES_BOTTOM
#undef ADDWALL

/*****************
 *               *
 * Flats         *
 *               *
 *****************/

static void gld_DrawFlat(GLFlat *flat)
{
  int loopnum; // current loop number
  GLLoopDef *currentloop; // the current loop
  dboolean has_detail;
  int has_offset;
  unsigned int flags;

  rendered_visplanes++;
  
  has_detail =
    scene_has_details &&
    gl_arb_multitexture &&
    flat->gltexture->detail;

  has_offset = (has_detail || (flat->flags & GLFLAT_HAVE_OFFSET));

  if ((sectorloops[flat->sectornum].flags & SECTOR_CLAMPXY) && (!has_detail) &&
      ((tex_filter[MIP_TEXTURE].mag_filter == GL_NEAREST) ||
       (flat->gltexture->flags & GLTEXTURE_HIRES)) &&
      !(flat->flags & GLFLAT_HAVE_OFFSET))
    flags = GLTEXTURE_CLAMPXY;
  else
    flags = 0;
  
  gld_BindFlat(flat->gltexture, flags);
  gld_StaticLightAlpha(flat->light, flat->alpha);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,flat->z,0.0f);
#endif

  if (has_offset)
  {
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(flat->uoffs, flat->voffs, 0.0f);
  }
  
  gld_BindDetailARB(flat->gltexture, has_detail);
  if (has_detail)
  {
    float w, h, dx, dy;
    detail_t *detail = flat->gltexture->detail;

    GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
    gld_StaticLightAlpha(flat->light, flat->alpha);
    
    glPushMatrix();

    w = flat->gltexture->detail_width;
    h = flat->gltexture->detail_height;
    dx = detail->offsetx;
    dy = detail->offsety;

    if ((flat->flags & GLFLAT_HAVE_OFFSET) || dx || dy)
    {
      glTranslatef(flat->uoffs * w + dx, flat->voffs * h + dy, 0.0f);
    }

    glScalef(w, h, 1.0f);
  }

  if (flat->sectornum>=0)
  {
    // go through all loops of this sector
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
    if (gl_use_display_lists)
    {
      int display_list = (has_detail ? flats_detail_display_list : flats_display_list);
      glCallList(display_list + flat->sectornum);
    }
    else
    {
      for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
      {
        // set the current loop
        currentloop=&sectorloops[flat->sectornum].loops[loopnum];
        glDrawArrays(currentloop->mode,currentloop->vertexindex,currentloop->vertexcount);
      }
    }
#else
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
        if (has_detail)
        {
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, (GLfloat*)&flats_vbo[vertexnum].u);
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, (GLfloat*)&flats_vbo[vertexnum].u);
        }
        else
        {
          glTexCoord2fv((GLfloat*)&flats_vbo[vertexnum].u);
        }
        // set vertex coordinate
        //glVertex3fv((GLfloat*)&flats_vbo[vertexnum].x);
        glVertex3f(flats_vbo[vertexnum].x, flat->z, flats_vbo[vertexnum].z);
      }
      // end of loop
      glEnd();
    }
#endif
  }

  //e6y
  if (has_detail)
  {
    glPopMatrix();
    GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
  }

  if (has_offset)
  {
    glPopMatrix();
  }

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
#endif
}

// gld_AddFlat
//
// This draws on flat for the sector "num"
// The ceiling boolean indicates if the flat is a floor(false) or a ceiling(true)

static void gld_AddFlat(int sectornum, dboolean ceiling, visplane_t *plane)
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
  flat.flags = (ceiling ? GLFLAT_CEILING : 0);

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
    flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel, GLDIT_FLOOR);
    // calculate texture offsets
    if (sector->floor_xoffs | sector->floor_yoffs)
    {
      flat.flags |= GLFLAT_HAVE_OFFSET;
      flat.uoffs=(float)sector->floor_xoffs/(float)(FRACUNIT*64);
      flat.voffs=(float)sector->floor_yoffs/(float)(FRACUNIT*64);
    }
    else
    {
      flat.uoffs=0.0f;
      flat.voffs=0.0f;
    }
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
    flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel, GLDIT_CEILING);
    // calculate texture offsets
    if (sector->ceiling_xoffs | sector->ceiling_yoffs)
    {
      flat.flags |= GLFLAT_HAVE_OFFSET;
      flat.uoffs=(float)sector->ceiling_xoffs/(float)(FRACUNIT*64);
      flat.voffs=(float)sector->ceiling_yoffs/(float)(FRACUNIT*64);
    }
    else
    {
      flat.uoffs=0.0f;
      flat.voffs=0.0f;
    }
  }

  // get height from plane
  flat.z=(float)plane->height/MAP_SCALE;

  if (gl_blend_animations)
  {
    anim_t *anim = anim_flats[flat.gltexture->index - firstflat].anim;
    if (anim)
    {
      int currpic, nextpic;

      flat.alpha = 1.0f - ((float)tic_vars.frac + ((leveltime - 1) % anim->speed) * 65536.0f) / (65536.0f * anim->speed);
      gld_AddDrawItem(((flat.flags & GLFLAT_CEILING) ? GLDIT_ACEILING : GLDIT_AFLOOR), &flat);

      currpic = flat.gltexture->index - firstflat - anim->basepic;
      nextpic = anim->basepic + (currpic + 1) % anim->numpics;
      flat.gltexture = gld_RegisterFlat(nextpic, true);
    }
  }

  flat.alpha = 1.0;

  if (flat.gltexture->detail)
    scene_has_flat_details++;

  gld_AddDrawItem(((flat.flags & GLFLAT_CEILING) ? GLDIT_CEILING : GLDIT_FLOOR), &flat);
}

void gld_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling)
{
  subsector_t *subsector;

  subsector = &subsectors[subsectornum];
  if (!subsector)
    return;

  // render the floor
  if (floor && floor->height < viewz)
    gld_AddFlat(subsector->sector->iSectorID, false, floor);
  // render the ceiling
  if (ceiling && ceiling->height > viewz)
    gld_AddFlat(subsector->sector->iSectorID, true, ceiling);
}

/*****************
 *               *
 * Sprites       *
 *               *
 *****************/

static void gld_DrawSprite(GLSprite *sprite)
{
  GLint blend_src, blend_dst;
  int restore = 0;

  rendered_vissprites++;

  gld_BindPatch(sprite->gltexture,sprite->cm);

  if (!(sprite->flags & MF_NO_DEPTH_TEST))
  {
    if(sprite->flags & MF_SHADOW)
    {
      glGetIntegerv(GL_BLEND_SRC, &blend_src);
      glGetIntegerv(GL_BLEND_DST, &blend_dst);
      glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
      //glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
      glAlphaFunc(GL_GEQUAL,0.1f);
      glColor4f(0.2f,0.2f,0.2f,0.33f);
      restore = 1;
    }
    else
    {
      if(sprite->flags & MF_TRANSLUCENT)
        gld_StaticLightAlpha(sprite->light,(float)tran_filter_pct/100.0f);
      else
        gld_StaticLight(sprite->light);
    }
  }

  if (!render_paperitems && !(sprite->flags & (MF_SOLID | MF_SPAWNCEILING)))
  {
    float x1, x2, x3, x4, z1, z2, z3, z4;
    float y1, y2, cy, ycenter, y1c, y2c;
    float y1z2_y, y2z2_y;

    ycenter = (float)fabs(sprite->y1 - sprite->y2) * 0.5f;
    y1c = sprite->y1 - ycenter;
    y2c = sprite->y2 - ycenter;
    cy = sprite->y + ycenter;

    y1z2_y = -(y1c * sin_paperitems_pitch);
    y2z2_y = -(y2c * sin_paperitems_pitch);

    x1 = +(sprite->x1 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    x2 = +(sprite->x2 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    x3 = +(sprite->x1 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;
    x4 = +(sprite->x2 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;

    y1 = +(y1c * cos_paperitems_pitch) + cy;
    y2 = +(y2c * cos_paperitems_pitch) + cy;

    z1 = -(sprite->x1 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    z2 = -(sprite->x2 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    z3 = -(sprite->x1 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;
    z4 = -(sprite->x2 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(sprite->ul, sprite->vt); glVertex3f(x1, y1, z1);
    glTexCoord2f(sprite->ur, sprite->vt); glVertex3f(x2, y1, z2);
    glTexCoord2f(sprite->ul, sprite->vb); glVertex3f(x3, y2, z3);
    glTexCoord2f(sprite->ur, sprite->vb); glVertex3f(x4, y2, z4);
    glEnd();
  }
  else
  {
    float x1, x2, y1, y2, z1, z2;

    x1 = +(sprite->x1 * cos_inv_yaw) + sprite->x;
    x2 = +(sprite->x2 * cos_inv_yaw) + sprite->x;

    y1 = sprite->y + sprite->y1;
    y2 = sprite->y + sprite->y2;

    z2 = -(sprite->x1 * sin_inv_yaw) + sprite->z;
    z1 = -(sprite->x2 * sin_inv_yaw) + sprite->z;

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(sprite->ul, sprite->vt); glVertex3f(x1, y1, z2);
    glTexCoord2f(sprite->ur, sprite->vt); glVertex3f(x2, y1, z1);
    glTexCoord2f(sprite->ul, sprite->vb); glVertex3f(x1, y2, z2);
    glTexCoord2f(sprite->ur, sprite->vb); glVertex3f(x2, y2, z1);
    glEnd();
  }

  if (restore)
  {
    glBlendFunc(blend_src, blend_dst);
    glAlphaFunc(GL_GEQUAL, gl_mask_sprite_threshold_f);
  }
}

static void gld_AddHealthBar(mobj_t* thing, GLSprite *sprite)
{
  if (((thing->flags & (MF_COUNTKILL | MF_CORPSE)) == MF_COUNTKILL) && (thing->health > 0))
  {
    GLHealthBar hbar;
    int health_percent = thing->health * 100 / thing->info->spawnhealth;

    hbar.cm = -1;
    if (health_percent <= health_bar_red)
      hbar.cm = CR_RED;
    else if (health_percent <= health_bar_yellow)
      hbar.cm = CR_YELLOW;
    else if (health_percent <= health_bar_green)
      hbar.cm = CR_GREEN;

    if (hbar.cm >= 0)
    {
      float sx2 = (float)thing->radius / 2.0f / MAP_SCALE;
      float sx1 = sx2 - (float)health_percent * (float)thing->radius / 100.0f / MAP_SCALE;
      float sx3 = -sx2;

      hbar.x1 = +(sx1 * cos_inv_yaw) + sprite->x;
      hbar.x2 = +(sx2 * cos_inv_yaw) + sprite->x;
      hbar.x3 = +(sx3 * cos_inv_yaw) + sprite->x;

      hbar.z1 = -(sx1 * sin_inv_yaw) + sprite->z;
      hbar.z2 = -(sx2 * sin_inv_yaw) + sprite->z;
      hbar.z3 = -(sx3 * sin_inv_yaw) + sprite->z;

      hbar.y = sprite->y + sprite->y1 + 2.0f / MAP_COEFF;

      gld_AddDrawItem(GLDIT_HBAR, &hbar);
    }
  }
}

static void gld_DrawHealthBars(void)
{
  int i, count;
  int cm = -1;

  count = gld_drawinfo.num_items[GLDIT_HBAR];
  if (count > 0)
  {
    gld_EnableTexture2D(GL_TEXTURE0_ARB, false);

    glBegin(GL_LINES);
    for (i = count - 1; i >= 0; i--)
    {
      GLHealthBar *hbar = gld_drawinfo.items[GLDIT_HBAR][i].item.hbar;
      if (hbar->cm != cm)
      {
        cm = hbar->cm;
        glColor4f(cm2RGB[cm][0], cm2RGB[cm][1], cm2RGB[cm][2], 1.0f);
      }

      glVertex3f(hbar->x1, hbar->y, hbar->z1);
      glVertex3f(hbar->x2, hbar->y, hbar->z2);
    }
    glEnd();

    if (health_bar_full_length)
    {
      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
      glBegin(GL_LINES);
      for (i = count - 1; i >= 0; i--)
      {
        GLHealthBar *hbar = gld_drawinfo.items[GLDIT_HBAR][i].item.hbar;

        glVertex3f(hbar->x1, hbar->y, hbar->z1);
        glVertex3f(hbar->x3, hbar->y, hbar->z3);
      }
      glEnd();
    }

    gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
  }
}

void gld_ProjectSprite(mobj_t* thing, int lightlevel)
{
  fixed_t   tx;
  spritedef_t   *sprdef;
  spriteframe_t *sprframe;
  int       lump;
  dboolean   flip;
  int heightsec;      // killough 3/27/98

  // transform the origin point
  //e6y
  fixed_t tr_x, tr_y;
  fixed_t fx, fy, fz;
  fixed_t gxt, gyt;
  fixed_t tz;

  GLSprite sprite;
  const rpatch_t* patch;

  int frustum_culling = HaveMouseLook() && gl_sprites_frustum_culling;
  int mlook = HaveMouseLook() || (render_fov > FOV90);

  if (!paused && movement_smooth)
  {
    fx = thing->PrevX + FixedMul (tic_vars.frac, thing->x - thing->PrevX);
    fy = thing->PrevY + FixedMul (tic_vars.frac, thing->y - thing->PrevY);
    fz = thing->PrevZ + FixedMul (tic_vars.frac, thing->z - thing->PrevZ);
  }
  else
  {
    fx = thing->x;
    fy = thing->y;
    fz = thing->z;
  }

  tr_x = fx - viewx;
  tr_y = fy - viewy;

  gxt = FixedMul(tr_x, viewcos);
  gyt = -FixedMul(tr_y, viewsin);

  tz = gxt - gyt;

  // thing is behind view plane?
  if (tz < r_near_clip_plane)
    return;

  gxt = -FixedMul(tr_x, viewsin);
  gyt = FixedMul(tr_y, viewcos);
  tx = -(gyt + gxt);

  //e6y
  if (!render_paperitems && mlook)
  {
    if (tz >= MINZ && (D_abs(tx) >> 5) > tz)
      return;
  }
  else
  {
    // too far off the side?
    if (D_abs(tx) > (tz << 2))
      return;
  }

  // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
  if ((unsigned) thing->sprite >= (unsigned)numsprites)
    I_Error ("R_ProjectSprite: Invalid sprite number %i", thing->sprite);
#endif

  sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
  if ((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
    I_Error ("R_ProjectSprite: Invalid sprite frame %i : %i", thing->sprite, thing->frame);
#endif

  if (!sprdef->spriteframes)
    I_Error("R_ProjectSprite: Missing spriteframes %i : %i", thing->sprite, thing->frame);

  sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
  {
    // choose a different rotation based on player view
    angle_t rot;
    angle_t ang = R_PointToAngle2(viewx, viewy, fx, fy);
    if (sprframe->lump[0] == sprframe->lump[1])
    {
      rot = (ang - thing->angle + (angle_t)(ANG45/2)*9) >> 28;
    }
    else
    {
      rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9 -
        (angle_t)(ANG180 / 16)) >> 28;
    }
    lump = sprframe->lump[rot];
    flip = (dboolean)(sprframe->flip & (1 << rot));
  }
  else
  {
    // use single rotation for all views
    lump = sprframe->lump[0];
    flip = (dboolean)(sprframe->flip & 1);
  }
  lump += firstspritelump;

  patch = R_CachePatchNum(lump);
  thing->patch_width = patch->width;

  // killough 4/9/98: clip things which are out of view due to height
  if(!mlook)
  {
    int x1, x2;
    fixed_t xscale = FixedDiv(projection, tz);
    /* calculate edges of the shape
    * cph 2003/08/1 - fraggle points out that this offset must be flipped
    * if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
    if (flip)
      tx -= (patch->width - patch->leftoffset) << FRACBITS;
    else
      tx -= patch->leftoffset << FRACBITS;

    x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
    tx += patch->width << FRACBITS;
    x2 = ((centerxfrac + FixedMul (tx, xscale) - FRACUNIT/2) >> FRACBITS);

    // off the side?
    if (x1 > viewwidth || x2 < 0)
      goto unlock_patch;
  }

  // killough 3/27/98: exclude things totally separated
  // from the viewer, by either water or fake ceilings
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings

  heightsec = thing->subsector->sector->heightsec;
  if (heightsec != -1)   // only clip things which are in special sectors
  {
    int phs = viewplayer->mo->subsector->sector->heightsec;
    fixed_t gzt = fz + (patch->topoffset << FRACBITS);
    if (phs != -1 && viewz < sectors[phs].floorheight ?
        fz >= sectors[heightsec].floorheight :
        gzt < sectors[heightsec].floorheight)
      goto unlock_patch;
    if (phs != -1 && viewz > sectors[phs].ceilingheight ?
        gzt < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
        fz >= sectors[heightsec].ceilingheight)
      goto unlock_patch;
  }

  //e6y FIXME!!!
  if (thing == players[displayplayer].mo && walkcamera.type != 2)
    goto unlock_patch;

  sprite.x =-(float)fx / MAP_SCALE;
  sprite.y = (float)fz / MAP_SCALE;
  sprite.z = (float)fy / MAP_SCALE;

  // Bring items up out of floor by configurable amount times .01 Mead 8/13/03
  sprite.y += gl_sprite_offset;

  sprite.x2 = (float)patch->leftoffset / MAP_COEFF;
  sprite.x1 = sprite.x2 - ((float)patch->width / MAP_COEFF);
  sprite.y1 = (float)patch->topoffset / MAP_COEFF;
  sprite.y2 = sprite.y1 - ((float)patch->height / MAP_COEFF);

  // e6y
  // if the sprite is below the floor, and it's not a hanger/floater/missile, 
  // and it's not a fully dead corpse, move it up
  if ((gl_spriteclip != spriteclip_const) &&
      (sprite.y2 < 0) && (sprite.y2 >= (float)(-gl_spriteclip_threshold_f)) &&
      !(thing->flags & (MF_SPAWNCEILING|MF_FLOAT|MF_MISSILE|MF_NOGRAVITY)) &&
      ((gl_spriteclip == spriteclip_always) || !((thing->flags & MF_CORPSE) && thing->tics == -1)))
  {
    sprite.y1 -= sprite.y2;
    sprite.y2 = 0.0f;
  }

  if (frustum_culling)
  {
    if (!gld_SphereInFrustum(
      sprite.x + cos_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
      sprite.y + (sprite.y1 + sprite.y2) / 2.0f,
      sprite.z - sin_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
      //1.5 == sqrt(2) + small delta for MF_FOREGROUND
      (float)(MAX(patch->width, patch->height)) / MAP_COEFF / 2.0f * 1.5f))
    {
      goto unlock_patch;
    }
  }

  sprite.scale = FixedDiv(projectiony, tz);;
  if ((thing->frame & FF_FULLBRIGHT) || show_alive)
  {
    sprite.fogdensity = 0.0f;
    sprite.light = 1.0f;
  }
  else
  {
    sprite.light = gld_CalcLightLevel(lightlevel+(extralight<<5));
    sprite.fogdensity = gld_CalcFogDensity(thing->subsector->sector, lightlevel, GLDIT_SPRITE);
  }
  sprite.cm = CR_LIMIT + (int)((thing->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  sprite.gltexture = gld_RegisterPatch(lump, sprite.cm);
  if (!sprite.gltexture)
    goto unlock_patch;
  sprite.flags = thing->flags;

  if (thing->flags & MF_FOREGROUND)
    scene_has_overlapped_sprites = true;

  sprite.index = gl_spriteindex++;
  sprite.xy = thing->x + (thing->y >> 16); 

  sprite.vt = 0.0f;
  sprite.vb = sprite.gltexture->scaleyfac;
  if (flip)
  {
    sprite.ul = 0.0f;
    sprite.ur = sprite.gltexture->scalexfac;
  }
  else
  {
    sprite.ul = sprite.gltexture->scalexfac;
    sprite.ur = 0.0f;
  }

  //e6y: support for transparent sprites
  if (sprite.flags & MF_NO_DEPTH_TEST)
  {
    gld_AddDrawItem(GLDIT_ASPRITE, &sprite);
  }
  else
  {
    gld_AddDrawItem((gl_sprite_blend || (sprite.flags & (MF_SHADOW | MF_TRANSLUCENT)) ? GLDIT_TSPRITE : GLDIT_SPRITE), &sprite);
    gld_ProcessThingShadow(thing);
  }

  if (health_bar)
  {
    gld_AddHealthBar(thing, &sprite);
  }

unlock_patch:
  R_UnlockPatchNum(lump);
}

/*****************
 *               *
 * Draw          *
 *               *
 *****************/

//e6y
void gld_ProcessWall(GLWall *wall)
{
  // e6y
  // The ultimate 'ATI sucks' fix: Some of ATIs graphics cards are so unprecise when 
  // rendering geometry that each and every border between polygons must be seamless, 
  // otherwise there are rendering artifacts worse than anything that could be seen 
  // on Geforce 2's! Made this a menu option because the speed impact is quite severe
  // and this special handling is not necessary on modern NVidia cards.
  seg_t *seg = wall->seg;

  wall->glseg->fracleft  = 0;
  wall->glseg->fracright = 0;

  gld_RecalcVertexHeights(seg->linedef->v1);
  gld_RecalcVertexHeights(seg->linedef->v2);

  gld_DrawWall(wall);
}

static int C_DECL dicmp_wall(const void *a, const void *b)
{
  GLTexture *tx1 = ((const GLDrawItem *)a)->item.wall->gltexture;
  GLTexture *tx2 = ((const GLDrawItem *)b)->item.wall->gltexture;
  return tx1 - tx2;
}
static int C_DECL dicmp_flat(const void *a, const void *b)
{
  GLTexture *tx1 = ((const GLDrawItem *)a)->item.flat->gltexture;
  GLTexture *tx2 = ((const GLDrawItem *)b)->item.flat->gltexture;
  return tx1 - tx2;
}
static int C_DECL dicmp_sprite(const void *a, const void *b)
{
  GLTexture *tx1 = ((const GLDrawItem *)a)->item.sprite->gltexture;
  GLTexture *tx2 = ((const GLDrawItem *)b)->item.sprite->gltexture;
  return tx1 - tx2;
}

static int C_DECL dicmp_sprite_scale(const void *a, const void *b)
{
  GLSprite *sprite1 = ((const GLDrawItem *)a)->item.sprite;
  GLSprite *sprite2 = ((const GLDrawItem *)b)->item.sprite;

  if (sprite1->scale != sprite2->scale)
  {
    return sprite2->scale - sprite1->scale;
  }
  else
  {
    return sprite1->gltexture - sprite2->gltexture;
  }
}

static void gld_DrawItemsSortByTexture(GLDrawItemType itemtype)
{
  typedef int(C_DECL *DICMP_ITEM)(const void *a, const void *b);

  static DICMP_ITEM itemfuncs[GLDIT_TYPES] = {
    0,
    dicmp_wall, dicmp_wall, dicmp_wall, dicmp_wall, dicmp_wall,
    dicmp_wall, dicmp_wall,
    dicmp_flat, dicmp_flat,
    dicmp_flat, dicmp_flat,
    dicmp_sprite, dicmp_sprite_scale, dicmp_sprite,
    0,
    0,
  };

  if (itemfuncs[itemtype] && gld_drawinfo.num_items[itemtype] > 1)
  {
    qsort(gld_drawinfo.items[itemtype], gld_drawinfo.num_items[itemtype],
      sizeof(gld_drawinfo.items[itemtype][0]), itemfuncs[itemtype]);
  }
}

static int no_overlapped_sprites;
static int C_DECL dicmp_sprite_by_pos(const void *a, const void *b)
{
  GLSprite *s1 = ((const GLDrawItem *)a)->item.sprite;
  GLSprite *s2 = ((const GLDrawItem *)b)->item.sprite;
  int res = s2->xy - s1->xy;
  no_overlapped_sprites = no_overlapped_sprites && res;
  return res;
}

static void gld_DrawItemsSort(GLDrawItemType itemtype, int (C_DECL *PtFuncCompare)(const void *, const void *))
{
  qsort(gld_drawinfo.items[itemtype], gld_drawinfo.num_items[itemtype],
    sizeof(gld_drawinfo.items[itemtype][0]), PtFuncCompare);
}

static void gld_DrawItemsSortSprites(GLDrawItemType itemtype)
{
  static const float delta = 0.2f / MAP_COEFF;
  int i;

  if (scene_has_overlapped_sprites && sprites_doom_order == DOOM_ORDER_STATIC)
  {
    for (i = 0; i < gld_drawinfo.num_items[itemtype]; i++)
    {
      GLSprite *sprite = gld_drawinfo.items[itemtype][i].item.sprite;
      if (sprite->flags & MF_FOREGROUND)
      {
        sprite->index = gl_spriteindex;
        sprite->x -= delta * sin_inv_yaw;
        sprite->z -= delta * cos_inv_yaw;
      }
    }
  }

  if (sprites_doom_order == DOOM_ORDER_DYNAMIC)
  {
    no_overlapped_sprites = true;
    gld_DrawItemsSort(itemtype, dicmp_sprite_by_pos); // back to front

    if (!no_overlapped_sprites)
    {
      // there are overlapped sprites
      int count = gld_drawinfo.num_items[itemtype];

      i = 1;
      while (i < count)
      {
        GLSprite *sprite1 = gld_drawinfo.items[itemtype][i - 1].item.sprite;
        GLSprite *sprite2 = gld_drawinfo.items[itemtype][i - 0].item.sprite;

        if (sprite1->xy == sprite2->xy)
        {
          GLSprite *sprite = (sprite1->index > sprite2->index ? sprite1 : sprite2);
          i++;
          while (i < count && gld_drawinfo.items[itemtype][i].item.sprite->xy == sprite1->xy)
          {
            if (gld_drawinfo.items[itemtype][i].item.sprite->index > sprite->index)
            {
              sprite = gld_drawinfo.items[itemtype][i].item.sprite;
            }
            i++;
          }

          // 'nearest'
          sprite->index = gl_spriteindex;
          sprite->x -= delta * sin_inv_yaw;
          sprite->z -= delta * cos_inv_yaw;
        }
        i++;
      }
    }
  }

  gld_DrawItemsSortByTexture(itemtype);
}

//
// projected walls
//
void gld_DrawProjectedWalls(GLDrawItemType itemtype)
{
  int i;

  if (gl_use_stencil && gld_drawinfo.num_items[itemtype] > 0)
  {
    // Push bleeding floor/ceiling textures back a little in the z-buffer
    // so they don't interfere with overlapping mid textures.
    glPolygonOffset(1.0f, 128.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_STENCIL_TEST);
    gld_DrawItemsSortByTexture(itemtype);
    for (i = gld_drawinfo.num_items[itemtype] - 1; i >= 0; i--)
    {
      GLWall *wall = gld_drawinfo.items[itemtype][i].item.wall;

      if (gl_use_fog)
      {
        // calculation of fog density for flooded walls
        if (wall->seg->backsector)
        {
          wall->fogdensity = gld_CalcFogDensity(wall->seg->frontsector,
            wall->seg->backsector->lightlevel, itemtype);
        }

        gld_SetFog(wall->fogdensity);
      }

      gld_ProcessWall(wall);
    }
    glDisable(GL_STENCIL_TEST);

    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

void gld_InitDisplayLists(void)
{
  int i;
  int loopnum; // current loop number
  GLLoopDef *currentloop;

  if (gl_use_display_lists)
  {
    flats_display_list_size = numsectors;
    flats_display_list = glGenLists(flats_display_list_size);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if (gl_ext_arb_vertex_buffer_object)
    {
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, flats_vbo_id);
    }
    glVertexPointer(3, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_u);

    for (i = 0; i < flats_display_list_size; i++)
    {
      glNewList(flats_display_list + i, GL_COMPILE);

      for (loopnum = 0; loopnum < sectorloops[i].loopcount; loopnum++)
      {
        // set the current loop
        currentloop = &sectorloops[i].loops[loopnum];
        glDrawArrays(currentloop->mode, currentloop->vertexindex, currentloop->vertexcount);
      }

      glEndList();
    }

    // duplicated display list for flats with enabled detail ARB
    if (details_count && gl_arb_multitexture)
    {
      flats_detail_display_list_size = numsectors;
      flats_detail_display_list = glGenLists(flats_detail_display_list_size);

      gld_EnableClientCoordArray(GL_TEXTURE1_ARB, true);

      for (i = 0; i < flats_display_list_size; i++)
      {
        glNewList(flats_detail_display_list + i, GL_COMPILE);

        for (loopnum = 0; loopnum < sectorloops[i].loopcount; loopnum++)
        {
          // set the current loop
          currentloop = &sectorloops[i].loops[loopnum];
          glDrawArrays(currentloop->mode, currentloop->vertexindex, currentloop->vertexcount);
        }

        glEndList();
      }

      gld_EnableClientCoordArray(GL_TEXTURE1_ARB, false);
    }

    if (gl_ext_arb_vertex_buffer_object)
    {
      // bind with 0, so, switch back to normal pointer operation
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }
}

void gld_CleanDisplayLists(void)
{
  if (gl_use_display_lists)
  {
    if (flats_display_list_size > 0)
    {
      glDeleteLists(flats_display_list, flats_display_list_size);
      flats_display_list = 0;
      flats_display_list_size = 0;
    }

    if (flats_detail_display_list_size > 0)
    {
      glDeleteLists(flats_detail_display_list, flats_detail_display_list_size);
      flats_detail_display_list = 0;
      flats_detail_display_list_size = 0;
    }
  }
}

void gld_DrawScene(player_t *player)
{
  int i;
  int skybox;

  //e6y: must call it twice for correct initialisation
  glEnable(GL_ALPHA_TEST);

  //e6y: the same with fog
  gl_EnableFog(true);
  gl_EnableFog(false);

  gld_EnableDetail(false);
  gld_InitFrameDetails();

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (!gl_use_display_lists)
  {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }
#endif

  //e6y: skybox
  skybox = 0;
  if (gl_drawskys != skytype_none)
  {
    skybox = gld_DrawBoxSkyBox();
  }

  if (!skybox)
  {
    if (gl_drawskys == skytype_skydome)
    {
      gld_DrawDomeSkyBox();
    }
    //e6y: 3d emulation of screen quad
    if (gl_drawskys == skytype_screen)
    {
      gld_DrawScreenSkybox();
    }
  }

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (!gl_use_display_lists)
  {
    if (gl_ext_arb_vertex_buffer_object)
    {
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, flats_vbo_id);
    }
    glVertexPointer(3, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_u);
  }
#endif

  glsl_SetActiveShader(sh_main);

  //
  // opaque stuff
  //

  glBlendFunc(GL_ONE, GL_ZERO);

  // solid geometry
  glDisable(GL_ALPHA_TEST);

  // enable backside removing
  glEnable(GL_CULL_FACE);

  // floors
  glCullFace(GL_FRONT);
  gld_DrawItemsSortByTexture(GLDIT_FLOOR);
  for (i = gld_drawinfo.num_items[GLDIT_FLOOR] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_FLOOR][i].item.flat->fogdensity);
    gld_DrawFlat(gld_drawinfo.items[GLDIT_FLOOR][i].item.flat);
  }

  // ceilings
  glCullFace(GL_BACK);
  gld_DrawItemsSortByTexture(GLDIT_CEILING);
  for (i = gld_drawinfo.num_items[GLDIT_CEILING] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_CEILING][i].item.flat->fogdensity);
    gld_DrawFlat(gld_drawinfo.items[GLDIT_CEILING][i].item.flat);
  }

  // disable backside removing
  glDisable(GL_CULL_FACE);

  // detail texture works only with flats and walls
  gld_EnableDetail(false);

  // top, bottom, one-sided walls
  gld_DrawItemsSortByTexture(GLDIT_WALL);
  for (i = gld_drawinfo.num_items[GLDIT_WALL] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_WALL][i].item.wall->fogdensity);
    gld_ProcessWall(gld_drawinfo.items[GLDIT_WALL][i].item.wall);
  }

  // masked geometry
  glEnable(GL_ALPHA_TEST);

  gld_DrawItemsSortByTexture(GLDIT_MWALL);

  if (!gl_arb_multitexture && render_usedetail && gl_use_stencil &&
      gld_drawinfo.num_items[GLDIT_MWALL] > 0)
  {
    // opaque mid walls without holes
    for (i = gld_drawinfo.num_items[GLDIT_MWALL] - 1; i >= 0; i--)
    {
      GLWall *wall = gld_drawinfo.items[GLDIT_MWALL][i].item.wall;
      if (!(wall->gltexture->flags & GLTEXTURE_HASHOLES))
      {
        gld_SetFog(wall->fogdensity);
        gld_ProcessWall(wall);
      }
    }

    // opaque mid walls with holes

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    for (i = gld_drawinfo.num_items[GLDIT_MWALL] - 1; i >= 0; i--)
    {
      GLWall *wall = gld_drawinfo.items[GLDIT_MWALL][i].item.wall;
      if (wall->gltexture->flags & GLTEXTURE_HASHOLES)
      {
        gld_SetFog(wall->fogdensity);
        gld_ProcessWall(wall);
      }
    }

    glStencilFunc(GL_EQUAL, 1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);

    // details for opaque mid walls with holes
    gld_DrawItemsSortByDetail(GLDIT_MWALL);
    for (i = gld_drawinfo.num_items[GLDIT_MWALL] - 1; i >= 0; i--)
    {
      GLWall *wall = gld_drawinfo.items[GLDIT_MWALL][i].item.wall;
      if (wall->gltexture->flags & GLTEXTURE_HASHOLES)
      {
        gld_SetFog(wall->fogdensity);
        gld_DrawWallDetail_NoARB(wall);
      }
    }

    //restoring
    SetFrameTextureMode();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_STENCIL_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
  }
  else
  {
    // opaque mid walls
    for (i = gld_drawinfo.num_items[GLDIT_MWALL] - 1; i >= 0; i--)
    {
      gld_SetFog(gld_drawinfo.items[GLDIT_MWALL][i].item.wall->fogdensity);
      gld_ProcessWall(gld_drawinfo.items[GLDIT_MWALL][i].item.wall);
    }
  }

  gl_EnableFog(false);
  gld_EnableDetail(false);

  // projected walls
  gld_DrawProjectedWalls(GLDIT_FWALL);

  gl_EnableFog(false);
  glEnable(GL_ALPHA_TEST);

  // normal sky (not a skybox)
  if (!skybox && (gl_drawskys == skytype_none || gl_drawskys == skytype_standard))
  {
    rendered_segs += gld_drawinfo.num_items[GLDIT_SWALL];
    // fake strips of sky
    glsl_SetActiveShader(NULL);
    gld_DrawStripsSky();
    glsl_SetActiveShader(sh_main);
  }

  // opaque sprites
  glAlphaFunc(GL_GEQUAL, gl_mask_sprite_threshold_f);
  gld_DrawItemsSortSprites(GLDIT_SPRITE);
  for (i = gld_drawinfo.num_items[GLDIT_SPRITE] - 1; i >= 0; i--)
  {
    gld_SetFog(gld_drawinfo.items[GLDIT_SPRITE][i].item.sprite->fogdensity);
    gld_DrawSprite(gld_drawinfo.items[GLDIT_SPRITE][i].item.sprite);
  }
  glAlphaFunc(GL_GEQUAL, 0.5f);

  // mode for viewing all the alive monsters
  if (show_alive)
  {
    const int period = 250;
    float color;
    int step = (SDL_GetTicks() % (period * 2)) + 1;
    if (step > period)
    {
      step = period * 2 - step;
    }
    color = 0.1f + 0.9f * (float)step / (float)period;

    R_AddAllAliveMonstersSprites();
    glDisable(GL_DEPTH_TEST);
    gld_DrawItemsSortByTexture(GLDIT_ASPRITE);
    glColor4f(1.0f, color, color, 1.0f);
    for (i = gld_drawinfo.num_items[GLDIT_ASPRITE] - 1; i >= 0; i--)
    {
      gld_DrawSprite(gld_drawinfo.items[GLDIT_ASPRITE][i].item.sprite);
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
  }

  if (health_bar)
  {
    glsl_SetActiveShader(NULL);
    gld_DrawHealthBars();
    glsl_SetActiveShader(sh_main);
  }

  //
  // transparent stuff
  //

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (gl_blend_animations)
  {
    // enable backside removing
    glEnable(GL_CULL_FACE);

    // animated floors
    glCullFace(GL_FRONT);
    gld_DrawItemsSortByTexture(GLDIT_AFLOOR);
    for (i = gld_drawinfo.num_items[GLDIT_AFLOOR] - 1; i >= 0; i--)
    {
      gld_SetFog(gld_drawinfo.items[GLDIT_AFLOOR][i].item.flat->fogdensity);
      gld_DrawFlat(gld_drawinfo.items[GLDIT_AFLOOR][i].item.flat);
    }

    glCullFace(GL_BACK);
    gld_DrawItemsSortByTexture(GLDIT_ACEILING);
    for (i = gld_drawinfo.num_items[GLDIT_ACEILING] - 1; i >= 0; i--)
    {
      gld_SetFog(gld_drawinfo.items[GLDIT_ACEILING][i].item.flat->fogdensity);
      gld_DrawFlat(gld_drawinfo.items[GLDIT_ACEILING][i].item.flat);
    }

    // disable backside removing
    glDisable(GL_CULL_FACE);
  }

  if (gl_blend_animations)
  {
    gld_DrawItemsSortByTexture(GLDIT_AWALL);
    for (i = gld_drawinfo.num_items[GLDIT_AWALL] - 1; i >= 0; i--)
    {
      gld_SetFog(gld_drawinfo.items[GLDIT_AWALL][i].item.wall->fogdensity);
      gld_ProcessWall(gld_drawinfo.items[GLDIT_AWALL][i].item.wall);
    }

    // projected animated walls
    gld_DrawProjectedWalls(GLDIT_FAWALL);
  }

  glsl_SetActiveShader(NULL);
  gld_RenderShadows();
  glsl_SetActiveShader(sh_main);

  if (gld_drawinfo.num_items[GLDIT_TWALL] > 0 || gld_drawinfo.num_items[GLDIT_TSPRITE] > 0)
  {
    if (gld_drawinfo.num_items[GLDIT_TWALL] > 0)
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
    }

    glEnable(GL_ALPHA_TEST);

    // transparent sprites
    // sorting is necessary only for transparent sprites.
    // from back to front
    if (gld_drawinfo.num_items[GLDIT_TSPRITE] > 0)
    {
      glAlphaFunc(GL_GEQUAL, gl_mask_sprite_threshold_f);
      glDepthMask(GL_FALSE);
      gld_DrawItemsSortSprites(GLDIT_TSPRITE);
      for (i = gld_drawinfo.num_items[GLDIT_TSPRITE] - 1; i >= 0; i--)
      {
        gld_SetFog(gld_drawinfo.items[GLDIT_TSPRITE][i].item.sprite->fogdensity);
        gld_DrawSprite(gld_drawinfo.items[GLDIT_TSPRITE][i].item.sprite);
      }
      glDepthMask(GL_TRUE);
    }

    // restoring
    glAlphaFunc(GL_GEQUAL, 0.5f);
  }

  // e6y: detail
  if (!gl_arb_multitexture && render_usedetail)
    gld_DrawDetail_NoARB();

  gld_EnableDetail(false);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (!gl_use_display_lists)
  {
    if (gl_ext_arb_vertex_buffer_object)
    {
      // bind with 0, so, switch back to normal pointer operation
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }
#endif

  glsl_SetActiveShader(NULL);
}
