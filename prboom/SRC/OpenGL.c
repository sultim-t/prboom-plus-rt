/* 
 * OpenGL.c
 *
 * Part of the Plasma Game Server
 * written/copyright Ryan Haksi, 1998
 * 
 * If you use this file as part of a publically released project, please 
 * grant me the courtesy of a mention in the credits section somewhere.
 * Other than that opengl.h and opengl.c are yours to do with what you wish
 *
 * Current email address(es): cryogen@unix.infoserve.net
 * Current web page address: http://home.bc.rogers.wave.ca/borealis/ryan.html
 * 
 * If you add functions to this file, please email me the updated copy so that
 * I can maintain the most up to date copy.

 * Oh, and if anyone can figure out a way to shadow the wgl calls
 * transparently rather than the way I'm doing it now, lemme know. Its not
 * a big deal but things like this irk me.

 */


/*
 * A special thanks goes out to Dan Mintz ( brink@silcon.com ) for providing the full opengl function list
 * as well as the the ARB multitexture and EXT compiled array stuff.
 */


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "opengl.h"

#define MINIGL 0

HINSTANCE openglInst = NULL;
BOOL      openglBypassGDI = FALSE;

/*
#if REFERENCE_MINIGL_FUNCTION_LIST
  void            (APIENTRY *fnglAlphaFunc)(GLenum func, GLclampf ref)=NULL;
  void            (APIENTRY *fnglBegin)(GLenum mode)=NULL;
  void            (APIENTRY *fnglBindTexture)(GLenum target, GLuint texture)=NULL;
  void            (APIENTRY *fnglBlendFunc)(GLenum sfactor, GLenum dfactor)=NULL;
  void            (APIENTRY *fnglClear)(GLbitfield mask)=NULL;
  void            (APIENTRY *fnglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)=NULL;
  void            (APIENTRY *fnglClearDepth)(GLclampd depth)=NULL;
  void            (APIENTRY *fnglColor3f)(GLfloat red, GLfloat green, GLfloat blue)=NULL;
  void            (APIENTRY *fnglColor3fv)(const GLfloat *v)=NULL;
  void            (APIENTRY *fnglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)=NULL;
  void            (APIENTRY *fnglColor4fv)(const GLfloat *v)=NULL;
  void            (APIENTRY *fnglCullFace)(GLenum mode)=NULL;
  void            (APIENTRY *fnglDepthFunc)(GLenum func)=NULL;
  void            (APIENTRY *fnglDisable)(GLenum cap)=NULL;
  void            (APIENTRY *fnglEnable)(GLenum cap)=NULL;
  void            (APIENTRY *fnglEnd)(void)=NULL;
  void            (APIENTRY *fnglFinish)(void)=NULL;
  void            (APIENTRY *fnglFrontFace)(GLenum mode)=NULL;
  void            (APIENTRY *fnglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)=NULL;
  GLenum          (APIENTRY *fnglGetError)(void)=NULL;
  void            (APIENTRY *fnglGetFloatv)(GLenum pname, GLfloat *params)=NULL;
  void            (APIENTRY *fnglGetIntegerv)(GLenum pname, GLint *params)=NULL;
  const GLubyte * (APIENTRY *fnglGetString)(GLenum name)=NULL;
  void            (APIENTRY *fnglHint)(GLenum target, GLenum mode)=NULL;
  GLboolean       (APIENTRY *fnglIsEnabled)(GLenum cap)=NULL;
  void            (APIENTRY *fnglLoadIdentity)(void)=NULL;
  void            (APIENTRY *fnglLoadMatrixf)(const GLfloat *m)=NULL;
  void            (APIENTRY *fnglMatrixMode)(GLenum mode)=NULL;
  void            (APIENTRY *fnglMultMatrixf)(const GLfloat *m)=NULL;
  void            (APIENTRY *fnglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)=NULL;
  void            (APIENTRY *fnglPixelStorei)(GLenum pname, GLint param)=NULL;
  void            (APIENTRY *fnglPolygonOffset)(GLfloat factor, GLfloat units)=NULL;
  void            (APIENTRY *fnglPopMatrix)(void)=NULL;
  void            (APIENTRY *fnglPushMatrix)(void)=NULL;
  void            (APIENTRY *fnglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)=NULL;
  void            (APIENTRY *fnglScalef)(GLfloat x, GLfloat y, GLfloat z)=NULL;
  void            (APIENTRY *fnglShadeModel)(GLenum mode)=NULL;
  void            (APIENTRY *fnglTexCoord2f)(GLfloat s, GLfloat t)=NULL;
  void            (APIENTRY *fnglTexCoord2fv)(const GLfloat *v)=NULL;
  void            (APIENTRY *fnglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r)=NULL;
  void            (APIENTRY *fnglTexCoord3fv)(const GLfloat *v)=NULL;
  void            (APIENTRY *fnglTexEnvf)(GLenum target, GLenum pname, GLfloat param)=NULL;
  void            (APIENTRY *fnglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
  void            (APIENTRY *fnglTexParameterf)(GLenum target, GLenum pname, GLfloat param)=NULL;
  void            (APIENTRY *fnglTranslatef)(GLfloat x, GLfloat y, GLfloat z)=NULL;
  void            (APIENTRY *fnglVertex2f)(GLfloat x, GLfloat y)=NULL;
  void            (APIENTRY *fnglVertex3f)(GLfloat x, GLfloat y, GLfloat z)=NULL;
  void            (APIENTRY *fnglVertex3fv)(const GLfloat *v)=NULL;
  void            (APIENTRY *fnglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)=NULL;
  void            (APIENTRY *fnglViewport)(GLint x, GLint y, GLsizei width, GLsizei height)=NULL;
#endif
*/

void			      (APIENTRY *fnglAccum)(GLenum op, GLfloat value)=NULL;
void			      (APIENTRY *fnglAlphaFunc)(GLenum func, GLclampf ref)=NULL;
GLboolean		    (APIENTRY *fnglAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences)=NULL;
void			      (APIENTRY *fnglArrayElement)(GLint i)=NULL;
void			      (APIENTRY *fnglBegin)(GLenum mode)=NULL;
void			      (APIENTRY *fnglBindTexture)(GLenum target, GLuint texture)=NULL;
void			      (APIENTRY *fnglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)=NULL;
void			      (APIENTRY *fnglBlendFunc)(GLenum sfactor, GLenum dfactor)=NULL;
void			      (APIENTRY *fnglCallList)(GLuint list)=NULL;
void			      (APIENTRY *fnglCallLists)(GLsizei n, GLenum type, const GLvoid *lists)=NULL;
void			      (APIENTRY *fnglClear)(GLbitfield mask)=NULL;
void			      (APIENTRY *fnglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)=NULL;
void			      (APIENTRY *fnglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)=NULL;
void			      (APIENTRY *fnglClearDepth)(GLclampd depth)=NULL;
void			      (APIENTRY *fnglClearIndex)(GLfloat c)=NULL;
void			      (APIENTRY *fnglClearStencil)(GLint s)=NULL;
void			      (APIENTRY *fnglClipPlane)(GLenum plane, const GLdouble *equation)=NULL;
void			      (APIENTRY *fnglColor3b)(GLbyte red, GLbyte green, GLbyte blue)=NULL;
void			      (APIENTRY *fnglColor3bv)(const GLbyte *v)=NULL;
void			      (APIENTRY *fnglColor3d)(GLdouble red, GLdouble green, GLdouble blue)=NULL;
void			      (APIENTRY *fnglColor3dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglColor3f)(GLfloat red, GLfloat green, GLfloat blue)=NULL;
void			      (APIENTRY *fnglColor3fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglColor3i)(GLint red, GLint green, GLint blue)=NULL;
void			      (APIENTRY *fnglColor3iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglColor3s)(GLshort red, GLshort green, GLshort blue)=NULL;
void			      (APIENTRY *fnglColor3sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglColor3ub)(GLubyte red, GLubyte green, GLubyte blue)=NULL;
void			      (APIENTRY *fnglColor3ubv)(const GLubyte *v)=NULL;
void			      (APIENTRY *fnglColor3ui)(GLuint red, GLuint green, GLuint blue)=NULL;
void			      (APIENTRY *fnglColor3uiv)(const GLuint *v)=NULL;
void			      (APIENTRY *fnglColor3us)(GLushort red, GLushort green, GLushort blue)=NULL;
void			      (APIENTRY *fnglColor3usv)(const GLushort *v)=NULL;
void			      (APIENTRY *fnglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)=NULL;
void			      (APIENTRY *fnglColor4bv)(const GLbyte *v)=NULL;
void			      (APIENTRY *fnglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)=NULL;
void			      (APIENTRY *fnglColor4dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)=NULL;
void			      (APIENTRY *fnglColor4fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglColor4i)(GLint red, GLint green, GLint blue, GLint alpha)=NULL;
void			      (APIENTRY *fnglColor4iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha)=NULL;
void			      (APIENTRY *fnglColor4sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)=NULL;
void			      (APIENTRY *fnglColor4ubv)(const GLubyte *v)=NULL;
void			      (APIENTRY *fnglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha)=NULL;
void			      (APIENTRY *fnglColor4uiv)(const GLuint *v)=NULL;
void			      (APIENTRY *fnglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha)=NULL;
void			      (APIENTRY *fnglColor4usv)(const GLushort *v)=NULL;
void			      (APIENTRY *fnglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)=NULL;
void			      (APIENTRY *fnglColorMaterial)(GLenum face, GLenum mode)=NULL;
void			      (APIENTRY *fnglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)=NULL;
void			      (APIENTRY *fnglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)=NULL;
void			      (APIENTRY *fnglCopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)=NULL;
void			      (APIENTRY *fnglCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)=NULL;
void			      (APIENTRY *fnglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)=NULL;
void			      (APIENTRY *fnglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)=NULL;
void			      (APIENTRY *fnglCullFace)(GLenum mode)=NULL;
void			      (APIENTRY *fnglDeleteLists)(GLuint list, GLsizei range)=NULL;
void			      (APIENTRY *fnglDeleteTextures)(GLsizei n, const GLuint *textures)=NULL;
void			      (APIENTRY *fnglDepthFunc)(GLenum func)=NULL;
void			      (APIENTRY *fnglDepthMask)(GLboolean flag)=NULL;
void			      (APIENTRY *fnglDepthRange)(GLclampd zNear, GLclampd zFar)=NULL;
void			      (APIENTRY *fnglDisable)(GLenum cap)=NULL;
void			      (APIENTRY *fnglDisableClientState)(GLenum array)=NULL;
void			      (APIENTRY *fnglDrawArrays)(GLenum mode, GLint first, GLsizei count)=NULL;
void			      (APIENTRY *fnglDrawBuffer)(GLenum mode)=NULL;
void			      (APIENTRY *fnglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)=NULL;
void			      (APIENTRY *fnglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglEdgeFlag)(GLboolean flag)=NULL;
void			      (APIENTRY *fnglEdgeFlagPointer)(GLsizei stride, const GLboolean *pointer)=NULL;
void			      (APIENTRY *fnglEdgeFlagv)(const GLboolean *flag)=NULL;
void			      (APIENTRY *fnglEnable)(GLenum cap)=NULL;
void			      (APIENTRY *fnglEnableClientState)(GLenum array)=NULL;
void			      (APIENTRY *fnglEnd)(void)=NULL;
void			      (APIENTRY *fnglEndList)(void)=NULL;
void			      (APIENTRY *fnglEvalCoord1d)(GLdouble u)=NULL;
void			      (APIENTRY *fnglEvalCoord1dv)(const GLdouble *u)=NULL;
void			      (APIENTRY *fnglEvalCoord1f)(GLfloat u)=NULL;
void			      (APIENTRY *fnglEvalCoord1fv)(const GLfloat *u)=NULL;
void			      (APIENTRY *fnglEvalCoord2d)(GLdouble u, GLdouble v)=NULL;
void			      (APIENTRY *fnglEvalCoord2dv)(const GLdouble *u)=NULL;
void			      (APIENTRY *fnglEvalCoord2f)(GLfloat u, GLfloat v)=NULL;
void			      (APIENTRY *fnglEvalCoord2fv)(const GLfloat *u)=NULL;
void			      (APIENTRY *fnglEvalMesh1)(GLenum mode, GLint i1, GLint i2)=NULL;
void			      (APIENTRY *fnglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)=NULL;
void			      (APIENTRY *fnglEvalPoint1)(GLint i)=NULL;
void			      (APIENTRY *fnglEvalPoint2)(GLint i, GLint j)=NULL;
void			      (APIENTRY *fnglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer)=NULL;
void			      (APIENTRY *fnglFinish)(void)=NULL;
void			      (APIENTRY *fnglFlush)(void)=NULL;
void			      (APIENTRY *fnglFogf)(GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglFogfv)(GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglFogi)(GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglFogiv)(GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglFrontFace)(GLenum mode)=NULL;
void			      (APIENTRY *fnglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)=NULL;
GLuint			    (APIENTRY *fnglGenLists)(GLsizei range)=NULL;
void			      (APIENTRY *fnglGenTextures)(GLsizei n, GLuint *textures)=NULL;
void			      (APIENTRY *fnglGetBooleanv)(GLenum pname, GLboolean *params)=NULL;
void			      (APIENTRY *fnglGetClipPlane)(GLenum plane, GLdouble *equation)=NULL;
void			      (APIENTRY *fnglGetDoublev)(GLenum pname, GLdouble *params)=NULL;
GLenum			    (APIENTRY *fnglGetError)(void)=NULL;
void			      (APIENTRY *fnglGetFloatv)(GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetIntegerv)(GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetLightfv)(GLenum light, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetLightiv)(GLenum light, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetMapdv)(GLenum target, GLenum query, GLdouble *v)=NULL;
void			      (APIENTRY *fnglGetMapfv)(GLenum target, GLenum query, GLfloat *v)=NULL;
void			      (APIENTRY *fnglGetMapiv)(GLenum target, GLenum query, GLint *v)=NULL;
void			      (APIENTRY *fnglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetMaterialiv)(GLenum face, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetPixelMapfv)(GLenum map, GLfloat *values)=NULL;
void			      (APIENTRY *fnglGetPixelMapuiv)(GLenum map, GLuint *values)=NULL;
void			      (APIENTRY *fnglGetPixelMapusv)(GLenum map, GLushort *values)=NULL;
void			      (APIENTRY *fnglGetPointerv)(GLenum pname, GLvoid* *params)=NULL;
void			      (APIENTRY *fnglGetPolygonStipple)(GLubyte *mask)=NULL;
const GLubyte*	(APIENTRY *fnglGetString)(GLenum name)=NULL;
void			      (APIENTRY *fnglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetTexEnviv)(GLenum target, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params)=NULL;
void			      (APIENTRY *fnglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params)=NULL;
void			      (APIENTRY *fnglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params)=NULL;
void			      (APIENTRY *fnglHint)(GLenum target, GLenum mode)=NULL;
void			      (APIENTRY *fnglIndexMask)(GLuint mask)=NULL;
void			      (APIENTRY *fnglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer)=NULL;
void			      (APIENTRY *fnglIndexd)(GLdouble c)=NULL;
void			      (APIENTRY *fnglIndexdv)(const GLdouble *c)=NULL;
void			      (APIENTRY *fnglIndexf)(GLfloat c)=NULL;
void			      (APIENTRY *fnglIndexfv)(const GLfloat *c)=NULL;
void			      (APIENTRY *fnglIndexi)(GLint c)=NULL;
void			      (APIENTRY *fnglIndexiv)(const GLint *c)=NULL;
void			      (APIENTRY *fnglIndexs)(GLshort c)=NULL;
void			      (APIENTRY *fnglIndexsv)(const GLshort *c)=NULL;
void			      (APIENTRY *fnglIndexub)(GLubyte c)=NULL;
void			      (APIENTRY *fnglIndexubv)(const GLubyte *c)=NULL;
void			      (APIENTRY *fnglInitNames)(void)=NULL;
void			      (APIENTRY *fnglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer)=NULL;
GLboolean		    (APIENTRY *fnglIsEnabled)(GLenum cap)=NULL;
GLboolean		    (APIENTRY *fnglIsList)(GLuint list)=NULL;
GLboolean		    (APIENTRY *fnglIsTexture)(GLuint texture)=NULL;
void			      (APIENTRY *fnglLightModelf)(GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglLightModelfv)(GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglLightModeli)(GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglLightModeliv)(GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglLightf)(GLenum light, GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglLightfv)(GLenum light, GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglLighti)(GLenum light, GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglLightiv)(GLenum light, GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglLineStipple)(GLint factor, GLushort pattern)=NULL;
void			      (APIENTRY *fnglLineWidth)(GLfloat width)=NULL;
void			      (APIENTRY *fnglListBase)(GLuint base)=NULL;
void			      (APIENTRY *fnglLoadIdentity)(void)=NULL;
void			      (APIENTRY *fnglLoadMatrixd)(const GLdouble *m)=NULL;
void			      (APIENTRY *fnglLoadMatrixf)(const GLfloat *m)=NULL;
void			      (APIENTRY *fnglLoadName)(GLuint name)=NULL;
void			      (APIENTRY *fnglLogicOp)(GLenum opcode)=NULL;
void			      (APIENTRY *fnglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)=NULL;
void			      (APIENTRY *fnglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)=NULL;
void			      (APIENTRY *fnglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)=NULL;
void			      (APIENTRY *fnglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)=NULL;
void			      (APIENTRY *fnglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2)=NULL;
void			      (APIENTRY *fnglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2)=NULL;
void			      (APIENTRY *fnglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)=NULL;
void			      (APIENTRY *fnglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)=NULL;
void			      (APIENTRY *fnglMaterialf)(GLenum face, GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglMateriali)(GLenum face, GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglMaterialiv)(GLenum face, GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglMatrixMode)(GLenum mode)=NULL;
void			      (APIENTRY *fnglMultMatrixd)(const GLdouble *m)=NULL;
void			      (APIENTRY *fnglMultMatrixf)(const GLfloat *m)=NULL;
void			      (APIENTRY *fnglNewList)(GLuint list, GLenum mode)=NULL;
void			      (APIENTRY *fnglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz)=NULL;
void			      (APIENTRY *fnglNormal3bv)(const GLbyte *v)=NULL;
void			      (APIENTRY *fnglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz)=NULL;
void			      (APIENTRY *fnglNormal3dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz)=NULL;
void			      (APIENTRY *fnglNormal3fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglNormal3i)(GLint nx, GLint ny, GLint nz)=NULL;
void			      (APIENTRY *fnglNormal3iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglNormal3s)(GLshort nx, GLshort ny, GLshort nz)=NULL;
void			      (APIENTRY *fnglNormal3sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer)=NULL;
void			      (APIENTRY *fnglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)=NULL;
void			      (APIENTRY *fnglPassThrough)(GLfloat token)=NULL;
void			      (APIENTRY *fnglPixelMapfv)(GLenum map, GLint mapsize, const GLfloat *values)=NULL;
void			      (APIENTRY *fnglPixelMapuiv)(GLenum map, GLint mapsize, const GLuint *values)=NULL;
void			      (APIENTRY *fnglPixelMapusv)(GLenum map, GLint mapsize, const GLushort *values)=NULL;
void			      (APIENTRY *fnglPixelStoref)(GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglPixelStorei)(GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglPixelTransferf)(GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglPixelTransferi)(GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglPixelZoom)(GLfloat xfactor, GLfloat yfactor)=NULL;
void			      (APIENTRY *fnglPointSize)(GLfloat size)=NULL;
void			      (APIENTRY *fnglPolygonMode)(GLenum face, GLenum mode)=NULL;
void			      (APIENTRY *fnglPolygonOffset)(GLfloat factor, GLfloat units)=NULL;
void			      (APIENTRY *fnglPolygonStipple)(const GLubyte *mask)=NULL;
void			      (APIENTRY *fnglPopAttrib)(void)=NULL;
void			      (APIENTRY *fnglPopClientAttrib)(void)=NULL;
void			      (APIENTRY *fnglPopMatrix)(void)=NULL;
void			      (APIENTRY *fnglPopName)(void)=NULL;
void			      (APIENTRY *fnglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities)=NULL;
void			      (APIENTRY *fnglPushAttrib)(GLbitfield mask)=NULL;
void			      (APIENTRY *fnglPushClientAttrib)(GLbitfield mask)=NULL;
void			      (APIENTRY *fnglPushMatrix)(void)=NULL;
void			      (APIENTRY *fnglPushName)(GLuint name)=NULL;
void			      (APIENTRY *fnglRasterPos2d)(GLdouble x, GLdouble y)=NULL;
void			      (APIENTRY *fnglRasterPos2dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglRasterPos2f)(GLfloat x, GLfloat y)=NULL;
void			      (APIENTRY *fnglRasterPos2fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglRasterPos2i)(GLint x, GLint y)=NULL;
void			      (APIENTRY *fnglRasterPos2iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglRasterPos2s)(GLshort x, GLshort y)=NULL;
void			      (APIENTRY *fnglRasterPos2sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z)=NULL;
void			      (APIENTRY *fnglRasterPos3dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z)=NULL;
void			      (APIENTRY *fnglRasterPos3fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglRasterPos3i)(GLint x, GLint y, GLint z)=NULL;
void			      (APIENTRY *fnglRasterPos3iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglRasterPos3s)(GLshort x, GLshort y, GLshort z)=NULL;
void			      (APIENTRY *fnglRasterPos3sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)=NULL;
void			      (APIENTRY *fnglRasterPos4dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)=NULL;
void			      (APIENTRY *fnglRasterPos4fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglRasterPos4i)(GLint x, GLint y, GLint z, GLint w)=NULL;
void			      (APIENTRY *fnglRasterPos4iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w)=NULL;
void			      (APIENTRY *fnglRasterPos4sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglReadBuffer)(GLenum mode)=NULL;
void			      (APIENTRY *fnglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)=NULL;
void			      (APIENTRY *fnglRectdv)(const GLdouble *v1, const GLdouble *v2)=NULL;
void			      (APIENTRY *fnglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)=NULL;
void			      (APIENTRY *fnglRectfv)(const GLfloat *v1, const GLfloat *v2)=NULL;
void			      (APIENTRY *fnglRecti)(GLint x1, GLint y1, GLint x2, GLint y2)=NULL;
void			      (APIENTRY *fnglRectiv)(const GLint *v1, const GLint *v2)=NULL;
void			      (APIENTRY *fnglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2)=NULL;
void			      (APIENTRY *fnglRectsv)(const GLshort *v1, const GLshort *v2)=NULL;
GLint			      (APIENTRY *fnglRenderMode)(GLenum mode)=NULL;
void			      (APIENTRY *fnglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)=NULL;
void			      (APIENTRY *fnglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)=NULL;
void			      (APIENTRY *fnglScaled)(GLdouble x, GLdouble y, GLdouble z)=NULL;
void			      (APIENTRY *fnglScalef)(GLfloat x, GLfloat y, GLfloat z)=NULL;
void			      (APIENTRY *fnglScissor)(GLint x, GLint y, GLsizei width, GLsizei height)=NULL;
void			      (APIENTRY *fnglSelectBuffer)(GLsizei size, GLuint *buffer)=NULL;
void			      (APIENTRY *fnglShadeModel)(GLenum mode)=NULL;
void			      (APIENTRY *fnglStencilFunc)(GLenum func, GLint ref, GLuint mask)=NULL;
void			      (APIENTRY *fnglStencilMask)(GLuint mask)=NULL;
void			      (APIENTRY *fnglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass)=NULL;
void			      (APIENTRY *fnglTexCoord1d)(GLdouble s)=NULL;
void			      (APIENTRY *fnglTexCoord1dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglTexCoord1f)(GLfloat s)=NULL;
void			      (APIENTRY *fnglTexCoord1fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglTexCoord1i)(GLint s)=NULL;
void			      (APIENTRY *fnglTexCoord1iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglTexCoord1s)(GLshort s)=NULL;
void			      (APIENTRY *fnglTexCoord1sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglTexCoord2d)(GLdouble s, GLdouble t)=NULL;
void			      (APIENTRY *fnglTexCoord2dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglTexCoord2f)(GLfloat s, GLfloat t)=NULL;
void			      (APIENTRY *fnglTexCoord2fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglTexCoord2i)(GLint s, GLint t)=NULL;
void			      (APIENTRY *fnglTexCoord2iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglTexCoord2s)(GLshort s, GLshort t)=NULL;
void			      (APIENTRY *fnglTexCoord2sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r)=NULL;
void			      (APIENTRY *fnglTexCoord3dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r)=NULL;
void			      (APIENTRY *fnglTexCoord3fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglTexCoord3i)(GLint s, GLint t, GLint r)=NULL;
void			      (APIENTRY *fnglTexCoord3iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglTexCoord3s)(GLshort s, GLshort t, GLshort r)=NULL;
void			      (APIENTRY *fnglTexCoord3sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q)=NULL;
void			      (APIENTRY *fnglTexCoord4dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)=NULL;
void			      (APIENTRY *fnglTexCoord4fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglTexCoord4i)(GLint s, GLint t, GLint r, GLint q)=NULL;
void			      (APIENTRY *fnglTexCoord4iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q)=NULL;
void			      (APIENTRY *fnglTexCoord4sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)=NULL;
void			      (APIENTRY *fnglTexEnvf)(GLenum target, GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglTexEnvi)(GLenum target, GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglTexEnviv)(GLenum target, GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglTexGend)(GLenum coord, GLenum pname, GLdouble param)=NULL;
void			      (APIENTRY *fnglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params)=NULL;
void			      (APIENTRY *fnglTexGenf)(GLenum coord, GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglTexGeni)(GLenum coord, GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglTexGeniv)(GLenum coord, GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglTexImage1D)(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglTexParameterf)(GLenum target, GLenum pname, GLfloat param)=NULL;
void			      (APIENTRY *fnglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params)=NULL;
void			      (APIENTRY *fnglTexParameteri)(GLenum target, GLenum pname, GLint param)=NULL;
void			      (APIENTRY *fnglTexParameteriv)(GLenum target, GLenum pname, const GLint *params)=NULL;
void			      (APIENTRY *fnglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
void			      (APIENTRY *fnglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)=NULL;
void		        (APIENTRY *fnglTranslated)(GLdouble x, GLdouble y, GLdouble z)=NULL;
void			      (APIENTRY *fnglTranslatef)(GLfloat x, GLfloat y, GLfloat z)=NULL;
void			      (APIENTRY *fnglVertex2d)(GLdouble x, GLdouble y)=NULL;
void			      (APIENTRY *fnglVertex2dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglVertex2f)(GLfloat x, GLfloat y)=NULL;
void			      (APIENTRY *fnglVertex2fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglVertex2i)(GLint x, GLint y)=NULL;
void			      (APIENTRY *fnglVertex2iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglVertex2s)(GLshort x, GLshort y)=NULL;
void			      (APIENTRY *fnglVertex2sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglVertex3d)(GLdouble x, GLdouble y, GLdouble z)=NULL;
void			      (APIENTRY *fnglVertex3dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglVertex3f)(GLfloat x, GLfloat y, GLfloat z)=NULL;
void			      (APIENTRY *fnglVertex3fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglVertex3i)(GLint x, GLint y, GLint z)=NULL;
void			      (APIENTRY *fnglVertex3iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglVertex3s)(GLshort x, GLshort y, GLshort z)=NULL;
void			      (APIENTRY *fnglVertex3sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)=NULL;
void			      (APIENTRY *fnglVertex4dv)(const GLdouble *v)=NULL;
void			      (APIENTRY *fnglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)=NULL;
void			      (APIENTRY *fnglVertex4fv)(const GLfloat *v)=NULL;
void			      (APIENTRY *fnglVertex4i)(GLint x, GLint y, GLint z, GLint w)=NULL;
void			      (APIENTRY *fnglVertex4iv)(const GLint *v)=NULL;
void			      (APIENTRY *fnglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w)=NULL;
void			      (APIENTRY *fnglVertex4sv)(const GLshort *v)=NULL;
void			      (APIENTRY *fnglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)=NULL;
void			      (APIENTRY *fnglViewport)(GLint x, GLint y, GLsizei width, GLsizei height)=NULL;

//////////////// For GL_ARB_multitexture extension. //////////////////////

void			(APIENTRY *fnglActiveTextureARB)(GLenum target);
void			(APIENTRY *fnglClientActiveTextureARB)(GLenum target);
void      (APIENTRY *fnglMultiTexCoord1dARB)(GLenum target, GLdouble s);
void      (APIENTRY *fnglMultiTexCoord1dvARB)(GLenum target, const GLdouble *v);
void      (APIENTRY *fnglMultiTexCoord1fARB)(GLenum target, GLfloat s);
void      (APIENTRY *fnglMultiTexCoord1fvARB)(GLenum target, const GLfloat *v);
void      (APIENTRY *fnglMultiTexCoord1iARB)(GLenum target, GLint s);
void      (APIENTRY *fnglMultiTexCoord1ivARB)(GLenum target, const GLint *v);
void      (APIENTRY *fnglMultiTexCoord1sARB)(GLenum target, GLshort s);
void      (APIENTRY *fnglMultiTexCoord1svARB)(GLenum target, const GLshort *v);
void      (APIENTRY *fnglMultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t);
void      (APIENTRY *fnglMultiTexCoord2dvARB)(GLenum target, const GLdouble *v);
void      (APIENTRY *fnglMultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);
void      (APIENTRY *fnglMultiTexCoord2fvARB)(GLenum target, const GLfloat *v);
void      (APIENTRY *fnglMultiTexCoord2iARB)(GLenum target, GLint s, GLint t);
void      (APIENTRY *fnglMultiTexCoord2ivARB)(GLenum target, const GLint *v);
void      (APIENTRY *fnglMultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t);
void      (APIENTRY *fnglMultiTexCoord2svARB)(GLenum target, const GLshort *v);
void      (APIENTRY *fnglMultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
void      (APIENTRY *fnglMultiTexCoord3dvARB)(GLenum target, const GLdouble *v);
void      (APIENTRY *fnglMultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
void      (APIENTRY *fnglMultiTexCoord3fvARB)(GLenum target, const GLfloat *v);
void      (APIENTRY *fnglMultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r);
void      (APIENTRY *fnglMultiTexCoord3ivARB)(GLenum target, const GLint *v);
void      (APIENTRY *fnglMultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r);
void      (APIENTRY *fnglMultiTexCoord3svARB)(GLenum target, const GLshort *v);
void      (APIENTRY *fnglMultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void      (APIENTRY *fnglMultiTexCoord4dvARB)(GLenum target, const GLdouble *v);
void      (APIENTRY *fnglMultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void      (APIENTRY *fnglMultiTexCoord4fvARB)(GLenum target, const GLfloat *v);
void      (APIENTRY *fnglMultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q);
void      (APIENTRY *fnglMultiTexCoord4ivARB)(GLenum target, const GLint *v);
void      (APIENTRY *fnglMultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
void      (APIENTRY *fnglMultiTexCoord4svARB)(GLenum target, const GLshort *v);

///////////////// For GL_EXT_compiled_vertex_array extension. ////////////

void			(APIENTRY *fnglLockArraysEXT)(GLint first, GLsizei count);
void			(APIENTRY *fnglUnlockArraysEXT)(void);

///////////////// For WGL_EXT_swap_control extension. ////////////

GLboolean (APIENTRY *fnwglSwapIntervalEXT)(GLint interval);
int       (APIENTRY *fnwglGetSwapIntervalEXT)(void);

///////////////// For GL_EXT_point_parameters extension. ////////////

void      (APIENTRY *fnglPointParameterfEXT)(GLenum pname, GLfloat param);
void      (APIENTRY *fnglPointParameterfvEXT)(GLenum pname, GLfloat *params);

//////////////////////////////////////////////////////////////////////////

void OpenGLFindDriver(char *openglLib) {
  openglLib[0]=0;

  /* attempt to autodetermine opengl driver */
  /* Voodoo Rush, Voodoo1, Voodoo2 */
  if (!*openglLib) {
    HINSTANCE found;
    found = LoadLibrary("glide2x.dll");
    if (!found) found = LoadLibrary("glide3x.dll");

    if (found) {
      FreeLibrary(found);
#if (MINIGL)
      strcpy(openglLib, "3dfxgl.dll");
      found = LoadLibrary(openglLib);
      if (!found) {
        strcpy(openglLib, "3dfxvgl.dll");
        found = LoadLibrary(openglLib);
      }
#else
      strcpy(openglLib, "3dfxvgl.dll");
      found = LoadLibrary(openglLib);
#endif
      if (found) {
        FreeLibrary(found);
        return;
      }
    }
  }

#if (MINIGL)
  /* PowerVR? Will this work */
  if (!*openglLib) {
    HINSTANCE found;
    found = LoadLibrary("pvrhal32.dll");
    if (found) {
      FreeLibrary(found);
      strcpy(openglLib, "pvrgl.dll");
      found = LoadLibrary(openglLib);
      if (found) {
        FreeLibrary(found);
        return;
      }
    }
  }
#endif

  /* Fallback to system opengl MCD/ICD driver */
  strcpy(openglLib, "opengl32.dll");
}

int OpenGLInit(char *lib) {
  /* load dll */ 
  openglInst = LoadLibrary(lib); 
  if (openglInst == NULL) { 
    MessageBox(GetFocus(), "Could not open OpenGL Library", "Error", MB_OK); 
    openglInst = NULL; 
    return -666; 
  }

  /* load all the function pointers */
#if (MINIGL)
  if (!(fnglAlphaFunc     = (void*)GetProcAddress(openglInst, "glAlphaFunc"))) return -1;
  if (!(fnglBegin         = (void*)GetProcAddress(openglInst, "glBegin"))) return -2;
  if (!(fnglBindTexture   = (void*)GetProcAddress(openglInst, "glBindTexture"))) return -3;
  if (!(fnglBlendFunc     = (void*)GetProcAddress(openglInst, "glBlendFunc"))) return -4;
  if (!(fnglClear         = (void*)GetProcAddress(openglInst, "glClear"))) return -5;
  if (!(fnglClearColor    = (void*)GetProcAddress(openglInst, "glClearColor"))) return -6;
  if (!(fnglClearDepth    = (void*)GetProcAddress(openglInst, "glClearDepth"))) return -7;
  if (!(fnglColor3f       = (void*)GetProcAddress(openglInst, "glColor3f"))) return -8;
  if (!(fnglColor3fv      = (void*)GetProcAddress(openglInst, "glColor3fv"))) return -9;
  if (!(fnglColor4f       = (void*)GetProcAddress(openglInst, "glColor4f"))) return -10;
  if (!(fnglColor4fv      = (void*)GetProcAddress(openglInst, "glColor4fv"))) return -11;
  if (!(fnglCullFace      = (void*)GetProcAddress(openglInst, "glCullFace"))) return -12;
  if (!(fnglDepthFunc     = (void*)GetProcAddress(openglInst, "glDepthFunc"))) return -13;
  if (!(fnglDisable       = (void*)GetProcAddress(openglInst, "glDisable"))) return -14;
  if (!(fnglEnable        = (void*)GetProcAddress(openglInst, "glEnable"))) return -15;
  if (!(fnglEnd           = (void*)GetProcAddress(openglInst, "glEnd"))) return -16;
  if (!(fnglFinish        = (void*)GetProcAddress(openglInst, "glFinish"))) return -17;
  if (!(fnglFrontFace     = (void*)GetProcAddress(openglInst, "glFrontFace"))) return -18;
  if (!(fnglFrustum       = (void*)GetProcAddress(openglInst, "glFrustum"))) return -19;
  if (!(fnglGetError      = (void*)GetProcAddress(openglInst, "glGetError"))) return -20;
  if (!(fnglGetFloatv     = (void*)GetProcAddress(openglInst, "glGetFloatv"))) return -21;
  if (!(fnglGetIntegerv   = (void*)GetProcAddress(openglInst, "glGetIntegerv"))) return -22;
  if (!(fnglGetString     = (void*)GetProcAddress(openglInst, "glGetString"))) return -23;
  if (!(fnglHint          = (void*)GetProcAddress(openglInst, "glHint"))) return -24;
  if (!(fnglIsEnabled     = (void*)GetProcAddress(openglInst, "glIsEnabled"))) return -25;
  if (!(fnglLoadIdentity  = (void*)GetProcAddress(openglInst, "glLoadIdentity"))) return -26;
  if (!(fnglLoadMatrixf   = (void*)GetProcAddress(openglInst, "glLoadMatrixf"))) return -27;
  if (!(fnglMatrixMode    = (void*)GetProcAddress(openglInst, "glMatrixMode"))) return -28;
  if (!(fnglMultMatrixf   = (void*)GetProcAddress(openglInst, "glMultMatrixf"))) return -29;
  if (!(fnglOrtho         = (void*)GetProcAddress(openglInst, "glOrtho"))) return -30;
  if (!(fnglPixelStorei   = (void*)GetProcAddress(openglInst, "glPixelStorei"))) return -31;
  if (!(fnglPolygonOffset = (void*)GetProcAddress(openglInst, "glPolygonOffset"))) return -32;
  if (!(fnglPopMatrix     = (void*)GetProcAddress(openglInst, "glPopMatrix"))) return -33;
  if (!(fnglPushMatrix    = (void*)GetProcAddress(openglInst, "glPushMatrix"))) return -34;
  if (!(fnglRotatef       = (void*)GetProcAddress(openglInst, "glRotatef"))) return -35;
  if (!(fnglScalef        = (void*)GetProcAddress(openglInst, "glScalef"))) return -36;
  if (!(fnglShadeModel    = (void*)GetProcAddress(openglInst, "glShadeModel"))) return -37;
  if (!(fnglTexCoord2f    = (void*)GetProcAddress(openglInst, "glTexCoord2f"))) return -38;
  if (!(fnglTexCoord2fv   = (void*)GetProcAddress(openglInst, "glTexCoord2fv"))) return -39;
  if (!(fnglTexCoord3f    = (void*)GetProcAddress(openglInst, "glTexCoord3f"))) return -40;
  if (!(fnglTexCoord3fv   = (void*)GetProcAddress(openglInst, "glTexCoord3fv"))) return -41;
  if (!(fnglTexEnvf       = (void*)GetProcAddress(openglInst, "glTexEnvf"))) return -42;
  if (!(fnglTexImage2D    = (void*)GetProcAddress(openglInst, "glTexImage2D"))) return -43;
  if (!(fnglTexParameterf = (void*)GetProcAddress(openglInst, "glTexParameterf"))) return -44;
  if (!(fnglTranslatef    = (void*)GetProcAddress(openglInst, "glTranslatef"))) return -45;
  if (!(fnglVertex2f      = (void*)GetProcAddress(openglInst, "glVertex2f"))) return -46;
  if (!(fnglVertex3f      = (void*)GetProcAddress(openglInst, "glVertex3f"))) return -47;
  if (!(fnglVertex3fv     = (void*)GetProcAddress(openglInst, "glVertex3fv"))) return -48;
  if (!(fnglVertex4f      = (void*)GetProcAddress(openglInst, "glVertex4f"))) return -49;
  if (!(fnglViewport      = (void*)GetProcAddress(openglInst, "glViewport"))) return -50;
#else
	if(!(fnglAccum                  = (void *)GetProcAddress(openglInst, "glAccum"))) return -1;
	if(!(fnglAlphaFunc              = (void *)GetProcAddress(openglInst, "glAlphaFunc"))) return -1;
	if(!(fnglAreTexturesResident    = (void *)GetProcAddress(openglInst, "glAreTexturesResident"))) return -1;
	if(!(fnglArrayElement           = (void *)GetProcAddress(openglInst, "glArrayElement"))) return -1;
	if(!(fnglBegin                  = (void *)GetProcAddress(openglInst, "glBegin"))) return -1;
	if(!(fnglBindTexture            = (void *)GetProcAddress(openglInst, "glBindTexture"))) return -1;
	if(!(fnglBitmap                 = (void *)GetProcAddress(openglInst, "glBitmap"))) return -1;
	if(!(fnglBlendFunc              = (void *)GetProcAddress(openglInst, "glBlendFunc"))) return -1;
	if(!(fnglCallList               = (void *)GetProcAddress(openglInst, "glCallList"))) return -1;
	if(!(fnglCallLists              = (void *)GetProcAddress(openglInst, "glCallLists"))) return -1;
	if(!(fnglClear                  = (void *)GetProcAddress(openglInst, "glClear"))) return -1;
	if(!(fnglClearAccum             = (void *)GetProcAddress(openglInst, "glClearAccum"))) return -1;
	if(!(fnglClearColor             = (void *)GetProcAddress(openglInst, "glClearColor"))) return -1;
	if(!(fnglClearDepth             = (void *)GetProcAddress(openglInst, "glClearDepth"))) return -1;
	if(!(fnglClearIndex             = (void *)GetProcAddress(openglInst, "glClearIndex"))) return -1;
	if(!(fnglClearStencil           = (void *)GetProcAddress(openglInst, "glClearStencil"))) return -1;
	if(!(fnglClipPlane              = (void *)GetProcAddress(openglInst, "glClipPlane"))) return -1;
	if(!(fnglColor3b                = (void *)GetProcAddress(openglInst, "glColor3b"))) return -1;
	if(!(fnglColor3bv               = (void *)GetProcAddress(openglInst, "glColor3bv"))) return -1;
	if(!(fnglColor3d                = (void *)GetProcAddress(openglInst, "glColor3d"))) return -1;
	if(!(fnglColor3dv               = (void *)GetProcAddress(openglInst, "glColor3dv"))) return -1;
	if(!(fnglColor3f                = (void *)GetProcAddress(openglInst, "glColor3f"))) return -1;
	if(!(fnglColor3fv               = (void *)GetProcAddress(openglInst, "glColor3fv"))) return -1;
	if(!(fnglColor3i                = (void *)GetProcAddress(openglInst, "glColor3i"))) return -1;
	if(!(fnglColor3iv               = (void *)GetProcAddress(openglInst, "glColor3iv"))) return -1;
	if(!(fnglColor3s                = (void *)GetProcAddress(openglInst, "glColor3s"))) return -1;
	if(!(fnglColor3sv               = (void *)GetProcAddress(openglInst, "glColor3sv"))) return -1;
	if(!(fnglColor3ub               = (void *)GetProcAddress(openglInst, "glColor3ub"))) return -1;
	if(!(fnglColor3ubv              = (void *)GetProcAddress(openglInst, "glColor3ubv"))) return -1;
	if(!(fnglColor3ui               = (void *)GetProcAddress(openglInst, "glColor3ui"))) return -1;
	if(!(fnglColor3uiv              = (void *)GetProcAddress(openglInst, "glColor3uiv"))) return -1;
	if(!(fnglColor3us               = (void *)GetProcAddress(openglInst, "glColor3us"))) return -1;
	if(!(fnglColor3usv              = (void *)GetProcAddress(openglInst, "glColor3usv"))) return -1;
	if(!(fnglColor4b                = (void *)GetProcAddress(openglInst, "glColor4b"))) return -1;
	if(!(fnglColor4bv               = (void *)GetProcAddress(openglInst, "glColor4bv"))) return -1;
	if(!(fnglColor4d                = (void *)GetProcAddress(openglInst, "glColor4d"))) return -1;
	if(!(fnglColor4dv               = (void *)GetProcAddress(openglInst, "glColor4dv"))) return -1;
	if(!(fnglColor4f                = (void *)GetProcAddress(openglInst, "glColor4f"))) return -1;
	if(!(fnglColor4fv               = (void *)GetProcAddress(openglInst, "glColor4fv"))) return -1;
	if(!(fnglColor4i                = (void *)GetProcAddress(openglInst, "glColor4i"))) return -1;
	if(!(fnglColor4iv               = (void *)GetProcAddress(openglInst, "glColor4iv"))) return -1;
	if(!(fnglColor4s                = (void *)GetProcAddress(openglInst, "glColor4s"))) return -1;
	if(!(fnglColor4sv               = (void *)GetProcAddress(openglInst, "glColor4sv"))) return -1;
	if(!(fnglColor4ub               = (void *)GetProcAddress(openglInst, "glColor4ub"))) return -1;
	if(!(fnglColor4ubv              = (void *)GetProcAddress(openglInst, "glColor4ubv"))) return -1;
	if(!(fnglColor4ui               = (void *)GetProcAddress(openglInst, "glColor4ui"))) return -1;
	if(!(fnglColor4uiv              = (void *)GetProcAddress(openglInst, "glColor4uiv"))) return -1;
	if(!(fnglColor4us               = (void *)GetProcAddress(openglInst, "glColor4us"))) return -1;
	if(!(fnglColor4usv              = (void *)GetProcAddress(openglInst, "glColor4usv"))) return -1;
	if(!(fnglColorMask              = (void *)GetProcAddress(openglInst, "glColorMask"))) return -1;
	if(!(fnglColorMaterial          = (void *)GetProcAddress(openglInst, "glColorMaterial"))) return -1;
	if(!(fnglColorPointer           = (void *)GetProcAddress(openglInst, "glColorPointer"))) return -1;
	if(!(fnglCopyPixels             = (void *)GetProcAddress(openglInst, "glCopyPixels"))) return -1;
	if(!(fnglCopyTexImage1D         = (void *)GetProcAddress(openglInst, "glCopyTexImage1D"))) return -1;
	if(!(fnglCopyTexImage2D         = (void *)GetProcAddress(openglInst, "glCopyTexImage2D"))) return -1;
	if(!(fnglCopyTexSubImage1D      = (void *)GetProcAddress(openglInst, "glCopyTexSubImage1D"))) return -1;
	if(!(fnglCopyTexSubImage2D      = (void *)GetProcAddress(openglInst, "glCopyTexSubImage2D"))) return -1;
	if(!(fnglCullFace               = (void *)GetProcAddress(openglInst, "glCullFace"))) return -1;
	if(!(fnglDeleteLists            = (void *)GetProcAddress(openglInst, "glDeleteLists"))) return -1;
	if(!(fnglDeleteTextures         = (void *)GetProcAddress(openglInst, "glDeleteTextures"))) return -1;
	if(!(fnglDepthFunc              = (void *)GetProcAddress(openglInst, "glDepthFunc"))) return -1;
	if(!(fnglDepthMask              = (void *)GetProcAddress(openglInst, "glDepthMask"))) return -1;
	if(!(fnglDepthRange             = (void *)GetProcAddress(openglInst, "glDepthRange"))) return -1;
	if(!(fnglDisable                = (void *)GetProcAddress(openglInst, "glDisable"))) return -1;
	if(!(fnglDisableClientState     = (void *)GetProcAddress(openglInst, "glDisableClientState"))) return -1;
	if(!(fnglDrawArrays             = (void *)GetProcAddress(openglInst, "glDrawArrays"))) return -1;
	if(!(fnglDrawBuffer             = (void *)GetProcAddress(openglInst, "glDrawBuffer"))) return -1;
	if(!(fnglDrawElements           = (void *)GetProcAddress(openglInst, "glDrawElements"))) return -1;
	if(!(fnglDrawPixels             = (void *)GetProcAddress(openglInst, "glDrawPixels"))) return -1;
	if(!(fnglEdgeFlag               = (void *)GetProcAddress(openglInst, "glEdgeFlag"))) return -1;
	if(!(fnglEdgeFlagPointer        = (void *)GetProcAddress(openglInst, "glEdgeFlagPointer"))) return -1;
	if(!(fnglEdgeFlagv              = (void *)GetProcAddress(openglInst, "glEdgeFlagv"))) return -1;
	if(!(fnglEnable                 = (void *)GetProcAddress(openglInst, "glEnable"))) return -1;
	if(!(fnglEnableClientState      = (void *)GetProcAddress(openglInst, "glEnableClientState"))) return -1;
	if(!(fnglEnd                    = (void *)GetProcAddress(openglInst, "glEnd"))) return -1;
	if(!(fnglEndList                = (void *)GetProcAddress(openglInst, "glEndList"))) return -1;
	if(!(fnglEvalCoord1d            = (void *)GetProcAddress(openglInst, "glEvalCoord1d"))) return -1;
	if(!(fnglEvalCoord1dv           = (void *)GetProcAddress(openglInst, "glEvalCoord1dv"))) return -1;	
	if(!(fnglEvalCoord1f            = (void *)GetProcAddress(openglInst, "glEvalCoord1f"))) return -1;
	if(!(fnglEvalCoord1fv           = (void *)GetProcAddress(openglInst, "glEvalCoord1fv"))) return -1;
	if(!(fnglEvalCoord2d            = (void *)GetProcAddress(openglInst, "glEvalCoord2d"))) return -1;
	if(!(fnglEvalCoord2dv           = (void *)GetProcAddress(openglInst, "glEvalCoord2dv"))) return -1;
	if(!(fnglEvalCoord2f            = (void *)GetProcAddress(openglInst, "glEvalCoord2f"))) return -1;
	if(!(fnglEvalCoord2fv           = (void *)GetProcAddress(openglInst, "glEvalCoord2fv"))) return -1;
	if(!(fnglEvalMesh1              = (void *)GetProcAddress(openglInst, "glEvalMesh1"))) return -1;
	if(!(fnglEvalMesh2              = (void *)GetProcAddress(openglInst, "glEvalMesh2"))) return -1;
	if(!(fnglEvalPoint1             = (void *)GetProcAddress(openglInst, "glEvalPoint1"))) return -1;
	if(!(fnglEvalPoint2             = (void *)GetProcAddress(openglInst, "glEvalPoint2"))) return -1;
	if(!(fnglFeedbackBuffer         = (void *)GetProcAddress(openglInst, "glFeedbackBuffer"))) return -1;
	if(!(fnglFinish                 = (void *)GetProcAddress(openglInst, "glFinish"))) return -1;
	if(!(fnglFlush                  = (void *)GetProcAddress(openglInst, "glFlush"))) return -1;
	if(!(fnglFogf                   = (void *)GetProcAddress(openglInst, "glFogf"))) return -1;
	if(!(fnglFogfv                  = (void *)GetProcAddress(openglInst, "glFogfv"))) return -1;
	if(!(fnglFogi                   = (void *)GetProcAddress(openglInst, "glFogi"))) return -1;
	if(!(fnglFogiv                  = (void *)GetProcAddress(openglInst, "glFogiv"))) return -1;
	if(!(fnglFrontFace              = (void *)GetProcAddress(openglInst, "glFrontFace"))) return -1;
	if(!(fnglFrustum                = (void *)GetProcAddress(openglInst, "glFrustum"))) return -1;
	if(!(fnglGenLists               = (void *)GetProcAddress(openglInst, "glGenLists"))) return -1;
	if(!(fnglGenTextures            = (void *)GetProcAddress(openglInst, "glGenTextures"))) return -1;
	if(!(fnglGetBooleanv            = (void *)GetProcAddress(openglInst, "glGetBooleanv"))) return -1;
	if(!(fnglGetClipPlane           = (void *)GetProcAddress(openglInst, "glGetClipPlane"))) return -1;
	if(!(fnglGetDoublev             = (void *)GetProcAddress(openglInst, "glGetDoublev"))) return -1;
	if(!(fnglGetError               = (void *)GetProcAddress(openglInst, "glGetError"))) return -1;
	if(!(fnglGetFloatv              = (void *)GetProcAddress(openglInst, "glGetFloatv"))) return -1;
	if(!(fnglGetIntegerv            = (void *)GetProcAddress(openglInst, "glGetIntegerv"))) return -1;
	if(!(fnglGetLightfv             = (void *)GetProcAddress(openglInst, "glGetLightfv"))) return -1;
	if(!(fnglGetLightiv             = (void *)GetProcAddress(openglInst, "glGetLightiv"))) return -1;
	if(!(fnglGetMapdv               = (void *)GetProcAddress(openglInst, "glGetMapdv"))) return -1;
	if(!(fnglGetMapfv               = (void *)GetProcAddress(openglInst, "glGetMapfv"))) return -1;
	if(!(fnglGetMapiv               = (void *)GetProcAddress(openglInst, "glGetMapiv"))) return -1;
	if(!(fnglGetMaterialfv          = (void *)GetProcAddress(openglInst, "glGetMaterialfv"))) return -1;
	if(!(fnglGetMaterialiv          = (void *)GetProcAddress(openglInst, "glGetMaterialiv"))) return -1;
	if(!(fnglGetPixelMapfv          = (void *)GetProcAddress(openglInst, "glGetPixelMapfv"))) return -1;
	if(!(fnglGetPixelMapuiv         = (void *)GetProcAddress(openglInst, "glGetPixelMapuiv"))) return -1;
	if(!(fnglGetPixelMapusv         = (void *)GetProcAddress(openglInst, "glGetPixelMapusv"))) return -1;
	if(!(fnglGetPointerv            = (void *)GetProcAddress(openglInst, "glGetPointerv"))) return -1;
	if(!(fnglGetPolygonStipple      = (void *)GetProcAddress(openglInst, "glGetPolygonStipple"))) return -1;
	if(!(fnglGetString              = (void *)GetProcAddress(openglInst, "glGetString"))) return -1;
	if(!(fnglGetTexEnvfv            = (void *)GetProcAddress(openglInst, "glGetTexEnvfv"))) return -1;
	if(!(fnglGetTexEnviv            = (void *)GetProcAddress(openglInst, "glGetTexEnviv"))) return -1;
	if(!(fnglGetTexGendv            = (void *)GetProcAddress(openglInst, "glGetTexGendv"))) return -1;
	if(!(fnglGetTexGenfv            = (void *)GetProcAddress(openglInst, "glGetTexGenfv"))) return -1;
	if(!(fnglGetTexGeniv            = (void *)GetProcAddress(openglInst, "glGetTexGeniv"))) return -1;
	if(!(fnglGetTexImage            = (void *)GetProcAddress(openglInst, "glGetTexImage"))) return -1;
	if(!(fnglGetTexLevelParameterfv = (void *)GetProcAddress(openglInst, "glGetTexLevelParameterfv"))) return -1;
	if(!(fnglGetTexLevelParameteriv = (void *)GetProcAddress(openglInst, "glGetTexLevelParameteriv"))) return -1;
	if(!(fnglGetTexParameterfv      = (void *)GetProcAddress(openglInst, "glGetTexParameterfv"))) return -1;
	if(!(fnglGetTexParameteriv      = (void *)GetProcAddress(openglInst, "glGetTexParameteriv"))) return -1;
	if(!(fnglHint                   = (void *)GetProcAddress(openglInst, "glHint"))) return -1;
	if(!(fnglIndexMask              = (void *)GetProcAddress(openglInst, "glIndexMask"))) return -1;
	if(!(fnglIndexPointer           = (void *)GetProcAddress(openglInst, "glIndexPointer"))) return -1;
	if(!(fnglIndexd                 = (void *)GetProcAddress(openglInst, "glIndexd"))) return -1;
	if(!(fnglIndexdv                = (void *)GetProcAddress(openglInst, "glIndexdv"))) return -1;
	if(!(fnglIndexf                 = (void *)GetProcAddress(openglInst, "glIndexf"))) return -1;
	if(!(fnglIndexfv                = (void *)GetProcAddress(openglInst, "glIndexfv"))) return -1;
	if(!(fnglIndexi                 = (void *)GetProcAddress(openglInst, "glIndexi"))) return -1;
	if(!(fnglIndexiv                = (void *)GetProcAddress(openglInst, "glIndexiv"))) return -1;
	if(!(fnglIndexs                 = (void *)GetProcAddress(openglInst, "glIndexs"))) return -1;
	if(!(fnglIndexsv                = (void *)GetProcAddress(openglInst, "glIndexsv"))) return -1;
	if(!(fnglIndexub                = (void *)GetProcAddress(openglInst, "glIndexub"))) return -1;
	if(!(fnglIndexubv               = (void *)GetProcAddress(openglInst, "glIndexubv"))) return -1;
	if(!(fnglInitNames              = (void *)GetProcAddress(openglInst, "glInitNames"))) return -1;
	if(!(fnglInterleavedArrays      = (void *)GetProcAddress(openglInst, "glInterleavedArrays"))) return -1;
	if(!(fnglIsEnabled              = (void *)GetProcAddress(openglInst, "glIsEnabled"))) return -1;
	if(!(fnglIsList                 = (void *)GetProcAddress(openglInst, "glIsList"))) return -1;
	if(!(fnglIsTexture              = (void *)GetProcAddress(openglInst, "glIsTexture"))) return -1;
	if(!(fnglLightModelf            = (void *)GetProcAddress(openglInst, "glLightModelf"))) return -1;
	if(!(fnglLightModelfv           = (void *)GetProcAddress(openglInst, "glLightModelfv"))) return -1;
	if(!(fnglLightModeli            = (void *)GetProcAddress(openglInst, "glLightModeli"))) return -1;
	if(!(fnglLightModeliv           = (void *)GetProcAddress(openglInst, "glLightModeliv"))) return -1;
	if(!(fnglLightf                 = (void *)GetProcAddress(openglInst, "glLightf"))) return -1;
	if(!(fnglLightfv                = (void *)GetProcAddress(openglInst, "glLightfv"))) return -1;
	if(!(fnglLighti                 = (void *)GetProcAddress(openglInst, "glLighti"))) return -1;
	if(!(fnglLightiv                = (void *)GetProcAddress(openglInst, "glLightiv"))) return -1;
	if(!(fnglLineStipple            = (void *)GetProcAddress(openglInst, "glLineStipple"))) return -1;
	if(!(fnglLineWidth              = (void *)GetProcAddress(openglInst, "glLineWidth"))) return -1;
	if(!(fnglListBase               = (void *)GetProcAddress(openglInst, "glListBase"))) return -1;
	if(!(fnglLoadIdentity           = (void *)GetProcAddress(openglInst, "glLoadIdentity"))) return -1;
	if(!(fnglLoadMatrixd            = (void *)GetProcAddress(openglInst, "glLoadMatrixd"))) return -1;
	if(!(fnglLoadMatrixf            = (void *)GetProcAddress(openglInst, "glLoadMatrixf"))) return -1;
	if(!(fnglLoadName               = (void *)GetProcAddress(openglInst, "glLoadName"))) return -1;
	if(!(fnglLogicOp                = (void *)GetProcAddress(openglInst, "glLogicOp"))) return -1;
	if(!(fnglMap1d                  = (void *)GetProcAddress(openglInst, "glMap1d"))) return -1;
	if(!(fnglMap1f                  = (void *)GetProcAddress(openglInst, "glMap1f"))) return -1;
	if(!(fnglMap2d                  = (void *)GetProcAddress(openglInst, "glMap2d"))) return -1;
	if(!(fnglMap2f                  = (void *)GetProcAddress(openglInst, "glMap2f"))) return -1;
	if(!(fnglMapGrid1d              = (void *)GetProcAddress(openglInst, "glMapGrid1d"))) return -1;
	if(!(fnglMapGrid1f              = (void *)GetProcAddress(openglInst, "glMapGrid1f"))) return -1;
	if(!(fnglMapGrid2d              = (void *)GetProcAddress(openglInst, "glMapGrid2d"))) return -1;
	if(!(fnglMapGrid2f              = (void *)GetProcAddress(openglInst, "glMapGrid2f"))) return -1;
	if(!(fnglMaterialf              = (void *)GetProcAddress(openglInst, "glMaterialf"))) return -1;
	if(!(fnglMaterialfv             = (void *)GetProcAddress(openglInst, "glMaterialfv"))) return -1;
	if(!(fnglMateriali              = (void *)GetProcAddress(openglInst, "glMateriali"))) return -1;
	if(!(fnglMaterialiv             = (void *)GetProcAddress(openglInst, "glMaterialiv"))) return -1;
	if(!(fnglMatrixMode             = (void *)GetProcAddress(openglInst, "glMatrixMode"))) return -1;
	if(!(fnglMultMatrixd            = (void *)GetProcAddress(openglInst, "glMultMatrixd"))) return -1;
	if(!(fnglMultMatrixf            = (void *)GetProcAddress(openglInst, "glMultMatrixf"))) return -1;
	if(!(fnglNewList                = (void *)GetProcAddress(openglInst, "glNewList"))) return -1;
	if(!(fnglNormal3b               = (void *)GetProcAddress(openglInst, "glNormal3b"))) return -1;
	if(!(fnglNormal3bv              = (void *)GetProcAddress(openglInst, "glNormal3bv"))) return -1;
	if(!(fnglNormal3d               = (void *)GetProcAddress(openglInst, "glNormal3d"))) return -1;
	if(!(fnglNormal3dv              = (void *)GetProcAddress(openglInst, "glNormal3dv"))) return -1;
	if(!(fnglNormal3f               = (void *)GetProcAddress(openglInst, "glNormal3f"))) return -1;
	if(!(fnglNormal3fv              = (void *)GetProcAddress(openglInst, "glNormal3fv"))) return -1;
	if(!(fnglNormal3i               = (void *)GetProcAddress(openglInst, "glNormal3i"))) return -1;
	if(!(fnglNormal3iv              = (void *)GetProcAddress(openglInst, "glNormal3iv"))) return -1;
	if(!(fnglNormal3s               = (void *)GetProcAddress(openglInst, "glNormal3s"))) return -1;
	if(!(fnglNormal3sv              = (void *)GetProcAddress(openglInst, "glNormal3sv"))) return -1;
	if(!(fnglNormalPointer          = (void *)GetProcAddress(openglInst, "glNormalPointer"))) return -1;
	if(!(fnglOrtho                  = (void *)GetProcAddress(openglInst, "glOrtho"))) return -1;
	if(!(fnglPassThrough            = (void *)GetProcAddress(openglInst, "glPassThrough"))) return -1;
	if(!(fnglPixelMapfv             = (void *)GetProcAddress(openglInst, "glPixelMapfv"))) return -1;
	if(!(fnglPixelMapuiv            = (void *)GetProcAddress(openglInst, "glPixelMapuiv"))) return -1;
	if(!(fnglPixelMapusv            = (void *)GetProcAddress(openglInst, "glPixelMapusv"))) return -1;
	if(!(fnglPixelStoref            = (void *)GetProcAddress(openglInst, "glPixelStoref"))) return -1;
	if(!(fnglPixelStorei            = (void *)GetProcAddress(openglInst, "glPixelStorei"))) return -1;
	if(!(fnglPixelTransferf         = (void *)GetProcAddress(openglInst, "glPixelTransferf"))) return -1;
	if(!(fnglPixelTransferi         = (void *)GetProcAddress(openglInst, "glPixelTransferi"))) return -1;
	if(!(fnglPixelZoom              = (void *)GetProcAddress(openglInst, "glPixelZoom"))) return -1;
	if(!(fnglPointSize              = (void *)GetProcAddress(openglInst, "glPointSize"))) return -1;
	if(!(fnglPolygonMode            = (void *)GetProcAddress(openglInst, "glPolygonMode"))) return -1;
	if(!(fnglPolygonOffset          = (void *)GetProcAddress(openglInst, "glPolygonOffset"))) return -1;
	if(!(fnglPolygonStipple         = (void *)GetProcAddress(openglInst, "glPolygonStipple"))) return -1;
	if(!(fnglPopAttrib              = (void *)GetProcAddress(openglInst, "glPopAttrib"))) return -1;
	if(!(fnglPopClientAttrib        = (void *)GetProcAddress(openglInst, "glPopClientAttrib"))) return -1;
	if(!(fnglPopMatrix              = (void *)GetProcAddress(openglInst, "glPopMatrix"))) return -1;
	if(!(fnglPopName                = (void *)GetProcAddress(openglInst, "glPopName"))) return -1;
	if(!(fnglPrioritizeTextures     = (void *)GetProcAddress(openglInst, "glPrioritizeTextures"))) return -1;
	if(!(fnglPushAttrib             = (void *)GetProcAddress(openglInst, "glPushAttrib"))) return -1;
	if(!(fnglPushClientAttrib       = (void *)GetProcAddress(openglInst, "glPushClientAttrib"))) return -1;
	if(!(fnglPushMatrix             = (void *)GetProcAddress(openglInst, "glPushMatrix"))) return -1;
	if(!(fnglPushName               = (void *)GetProcAddress(openglInst, "glPushName"))) return -1;
	if(!(fnglRasterPos2d            = (void *)GetProcAddress(openglInst, "glRasterPos2d"))) return -1;
	if(!(fnglRasterPos2dv           = (void *)GetProcAddress(openglInst, "glRasterPos2dv"))) return -1;
	if(!(fnglRasterPos2f            = (void *)GetProcAddress(openglInst, "glRasterPos2f"))) return -1;
	if(!(fnglRasterPos2fv           = (void *)GetProcAddress(openglInst, "glRasterPos2fv"))) return -1;
	if(!(fnglRasterPos2i            = (void *)GetProcAddress(openglInst, "glRasterPos2i"))) return -1;
	if(!(fnglRasterPos2iv           = (void *)GetProcAddress(openglInst, "glRasterPos2iv"))) return -1;
	if(!(fnglRasterPos2s            = (void *)GetProcAddress(openglInst, "glRasterPos2s"))) return -1;
	if(!(fnglRasterPos2sv           = (void *)GetProcAddress(openglInst, "glRasterPos2sv"))) return -1;
	if(!(fnglRasterPos3d            = (void *)GetProcAddress(openglInst, "glRasterPos3d"))) return -1;
	if(!(fnglRasterPos3dv           = (void *)GetProcAddress(openglInst, "glRasterPos3dv"))) return -1;
	if(!(fnglRasterPos3f            = (void *)GetProcAddress(openglInst, "glRasterPos3f"))) return -1;
	if(!(fnglRasterPos3fv           = (void *)GetProcAddress(openglInst, "glRasterPos3fv"))) return -1;
	if(!(fnglRasterPos3i            = (void *)GetProcAddress(openglInst, "glRasterPos3i"))) return -1;
	if(!(fnglRasterPos3iv           = (void *)GetProcAddress(openglInst, "glRasterPos3iv"))) return -1;
	if(!(fnglRasterPos3s            = (void *)GetProcAddress(openglInst, "glRasterPos3s"))) return -1;
	if(!(fnglRasterPos3sv           = (void *)GetProcAddress(openglInst, "glRasterPos3sv"))) return -1;
	if(!(fnglRasterPos4d            = (void *)GetProcAddress(openglInst, "glRasterPos4d"))) return -1;
	if(!(fnglRasterPos4dv           = (void *)GetProcAddress(openglInst, "glRasterPos4dv"))) return -1;
	if(!(fnglRasterPos4f            = (void *)GetProcAddress(openglInst, "glRasterPos4f"))) return -1;
	if(!(fnglRasterPos4fv           = (void *)GetProcAddress(openglInst, "glRasterPos4fv"))) return -1;
	if(!(fnglRasterPos4i            = (void *)GetProcAddress(openglInst, "glRasterPos4i"))) return -1;
	if(!(fnglRasterPos4iv           = (void *)GetProcAddress(openglInst, "glRasterPos4iv"))) return -1;
	if(!(fnglRasterPos4s            = (void *)GetProcAddress(openglInst, "glRasterPos4s"))) return -1;
	if(!(fnglRasterPos4sv           = (void *)GetProcAddress(openglInst, "glRasterPos4sv"))) return -1;
	if(!(fnglReadBuffer             = (void *)GetProcAddress(openglInst, "glReadBuffer"))) return -1;
	if(!(fnglReadPixels             = (void *)GetProcAddress(openglInst, "glReadPixels"))) return -1;
	if(!(fnglRectd                  = (void *)GetProcAddress(openglInst, "glRectd"))) return -1;
	if(!(fnglRectdv                 = (void *)GetProcAddress(openglInst, "glRectdv"))) return -1;
	if(!(fnglRectf                  = (void *)GetProcAddress(openglInst, "glRectf"))) return -1;
	if(!(fnglRectfv                 = (void *)GetProcAddress(openglInst, "glRectfv"))) return -1;
	if(!(fnglRecti                  = (void *)GetProcAddress(openglInst, "glRecti"))) return -1;
	if(!(fnglRectiv                 = (void *)GetProcAddress(openglInst, "glRectiv"))) return -1;
	if(!(fnglRects                  = (void *)GetProcAddress(openglInst, "glRects"))) return -1;
	if(!(fnglRectsv                 = (void *)GetProcAddress(openglInst, "glRectsv"))) return -1;
	if(!(fnglRenderMode             = (void *)GetProcAddress(openglInst, "glRenderMode"))) return -1;
	if(!(fnglRotated                = (void *)GetProcAddress(openglInst, "glRotated"))) return -1;
	if(!(fnglRotatef                = (void *)GetProcAddress(openglInst, "glRotatef"))) return -1;
	if(!(fnglScaled                 = (void *)GetProcAddress(openglInst, "glScaled"))) return -1;
	if(!(fnglScalef                 = (void *)GetProcAddress(openglInst, "glScalef"))) return -1;
	if(!(fnglScissor                = (void *)GetProcAddress(openglInst, "glScissor"))) return -1;
	if(!(fnglSelectBuffer           = (void *)GetProcAddress(openglInst, "glSelectBuffer"))) return -1;
	if(!(fnglShadeModel             = (void *)GetProcAddress(openglInst, "glShadeModel"))) return -1;
	if(!(fnglStencilFunc            = (void *)GetProcAddress(openglInst, "glStencilFunc"))) return -1;
	if(!(fnglStencilMask            = (void *)GetProcAddress(openglInst, "glStencilMask"))) return -1;
	if(!(fnglStencilOp              = (void *)GetProcAddress(openglInst, "glStencilOp"))) return -1;
	if(!(fnglTexCoord1d             = (void *)GetProcAddress(openglInst, "glTexCoord1d"))) return -1;
	if(!(fnglTexCoord1dv            = (void *)GetProcAddress(openglInst, "glTexCoord1dv"))) return -1;
	if(!(fnglTexCoord1f             = (void *)GetProcAddress(openglInst, "glTexCoord1f"))) return -1;
	if(!(fnglTexCoord1fv            = (void *)GetProcAddress(openglInst, "glTexCoord1fv"))) return -1;
	if(!(fnglTexCoord1i             = (void *)GetProcAddress(openglInst, "glTexCoord1i"))) return -1;
	if(!(fnglTexCoord1iv            = (void *)GetProcAddress(openglInst, "glTexCoord1iv"))) return -1;
	if(!(fnglTexCoord1s             = (void *)GetProcAddress(openglInst, "glTexCoord1s"))) return -1;
	if(!(fnglTexCoord1sv            = (void *)GetProcAddress(openglInst, "glTexCoord1sv"))) return -1;
	if(!(fnglTexCoord2d             = (void *)GetProcAddress(openglInst, "glTexCoord2d"))) return -1;
	if(!(fnglTexCoord2dv            = (void *)GetProcAddress(openglInst, "glTexCoord2dv"))) return -1;
	if(!(fnglTexCoord2f             = (void *)GetProcAddress(openglInst, "glTexCoord2f"))) return -1;
	if(!(fnglTexCoord2fv            = (void *)GetProcAddress(openglInst, "glTexCoord2fv"))) return -1;
	if(!(fnglTexCoord2i             = (void *)GetProcAddress(openglInst, "glTexCoord2i"))) return -1;
	if(!(fnglTexCoord2iv            = (void *)GetProcAddress(openglInst, "glTexCoord2iv"))) return -1;
	if(!(fnglTexCoord2s             = (void *)GetProcAddress(openglInst, "glTexCoord2s"))) return -1;
	if(!(fnglTexCoord2sv            = (void *)GetProcAddress(openglInst, "glTexCoord2sv"))) return -1;
	if(!(fnglTexCoord3d             = (void *)GetProcAddress(openglInst, "glTexCoord3d"))) return -1;
	if(!(fnglTexCoord3dv            = (void *)GetProcAddress(openglInst, "glTexCoord3dv"))) return -1;
	if(!(fnglTexCoord3f             = (void *)GetProcAddress(openglInst, "glTexCoord3f"))) return -1;
	if(!(fnglTexCoord3fv            = (void *)GetProcAddress(openglInst, "glTexCoord3fv"))) return -1;
	if(!(fnglTexCoord3i             = (void *)GetProcAddress(openglInst, "glTexCoord3i"))) return -1;
	if(!(fnglTexCoord3iv            = (void *)GetProcAddress(openglInst, "glTexCoord3iv"))) return -1;
	if(!(fnglTexCoord3s             = (void *)GetProcAddress(openglInst, "glTexCoord3s"))) return -1;
	if(!(fnglTexCoord3sv            = (void *)GetProcAddress(openglInst, "glTexCoord3sv"))) return -1;
	if(!(fnglTexCoord4d             = (void *)GetProcAddress(openglInst, "glTexCoord4d"))) return -1;
	if(!(fnglTexCoord4dv            = (void *)GetProcAddress(openglInst, "glTexCoord4dv"))) return -1;
	if(!(fnglTexCoord4f             = (void *)GetProcAddress(openglInst, "glTexCoord4f"))) return -1;
	if(!(fnglTexCoord4fv            = (void *)GetProcAddress(openglInst, "glTexCoord4fv"))) return -1;
	if(!(fnglTexCoord4i             = (void *)GetProcAddress(openglInst, "glTexCoord4i"))) return -1;
	if(!(fnglTexCoord4iv            = (void *)GetProcAddress(openglInst, "glTexCoord4iv"))) return -1;
	if(!(fnglTexCoord4s             = (void *)GetProcAddress(openglInst, "glTexCoord4s"))) return -1;
	if(!(fnglTexCoord4sv            = (void *)GetProcAddress(openglInst, "glTexCoord4sv"))) return -1;
	if(!(fnglTexCoordPointer        = (void *)GetProcAddress(openglInst, "glTexCoordPointer"))) return -1;
	if(!(fnglTexEnvf                = (void *)GetProcAddress(openglInst, "glTexEnvf"))) return -1;
	if(!(fnglTexEnvfv               = (void *)GetProcAddress(openglInst, "glTexEnvfv"))) return -1;
	if(!(fnglTexEnvi                = (void *)GetProcAddress(openglInst, "glTexEnvi"))) return -1;
	if(!(fnglTexEnviv               = (void *)GetProcAddress(openglInst, "glTexEnviv"))) return -1;
	if(!(fnglTexGend                = (void *)GetProcAddress(openglInst, "glTexGend"))) return -1;
	if(!(fnglTexGendv               = (void *)GetProcAddress(openglInst, "glTexGendv"))) return -1;
	if(!(fnglTexGenf                = (void *)GetProcAddress(openglInst, "glTexGenf"))) return -1;
	if(!(fnglTexGenfv               = (void *)GetProcAddress(openglInst, "glTexGenfv"))) return -1;
	if(!(fnglTexGeni                = (void *)GetProcAddress(openglInst, "glTexGeni"))) return -1;
	if(!(fnglTexGeniv               = (void *)GetProcAddress(openglInst, "glTexGeniv"))) return -1;
	if(!(fnglTexImage1D             = (void *)GetProcAddress(openglInst, "glTexImage1D"))) return -1;
	if(!(fnglTexImage2D             = (void *)GetProcAddress(openglInst, "glTexImage2D"))) return -1;
	if(!(fnglTexParameterf          = (void *)GetProcAddress(openglInst, "glTexParameterf"))) return -1;
	if(!(fnglTexParameterfv         = (void *)GetProcAddress(openglInst, "glTexParameterfv"))) return -1;
	if(!(fnglTexParameteri          = (void *)GetProcAddress(openglInst, "glTexParameteri"))) return -1;
	if(!(fnglTexParameteriv         = (void *)GetProcAddress(openglInst, "glTexParameteriv"))) return -1;
	if(!(fnglTexSubImage1D          = (void *)GetProcAddress(openglInst, "glTexSubImage1D"))) return -1;
	if(!(fnglTexSubImage2D          = (void *)GetProcAddress(openglInst, "glTexSubImage2D"))) return -1;
	if(!(fnglTranslated             = (void *)GetProcAddress(openglInst, "glTranslated"))) return -1;
	if(!(fnglTranslatef             = (void *)GetProcAddress(openglInst, "glTranslatef"))) return -1;
	if(!(fnglVertex2d               = (void *)GetProcAddress(openglInst, "glVertex2d"))) return -1;
	if(!(fnglVertex2dv              = (void *)GetProcAddress(openglInst, "glVertex2dv"))) return -1;
	if(!(fnglVertex2f               = (void *)GetProcAddress(openglInst, "glVertex2f"))) return -1;
	if(!(fnglVertex2fv              = (void *)GetProcAddress(openglInst, "glVertex2fv"))) return -1;
	if(!(fnglVertex2i               = (void *)GetProcAddress(openglInst, "glVertex2i"))) return -1;
	if(!(fnglVertex2iv              = (void *)GetProcAddress(openglInst, "glVertex2iv"))) return -1;
	if(!(fnglVertex2s               = (void *)GetProcAddress(openglInst, "glVertex2s"))) return -1;
	if(!(fnglVertex2sv              = (void *)GetProcAddress(openglInst, "glVertex2sv"))) return -1;
	if(!(fnglVertex3d               = (void *)GetProcAddress(openglInst, "glVertex3d"))) return -1;
	if(!(fnglVertex3dv              = (void *)GetProcAddress(openglInst, "glVertex3dv"))) return -1;
	if(!(fnglVertex3f               = (void *)GetProcAddress(openglInst, "glVertex3f"))) return -1;
	if(!(fnglVertex3fv              = (void *)GetProcAddress(openglInst, "glVertex3fv"))) return -1;
	if(!(fnglVertex3i               = (void *)GetProcAddress(openglInst, "glVertex3i"))) return -1;
	if(!(fnglVertex3iv              = (void *)GetProcAddress(openglInst, "glVertex3iv"))) return -1;
	if(!(fnglVertex3s               = (void *)GetProcAddress(openglInst, "glVertex3s"))) return -1;
	if(!(fnglVertex3sv              = (void *)GetProcAddress(openglInst, "glVertex3sv"))) return -1;
	if(!(fnglVertex4d               = (void *)GetProcAddress(openglInst, "glVertex4d"))) return -1;
	if(!(fnglVertex4dv              = (void *)GetProcAddress(openglInst, "glVertex4dv"))) return -1;
	if(!(fnglVertex4f               = (void *)GetProcAddress(openglInst, "glVertex4f"))) return -1;
	if(!(fnglVertex4fv              = (void *)GetProcAddress(openglInst, "glVertex4fv"))) return -1;
	if(!(fnglVertex4i               = (void *)GetProcAddress(openglInst, "glVertex4i"))) return -1;
	if(!(fnglVertex4iv              = (void *)GetProcAddress(openglInst, "glVertex4iv"))) return -1;
	if(!(fnglVertex4s               = (void *)GetProcAddress(openglInst, "glVertex4s"))) return -1;
	if(!(fnglVertex4sv              = (void *)GetProcAddress(openglInst, "glVertex4sv"))) return -1;
	if(!(fnglVertexPointer          = (void *)GetProcAddress(openglInst, "glVertexPointer"))) return -1;
	if(!(fnglViewport               = (void *)GetProcAddress(openglInst, "glViewport"))) return -1;
#endif

  /* extension functions - check that the extension is supported before using these */
  //////////////// For GL_ARB_multitexture extension. //////////////////////
	fnglActiveTextureARB        = (void *)OpenGLGetProcAddress("glActiveTextureARB");
  fnglClientActiveTextureARB  = (void *)OpenGLGetProcAddress("glClientActiveTextureARB");
  fnglMultiTexCoord1dARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord1dARB");
  fnglMultiTexCoord1dvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord1dvARB");
  fnglMultiTexCoord1fARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord1fARB");
  fnglMultiTexCoord1fvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord1fvARB");
  fnglMultiTexCoord1iARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord1iARB");
  fnglMultiTexCoord1ivARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord1ivARB");
  fnglMultiTexCoord1sARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord1sARB");
  fnglMultiTexCoord1svARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord1svARB");
  fnglMultiTexCoord2dARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord2dARB");
  fnglMultiTexCoord2dvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord2dvARB");
  fnglMultiTexCoord2fARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord2fARB");
  fnglMultiTexCoord2fvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord2fvARB");
  fnglMultiTexCoord2iARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord2iARB");
  fnglMultiTexCoord2ivARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord2ivARB");
  fnglMultiTexCoord2sARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord2sARB");
  fnglMultiTexCoord2svARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord2svARB");
  fnglMultiTexCoord3dARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord3dARB");
  fnglMultiTexCoord3dvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord3dvARB");
  fnglMultiTexCoord3fARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord3fARB");
  fnglMultiTexCoord3fvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord3fvARB");
  fnglMultiTexCoord3iARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord3iARB");
  fnglMultiTexCoord3ivARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord3ivARB");
  fnglMultiTexCoord3sARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord3sARB");
  fnglMultiTexCoord3svARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord3svARB");
  fnglMultiTexCoord4dARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord4dARB");
  fnglMultiTexCoord4dvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord4dvARB");
  fnglMultiTexCoord4fARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord4fARB");
  fnglMultiTexCoord4fvARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord4fvARB");
  fnglMultiTexCoord4iARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord4iARB");
  fnglMultiTexCoord4ivARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord4ivARB");
  fnglMultiTexCoord4sARB      = (void *)OpenGLGetProcAddress("glMultiTexCoord4sARB");
  fnglMultiTexCoord4svARB     = (void *)OpenGLGetProcAddress("glMultiTexCoord4svARB");

  ///////////////// For GL_EXT_compiled_vertex_array extension. ////////////
	fnglLockArraysEXT           = (void *)OpenGLGetProcAddress("glLockArraysEXT");
	fnglUnlockArraysEXT         = (void *)OpenGLGetProcAddress("glUnlockArraysEXT");

  ///////////////// For WGL_EXT_swap_control extension. ////////////
  fnwglSwapIntervalEXT        = (void *)OpenGLGetProcAddress("wglSwapIntervalEXT");
  fnwglGetSwapIntervalEXT     = (void *)OpenGLGetProcAddress("wglGetSwapIntervalEXT");

  ///////////////// For GL_EXT_point_parameters extension. ////////////
  fnglPointParameterfEXT      = (void *)OpenGLGetProcAddress("glPointParameterfEXT");
  fnglPointParameterfvEXT     = (void *)OpenGLGetProcAddress("glPointParameterfvEXT");

  /* 
   * Anything other than an ICD/MCD full gl driver bypasses the GDI entirely
   */
  if (strcmp(lib, "opengl32.dll")) openglBypassGDI = TRUE;

  return 0; 
}



void OpenGLUnInit(void) {
  FreeLibrary(openglInst); 
  openglInst = NULL; 
} 

/* Determine if an extension is supported, must be called after a valid
 * context has been created */
GLboolean OpenGLCheckExtension(char *extName) {
	char *extensions = (char *)glGetString(GL_EXTENSIONS);
  if (strstr(extensions, extName)) return GL_TRUE;
  return GL_FALSE;
}
	
void APIENTRY glAccum(GLenum op, GLfloat value) {
	(*fnglAccum)(op,value);
}

void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {
	(*fnglAlphaFunc)(func,ref);
}

GLboolean APIENTRY glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences) {
	return (*fnglAreTexturesResident)(n,textures,residences);
}

void APIENTRY glArrayElement(GLint i) {
	(*fnglArrayElement)(i);
}

void APIENTRY glBegin(GLenum mode) {
	(*fnglBegin)(mode);
}

void APIENTRY glBindTexture(GLenum target, GLuint texture) {
	(*fnglBindTexture)(target,texture);
}

void APIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
	(*fnglBitmap)(width,height,xorig,yorig,xmove,ymove,bitmap);
}

void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) {
	(*fnglBlendFunc)(sfactor,dfactor);
}

void APIENTRY glCallList(GLuint list) {
	(*fnglCallList)(list);
}

void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
	(*fnglCallLists)(n,type,lists);
}

void APIENTRY glClear(GLbitfield mask) {
	(*fnglClear)(mask);
}

void APIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	(*fnglClearAccum)(red,green,blue,alpha);
}

void  APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	(*fnglClearColor)(red,green,blue,alpha);
}

void  APIENTRY glClearDepth(GLclampd depth) {
	(*fnglClearDepth)(depth);
}

void APIENTRY glClearIndex(GLfloat c) {
	(*fnglClearIndex)(c);
}

void APIENTRY glClearStencil(GLint s) {
	(*fnglClearStencil)(s);
}

void APIENTRY glClipPlane(GLenum plane, const GLdouble *equation) {
	(*fnglClipPlane)(plane,equation);
}

void APIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue) {
	(*fnglColor3b)(red,green,blue);
}

void APIENTRY glColor3bv(const GLbyte *v) {
	(*fnglColor3bv)(v);
}

void APIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
	(*fnglColor3d)(red,green,blue);
}

void APIENTRY glColor3dv(const GLdouble *v) {
	(*fnglColor3dv)(v);
}

void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	(*fnglColor3f)(red,green,blue);
}

void APIENTRY glColor3fv(const GLfloat *v) {
	(*fnglColor3fv)(v);
}

void APIENTRY glColor3i(GLint red, GLint green, GLint blue) {
	(*fnglColor3i)(red,green,blue);
}

void APIENTRY glColor3iv(const GLint *v) {
	(*fnglColor3iv)(v);
}

void APIENTRY glColor3s(GLshort red, GLshort green, GLshort blue) {
	(*fnglColor3s)(red,green,blue);
}

void APIENTRY glColor3sv(const GLshort *v) {
	(*fnglColor3sv)(v);
}

void APIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
	(*fnglColor3ub)(red,green,blue);
}

void APIENTRY glColor3ubv(const GLubyte *v) {
	(*fnglColor3ubv)(v);
}

void APIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue) {
	(*fnglColor3ui)(red,green,blue);
}

void APIENTRY glColor3uiv(const GLuint *v) {
	(*fnglColor3uiv)(v);
}

void APIENTRY glColor3us(GLushort red, GLushort green, GLushort blue) {
	(*fnglColor3us)(red,green,blue);
}

void APIENTRY glColor3usv(const GLushort *v) {
	(*fnglColor3usv)(v);
}

void APIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
	(*fnglColor4b)(red,green,blue,alpha);
}

void APIENTRY glColor4bv(const GLbyte *v) {
	(*fnglColor4bv)(v);
}

void APIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
	(*fnglColor4d)(red,green,blue,alpha);
}

void APIENTRY glColor4dv(const GLdouble *v) {
	(*fnglColor4dv)(v);
}

void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	(*fnglColor4f)(red,green,blue,alpha);
}

void APIENTRY glColor4fv(const GLfloat *v) {
	(*fnglColor4fv)(v);
}

void APIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha) {
	(*fnglColor4i)(red,green,blue,alpha);
}

void APIENTRY glColor4iv(const GLint *v) {
	(*fnglColor4iv)(v);
}

void APIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha) {
	(*fnglColor4s)(red,green,blue,alpha);
}

void APIENTRY glColor4sv(const GLshort *v) {
	(*fnglColor4sv)(v);
}

void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	(*fnglColor4ub)(red,green,blue,alpha);
}

void APIENTRY glColor4ubv(const GLubyte *v) {
	(*fnglColor4ubv)(v);
}

void APIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha) {
	(*fnglColor4ui)(red,green,blue,alpha);
}

void APIENTRY glColor4uiv(const GLuint *v) {
	(*fnglColor4uiv)(v);
}

void APIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) {
	(*fnglColor4us)(red,green,blue,alpha);
}

void APIENTRY glColor4usv(const GLushort *v) {
	(*fnglColor4usv)(v);
}

void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	(*fnglColorMask)(red,green,blue,alpha);
}

void APIENTRY glColorMaterial(GLenum face, GLenum mode) {
	(*fnglColorMaterial)(face,mode);
}

void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	(*fnglColorPointer)(size,type,stride,pointer);
}

void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
	(*fnglCopyPixels)(x,y,width,height,type);
}

void APIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
	(*fnglCopyTexImage1D)(target,level,internalformat,x,y,width,border);
}

void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	(*fnglCopyTexImage2D)(target,level,internalformat,x,y,width,height,border);
}

void APIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
	(*fnglCopyTexSubImage1D)(target,level,xoffset,x,y,width);
}

void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	(*fnglCopyTexSubImage2D)(target,level,xoffset,yoffset,x,y,width,height);
}

void APIENTRY glCullFace(GLenum mode) {
	(*fnglCullFace)(mode);
}

void APIENTRY glDeleteLists(GLuint list, GLsizei range) {
	(*fnglDeleteLists)(list,range);
}

void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures) {
	(*fnglDeleteTextures)(n,textures);
}

void APIENTRY glDepthFunc(GLenum func) {
	(*fnglDepthFunc)(func);
}

void APIENTRY glDepthMask(GLboolean flag) {
	(*fnglDepthMask)(flag);
}

void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar) {
	(*fnglDepthRange)(zNear,zFar);
}

void APIENTRY glDisable(GLenum cap) {
	(*fnglDisable)(cap);
}

void APIENTRY glDisableClientState(GLenum array) {
	(*fnglDisableClientState)(array);
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	(*fnglDrawArrays)(mode,first,count);
}

void APIENTRY glDrawBuffer(GLenum mode) {
	(*fnglDrawBuffer)(mode);
}

void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
	(*fnglDrawElements)(mode,count,type,indices);
}

void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglDrawPixels)(width,height,format,type,pixels);
}

void APIENTRY glEdgeFlag(GLboolean flag) {
	(*fnglEdgeFlag)(flag);
}

void APIENTRY glEdgeFlagPointer(GLsizei stride, const GLboolean *pointer) {
	(*fnglEdgeFlagPointer)(stride,pointer);
}

void APIENTRY glEdgeFlagv(const GLboolean *flag) {
	(*fnglEdgeFlagv)(flag);
}

void APIENTRY glEnable(GLenum cap) {
	(*fnglEnable)(cap);
}

void APIENTRY glEnableClientState(GLenum array) {
	(*fnglEnableClientState)(array);
}

void APIENTRY glEnd(void) {
	(*fnglEnd)();
}

void APIENTRY glEndList(void) {
	(*fnglEndList)();
}

void APIENTRY glEvalCoord1d(GLdouble u) {
	(*fnglEvalCoord1d)(u);
}

void APIENTRY glEvalCoord1dv(const GLdouble *u) {
	(*fnglEvalCoord1dv)(u);
}

void APIENTRY glEvalCoord1f(GLfloat u) {
	(*fnglEvalCoord1f)(u);
}

void APIENTRY glEvalCoord1fv(const GLfloat *u) {
	(*fnglEvalCoord1fv)(u);
}

void APIENTRY glEvalCoord2d(GLdouble u, GLdouble v) {
	(*fnglEvalCoord2d)(u,v);
}

void APIENTRY glEvalCoord2dv(const GLdouble *u) {
	(*fnglEvalCoord2dv)(u);
}

void APIENTRY glEvalCoord2f(GLfloat u, GLfloat v) {
	(*fnglEvalCoord2f)(u,v);
}

void APIENTRY glEvalCoord2fv(const GLfloat *u) {
	(*fnglEvalCoord2fv)(u);
}

void APIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2) {
	(*fnglEvalMesh1)(mode,i1,i2);
}

void APIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
	(*fnglEvalMesh2)(mode,i1,i2,j1,j2);
}

void APIENTRY glEvalPoint1(GLint i) {
	(*fnglEvalPoint1)(i);
}

void APIENTRY glEvalPoint2(GLint i, GLint j) {
	(*fnglEvalPoint2)(i,j);
}

void APIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer) {
	(*fnglFeedbackBuffer)(size,type,buffer);
}

void APIENTRY glFinish(void) {
	(*fnglFinish)();
}

void APIENTRY glFlush(void) {
	(*fnglFlush)();
}

void APIENTRY glFogf(GLenum pname, GLfloat param) {
	(*fnglFogf)(pname,param);
}

void APIENTRY glFogfv(GLenum pname, const GLfloat *params) {
	(*fnglFogfv)(pname,params);
}

void APIENTRY glFogi(GLenum pname, GLint param) {
	(*fnglFogi)(pname,param);
}

void APIENTRY glFogiv(GLenum pname, const GLint *params) {
	(*fnglFogiv)(pname,params);
}

void APIENTRY glFrontFace(GLenum mode) {
	(*fnglFrontFace)(mode);
}

void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglFrustum)(left,right,bottom,top,zNear,zFar);
}

GLuint APIENTRY glGenLists(GLsizei range) {
	return (*fnglGenLists)(range);
}

void APIENTRY glGenTextures(GLsizei n, GLuint *textures) {
	(*fnglGenTextures)(n,textures);
}

void APIENTRY glGetBooleanv(GLenum pname, GLboolean *params) {
	(*fnglGetBooleanv)(pname,params);
}

void APIENTRY glGetClipPlane(GLenum plane, GLdouble *equation) {
	(*fnglGetClipPlane)(plane,equation);
}

void APIENTRY glGetDoublev(GLenum pname, GLdouble *params) {
	(*fnglGetDoublev)(pname,params);
}

GLenum APIENTRY glGetError(void) {
	return (*fnglGetError)();
}

void APIENTRY glGetFloatv(GLenum pname, GLfloat *params) {
	(*fnglGetFloatv)(pname, params);
}

void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {
	(*fnglGetIntegerv)(pname,params);
}

void APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
	(*fnglGetLightfv)(light,pname,params);
}

void APIENTRY glGetLightiv(GLenum light, GLenum pname, GLint *params) {
	(*fnglGetLightiv)(light,pname,params);
}

void APIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble *v) {
	(*fnglGetMapdv)(target,query,v);
}

void APIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat *v) {
	(*fnglGetMapfv)(target,query,v);
}

void APIENTRY glGetMapiv(GLenum target, GLenum query, GLint *v) {
	(*fnglGetMapiv)(target,query,v);
}

void APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
	(*fnglGetMaterialfv)(face,pname,params);
}

void APIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint *params) {
	(*fnglGetMaterialiv)(face,pname,params);
}

void APIENTRY glGetPixelMapfv(GLenum map, GLfloat *values) {
	(*fnglGetPixelMapfv)(map,values);
}

void APIENTRY glGetPixelMapuiv(GLenum map, GLuint *values) {
	(*fnglGetPixelMapuiv)(map,values);
}

void APIENTRY glGetPixelMapusv(GLenum map, GLushort *values) {
	(*fnglGetPixelMapusv)(map,values);
}

void APIENTRY glGetPointerv(GLenum pname, GLvoid* *params) {
	(*fnglGetPointerv)(pname,params);
}

void APIENTRY glGetPolygonStipple(GLubyte *mask) {
	(*fnglGetPolygonStipple)(mask);
}

const GLubyte* APIENTRY glGetString(GLenum name) {
	return (*fnglGetString)(name);
}

void APIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params) {
	(*fnglGetTexEnvfv)(target,pname,params);
}

void APIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params) {
	(*fnglGetTexEnviv)(target,pname,params);
}

void APIENTRY glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params) {
	(*fnglGetTexGendv)(coord,pname,params);
}

void APIENTRY glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params) {
	(*fnglGetTexGenfv)(coord,pname,params);
}

void APIENTRY glGetTexGeniv(GLenum coord, GLenum pname, GLint *params) {
	(*fnglGetTexGeniv)(coord,pname,params);
}

void APIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) {
	(*fnglGetTexImage)(target,level,format,type,pixels);
}

void APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
	(*fnglGetTexLevelParameterfv)(target,level,pname,params);
}

void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
	(*fnglGetTexLevelParameteriv)(target,level,pname,params);
}

void APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
	(*fnglGetTexParameterfv)(target,pname,params);
}

void APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
	(*fnglGetTexParameteriv)(target,pname,params);
}

void APIENTRY glHint(GLenum target, GLenum mode) {
	(*fnglHint)(target,mode);
}

void APIENTRY glIndexMask(GLuint mask) {
	(*fnglIndexMask)(mask);
}

void APIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
	(*fnglIndexPointer)(type,stride,pointer);
}

void APIENTRY glIndexd(GLdouble c) {
	(*fnglIndexd)(c);
}

void APIENTRY glIndexdv(const GLdouble *c) {
	(*fnglIndexdv)(c);
}

void APIENTRY glIndexf(GLfloat c) {
	(*fnglIndexf)(c);
}

void APIENTRY glIndexfv(const GLfloat *c) {
	(*fnglIndexfv)(c);
}

void APIENTRY glIndexi(GLint c) {
	(*fnglIndexi)(c);
}

void APIENTRY glIndexiv(const GLint *c) {
	(*fnglIndexiv)(c);
}

void APIENTRY glIndexs(GLshort c) {
	(*fnglIndexs)(c);
}

void APIENTRY glIndexsv(const GLshort *c) {
	(*fnglIndexsv)(c);
}

void APIENTRY glIndexub(GLubyte c) {
	(*fnglIndexub)(c);
}

void APIENTRY glIndexubv(const GLubyte *c) {
	(*fnglIndexubv)(c);
}

void APIENTRY glInitNames(void) {
	(*fnglInitNames)();
}

void APIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer) {
	(*fnglInterleavedArrays)(format,stride,pointer);
}

GLboolean APIENTRY glIsEnabled(GLenum cap) {
	return (*fnglIsEnabled)(cap);
}

GLboolean APIENTRY glIsList(GLuint list) {
	return (*fnglIsList)(list);
}

GLboolean APIENTRY glIsTexture(GLuint texture) {
	return (*fnglIsTexture)(texture);
}

void APIENTRY glLightModelf(GLenum pname, GLfloat param) {
	(*fnglLightModelf)(pname,param);
}

void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params) {
	(*fnglLightModelfv)(pname,params);
}

void APIENTRY glLightModeli(GLenum pname, GLint param) {
	(*fnglLightModeli)(pname,param);
}

void APIENTRY glLightModeliv(GLenum pname, const GLint *params) {
	(*fnglLightModeliv)(pname,params);
}

void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param) {
	(*fnglLightf)(light,pname,param);
}

void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
	(*fnglLightfv)(light,pname,params);
}

void APIENTRY glLighti(GLenum light, GLenum pname, GLint param) {
	(*fnglLighti)(light,pname,param);
}

void APIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params) {
	(*fnglLightiv)(light,pname,params);
}

void APIENTRY glLineStipple(GLint factor, GLushort pattern) {
	(*fnglLineStipple)(factor,pattern);
}

void APIENTRY glLineWidth(GLfloat width) {
	(*fnglLineWidth)(width);
}

void APIENTRY glListBase(GLuint base) {
	(*fnglListBase)(base);
}

void APIENTRY glLoadIdentity(void) {
	(*fnglLoadIdentity)();
}

void APIENTRY glLoadMatrixd(const GLdouble *m) {
	(*fnglLoadMatrixd)(m);
}

void APIENTRY glLoadMatrixf(const GLfloat *m) {
	(*fnglLoadMatrixf)(m);
}

void APIENTRY glLoadName(GLuint name) {
	(*fnglLoadName)(name);
}

void APIENTRY glLogicOp(GLenum opcode) {
	(*fnglLogicOp)(opcode);
}

void APIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
	(*fnglMap1d)(target,u1,u2,stride,order,points);
}

void APIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
	(*fnglMap1f)(target,u1,u2,stride,order,points);
}

void APIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
	(*fnglMap2d)(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points);
}

void APIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
	(*fnglMap2f)(target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points);
}

void APIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) {
	(*fnglMapGrid1d)(un,u1,u2);
}

void APIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {
	(*fnglMapGrid1f)(un,u1,u2);
}

void APIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
	(*fnglMapGrid2d)(un,u1,u2,vn,v1,v2);
}

void APIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
	(*fnglMapGrid2f)(un,u1,u2,vn,v1,v2);
}

void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param) {
	
	(*fnglMaterialf)(face,pname,param);
}

void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
	(*fnglMaterialfv)(face,pname,params);
}

void APIENTRY glMateriali(GLenum face, GLenum pname, GLint param) {
	(*fnglMateriali)(face,pname,param);
}

void APIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params) {
	(*fnglMaterialiv)(face,pname,params);
}

void APIENTRY glMatrixMode(GLenum mode) {
	(*fnglMatrixMode)(mode);
}

void APIENTRY glMultMatrixd(const GLdouble *m) {
	(*fnglMultMatrixd)(m);
}

void APIENTRY glMultMatrixf(const GLfloat *m) {
	(*fnglMultMatrixf)(m);
}

void APIENTRY glNewList(GLuint list, GLenum mode) {
	(*fnglNewList)(list,mode);
}

void APIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) {
	(*fnglNormal3b)(nx,ny,nz);
}

void APIENTRY glNormal3bv(const GLbyte *v) {
	(*fnglNormal3bv)(v);
}

void APIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
	(*fnglNormal3d)(nx,ny,nz);
}

void APIENTRY glNormal3dv(const GLdouble *v) {
	(*fnglNormal3dv)(v);
}

void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
	(*fnglNormal3f)(nx,ny,nz);
}

void APIENTRY glNormal3fv(const GLfloat *v) {
	(*fnglNormal3fv)(v);
}

void APIENTRY glNormal3i(GLint nx, GLint ny, GLint nz) {
	(*fnglNormal3i)(nx,ny,nz);
}

void APIENTRY glNormal3iv(const GLint *v) {
	(*fnglNormal3iv)(v);
}

void APIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz) {
	(*fnglNormal3s)(nx,ny,nz);
}

void APIENTRY glNormal3sv(const GLshort *v) {
	(*fnglNormal3sv)(v);
}

void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
	(*fnglNormalPointer)(type,stride,pointer);
}

void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglOrtho)(left,right,bottom,top,zNear,zFar);
}

void APIENTRY glPassThrough(GLfloat token) {
	(*fnglPassThrough)(token);
}

void APIENTRY glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values) {
	(*fnglPixelMapfv)(map,mapsize,values);
}

void APIENTRY glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values) {
	(*fnglPixelMapuiv)(map,mapsize,values);
}

void APIENTRY glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values) {
	(*fnglPixelMapusv)(map,mapsize,values);
}

void APIENTRY glPixelStoref(GLenum pname, GLfloat param) {
	(*fnglPixelStoref)(pname,param);
}

void APIENTRY glPixelStorei(GLenum pname, GLint param) {
	(*fnglPixelStorei)(pname, param);
}

void APIENTRY glPixelTransferf(GLenum pname, GLfloat param) {
	(*fnglPixelTransferf)(pname,param);
}

void APIENTRY glPixelTransferi(GLenum pname, GLint param) {
	(*fnglPixelTransferi)(pname,param);
}

void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor) {
	(*fnglPixelZoom)(xfactor,yfactor);
}

void APIENTRY glPointSize(GLfloat size) {
	(*fnglPointSize)(size);
}

void APIENTRY glPolygonMode(GLenum face, GLenum mode) {
	(*fnglPolygonMode)(face,mode);
}

void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {
	(*fnglPolygonOffset)(factor,units);
}

void APIENTRY glPolygonStipple(const GLubyte *mask) {
	(*fnglPolygonStipple)(mask);
}

void APIENTRY glPopAttrib(void) {
	(*fnglPopAttrib)();
}

void APIENTRY glPopClientAttrib(void) {
	(*fnglPopClientAttrib)();
}

void APIENTRY glPopMatrix(void) {
	(*fnglPopMatrix)();
}

void APIENTRY glPopName(void) {
	(*fnglPopName)();
}

void APIENTRY glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities) {
	(*fnglPrioritizeTextures)(n,textures,priorities);
}

void APIENTRY glPushAttrib(GLbitfield mask) {
	(*fnglPushAttrib)(mask);
}

void APIENTRY glPushClientAttrib(GLbitfield mask) {
	(*fnglPushClientAttrib)(mask);
}

void APIENTRY glPushMatrix(void) {
	(*fnglPushMatrix)();
}

void APIENTRY glPushName(GLuint name) {
	(*fnglPushName)(name);
}

void APIENTRY glRasterPos2d(GLdouble x, GLdouble y) {
	(*fnglRasterPos2d)(x,y);
}

void APIENTRY glRasterPos2dv(const GLdouble *v) {
	(*fnglRasterPos2dv)(v);
}

void APIENTRY glRasterPos2f(GLfloat x, GLfloat y) {
	(*fnglRasterPos2f)(x,y);
}

void APIENTRY glRasterPos2fv(const GLfloat *v) {
	(*fnglRasterPos2fv)(v);
}

void APIENTRY glRasterPos2i(GLint x, GLint y) {
	(*fnglRasterPos2i)(x,y);
}

void APIENTRY glRasterPos2iv(const GLint *v) {
	(*fnglRasterPos2iv)(v);
}

void APIENTRY glRasterPos2s(GLshort x, GLshort y) {
	(*fnglRasterPos2s)(x,y);
}

void APIENTRY glRasterPos2sv(const GLshort *v) {
	(*fnglRasterPos2sv)(v);
}

void APIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) {
	(*fnglRasterPos3d)(x,y,z);
}

void APIENTRY glRasterPos3dv(const GLdouble *v) {
	(*fnglRasterPos3dv)(v);
}

void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglRasterPos3f)(x,y,z);
}

void APIENTRY glRasterPos3fv(const GLfloat *v) {
	(*fnglRasterPos3fv)(v);
}

void APIENTRY glRasterPos3i(GLint x, GLint y, GLint z) {
	(*fnglRasterPos3i)(x,y,z);
}

void APIENTRY glRasterPos3iv(const GLint *v) {
	(*fnglRasterPos3iv)(v);
}

void APIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z) {
	(*fnglRasterPos3s)(x,y,z);
}

void APIENTRY glRasterPos3sv(const GLshort *v) {
	(*fnglRasterPos3sv)(v);
}

void APIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	(*fnglRasterPos4d)(x,y,z,w);
}

void APIENTRY glRasterPos4dv(const GLdouble *v) {
	(*fnglRasterPos4dv)(v);
}

void APIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	(*fnglRasterPos4f)(x,y,z,w);
}

void APIENTRY glRasterPos4fv(const GLfloat *v) {
	(*fnglRasterPos4fv)(v);
}

void APIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w) {
	(*fnglRasterPos4i)(x,y,z,w);
}

void APIENTRY glRasterPos4iv(const GLint *v) {
	(*fnglRasterPos4iv)(v);
}

void APIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) {
	(*fnglRasterPos4s)(x,y,z,w);
}

void APIENTRY glRasterPos4sv(const GLshort *v) {
	(*fnglRasterPos4sv)(v);
}

void APIENTRY glReadBuffer(GLenum mode) {
	(*fnglReadBuffer)(mode);
}

void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
	(*fnglReadPixels)(x,y,width,height,format,type,pixels);
}

void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
	(*fnglRectd)(x1,y1,x2,y2);
}

void APIENTRY glRectdv(const GLdouble *v1, const GLdouble *v2) {
	(*fnglRectdv)(v1,v2);
}

void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	(*fnglRectf)(x1,y1,x2,y2);
}

void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2) {
	(*fnglRectfv)(v1,v2);
}

void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
	(*fnglRecti)(x1,y1,x2,y2);
}

void APIENTRY glRectiv(const GLint *v1, const GLint *v2) {
	(*fnglRectiv)(v1,v2);
}

void APIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
	(*fnglRects)(x1,y1,x2,y2);
}

void APIENTRY glRectsv(const GLshort *v1, const GLshort *v2) {
	
	(*fnglRectsv)(v1,v2);
}

GLint APIENTRY glRenderMode(GLenum mode) {
	return (*fnglRenderMode)(mode);
}

void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
	(*fnglRotated)(angle,x,y,z);
}

void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	(*fnglRotatef)(angle,x,y,z);
}

void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z) {
	(*fnglScaled)(x,y,z);
}

void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglScalef)(x,y,z);
}

void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	(*fnglScissor)(x,y,width,height);
}

void APIENTRY glSelectBuffer(GLsizei size, GLuint *buffer) {
	(*fnglSelectBuffer)(size,buffer);
}

void  APIENTRY glShadeModel (GLenum mode) {
	(*fnglShadeModel)(mode);
}

void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask) {
	(*fnglStencilFunc)(func,ref,mask);
}

void APIENTRY glStencilMask(GLuint mask) {
	(*fnglStencilMask)(mask);
}

void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
	(*fnglStencilOp)(fail,zfail,zpass);
}

void APIENTRY glTexCoord1d(GLdouble s) {
	(*fnglTexCoord1d)(s);
}

void APIENTRY glTexCoord1dv(const GLdouble *v) {
	(*fnglTexCoord1dv)(v);
}

void APIENTRY glTexCoord1f(GLfloat s) {
	(*fnglTexCoord1f)(s);
}

void APIENTRY glTexCoord1fv(const GLfloat *v) {
	(*fnglTexCoord1fv)(v);
}

void APIENTRY glTexCoord1i(GLint s) {
	(*fnglTexCoord1i)(s);
}

void APIENTRY glTexCoord1iv(const GLint *v) {
	(*fnglTexCoord1iv)(v);
}

void APIENTRY glTexCoord1s(GLshort s) {
	(*fnglTexCoord1s)(s);
}

void APIENTRY glTexCoord1sv(const GLshort *v) {
	(*fnglTexCoord1sv)(v);
}

void APIENTRY glTexCoord2d(GLdouble s, GLdouble t) {
	(*fnglTexCoord2d)(s,t);
}

void APIENTRY glTexCoord2dv(const GLdouble *v) {
	(*fnglTexCoord2dv)(v);
}

void APIENTRY glTexCoord2f(GLfloat s, GLfloat t) {
	(*fnglTexCoord2f)(s,t);
}

void APIENTRY glTexCoord2fv(const GLfloat *v) {
	(*fnglTexCoord2fv)(v);
}

void APIENTRY glTexCoord2i(GLint s, GLint t) {
	(*fnglTexCoord2i)(s,t);
}

void APIENTRY glTexCoord2iv(const GLint *v) {
	
	(*fnglTexCoord2iv)(v);
}

void APIENTRY glTexCoord2s(GLshort s, GLshort t) {
	(*fnglTexCoord2s)(s,t);
}

void APIENTRY glTexCoord2sv(const GLshort *v) {
	(*fnglTexCoord2sv)(v);
}

void APIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) {
	(*fnglTexCoord3d)(s,t,r);
}

void APIENTRY glTexCoord3dv(const GLdouble *v) {
	(*fnglTexCoord3dv)(v);
}

void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
	(*fnglTexCoord3f)(s,t,r);
}

void APIENTRY glTexCoord3fv(const GLfloat *v) {
	(*fnglTexCoord3fv)(v);
}

void APIENTRY glTexCoord3i(GLint s, GLint t, GLint r) {
	(*fnglTexCoord3i)(s,t,r);
}

void APIENTRY glTexCoord3iv(const GLint *v) {
	(*fnglTexCoord3iv)(v);
}

void APIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r) {
	(*fnglTexCoord3s)(s,t,r);
}

void APIENTRY glTexCoord3sv(const GLshort *v) {
	(*fnglTexCoord3sv)(v);
}

void APIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
	(*fnglTexCoord4d)(s,t,r,q);
}

void APIENTRY glTexCoord4dv(const GLdouble *v) {
	(*fnglTexCoord4dv)(v);
}

void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
	(*fnglTexCoord4f)(s,t,r,q);
}

void APIENTRY glTexCoord4fv(const GLfloat *v) {
	(*fnglTexCoord4fv)(v);
}

void APIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q) {
	(*fnglTexCoord4i)(s,t,r,q);
}

void APIENTRY glTexCoord4iv(const GLint *v) {
	(*fnglTexCoord4iv)(v);
}

void APIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) {
	(*fnglTexCoord4s)(s,t,r,q);
}

void APIENTRY glTexCoord4sv(const GLshort *v) {
	(*fnglTexCoord4sv)(v);
}

void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	(*fnglTexCoordPointer)(size,type,stride,pointer);
}

void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexEnvf)(target,pname,param);
}

void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
	(*fnglTexEnvfv)(target,pname,params);
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) {
	(*fnglTexEnvi)(target,pname,param);
}

void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
	(*fnglTexEnviv)(target,pname,params);
}

void APIENTRY glTexGend(GLenum target, GLenum pname, GLdouble param) {
	(*fnglTexGend)(target,pname,param);
}

void APIENTRY glTexGendv(GLenum target, GLenum pname, const GLdouble *params) {
	(*fnglTexGendv)(target,pname,params);
}

void APIENTRY glTexGenf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexGenf)(target,pname,param);
}

void APIENTRY glTexGenfv(GLenum target, GLenum pname, const GLfloat *params) {
	(*fnglTexGenfv)(target,pname,params);
}

void APIENTRY glTexGeni(GLenum target, GLenum pname, GLint param) {
	(*fnglTexGeni)(target,pname,param);
}

void APIENTRY glTexGeniv(GLenum target, GLenum pname, const GLint *params) {
	(*fnglTexGeniv)(target,pname,params);
}

void APIENTRY glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexImage1D)(target,level,components,width,border,format,type,pixels);
}

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexImage2D)(target,level,internalformat,width,height,border,format,type,pixels);
}

void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexParameterf)(target,pname,param);
}

void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
	(*fnglTexParameterfv)(target,pname,params);
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {
	(*fnglTexParameteri)(target,pname,param);
}

void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
	(*fnglTexParameteriv)(target,pname,params);
}

void APIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexSubImage1D)(target,level,xoffset,width,format,type,pixels);
}

void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexSubImage2D)(target,level,xoffset,yoffset,width,height,format,type,pixels);
}

void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z) {
	(*fnglTranslated)(x,y,z);
}

void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglTranslatef)(x,y,z);
}

void APIENTRY glVertex2d(GLdouble x, GLdouble y) {
	(*fnglVertex2d)(x,y);
}

void APIENTRY glVertex2dv(const GLdouble *v) {
	(*fnglVertex2dv)(v);
}

void APIENTRY glVertex2f(GLfloat x, GLfloat y) {
	(*fnglVertex2f)(x,y);
}

void APIENTRY glVertex2fv(const GLfloat *v) {
	(*fnglVertex2fv)(v);
}

void APIENTRY glVertex2i(GLint x, GLint y) {
	(*fnglVertex2i)(x,y);
}

void APIENTRY glVertex2iv(const GLint *v) {
	(*fnglVertex2iv)(v);
}

void APIENTRY glVertex2s(GLshort x, GLshort y) {
	(*fnglVertex2s)(x,y);
}

void APIENTRY glVertex2sv(const GLshort *v) {
	(*fnglVertex2sv)(v);
}

void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
	(*fnglVertex3d)(x,y,z);
}

void APIENTRY glVertex3dv(const GLdouble *v) {
	(*fnglVertex3dv)(v);
}

void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglVertex3f)(x,y,z);
}

void APIENTRY glVertex3fv(const GLfloat *v) {
	(*fnglVertex3fv)(v);
}

void APIENTRY glVertex3i(GLint x, GLint y, GLint z) {
	(*fnglVertex3i)(x,y,z);
}

void APIENTRY glVertex3iv(const GLint *v) {
	(*fnglVertex3iv)(v);
}

void APIENTRY glVertex3s(GLshort x, GLshort y, GLshort z) {
	(*fnglVertex3s)(x,y,z);
}

void APIENTRY glVertex3sv(const GLshort *v) {
	(*fnglVertex3sv)(v);
}

void APIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	(*fnglVertex4d)(x,y,z,w);
}

void APIENTRY glVertex4dv(const GLdouble *v) {
	(*fnglVertex4dv)(v);
}

void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	(*fnglVertex4f)(x,y,z,w);
}

void APIENTRY glVertex4fv(const GLfloat *v) {
	(*fnglVertex4fv)(v);
}

void APIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w) {
	(*fnglVertex4i)(x,y,z,w);
}

void APIENTRY glVertex4iv(const GLint *v) {
	(*fnglVertex4iv)(v);
}

void APIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) {
	(*fnglVertex4s)(x,y,z,w);
}

void APIENTRY glVertex4sv(const GLshort *v) {
	(*fnglVertex4sv)(v);
}

void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	(*fnglVertexPointer)(size,type,stride,pointer);
}

void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	(*fnglViewport)(x,y,width,height);
}

///////////////////// For GL_ARB_multitexture extension. /////////////////

void glActiveTextureARB(GLenum target) {
	(*fnglActiveTextureARB)(target);
}

void glClientActiveTextureARB(GLenum target) {
	(*fnglClientActiveTextureARB)(target);
}

void glMultiTexCoord1dARB(GLenum target, GLdouble s) {
	(*fnglMultiTexCoord1dARB)(target,s);
}

void glMultiTexCoord1dvARB(GLenum target, const GLdouble *v) {
	(*fnglMultiTexCoord1dvARB)(target,v);
}

void glMultiTexCoord1fARB(GLenum target, GLfloat s) {
	(*fnglMultiTexCoord1fARB)(target,s);
}

void glMultiTexCoord1fvARB(GLenum target, const GLfloat *v) {
	(*fnglMultiTexCoord1fvARB)(target,v);
}

void glMultiTexCoord1iARB(GLenum target, GLint s) {
	(*fnglMultiTexCoord1iARB)(target,s);
}

void glMultiTexCoord1ivARB(GLenum target, const GLint *v) {
	(*fnglMultiTexCoord1ivARB)(target,v);
}

void glMultiTexCoord1sARB(GLenum target, GLshort s) {
	(*fnglMultiTexCoord1sARB)(target,s);
}

void glMultiTexCoord1svARB(GLenum target, const GLshort *v) {
	(*fnglMultiTexCoord1svARB)(target,v);
}

void glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t) {
	(*fnglMultiTexCoord2dARB)(target,s,t);
}

void glMultiTexCoord2dvARB(GLenum target, const GLdouble *v) {
	(*fnglMultiTexCoord2dvARB)(target,v);
}

void glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
	(*fnglMultiTexCoord2fARB)(target,s,t);
}

void glMultiTexCoord2fvARB(GLenum target, const GLfloat *v) {
	(*fnglMultiTexCoord2fvARB)(target,v);
}

void glMultiTexCoord2iARB(GLenum target, GLint s, GLint t) {
	(*fnglMultiTexCoord2iARB)(target,s,t);
}

void glMultiTexCoord2ivARB(GLenum target, const GLint *v) {
	(*fnglMultiTexCoord2ivARB)(target,v);
}

void glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t) {
	(*fnglMultiTexCoord2fARB)(target,s,t);
}

void glMultiTexCoord2svARB(GLenum target, const GLshort *v) {
	(*fnglMultiTexCoord2svARB)(target,v);
}

void glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r) {
	(*fnglMultiTexCoord3dARB)(target,s,t,r);
}

void glMultiTexCoord3dvARB(GLenum target, const GLdouble *v) {
	(*fnglMultiTexCoord3dvARB)(target,v);
}

void glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
	(*fnglMultiTexCoord3fARB)(target,s,t,r);
}

void glMultiTexCoord3fvARB(GLenum target, const GLfloat *v) {
	(*fnglMultiTexCoord3fvARB)(target,v);
}

void glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r) {
	(*fnglMultiTexCoord3iARB)(target,s,t,r);
}

void glMultiTexCoord3ivARB(GLenum target, const GLint *v) {
	(*fnglMultiTexCoord3ivARB)(target,v);
}

void glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r) {
	(*fnglMultiTexCoord3sARB)(target,s,t,r);
}

void glMultiTexCoord3svARB(GLenum target, const GLshort *v) {
	(*fnglMultiTexCoord3svARB)(target,v);
}

void glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
	(*fnglMultiTexCoord4dARB)(target,s,t,r,q);
}

void glMultiTexCoord4dvARB(GLenum target, const GLdouble *v) {
	(*fnglMultiTexCoord4dvARB)(target,v);
}

void glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
	(*fnglMultiTexCoord4fARB)(target,s,t,r,q);
}

void glMultiTexCoord4fvARB(GLenum target, const GLfloat *v) {
	(*fnglMultiTexCoord4fvARB)(target,v);
}

void glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q) {
	(*fnglMultiTexCoord4iARB)(target,s,t,r,q);
}

void glMultiTexCoord4ivARB(GLenum target, const GLint *v) {
	(*fnglMultiTexCoord4ivARB)(target,v);
}

void glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
	(*fnglMultiTexCoord4sARB)(target,s,t,r,q);
}

void glMultiTexCoord4svARB(GLenum target, const GLshort *v) {
	(*fnglMultiTexCoord4svARB)(target,v);
}

///////////////// For GL_EXT_compiled_vertex_array extension. ////////////

void glLockArraysEXT(GLint first, GLsizei count) {
	(*fnglLockArraysEXT)(first,count);
}

void glUnlockArraysEXT(void) {
	(*fnglUnlockArraysEXT)();
}

///////////////// For WGL_EXT_swap_control extension. ////////////

GLboolean wglSwapIntervalEXT(GLint interval) {
  if(fnwglSwapIntervalEXT) {
    /* 0 = disable sync, 1 = wait for sync */
    return (*fnwglSwapIntervalEXT)(0);
  } else
    return FALSE;
}

GLint wglGetSwapIntervalEXT(void) {
  if(fnwglGetSwapIntervalEXT) {
    /* 0 = disable sync, 1 = wait for sync */
    return (*fnwglGetSwapIntervalEXT)();
  } else
    return -1;
}

///////////////// For GL_EXT_point_parameters extension. ////////////

void glPointParameterfEXT(GLenum pname, GLfloat param) {
  (*fnglPointParameterfEXT)(pname, param);
}

void glPointParameterfvEXT(GLenum pname, GLfloat *params) {
  (*fnglPointParameterfvEXT)(pname, params);
}

/******************************************************************************/
/* replaced WGL* functions */

/*
 * These cant? be fixed like the above functions becuase they are declared in gdi.h
 * and for some reason the compiler will insist on declaring them as EXPORTED
 * dll functions... weird
 */
HGLRC APIENTRY OpenGLCreateContext(HDC hdc) {
  HGLRC (APIENTRY *fnwglCreateContext)(HDC hdc);

  if (!openglInst) return NULL;
  fnwglCreateContext = (void*)GetProcAddress(openglInst, "wglCreateContext");
  if (!fnwglCreateContext) return NULL;
  return (*fnwglCreateContext)(hdc);
}


BOOL APIENTRY OpenGLDeleteContext(HGLRC hglrc) {
  BOOL (APIENTRY *fnwglDeleteContext)(HGLRC hglrc);

  if (!openglInst) return FALSE;
  fnwglDeleteContext = (void*)GetProcAddress(openglInst, "wglDeleteContext");
  if (!fnwglDeleteContext) return FALSE;
  return (*fnwglDeleteContext)(hglrc);
}


BOOL APIENTRY OpenGLMakeCurrent(HDC hdc, HGLRC hglrc) {
  BOOL (APIENTRY *fnwglMakeCurrent)(HDC hdc, HGLRC hglrc);

  if (!openglInst) return FALSE;
  fnwglMakeCurrent = (void*)GetProcAddress(openglInst, "wglMakeCurrent");
  if (!fnwglMakeCurrent) return FALSE;
  return (*fnwglMakeCurrent)(hdc, hglrc);
}


BOOL APIENTRY OpenGLSwapLayerBuffers(HDC hdc, UINT fuPlanes) {
  BOOL (APIENTRY *fnwglSwapLayerBuffers)(HDC hdc, UINT fuPlanes);

  if (!openglInst) return FALSE;
  fnwglSwapLayerBuffers = (void*)GetProcAddress(openglInst, "wglSwapLayerBuffers");
  if (!fnwglSwapLayerBuffers) return FALSE;
  return (*fnwglSwapLayerBuffers)(hdc, fuPlanes);
}

PROC APIENTRY OpenGLGetProcAddress(LPCSTR lpcstr) {
	PROC (APIENTRY *fnwglGetProcAddress)(LPCSTR lpcstr);

	if(!openglInst) return FALSE;
	fnwglGetProcAddress = (void *)GetProcAddress(openglInst, "wglGetProcAddress");
	if(!fnwglGetProcAddress) return FALSE;
	return (*fnwglGetProcAddress)(lpcstr);
}

/******************************************************************************/
/* replaced GDI functions */
/* 
 * When using MiniGL drivers you can't call the GDI functions (ie SwapBuffers etc)
 * because they will redirect the call to the system ogl dll lib, not the minigl dll
 * that you are using.
 */

BOOL APIENTRY OpenGLSwapBuffers(HDC hdc) {
  BOOL (APIENTRY *fnSwapBuffers)(HDC hdc);

  if (!openglBypassGDI) {
    // Try GDI version first
    int retValue;
    retValue = SwapBuffers(hdc);
    if (retValue) return retValue;
  }
  if (!openglInst) return FALSE;
  fnSwapBuffers = (void*)GetProcAddress(openglInst, "wglSwapBuffers");
  if (!fnSwapBuffers) return FALSE;
  return (*fnSwapBuffers)(hdc);
}

int   APIENTRY OpenGLChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd) {
  int (APIENTRY *fnChoosePixelFormat)(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd);

  if (!openglBypassGDI) {
    // Try GDI version first
    int retValue;
    retValue = ChoosePixelFormat(hdc, ppfd);
    if (retValue) return retValue;
  }
  // Bypass GDI entirely
  if (!openglInst) return FALSE;
  fnChoosePixelFormat = (void*)GetProcAddress(openglInst, "wglChoosePixelFormat");
  if (!fnChoosePixelFormat) return FALSE;
  openglBypassGDI = TRUE;
  return (*fnChoosePixelFormat)(hdc, ppfd); 
}

int   APIENTRY OpenGLDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd) {
  int (APIENTRY *fnDescribePixelFormat)(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);

  if (!openglBypassGDI) {
    // Try GDI version first
    int retValue;
    retValue = DescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
    if (retValue) return retValue;
  }
  // Bypass GDI entirely
  if (!openglInst) return FALSE;
  fnDescribePixelFormat = (void*)GetProcAddress(openglInst, "wglDescribePixelFormat");
  if (!fnDescribePixelFormat) return FALSE;
  openglBypassGDI = TRUE;
  return (*fnDescribePixelFormat)(hdc, iPixelFormat, nBytes, ppfd);
}

BOOL  APIENTRY OpenGLSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd) {
  BOOL (APIENTRY *fnSetPixelFormat)(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd);

  if (!openglBypassGDI) {
    // Try GDI version first
    int retValue;
    retValue = SetPixelFormat(hdc, iPixelFormat, ppfd);
    if (retValue) return retValue;
  }
  // Bypass GDI entirely
  if (!openglInst) return FALSE;
  fnSetPixelFormat = (void*)GetProcAddress(openglInst, "wglSetPixelFormat");
  if (!fnSetPixelFormat) return FALSE;
  openglBypassGDI = TRUE;
  return (*fnSetPixelFormat)(hdc, iPixelFormat, ppfd);
}

//-----------------------------------------------------------------------------
//
// $Log: OpenGL.c,v $
// Revision 1.2  2000/04/10 21:13:13  proff_fs
// added Log to OpenGL files
//
// Revision 1.1.1.1  2000/04/09 18:21:44  proff_fs
// Initial login
//
//-----------------------------------------------------------------------------
