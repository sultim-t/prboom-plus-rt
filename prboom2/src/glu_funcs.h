/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: glu_funcs.h,v 1.3 2002/11/16 23:02:19 proff_fs Exp $
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

#define PROTOTYPE(ret, func, param) extern ret p_##func param;

PROTOTYPE(GLint, gluBuild2DMipmaps, (GLenum, GLint, GLint, GLint, GLenum, GLenum, const void *))
PROTOTYPE(void, gluDeleteTess, (GLUtriangulatorObj *))
PROTOTYPE(const GLubyte *, gluErrorString, (GLenum))
PROTOTYPE(GLUtriangulatorObj *, gluNewTess, (void))
PROTOTYPE(GLint, gluScaleImage, (GLenum, GLint, GLint, GLenum, const void *, GLint, GLint, GLenum, void *))
PROTOTYPE(void, gluNextContour, (GLUtriangulatorObj *, GLenum))
PROTOTYPE(void, gluBeginPolygon, (GLUtriangulatorObj *))
PROTOTYPE(void, gluTessCallback, (GLUtriangulatorObj *, GLenum, void *))
PROTOTYPE(void, gluEndPolygon, (GLUtriangulatorObj *))
PROTOTYPE(void, gluTessVertex, (GLUtriangulatorObj *, GLdouble *, void *))

#undef PROTOTYPE
