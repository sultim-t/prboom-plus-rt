// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: gl_intern.h,v 1.2 2000/05/07 20:19:33 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//
//---------------------------------------------------------------------

#ifndef _GL_INTERN_H
#define _GL_INTERN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "SDL.h"
#include "w_wad.h"
#include "m_argv.h"
#include "z_zone.h"
#include "d_event.h"
#include "v_video.h"
#include "doomstat.h"

#define MAP_COEFF 128
#define MAP_SCALE	(MAP_COEFF<<FRACBITS) // 6553600 -- nicolas

//#define CR_SPRITETRAN 128
//#define CR_SPRITESTART CR_DEFAULT

#define GLMalloc(n) Z_Malloc(n,PU_STATIC,0)
#define GLRealloc(p,n) Z_Realloc(p,n,PU_STATIC,0)
#define GLFree(p) Z_Free(p)

typedef struct
{
	int iLump;
	boolean bFlip;
	float fLightLevel;
	boolean rendered;
	mobj_t *p_Obj;
} GLSprite;

typedef struct
{	
	int width,height;
  int leftoffset,topoffset;
  int tex_width,tex_height;
  int glTexID[CR_LIMIT+MAXPLAYERS];
} GLTexture;

void gld_StaticLight3f(GLfloat fRed, GLfloat fGreen, GLfloat fBlue);
void gld_StaticLight4f(GLfloat fRed, GLfloat fGreen, GLfloat fBlue, GLfloat fAlpha);

GLTexture *gld_GetGLTexture( short sTexture,short xOffset,short yOffset,float *fU1,float *fU2,
					   float *fV1,float *fV2,float *fU1Off,float *fV2Off);
GLTexture *gld_RegisterTexture(int texture_num);

void gld_OutputLevelInfo(void);
#endif // _GL_INTERN_H

//-----------------------------------------------------------------------------
//
// $Log: gl_intern.h,v $
// Revision 1.2  2000/05/07 20:19:33  proff_fs
// changed use of colormaps from pointers to numbers.
// That's needed for OpenGL.
// The OpenGL part is slightly better now.
// Added some typedefs to reduce warnings in VisualC.
// Messages are also scaled now, because at 800x600 and
// above you can't read them even on a 21" monitor.
//
// Revision 1.1  2000/05/04 16:40:00  proff_fs
// added OpenGL stuff. Not complete yet.
// Only the playerview is rendered.
// The normal output is displayed in a small window.
// The level is only drawn in debugmode to the window.
//
// Revision 1.4  2000/04/30 15:28:24  proff_fs
// fixed the OpenGL middle-texture alignment bug
//
// Revision 1.3  2000/04/26 20:00:03  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.2  2000/04/10 21:13:13  proff_fs
// added Log to OpenGL files
//
// Revision 1.1.1.1  2000/04/09 18:21:44  proff_fs
// Initial login
//
//-----------------------------------------------------------------------------
