/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_main.c,v 1.13 2000/05/15 23:45:01 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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

#include "gl_intern.h"
#include "gl_struct.h"

extern int tran_filter_pct;

boolean use_fog=false;
boolean use_maindisplay_list=false;
boolean use_display_lists=false;

GLuint gld_DisplayList=0;

extern void R_DrawPlayerSprites (void);

#define MAP_COEFF 128
#define MAP_SCALE	(MAP_COEFF<<FRACBITS) // 6553600 -- nicolas

#define SCALE_X(x)		((flags & VPT_STRETCH)?((float)x)*(float)SCREENWIDTH/320.0f:(float)x)
#define SCALE_Y(y)		((flags & VPT_STRETCH)?((float)y)*(float)SCREENHEIGHT/200.0f:(float)y)

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags)
{ 
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

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
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  if (flags & VPT_FLIP)
  {
    fU1=(float)gltexture->width/(float)gltexture->tex_width;
    fU2=0.0f;
  }
  else
  {
    fU1=0.0f;
    fU2=(float)gltexture->width/(float)gltexture->tex_width;
  }
  xpos=SCALE_X(x-gltexture->leftoffset);
  ypos=SCALE_Y(y-gltexture->topoffset);
  width=SCALE_X(gltexture->realtexwidth);
  height=SCALE_Y(gltexture->realtexheight);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
		glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
		glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
		glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
	glEnd();
}

void gld_DrawPatchFromMem(int x, int y, const patch_t *patch, int cm, enum patch_translation_e flags)
{ 
  extern int gld_GetTexDimension(int value);
  extern void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy, int cm);

  GLTexture *gltexture;
  unsigned char *buffer;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

  gltexture=(GLTexture *)GLMalloc(sizeof(GLTexture));
  if (!gltexture)
    return;
  gltexture->realtexwidth=patch->width;
  gltexture->realtexheight=patch->height;
  gltexture->leftoffset=patch->leftoffset;
  gltexture->topoffset=patch->topoffset;
  gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
  gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
  gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
  gltexture->buffer_width=gltexture->tex_width;
  gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
  gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
  gltexture->buffer_width=max(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->buffer_height=max(gltexture->realtexheight, gltexture->tex_height);
#endif
  gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
  if (gltexture->realtexwidth>gltexture->buffer_width)
    return;
  if (gltexture->realtexheight>gltexture->buffer_height)
    return;
  buffer=(unsigned char*)GLMalloc(gltexture->buffer_size);
  memset(buffer,0,gltexture->buffer_size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm);
	glGenTextures(1,&gltexture->glTexID[cm]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
#ifdef USE_GLU_IMAGESCALE
  if ((gltexture->buffer_width>gltexture->tex_width) ||
      (gltexture->buffer_height>gltexture->tex_height)
     )
  {
    unsigned char *scaledbuffer;

    scaledbuffer=(unsigned char*)GLMalloc(gltexture->tex_width*gltexture->tex_height*4);
    if (scaledbuffer)
    {
      gluScaleImage(GL_RGBA,
                    gltexture->buffer_width, gltexture->buffer_height,
                    GL_UNSIGNED_BYTE,buffer,
                    gltexture->tex_width, gltexture->tex_height,
                    GL_UNSIGNED_BYTE,scaledbuffer);
      GLFree(buffer);
      buffer=scaledbuffer;
      glTexImage2D( GL_TEXTURE_2D, 0, 4,
                    gltexture->tex_width, gltexture->tex_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  }
  else
#endif /* USE_GLU_IMAGESCALE */
  {
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  gltexture->buffer_width, gltexture->buffer_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  }
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLFree(buffer);

  fV1=0.0f;
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  if (flags & VPT_FLIP)
  {
    fU1=(float)gltexture->width/(float)gltexture->tex_width;
    fU2=0.0f;
  }
  else
  {
    fU1=0.0f;
    fU2=(float)gltexture->width/(float)gltexture->tex_width;
  }
  xpos=SCALE_X(x-gltexture->leftoffset);
  ypos=SCALE_Y(y-gltexture->topoffset);
  width=SCALE_X(gltexture->realtexwidth);
  height=SCALE_Y(gltexture->realtexheight);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
		glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
		glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
		glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
	glEnd();

	glDeleteTextures(1,&gltexture->glTexID[cm]);
  GLFree(gltexture);
}

#undef SCALE_X
#undef SCALE_Y

void gld_DrawBackground(const char* name)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int width,height;

  gltexture=gld_RegisterFlat(R_FlatNumForName(name), false);
  gld_BindFlat(gltexture);
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

void gld_DrawLine(int x0, int y0, int x1, int y1, byte BaseColor)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

	glDisable(GL_TEXTURE_2D);
	glColor3f((float)playpal[3*BaseColor]/255.0f,
			      (float)playpal[3*BaseColor+1]/255.0f,
			      (float)playpal[3*BaseColor+2]/255.0f);
	glBegin(GL_LINES);
		glVertex2i( x0, y0 );
		glVertex2i( x1, y1 );
	glEnd();
	glEnable(GL_TEXTURE_2D);
  W_UnlockLumpName("PLAYPAL");
}

float gld_CalcLightLevel(int lightlevel)
{
  return ((float)max(min(lightlevel,255),0)*(0.75f+0.25f*(float)usegamma))/255.0f;
}

void gld_StaticLight(float light)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    glColor3f(0.5f, 1.0f, 0.0f);
    return;
  }
	else
  	if (player->fixedcolormap)
    {
      glColor3f(1.0f, 1.0f, 1.0f);
      return;
    }
  //glColor4f(light, light, light, 0.5f);
  glColor3f(light, light, light);
}

void gld_StaticLightAlpha(float light, float alpha)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    glColor4f(0.5f, 1.0f, 0.0f, alpha);
    return;
  }
	else
  	if (player->fixedcolormap)
    {
      glColor4f(1.0f, 1.0f, 1.0f, alpha);
      return;
    }
  glColor4f(light, light, light, alpha);
}

void gld_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int x1,y1,x2,y2;
  float scale;
  float light;

  gltexture=gld_RegisterPatch(firstspritelump+weaponlump, CR_DEFAULT);
  if (!gltexture)
    return;
  gld_BindPatch(gltexture, CR_DEFAULT);
  fU1=0;
  fV1=0;
  fU2=(float)gltexture->width/(float)gltexture->tex_width;
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  x1=viewwindowx+vis->x1;
  x2=viewwindowx+vis->x2;
  scale=((float)vis->scale/(float)FRACUNIT);
  y1=viewwindowy+centery-(int)(((float)vis->texturemid/(float)FRACUNIT)*scale);
  y2=y1+(int)((float)gltexture->realtexheight*scale)+1;
  light=gld_CalcLightLevel(lightlevel);

  if (viewplayer->mo->flags & MF_SHADOW)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
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
  if(viewplayer->mo->flags & MF_SHADOW)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor3f(1.0f,1.0f,1.0f);
}

void gld_InitDrawScene(void)
{
  if (use_maindisplay_list)
  {
    if (gld_DisplayList==0)
      gld_DisplayList=glGenLists(1);
    if (gld_DisplayList)
      glNewList(gld_DisplayList,GL_COMPILE);
  }
}

GLvoid gld_Set2DMode()
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

void gld_Finish()
{
  if (use_maindisplay_list)
    if (gld_DisplayList)
    {
      glEndList();
      glCallList(gld_DisplayList);
    }
  gld_Set2DMode();
  glFinish();
	SDL_GL_SwapBuffers();
}

void gld_InitCommandLine()
{
}

/*****************************
 *
 * FLATS
 *
 *****************************/

static FILE *levelinfo;

/* GLLoopDef is the struct for one loop. A loop is a list of vertexes
 * for triangles, which is calculated by the gluTesselator in gld_PrecalculateSector
 * and in gld_PreprocessCarvedFlat
 */
typedef struct
{
  float u,v;
  float x,y;
} GLVertex;

typedef struct
{
  GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
  int vertexcount; // number of vertexes in this loop
  GLVertex *gl_vertexes; // list of pointers to Doom's vertexes
} GLLoopDef;

// GLSector is the struct for a sector with a list of loops.

typedef struct
{
  int loopcount; // number of loops for this sector
  GLLoopDef *loops; // the loops itself
  GLuint gl_list;
} GLSector;

// this is the list for all sectors to the loops
static GLSector *sectorloops=NULL;

// gld_DrawFlat
//
// This draws on flat for the sector "num"
// The ceiling boolean indicates if the flat is a floor(false) or a ceiling(true)

static void gld_DrawFlat(int num, boolean ceiling, visplane_t *plane)
{
  int loopnum; // current loop number
  GLLoopDef *currentloop; // the current loop
  int vertexnum; // the current vertexnumber
  GLVertex *currentvertex; // the current vertex
  sector_t *sector; // the sector we want to draw
  sector_t tempsec; // needed for R_FakeFlat
  float light; // the lightlevel of the flat
  GLTexture *gltexture; // the texture
  float uoffs,voffs; // the texture coordinates
  float z; // the z position of the flat (height)
  int floorlightlevel;      // killough 3/16/98: set floor lightlevel
  int ceilinglightlevel;    // killough 4/11/98

  sector=&sectors[num]; // get the sector
  sector=R_FakeFlat(sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false); // for boom effects
  if (!ceiling) // if it is a floor ...
    glCullFace(GL_FRONT);
  else
    glCullFace(GL_BACK);
  if (!ceiling) // if it is a floor ...
  {
    if (sector->floorpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    gltexture=gld_RegisterFlat(flattranslation[sector->floorpic], true);
    if (!gltexture)
      return;
    // get the lightlevel
    light=gld_CalcLightLevel(floorlightlevel+(extralight<<LIGHTSEGSHIFT));
    // calculate texture offsets
    uoffs=(float)sector->floor_xoffs/(float)FRACUNIT;
    voffs=(float)sector->floor_yoffs/(float)FRACUNIT;
    // get height
    z=(float)sector->floorheight/(float)MAP_SCALE;
  }
  else // if it is a ceiling ...
  {
    if (sector->ceilingpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    gltexture=gld_RegisterFlat(flattranslation[sector->ceilingpic], true);
    if (!gltexture)
      return;
    // get the lightlevel
    light=gld_CalcLightLevel(ceilinglightlevel+(extralight<<LIGHTSEGSHIFT));
    // calculate texture offsets
    uoffs=(float)sector->ceiling_xoffs/(float)FRACUNIT;
    voffs=(float)sector->ceiling_yoffs/(float)FRACUNIT;
    // get height
    z=(float)sector->ceilingheight/(float)MAP_SCALE;
  }
  if (plane)
  {
    z=(float)plane->height/(float)MAP_SCALE;
    light=gld_CalcLightLevel(plane->lightlevel+(extralight<<LIGHTSEGSHIFT));
  }
  gld_BindFlat(gltexture);
  gld_StaticLight(light);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(uoffs/64.0f,voffs/64.0f,0.0f);
  // go through all loops of this sector
  if (!sectorloops[num].gl_list)
  {
    if (use_display_lists)
    {
      sectorloops[num].gl_list=glGenLists(1);
      if (!sectorloops[num].gl_list)
      {
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        return;
      }
      glNewList(sectorloops[num].gl_list,GL_COMPILE_AND_EXECUTE);
    }
    for (loopnum=0; loopnum<sectorloops[num].loopcount; loopnum++)
    {
      // set the current loop
      currentloop=&sectorloops[num].loops[loopnum];
      if (!currentloop)
        continue;
      // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
      glBegin(currentloop->mode);
      // go through all vertexes of this loop
      for (vertexnum=0; vertexnum<currentloop->vertexcount; vertexnum++)
      {
        // set the current vertex
        currentvertex=&currentloop->gl_vertexes[vertexnum];
        if (!currentvertex)
          continue;
        // set texture coordinate of this vertex
        glTexCoord2f(currentvertex->u,currentvertex->v);
        // set vertex coordinate
        glVertex3f(currentvertex->x,0.0f,currentvertex->y);
      }
      // end of loop
      glEnd();
    }
    if (use_display_lists)
      glEndList();
  }
  else
    glCallList(sectorloops[num].gl_list);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

static boolean *sectorclosed=NULL; // list of all sectors with true if sector closed
static boolean *sectorrendered=NULL; // true if sector rendered (only here for malloc)

/* proff - 05/15/2000
 * The idea and algorithm to compute the flats with nodes and subsectors is
 * originaly from JHexen. I have redone it.
 */

#define FIX2DBL(x)		((double)(x))
#define MAX_CC_SIDES	64

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
static vertex_t *gld_FlatEdgeClipper(int *numpoints, vertex_t *points, int numclippers, divline_t *clippers)
{
	unsigned char	sidelist[MAX_CC_SIDES];
	int				i, k, num = *numpoints;

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
			if(endIdx == num) endIdx = 0;	// Wrap-around.
			// Clipping will happen when the ends are on different sides.
			if(sidelist[startIdx] != sidelist[endIdx])
			{
				vertex_t newvert;

        gld_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

				// Add the new vertex. Also modify the sidelist.
				points = (vertex_t*)GLRealloc(points,(++num)*sizeof(vertex_t));
				if(num >= MAX_CC_SIDES) I_Error("Too many points in carver.\n");

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

static void gld_FlatConvexCarver(subsector_t *ssec, int num, divline_t *list)
{
	int numclippers = num+ssec->numlines;
	divline_t *clippers;
	int i, numedgepoints;
	vertex_t *edgepoints;
	
  if (sectorclosed[ssec->sector->iSectorID])
    return;
  clippers=(divline_t*)GLMalloc(numclippers*sizeof(divline_t));
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
	edgepoints = (vertex_t*)GLMalloc(numedgepoints*sizeof(vertex_t));
	
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
      GLVertex *gl_vertexes;

      gl_vertexes=GLMalloc(numedgepoints*sizeof(GLVertex));
      if (gl_vertexes)
      {
        int currentsector=ssec->sector->iSectorID;

        sectorloops[ currentsector ].loopcount++;
        sectorloops[ currentsector ].loops=GLRealloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount);
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=GL_TRIANGLE_FAN;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=numedgepoints;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=gl_vertexes;

	      for(i = 0;  i < numedgepoints; i++)
	      {
		      gl_vertexes[i].u = ( (float)edgepoints[i].x/(float)FRACUNIT)/64.0f;
		      gl_vertexes[i].v = (-(float)edgepoints[i].y/(float)FRACUNIT)/64.0f;
          gl_vertexes[i].x = -(float)edgepoints[i].x/(float)MAP_SCALE;
          gl_vertexes[i].y =  (float)edgepoints[i].y/(float)MAP_SCALE;
	      }
      }
    }
	}
	// We're done, free the edgepoints memory.
	GLFree(edgepoints);
  GLFree(clippers);
}

static void gld_CarveFlats(int bspnode, int numdivlines, divline_t *divlines)
{
	node_t		*nod;
	divline_t	*childlist, *dl;
	int			childlistsize = numdivlines+1;
	
	// If this is a subsector we are dealing with, begin carving with the
	// given list.
	if(bspnode & NF_SUBSECTOR)
	{
		// We have arrived at a subsector. The divline list contains all
		// the partition lines that carve out the subsector.
		int ssidx = bspnode & (~NF_SUBSECTOR);
		gld_FlatConvexCarver(subsectors+ssidx, numdivlines, divlines);
		return;
	}

	// Get a pointer to the node.
	nod = nodes + bspnode;

	// Allocate a new list for each child.
	childlist = (divline_t*)GLMalloc(childlistsize*sizeof(divline_t));

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
	GLFree(childlist);
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
  sectorloops[ currentsector ].loops=GLRealloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount);
  // set initial values for current loop
  // currentloop is -> sectorloops[currentsector].loopcount-1
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=type;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=0;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=NULL;
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
    if (vert[1]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[1]->x>>FRACBITS, vert[1]->y>>FRACBITS, vert[1]);
    if (vert[2]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[2]->x>>FRACBITS, vert[2]->y>>FRACBITS, vert[2]);
    if (vert[3]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[3]->x>>FRACBITS, vert[3]->y>>FRACBITS, vert[3]);
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
  GLVertex *gl_vertexes;
  int vertexcount;

#ifdef _DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tVertex : x %10i, y %10i\n", vert->x>>FRACBITS, vert->y>>FRACBITS);
#endif
  // set vertexcount and vertexes variables to the values for the current loop, so the
  // rest of this function isn't to complicated
  vertexcount=sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount;
  gl_vertexes=sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes;

  // increase vertex count
  vertexcount++;
  // realloc memory to get space for new vertex
  gl_vertexes=GLRealloc(gl_vertexes, vertexcount*sizeof(GLVertex));
  // add the new vertex (vert is the second argument of gluTessVertex)
  gl_vertexes[vertexcount-1].u=( (float)vert->x/(float)FRACUNIT)/64.0f;
  gl_vertexes[vertexcount-1].v=(-(float)vert->y/(float)FRACUNIT)/64.0f;
  gl_vertexes[vertexcount-1].x=-(float)vert->x/(float)MAP_SCALE;
  gl_vertexes[vertexcount-1].y= (float)vert->y/(float)MAP_SCALE;

  // set values of the loop to new values
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=vertexcount;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=gl_vertexes;
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
  vertex_t *currentvertex;
  angle_t lineangle;
  angle_t angle;
  angle_t bestangle;
  sector_t *backsector;
  GLUtesselator *tess;
  GLdouble v[1000][3];
  int vertexnum;

  currentsector=num;
  lineadded=GLRealloc(lineadded,sectors[num].linecount*sizeof(boolean));
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
    GLFree(lineadded);
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
    if (sectors[num].lines[i]->sidenum[0]>=0)
      if (sectors[num].lines[i]->sidenum[1]>=0)
        if (sides[sectors[num].lines[i]->sidenum[0]].sector
          ==sides[sectors[num].lines[i]->sidenum[1]].sector)
        {
          lineadded[i]=true;
          if (levelinfo) fprintf(levelinfo, "line %4i (iLineID %4i) has both sides in same sector (removed)\n", i, sectors[num].lines[i]->iLineID);
        }
  }
  // initialize variables
  linecount=sectors[num].linecount;
  oldline=0;
  currentline=0;
  startvertex=sectors[num].lines[currentline]->v2;
  currentloop=0;
  vertexnum=0;
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
          if ((sectors[num].lines[currentline]->sidenum[0]!=-1) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
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
    if ((sectors[num].lines[currentline]->sidenum[0]!=-1) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
    {
      // v2 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v2;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y,sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      if (lineangle>=180)
        lineangle=lineangle-360;
      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped false\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    else // ... or on the back side
    {
      // v1 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v1;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y,sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      if (lineangle>=180)
        lineangle=lineangle-360;
      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped true\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    // calculate coordinates for the glu tesselation functions
    v[vertexnum][0]=-(double)currentvertex->x/(double)MAP_SCALE;
    v[vertexnum][1]= 0.0;
    v[vertexnum][2]= (double)currentvertex->y/(double)MAP_SCALE;
    // add the vertex to the tesselator, currentvertex is the pointer to the vertexlist of doom
    // v[vertexnum] is the GLdouble array of the current vertex
    if (levelinfo) fprintf(levelinfo, "\t\tgluTessVertex(%i, %i)\n",currentvertex->x>>FRACBITS,currentvertex->y>>FRACBITS);
    gluTessVertex(tess, v[vertexnum], currentvertex);
    // increase vertexindex
    vertexnum++;
    // decrease linecount of current sector
    linecount--;
    // find the next line
    oldline=currentline; // if this isn't changed at the end of the search, a new loop will start
    bestline=-1; // set to start values
    bestlinecount=0;
    // set backsector if there is one
    if (sectors[num].lines[currentline]->sidenum[1]!=-1)
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
          if ((sectors[num].lines[i]->sidenum[0]!=-1) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
            angle = R_PointToAngle2(sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y,sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y);
          else
            angle = R_PointToAngle2(sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y,sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y);
          angle=(angle>>ANGLETOFINESHIFT)*360/8192;
          if (angle>=180)
            angle=angle-360;
          // check if line is flipped ...
          if ((sectors[num].lines[i]->sidenum[0]!=-1) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
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
            if (abs(lineangle-angle)<abs(bestangle))
            {
              bestline=i;
              bestangle=lineangle-angle;
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
  GLFree(lineadded);
}

#endif /* USE_GLU_TESS */

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
#ifdef USE_GLU_TESS
  int i;
  char *vertexcheck;
  int v1num;
  int v2num;
  int j;
#endif

#ifdef _DEBUG
  char buffer[MAX_PATH];

  levelinfo=fopen("levelinfo.txt","r");
  if (levelinfo)
  {
    fscanf(levelinfo,"%s\n",&buffer);
    fclose(levelinfo);
  }
  levelinfo=fopen("levelinfo.txt","w");
  if (levelinfo)
  {
    fprintf(levelinfo,"%s\n",&buffer);
    if (gamemode==commercial)
      fprintf(levelinfo,"MAP%02i\n",gamemap);
    else
      fprintf(levelinfo,"E%iM%i\n",gameepisode,gamemap);
  }
#endif

  GLFree(sectorclosed);
  sectorclosed=GLMalloc(numsectors*sizeof(boolean));
  memset(sectorclosed, 0, sizeof(boolean)*numsectors);
  GLFree(sectorrendered);
  sectorrendered=GLMalloc(numsectors*sizeof(boolean));
  GLFree(sectorloops);
  sectorloops=GLMalloc(sizeof(GLSector)*numsectors);
  memset(sectorloops, 0, sizeof(GLSector)*numsectors);

#ifdef USE_GLU_TESS
  vertexcheck=GLMalloc(numvertexes*sizeof(char));
  if (!vertexcheck)
  {
    if (levelinfo) fclose(levelinfo);
    return;
  }

  for (i=0; i<numsectors; i++)
  {
    memset(vertexcheck,0,numvertexes*sizeof(char));
    for (j=0; j<sectors[i].linecount; j++)
    {
      v1num=((int)sectors[i].lines[j]->v1-(int)vertexes)/sizeof(vertex_t);
      v2num=((int)sectors[i].lines[j]->v2-(int)vertexes)/sizeof(vertex_t);
      if ((v1num>=numvertexes) || (v2num>=numvertexes))
        continue;
      if (sectors[i].lines[j]->sidenum[0]>=0)
        if (sides[sectors[i].lines[j]->sidenum[0]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=1;
          vertexcheck[v2num]|=2;
        }
      if (sectors[i].lines[j]->sidenum[1]>=0)
        if (sides[sectors[i].lines[j]->sidenum[1]].sector==&sectors[i])
        {
          vertexcheck[v1num]|=2;
          vertexcheck[v2num]|=1;
        }
    }
    if (sectors[i].linecount<3)
    {
      lprintf(LO_ERROR, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      if (levelinfo) fprintf(levelinfo, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      sectorclosed[i]=false;
    }
    else
    {
      sectorclosed[i]=true;
      for (j=0; j<numvertexes; j++)
      {
        if ((vertexcheck[j]==1) || (vertexcheck[j]==2))
        {
          lprintf(LO_ERROR, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
          if (levelinfo) fprintf(levelinfo, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
          sectorclosed[i]=false;
        }
      }
    }
    if (sectorclosed[i])
      gld_PrecalculateSector(i);
  }
  GLFree(vertexcheck);
#endif /* USE_GLU_TESS */

  gld_CarveFlats(numnodes-1, 0, 0);
  if (levelinfo) fclose(levelinfo);
}

void gld_DrawSky(float yaw)
{
  GLTexture *gltexture;
  float u,ou,v;

  gltexture=gld_RegisterTexture(skytexture, false);
  gld_BindTexture(gltexture);
  v=128.0f/(float)gltexture->tex_height*1.6f;
  u=256.0f/(float)gltexture->tex_width;
  ou=u*(yaw/90.0f);
  glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(ou     , 0.0f);  glVertex2f( 0.0f, 0.0f);
   	glTexCoord2f(ou     , v);     glVertex2f( 0.0f, (float)SCREENHEIGHT);
	  glTexCoord2f(ou+u, 0.0f);  glVertex2f( (float)SCREENWIDTH, 0.0f);
	  glTexCoord2f(ou+u, v);     glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
  glEnd();
}

static float roll     = 0.0f;
static float yaw      = 0.0f;
static float inv_yaw  = 0.0f;
static float pitch    = 0.0f;

void gld_StartDrawScene(void)
{
  float trZ = -5.0f;
  float trY ;
  float xCamera,yCamera;

  extern int screenblocks;
  int height;

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
	xCamera=-(float)viewx/(float)MAP_SCALE;
	yCamera=(float)viewy/(float)MAP_SCALE;
	trY=(float)viewz/(float)MAP_SCALE;
	
	yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	inv_yaw=-90.0f+(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	// SKY
	//gld_StaticLight3f(1.0f,1.0f,1.0f);
	//gld_DrawSky(yaw);

	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

//	gluPerspective(64.0f, 320.0f/200.0f, 0.01f, 2*fSkyRadius);
	gluPerspective(64.0f, 320.0f/200.0f, 0.05f, 64.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
  glRotatef(roll,  0.0f, 0.0f, 1.0f);
	glRotatef(pitch, 1.0f, 0.0f, 0.0f);
	glRotatef(yaw,   0.0f, 1.0f, 0.0f);
	glTranslatef(-xCamera, -trY, -yCamera);

  if (use_fog)
	  glEnable(GL_FOG);
  else
    glDisable(GL_FOG);
  // set all sectors to not rendered
  memset(sectorrendered, 0, numsectors*sizeof(boolean));
}

static float extra_red=0.0f;
static float extra_green=0.0f;
static float extra_blue=0.0f;
static float extra_alpha=0.0f;

void gld_EndDrawScene(void)
{
	glDisable(GL_POLYGON_SMOOTH);

	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT); 
	glDisable(GL_FOG); 
	gld_Set2DMode();

	if (viewangleoffset <= 1024<<ANGLETOFINESHIFT || 
	 	viewangleoffset >=-1024<<ANGLETOFINESHIFT)
  {	// don't draw on side views
		R_DrawPlayerSprites ();
	}
  if (extra_alpha>0.0f)
  {
    glDisable(GL_ALPHA_TEST);
	  glColor4f(extra_red, extra_green, extra_blue, extra_alpha);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_STRIP);
  		glVertex2f( 0.0f, 0.0f);
	  	glVertex2f( 0.0f, (float)SCREENHEIGHT);
		  glVertex2f( (float)SCREENWIDTH, 0.0f);
		  glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
  }

	glColor3f(1.0f,1.0f,1.0f);
  glDisable(GL_SCISSOR_TEST);
}

int fog_density=200;

void gld_Init(int width, int height)
{ 
  GLfloat params[4]={0.0f,0.0f,1.0f,0.0f};
	GLfloat BlackFogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT); 

	glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
  glClearDepth(1.0f);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gld_max_texturesize);
  //gld_max_texturesize=1;
  lprintf(LO_INFO,"gld_max_texturesize=%i\n",gld_max_texturesize);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // proff_dis
  glShadeModel(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);
 	glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL,0.5f);
	glDisable(GL_CULL_FACE);

  glTexGenfv(GL_Q,GL_EYE_PLANE,params);
  glTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glFogi (GL_FOG_MODE, GL_EXP);
	glFogfv(GL_FOG_COLOR, BlackFogColor);
	glFogf (GL_FOG_DENSITY, (float)fog_density/1000.0f);
	glHint (GL_FOG_HINT, GL_NICEST);
	glFogf (GL_FOG_START, 0.0f);
	glFogf (GL_FOG_END, 1.0f);
}

void gld_SetPalette(int palette)
{
  extra_red=0.0f;
  extra_green=0.0f;
  extra_blue=0.0f;
  extra_alpha=0.0f;
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

void gld_FillBlock(int x, int y, int width, int height, int col)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

	glDisable(GL_TEXTURE_2D);
	glColor3f((float)playpal[3*col]/255.0f,
			      (float)playpal[3*col+1]/255.0f,
			      (float)playpal[3*col+2]/255.0f);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i( x, y );
		glVertex2i( x, y+height );
		glVertex2i( x+width, y );
		glVertex2i( x+width, y+height );
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
  W_UnlockLumpName("PLAYPAL");
}

/*****************
 *               *
 * Walls         *
 *               *
 *****************/

typedef struct
{
  float x1,x2;
  float z1,z2;
  float linelength;
} GLSeg;

GLSeg *gl_segs=NULL;

typedef struct
{
  int segnum;
  float ytop,ybottom;
  float ul,ur,vt,vb;
  float ou,ov;
  float light;
  float lineheight;
  boolean trans;
  boolean sky;
  boolean skyflip;
  float skyymid;
  float skyyaw;
  GLTexture *gltexture;
} GLWall;

static void gld_DrawWall(GLWall *wall)
{
  GLSeg *glseg=&gl_segs[wall->segnum];

  if (wall->gltexture)
  {
    gld_BindTexture(wall->gltexture);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    if (wall->trans)
      gld_StaticLightAlpha(wall->light, (float)tran_filter_pct/100.0f);
    else
      gld_StaticLight(wall->light);
  }
  else
  {
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f,0.0f,0.0f,1.0f);
  }
  if (wall->sky)
  {
    if (wall->gltexture)
    {
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_Q);
      glMatrixMode(GL_TEXTURE);
      glPushMatrix();
      if (wall->skyflip)
        glScalef(-128.0f/(float)wall->gltexture->buffer_width,200.0f/320.0f*2.0f,1.0f);
      else
        glScalef(128.0f/(float)wall->gltexture->buffer_width,200.0f/320.0f*2.0f,1.0f);
      glTranslatef(-2.0f*(wall->skyyaw/90.0f),200.0f/320.0f*(wall->skyymid/100.0f),0.0f);
    }
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBegin(GL_TRIANGLE_STRIP);
      glVertex3f(glseg->x1,wall->ytop,glseg->z1);
      glVertex3f(glseg->x1,wall->ybottom,glseg->z1);
      glVertex3f(glseg->x2,wall->ytop,glseg->z2);
      glVertex3f(glseg->x2,wall->ybottom,glseg->z2);
    glEnd();
    if (wall->gltexture)
    {
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glDisable(GL_TEXTURE_GEN_Q);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_S);
    }
  }
  else
  {
    glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(wall->ul+wall->ou,wall->vt+wall->ov); glVertex3f(glseg->x1,wall->ytop,glseg->z1);
      glTexCoord2f(wall->ul+wall->ou,wall->vb+wall->ov); glVertex3f(glseg->x1,wall->ybottom,glseg->z1);
      glTexCoord2f(wall->ur+wall->ou,wall->vt+wall->ov); glVertex3f(glseg->x2,wall->ytop,glseg->z2);
      glTexCoord2f(wall->ur+wall->ou,wall->vb+wall->ov); glVertex3f(glseg->x2,wall->ybottom,glseg->z2);
    glEnd();
  }
  if (!wall->gltexture)
    glEnable(GL_TEXTURE_2D);
}

#define LINE seg->linedef
#define CALC_Y_VALUES(walldef, floor_height, ceiling_height)\
  (walldef).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+0.001f;\
  (walldef).ybottom=((float)(floor_height)/(float)MAP_SCALE)-0.001f;\
  (walldef).lineheight=(float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT))

#define CALC_TEX_VALUES_TOP(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  /*(w).ov=0.0f;*/\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_MIDDLE2S(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=0.0f;\
  /*(w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;*/\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, v_offset)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->realtexwidth;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height),\
    (w).ov-=((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define SKYTEXTURE(sky1,sky2)\
      if ((sky1) & PL_SKYFLAT)\
      {\
	      const line_t *l = &lines[sky1 & ~PL_SKYFLAT];\
	      const side_t *s = *l->sidenum + sides;\
        wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	      wall.skyyaw=270.0f-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;\
	      wall.skyymid = (float)s->rowoffset/(float)FRACUNIT - 28.0f;\
	      wall.skyflip = l->special==272 ? false : true;\
      }\
      else\
      if ((sky2) & PL_SKYFLAT)\
      {\
	      const line_t *l = &lines[sky2 & ~PL_SKYFLAT];\
	      const side_t *s = *l->sidenum + sides;\
        wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	      wall.skyyaw=270.0f-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;\
	      wall.skyymid = (float)s->rowoffset/(float)FRACUNIT - 28.0f;\
	      wall.skyflip = l->special==272 ? false : true;\
      }\
      else\
      {\
        wall.gltexture=gld_RegisterTexture(skytexture, false);\
	      wall.skyyaw=yaw;\
	      wall.skyymid = 100.0f;\
	      wall.skyflip = false;\
      }

void gld_AddWall(seg_t *seg)
{
  GLWall wall;
  GLTexture *temptex;
  sector_t *frontsector;
  sector_t *backsector;
  sector_t ftempsec; // needed for R_FakeFlat
  sector_t btempsec; // needed for R_FakeFlat

  if (!seg->frontsector)
    return;
  frontsector=R_FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
  if (!frontsector)
    return;
  wall.segnum=seg->iSegID;
  wall.light=gld_CalcLightLevel(frontsector->lightlevel+(extralight<<LIGHTSEGSHIFT));
  wall.gltexture=NULL;
  wall.trans=0;

  if (!seg->backsector) /* onesided */
  {
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      wall.ybottom=(float)frontsector->ceilingheight/(float)MAP_SCALE;
      SKYTEXTURE(frontsector->sky,frontsector->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ytop=(float)frontsector->floorheight/(float)MAP_SCALE;
      wall.ybottom=-255.0f;
      SKYTEXTURE(frontsector->sky,frontsector->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
    if (temptex)
    {
      wall.gltexture=temptex;
      CALC_Y_VALUES(wall, frontsector->floorheight, frontsector->ceilingheight);
      CALC_TEX_VALUES_MIDDLE1S(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0);
      wall.sky=false;
      gld_DrawWall(&wall);
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
      if (   (floor_height==backsector->floorheight)
          && (backsector->ceilingpic==skyflatnum)
          && (   (texturetranslation[seg->sidedef->toptexture]==R_TextureNumForName("-"))
              || (texturetranslation[seg->sidedef->bottomtexture]==R_TextureNumForName("-"))
              || (texturetranslation[seg->sidedef->midtexture]==R_TextureNumForName("-"))
             )
         )
      {
        wall.ybottom=(float)backsector->floorheight/(float)MAP_SCALE;
        SKYTEXTURE(frontsector->sky,backsector->sky);
        wall.sky=true;
        gld_DrawWall(&wall);
      }
      else
        if (backsector->ceilingpic!=skyflatnum)
        {
          wall.ybottom=(float)ceiling_height/(float)MAP_SCALE;
          SKYTEXTURE(frontsector->sky,backsector->sky);
          wall.sky=true;
          gld_DrawWall(&wall);
        }
    }
    if (floor_height<ceiling_height)
    {
      if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
      {
        temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->toptexture], true);
        if (temptex)
        {
          wall.gltexture=temptex;
          CALC_Y_VALUES(wall, floor_height, ceiling_height);
          CALC_TEX_VALUES_TOP(wall, seg, (LINE->flags & (ML_DONTPEGBOTTOM | ML_DONTPEGTOP))==0);
          wall.sky=false;
          gld_DrawWall(&wall);
        }
      }
    }
    
    /* midtexture */
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
    if (temptex)
    {
      wall.gltexture=temptex;
      if ( (LINE->flags & ML_DONTPEGBOTTOM) >0)
      {
        if (backsector->ceilingheight<=frontsector->floorheight)
          goto bottomtexture;
        floor_height=max(frontsector->floorheight,backsector->floorheight)+(seg->sidedef->rowoffset);
        ceiling_height=floor_height+(wall.gltexture->realtexheight<<FRACBITS);
      }
      else
      {
        if (backsector->ceilingheight<=frontsector->floorheight)
          goto bottomtexture;
        ceiling_height=min(frontsector->ceilingheight,backsector->ceilingheight)+(seg->sidedef->rowoffset);
        floor_height=ceiling_height-(wall.gltexture->realtexheight<<FRACBITS);
      }
      CALC_Y_VALUES(wall, floor_height, ceiling_height);
      CALC_TEX_VALUES_MIDDLE2S(wall, seg, /*(LINE->flags & ML_DONTPEGBOTTOM)>0*/false);
      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.trans=1;
      wall.sky=false;
      gld_DrawWall(&wall);
    }
bottomtexture:
    /* bottomtexture */
    ceiling_height=backsector->floorheight;
    floor_height=frontsector->floorheight;
    if ((frontsector->floorpic==skyflatnum) && (backsector->floorpic!=skyflatnum))
    {
      wall.ytop=(float)floor_height/(float)MAP_SCALE;
      wall.ybottom=-255.0f;
      SKYTEXTURE(frontsector->sky,backsector->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    if (floor_height<ceiling_height)
    {
      temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->bottomtexture], true);
      if (temptex)
      {
        wall.gltexture=temptex;
        CALC_Y_VALUES(wall, floor_height, ceiling_height);
        CALC_TEX_VALUES_BOTTOM(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0, floor_height-frontsector->ceilingheight);
        wall.sky=false;
        gld_DrawWall(&wall);
      }
    }
  }
}

#undef LINE
#undef FRONTSECTOR
#undef BACKSECTOR
#undef CALC_Y_VALUES
#undef CALC_TEX_VALUES_TOP
#undef CALC_TEX_VALUES_MIDDLE
#undef CALC_TEX_VALUES_BOTTOM
#undef SKYTEXTURE

static void gld_PreprocessSegs(void)
{
  int i;

  GLFree(gl_segs);
  gl_segs=GLMalloc(numsegs*sizeof(GLSeg));
  for (i=0; i<numsegs; i++)
  {
    float dx,dy;

    gl_segs[i].x1=-(float)segs[i].v1->x/(float)MAP_SCALE;
    gl_segs[i].z1= (float)segs[i].v1->y/(float)MAP_SCALE;
    gl_segs[i].x2=-(float)segs[i].v2->x/(float)MAP_SCALE;
    gl_segs[i].z2= (float)segs[i].v2->y/(float)MAP_SCALE;
    dx=((float)segs[i].v2->x/(float)FRACUNIT)-((float)segs[i].v1->x/(float)FRACUNIT);
    dy=((float)segs[i].v2->y/(float)FRACUNIT)-((float)segs[i].v1->y/(float)FRACUNIT);
    gl_segs[i].linelength=(float)sqrt(dx*dx+dy*dy);
  }
}

/*****************
 *               *
 * Flats         *
 *               *
 *****************/

void gld_DrawPlane(sector_t *sector, visplane_t *floorplane, visplane_t *ceilingplane)
{
  // check if all arrays are allocated
  if (!sectorclosed)
    return;
  if (!sectorrendered)
    return;

  if (!sectorrendered[sector->iSectorID]) // if not already rendered
  {
    // enable backside removing
    glEnable(GL_CULL_FACE);
    // render the floor
    gld_DrawFlat(sector->iSectorID, false, floorplane);
    // render the ceiling
    gld_DrawFlat(sector->iSectorID, true, ceilingplane);
    // set rendered true
    sectorrendered[sector->iSectorID]=true;
    // disable backside removing
    glDisable(GL_CULL_FACE);
  }
}

/*****************
 *               *
 * Sprites       *
 *               *
 *****************/

void gld_DrawSprite(vissprite_t *vspr)
{
  mobj_t *pSpr=vspr->thing;
	GLTexture *gltexture;
  int cm;
  boolean shadow;
  boolean trans;
  float x,y,z;
  float vt,vb;
  float ul,ur;
  float w,h;
  float voff,hoff;
	float light;

	if (pSpr->frame & FF_FULLBRIGHT)
		light = 1.0f;
	else
		light = gld_CalcLightLevel(pSpr->subsector->sector->lightlevel+(extralight<<LIGHTSEGSHIFT));
  cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  gltexture=gld_RegisterPatch(vspr->patch+firstspritelump,cm);
  if (!gltexture)
    return;
  shadow = (pSpr->flags & MF_SHADOW) != 0;
  trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
  x=-(float)pSpr->x/(float)MAP_SCALE;
  y= (float)pSpr->z/(float)MAP_SCALE;
  z= (float)pSpr->y/(float)MAP_SCALE;
  vt=0.0f;
  vb=(float)gltexture->height/(float)gltexture->tex_height;
  if (vspr->flip)
  {
    ul=0.0f;
    ur=(float)gltexture->width/(float)gltexture->tex_width;
  }
  else
  {
    ul=(float)gltexture->width/(float)gltexture->tex_width;
    ur=0.0f;
  }
  hoff=(float)gltexture->leftoffset/(float)(MAP_COEFF);
  voff=(float)gltexture->topoffset/(float)(MAP_COEFF);
  w=(float)gltexture->realtexwidth/(float)(MAP_COEFF);
  h=(float)gltexture->realtexheight/(float)(MAP_COEFF);
  gld_BindPatch(gltexture,cm);
  glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(x,y,z);
	glRotatef(inv_yaw,0.0f,1.0f,0.0f);
	if(shadow)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
  }
  else
  {
		if(trans)
      gld_StaticLightAlpha(light,(float)tran_filter_pct/100.0f);
		else
      gld_StaticLight(light);
  }
  glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(ul, vt); glVertex3f( hoff-w, voff  , 0.0f);
		glTexCoord2f(ur, vt); glVertex3f( hoff  , voff  , 0.0f);
		glTexCoord2f(ul, vb); glVertex3f( hoff-w, voff-h, 0.0f);
		glTexCoord2f(ur, vb); glVertex3f( hoff  , voff-h, 0.0f);
	glEnd();
		
  glPopMatrix();

	if(shadow)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gld_DrawScene(player_t *player)
{
  extern drawseg_t	*drawsegs, *ds_p;
	drawseg_t *ds;

  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
	{
    gld_AddWall(ds->curline);
	}
}

void gld_PreprocessLevel(void)
{
#ifdef _DEBUG
  void gld_Precache(void);

  //gld_Precache();
#endif
  gld_PreprocessSectors();
  gld_PreprocessSegs();
}
