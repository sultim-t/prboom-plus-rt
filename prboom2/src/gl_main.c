/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_main.c,v 1.8 2000/05/13 05:54:04 jessh Exp $
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

GLuint gld_DisplayList=0;

extern int tran_filter_pct;

boolean use_fog=false;

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
  width=SCALE_X(gltexture->width);
  height=SCALE_Y(gltexture->height);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
		glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
		glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
		glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
	glEnd();
}

void gld_DrawPatchFromMem(int x, int y, const patch_t *patch, int cm, enum patch_translation_e flags)
{ 
  extern int gld_GetTexHeightGL(int value);
  extern void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy, int cm);

  GLTexture *gltexture;
  int size;
  unsigned char *buffer;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

  gltexture=(GLTexture *)GLMalloc(sizeof(GLTexture));
  if (!gltexture)
    return;
  gltexture->width=patch->width;
  gltexture->height=patch->height;
  gltexture->leftoffset=patch->leftoffset;
  gltexture->topoffset=patch->topoffset;
  gltexture->tex_width=gld_GetTexHeightGL(gltexture->width);
  gltexture->tex_height=gld_GetTexHeightGL(gltexture->height);
  size=gltexture->tex_width*gltexture->tex_height*4;
  buffer=(unsigned char*)GLMalloc(size);
  memset(buffer,0,size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm);
	glGenTextures(1,&gltexture->glTexID[cm]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                gltexture->tex_width, gltexture->tex_height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
  width=SCALE_X(gltexture->width);
  height=SCALE_Y(gltexture->height);
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

  gltexture=gld_RegisterFlat(firstflat+R_FlatNumForName(name), false);
  gld_BindFlat(gltexture);
  if (!gltexture)
    return;
  fU1=0;
  fV1=0;
  fU2=(float)SCREENWIDTH/(float)gltexture->width;
  fV2=(float)SCREENHEIGHT/(float)gltexture->height;
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
  // return (float)max(min(lightlevel,256),0)/256.0f;
  return ((float)max(min(lightlevel,255),0)*(0.75f+0.25f*(float)usegamma))/255.0f;
}

void gld_StaticLight3f(GLfloat fRed, GLfloat fGreen, GLfloat fBlue)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    fRed=1.0f;
    fGreen=1.0f;
    fBlue=0.0f;
  }
	else
  	if (player->fixedcolormap)
    {
      fRed=1.0f;
      fGreen=1.0f;
      fBlue=1.0f;
    }
  //glColor4f(fRed, fGreen, fBlue, 0.5f);
  glColor3f(fRed, fGreen, fBlue);
}

void gld_StaticLight4f(GLfloat fRed, GLfloat fGreen, GLfloat fBlue, GLfloat fAlpha)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    fRed=1.0f;
    fGreen=1.0f;
    fBlue=0.0f;
  }
	else
  	if (player->fixedcolormap)
    {
      fRed=1.0f;
      fGreen=1.0f;
      fBlue=1.0f;
    }
  glColor4f(fRed, fGreen, fBlue, fAlpha);
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
  y2=y1+(int)((float)gltexture->height*scale)+1;
  light=gld_CalcLightLevel(lightlevel);

  if(viewplayer->mo->flags & MF_SHADOW)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    gld_StaticLight4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
  }
  else
  {
		if(viewplayer->mo->flags & MF_TRANSLUCENT)
      gld_StaticLight4f(light,light,light,(float)tran_filter_pct/100.0f);
    else
  		gld_StaticLight3f(light,light,light);
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

//////////////////////////////
//
// SPRITES
//
//////////////////////////////
/*
typedef struct
{
	int iLump;
	boolean bFlip;
	float fLightLevel;
	boolean rendered;
	mobj_t *p_Obj;
} GLSprite;

#define MAXVISSPRITES  	128
GLSprite SpriteRenderQueue[MAXVISSPRITES];
int NumSpritesInQueue;

void gld_AddSpriteToRenderQueue(mobj_t *pSpr,int lump, boolean flip)
{
	if (NumSpritesInQueue>=MAXVISSPRITES)
		return;
	
	SpriteRenderQueue[NumSpritesInQueue].p_Obj=pSpr;
	SpriteRenderQueue[NumSpritesInQueue].iLump=lump;
	SpriteRenderQueue[NumSpritesInQueue].rendered=false;

	if (pSpr->frame & FF_FULLBRIGHT)
		SpriteRenderQueue[NumSpritesInQueue].fLightLevel = 1.0f;
	else
		SpriteRenderQueue[NumSpritesInQueue].fLightLevel=gld_CalcLightLevel(pSpr->subsector->sector->lightlevel+(extralight<<LIGHTSEGSHIFT));
	
	SpriteRenderQueue[NumSpritesInQueue].bFlip=flip;
	NumSpritesInQueue++;
}

void gld_DrawSprites(player_t *player)
{
  GLTexture *gltexture;
  GLSprite *sprite;
  float fU1,fU2,fV1,fV2;
  float x,y,z;
  int i;
  float fWidth,fLOffset,fHeight,fTOffset;
  int cm;
  int last_lumpnum;

  last_lumpnum=-1;
  gltexture=NULL;
	for (i=NumSpritesInQueue-1;i>=0;i--)
//	for (i=0;i<NumSpritesInQueue;i++)
	{
    sprite=&SpriteRenderQueue[i];
    if (last_lumpnum!=(sprite->iLump+firstspritelump))
    {
      cm=CR_LIMIT+(int)((sprite->p_Obj->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
      gltexture=gld_RegisterPatch(sprite->iLump+firstspritelump, cm);
    }
    if (!gltexture)
      continue;
    gld_BindPatch(gltexture, cm);
    x=-(float)sprite->p_Obj->x/(float)MAP_SCALE;
    y=(float)sprite->p_Obj->z/(float)MAP_SCALE;
    z=(float)sprite->p_Obj->y/(float)MAP_SCALE;

    fV1=0;
    fV2=(float)gltexture->height/(float)gltexture->tex_height;
    if (sprite->bFlip)
    {
      fU1=0;
      fU2=(float)gltexture->width/(float)gltexture->tex_width;
    }
    else
    {
      fU1=(float)gltexture->width/(float)gltexture->tex_width;
      fU2=0;
    }
    fLOffset=(float)gltexture->leftoffset/(float)(MAP_COEFF);
    fWidth=(float)gltexture->width/(float)(MAP_COEFF);
    fTOffset=(float)gltexture->topoffset/(float)(MAP_COEFF);
		fHeight = (float)gltexture->height/(float)(MAP_COEFF);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x,y,z);
		glRotatef(-90.0f+(float)(player->mo->angle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES,0.0f,1.0f,0.0f);
		if(sprite->p_Obj->flags & MF_SHADOW)
    {
      glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
      gld_StaticLight4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
    }
    else
    {
  		if(sprite->p_Obj->flags & MF_TRANSLUCENT)
        gld_StaticLight4f(sprite->fLightLevel,sprite->fLightLevel,sprite->fLightLevel,(float)tran_filter_pct/100.0f);
			else
        gld_StaticLight3f(sprite->fLightLevel,sprite->fLightLevel,sprite->fLightLevel);
    }
	  glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(fU1, fV1); glVertex3f( -fWidth+fLOffset, fTOffset, 0);
			glTexCoord2f(fU2, fV1); glVertex3f( fLOffset, fTOffset, 0);
			glTexCoord2f(fU1, fV2); glVertex3f( -fWidth+fLOffset, -fHeight+fTOffset, 0);
			glTexCoord2f(fU2, fV2); glVertex3f( fLOffset, -fHeight+fTOffset, 0);
  	glEnd();
		
    glPopMatrix();

		if(sprite->p_Obj->flags & MF_SHADOW)
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
}
*/
void gld_InitDrawScene(void)
{
//  NumSpritesInQueue=0;
//  if (gld_DisplayList==0)
//    gld_DisplayList=glGenLists(1);
  if (gld_DisplayList)
    glNewList(gld_DisplayList,GL_COMPILE);
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

//void gld_DrawSoftwareScreen();

void gld_Finish()
{
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

//////////////////////////////
//
// WALLS
//
//////////////////////////////
/*
typedef struct
{
  float x1,x2;
  float ytop,ybottom;
  float z1,z2;
  float ul,ur,vt,vb;
  float ou,ov;
  float light;
  float linelength;
  float lineheight;
  boolean trans;
  GLTexture *gltexture;
} walldef_t;

static void gld_DrawWalldef(walldef_t *walldef)
{
  if (walldef->gltexture)
  {
    gld_BindTexture(walldef->gltexture);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    if (walldef->trans)
      gld_StaticLight4f(walldef->light, walldef->light, walldef->light, 0.5f);
    else
      gld_StaticLight3f(walldef->light, walldef->light, walldef->light);
  }
  else
  {
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f,0.0f,0.0f,0.0f);
  }
#ifdef _DEBUG
  if (walldef->light==-1)
    glColor3f(1.0f, 0.0f, 0.0f);
  if (walldef->light==-2)
    glColor3f(0.0f, 1.0f, 0.0f);
  if (walldef->light==-3)
    glColor3f(0.5f, 0.5f, 1.0f);
#endif
  walldef->ytop+=0.001f;
  walldef->ybottom-=0.001f;
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(walldef->ul+walldef->ou,walldef->vt+walldef->ov); glVertex3f(walldef->x1,walldef->ytop,walldef->z1);
    glTexCoord2f(walldef->ul+walldef->ou,walldef->vb+walldef->ov); glVertex3f(walldef->x1,walldef->ybottom,walldef->z1);
    glTexCoord2f(walldef->ur+walldef->ou,walldef->vt+walldef->ov); glVertex3f(walldef->x2,walldef->ytop,walldef->z2);
    glTexCoord2f(walldef->ur+walldef->ou,walldef->vb+walldef->ov); glVertex3f(walldef->x2,walldef->ybottom,walldef->z2);
  glEnd();
  if (!walldef->gltexture)
    glEnable(GL_TEXTURE_2D);
#ifdef _DEBUG
  if (walldef->light<0)
    glEnable(GL_BLEND);
#endif
}

static void gld_CalcXZCoords(walldef_t *walldef, seg_t *seg)
{
  float dx,dy;

  walldef->x1=-(float)seg->v1->x/(float)MAP_SCALE;
  walldef->z1= (float)seg->v1->y/(float)MAP_SCALE;
  walldef->x2=-(float)seg->v2->x/(float)MAP_SCALE;
  walldef->z2= (float)seg->v2->y/(float)MAP_SCALE;
  dx=((float)seg->v2->x/(float)FRACUNIT)-((float)seg->v1->x/(float)FRACUNIT);
  dy=((float)seg->v2->y/(float)FRACUNIT)-((float)seg->v1->y/(float)FRACUNIT);
  walldef->linelength=(float)sqrt(dx*dx+dy*dy);
}

static void gld_CalcYCoords(walldef_t *walldef, int floor_height, int ceiling_height)
{
  walldef->ytop=(float)ceiling_height/(float)MAP_SCALE;
  walldef->ybottom=(float)floor_height/(float)MAP_SCALE;
  walldef->lineheight=(float)fabs((ceiling_height/(float)FRACUNIT)-(floor_height/(float)FRACUNIT));
}

static void gld_CalcTextureCoords(walldef_t *walldef, seg_t *seg, boolean peg, boolean bottomtex, boolean middletex, int v_offset)
{
  walldef->ou=((float)(seg->sidedef->textureoffset+seg->offset)/(float)FRACUNIT)/(float)walldef->gltexture->tex_width;
  if (middletex)
    walldef->ov=0.0f;
  else
    walldef->ov=((float)(seg->sidedef->rowoffset)/(float)FRACUNIT)/(float)walldef->gltexture->tex_height;
  walldef->ul=0.0f;
  walldef->ur=(float)walldef->linelength/(float)walldef->gltexture->tex_width;
  if (peg)
  {
    walldef->vb=((float)walldef->gltexture->height/(float)walldef->gltexture->tex_height);
    walldef->vt=walldef->vb-((float)walldef->lineheight/(float)walldef->gltexture->tex_height);
    if (bottomtex)
      walldef->ov-=((float)v_offset/(float)FRACUNIT)/(float)walldef->gltexture->tex_height;
  }
  else
  {
    walldef->vt=0.0f;
    walldef->vb=(float)walldef->lineheight/(float)walldef->gltexture->tex_height;
  }
}

void gld_DrawSeg(seg_t *seg, boolean sky_only)
{
  walldef_t walldef;
  line_t *line;
  int floor_height,ceiling_height;
  sector_t *frontsector;
  sector_t *backsector;

  line=seg->linedef;

  frontsector=seg->frontsector;
  if (!frontsector)
    return;
  backsector=seg->backsector;

  gld_CalcXZCoords(&walldef, seg);
  walldef.light=gld_CalcLightLevel(frontsector->lightlevel+(extralight<<LIGHTSEGSHIFT));
  walldef.gltexture=NULL;
  walldef.trans=0;

  if (backsector==NULL) // onesided
  {
    ceiling_height=frontsector->ceilingheight;
    floor_height=frontsector->floorheight;
    if (sky_only)
    {
      if (frontsector->ceilingpic==skyflatnum)
      {
        walldef.ytop=255.0f;
        walldef.ybottom=(float)ceiling_height/(float)MAP_SCALE;
        gld_DrawWalldef(&walldef);
      }
      if (frontsector->floorpic==skyflatnum)
      {
        walldef.ytop=(float)floor_height/(float)MAP_SCALE;
        walldef.ybottom=-255.0f;
        gld_DrawWalldef(&walldef);
      }
    }
    else
    {
      walldef.gltexture=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
      if (walldef.gltexture)
      {
        gld_CalcYCoords(&walldef, floor_height, ceiling_height);
        gld_CalcTextureCoords(&walldef, seg, (line->flags & ML_DONTPEGBOTTOM)>0, false, false, 0);
        gld_DrawWalldef(&walldef);
      }
    }
  }
  else // twosided
  {
    // toptexture
    ceiling_height=frontsector->ceilingheight;
    floor_height=backsector->ceilingheight;
    if (sky_only)
    {
      if ((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic!=skyflatnum))
      {
        walldef.ytop=255.0f;
        walldef.ybottom=(float)ceiling_height/(float)MAP_SCALE;
        gld_DrawWalldef(&walldef);
      }
    }
    else
      if (floor_height<ceiling_height)
      {
        if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
          walldef.gltexture=gld_RegisterTexture(texturetranslation[seg->sidedef->toptexture], true);
        if (walldef.gltexture)
        {
          gld_CalcYCoords(&walldef, floor_height, ceiling_height);
          gld_CalcTextureCoords(&walldef, seg, (line->flags & (ML_DONTPEGBOTTOM | ML_DONTPEGTOP))==0, false, false, 0);
          gld_DrawWalldef(&walldef);
        }
      }
    // midtexture
    if (!sky_only)
    {
      walldef.gltexture=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
      if (walldef.gltexture)
      {
        if ( (line->flags & ML_DONTPEGBOTTOM) >0)
        {
          if (backsector->ceilingheight<=frontsector->floorheight)
            goto bottomtexture;
          floor_height=max(frontsector->floorheight,backsector->floorheight)+(seg->sidedef->rowoffset);
          ceiling_height=floor_height+(walldef.gltexture->height<<FRACBITS);
        }
        else
        {
          if (backsector->ceilingheight<=frontsector->floorheight)
            goto bottomtexture;
          ceiling_height=min(frontsector->ceilingheight,backsector->ceilingheight)+(seg->sidedef->rowoffset);
          floor_height=ceiling_height-(walldef.gltexture->height<<FRACBITS);
        }
        gld_CalcYCoords(&walldef, floor_height, ceiling_height);
        gld_CalcTextureCoords(&walldef, seg, false, false, true, 0);
#ifdef _DEBUG
        if (line->flags & ML_DONTPEGBOTTOM)
          walldef.light=-1;
        if (line->flags & ML_DONTPEGTOP)
          walldef.light=-2;
        if ((line->flags & ML_DONTPEGBOTTOM) && (line->flags & ML_DONTPEGTOP))
          walldef.light=-3;
#endif
        if (seg->linedef->tranlump >= 0 && general_translucency)
          walldef.trans=1;
        gld_DrawWalldef(&walldef);
      }
    }
bottomtexture:
    // bottomtexture
    ceiling_height=backsector->floorheight;
    floor_height=frontsector->floorheight;
    if (sky_only)
    {
      if ((frontsector->floorpic==skyflatnum) && (backsector->floorpic!=skyflatnum))
      {
        walldef.ytop=(float)floor_height/(float)MAP_SCALE;
        walldef.ybottom=-255.0f;
        gld_DrawWalldef(&walldef);
      }
    }
    else
      if (floor_height<ceiling_height)
      {
        walldef.gltexture=gld_RegisterTexture(texturetranslation[seg->sidedef->bottomtexture], true);
        if (walldef.gltexture)
        {
          gld_CalcYCoords(&walldef, floor_height, ceiling_height);
          gld_CalcTextureCoords(&walldef, seg, (line->flags & ML_DONTPEGBOTTOM)>0, true, false, floor_height-frontsector->ceilingheight);
          gld_DrawWalldef(&walldef);
        }
      }
  }
}

void gld_DrawSkyWalls(void)
{
  extern drawseg_t	*drawsegs, *ds_p;
	drawseg_t *ds;

  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
	{
    gld_DrawSeg(ds->curline,true);
	}
}

void gld_DrawWalls(void)
{
  extern drawseg_t	*drawsegs, *ds_p;
	drawseg_t *ds;

  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
	{
    gld_DrawSeg(ds->curline,false);
	}
}
*/
//////////////////////////////
//
// FLATS
//
//////////////////////////////

// GLLoopDef is the struct for one loop. A loop is a list of vertexes
// for triangles, which is calculated by the gluTesselator in gld_PrecalculateSector

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

static void gld_DrawFlat(int num, boolean ceiling)
{
  int loopnum; // current loop number
  GLLoopDef *currentloop; // the current loop
  int vertexnum; // the current vertexnumber
  GLVertex *currentvertex; // the current vertex
  sector_t *sector; // the sector we want to draw
  //sector_t tempsec; // needed for R_FakeFlat
  float light; // the lightlevel of the flat
  GLTexture *gltexture; // the texture
  float uoffs,voffs; // the texture coordinates
  float z; // the z position of the flat (height)

  sector=&sectors[num]; // get the sector
  //sector=R_FakeFlat(sector, &tempsec, NULL, NULL, false); // for boom effects
  if (!ceiling) // if it is a floor ...
  {
    glCullFace(GL_FRONT);
    if (sector->floorpic == skyflatnum) // don't draw if sky
      return;
    else
    {
      // get the texture. flattranslation is maintained by doom and
      // contains the number of the current animation frame
      gltexture=gld_RegisterFlat(firstflat+flattranslation[sector->floorpic], true);
      if (!gltexture)
        return;
      gld_BindFlat(gltexture);
      // get the lightlevel
      if (sector->floorlightsec!=-1)
        light=gld_CalcLightLevel(sectors[sector->floorlightsec].lightlevel+(extralight<<LIGHTSEGSHIFT));
      else
        light=gld_CalcLightLevel(sector->lightlevel+(extralight<<LIGHTSEGSHIFT));
      // set it
      gld_StaticLight3f(light, light, light);
    }
    // calculate texture offsets
    uoffs=(float)sector->floor_xoffs/(float)FRACUNIT;
    voffs=(float)sector->floor_yoffs/(float)FRACUNIT;
    // get height
    z=(float)sector->floorheight/(float)MAP_SCALE;
  }
  else // if it is a ceiling ...
  {
    glCullFace(GL_BACK);
    if (sector->ceilingpic == skyflatnum) // don't draw if sky
      return;
    else
    {
      // get the texture. flattranslation is maintained by doom and
      // contains the number of the current animation frame
      gltexture=gld_RegisterFlat(firstflat+flattranslation[sector->ceilingpic], true);
      if (!gltexture)
        return;
      gld_BindFlat(gltexture);
      // get the lightlevel
      if (sector->ceilinglightsec!=-1)
        light=gld_CalcLightLevel(sectors[sector->ceilinglightsec].lightlevel+(extralight<<LIGHTSEGSHIFT));
      else
        light=gld_CalcLightLevel(sector->lightlevel+(extralight<<LIGHTSEGSHIFT));
      // set it
      gld_StaticLight3f(light, light, light);
    }
    // calculate texture offsets
    uoffs=(float)sector->ceiling_xoffs/(float)FRACUNIT;
    voffs=(float)sector->ceiling_yoffs/(float)FRACUNIT;
    // get height
    z=(float)sector->ceilingheight/(float)MAP_SCALE;
  }
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(uoffs/64.0f,voffs/64.0f,0.0f);
  // go through all loops of this sector
  if (!sectorloops[num].gl_list)
  {
    sectorloops[num].gl_list=glGenLists(1);
    if (sectorloops[num].gl_list)
    {
      glNewList(sectorloops[num].gl_list,GL_COMPILE_AND_EXECUTE);
      for (loopnum=0; loopnum<sectorloops[num].loopcount; loopnum++)
      {
        // set the current loop
        currentloop=&sectorloops[num].loops[loopnum];
        // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
        glBegin(currentloop->mode);
        // go through all vertexes of this loop
        for (vertexnum=0; vertexnum<currentloop->vertexcount; vertexnum++)
        {
          // set the current vertex
          currentvertex=&currentloop->gl_vertexes[vertexnum];
          // set texture coordinate of this vertex
          glTexCoord2f(currentvertex->u,currentvertex->v);
          // set vertex coordinate
          glVertex3f(currentvertex->x,0.0f,currentvertex->y);
        }
        // end of loop
        glEnd();
      }
      glEndList();
    }
  }
  else
    glCallList(sectorloops[num].gl_list);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

static boolean *sectorclosed=NULL; // list of all sectors with true if sector closed
static boolean *sectorrendered=NULL; // true if sector rendered (only here for malloc)

// gld_DrawFloorsCeilings
//
// draw all visible floors and ceilings
/*
void gld_DrawFloorsCeilings(void)
{
  extern drawseg_t	*drawsegs, *ds_p;
	drawseg_t *ds;

  // check if all arrays are allocated
  if (!sectorclosed)
    return;
  if (!sectorrendered)
    return;

  // enable backside removing
  glEnable(GL_CULL_FACE);
  for (ds=ds_p ; ds-- > drawsegs ; ) // go through all segs
    if (!sectorrendered[ds->curline->frontsector->iSectorID]) // if not already rendered
      if (sectorclosed[ds->curline->frontsector->iSectorID]) // check if sector is closed
      {
        // render the floor
        glCullFace(GL_FRONT);
        gld_DrawFlat(ds->curline->frontsector->iSectorID,false);
        // render the ceiling
        glCullFace(GL_BACK);
        gld_DrawFlat(ds->curline->frontsector->iSectorID,true);
        // set rendered true
        sectorrendered[ds->curline->frontsector->iSectorID]=true;
      }
      else
        sectorrendered[ds->curline->frontsector->iSectorID]=true;
  // disable backside removing
  glDisable(GL_CULL_FACE);
}
*/
static FILE *levelinfo;
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
  sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount,PU_LEVEL,0);
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
  gl_vertexes=Z_Realloc(gl_vertexes, vertexcount*sizeof(GLVertex),PU_LEVEL,0);
  // add the new vertex (vert is the second argument of gluTessVertex)
  gl_vertexes[vertexcount-1].u=(-(float)vert->x/(float)FRACUNIT)/64.0f;
  gl_vertexes[vertexcount-1].v=( (float)vert->y/(float)FRACUNIT)/64.0f;
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
  int i,j;
  char *vertexcheck;
  int v1num;
  int v2num;

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

  sectorclosed=GLRealloc(sectorclosed,numsectors*sizeof(boolean));
  sectorrendered=GLRealloc(sectorrendered,numsectors*sizeof(boolean));
  sectorloops=GLRealloc(sectorloops, sizeof(GLSector)*numsectors);
  memset(sectorloops, 0, sizeof(GLSector)*numsectors);
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
      lprintf(LO_ERROR, "Sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      if (levelinfo) fprintf(levelinfo, "Sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
      sectorclosed[i]=false;
    }
    else
      sectorclosed[i]=true;
    for (j=0; j<numvertexes; j++)
    {
      if ((vertexcheck[j]==1) || (vertexcheck[j]==2))
      {
        lprintf(LO_ERROR, "Sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
        if (levelinfo) fprintf(levelinfo, "Sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
        sectorclosed[i]=false;
      }
    }
    if (sectorclosed[i])
      gld_PrecalculateSector(i);
  }
  if (levelinfo) fclose(levelinfo);
  GLFree(vertexcheck);
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
	xCamera=-(float)viewplayer->mo->x/(float)MAP_SCALE;
	yCamera=(float)viewplayer->mo->y/(float)MAP_SCALE;
	trY=(float)viewplayer->viewz/(float)MAP_SCALE;
	
	yaw=270.0f-(float)(viewplayer->mo->angle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	inv_yaw=-90.0f+(float)(viewplayer->mo->angle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;

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
	  glColor4f(extra_red, extra_green, extra_blue, extra_alpha);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_STRIP);
  		glVertex2f( 0.0f, 0.0f);
	  	glVertex2f( 0.0f, (float)SCREENHEIGHT);
		  glVertex2f( (float)SCREENWIDTH, 0.0f);
		  glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
    glEnable(GL_TEXTURE_2D);
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

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // proff_dis
  glShadeModel(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);
 	glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_NOTEQUAL,0.0f);
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
      gld_StaticLight4f(wall->light, wall->light, wall->light, (float)tran_filter_pct/100.0f);
    else
      gld_StaticLight3f(wall->light, wall->light, wall->light);
  }
  else
  {
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f,0.0f,0.0f,1.0f);
  }
  if (wall->sky)
  {
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_Q);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    if (wall->skyflip)
      glScalef(-1.0f,3.5f,1.0f);
    else
      glScalef(1.0f,3.5f,1.0f);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBegin(GL_TRIANGLE_STRIP);
      glVertex3f(glseg->x1,wall->ytop,glseg->z1);
      glVertex3f(glseg->x1,wall->ybottom,glseg->z1);
      glVertex3f(glseg->x2,wall->ytop,glseg->z2);
      glVertex3f(glseg->x2,wall->ybottom,glseg->z2);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_S);
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
#define FRONTSECTOR seg->frontsector
#define BACKSECTOR seg->backsector
#define CALC_Y_VALUES(walldef, floor_height, ceiling_height)\
  (walldef).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+0.001f;\
  (walldef).ybottom=((float)(floor_height)/(float)MAP_SCALE)-0.001f;\
  (walldef).lineheight=(float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT))

#define CALC_TEX_VALUES_TOP(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->tex_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->tex_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->tex_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->tex_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->tex_height\
  )

#define CALC_TEX_VALUES_MIDDLE(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->tex_width;\
  (w).ov=0.0f;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->tex_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->tex_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->tex_height\
  )

#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, v_offset)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->tex_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->tex_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->tex_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->tex_height),\
    (w).ov-=((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->tex_height\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->tex_height\
  )

#define SKYTEXTURE(sky1,sky2)\
      if ((sky1) & PL_SKYFLAT)\
      {\
	      const line_t *l = &lines[sky1 & ~PL_SKYFLAT];\
	      const side_t *s = *l->sidenum + sides;\
        wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	      /* an += s->textureoffset;*/\
	      /* dc_texturemid = s->rowoffset - 28*FRACUNIT;*/\
	      wall.skyflip = l->special==272 ? false : true;\
      }\
      else\
      if ((sky2) & PL_SKYFLAT)\
      {\
	      const line_t *l = &lines[sky2 & ~PL_SKYFLAT];\
	      const side_t *s = *l->sidenum + sides;\
        wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	      /* an += s->textureoffset;*/\
	      /* dc_texturemid = s->rowoffset - 28*FRACUNIT;*/\
	      wall.skyflip = l->special==272 ? false : true;\
      }\
      else\
      {\
        wall.gltexture=gld_RegisterTexture(skytexture, false);\
	      wall.skyflip = false;\
      }

void gld_AddWall(seg_t *seg)
{
  GLWall wall;
  GLTexture *temptex;

  if (!FRONTSECTOR)
    return;
  wall.segnum=seg->iSegID;
  wall.light=gld_CalcLightLevel(FRONTSECTOR->lightlevel+(extralight<<LIGHTSEGSHIFT));
  wall.gltexture=NULL;
  wall.trans=0;

  if (BACKSECTOR==NULL) /* onesided */
  {
    if (FRONTSECTOR->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      wall.ybottom=(float)FRONTSECTOR->ceilingheight/(float)MAP_SCALE;
      SKYTEXTURE(FRONTSECTOR->sky,FRONTSECTOR->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    if (FRONTSECTOR->floorpic==skyflatnum)
    {
      wall.ytop=(float)FRONTSECTOR->floorheight/(float)MAP_SCALE;
      wall.ybottom=-255.0f;
      SKYTEXTURE(FRONTSECTOR->sky,FRONTSECTOR->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
    if (temptex)
    {
      wall.gltexture=temptex;
      CALC_Y_VALUES(wall, FRONTSECTOR->floorheight, FRONTSECTOR->ceilingheight);
      CALC_TEX_VALUES_MIDDLE(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0);
      wall.sky=false;
      gld_DrawWall(&wall);
    }
  }
  else /* twosided */
  {
    int floor_height,ceiling_height;

    /* toptexture */
    ceiling_height=FRONTSECTOR->ceilingheight;
    floor_height=BACKSECTOR->ceilingheight;
//    if ((FRONTSECTOR->ceilingpic==skyflatnum) && (BACKSECTOR->ceilingpic!=skyflatnum))
//    if ((FRONTSECTOR->ceilingpic==skyflatnum) && (BACKSECTOR->ceilingpic==skyflatnum))
    if (FRONTSECTOR->ceilingpic==skyflatnum)
//    if ((FRONTSECTOR->ceilingpic==skyflatnum) || (BACKSECTOR->ceilingpic==skyflatnum))
    {
      wall.ytop=255.0f;
      if (floor_height==BACKSECTOR->floorheight)
        wall.ybottom=(float)BACKSECTOR->floorheight/(float)MAP_SCALE;
      else
        wall.ybottom=(float)ceiling_height/(float)MAP_SCALE;
      SKYTEXTURE(FRONTSECTOR->sky,BACKSECTOR->sky);
      wall.sky=true;
      gld_DrawWall(&wall);
    }
    if (floor_height<ceiling_height)
    {
      if (!((FRONTSECTOR->ceilingpic==skyflatnum) && (BACKSECTOR->ceilingpic==skyflatnum)))
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
        if (BACKSECTOR->ceilingheight<=FRONTSECTOR->floorheight)
          goto bottomtexture;
        floor_height=max(FRONTSECTOR->floorheight,BACKSECTOR->floorheight)+(seg->sidedef->rowoffset);
        ceiling_height=floor_height+(wall.gltexture->height<<FRACBITS);
      }
      else
      {
        if (BACKSECTOR->ceilingheight<=FRONTSECTOR->floorheight)
          goto bottomtexture;
        ceiling_height=min(FRONTSECTOR->ceilingheight,BACKSECTOR->ceilingheight)+(seg->sidedef->rowoffset);
        floor_height=ceiling_height-(wall.gltexture->height<<FRACBITS);
      }
      CALC_Y_VALUES(wall, floor_height, ceiling_height);
      CALC_TEX_VALUES_MIDDLE(wall, seg, false);
      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.trans=1;
      wall.sky=false;
      gld_DrawWall(&wall);
    }
bottomtexture:
    /* bottomtexture */
    ceiling_height=BACKSECTOR->floorheight;
    floor_height=FRONTSECTOR->floorheight;
//    if ((FRONTSECTOR->floorpic==skyflatnum) && (BACKSECTOR->floorpic!=skyflatnum))
//    if ((FRONTSECTOR->floorpic==skyflatnum) && (BACKSECTOR->floorpic==skyflatnum))
    if (FRONTSECTOR->floorpic==skyflatnum)
    {
      wall.ytop=(float)floor_height/(float)MAP_SCALE;
      wall.ybottom=-255.0f;
      SKYTEXTURE(FRONTSECTOR->sky,BACKSECTOR->sky);
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
        CALC_TEX_VALUES_BOTTOM(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0, floor_height-FRONTSECTOR->ceilingheight);
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

  // enable backside removing
  glEnable(GL_CULL_FACE);
    if (!sectorrendered[sector->iSectorID]) // if not already rendered
      if (sectorclosed[sector->iSectorID]) // check if sector is closed
      {
        // render the floor
        gld_DrawFlat(sector->iSectorID, false);
        // render the ceiling
        gld_DrawFlat(sector->iSectorID, true);
        // set rendered true
        sectorrendered[sector->iSectorID]=true;
      }
      else
        sectorrendered[sector->iSectorID]=true;
  // disable backside removing
  glDisable(GL_CULL_FACE);
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
  w=(float)gltexture->width/(float)(MAP_COEFF);
  h=(float)gltexture->height/(float)(MAP_COEFF);
  gld_BindPatch(gltexture,cm);
  glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(x,y,z);
	glRotatef(inv_yaw,0.0f,1.0f,0.0f);
	if(shadow)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    gld_StaticLight4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
  }
  else
  {
		if(trans)
      gld_StaticLight4f(light,light,light,(float)tran_filter_pct/100.0f);
		else
      gld_StaticLight3f(light,light,light);
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

/*
  // SKY-TEST
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_Q);
*/
  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
	{
    gld_AddWall(ds->curline);
	}
/*
  glDisable(GL_TEXTURE_GEN_Q);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_GEN_S);
*/
}

void gld_PreprocessLevel(void)
{
  gld_PreprocessSectors();
  gld_PreprocessSegs();
}
