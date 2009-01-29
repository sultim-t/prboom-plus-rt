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

#include <SDL.h>
#include <SDL_opengl.h>

#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "r_plane.h"
#include "r_sky.h"

#include "e6y.h"

int gl_drawskys;

static PalEntry_t *SkyColor;

SkyBoxParams_t SkyBox;

void gld_InitSky(void)
{
  memset(&SkyBox, 0, sizeof(SkyBox));
  SkyBox.index = -1;
}

void gld_InitFrameSky(void)
{
  SkyBox.type = SKY_NONE;
  SkyBox.wall.gltexture = NULL;
  SkyBox.x_scale = 0;
  SkyBox.y_scale = 0;
  SkyBox.x_offset = 0;
  SkyBox.y_offset = 0;
}

void gld_DrawSkybox(void)
{
  //if (gl_drawskys == skytype_screen)
  //  gld_DrawScreenSkybox();

  if (gl_drawskys == skytype_skydome)
    gld_DrawDomeSkyBox();
}

void gld_GetScreenSkyScale(GLWall *wall, float *scale_x, float *scale_y)
{
  float sx, sy;

  if (!mlook_or_fov)
  {
    if ((wall->flag & GLDWF_SKYFLIP) == GLDWF_SKYFLIP)
      sx = -128.0f / (float)wall->gltexture->buffer_width;
    else
      sx = +128.0f / (float)wall->gltexture->buffer_width;

    sy = 200.0f / 320.0f * (256 / wall->gltexture->buffer_height);
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

      case skytype_skydome:
        if (s)
        {
          SkyBox.x_offset = (float)s->textureoffset * 180.0f / (float)ANG180;
          SkyBox.y_offset = (float)s->rowoffset / MAP_SCALE;
        }
        break;
      }
    }
  }
}

void gld_DrawSkyCaps(void)
{
  if (SkyBox.type && SkyBox.wall.gltexture)
  {             
    float maxcoord = 255.0f;
    boolean mlook = GetMouseLook();

    if (mlook)
    {
      gld_BindTexture(SkyBox.wall.gltexture);

      glMatrixMode(GL_TEXTURE);
      glPushMatrix();

      glScalef(SkyBox.x_scale, SkyBox.y_scale, 1.0f);
      glTranslatef(SkyBox.wall.skyyaw, SkyBox.wall.skyymid, 0.0f);

      if (SkyBox.type & SKY_CEILING)
      {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-maxcoord,+maxcoord,+maxcoord);
        glVertex3f(+maxcoord,+maxcoord,+maxcoord);
        glVertex3f(-maxcoord,+maxcoord,-maxcoord);
        glVertex3f(+maxcoord,+maxcoord,-maxcoord);
        glEnd();
      }

      if (SkyBox.type & SKY_FLOOR)
      {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(-maxcoord,-maxcoord,+maxcoord);
        glVertex3f(+maxcoord,-maxcoord,+maxcoord);
        glVertex3f(-maxcoord,-maxcoord,-maxcoord);
        glVertex3f(+maxcoord,-maxcoord,-maxcoord);
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

  PalEntry->r = (float)r / 255.0f;
  PalEntry->g = (float)g / 255.0f;
  PalEntry->b = (float)b / 255.0f;
  return;
}

// It is an alternative way of drawing the sky (gl_drawskys == skytype_screen)
// This method make sense only for old hardware which have no support for GL_TEXTURE_GEN_*
// Voodoo as example
void gld_DrawScreenSkybox(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho((GLdouble)0, (GLdouble)SCREENWIDTH, (GLdouble) SCREENHEIGHT,
    (GLdouble) 0, (GLdouble) -1.0, (GLdouble) 1.0);

  glDepthRange(1.0f, 1.0f);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  if (gld_drawinfo.num_items[GLDIT_SWALL] > 0)
  {
    float fU1,fU2,fV1,fV2;
    float k1, k2;

    GLWall *wall = wall = gld_drawinfo.items[GLDIT_SWALL][0].item.wall;

    gld_BindTexture(wall->gltexture);

    if (!mlook_or_fov)
    {
      k1 = wall->gltexture->buffer_width / 128.0f;
      k2 = 256.0f / wall->gltexture->buffer_width;

      fV1 = 0.0f;
      fV2 = 320.0f/200.0f;
    }
    else 
    {
      if (wall->gltexture->buffer_width <= 512)
      {
        k1 = wall->gltexture->buffer_width / 64.0f;
        k2 = 128.0f / wall->gltexture->buffer_width;
      }
      else
      {
        k1 = wall->gltexture->buffer_width / 128.0f;
        k2 = 256.0f / wall->gltexture->buffer_width;
      }

      skyYShift = viewPitch<skyUpAngle ? 0.0f : (float)(0.0f-0.6f)/(skyUpAngle-0.0f)*(viewPitch)+0.6f;

      fV1 = skyYShift*fovscale;
      fV2 = fV1 + 320.0f/200.0f/2.0f;
    }

    if ((wall->flag&GLDWF_SKYFLIP) == GLDWF_SKYFLIP)
    {
      fU1 = 0.5f - wall->skyyaw/(k1/fovscale);
      fU2 = fU1 + (k2/fovscale);
    }
    else
    {
      fU2 = 0.5f + wall->skyyaw/(k1/fovscale);
      fU1 = fU2 + (k2/fovscale);
    }

    glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(fU1, fV1); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(fU1, fV2); glVertex2f(0.0f, (float)SCREENHEIGHT);
      glTexCoord2f(fU2, fV1); glVertex2f((float)SCREENWIDTH, 0.0f);
      glTexCoord2f(fU2, fV2); glVertex2f((float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
  }

  glDepthRange(0.0f, 1.0f);
}

// The texture offset to be applied to the texture coordinates in SkyVertex().
static angle_t maxSideAngle = ANG180 / 3;
static int rows, columns;	
static fixed_t scale = 10000 << FRACBITS;
static boolean yflip;
static int texw;
static float yMult, yAdd;
static boolean foglayer;
static float delta = 0.0f;

int gl_sky_detail = 16;

#define SKYHEMI_UPPER		0x1
#define SKYHEMI_LOWER		0x2
#define SKYHEMI_JUST_CAP	0x4	// Just draw the top or bottom cap.

#define ANGLE_MAX		(0xffffffff)
#define TO_GL(v) ((float)(v)/(float)MAP_SCALE)

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void gld_GetSkyCapColors(void)
{
  int width, height;
  unsigned char *buffer = NULL;

  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

  buffer = malloc(width * height * 4);
  glGetTexImage(GL_TEXTURE_2D, 0, gl_tex_format, GL_UNSIGNED_BYTE, buffer);

  averageColor(&SkyBox.CeilingSkyColor, (unsigned int*)buffer, width * MIN(30, height), 0);

  if (height > 30)
  {
    averageColor(&SkyBox.FloorSkyColor, 
      ((unsigned int*)buffer) + (height - 30) * width, width * 30, 0);
  }
  else
  {
    SkyBox.FloorSkyColor = SkyBox.CeilingSkyColor;
  }

  free(buffer);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

static void SkyVertex(int r, int c)
{
  angle_t topAngle= (angle_t)(c / (float)columns * ANGLE_MAX);
  angle_t sideAngle = maxSideAngle * (rows - r) / rows;
  fixed_t height = finesine[sideAngle>>ANGLETOFINESHIFT];
  fixed_t realRadius = FixedMul(scale, finecosine[sideAngle>>ANGLETOFINESHIFT]);
  fixed_t x = FixedMul(realRadius, finecosine[topAngle>>ANGLETOFINESHIFT]);
  fixed_t y = (!yflip) ? FixedMul(scale, height) : FixedMul(scale, height) * -1;
  fixed_t z = FixedMul(realRadius, finesine[topAngle>>ANGLETOFINESHIFT]);
  float fx, fy, fz;
  float u, v;
  float timesRepeat;

  timesRepeat = (short)(4 * (256.0f / texw));
  if (timesRepeat == 0.0f)
    timesRepeat = 1.0f;

  if (!foglayer)
  {
    glColor4f(1.0f, 1.0f, 1.0f, (r == 0 ? 0.0f : 1.0f));
    
    // And the texture coordinates.
    if(!yflip)	// Flipped Y is for the lower hemisphere.
    {
      u = (-timesRepeat * c / (float)columns) ;
      v = (r / (float)rows) * 1.f * yMult + yAdd;
    }
    else
    {
      u = (-timesRepeat * c / (float)columns) ;
      v = ((rows-r)/(float)rows) * 1.f * yMult + yAdd;
    }

    glTexCoord2f(u, v);
  }
  // And finally the vertex.
  fx =-TO_GL(x);	// Doom mirrors the sky vertically!
  fy = TO_GL(y);
  fz = TO_GL(z);
  glVertex3f(fx, fy + delta, fz);
}


//-----------------------------------------------------------------------------
//
// Hemi is Upper or Lower. Zero is not acceptable.
// The current texture is used. SKYHEMI_NO_TOPCAP can be used.
//
//-----------------------------------------------------------------------------

static void RenderSkyHemisphere(int hemi)
{
  int r, c;

  yflip = (hemi & SKYHEMI_LOWER);

  // The top row (row 0) is the one that's faded out.
  // There must be at least 4 columns. The preferable number
  // is 4n, where n is 1, 2, 3... There should be at least
  // two rows because the first one is always faded.
  rows = 4;

  if (hemi & SKYHEMI_JUST_CAP)
  {
    return;
  }

  delta = 0.0f;

  // Draw the cap as one solid color polygon
  if (!foglayer)
  {
    columns = 4 * gl_sky_detail;
    if (mlook_or_fov)
    {
      foglayer = true;
      glDisable(GL_TEXTURE_2D);

      glColor3f(SkyColor->r, SkyColor->g ,SkyColor->b);
      glBegin(GL_TRIANGLE_FAN);
      for(c = 0; c < columns; c++)
      {
        SkyVertex(1, c);
      }
      glEnd();

      glEnable(GL_TEXTURE_2D);
      foglayer = false;
    }
  }
  else
  {
    columns = 4;	// no need to do more!
    glBegin(GL_TRIANGLE_FAN);
    for(c = 0; c < columns; c++)
    {
      SkyVertex(0, c);
    }
    glEnd();
  }

  if (hemi & SKYHEMI_UPPER)
  {
      delta = -5.0f / 128.0f;
  }

  if (hemi & SKYHEMI_LOWER)
  {
      delta = 5.0f / 128.0f;
  }

  // The total number of triangles per hemisphere can be calculated
  // as follows: rows * columns * 2 + 2 (for the top cap).
  //glColor3f(1.0f, 1.0f , 1.0f);
  for(r = 0; r < rows; r++)
  {
    if (yflip)
    {
      glBegin(GL_TRIANGLE_STRIP);
      SkyVertex(r + 1, 0);
      SkyVertex(r, 0);
      for(c = 1; c <= columns; c++)
      {
        SkyVertex(r + 1, c);
        SkyVertex(r, c);
      }
      glEnd();
    }
    else
    {
      glBegin(GL_TRIANGLE_STRIP);
      SkyVertex(r, 0);
      SkyVertex(r + 1, 0);
      for(c = 1; c <= columns; c++)
      {
        SkyVertex(r, c);
        SkyVertex(r + 1, c);
      }
      glEnd();
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
	int texh;

  if (!sky || !sky->wall.gltexture)
    return;

  yflip = (sky->wall.flag & GLDWF_SKYFLIP);

  gld_BindTexture(sky->wall.gltexture);

  if (sky->wall.gltexture->index != sky->index)
  {
    sky->index = sky->wall.gltexture->index;
    gld_GetSkyCapColors();
  }

  texw = sky->wall.gltexture->buffer_width;
  texh = sky->wall.gltexture->buffer_height;
  if (texh > 190)
    texh = 190;

  glRotatef(-180.0f + sky->x_offset, 0.f, 1.f, 0.f);

  yAdd = sky->y_offset / texh;
  yMult = (texh <= 180 ? 1.0f : 180.0f / texh);

  //if (gl_FrameSkies & SKY_CEILING)
  {
    SkyColor = &sky->CeilingSkyColor;
    RenderSkyHemisphere(SKYHEMI_UPPER);
  }

  //if (gl_FrameSkies & SKY_FLOOR)
  {
    if (texh <= 180)
      yMult = 1.0f;
    else
      yAdd += 180.0f/texh;

    SkyColor = &sky->FloorSkyColor;
    RenderSkyHemisphere(SKYHEMI_LOWER);
  }

  glRotatef(180.0f - sky->x_offset, 0, 1, 0);
  glScalef(1.0f, 1.0f, 1.0f);
}

void gld_DrawDomeSkyBox(void)
{
  if (SkyBox.wall.gltexture)
  {
    GLint shading_mode = GL_FLAT;
    
    glGetIntegerv(GL_SHADE_MODEL, &shading_mode);
    glShadeModel(GL_SMOOTH);

    glDepthMask(false);

    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(roll,  0.0f, 0.0f, 1.0f);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw,   0.0f, 1.0f, 0.0f);
    glScalef(-2.0f, 2.0f, 2.0f);
    glTranslatef(0.f, -1000.0f/128.0f, 0.f);
    //glTranslatef(xCamera, zCamera, -yCamera);

    RenderDome(&SkyBox);

    glPopMatrix();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);

    glShadeModel(shading_mode);
  }
}
