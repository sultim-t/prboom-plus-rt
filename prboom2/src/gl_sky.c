/*
** gl_sky.cpp
**
** Draws the sky.  Loosely based on the JDoom sky and the ZDoomGL 0.66.2 sky.
**
**---------------------------------------------------------------------------
** Copyright 2003 Tim Stump
** Copyright 2005 Christoph Oelckers
** Copyright 2009 Andrey Budko
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
** 4. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
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

#include "gl_opengl.h"

#include <SDL.h>

#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "r_plane.h"
#include "r_sky.h"
#include "r_main.h"
#include "sc_man.h"
#include "m_misc.h"
#include "lprintf.h"

#include "e6y.h"

typedef struct
{
  int mode;
  int vertexcount;
  int vertexindex;
  int use_texture;
} GLSkyLoopDef;

typedef struct
{
  int id;
  int rows, columns;
  int loopcount;
  GLSkyLoopDef *loops;
  vbo_vertex_t *data;
} GLSkyVBO;

int gl_skymode;
int gl_drawskys;
// Sky stretching is rather pointless with the GL renderer
// now that it can handle all sky heights.
int gl_stretchsky = false;

static PalEntry_t *SkyColor;

SkyBoxParams_t SkyBox;
float y_offset_saved;

// skybox
box_skybox_t *BoxSkybox = NULL;
int BoxSkyboxCount = 0;

box_skybox_t *BoxSkybox_default;

void gld_InitSky(void)
{
  memset(&SkyBox, 0, sizeof(SkyBox));
  SkyBox.index = -1;
  y_offset_saved = 0;
}

void gld_InitFrameSky(void)
{
  SkyBox.type = SKY_NONE;
  SkyBox.wall.gltexture = NULL;
  SkyBox.x_scale = 0;
  SkyBox.y_scale = 0;
  SkyBox.x_offset = 0;
  SkyBox.y_offset = 0;
  SkyBox.side = NULL;
}

void gld_DrawFakeSkyStrips(void)
{
  int i;

  // This draws a valid z-buffer into the stencil's contents to ensure it
  // doesn't get overwritten by the level's geometry.

  // Because some of outdated hardware has no support for
  // glColorMask(0, 0, 0, 0) or something, 
  // I need to render fake strips of sky before dome with using
  // full clearing of color buffer (only in compatibility mode)

  if (!gl_compatibility)
  {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // no graphics
  }
  gld_EnableTexture2D(GL_TEXTURE0_ARB, false);

  for (i = gld_drawinfo.num_items[GLDIT_SWALL] - 1; i >= 0; i--)
  {
    GLWall* wall = gld_drawinfo.items[GLDIT_SWALL][i].item.wall;

    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);
    glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);
    glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);
    glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);
    glEnd();
  }

  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
  if (!gl_compatibility)
  {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }
  else
  {
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

void gld_GetScreenSkyScale(GLWall *wall, float *scale_x, float *scale_y)
{
  float sx, sy;

  sx = (wall->flag == GLDWF_SKYFLIP ? -128.0f : 128.0f);

  if (!mlook_or_fov)
  {
    sx = sx / (float)wall->gltexture->buffer_width;
    sy = 200.0f / 160.0f;//wall->gltexture->buffer_height;
  }
  else 
  {
    sx = sx * skyscale / (float)wall->gltexture->buffer_width;
    sy = 127.0f * skyscale / 160.0f;
  }

  *scale_x = sx;
  *scale_y = sy;
}

// Sky textures with a zero index should be forced
// See third episode of requiem.wad
void gld_AddSkyTexture(GLWall *wall, int sky1, int sky2, int skytype)
{
  side_t *s = NULL;
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
    s = *l->sidenum + sides;
    SkyBox.side = s;
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
        wall->skyyaw  = -2.0f*(((270.0f-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES)+90.0f)/90.0f/skyscale);
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
    SkyBox.type |= skytype;

    wall->gltexture->flags |= GLTEXTURE_SKY;

    gld_AddDrawItem(GLDIT_SWALL, wall);

    if (!SkyBox.wall.gltexture)
    {
      SkyBox.wall = *wall;

      switch (gl_drawskys)
      {
      case skytype_standard:
        gld_GetScreenSkyScale(wall, &SkyBox.x_scale, &SkyBox.y_scale);
        break;

      case skytype_screen:
        if (s)
        {
          SkyBox.x_offset = (float)s->textureoffset;
          SkyBox.y_offset = (float)s->rowoffset / (float)FRACUNIT;
        }
        break;
      case skytype_skydome:
        if (s)
        {
          SkyBox.x_offset = (float)s->textureoffset * 180.0f / (float)ANG180;
          SkyBox.y_offset = (float)s->rowoffset / (float)FRACUNIT;
        }
        break;
      }
    }
  }
}

void gld_DrawStripsSky(void)
{
  int i;
  float skyymid_multiplier;
  GLTexture *gltexture = NULL;

  if (gl_drawskys == skytype_standard)
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

    SetTextureMode(TM_OPAQUE);
  }

  gld_EnableDetail(false);
  glMatrixMode(GL_TEXTURE);

  skyymid_multiplier = 1.0f;
  if (tallscreen)
  {
    skyymid_multiplier = (float)ratio_multiplier / ratio_scale;
  }

  for (i = gld_drawinfo.num_items[GLDIT_SWALL] - 1; i >= 0; i--)
  {
    GLWall *wall = gld_drawinfo.items[GLDIT_SWALL][i].item.wall;

    gltexture = (gl_drawskys == skytype_none ? NULL : wall->gltexture);
    gld_BindTexture(gltexture, 0);

    if (!gltexture)
    {
      glColor4f(1.0f,0.0f,0.0f,1.0f);
    }

    if (gltexture)
    {
      float sx, sy;

      glPushMatrix();

      gld_GetScreenSkyScale(wall, &sx, &sy);
      glScalef(sx, sy * skyymid_multiplier, 1.0f);
      glTranslatef(wall->skyyaw, wall->skyymid / skyymid_multiplier, 0.0f);
    }

#if 0
    {
      float r = (float)(wall->seg->sidedef - sides) / (float)(numsides - 1); 
      float g = (float)wall->seg->linedef->iLineID / (float)(numlines - 1); 
      float b = (float)i / (float)(gld_drawinfo.num_items[GLDIT_SWALL] - 1);
      glColor4f(r, g, b, 1.0f);
    }
#endif

    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);
    glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);
    glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);
    glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);
    glEnd();

    if (gltexture)
    {
      glPopMatrix();
    }
  }

  glMatrixMode(GL_MODELVIEW);

  gld_DrawSkyCaps();

  if (gl_drawskys == skytype_standard)
  {
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_S);

    if (comp[comp_skymap] && (invul_method & INVUL_BW))
      glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_COMBINE);

    if (comp[comp_skymap] && gl_shared_texture_palette)
      glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

    SetFrameTextureMode();
  }
}

void gld_DrawSkyCaps(void)
{
  if (SkyBox.type && SkyBox.wall.gltexture)
  {             
    dboolean mlook = GetMouseLook();

    if (mlook)
    {
      gld_BindTexture(SkyBox.wall.gltexture, 0);

      glMatrixMode(GL_TEXTURE);
      glPushMatrix();

      glScalef(SkyBox.x_scale, SkyBox.y_scale, 1.0f);
      glTranslatef(SkyBox.wall.skyyaw, SkyBox.wall.skyymid, 0.0f);

      if (SkyBox.type & SKY_CEILING)
      {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-MAXCOORD,+MAXCOORD,+MAXCOORD);
        glVertex3f(+MAXCOORD,+MAXCOORD,+MAXCOORD);
        glVertex3f(-MAXCOORD,+MAXCOORD,-MAXCOORD);
        glVertex3f(+MAXCOORD,+MAXCOORD,-MAXCOORD);
        glEnd();
      }

      if (SkyBox.type & SKY_FLOOR)
      {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-MAXCOORD,-MAXCOORD,+MAXCOORD);
        glVertex3f(+MAXCOORD,-MAXCOORD,+MAXCOORD);
        glVertex3f(-MAXCOORD,-MAXCOORD,-MAXCOORD);
        glVertex3f(+MAXCOORD,-MAXCOORD,-MAXCOORD);
        glEnd();
      }

      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
    }
  }
}

//===========================================================================
//
// averageColor
//  input is RGBA8 pixel format.
//	The resulting RGB color can be scaled uniformly so that the highest 
//	component becomes one.
//
//===========================================================================
#define APART(c)			(((c)>>24)&0xff)
#define RPART(c)			(((c)>>16)&0xff)
#define GPART(c)			(((c)>>8)&0xff)
#define BPART(c)			((c)&0xff)

void averageColor(PalEntry_t * PalEntry, const unsigned int *data, int size, fixed_t maxout_factor)
{
  int i;
  int maxv;
  unsigned int r, g, b;
  
  // First clear them.
  r = g = b = 0;
  if (size == 0) 
  {
    PalEntry->r = 255;
    PalEntry->g = 255;
    PalEntry->b = 255;
    return;
  }
  for(i = 0; i < size; i++)
  {
    r += BPART(data[i]);
    g += GPART(data[i]);
    b += RPART(data[i]);
  }

  r = r / size;
  g = g / size;
  b = b / size;

  maxv=MAX(MAX(r,g),b);

  if(maxv && maxout_factor)
  {
    maxout_factor = FixedMul(maxout_factor, 255);
    r = r * maxout_factor / maxv;
    g = g * maxout_factor / maxv;
    b = b * maxout_factor / maxv;
  }

  PalEntry->r = r;
  PalEntry->g = g;
  PalEntry->b = b;
  return;
}

// It is an alternative way of drawing the sky (gl_drawskys == skytype_screen)
// This method make sense only for old hardware which have no support for GL_TEXTURE_GEN_*
// Voodoo as example
void gld_DrawScreenSkybox(void)
{
  if (SkyBox.wall.gltexture)
  {
    #define WRAPANGLE (ANGLE_MAX/4)

    float fU1, fU2, fV1, fV2;
    GLWall *wall = &SkyBox.wall;
    angle_t angle;
    int i, k;
    float w;

    if (!gl_compatibility)
    {
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // no graphics
    }
    gld_EnableTexture2D(GL_TEXTURE0_ARB, false);

    for (i = gld_drawinfo.num_items[GLDIT_SWALL] - 1; i >= 0; i--)
    {
      GLWall* wall = gld_drawinfo.items[GLDIT_SWALL][i].item.wall;

      glBegin(GL_TRIANGLE_STRIP);
      glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);
      glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);
      glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);
      glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);
      glEnd();
    }

    gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
    if (!gl_compatibility)
    {
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    else
    {
      glClear(GL_COLOR_BUFFER_BIT);
    }

    if (!mlook_or_fov)
    {
      fV1 = SkyBox.y_offset / 127.0f;
      fV2 = fV1 + 320.0f / 200.0f;
    }
    else
    {
      float f = viewPitch * 2 + 40 / skyscale;
      f = BETWEEN(0, 127, f);
      fV1 = (f + SkyBox.y_offset) / 127.0f * skyscale;
      fV2 = fV1 + 1.0f;
    }

    k = MAX(wall->gltexture->buffer_width, 256) / 256;
    angle = ((viewangle - ANG45) / k) % WRAPANGLE;

    if (wall->flag == GLDWF_SKYFLIP)
    {
      fU1 = -((float)angle + SkyBox.x_offset) / (WRAPANGLE - 1);
      fU2 = fU1 + 1.0f / k;
    }
    else
    {
      fU2 = ((float)angle + SkyBox.x_offset) / (WRAPANGLE - 1);
      fU1 = fU2 + 1.0f / k;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);

    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gld_BindTexture(wall->gltexture, 0);
    w = 160.0f * SCREENWIDTH / WIDE_SCREENWIDTH;
    glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(fU1, fV1); glVertex3f(-w, +100.5f, -screen_skybox_zplane);
      glTexCoord2f(fU1, fV2); glVertex3f(-w, -100.5f, -screen_skybox_zplane);
      glTexCoord2f(fU2, fV1); glVertex3f(+w, +100.5f, -screen_skybox_zplane);
      glTexCoord2f(fU2, fV2); glVertex3f(+w, -100.5f, -screen_skybox_zplane);
    glEnd();

    glPopMatrix();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
  }
}

// The texture offset to be applied to the texture coordinates in SkyVertex().
static int rows, columns;	
static dboolean yflip;
static int texw;
static float yMult, yAdd;
static dboolean foglayer;
static float delta = 0.0f;

int gl_sky_detail = 16;

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void gld_GetSkyCapColors(void)
{
  int color, width, height;
  int frame_fixedcolormap_saved;
  unsigned char *buffer = NULL;
  const unsigned char *playpal = V_GetPlaypal();
  const lighttable_t *colormap;
  const lighttable_t *fixedcolormap_saved;
  PalEntry_t *ceiling_rgb = &SkyBox.CeilingSkyColor[0];
  PalEntry_t *floor_rgb = &SkyBox.FloorSkyColor[0];
  
  // saving current colormap
  fixedcolormap_saved = fixedcolormap;
  frame_fixedcolormap_saved = frame_fixedcolormap;

  fixedcolormap = fullcolormap;
  frame_fixedcolormap = 0;

  gld_BindTexture(SkyBox.wall.gltexture, 0);

  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

  buffer = malloc(width * height * 4);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

  averageColor(ceiling_rgb, (unsigned int*)buffer, width * MIN(30, height), 0);

  if (height > 30)
  {
    averageColor(floor_rgb, 
      ((unsigned int*)buffer) + (height - 30) * width, width * 30, 0);
  }
  else
  {
    *floor_rgb = *ceiling_rgb;
  }

  colormap = fullcolormap + INVERSECOLORMAP * 256 * sizeof(lighttable_t);

  color = V_BestColor(playpal, ceiling_rgb->r, ceiling_rgb->g, ceiling_rgb->b);
  SkyBox.CeilingSkyColor[1].r = playpal[colormap[color] * 3 + 0];
  SkyBox.CeilingSkyColor[1].g = playpal[colormap[color] * 3 + 1];
  SkyBox.CeilingSkyColor[1].b = playpal[colormap[color] * 3 + 2];

  color = V_BestColor(playpal, floor_rgb->r, floor_rgb->g, floor_rgb->b);
  SkyBox.FloorSkyColor[1].r = playpal[colormap[color] * 3 + 0];
  SkyBox.FloorSkyColor[1].g = playpal[colormap[color] * 3 + 1];
  SkyBox.FloorSkyColor[1].b = playpal[colormap[color] * 3 + 2];

  // restorin current colormap
  fixedcolormap = fixedcolormap_saved;
  frame_fixedcolormap = frame_fixedcolormap_saved;

  free(buffer);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

static void SkyVertex(vbo_vertex_t *vbo, int r, int c)
{
  static fixed_t scale = 10000 << FRACBITS;
  static angle_t maxSideAngle = ANG180 / 3;

  angle_t topAngle= (angle_t)(c / (float)columns * ANGLE_MAX);
  angle_t sideAngle = maxSideAngle * (rows - r) / rows;
  fixed_t height = finesine[sideAngle>>ANGLETOFINESHIFT];
  fixed_t realRadius = FixedMul(scale, finecosine[sideAngle>>ANGLETOFINESHIFT]);
  fixed_t x = FixedMul(realRadius, finecosine[topAngle>>ANGLETOFINESHIFT]);
  fixed_t y = (!yflip) ? FixedMul(scale, height) : FixedMul(scale, height) * -1;
  fixed_t z = FixedMul(realRadius, finesine[topAngle>>ANGLETOFINESHIFT]);
  float timesRepeat;

  timesRepeat = (short)(4 * (256.0f / texw));
  if (timesRepeat == 0.0f)
    timesRepeat = 1.0f;

  if (!foglayer)
  {
    vbo->r = 255;
    vbo->g = 255;
    vbo->b = 255;
    vbo->a = (r == 0 ? 0 : 255);
    
    // And the texture coordinates.
    if(!yflip)	// Flipped Y is for the lower hemisphere.
    {
      vbo->u = (-timesRepeat * c / (float)columns) ;
      vbo->v = (r / (float)rows) * 1.f * yMult + yAdd;
    }
    else
    {
      vbo->u = (-timesRepeat * c / (float)columns) ;
      vbo->v = ((rows-r)/(float)rows) * 1.f * yMult + yAdd;
    }

    if (SkyBox.wall.flag == GLDWF_SKYFLIP)
      vbo->u = -vbo->u;
  }

  if (r != 4)
  {
    y += FRACUNIT * 300;
  }

  // And finally the vertex.
  vbo->x =-(float)x/(float)MAP_SCALE;	// Doom mirrors the sky vertically!
  vbo->y = (float)y/(float)MAP_SCALE + delta;
  vbo->z = (float)z/(float)MAP_SCALE;
}

GLSkyVBO sky_vbo[2];

static void gld_BuildSky(int row_count, int col_count, SkyBoxParams_t *sky, int cm)
{
  int texh, c, r;
  vbo_vertex_t *vertex_p;
  int vertex_count = 2 * row_count * (col_count * 2 + 2) + col_count * 2;
  int vbo_idx = (cm == INVERSECOLORMAP ? 1 : 0);
  GLSkyVBO *vbo = &sky_vbo[vbo_idx];

  if ((vbo->columns != col_count) || (vbo->rows != row_count))
  {
    free(vbo->loops);
    free(vbo->data);
    memset(vbo, 0, sizeof(vbo[0]));
  }

  if (!vbo->data)
  {
    memset(vbo, 0, sizeof(vbo[0]));
    vbo->loops = malloc((row_count * 2 + 2) * sizeof(vbo->loops[0]));
    // create vertex array
    vbo->data = malloc(vertex_count * sizeof(vbo->data[0]));
  }

  vbo->columns = col_count;
  vbo->rows = row_count;

  texh = sky->wall.gltexture->buffer_height;
  if (texh > 190 && gl_stretchsky)
    texh = 190;
  texw = sky->wall.gltexture->buffer_width;

  vertex_p = &vbo->data[0];
  vbo->loopcount = 0;
  for (yflip = 0; yflip < 2; yflip++)
  {
    vbo->loops[vbo->loopcount].mode = GL_TRIANGLE_FAN;
    vbo->loops[vbo->loopcount].vertexindex = vertex_p - &vbo->data[0];
    vbo->loops[vbo->loopcount].vertexcount = col_count;
    vbo->loops[vbo->loopcount].use_texture = false;
    vbo->loopcount++;

    yAdd = sky->y_offset / texh;
    yMult = (texh <= 180 ? 1.0f : 180.0f / texh);
    if (yflip == 0)
    {
      SkyColor = &sky->CeilingSkyColor[vbo_idx];
    }
    else
    {
      SkyColor = &sky->FloorSkyColor[vbo_idx];
      if (texh <= 180) yMult = 1.0f; else yAdd += 180.0f / texh;
    }

    delta = 0.0f;
    foglayer = true;
    for(c = 0; c < col_count; c++)
    {
      SkyVertex(vertex_p, 1, c);
      vertex_p->r = SkyColor->r;
      vertex_p->g = SkyColor->g;
      vertex_p->b = SkyColor->b;
      vertex_p->a = 255;
      vertex_p++;
    }
    foglayer = false;

    delta = (yflip ? 5.0f : -5.0f) / MAP_COEFF;

    for(r = 0; r < row_count; r++)
    {
      vbo->loops[vbo->loopcount].mode = GL_TRIANGLE_STRIP;
      vbo->loops[vbo->loopcount].vertexindex = vertex_p - &vbo->data[0];
      vbo->loops[vbo->loopcount].vertexcount = 2 * col_count + 2;
      vbo->loops[vbo->loopcount].use_texture = true;
      vbo->loopcount++;

      for(c = 0; c <= col_count; c++)
      {
        SkyVertex(vertex_p++, r + (yflip ? 1 : 0), (c ? c : 0));
        SkyVertex(vertex_p++, r + (yflip ? 0 : 1), (c ? c : 0));
      }
    }
  }
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

static void RenderDome(SkyBoxParams_t *sky)
{
  int i, j;
  int vbosize;
  GLSkyVBO *vbo;

  if (!sky || !sky->wall.gltexture)
    return;

  if (invul_method == INVUL_CM && frame_fixedcolormap == INVERSECOLORMAP)
    vbo = &sky_vbo[1]; 
  else
    vbo = &sky_vbo[0]; 
  

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  // be sure the second ARB is not enabled
  gld_EnableDetail(false);
#endif

  glRotatef(-180.0f + sky->x_offset, 0.f, 1.f, 0.f);

  rows = 4;
  columns = 4 * gl_sky_detail;

  vbosize = 2 * rows * (columns * 2 + 2) + columns * 2;

  if (sky->y_offset != y_offset_saved ||
      sky->wall.gltexture->index != sky->index)
  {
    y_offset_saved = sky->y_offset;
    
    if (sky->wall.gltexture->index != sky->index)
    {
      sky->index = sky->wall.gltexture->index;
      gld_GetSkyCapColors();
    }

    gld_BuildSky(rows, columns, sky, 0);
    gld_BuildSky(rows, columns, sky, INVERSECOLORMAP);

#ifdef USE_VBO
    if (gl_ext_arb_vertex_buffer_object)
    {
      if (vbo->id)
      {
        // delete VBO when already exists
        GLEXT_glDeleteBuffersARB(1, &vbo->id);
      }
      // generate a new VBO and get the associated ID
      GLEXT_glGenBuffersARB(1, &vbo->id);
      // bind VBO in order to use
      GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, vbo->id);
      // upload data to VBO
      GLEXT_glBufferDataARB(GL_ARRAY_BUFFER,
        vbosize * sizeof(vbo->data[0]),
        vbo->data, GL_STATIC_DRAW_ARB);
    }
#endif
  }

  gld_BindTexture(SkyBox.wall.gltexture, 0);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (gl_ext_arb_vertex_buffer_object)
  {
    // bind VBO in order to use
    GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, vbo->id);
  }

  // activate and specify pointers to arrays
  glVertexPointer(3, GL_FLOAT, sizeof(vbo->data[0]), sky_vbo_x);
  glTexCoordPointer(2, GL_FLOAT, sizeof(vbo->data[0]), sky_vbo_u);
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vbo->data[0]), sky_vbo_r);

  // activate vertex array, texture coord array and color arrays
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
#endif

  if (!gl_stretchsky)
  {
    int texh = sky->wall.gltexture->buffer_height;

    if (texh <= 180)
    {
      glScalef(1.0f, (float)texh / 230.0f, 1.0f);
    }
    else
    {
      if (texh > 190)
        glScalef(1.0f, 230.0f / 240.0f, 1.0f);
    }
  }

  for(j = (HaveMouseLook() || !gl_stretchsky ? 0 : 1); j < 2; j++)
  {
    gld_EnableTexture2D(GL_TEXTURE0_ARB, j != 0);

    for(i = 0; i < vbo->loopcount; i++)
    {
      GLSkyLoopDef *loop = &vbo->loops[i];

      if (j == 0 ? loop->use_texture : !loop->use_texture)
        continue;

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
      glDrawArrays(loop->mode, loop->vertexindex, loop->vertexcount);
#else
      {
        int k;
        glBegin(loop->mode);
        for (k = loop->vertexindex; k < (loop->vertexindex + loop->vertexcount); k++)
        {
          vbo_vertex_t *v = &vbo->data[k];
          if (loop->use_texture)
          {
            glTexCoord2fv((GLfloat*)&v->u);
          }
          glColor4ubv((GLubyte*)&v->r);
          glVertex3fv((GLfloat*)&v->x);
        }
        glEnd();
      }
#endif
    }
  }

  glScalef(1.0f, 1.0f, 1.0f);

  // current color is undefined after glDrawArrays
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (gl_ext_arb_vertex_buffer_object)
  {
    // bind with 0, so, switch back to normal pointer operation
    GLEXT_glBindBufferARB(GL_ARRAY_BUFFER, 0);
  }
  // deactivate color array
  glDisableClientState(GL_COLOR_ARRAY);
#endif
}

void gld_DrawDomeSkyBox(void)
{
  if (SkyBox.wall.gltexture)
  {
    GLint shading_mode = GL_FLAT;

    gld_DrawFakeSkyStrips();

    glGetIntegerv(GL_SHADE_MODEL, &shading_mode);
    glShadeModel(GL_SMOOTH);

    glDepthMask(false);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    SetTextureMode(TM_OPAQUE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(roll,  0.0f, 0.0f, 1.0f);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw,   0.0f, 1.0f, 0.0f);
    glScalef(-2.0f, 2.0f, 2.0f);
    glTranslatef(0.f, -1250.0f / MAP_COEFF, 0.f);

    RenderDome(&SkyBox);

    glPopMatrix();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);

    SetFrameTextureMode();

    glShadeModel(shading_mode);
  }
}

int R_BoxSkyboxNumForName(const char *name)
{
  int i;

  for (i = 0; i < BoxSkyboxCount; i++)
  {
    if (!strcasecmp(BoxSkybox[i].name, name))
    {
      return i;
    }
  }

  return -1;
}

void R_SetBoxSkybox(int texture)
{
  int i;

  BoxSkybox_default = NULL;

  for (i = 0; i < BoxSkyboxCount; i++)
  {
    if (R_CheckTextureNumForName(BoxSkybox[i].name) == texture)
    {
      BoxSkybox_default = &BoxSkybox[i];
      return;
    }
  }
}

box_skybox_t* R_GetBoxSkybox(int index)
{
  if (index >= 0 && index < BoxSkyboxCount)
    return &BoxSkybox[index];
  else
    return NULL;
}

void gld_ParseSkybox(void)
{
  if (SC_GetString())
  {
    box_skybox_t sb;
    memset(&sb, 0, sizeof(sb));

    strncpy(sb.name, sc_String, 8);
    sb.name[8] = 0;
    M_Strupr(sb.name);

    while (SC_Check())
    {
      SC_GetString();
      if (SC_Compare("fliptop"))
      {
        sb.fliptop = true;
      }
    }

    if (SC_GetString() && SC_Compare("{"))
    {
      int facecount = 0;

      while (SC_GetString() && !SC_Compare("}"))
      {
        if (facecount < 6) 
        {
          strcpy(sb.faces[facecount], sc_String);
        }
        facecount++;
      }

      if (SC_Compare("}") && (facecount == 3 || facecount == 6))
      {
        int i;
        int ok = true;

        for (i = 0; i < facecount; i++)
        {
          if (R_CheckTextureNumForName(sb.faces[i]) == -1)
          {
            ok = false;
            break;
          }
        }

        if (ok)
        {
          BoxSkyboxCount++;
          BoxSkybox = realloc(BoxSkybox, BoxSkyboxCount * sizeof(BoxSkybox[0]));
          memcpy(&BoxSkybox[BoxSkyboxCount - 1], &sb, sizeof(sb));
        }
      }
    }
  }

  R_SetBoxSkybox(skytexture);
}

int gld_BindFace(box_skybox_t *sb, int index)
{
  int lump;
  GLTexture *gltexture;
  char *name = sb->faces[index];

#if 0
  lump = W_CheckNumForName(name);
  if (lump != -1)
  {
    gltexture = gld_RegisterPatch(lump, CR_DEFAULT);
    gltexture->wrap_mode = GLEXT_CLAMP_TO_EDGE;
    gld_BindPatch(gltexture, CR_DEFAULT);
    return true;
  }

  //lump = R_FlatNumForName(name);
  lump = (W_CheckNumForName)(name, ns_flats);
  if (lump != -1)
  {
    lump -= firstflat;
    gltexture = gld_RegisterFlat(lump, true);
    gltexture->wrap_mode = GLEXT_CLAMP_TO_EDGE;
    gld_BindFlat(gltexture);
    return true;
  }
#endif

  lump = R_CheckTextureNumForName(name);
  if (lump != -1)
  {
    gltexture = gld_RegisterTexture(lump, false, false);
    gld_BindTexture(gltexture, GLTEXTURE_CLAMPXY);
    return true;
  }

  return false;
}

int gld_DrawBoxSkyBox(void)
{
  int faces;
  box_skybox_t *sb;

  if (BoxSkyboxCount == 0)
    return false;

  if (SkyBox.side)
  {
    sb = R_GetBoxSkybox(SkyBox.side->skybox_index);
  }
  else
  {
    sb = BoxSkybox_default;
  }

  if (!sb)
  {
    return false;
  }

  gld_DrawFakeSkyStrips();

  glDepthMask(false);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_ALPHA_TEST);

  SetTextureMode(TM_OPAQUE);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glLoadIdentity();

  glRotatef(roll,  0.0f, 0.0f, 1.0f);
  glRotatef(pitch, 1.0f, 0.0f, 0.0f);
  glRotatef(yaw,   0.0f, 1.0f, 0.0f);
  glScalef(-2.0f, 2.0f, 2.0f);

  if (SkyBox.side)
  {
    float xoffset = (float)SkyBox.side->textureoffset * 180.0f / (float)ANG180;
    glRotatef(-180.0f + xoffset, 0.0f, 1.0f, 0.0f);
  }

  if (sb->faces[5][0]) 
  {
    faces = 4;

    // north
    gld_BindFace(sb, 0);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(+MAXCOORD, MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-MAXCOORD, -MAXCOORD, -MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, -MAXCOORD);
    glEnd();

    // east
    gld_BindFace(sb, 1);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-MAXCOORD, -MAXCOORD, +MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-MAXCOORD, -MAXCOORD, -MAXCOORD);
    glEnd();

    // south
    gld_BindFace(sb, 2);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, +MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-MAXCOORD, -MAXCOORD, +MAXCOORD);
    glEnd();

    // west
    gld_BindFace(sb, 3);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, -MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, +MAXCOORD);
    glEnd();
  }
  else
  {
    faces = 1;

    // all 4 sides
    gld_BindFace(sb, 0);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(.25f, 0);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(.25f, 1);
    glVertex3f(-MAXCOORD, -MAXCOORD, -MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, -MAXCOORD);
    glEnd();

    // east
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(.25f, 0);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(.5f, 0);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(.5f, 1);
    glVertex3f(-MAXCOORD, -MAXCOORD, +MAXCOORD);
    glTexCoord2f(.25f, 1);
    glVertex3f(-MAXCOORD, -MAXCOORD, -MAXCOORD);
    glEnd();

    // south
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(.5f, 0);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(.75f, 0);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(.75f, 1);
    glVertex3f(+MAXCOORD, -MAXCOORD, +MAXCOORD);
    glTexCoord2f(.5f, 1);
    glVertex3f(-MAXCOORD, -MAXCOORD, +MAXCOORD);
    glEnd();

    // west
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(.75f, 0);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(+MAXCOORD, -MAXCOORD, -MAXCOORD);
    glTexCoord2f(.75f, 1);
    glVertex3f(+MAXCOORD, -MAXCOORD, +MAXCOORD);
    glEnd();
  }

  // top
  gld_BindFace(sb, faces);
  glBegin(GL_TRIANGLE_FAN);
  if (!sb->fliptop)
  {
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
  }
  else
  {
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, +MAXCOORD);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-MAXCOORD, +MAXCOORD, -MAXCOORD);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(+MAXCOORD, +MAXCOORD, -MAXCOORD);
  }
  glEnd();

  // bottom
  gld_BindFace(sb, faces + 1);

  glBegin(GL_TRIANGLE_FAN);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(+MAXCOORD, -MAXCOORD, -MAXCOORD);
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(-MAXCOORD, -MAXCOORD, -MAXCOORD);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(-MAXCOORD, -MAXCOORD, +MAXCOORD);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(+MAXCOORD, -MAXCOORD, +MAXCOORD);
  glEnd();

  glPopMatrix();

  glEnable(GL_ALPHA_TEST);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(true);

  SetFrameTextureMode();

  return true;
}
