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

#ifndef _GL_INTERN_H
#define _GL_INTERN_H

#include "v_video.h"

#define MAXCOORD (32767.0f / MAP_COEFF)

#define SMALLDELTA 0.001f

typedef enum
{
  GLDT_UNREGISTERED,
  GLDT_BROKEN,
  GLDT_PATCH,
  GLDT_TEXTURE,
  GLDT_FLAT
} GLTexType;

typedef enum
{
  MIP_TEXTURE,
  MIP_SPRITE,
  MIP_PATCH,

  MIP_COUNT
} GLMipType;

typedef struct tex_filter_s
{
  int mipmap;
  int mag_filter;
  int min_filter;
} tex_filter_t;

typedef enum
{
  GLTEXTURE_SPRITE    = 0x00000002,
  GLTEXTURE_HASHOLES  = 0x00000004,
  GLTEXTURE_SKY       = 0x00000008,
  GLTEXTURE_HIRES     = 0x00000010,
  GLTEXTURE_HASNOHIRES= 0x00000020,
  GLTEXTURE_CLAMPX    = 0x00000040,
  GLTEXTURE_CLAMPY    = 0x00000080,
  GLTEXTURE_CLAMPXY   = (GLTEXTURE_CLAMPX | GLTEXTURE_CLAMPY),
  GLTEXTURE_MIPMAP    = 0x00000100,
} GLTexture_flag_t;

typedef struct gl_strip_coords_s
{
  GLfloat v[4][3];

  GLfloat t[4][2];
} gl_strip_coords_t;

#define PLAYERCOLORMAP_COUNT (3)

typedef struct detail_s
{
  GLuint texid;
  int texture_num;
  float width, height;
  float offsetx, offsety;
} detail_t;

typedef struct
{
  int index;
  int width,height;
  int leftoffset,topoffset;
  int tex_width,tex_height;
  int realtexwidth, realtexheight;
  int buffer_width,buffer_height;
  int buffer_size;
  
  //e6y: support for Boom colormaps
  GLuint ***glTexExID;
  unsigned int texflags[CR_LIMIT+MAXPLAYERS][PLAYERCOLORMAP_COUNT];
  GLuint *texid_p;
  unsigned int *texflags_p;

  int cm;
  int player_cm;

  GLTexType textype;
  unsigned int flags;
  float scalexfac, scaleyfac; //e6y: right/bottom UV coordinates for patch drawing

  //detail
  detail_t *detail;
  float detail_width, detail_height;
} GLTexture;

typedef struct
{
  float x1,x2;
  float z1,z2;
  dboolean fracleft, fracright; //e6y
} GLSeg;

typedef struct
{
  GLSeg *glseg;
  float ytop,ybottom;
  float ul,ur,vt,vb;
  float light;
  float fogdensity;
  float alpha;
  float skyymid;
  float skyyaw;
  GLTexture *gltexture;
  byte flag;
  seg_t *seg;
} GLWall;

typedef enum
{
  GLFLAT_CEILING      = 0x00000001,
  GLFLAT_HAVE_OFFSET  = 0x00000002,
} GLFlat_flag_t;

typedef struct
{
  int sectornum;
  float light; // the lightlevel of the flat
  float fogdensity;
  float uoffs,voffs; // the texture coordinates
  float z; // the z position of the flat (height)
  GLTexture *gltexture;
  unsigned int flags;
  float alpha;
} GLFlat;

/* GLLoopDef is the struct for one loop. A loop is a list of vertexes
 * for triangles, which is calculated by the gluTesselator in gld_PrecalculateSector
 * and in gld_PreprocessCarvedFlat
 */
typedef struct
{
  int index;   // subsector index
  GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
  int vertexcount; // number of vertexes in this loop
  int vertexindex; // index into vertex list
} GLLoopDef;

// GLSector is the struct for a sector with a list of loops.

#define SECTOR_CLAMPXY   0x00000001
typedef struct
{
  int loopcount; // number of loops for this sector
  GLLoopDef *loops; // the loops itself
  unsigned int flags;
} GLSector;

typedef struct
{
  int loopcount; // number of loops for this sector
  GLLoopDef *loops; // the loops itself
} GLMapSubsector;

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

typedef struct
{
  GLLoopDef loop; // the loops itself
} GLSubSector;

typedef struct
{
  float x, y, z;
  float radius;
  float light;
} GLShadow;

typedef struct
{
  int cm;

  float x1, x2, x3;
  float z1, z2, z3;
  float y;
} GLHealthBar;

extern GLSeg *gl_segs;
extern GLSeg *gl_lines;

#define GLDWF_TOP 1
#define GLDWF_M1S 2
#define GLDWF_M2S 3
#define GLDWF_BOT 4
#define GLDWF_TOPFLUD 5 //e6y: project the ceiling plane into the gap
#define GLDWF_BOTFLUD 6 //e6y: project the floor plane into the gap
#define GLDWF_SKY 7
#define GLDWF_SKYFLIP 8

typedef struct
{
  int cm;
  float x,y,z;
  float vt,vb;
  float ul,ur;
  float x1,y1;
  float x2,y2;
  float light;
  float fogdensity;
  fixed_t scale;
  GLTexture *gltexture;
  uint_64_t flags;
  int index;
  int xy;
} GLSprite;

typedef enum
{
  GLDIT_NONE,

  GLDIT_WALL,    // opaque wall
  GLDIT_MWALL,   // opaque mid wall
  GLDIT_FWALL,   // projected wall
  GLDIT_TWALL,   // transparent walls
  GLDIT_SWALL,   // sky walls

  GLDIT_AWALL,   // animated wall
  GLDIT_FAWALL,  // animated projected wall
  
  GLDIT_CEILING, // ceiling
  GLDIT_FLOOR,   // floor

  GLDIT_ACEILING, // animated ceiling
  GLDIT_AFLOOR,   // animated floor

  GLDIT_SPRITE,  // opaque sprite
  GLDIT_TSPRITE, // transparent sprites
  GLDIT_ASPRITE,

  GLDIT_SHADOW,

  GLDIT_HBAR,

  GLDIT_TYPES
} GLDrawItemType;

typedef struct GLDrawItem_s
{
  union
  {
    void *item;
    GLWall *wall;
    GLFlat *flat;
    GLSprite *sprite;
    GLShadow *shadow;
    GLHealthBar *hbar;
  } item;
} GLDrawItem;

typedef struct GLDrawDataItem_s
{
  byte *data;
  int maxsize;
  int size;
} GLDrawDataItem_t;

typedef struct
{
  GLDrawDataItem_t *data;
  int maxsize;
  int size;

  GLDrawItem *items[GLDIT_TYPES];
  int num_items[GLDIT_TYPES];
  int max_items[GLDIT_TYPES];
} GLDrawInfo;

void gld_AddDrawItem(GLDrawItemType itemtype, void *itemdata);

void gld_DrawTriangleStrip(GLWall *wall, gl_strip_coords_t *c);
void gld_DrawTriangleStripARB(GLWall *wall, gl_strip_coords_t *c1, gl_strip_coords_t *c2);

extern float roll;
extern float yaw;
extern float inv_yaw;
extern float pitch;

extern int gl_compatibility;
extern int gl_ztrick;
extern int gl_finish;

extern int gl_preprocessed; //e6y

extern GLDrawInfo gld_drawinfo;
void gld_FreeDrawInfo(void);
void gld_ResetDrawInfo(void);

extern GLSector *sectorloops;
extern GLMapSubsector *subsectorloops;

extern const char *gl_tex_format_string;
extern int gl_tex_format;
extern int gl_texture_filter_anisotropic;
extern int transparent_pal_index;
extern unsigned char gld_palmap[256];
extern tex_filter_t tex_filter[];
void gld_SetTexFilters(GLTexture *gltexture);

extern float xCamera,yCamera,zCamera;

//
//detail
//

int gld_IsDetailVisible(float x0, float y0, float x1, float y1, float x2, float y2);
void gld_InitDetail(void);
void gld_InitFrameDetails(void);
void gld_ParseDetail(void);
void gld_SetTexDetail(GLTexture *gltexture);

void gld_PreprocessDetail(void);
void gld_DrawDetail_NoARB(void);
void gld_EnableDetail(int enable);
void gld_DrawWallWithDetail(GLWall *wall);
void gld_BindDetail(GLTexture *gltexture, int enable);
void gld_BindDetailARB(GLTexture *gltexture, int enable);
void gld_DrawItemsSortByDetail(GLDrawItemType itemtype);
void gld_DrawWallDetail_NoARB(GLWall *wall);

extern int render_usedetail;
extern int scene_has_details;
extern detail_t *details;
extern int details_count;

extern int scene_has_wall_details;
extern int scene_has_flat_details;

extern GLuint* last_glTexID;
GLTexture *gld_RegisterTexture(int texture_num, dboolean mipmap, dboolean force);
void gld_BindTexture(GLTexture *gltexture, unsigned int flags);
GLTexture *gld_RegisterPatch(int lump, int cm, dboolean is_sprite);
void gld_BindPatch(GLTexture *gltexture, int cm);
GLTexture *gld_RegisterFlat(int lump, dboolean mipmap);
void gld_BindFlat(GLTexture *gltexture, unsigned int flags);
void gld_InitPalettedTextures(void);
int gld_GetTexDimension(int value);
void gld_SetTexturePalette(GLenum target);
void gld_Precache(void);

void SetFrameTextureMode(void);

//gamma
void gld_ResetGammaRamp(void);

//gl_vertex
void gld_SplitLeftEdge(const GLWall *wall, dboolean detail);
void gld_SplitRightEdge(const GLWall *wall, dboolean detail);
void gld_RecalcVertexHeights(const vertex_t *v);

//e6y
void gld_InitGLVersion(void);
void gld_ResetLastTexture(void);

unsigned char* gld_GetTextureBuffer(GLuint texid, int miplevel, int *width, int *height);

int gld_BuildTexture(GLTexture *gltexture, void *data, dboolean readonly, int width, int height);

//hires
extern unsigned int gl_has_hires;
int gld_HiRes_BuildTables(void);
void gld_InitHiRes(void);
int gld_LoadHiresTex(GLTexture *gltexture, int cm);
void gld_GetTextureTexID(GLTexture *gltexture, int cm);
GLuint CaptureScreenAsTexID(void);
void gld_ProgressUpdate(const char * text, int progress, int total);
int gld_ProgressStart(void);
int gld_ProgressEnd(void);

//FBO
#define INVUL_CM         0x00000001
#define INVUL_INV        0x00000002
#define INVUL_BW         0x00000004
extern GLuint glSceneImageFBOTexID;
extern GLuint glSceneImageTextureFBOTexID;

extern unsigned int invul_method;
extern float bw_red;
extern float bw_green;
extern float bw_blue;
extern int SceneInTexture;
void gld_InitFBO(void);
void gld_FreeScreenSizeFBO(void);

//motion bloor
extern int gl_motionblur;

extern int imageformats[];

//missing flats (fake floors and ceilings)

void gld_PreprocessFakeSectors(void);

void gld_SetupFloodStencil(GLWall *wall);
void gld_ClearFloodStencil(GLWall *wall);

void gld_SetupFloodedPlaneCoords(GLWall *wall, gl_strip_coords_t *c);
void gld_SetupFloodedPlaneLight(GLWall *wall);

//light
extern int gl_rellight;
void gld_StaticLightAlpha(float light, float alpha);
#define gld_StaticLight(light) gld_StaticLightAlpha(light, 1.0f)
void gld_InitLightTable(void);
typedef float (*gld_CalcLightLevel_f)(int lightlevel);
typedef float (*gld_Calc2DLightLevel_f)(int lightlevel);
gld_CalcLightLevel_f gld_CalcLightLevel;
gld_Calc2DLightLevel_f gld_Calc2DLightLevel;

//fog
extern int gl_fog;
extern int gl_use_fog;
void gl_EnableFog(int on);
void gld_SetFog(float fogdensity);
typedef float (*gld_CalcFogDensity_f)(sector_t *sector, int lightlevel, GLDrawItemType type);
gld_CalcFogDensity_f gld_CalcFogDensity;

//HQ resize
unsigned char* gld_HQResize(GLTexture *gltexture, unsigned char *inputBuffer, int inWidth, int inHeight, int *outWidth, int *outHeight);

// SkyBox
#define SKY_NONE    0
#define SKY_CEILING 1
#define SKY_FLOOR   2
typedef struct PalEntry_s
{
  unsigned char r, g, b;
} PalEntry_t;
typedef struct SkyBoxParams_s
{
  int index;
  unsigned int type;
  GLWall wall;
  float x_scale, y_scale;
  float x_offset, y_offset;
  // 0 - no colormap; 1 - INVUL inverse colormap
  PalEntry_t FloorSkyColor[2];
  PalEntry_t CeilingSkyColor[2];
  // for BoxSkybox
  side_t *side;
} SkyBoxParams_t;
extern SkyBoxParams_t SkyBox;
extern GLfloat gl_whitecolor[];
void gld_InitSky(void);
void gld_AddSkyTexture(GLWall *wall, int sky1, int sky2, int skytype);
void gld_GetSkyCapColors(void);
void gld_InitFrameSky(void);
void gld_DrawStripsSky(void);
void gld_DrawScreenSkybox(void);
void gld_GetScreenSkyScale(GLWall *wall, float *scale_x, float *scale_y);
void gld_DrawDomeSkyBox(void);
void gld_DrawSkyCaps(void);
int gld_DrawBoxSkyBox(void);

// shadows
void gld_InitShadows(void);
void gld_ProcessThingShadow(mobj_t *mo);
void gld_RenderShadows(void);

// VBO
typedef struct vbo_vertex_s
{
  float x, y, z;
  float u, v;
  unsigned char r, g, b, a;
} PACKEDATTR vbo_vertex_t;
#define NULL_VBO_VERTEX ((vbo_vertex_t*)NULL)
#define sky_vbo_x (gl_ext_arb_vertex_buffer_object ? &NULL_VBO_VERTEX->x : &vbo->data[0].x)
#define sky_vbo_u (gl_ext_arb_vertex_buffer_object ? &NULL_VBO_VERTEX->u : &vbo->data[0].u)
#define sky_vbo_r (gl_ext_arb_vertex_buffer_object ? &NULL_VBO_VERTEX->r : &vbo->data[0].r)

typedef struct vbo_xyz_uv_s
{
  float x, y, z;
  float u, v;
} PACKEDATTR vbo_xyz_uv_t;
extern vbo_xyz_uv_t *flats_vbo;
#define NULL_VBO_XYZ_UV ((vbo_xyz_uv_t*)NULL)
#define flats_vbo_x (gl_ext_arb_vertex_buffer_object ? &NULL_VBO_XYZ_UV->x : &flats_vbo[0].x)
#define flats_vbo_u (gl_ext_arb_vertex_buffer_object ? &NULL_VBO_XYZ_UV->u : &flats_vbo[0].u)

typedef struct vbo_xy_uv_rgba_s
{
  float x, y;
  float u, v;
  unsigned char r, g, b, a;
} PACKEDATTR vbo_xy_uv_rgba_t;

//BoxSkybox
typedef struct box_skybox_s
{
  char name[9];
  int fliptop;
  char faces[6][9];
  GLTexture texture[6];
} box_skybox_t;
box_skybox_t* R_GetBoxSkybox(int index);
void gld_ParseSkybox(void);
extern box_skybox_t *BoxSkybox_default;

// display lists
void gld_InitDisplayLists(void);
void gld_CleanDisplayLists(void);
extern int flats_display_list;

// preprocessing
extern byte *segrendered; // true if sector rendered (only here for malloc)
extern byte *linerendered[2]; // true if linedef rendered (only here for malloc)
extern GLuint flats_vbo_id;

#ifdef USE_SHADERS

typedef struct GLShader_s
{
  char name[256];
  GLhandleARB hShader;
  GLhandleARB hVertProg;
  GLhandleARB hFragProg;

  int lightlevel_index;
} GLShader;

extern GLShader *sh_main;

int glsl_Init(void);
void glsl_SetActiveShader(GLShader *shader);
void glsl_SetLightLevel(float lightlevel);
int glsl_IsActive(void);

#else

#define glsl_Init() 0
#define glsl_SetActiveShader(shader)
#define glsl_SetLightLevel(lightlevel)

#endif // USE_SHADERS

#endif // _GL_INTERN_H
