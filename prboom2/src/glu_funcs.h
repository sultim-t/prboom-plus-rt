/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: glu_funcs.h,v 1.1 2001/02/05 11:28:31 proff_fs Exp $
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

#ifndef PROTOTYPE
#define PROTOTYPE(ret, func, param)
#endif

PROTOTYPE(int, gluBuild2DMipmaps, (GLenum, GLint, GLint, GLint, GLenum, GLenum, const void *))
PROTOTYPE(void, gluDeleteTess, (GLUtesselator *))
PROTOTYPE(const GLubyte *, gluErrorString, (GLenum))
PROTOTYPE(GLUtesselator *, gluNewTess, (void))
PROTOTYPE(void, gluPerspective, (GLdouble, GLdouble, GLdouble, GLdouble))
PROTOTYPE(int, gluScaleImage, (GLenum, GLint, GLint, GLenum, const void *, GLint, GLint, GLenum, void *))
PROTOTYPE(void, gluTessBeginContour, (GLUtesselator *))
PROTOTYPE(void, gluTessBeginPolygon, (GLUtesselator *, void *))
PROTOTYPE(void, gluTessCallback, (GLUtesselator *, GLenum, void *))
PROTOTYPE(void, gluTessEndContour, (GLUtesselator *))
PROTOTYPE(void, gluTessEndPolygon, (GLUtesselator *))
PROTOTYPE(void, gluTessVertex, (GLUtesselator *, GLdouble *, void *))

#undef PROTOTYPE
