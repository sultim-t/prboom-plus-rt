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

float lighttable[5][256];

int gl_fog;
int gl_use_fog;
int gl_fog_color;

int gl_fogenabled;
int gl_distfog = 70;
float gl_CurrentFogDensity = -1.0f;
float distfogtable[2][256];

typedef float (*gld_CalcLightLevel_f)(int lightlevel);
typedef float (*gld_CalcFogDensity_f)(sector_t *sector, int lightlevel);

/*
 * lookuptable for lightvalues
 * calculated as follow:
 * floatlight=(1.0-exp((light^3)*gamma)) / (1.0-exp(1.0*gamma));
 * gamma=-0,2;-2,0;-4,0;-6,0;-8,0
 * light=0,0 .. 1,0
 */
void gld_InitLightTable(void)
{
  int i, g;
  float gamma[5] = {-0.2f, -2.0f, -4.0f, -6.0f, -8.0f};

  for (g = 0; g < 5; g++)
  {
    for (i = 0; i < 256; i++)
    {
      lighttable[g][i] = (float)((1.0f - exp(pow(i / 255.0f, 3) * gamma[g])) / (1.0f - exp(1.0f * gamma[g])));
    }
  }
}

static float gld_CalcLightLevel_glboom(int lightlevel)
{
  return (lighttable[usegamma][BETWEEN(0, 255, lightlevel)]);
}

static float gld_CalcLightLevel_gzdoom(int lightlevel)
{
  float light;

  if (lightlevel < 192) 
    light = (192.0f - (192 - lightlevel) * 1.95f);
  else
    light = (float)lightlevel;

  if (light < gl_light_ambient)
    light = (float)gl_light_ambient;

  return light / 255.0f;
}

static float gld_CalcLightLevel_mixed(int lightlevel)
{
  if (lightlevel < gl_light_ambient)
    return (float)gl_light_ambient / 255.0f;
  else
    return (float)lightlevel / 255.0f;
}

static gld_CalcLightLevel_f gld_CalcLightLevelFuncs[gl_lightmode_last] =
{
  gld_CalcLightLevel_glboom,
  gld_CalcLightLevel_gzdoom,
  gld_CalcLightLevel_mixed,
};

float gld_CalcLightLevel(int lightlevel)
{
  return gld_CalcLightLevelFuncs[gl_lightmode](lightlevel);
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
  if (sector && (sector->ceilingpic == skyflatnum || sector->floorpic == skyflatnum))
  {
    return 0;
  }
  else
  {
    return distfogtable[1][lightlevel];
  }
}

static float gld_CalcFogDensity_mixed(sector_t *sector, int lightlevel)
{
  if (sector && (sector->ceilingpic == skyflatnum || sector->floorpic == skyflatnum))
  {
    return 0;
  }
  else
  {
    return distfogtable[1][lightlevel];
  }
}

static gld_CalcFogDensity_f gld_CalcFogDensityFuncs[gl_lightmode_last] =
{
  gld_CalcFogDensity_glboom,
  gld_CalcFogDensity_gzdoom,
  gld_CalcFogDensity_mixed,
};

float gld_CalcFogDensity(sector_t *sector, int lightlevel)
{
  if (gl_use_fog)
  {
    return gld_CalcFogDensityFuncs[gl_lightmode](sector, lightlevel);
  }
  else
  {
    return 0;
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
