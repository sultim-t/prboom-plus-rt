/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_intern.h,v 1.21 2003/03/29 18:43:43 proff_fs Exp $
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
#include "SDL.h"
#include "gl_dyn.h"
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
#include "p_tick.h"
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
  int leftoffset,topoffset;
  int realtexwidth, realtexheight;
  int tex_width,tex_height;
	int width,height;
  int glTexID[CR_LIMIT+MAXPLAYERS];
  GLTexType textype;
  boolean mipmap;
} GLTexture;

extern int gld_max_texturesize;
extern char *gl_tex_format_string;
extern int gl_tex_format;
extern int gl_tex_filter;
extern int gl_mipmap_filter;
extern int gl_texture_filter_anisotropic;
extern int gl_paletted_texture;
extern int gl_shared_texture_palette;
extern boolean use_mipmapping;
extern int transparent_pal_index;
extern unsigned char gld_palmap[256];
extern GLTexture *last_gltexture;
extern int last_cm;
  
GLTexture *gld_RegisterTexture(int texture_num, boolean mipmap);
void gld_BindTexture(GLTexture *gltexture);
GLTexture *gld_RegisterPatch(int lump, int cm);
void gld_BindPatch(GLTexture *gltexture, int cm);
GLTexture *gld_RegisterFlat(int lump, boolean mipmap);
void gld_BindFlat(GLTexture *gltexture);
void gld_InitPalettedTextures(void);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

typedef struct
{
  GLfloat x;
  GLfloat y;
  GLfloat z;
} GLVertex;

typedef struct
{
  GLfloat u;
  GLfloat v;
} GLTexcoord;

extern GLVertex *gld_vertexes;
extern GLTexcoord *gld_texcoords;

/* GLLoopDef is the struct for one loop. A loop is a list of vertexes
 * for triangles, which is calculated by the p_gluTesselator in gld_PrecalculateSector
 * and in gld_PreprocessCarvedFlat
 */
typedef struct
{
  GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
  int vertexcount; // number of vertexes in this loop
  int vertexindex; // index into vertex list
} GLLoopDef;

// GLSector is the struct for a sector with a list of loops.

typedef struct
{
  int loopcount; // number of loops for this sector
  GLLoopDef *loops; // the loops itself
} GLSector;

extern GLSector *sectorloops;

typedef struct
{
  float x1,x2;
  float z1,z2;
} GLSeg;

extern GLSeg *gl_segs;

#define GLDWF_TOP 1
#define GLDWF_M1S 2
#define GLDWF_M2S 3
#define GLDWF_BOT 4
#define GLDWF_SKY 5
#define GLDWF_SKYFLIP 6

typedef struct
{
  GLSeg *glseg;
  float ytop,ybottom;
  float ul,ur,vt,vb;
  float light;
  float alpha;
  float skyymid;
  float skyyaw;
  GLTexture *gltexture;
  byte flag;
} GLWall;

typedef struct
{
  int sectornum;
  float light; // the lightlevel of the flat
  float uoffs,voffs; // the texture coordinates
  float z; // the z position of the flat (height)
  GLTexture *gltexture;
  boolean ceiling;
} GLFlat;

typedef struct
{
  int cm;
  float x,y,z;
  float vt,vb;
  float ul,ur;
  float x1,y1;
  float x2,y2;
	float light;
  fixed_t scale;
  GLTexture *gltexture;
  boolean shadow;
  boolean trans;
} GLSprite;

typedef enum
{
  GLDIT_NONE,
  GLDIT_WALL,
  GLDIT_FLAT,
  GLDIT_SPRITE
} GLDrawItemType;

typedef struct
{
  GLDrawItemType itemtype;
  int itemcount;
  int firstitemindex;
  byte rendermarker;
} GLDrawItem;

typedef struct
{
  GLWall *walls;
  int num_walls;
  int max_walls;
  GLFlat *flats;
  int num_flats;
  int max_flats;
  GLSprite *sprites;
  int num_sprites;
  int max_sprites;
  GLDrawItem *drawitems;
  int num_drawitems;
  int max_drawitems;
} GLDrawInfo;

extern GLDrawInfo gld_drawinfo;

extern byte *sectorrendered; // true if sector rendered (only here for malloc)
extern byte *segrendered; // true if sector rendered (only here for malloc)

#define MAP_COEFF 128.0f
#define MAP_SCALE	(MAP_COEFF*(float)FRACUNIT)

#endif // _GL_INTERN_H
