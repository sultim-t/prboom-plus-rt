/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_funcs.h,v 1.2 2002/08/11 14:21:52 proff_fs Exp $
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

PROTOTYPE(void, glAlphaFunc, (GLenum, GLclampf))
PROTOTYPE(void, glBegin, (GLenum))
PROTOTYPE(void, glBindTexture, (GLenum, GLuint))
PROTOTYPE(void, glBlendFunc, (GLenum, GLenum))
PROTOTYPE(void, glClear, (GLbitfield))
PROTOTYPE(void, glClearColor, (GLclampf, GLclampf, GLclampf, GLclampf))
PROTOTYPE(void, glClearDepth, (GLclampd))
PROTOTYPE(void, glColor3f, (GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glColor4f, (GLfloat, GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glColor4fv, (const GLfloat *))
PROTOTYPE(void, glCullFace, (GLenum))
PROTOTYPE(void, glDeleteTextures, (GLsizei, const GLuint *))
PROTOTYPE(void, glDepthFunc, (GLenum))
PROTOTYPE(void, glDisable, (GLenum))
PROTOTYPE(void, glDisableClientState, (GLenum))
PROTOTYPE(void, glDrawArrays, (GLenum, GLint, GLsizei))
PROTOTYPE(void, glEnable, (GLenum))
PROTOTYPE(void, glEnableClientState, (GLenum))
PROTOTYPE(void, glEnd, (void))
PROTOTYPE(void, glFinish, (void))
PROTOTYPE(void, glFogf, (GLenum, GLfloat))
PROTOTYPE(void, glFogfv, (GLenum, const GLfloat *))
PROTOTYPE(void, glFogi, (GLenum, GLint))
PROTOTYPE(void, glGenTextures, (GLsizei, GLuint *))
PROTOTYPE(void, glGetIntegerv, (GLenum, GLint *))
PROTOTYPE(const GLubyte *, glGetString, (GLenum))
PROTOTYPE(void, glGetTexParameteriv, (GLenum, GLenum, GLint *))
PROTOTYPE(void, glHint, (GLenum, GLenum))
PROTOTYPE(void, glLoadIdentity, (void))
PROTOTYPE(void, glMatrixMode, (GLenum))
PROTOTYPE(void, glOrtho, (GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
PROTOTYPE(void, glPixelStorei, (GLenum, GLint))
PROTOTYPE(void, glPopMatrix, (void))
PROTOTYPE(void, glPushMatrix, (void))
PROTOTYPE(void, glReadPixels, (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *))
PROTOTYPE(void, glRotatef, (GLfloat, GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glScalef, (GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glScissor, (GLint, GLint, GLsizei, GLsizei))
PROTOTYPE(void, glShadeModel, (GLenum))
PROTOTYPE(void, glTexCoord2f, (GLfloat, GLfloat))
PROTOTYPE(void, glTexCoord2fv, (const GLfloat *))
PROTOTYPE(void, glTexCoordPointer, (GLint, GLenum, GLsizei, const GLvoid *))
PROTOTYPE(void, glTexEnvf, (GLenum, GLenum, GLfloat))
PROTOTYPE(void, glTexGenf, (GLenum, GLenum, GLfloat))
PROTOTYPE(void, glTexGenfv, (GLenum, GLenum, const GLfloat *))
PROTOTYPE(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
PROTOTYPE(void, glTexParameteri, (GLenum, GLenum, GLint))
PROTOTYPE(void, glTexParameterf, (GLenum, GLenum, GLfloat))
PROTOTYPE(void, glTranslatef, (GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glVertex2f, (GLfloat, GLfloat))
PROTOTYPE(void, glVertex2i, (GLint, GLint))
PROTOTYPE(void, glVertex3f, (GLfloat, GLfloat, GLfloat))
PROTOTYPE(void, glVertex3fv, (const GLfloat *))
PROTOTYPE(void, glVertexPointer, (GLint, GLenum, GLsizei, const GLvoid *))
PROTOTYPE(void, glViewport, (GLint, GLint, GLsizei, GLsizei))

#undef PROTOTYPE
