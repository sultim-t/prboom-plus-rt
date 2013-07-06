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

#ifdef USE_SHADERS

#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include "doomstat.h"
#include "v_video.h"
#include "gl_opengl.h"
#include "gl_intern.h"
#include "r_main.h"
#include "w_wad.h"
#include "i_system.h"
#include "r_bsp.h"
#include "lprintf.h"
#include "e6y.h"
#include "r_things.h"

GLShader *sh_main = NULL;
static GLShader *active_shader = NULL;

static GLShader* gld_LoadShader(const char *vpname, const char *fpname);

int glsl_Init(void)
{
  static int init = false;

  //if (!init)
  {
    init = true;

    if (!gl_arb_shader_objects)
    {
      lprintf(LO_WARN, "glsl_Init: shaders expects OpenGL 2.0\n");
    }
    else
    {
      sh_main = gld_LoadShader("glvp", "glfp");
    }
  }

  return (sh_main != NULL);
}

static int ReadLump(const char *filename, const char *lumpname, unsigned char **buffer)
{
  FILE *file = NULL;
  int size = 0;
  const unsigned char *data;
  int lump;

  file = fopen(filename, "r");
  if (file)
  {
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *buffer = malloc(size + 1);
    size = fread(*buffer, 1, size, file);
    if (size > 0)
    {
      (*buffer)[size] = 0;
    }
    fclose(file);
  }
  else
  {
    char name[9];
    char* p;

    strncpy(name, lumpname, 9);
    name[8] = 0;
    for(p = name; *p; p++)
      *p = toupper(*p);

    lump = (W_CheckNumForName)(name, ns_prboom);

    if (lump != -1)
    {
      size = W_LumpLength(lump);
      data = W_CacheLumpNum(lump);
      *buffer = calloc(1, size + 1);
      memcpy (*buffer, data, size);
      (*buffer)[size] = 0;
      W_UnlockLumpNum(lump);
    }
  }

  return size;
}

static GLShader* gld_LoadShader(const char *vpname, const char *fpname)
{
#define buffer_size 2048
  int idx;
  int linked;
  char buffer[buffer_size];
  char *vp_data = NULL;
  char *fp_data = NULL;
  int vp_size, fp_size;
  size_t vp_fnlen, fp_fnlen;
  char *filename = NULL;
  GLShader* shader = NULL;

  vp_fnlen = doom_snprintf(NULL, 0, "%s/shaders/%s.txt", I_DoomExeDir(), vpname);
  fp_fnlen = doom_snprintf(NULL, 0, "%s/shaders/%s.txt", I_DoomExeDir(), fpname);
  filename = malloc(MAX(vp_fnlen, fp_fnlen) + 1);

  sprintf(filename, "%s/shaders/%s.txt", I_DoomExeDir(), vpname);
  vp_size = ReadLump(filename, vpname, &vp_data);

  sprintf(filename, "%s/shaders/%s.txt", I_DoomExeDir(), fpname);
  fp_size = ReadLump(filename, fpname, &fp_data);
  
  if (vp_data && fp_data)
  {
    shader = calloc(1, sizeof(GLShader));

    shader->hVertProg = GLEXT_glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    shader->hFragProg = GLEXT_glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);	

    GLEXT_glShaderSourceARB(shader->hVertProg, 1, &vp_data, &vp_size);
    GLEXT_glShaderSourceARB(shader->hFragProg, 1, &fp_data, &fp_size);

    GLEXT_glCompileShaderARB(shader->hVertProg);
    GLEXT_glCompileShaderARB(shader->hFragProg);

    shader->hShader = GLEXT_glCreateProgramObjectARB();

    GLEXT_glAttachObjectARB(shader->hShader, shader->hVertProg);
    GLEXT_glAttachObjectARB(shader->hShader, shader->hFragProg);

    GLEXT_glLinkProgramARB(shader->hShader);

    GLEXT_glGetInfoLogARB(shader->hShader, buffer_size, NULL, buffer);

    GLEXT_glGetObjectParameterivARB(shader->hShader, GL_OBJECT_LINK_STATUS_ARB, &linked);

    if (linked)
    {
      lprintf(LO_INFO, "gld_LoadShader: Shader \"%s+%s\" compiled OK: %s\n", vpname, fpname, buffer);

      shader->lightlevel_index = GLEXT_glGetUniformLocationARB(shader->hShader, "lightlevel");

      GLEXT_glUseProgramObjectARB(shader->hShader);

      idx = GLEXT_glGetUniformLocationARB(shader->hShader, "tex");
      GLEXT_glUniform1iARB(idx, 0);

      GLEXT_glUseProgramObjectARB(0);
    }
    else
    {
      lprintf(LO_ERROR, "gld_LoadShader: Error compiling shader \"%s+%s\": %s\n", vpname, fpname, buffer);
      free(shader);
      shader = NULL;
    }
  }

  free(filename);
  free(vp_data);
  free(fp_data);

  return shader;
}

void glsl_SetActiveShader(GLShader *shader)
{
  if (gl_lightmode == gl_lightmode_shaders)
  {
    if (shader != active_shader)
    {
      GLEXT_glUseProgramObjectARB((shader ? shader->hShader : 0));
      active_shader = shader;
    }
  }
}

void glsl_SetLightLevel(float lightlevel)
{
  if (sh_main)
  {
    GLEXT_glUniform1fARB(sh_main->lightlevel_index, lightlevel);
  }
}

int glsl_IsActive(void)
{
  return (gl_lightmode == gl_lightmode_shaders && sh_main);
}

#endif // USE_SHADERS
