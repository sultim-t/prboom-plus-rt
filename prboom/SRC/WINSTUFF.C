// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: WINSTUFF.C,v 1.2 2000/04/26 20:00:03 proff_fs Exp $
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

#define WIN32_LEAN_AND_MEAN

#include "z_zone.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include "SDL.h"
#include "doomtype.h"
#include "doomdef.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "v_video.h"
#include "i_system.h"
#include "lprintf.h"

extern char title[128];
char szTitle[256];
char szGLTitle[256];
char szConTitle[256];

HWND ghWnd;

H_boolean page_flip=true;
int use_vsync = 1;

// Variables for the console
HWND con_hWnd=0;
HFONT OemFont;
LONG OemWidth, OemHeight;
int ConWidth,ConHeight;
char szConName[] = "PrBoomConWinClass";
char Lines[(80+2)*25+1];
char *Last = NULL;

SDL_Surface *sdl_screen;
H_boolean fActive = false;
// proff: Removed fFullscreen
int vidFullScreen = 0;

extern int usemouse;
H_boolean noMouse = false;
H_boolean grabMouse = false;
extern int usejoystick;

static CALLBACK ConWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT paint;
  HDC dc;

  switch (iMsg) {
  case WM_CLOSE:
    return 1;
    break;
  case WM_PAINT:
	  if (dc = BeginPaint (con_hWnd, &paint))
    {
		  if (Last)
      {
			  char *row;
			  int line, last;

			  line = paint.rcPaint.top / OemHeight;
			  last = paint.rcPaint.bottom / OemHeight;
			  for (row = Lines + (line*(80+2)); line < last; line++)
        {
				  if (row[1]>0)
				    TextOut (dc, 0, line * OemHeight, &row[2], row[1]);
				  row += 80 + 2;
			  }
		  }
		  EndPaint (con_hWnd, &paint);
	  }
    return 0;
    break;
  default:
    return(DefWindowProc(hwnd,iMsg,wParam,lParam));
  }
}

static void I_PrintStr (int xp, const char *cp, int count, BOOL scroll) {
	RECT rect;
  HDC conDC;

	if (count)
  {
    conDC=GetDC(con_hWnd);
		TextOut (conDC, xp * OemWidth, ConHeight - OemHeight, cp, count);
    ReleaseDC(con_hWnd,conDC);
  }
	if (scroll) {
		rect.left = 0;
		rect.top = 0;
		rect.right = ConWidth;
		rect.bottom = ConHeight;
		ScrollWindowEx (con_hWnd, 0, -OemHeight, NULL, &rect, NULL, NULL, SW_ERASE|SW_INVALIDATE);
		UpdateWindow (con_hWnd);
	}
}

int I_ConPrintString (const char *outline) 
{
	const char *cp, *newcp;
	static int xp = 0;
	int newxp;
	BOOL scroll;

  if (!con_hWnd)
    return 0;
	cp = outline;
	while (*cp) {
		for (newcp = cp, newxp = xp;
			*newcp != '\n' && *newcp != '\0' && newxp < 80;
			 newcp++, newxp++) {
			if (*newcp == '\x8') {
				if (xp) xp--;
				newxp = xp;
				cp++;
			}
		}

		if (*cp) {
			const char *poop;
			int x;

			for (x = xp, poop = cp; poop < newcp; poop++, x++) {
        Last[x+2] = ((*poop) < 32) ? 32 : (*poop);
			}

			if (Last[1] < xp + (newcp - cp))
				Last[1] = xp + (newcp - cp);

			if (*newcp == '\n' || xp == 80) {
				if (*newcp != '\n') {
					Last[0] = 1;
				}
				memmove (Lines, Lines + (80 + 2), (80 + 2) * (25 - 1));
				Last[0] = 0;
				Last[1] = 0;
				newxp = 0;
				scroll = true;
			} else {
				scroll = false;
			}
			I_PrintStr (xp, cp, newcp - cp, scroll);

			xp = newxp;

			if (*newcp == '\n')
				cp = newcp + 1;
			else
				cp = newcp;
		}
	}

	return strlen (outline);
}

void Init_Console(void)
{
  memset(Lines,0,25*(80+2)+1);
	Last = Lines + (25 - 1) * (80 + 2);
}

int Init_ConsoleWin(void)
{
    HDC conDC;
    WNDCLASS wndclass;
	  TEXTMETRIC metrics;
	  RECT cRect;
    int width,height;
    int scr_width,scr_height;
    HINSTANCE hInstance;

    hInstance = GetModuleHandle(NULL);
    Init_Console();
    /* Register the frame class */
    wndclass.style         = CS_OWNDC;
    wndclass.lpfnWndProc   = (WNDPROC)ConWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon (hInstance, IDI_WINLOGO);
    wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
    wndclass.lpszMenuName  = szConName;
    wndclass.lpszClassName = szConName;

    if (!RegisterClass(&wndclass))
        return FALSE;

    width=100;
    height=100;
    con_hWnd = CreateWindow(szConName, szConName, 
             WS_CAPTION | WS_POPUP,
             0, 0, width, height,
             NULL, NULL, hInstance, NULL);
    conDC=GetDC(con_hWnd);
    OemFont = GetStockObject(OEM_FIXED_FONT);
	  SelectObject(conDC, OemFont);
	  GetTextMetrics(conDC, &metrics);
	  OemWidth = metrics.tmAveCharWidth;
	  OemHeight = metrics.tmHeight;
	  GetClientRect(con_hWnd, &cRect);
    width += (OemWidth * 80) - cRect.right;
    height += (OemHeight * 25) - cRect.bottom;
    // proff 11/09/98: Added code for centering console
    scr_width = GetSystemMetrics(SM_CXFULLSCREEN);
    scr_height = GetSystemMetrics(SM_CYFULLSCREEN);
    MoveWindow(con_hWnd, (scr_width-width)/2, (scr_height-height)/2, width, height, TRUE);
	  GetClientRect(con_hWnd, &cRect);
	  ConWidth = cRect.right;
	  ConHeight = cRect.bottom;
    SetTextColor(conDC, RGB(192,192,192));
  	SetBkColor(conDC, RGB(0,0,0));
		SetBkMode(conDC, OPAQUE);
    ReleaseDC(con_hWnd,conDC);
    ShowWindow(con_hWnd, SW_SHOW);
    UpdateWindow(con_hWnd);
}

void Done_ConsoleWin()
{
  DestroyWindow(con_hWnd);
  UnregisterClass(szConName,GetModuleHandle(NULL));
  con_hWnd=0;
}

void V_SetPal(unsigned char *pal)
{
  int c;
  int col;
  SDL_Color *orig_colors;

  if (sdl_screen == NULL)
    return;
  
  orig_colors = (SDL_Color *)malloc(256*sizeof(SDL_Color));
  if (orig_colors == NULL)
    return;

  col = 0;
  for ( c=0; c<256; ++c )
  {
    orig_colors[c].r = ((int)pal[col++]);
    orig_colors[c].g = ((int)pal[col++]);
    orig_colors[c].b = ((int)pal[col++]);
  }
  SDL_SetColors(sdl_screen, orig_colors, 0, 256);
}

void Set_Title(void)
{
  char *p, *pEnd;

#ifdef GL_DOOM
  {
    memset(szGLTitle,0,sizeof(szGLTitle));
    sprintf(szGLTitle,
            "PrBoom OpenGL %i.%02i beta - ",
            VERSION/100,VERSION%100);
  }
#else
  {
    memset(szTitle,0,sizeof(szTitle));
    sprintf(szTitle,
            "PrBoom %i.%02i - ",
            VERSION/100,VERSION%100);
  }
#endif

  p = title;
  pEnd = p + strlen(title) - 1;
  while (*p == ' ') p++;
  while (*pEnd == ' ') pEnd--;
  pEnd++;
  *pEnd = 0;
  if (pEnd>p)
  {
#ifdef GL_DOOM
    memcpy(&szGLTitle[strlen(szGLTitle)],p,strlen(p));
#else
    memcpy(&szTitle[strlen(szTitle)],p,strlen(p));
#endif
  }
#ifdef GL_DOOM
  SDL_WM_SetCaption(szGLTitle,szGLTitle);
#else
  SDL_WM_SetCaption(szTitle,szTitle);
#endif
}

void Init_Mouse(void)
{
// proff 08/15/98: Made -grabmouse default
//    if (M_CheckParm("-grabmouse"))
    grabMouse=true;
    if (M_CheckParm("-nomouse"))
      noMouse=true;
    else
      noMouse=(usemouse==0);
}

int Init_Winstuff(void)
{
  Done_ConsoleWin();
  Init_Mouse();
  // Set the windowtitle
  Set_Title();
  // proff 07/22/98: Added options -fullscr and -nofullscr
  if (M_CheckParm("-fullscr"))
    vidFullScreen=1;
  if (M_CheckParm("-nofullscr"))
    vidFullScreen=0;
#ifndef GL_DOOM
  if (vidFullScreen)
    sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 8, SDL_HWSURFACE | SDL_HWPALETTE | SDL_FULLSCREEN);
  else
    sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 8, SDL_HWSURFACE | SDL_HWPALETTE);
  if ( sdl_screen == NULL )
  {
    I_Error("Couldn't set 640x480x8 video mode: %s\n",SDL_GetError());
  }
#else
  SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  if (vidFullScreen)
    sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 16, SDL_OPENGL | SDL_FULLSCREEN);
  else
    sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 16, SDL_OPENGL);
  if ( sdl_screen == NULL )
  {
    I_Error("Couldn't set 640x480x8 video mode: %s\n",SDL_GetError());
  }
  gld_Init(SCREENWIDTH, SCREENHEIGHT);
#endif
  SDL_ShowCursor(0);
  return TRUE;
}

void Done_Winstuff(void)
{
}

void V_EndFrame (void)
{
  int y;
  int *Surface,*doommem;

  if (!sdl_screen)
    return;

  if ( SDL_MUSTLOCK(sdl_screen) )
  {
    if ( SDL_LockSurface(sdl_screen) < 0 )
      return;
  }
  Surface = (int *)((char *)sdl_screen->pixels
            +(((sdl_screen->h-SCREENHEIGHT)/2)*sdl_screen->pitch)
            +((sdl_screen->w-SCREENWIDTH)/2));
  doommem = (int *)screens[0];
  for (y=0; y<SCREENHEIGHT; y++)
  {
    memcpy(Surface,doommem,SCREENWIDTH);
    Surface += sdl_screen->pitch/4;
    doommem += SCREENWIDTH/4;
  }

  if ( SDL_MUSTLOCK(sdl_screen) )
  {
    SDL_UnlockSurface(sdl_screen);
  }
  SDL_UpdateRect(sdl_screen, 0, 0, SCREENWIDTH, SCREENHEIGHT);
}

// proff - I have taken this from lsdldoom
static int I_TranslateKey(SDL_keysym* key)
{
  int rc = 0;

  switch (key->sym) {
  case SDLK_LEFT:	rc = KEYD_LEFTARROW;	break;
  case SDLK_RIGHT:	rc = KEYD_RIGHTARROW;	break;
  case SDLK_DOWN:	rc = KEYD_DOWNARROW;	break;
  case SDLK_UP:		rc = KEYD_UPARROW;	break;
  case SDLK_ESCAPE:	rc = KEYD_ESCAPE;	break;
  case SDLK_RETURN:	rc = KEYD_ENTER;	break;
  case SDLK_TAB:	rc = KEYD_TAB;		break;
  case SDLK_F1:		rc = KEYD_F1;		break;
  case SDLK_F2:		rc = KEYD_F2;		break;
  case SDLK_F3:		rc = KEYD_F3;		break;
  case SDLK_F4:		rc = KEYD_F4;		break;
  case SDLK_F5:		rc = KEYD_F5;		break;
  case SDLK_F6:		rc = KEYD_F6;		break;
  case SDLK_F7:		rc = KEYD_F7;		break;
  case SDLK_F8:		rc = KEYD_F8;		break;
  case SDLK_F9:		rc = KEYD_F9;		break;
  case SDLK_F10:	rc = KEYD_F10;		break;
  case SDLK_F11:	rc = KEYD_F11;		break;
  case SDLK_F12:	rc = KEYD_F12;		break;
  case SDLK_BACKSPACE:
  case SDLK_DELETE:	rc = KEYD_BACKSPACE;	break;
  case SDLK_PAUSE:	rc = KEYD_PAUSE;	break;
  case SDLK_EQUALS:	rc = KEYD_EQUALS;	break;
  case SDLK_MINUS:	rc = KEYD_MINUS;	break;
  case SDLK_KP0:	rc = KEYD_KEYPAD0;	break;
  case SDLK_KP1:	rc = KEYD_KEYPAD1;	break;
  case SDLK_KP2:	rc = KEYD_KEYPAD2;	break;
  case SDLK_KP3:	rc = KEYD_KEYPAD3;	break;
  case SDLK_KP4:	rc = KEYD_KEYPAD4;	break;
  case SDLK_KP5:	rc = KEYD_KEYPAD0;	break;
  case SDLK_KP6:	rc = KEYD_KEYPAD6;	break;
  case SDLK_KP7:	rc = KEYD_KEYPAD7;	break;
  case SDLK_KP8:	rc = KEYD_KEYPAD8;	break;
  case SDLK_KP9:	rc = KEYD_KEYPAD9;	break;
  case SDLK_KP_PLUS:	rc = KEYD_KEYPADPLUS;	break;
  case SDLK_KP_MINUS:	rc = KEYD_KEYPADMINUS;	break;
  case SDLK_KP_DIVIDE:	rc = KEYD_KEYPADDIVIDE;	break;
  case SDLK_KP_MULTIPLY: rc = KEYD_KEYPADMULTIPLY; break;
  case SDLK_KP_ENTER:	rc = KEYD_KEYPADENTER;	break;
  case SDLK_KP_PERIOD:	rc = KEYD_KEYPADPERIOD;	break;
  case SDLK_NUMLOCK:	rc = KEYD_NUMLOCK;	break;
  case SDLK_SCROLLOCK:	rc = KEYD_SCROLLLOCK;	break;
  case SDLK_CAPSLOCK:	rc = KEYD_CAPSLOCK;	break;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:	rc = KEYD_RSHIFT;	break;
  case SDLK_LCTRL:
  case SDLK_RCTRL:	rc = KEYD_RCTRL;	break;
  case SDLK_LALT:
  case SDLK_LMETA:
  case SDLK_RALT:
  case SDLK_RMETA:	rc = KEYD_RALT;		break;
  default:		rc = key->sym;		break;
  }

  //lprintf(LO_INFO,"Key: %i %s\n",key->sym,SDL_GetKeyName(key->sym));
  return rc;

}

void V_GetMessages (void)
{
  SDL_Event sdl_event;
  event_t event;

  //SDL_PumpEvents();
  while (SDL_PollEvent(&sdl_event))
  {
    switch (sdl_event.type)
    {
    case SDL_KEYDOWN:
      event.type = ev_keydown;
      event.data1 = I_TranslateKey(&sdl_event.key.keysym);
      if (event.data1)
        D_PostEvent(&event);
      break;

    case SDL_KEYUP:
      event.type = ev_keyup;
      event.data1 = I_TranslateKey(&sdl_event.key.keysym);
      if (event.data1)
        D_PostEvent(&event);
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      if (usemouse)
      {
        Uint8 buttonstate;
        buttonstate = SDL_GetMouseState(NULL, NULL);
        event.type = ev_mouse;
        event.data1 = 0
          | (buttonstate & SDL_BUTTON(1) ? 1 : 0)
          | (buttonstate & SDL_BUTTON(2) ? 2 : 0)
          | (buttonstate & SDL_BUTTON(3) ? 4 : 0);
        event.data2 = event.data3 = 0;
        D_PostEvent(&event);
      }
      break;

    case SDL_MOUSEMOTION:
      /* Ignore mouse warp events */
      if (usemouse && ((sdl_event.motion.x != sdl_screen->w/2)||(sdl_event.motion.y != sdl_screen->h/2)))
      {
        /* Warp the mouse back to the center */
        if (grabMouse)
        {
          if ((sdl_event.motion.x < ((sdl_screen->w/2)-(sdl_screen->w/4))) ||
              (sdl_event.motion.x > ((sdl_screen->w/2)+(sdl_screen->w/4))) ||
              (sdl_event.motion.y < ((sdl_screen->h/2)-(sdl_screen->h/4))) ||
              (sdl_event.motion.y > ((sdl_screen->h/2)+(sdl_screen->h/4))) )
          SDL_WarpMouse(sdl_screen->w/2, sdl_screen->h/2);
        }
        event.type = ev_mouse;
        event.data1 = 0
          | (sdl_event.motion.state & SDL_BUTTON(1) ? 1 : 0)
          | (sdl_event.motion.state & SDL_BUTTON(2) ? 2 : 0)
          | (sdl_event.motion.state & SDL_BUTTON(3) ? 4 : 0);
        event.data2 = sdl_event.motion.xrel << 2;
        event.data3 = -sdl_event.motion.yrel << 2;
        D_PostEvent(&event);
      }
      break;

    default:
      break;
    }
  }
}
