/* Important note: Do not include any of the gl includes yourself!
 * include only this file, it will load gl.h etc properly so that
 * the compiler wont think it is calling a staticly bound dll 
 *
 * and make sure that you include windows.h before including this file
 */

#ifndef __PLASMA_OPENGL_H
#define __PLASMA_OPENGL_H

#include <windows.h>

/* do this so that it wont declare everything as dllimport */
#undef WINGDIAPI
#define WINGDIAPI

#ifdef __GL_H__
  #error Do not manually include gl.h or glu.h
#endif
#ifdef __gl_h__
  #error Do not manually include gl.h or glu.h
#endif

#include <GL/gl.h>
#include <GL/glu.h>

//////////////////////////////////////////////////////////////////////////
/* Extension functions Constants (ie EXT and ARB) */

// For GL_ARB_multitexture extension.
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURES_UNITS_ARB           0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3

// For GL_EXT_point_parameters extension.
#define GL_POINT_SIZE_MIN_EXT               0x8126
#define GL_POINT_SIZE_MAX_EXT               0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT    0x8128
#define GL_DISTANCE_ATTENUATION_EXT         0x8129

//////////////////////////////////////////////////////////////////////////
/* Extension functions (ie EXT and ARB) */

void      glLockArraysEXT(GLint first, GLsizei count);
void      glUnlockArraysEXT(void);
void      glActiveTextureARB(GLenum target);
void      glClientActiveTextureARB(GLenum target);
void      glMultiTexCoord1dARB(GLenum target, GLdouble s);
void      glMultiTexCoord1dvARB(GLenum target, const GLdouble *v);
void      glMultiTexCoord1fARB(GLenum target, GLfloat s);
void      glMultiTexCoord1fvARB(GLenum target, const GLfloat *v);
void      glMultiTexCoord1iARB(GLenum target, GLint s);
void      glMultiTexCoord1ivARB(GLenum target, const GLint *v);
void      glMultiTexCoord1sARB(GLenum target, GLshort s);
void      glMultiTexCoord1svARB(GLenum target, const GLshort *v);
void      glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t);
void      glMultiTexCoord2dvARB(GLenum target, const GLdouble *v);
void      glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t);
void      glMultiTexCoord2fvARB(GLenum target, const GLfloat *v);
void      glMultiTexCoord2iARB(GLenum target, GLint s, GLint t);
void      glMultiTexCoord2ivARB(GLenum target, const GLint *v);
void      glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t);
void      glMultiTexCoord2svARB(GLenum target, const GLshort *v);
void      glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r);
void      glMultiTexCoord3dvARB(GLenum target, const GLdouble *v);
void      glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r);
void      glMultiTexCoord3fvARB(GLenum target, const GLfloat *v);
void      glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r);
void      glMultiTexCoord3ivARB(GLenum target, const GLint *v);
void      glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r);
void      glMultiTexCoord3svARB(GLenum target, const GLshort *v);
void      glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void      glMultiTexCoord4dvARB(GLenum target, const GLdouble *v);
void      glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void      glMultiTexCoord4fvARB(GLenum target, const GLfloat *v);
void      glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
void      glMultiTexCoord4ivARB(GLenum target, const GLint *v);
void      glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
void      glMultiTexCoord4svARB(GLenum target, const GLshort *v);
GLboolean wglSwapIntervalEXT(GLint interval);
GLint     wglGetSwapIntervalEXT(void);
void      glPointParameterfEXT(GLenum pname, GLfloat param);
void      glPointParameterfvEXT(GLenum pname, GLfloat *params);

//////////////////////////////////////////////////////////////////////////

/* restore normal meaning of WINGDIAPI */
#undef WINGDIAPI
#define WINGDIAPI DECLSPEC_IMPORT

/* Bypasses the GDI SwapBuffers call, calls the minidriver directly instead */
BOOL openglBypassGDI;

void      OpenGLFindDriver(char *openglLib);
int       OpenGLInit(char *lib);
void      OpenGLUnInit(void);
GLboolean OpenGLCheckExtension(char *extName);

/* Use these instead of the wgl* functions */
HGLRC APIENTRY OpenGLCreateContext(HDC hdc);
BOOL  APIENTRY OpenGLDeleteContext(HGLRC hglrc);
BOOL  APIENTRY OpenGLMakeCurrent(HDC hdc, HGLRC hglrc);
BOOL  APIENTRY OpenGLSwapLayerBuffers(HDC hdc, UINT fuPlanes);
PROC  APIENTRY OpenGLGetProcAddress(LPCSTR lpcstr);

/* use these functions instead of the GDI ones */
BOOL  APIENTRY OpenGLSwapBuffers(HDC hdc);
int   APIENTRY OpenGLChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd);
int   APIENTRY OpenGLDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
BOOL  APIENTRY OpenGLSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd);

#endif

//-----------------------------------------------------------------------------
//
// $Log: OPENGL.H,v $
// Revision 1.2  2000/04/10 21:13:13  proff_fs
// added Log to OpenGL files
//
// Revision 1.1.1.1  2000/04/09 18:21:44  proff_fs
// Initial login
//
//-----------------------------------------------------------------------------
