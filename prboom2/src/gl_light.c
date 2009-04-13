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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <math.h>

#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "e6y.h"

gl_lightmode_t gl_lightmode;
const char *gl_lightmodes[] = {"glboom", "gzdoom"};
int gl_light_ambient;

int gl_fog;
int gl_use_fog;
int gl_fog_color;

int gl_fogenabled;
int gl_distfog = 70;
float gl_CurrentFogDensity = -1.0f;
float distfogtable[2][256];

typedef void (*gld_InitLightTable_f)(void);

typedef struct
{
  gld_InitLightTable_f Init;
  gld_CalcLightLevel_f GetLight;
  gld_CalcFogDensity_f GetFog;
} GLLight;

static float lighttable_glboom[5][256];
static float lighttable_gzdoom[256];

static void gld_InitLightTable_glboom(void);
static void gld_InitLightTable_gzdoom(void);

static float gld_CalcLightLevel_glboom(int lightlevel);
static float gld_CalcLightLevel_gzdoom(int lightlevel);

static float gld_CalcFogDensity_glboom(sector_t *sector, int lightlevel);
static float gld_CalcFogDensity_gzdoom(sector_t *sector, int lightlevel);

static GLLight gld_light[gl_lightmode_last] = {
  //gl_lightmode_glboom
  {gld_InitLightTable_glboom, gld_CalcLightLevel_glboom, gld_CalcFogDensity_glboom},
  //gl_lightmode_gzdoom
  {gld_InitLightTable_gzdoom, gld_CalcLightLevel_gzdoom, gld_CalcFogDensity_gzdoom},
};

gld_CalcLightLevel_f gld_CalcLightLevel = gld_CalcLightLevel_glboom;
gld_CalcFogDensity_f gld_CalcFogDensity = gld_CalcFogDensity_glboom;

void M_ChangeLightMode(void)
{
  gld_CalcLightLevel = gld_light[gl_lightmode].GetLight;
  gld_CalcFogDensity = gld_light[gl_lightmode].GetFog;

  if (gl_lightmode == gl_lightmode_glboom)
  {
    gld_SetGammaRamp(-1);
    gld_FlushTextures();
  }

  if (gl_lightmode == gl_lightmode_gzdoom)
  {
    gld_SetGammaRamp(useglgamma);
  }
}

void gld_InitLightTable(void)
{
  int i;

  for (i = 0; i < gl_lightmode_last; i++)
  {
    gld_light[i].Init();
  }
}

/*
 * lookuptable for lightvalues
 * calculated as follow:
 * floatlight=(1.0-exp((light^3)*gamma)) / (1.0-exp(1.0*gamma));
 * gamma=-0,2;-2,0;-4,0;-6,0;-8,0
 * light=0,0 .. 1,0
 */
static void gld_InitLightTable_glboom(void)
{
  int i, g;
  float gamma[5] = {-0.2f, -2.0f, -4.0f, -6.0f, -8.0f};

  for (g = 0; g < 5; g++)
  {
    for (i = 0; i < 256; i++)
    {
      lighttable_glboom[g][i] = (float)((1.0f - exp(pow(i / 255.0f, 3) * gamma[g])) / (1.0f - exp(1.0f * gamma[g])));
    }
  }
}

static void gld_InitLightTable_gzdoom(void)
{
  int i;
  float light;

  for (i = 0; i < 256; i++)
  {
    if (i < 192) 
      light = (192.0f - (192 - i) * 1.95f);
    else
      light = (float)i;

    if (light < gl_light_ambient)
      light = (float)gl_light_ambient;

    lighttable_gzdoom[i] = light / 255.0f;
  }
}

static float gld_CalcLightLevel_glboom(int lightlevel)
{
  return lighttable_glboom[usegamma][BETWEEN(0, 255, lightlevel)];
}

static float gld_CalcLightLevel_gzdoom(int lightlevel)
{
  return lighttable_gzdoom[BETWEEN(0, 255, lightlevel)];
}

void gld_StaticLightAlpha(float light, float alpha)
{
  player_t *player;
  player = &players[displayplayer];

  if (!player->fixedcolormap)
  {
    glColor4f(light, light, light, alpha);
  }
  else
  {
    if (!(invul_method & INVUL_BW))
    {
      glColor4f(1.0f, 1.0f, 1.0f, alpha);
    }
    else
    {
#ifdef USE_FBO_TECHNIQUE
      if (SceneInTexture)
      {
        if (gl_invul_bw_method == 0)
          glColor4f(0.5f, 0.5f, 0.5f, alpha);
        else
          glColor4f(1.0f, 1.0f, 1.0f, alpha);
      }
      else
#endif
      {
        glColor4f(bw_red, bw_green, bw_blue, alpha);
      }
    }
  }
}

void M_ChangeAllowFog(void)
{
  int i;
  GLfloat FogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

  FogColor[0] = ((float)((gl_fog_color >> 16) & 0xff)) / 255.0f;
  FogColor[1] = ((float)((gl_fog_color >>  8) & 0xff)) / 255.0f;
  FogColor[2] = ((float)((gl_fog_color >>  0) & 0xff)) / 255.0f;
  FogColor[3] = 0.0f;

  glFogi (GL_FOG_MODE, GL_EXP);
  glFogfv(GL_FOG_COLOR, FogColor);
  glHint (GL_FOG_HINT, GL_FASTEST);

  gl_CurrentFogDensity = -1;

  gl_EnableFog(true);
  gl_EnableFog(false);

  for (i = 0; i < 256; i++)
  {
    if (i < 164)
    {
      distfogtable[0][i] = (float)((gl_distfog >> 1) + (gl_distfog) * (164 - i) / 164);
    }
    else if (i < 230)
    {											    
      distfogtable[0][i] = (float)((gl_distfog >> 1) - (gl_distfog >> 1) * (i - 164) / (230 - 164));
    }
    else
    {
      distfogtable[0][i] = 0.0f;
    }

    if (i < 128)
    {
      distfogtable[1][i] = 6.0f + (gl_distfog >> 1) + (gl_distfog) * (128 - i) / 48;
    }
    else if (i < 216)
    {											    
      distfogtable[1][i] = (216.0f - i) / ((216.0f - 128.0f)) * gl_distfog / 10;
    }
    else
    {
      distfogtable[1][i] = 0.0f;
    }
  }
}

static float gld_CalcFogDensity_glboom(sector_t *sector, int lightlevel)
{
  return 0;
}

static float gld_CalcFogDensity_gzdoom(sector_t *sector, int lightlevel)
{
  if ((!gl_use_fog) ||
    (sector && (sector->ceilingpic == skyflatnum || sector->floorpic == skyflatnum)))
  {
    return 0;
  }
  else
  {
    return distfogtable[1][lightlevel];
  }
}

void gld_SetFog(float fogdensity)
{
  if (fogdensity)
  {
    gl_EnableFog(true);
    if (fogdensity != gl_CurrentFogDensity)
    {
      glFogf(GL_FOG_DENSITY, fogdensity / 1024.0f);
      gl_CurrentFogDensity = fogdensity;
    }
  }
  else
  {
    gl_EnableFog(false);
    gl_CurrentFogDensity = -1.0f;
  }
}

void gl_EnableFog(int on)
{
  if (on) 
  {
    if (!gl_fogenabled)
    glEnable(GL_FOG);
  }
  else 
  {
    if (gl_fogenabled)
      glDisable(GL_FOG);
  }
  gl_fogenabled=on;
}
