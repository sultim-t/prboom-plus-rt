/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_intern.h,v 1.13 2000/10/08 18:42:19 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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

#ifndef _GL_INTERN_H
#define _GL_INTERN_H

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
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL.h"
#include "doomtype.h"
#include "w_wad.h"
#include "m_argv.h"
#include "z_zone.h"
#include "d_event.h"
#include "v_video.h"
#include "doomstat.h"
#include "r_bsp.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_data.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "lprintf.h"

typedef enum
{
  GLDT_UNREGISTERED,
  GLDT_BROKEN,
  GLDT_PATCH,
  GLDT_TEXTURE,
  GLDT_FLAT
} GLTexType;

typedef struct
{
  int index;
	int width,height;
  int leftoffset,topoffset;
  int tex_width,tex_height;
  int realtexwidth, realtexheight;
  int buffer_width,buffer_height;
  int buffer_size;
  int glTexID[CR_LIMIT+MAXPLAYERS];
  GLTexType textype;
  boolean mipmap;
} GLTexture;

extern int gld_max_texturesize;
extern char *gl_tex_format_string;
extern int gl_tex_format;
extern int gl_tex_filter;
extern int gl_mipmap_filter;
#ifdef USE_GLU_MIPMAP
extern boolean use_mipmapping;
#endif
  
GLTexture *gld_RegisterTexture(int texture_num, boolean mipmap);
void gld_BindTexture(GLTexture *gltexture);
GLTexture *gld_RegisterPatch(int lump, int cm);
void gld_BindPatch(GLTexture *gltexture, int cm);
GLTexture *gld_RegisterFlat(int lump, boolean mipmap);
void gld_BindFlat(GLTexture *gltexture);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#endif // _GL_INTERN_H
