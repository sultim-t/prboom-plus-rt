// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: WINSTUFF.C,v 1.1 2000/04/09 18:03:26 proff_fs Exp $
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
// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined (_MSC_VER) || defined (__MINGW32__)
#ifndef GL_DOOM
#define DIRECTX
#endif
#endif
/*
#if defined (DIRECTX) && !defined (__MINGW32__)
#define __BYTEBOOL__
#define false 0
#define true !false
#endif
*/
#if defined (DIRECTX)
#include <ddraw.h>
#endif
#ifdef GL_DOOM
#include "opengl.h"
#include "gl_struct.h"
#endif
#include "doomtype.h"
#include "doomdef.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "v_video.h"
#include "i_system.h"
#include "lprintf.h"

extern int usemouse;
extern char title[128];
char szTitle[256];
char szGLTitle[256];
char szConTitle[256];

#ifdef GL_DOOM
char szGLName[] = "PrBoomGLWinClass";
HDC GL_DC = 0;
HGLRC gl_hRC  = 0;
    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
        1,                              // version number
        PFD_DRAW_TO_WINDOW |            // support window
        PFD_SUPPORT_OPENGL |            // support OpenGL
        PFD_DOUBLEBUFFER ,              // double buffered
        PFD_TYPE_RGBA,                  // RGBA type
        16,                             // 16-bit color depth
        0, 0, 0, 0, 0, 0,               // color bits ignored
        0,                              // no alpha buffer
        0,                              // shift bit ignored
        0,                              // no accumulation buffer
        0, 0, 0, 0,                     // accum bits ignored
        16,                             // 32-bit z-buffer      
        0,                              // no stencil buffer
        0,                              // no auxiliary buffer
        PFD_MAIN_PLANE,                 // main layer
        0,                              // reserved
        0, 0, 0                         // layer masks ignored
    };
#endif

HWND ghWnd;
char szAppName[] = "PrBoomWinClass";
HINSTANCE win_hInstance;
int frameX, frameY, capY;
// proff 06/30/98: Changed form constant value to defined value
// proff 08/17/98: Changed for high-res
//int MainWinWidth=SCREENWIDTH, MainWinHeight=SCREENHEIGHT;
int MainWinWidth;
int MainWinHeight;
int vidMultiply=1;
int vidNoModesEnum=0;
int vidDirectDrawDevice=0;
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

BITMAPINFO *View_bmi;
BYTE *ViewMem;
BYTE *ScaledVMem;
// proff 06/30/98: Changed form constant value to defined value
// proff 08/17/98: Changed for high-res
//int ViewMemPitch=SCREENWIDTH;
int ViewMemPitch;
RECT ViewRect;
HDC ViewDC = 0;
enum {
    Scale_None,
    Scale_Windows,
    Scale_Own
} ViewScale;

H_boolean fActive = false;
// proff: Removed fFullscreen
int vidFullScreen = 0;

H_boolean noMouse = false;
H_boolean grabMouse = false;
int MouseButtons = 0;
extern int usejoystick;

H_boolean noidle=false;

//H_boolean noDDraw = true;
#ifdef DIRECTX
LPDIRECTDRAW lpDD;
LPDIRECTDRAWSURFACE lpDDPSF;
LPDIRECTDRAWSURFACE lpDDBSF;
LPDIRECTDRAWPALETTE lpDDPal;
PALETTEENTRY ADDPal[256];
int BestWidth,BestHeight;
// proff 11/21/98: Added DirectDraw device selection
LPGUID DeviceGUIDs[16];
int DeviceCount=0;
int DDrawDevice=0;
#endif

int I_ScanCode2DoomCode (int a);
void V_GetMessages (void);

void (*FullscreenProc)(int fullscreen);

static void WinFullscreen(int fullscreen)
{
    vidFullScreen=0;
    lprintf(LO_WARN,"Fullscreen-Mode not available\n");
}

static CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    event_t event;
    H_boolean AltDown;
    RECT rect;
    RECT rectc;
    RECT rectw;

    switch (iMsg) {
// proff 07/29/98: Added WM_CLOSE
    case WM_CLOSE:
    return 1;
    break;
    case WM_MOVE:
    case WM_SIZE:
/*
        if ((!noMouse) && (grabMouse))
        {
            ClipCursor(NULL);
            GetWindowRect(ghWnd, &rectw);
            GetClientRect(ghWnd, &rectc);
            rect.left = rectw.left + frameX;
            rect.top = rectw.top + frameY + capY;
            rect.right = rect.left + (rectc.right - rectc.left);
            rect.bottom = rect.top + (rectc.bottom - rectc.top);
            ClipCursor(&rect);
        }
*/        
        break;
		case WM_KILLFOCUS:
      // proff 08/18/98: This sets the priority-class
			if (!noidle)
				SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
      break;
		case WM_SETFOCUS:
      // proff 08/18/98: This sets the priority-class
			SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
      break;
    case WM_DESTROY:
      // proff 08/18/98: This sets the priority-class
			SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
//      PostQuitMessage(0);
      break;
    case WM_ACTIVATE:
        fActive = (H_boolean)LOWORD(wParam);
        if (fActive)
        {
            #ifdef _DEBUG
            lprintf (LO_DEBUG, "WM_ACTIVATE true\n");
            #endif
            event.type = ev_keyup;
            event.data1 = KEYD_RCTRL;
            event.data2 = 0;
            event.data3 = 0;
            D_PostEvent(&event);
            event.data1 = KEYD_RALT;
            D_PostEvent(&event);
            MouseButtons=0;
            event.type = ev_mouse;
            event.data1 = MouseButtons;
            event.data2 = 0;
            event.data3 = 0;
            D_PostEvent(&event);
            ShowCursor(FALSE);
            if ((!noMouse) && (grabMouse))
            {
                ClipCursor(NULL);
                GetWindowRect(ghWnd, &rectw);
                GetClientRect(ghWnd, &rectc);
                rect.left = rectw.left + frameX;
                rect.top = rectw.top + frameY + capY;
                rect.right = rect.left + (rectc.right - rectc.left);
                rect.bottom = rect.top + (rectc.bottom - rectc.top);
                ClipCursor(&rect);
            }
        }
        else
        {
            #ifdef _DEBUG
            lprintf (LO_DEBUG, "WM_ACTIVATE false\n");
            #endif
            ShowCursor(TRUE);
            if ((!noMouse) && (grabMouse))
            {
                ClipCursor(NULL);
            }
        }
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        event.type = ev_keydown;
        event.data1 = I_ScanCode2DoomCode(((lParam >> 16) & 0x00ff));
        // proff 08/18/98: Now the pause-key works
        if (wParam==VK_PAUSE)
          event.data1=KEYD_PAUSE;
        event.data2 = 0;
        event.data3 = 0;
        if ( event.data1 != 0 )
            D_PostEvent(&event);
        AltDown = (GetAsyncKeyState(VK_MENU) < 0);
        if ((AltDown) & (wParam == VK_RETURN))
        {
            vidFullScreen = 1-vidFullScreen;
            if (FullscreenProc)
                FullscreenProc(vidFullScreen);
        }
       break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        event.type = ev_keyup;
        event.data1 = I_ScanCode2DoomCode(((lParam >> 16) & 0x00ff));
        // proff 08/18/98: Now the pause-key works
        if (wParam==VK_PAUSE)
          event.data1=KEYD_PAUSE;
        event.data2 = 0;
        event.data3 = 0;
        if ( event.data1 != 0 )
            D_PostEvent(&event);
       break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      if (noMouse)
        break;
      MouseButtons = 0;
      if (wParam & MK_LBUTTON)
        MouseButtons += 1;
      if (wParam & MK_RBUTTON)
        MouseButtons += 2;
      if (wParam & MK_MBUTTON)
        MouseButtons += 4;
      break;
    default:
        return(DefWindowProc(hwnd,iMsg,wParam,lParam));
    }
// proff 08/18/98: Removed because I think it's useless
//    return(DefWindowProc(hwnd,iMsg,wParam,lParam));
}

static CALLBACK ConWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT paint;
  HDC dc;

  switch (iMsg) {
  case WM_CLOSE:
    return 1;
    break;
  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYUP:
    SendMessage(ghWnd,iMsg,wParam,lParam);
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

void V_SetPal(unsigned char *pal)
{
    int c;
    int col;

    if (View_bmi == NULL)
        return;
#ifdef DIRECTX
    if ((vidFullScreen) & (lpDDPal != NULL) & (lpDDBSF != NULL))
    {
        col = 0;
        for (c=0; c<256; c++)
        {
            ADDPal[c].peRed = pal[col++];
            ADDPal[c].peGreen = pal[col++];
            ADDPal[c].peBlue = pal[col++];
            ADDPal[c].peFlags = 0;
        }
        lpDDPal->lpVtbl->SetEntries(lpDDPal,0,0,256,ADDPal);
    }
    else
#endif
    {
        col = 0;
        for (c=0; c<256; c++)
        {
            View_bmi->bmiColors[c].rgbRed = pal[col++];
            View_bmi->bmiColors[c].rgbGreen = pal[col++];
            View_bmi->bmiColors[c].rgbBlue = pal[col++];
            View_bmi->bmiColors[c].rgbReserved = 0;
        }
    }
}

void Init_Console(void)
{
  memset(Lines,0,25*(80+2)+1);
	Last = Lines + (25 - 1) * (80 + 2);
}

int Init_ConsoleWin(HINSTANCE hInstance)
{
    HDC conDC;
    WNDCLASS wndclass;
	  TEXTMETRIC metrics;
	  RECT cRect;
    int width,height;
    int scr_width,scr_height;

    Init_Console();
    win_hInstance = hInstance;
    /* Register the frame class */
    wndclass.style         = CS_OWNDC;
    wndclass.lpfnWndProc   = (WNDPROC)ConWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon (win_hInstance, IDI_WINLOGO);
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

int Init_Win()
{
    WNDCLASS wndclass;

    FullscreenProc = WinFullscreen;
    MainWinWidth=SCREENWIDTH;
    MainWinHeight=SCREENHEIGHT;

// proff 08/18/98: This disables the setting of the priority-class
    if (M_CheckParm("-noidle"))
      noidle=true;

    /* Register the frame class */
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = (WNDPROC)WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = win_hInstance;
    wndclass.hIcon         = LoadIcon (win_hInstance, IDI_WINLOGO);
    wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
//    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = szAppName;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
        return FALSE;

    ghWnd = CreateWindow(szAppName, szAppName, 
             WS_CAPTION | WS_POPUP,
// proff 06/30/98: Changed form constant value to variable
             0, 0, MainWinWidth, MainWinHeight,
             NULL, NULL, win_hInstance, NULL);
    ShowWindow(ghWnd, SW_HIDE);
    UpdateWindow(ghWnd);
}

#ifdef GL_DOOM

int Init_GLWin(void)
{  
    BYTE openglLib[256];
    WNDCLASS wndclass;

    OpenGLFindDriver(openglLib);
    if (OpenGLInit(openglLib)) {
      MessageBox(NULL, "Unable to load opengl dll", "OpenGLInit Error", MB_ICONERROR); 
      return false;
    }
    lprintf(LO_INFO, "Using OpenGL Driver: %s\n", openglLib);

    FullscreenProc = WinFullscreen;
    MainWinWidth=SCREENWIDTH;
    MainWinHeight=SCREENHEIGHT;

// proff 08/18/98: This disables the setting of the priority-class
    if (M_CheckParm("-noidle"))
      noidle=true;

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = (WNDPROC)WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = win_hInstance;
    wndclass.hIcon         = LoadIcon (win_hInstance, IDI_WINLOGO);
    wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
    wndclass.lpszMenuName  = szGLName;
    wndclass.lpszClassName = szGLName;

    if (!RegisterClass(&wndclass))
        I_Error("RegisterClass() failed!");
/* PROFF_GL
		if (M_CheckParm("-fullscreen"))
			fn_vChangeScreenResolution();
*/
    ghWnd = CreateWindow(szGLName, szGLName, 
             WS_CAPTION | WS_POPUP,
             400, 0, MainWinWidth, MainWinHeight,
             NULL, NULL, win_hInstance, NULL);
    ShowWindow(ghWnd, SW_HIDE);
    UpdateWindow(ghWnd);
    return true;
}
#endif

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
// proff 11/06/98: Added setting of console title
  memset(szConTitle,0,sizeof(szConTitle));
  sprintf(szConTitle,
          "PrBoom Console %i.%02i - ",
          VERSION/100,VERSION%100);

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
    memcpy(&szConTitle[strlen(szConTitle)],p,strlen(p));
  }
#ifdef GL_DOOM
  SetWindowText(ghWnd,szGLTitle);
#else
  SetWindowText(ghWnd,szTitle);
#endif
  SetWindowText(con_hWnd,szConTitle);
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

void Init_Dib(void)
{
    View_bmi=malloc(sizeof(BITMAPINFO)+256*4);
    memset(View_bmi,0,40);
    View_bmi->bmiHeader.biSize = 40;
    View_bmi->bmiHeader.biPlanes = 1;
    View_bmi->bmiHeader.biBitCount = 8;
    View_bmi->bmiHeader.biCompression = BI_RGB;
    if (ViewScale==Scale_Own)
    {
        View_bmi->bmiHeader.biWidth = SCREENWIDTH*2;
        View_bmi->bmiHeader.biHeight = SCREENHEIGHT*2;
        ViewMem = malloc((SCREENWIDTH*2)*(SCREENHEIGHT*2));
    }
    else if (ViewScale==Scale_Windows)
    {
        View_bmi->bmiHeader.biWidth = SCREENWIDTH;
        View_bmi->bmiHeader.biHeight = -SCREENHEIGHT;
        ViewMem=NULL;
    }
    else
    {
        View_bmi->bmiHeader.biWidth = SCREENWIDTH;
        View_bmi->bmiHeader.biHeight = -SCREENHEIGHT;
        ViewMem=NULL;
    }
    ViewDC = GetDC(ghWnd);
//    SetStretchBltMode(ViewDC,COLORONCOLOR);
}

#ifdef DIRECTX
static void Done_DDraw(void)
{
    if (lpDD)
        lpDD->lpVtbl->RestoreDisplayMode(lpDD);
    if (lpDD)
        lpDD->lpVtbl->SetCooperativeLevel(lpDD,NULL,DDSCL_NORMAL);
    if (lpDDPal)
        lpDDPal->lpVtbl->Release(lpDDPal);
    if (lpDDPSF)
        lpDDPSF->lpVtbl->Release(lpDDPSF);
    if (lpDD)
        lpDD->lpVtbl->Release(lpDD);
    lpDDPal=NULL;
    lpDDPSF=NULL;
    lpDD=NULL;
    MoveWindow(ghWnd,0,0,MainWinWidth,MainWinHeight,TRUE);
    BringWindowToTop(ghWnd);
}

static void DDrawFullscreen(int fullscreen)
{
  HRESULT error;
  DDSURFACEDESC ddSD;
  DDSCAPS ddSDC;
  int c;

  if (fullscreen)
  {
    vidFullScreen = 0;
    // proff 11/21/98: Added DirectDraw device selection
    error = DirectDrawCreate(DeviceGUIDs[DDrawDevice],&lpDD,NULL);
    if (error != DD_OK)
    {
      lprintf(LO_WARN,"Error: DirectDrawCreate failed!\n");
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    error = lpDD->lpVtbl->SetCooperativeLevel(lpDD,ghWnd,DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (error != DD_OK)
    {
      lprintf(LO_WARN,"Error: DirectDraw_SetCooperativeLevel failed!\n");
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    error = lpDD->lpVtbl->SetDisplayMode(lpDD,BestWidth,BestHeight,8);
    if (error != DD_OK)
    {
      lprintf(LO_WARN,"Error: DirectDraw_SetDisplayMode %ix%ix8 failed!\n",BestWidth,BestHeight);
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    else
      dprintf("DDrawMode %ix%i",BestWidth,BestHeight);
    ddSD.dwSize = sizeof(ddSD);
    ddSD.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddSD.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddSD.dwBackBufferCount = 1;
    error = lpDD->lpVtbl->CreateSurface(lpDD,&ddSD,&lpDDPSF,NULL);
    if (error != DD_OK)
    {
      lpDDPSF = NULL;
      lprintf(LO_WARN,"Error: DirectDraw_CreateSurface failed!\n");
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    ddSDC.dwCaps = DDSCAPS_BACKBUFFER;
    error = lpDDPSF->lpVtbl->GetAttachedSurface(lpDDPSF,&ddSDC,&lpDDBSF);
    if (error != DD_OK)
    {
      lpDDBSF = NULL;
      lprintf(LO_WARN,"Error: DirectDraw_GetAttachedSurface failed!\n");
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    error = lpDD->lpVtbl->CreatePalette(lpDD,DDPCAPS_8BIT | DDPCAPS_ALLOW256,ADDPal,&lpDDPal,NULL);
    if (error != DD_OK)
    {
      lpDDPal = NULL;
      lprintf(LO_WARN,"Error: DirectDraw_CreatePal failed!\n");
      FullscreenProc=WinFullscreen;
      Done_DDraw();
      return;
    }
    error = lpDDPSF->lpVtbl->SetPalette(lpDDPSF,lpDDPal);
    error = lpDDBSF->lpVtbl->SetPalette(lpDDBSF,lpDDPal);
    for (c=0; c<256; c++)
    {
      ADDPal[c].peRed = View_bmi->bmiColors[c].rgbRed;
      ADDPal[c].peGreen = View_bmi->bmiColors[c].rgbGreen;
      ADDPal[c].peBlue = View_bmi->bmiColors[c].rgbBlue;
      ADDPal[c].peFlags = 0;
    }
    lpDDPal->lpVtbl->SetEntries(lpDDPal,0,0,256,ADDPal);
    vidFullScreen = 1;
    lprintf(LO_INFO,"Fullscreen-Mode\n");
  }
  else
  {
    Done_DDraw();
    if (ViewScale==Scale_Own)
      ViewMemPitch=SCREENWIDTH*2;
    else
      ViewMemPitch=SCREENWIDTH;
    for (c=0; c<256; c++)
    {
      View_bmi->bmiColors[c].rgbRed = ADDPal[c].peRed;
      View_bmi->bmiColors[c].rgbGreen = ADDPal[c].peGreen;
      View_bmi->bmiColors[c].rgbBlue = ADDPal[c].peBlue;
    }
    lprintf(LO_INFO,"Windows-Mode\n");
  }
}

HRESULT WINAPI MyEnumModesCallback(LPDDSURFACEDESC lpDDSDesc, LPVOID lpContext)
{
  int SearchedWidth,SearchedHeight;

  SearchedWidth=SCREENWIDTH;
  if (ViewScale!=Scale_None)
    SearchedWidth*=2;
  SearchedHeight=SCREENHEIGHT;
  if (ViewScale!=Scale_None)
    SearchedHeight*=2;
  lprintf(LO_INFO,"W: %4i",lpDDSDesc->dwWidth);
  lprintf(LO_INFO,", H: %4i",lpDDSDesc->dwHeight);
  lprintf(LO_INFO,"\n");
  if (((int)lpDDSDesc->dwWidth>=SearchedWidth) & ((int)lpDDSDesc->dwHeight>=SearchedHeight))
    if ((BestWidth>(int)lpDDSDesc->dwWidth) & (BestHeight>(int)lpDDSDesc->dwHeight))
    {
      BestWidth=lpDDSDesc->dwWidth;
      BestHeight=lpDDSDesc->dwHeight;
    }
  return DDENUMRET_OK;
}

// proff 11/21/98: Added DirectDraw device selection
HRESULT WINAPI MyDDEnumCallback(GUID FAR *lpGUID, LPSTR  lpDriverDescription,
                      LPSTR  lpDriverName, LPVOID lpContext)
{
  if (DeviceCount==16)
    return DDENUMRET_CANCEL;
  lprintf(LO_INFO,"Device %i: %s\n",DeviceCount,lpDriverDescription);
  DeviceGUIDs[DeviceCount]=lpGUID;
  DeviceCount++;
  return DDENUMRET_OK;
}

void Init_DDraw(void)
{
  HRESULT error;
  DDSURFACEDESC DDSDesc;
  int i;

  // proff 11/21/98: Added DirectDraw device selection
  i = M_CheckParm ("-ddrawdevice");
  if ((i) || (vidDirectDrawDevice))
  {
    DirectDrawEnumerate(&MyDDEnumCallback,NULL);
    DDrawDevice=vidDirectDrawDevice;
    if (i)
      DDrawDevice=atoi(myargv[i+1]);
    DDrawDevice=(DDrawDevice<0) ? 0 : ((DDrawDevice>=DeviceCount) ? 0 : DDrawDevice);
  }

  FullscreenProc = DDrawFullscreen;
  BestWidth=INT_MAX;
  BestHeight=INT_MAX;
  if (!vidNoModesEnum)
  {
    error = DirectDrawCreate(DeviceGUIDs[DDrawDevice],&lpDD,NULL);
    // proff 11/21/98: End of additions and changes
    if (error != DD_OK)
    {
      lprintf(LO_WARN,"Error: DirectDrawCreate failed!\n");
      FullscreenProc=WinFullscreen;
      return;
    }
    error = lpDD->lpVtbl->SetCooperativeLevel(lpDD,ghWnd,DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (error != DD_OK)
    {
      lprintf(LO_WARN,"Error: DirectDraw_SetCooperativeLevel failed!\n");
      FullscreenProc=WinFullscreen;
      if (lpDD)
      {
        lpDD->lpVtbl->Release(lpDD);
        lpDD=NULL;
      }
      return;
    }
    DDSDesc.dwSize=sizeof(DDSURFACEDESC);
    DDSDesc.dwFlags=DDSD_PIXELFORMAT;
    DDSDesc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    DDSDesc.ddpfPixelFormat.dwFlags=DDPF_PALETTEINDEXED8 | DDPF_RGB;
#ifdef _MSC_VER
    DDSDesc.ddpfPixelFormat.dwRGBBitCount=8;
#else
    DDSDesc.ddpfPixelFormat.u1.dwRGBBitCount=8;
#endif
    lpDD->lpVtbl->EnumDisplayModes(lpDD,0,&DDSDesc,NULL,&MyEnumModesCallback);
    if (((BestWidth==INT_MAX) | (BestHeight==INT_MAX)) & (ViewScale!=Scale_None))
    {
      ViewScale=Scale_None;
      BestWidth=INT_MAX;
      BestHeight=INT_MAX;
      lpDD->lpVtbl->EnumDisplayModes(lpDD,0,&DDSDesc,NULL,&MyEnumModesCallback);
    }
  }
  if ((BestWidth==INT_MAX) | (BestHeight==INT_MAX))
  {
    BestWidth=0;
    BestHeight=0;
    lprintf(LO_WARN,"Error: No DirectDraw mode, which suits the needs, found!\n");
    lprintf(LO_WARN,"Searched Mode: W: %4i, H: %4i, BPP 8\n",SCREENWIDTH*(ViewScale!=Scale_None?2:1),SCREENHEIGHT*(ViewScale!=Scale_None?2:1));
    if (vidNoModesEnum)
    {
      lprintf(LO_WARN,"But we will try to set the searched mode!\n");
      BestWidth=SCREENWIDTH*(ViewScale!=Scale_None?2:1);
      BestHeight=SCREENHEIGHT*(ViewScale!=Scale_None?2:1);
    }
    else
      FullscreenProc=WinFullscreen;
  }
  else
  {
    lprintf(LO_INFO,"SearchedWidth: %4i, SearchedHeight: %4i\n",SCREENWIDTH*(ViewScale!=Scale_None?2:1),SCREENHEIGHT*(ViewScale!=Scale_None?2:1));
    lprintf(LO_INFO,"FoundWidth   : %4i, FoundHeight   : %4i\n",BestWidth,BestHeight);
  }
  if (lpDD)
    lpDD->lpVtbl->SetCooperativeLevel(lpDD,NULL,DDSCL_NORMAL);
  if (lpDD)
    lpDD->lpVtbl->Release(lpDD);
  lpDD=NULL;
  MoveWindow(ghWnd,0,0,MainWinWidth,MainWinHeight,TRUE);
  BringWindowToTop(ghWnd);
}
#endif

#ifdef GL_DOOM
BOOL SetupOpenGL(HDC hdc)
{
    int  pixelFormat;

    pixelFormat = OpenGLChoosePixelFormat(hdc, &pfd);

    if ( !pixelFormat )
	{
		I_Error("ChoosePixelFormat failed!");
		return FALSE;
	}
	if ( OpenGLSetPixelFormat(hdc, pixelFormat, &pfd) == FALSE ) 
	{
		I_Error("SetPixelFormat failed!");
		return FALSE;
	}
    return TRUE;
}

static void DisplayGLInfo()
{
  char info[2048];
//  const unsigned char *extension = glGetString(GL_EXTENSIONS);
  lprintf(LO_INFO, "Init GL Stuff.....\n"); 
  sprintf(info,"GL vendor    : %s\n", (char*)glGetString(GL_VENDOR));
  lprintf(LO_INFO, info); 
  sprintf(info,"GL version   : %s\n", (char*)glGetString(GL_VERSION));
  lprintf(LO_INFO, info); 
  sprintf(info,"GL renderer  : %s\n", (char*)glGetString(GL_RENDERER));
  lprintf(LO_INFO, info); 
  sprintf(info,"GL extensions: %s\n", (char*)glGetString(GL_EXTENSIONS));
  lprintf(LO_INFO, info); 
//  if (!strstr((char *)extension,"GL_EXT_paletted_texture"))
//  {
//	  lprintf(LO_INFO,"OpenGL ICD doesn't support paletted textures\n");
//  //	  g_bPaletteTexture = FALSE;
//  }
}

static void Done_GL(void)
{
  gld_CleanMemory();
  //	fn_vDesinitMemory();
	OpenGLMakeCurrent(NULL, NULL);
	if (gl_hRC)
  	OpenGLDeleteContext(gl_hRC);
//	fn_vRestoreScreenResolution();
  if (GL_DC)
    ReleaseDC(ghWnd,GL_DC);
}

static void GLFullscreen(int fullscreen)
{
  DEVMODE dmScreenSettings;       // Developer Mode
  if (fullscreen)
  {
    memset(&dmScreenSettings, 0, sizeof(DEVMODE));
    dmScreenSettings.dmSize         = sizeof(DEVMODE);
    dmScreenSettings.dmPelsWidth    = SCREENWIDTH;
    dmScreenSettings.dmPelsHeight   = SCREENHEIGHT;
    dmScreenSettings.dmFields       = DM_PELSWIDTH | DM_PELSHEIGHT;
    switch (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN))
    {
    case DISP_CHANGE_SUCCESSFUL:
      vidFullScreen=1;
      break;
    case DISP_CHANGE_BADFLAGS:
      lprintf(LO_WARN,"ChangeDisplaySettings returned DISP_CHANGE_BADFLAGS\n");
      vidFullScreen=0;
      break;
    case DISP_CHANGE_BADPARAM:
      lprintf(LO_WARN,"ChangeDisplaySettings returned DISP_CHANGE_BADPARAM\n");
      vidFullScreen=0;
      break;
    case DISP_CHANGE_BADMODE:
      lprintf(LO_WARN,"ChangeDisplaySettings returned DISP_CHANGE_BADMODE\n");
      vidFullScreen=0;
      break;
    default:
      lprintf(LO_WARN,"ChangeDisplaySettings failed\n");
      vidFullScreen=0;
      break;
    }
  }
  else
  {
    ChangeDisplaySettings(NULL, 0);
    vidFullScreen=0;
  }
  if (vidFullScreen)
  {
    SetWindowLong(ghWnd,GWL_STYLE,WS_POPUP);
    SetWindowPos(ghWnd,0,0,0,SCREENWIDTH,SCREENHEIGHT,SWP_SHOWWINDOW);
  }
  else
  {
    SetWindowLong(ghWnd,GWL_STYLE,WS_CAPTION | WS_POPUP);
    SetWindowPos(ghWnd,0,0,0,MainWinWidth,MainWinHeight,SWP_SHOWWINDOW);
  }
}

void Init_GL(void)
{
  GL_DC = GetDC(ghWnd);
	SetupOpenGL(GL_DC);
  gl_hRC = OpenGLCreateContext(GL_DC);
  OpenGLMakeCurrent(GL_DC, gl_hRC);
	DisplayGLInfo();
  gld_Init(SCREENWIDTH, SCREENHEIGHT);
  FullscreenProc = GLFullscreen;
}
#endif

int Init_Winstuff(void)
{
    int multiply;

    ShowWindow(ghWnd, SW_SHOW);
    V_GetMessages();
    Init_Mouse();
    if (vidMultiply==3)
      ViewScale= Scale_Own;
    else if (vidMultiply==2)
      ViewScale= Scale_Windows;
    else
      ViewScale= Scale_None;
    if (M_CheckParm("-m2"))
        ViewScale= Scale_Own;
    else if (M_CheckParm("-2"))
        ViewScale= Scale_Windows;

    // Set the windowtitle
    Set_Title();
#ifdef DIRECTX
    if (!M_CheckParm("-noddraw"))
      Init_DDraw();
#endif
    if (ViewScale==Scale_Own)
    {
        multiply=2;
        ViewMemPitch=SCREENWIDTH*2;
    }
    else if (ViewScale==Scale_Windows)
    {
        multiply=2;
        ViewMemPitch=SCREENWIDTH;
    }
    else
    {
        multiply=1;
        ViewScale= Scale_None;
        ViewMemPitch=SCREENWIDTH;
    }
#ifdef GL_DOOM
	  Init_GL();
#else
    Init_Dib();
#endif

    frameX = GetSystemMetrics(SM_CXFIXEDFRAME);
    frameY = GetSystemMetrics(SM_CYFIXEDFRAME);
    capY = GetSystemMetrics(SM_CYCAPTION);
    GetClientRect(ghWnd, &ViewRect);
    #ifdef _DEBUG
    lprintf( LO_DEBUG, "I_InitGraphics: Client area: %ux%u\n",
             ViewRect.right-ViewRect.left, ViewRect.bottom-ViewRect.top);
    #endif
    if ( (ViewRect.right-ViewRect.left) != (SCREENWIDTH *multiply) )
      MainWinWidth += (SCREENWIDTH *multiply) - (ViewRect.right -ViewRect.left);
    if ( (ViewRect.bottom-ViewRect.top) != (SCREENHEIGHT*multiply) )
      MainWinHeight+= (SCREENHEIGHT*multiply) - (ViewRect.bottom-ViewRect.top );
//    MainWinWidth =(frameX*2)+(SCREENWIDTH *multiply);
//    MainWinHeight=(frameY*2)+(SCREENHEIGHT*multiply)+capY;
    MoveWindow(ghWnd, 0, 0, MainWinWidth, MainWinHeight, TRUE);
    GetClientRect(ghWnd, &ViewRect);
    #ifdef _DEBUG
    lprintf( LO_DEBUG, "I_InitGraphics: Client area: %ux%u\n",
             ViewRect.right-ViewRect.left, ViewRect.bottom-ViewRect.top);
    #endif

    // proff 07/22/98: Added options -fullscr and -nofullscr
    if (M_CheckParm("-fullscr"))
        vidFullScreen=1;
    if (M_CheckParm("-nofullscr"))
        vidFullScreen=0;
    FullscreenProc(vidFullScreen);
#ifdef GL_DOOM
    BringWindowToTop(ghWnd);
#endif
    return TRUE;
}

void Done_Winstuff(void)
{
    if (FullscreenProc)
        FullscreenProc(false);
#ifdef DIRECTX
    Done_DDraw();
#endif
    if (ViewDC)
        ReleaseDC(ghWnd,ViewDC);
#ifdef GL_DOOM
    Done_GL();
#endif
    DestroyWindow(ghWnd);
    if ((!noMouse) && (grabMouse))
    {
        ClipCursor(NULL);
    }
}

void V_ScaleBy2D (void)
{
    unsigned int *olineptrs[2];
    register unsigned int *ilineptr;
    int x, y;
    register unsigned int twoopixels;
    register unsigned int twomoreopixels;
    register unsigned int fouripixels;

    ilineptr = (unsigned int *) screens[0];
    olineptrs[0] = (unsigned int *) &ScaledVMem[0];
    olineptrs[1] = (unsigned int *) &ScaledVMem[ViewMemPitch];

// proff 06/30/98: Changed form constant value to defined value
    for (y=SCREENHEIGHT; y>0; y--)
    {
// proff 06/30/98: Changed form constant value to defined value
        for (x=(SCREENWIDTH/4); x>0; x--)
        {
        fouripixels = *ilineptr++;
        twoopixels =    (fouripixels & 0xff000000)
            |    ((fouripixels>>8) & 0xffff00)
            |    ((fouripixels>>16) & 0xff);
        twomoreopixels =    ((fouripixels<<16) & 0xff000000)
            |    ((fouripixels<<8) & 0xffff00)
            |    (fouripixels & 0xff);
        *olineptrs[0]++ = twomoreopixels;
        *olineptrs[0]++ = twoopixels;
        *olineptrs[1]++ = twomoreopixels;
        *olineptrs[1]++ = twoopixels;
        }
        olineptrs[0] += ViewMemPitch>>2;
        olineptrs[1] += ViewMemPitch>>2;
    }
}

void V_ScaleBy2U (void)
{
    unsigned int *olineptrs[2];
    register unsigned int *ilineptr;
    int x, y;
    register unsigned int twoopixels;
    register unsigned int twomoreopixels;
    register unsigned int fouripixels;

// proff 06/30/98: Changed form constant value to defined value
    ilineptr = (unsigned int *) &screens[0][SCREENWIDTH*(SCREENHEIGHT-1)];
    olineptrs[0] = (unsigned int *) &ScaledVMem[0];
    olineptrs[1] = (unsigned int *) &ScaledVMem[ViewMemPitch];

// proff 06/30/98: Changed form constant value to defined value
    for (y=SCREENHEIGHT; y>0; y--)
    {
// proff 06/30/98: Changed form constant value to defined value
        for (x=(SCREENWIDTH/4); x>0; x--)
        {
        fouripixels = *ilineptr++;
        twoopixels =    (fouripixels & 0xff000000)
            |    ((fouripixels>>8) & 0xffff00)
            |    ((fouripixels>>16) & 0xff);
        twomoreopixels =    ((fouripixels<<16) & 0xff000000)
            |    ((fouripixels<<8) & 0xffff00)
            |    (fouripixels & 0xff);
        *olineptrs[0]++ = twomoreopixels;
        *olineptrs[0]++ = twoopixels;
        *olineptrs[1]++ = twomoreopixels;
        *olineptrs[1]++ = twoopixels;
        }
// proff 06/30/98: Changed form constant value to defined value
        ilineptr -= (SCREENWIDTH/2);
        olineptrs[0] += ViewMemPitch>>2;
        olineptrs[1] += ViewMemPitch>>2;
    }
}

#ifndef NOASM
void V_ScaleBy2Da(void);
void V_ScaleBy2Ua(void);
#endif //NOASM

void V_EndFrame (void)
{
#ifdef DIRECTX
    HRESULT error;
    DDSURFACEDESC ddSD;
    int y;
    static int SurfaceLost=2;
    int *Surface,*doommem;

    if ((vidFullScreen) & (lpDDPSF != NULL) & (lpDDBSF != NULL))
    {
        ddSD.dwSize = sizeof(ddSD);
        error = lpDDPSF->lpVtbl->GetSurfaceDesc(lpDDPSF,&ddSD);
        while (1)
        {
          error = lpDDPSF->lpVtbl->Lock(lpDDPSF,NULL,&ddSD,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT,NULL);
          if (error == DDERR_SURFACELOST)
          {
            SurfaceLost=2;
            lpDDPSF->lpVtbl->Restore(lpDDPSF);
            break;
          }
          if (error != DDERR_WASSTILLDRAWING)
            break;
        }
        if (error != DD_OK)
            return;
        if (SurfaceLost)
        {
            Surface = ddSD.lpSurface;
            for (y=0; (DWORD)y<ddSD.dwHeight; y++)
            {
                memset(Surface,0,ddSD.dwWidth);
#ifdef _MSC_VER
                Surface += ddSD.lPitch/4;
#else
                Surface += ddSD.u1.lPitch/4;
#endif
            }
            SurfaceLost--;
        }
        if (ViewScale==Scale_None)
        {
#ifdef _MSC_VER
            Surface = (int *)((char *)ddSD.lpSurface
                        +(((ddSD.dwHeight-SCREENHEIGHT)/2)*ddSD.lPitch)
                        +((ddSD.dwWidth-SCREENWIDTH)/2));
#else
            Surface = (int *)((char *)ddSD.lpSurface
                        +(((ddSD.dwHeight-SCREENHEIGHT)/2)*ddSD.u1.lPitch)
                        +((ddSD.dwWidth-SCREENWIDTH)/2));
#endif
            doommem = (int *)screens[0];
#ifdef _MSC_VER
            if (ddSD.lPitch==SCREENWIDTH)
#else
            if (ddSD.u1.lPitch==SCREENWIDTH)
#endif
              memcpy(Surface,doommem,SCREENWIDTH*SCREENHEIGHT);
            else
              for (y=0; y<SCREENHEIGHT; y++)
              {
                memcpy(Surface,doommem,SCREENWIDTH);
#ifdef _MSC_VER
                Surface += ddSD.lPitch/4;
#else
                Surface += ddSD.u1.lPitch/4;
#endif
                doommem += SCREENWIDTH/4;
              }
        }
        else
        {
#ifdef _MSC_VER
            ViewMemPitch = ddSD.lPitch;
            ScaledVMem =(char *)ddSD.lpSurface
                        +(((ddSD.dwHeight-(SCREENHEIGHT*2))/2)*ddSD.lPitch)
                        +((ddSD.dwWidth-(SCREENWIDTH*2))/2);
#else
            ViewMemPitch = ddSD.u1.lPitch;
            ScaledVMem =(char *)ddSD.lpSurface
                        +(((ddSD.dwHeight-(SCREENHEIGHT*2))/2)*ddSD.u1.lPitch)
                        +((ddSD.dwWidth-(SCREENWIDTH*2))/2);
#endif
#ifndef NOASM
            V_ScaleBy2Da();
#else
            V_ScaleBy2D();
#endif
        }
        error = lpDDPSF->lpVtbl->Unlock(lpDDPSF,ddSD.lpSurface);
        if (error != DD_OK)
        {
          lprintf(LO_DEBUG,"UnlockError\n");
            return;
        }
        if (page_flip)
        {
          error = lpDDPSF->lpVtbl->Flip(lpDDPSF,NULL,DDFLIP_WAIT);
          if (error != DD_OK)
            return;
        }
    }
    else
#endif
    {
        if (ViewScale==Scale_None)
        {
            if ( SetDIBitsToDevice(    ViewDC, 0, 0, SCREENWIDTH, SCREENHEIGHT,
                                    0, 0, 0, SCREENHEIGHT,
                                    screens[0], View_bmi, DIB_RGB_COLORS)
                == GDI_ERROR )
                I_Error("SetDIBitsToDevice failed");
        }
        else if (ViewScale==Scale_Windows)
        {
            if ( StretchDIBits(    ViewDC, 0, 0, SCREENWIDTH*2, SCREENHEIGHT*2,
                                0, 0, SCREENWIDTH, SCREENHEIGHT,
                                screens[0], View_bmi, DIB_RGB_COLORS, SRCCOPY)
                == GDI_ERROR )
                I_Error("StretchDIBits failed");
        }
        else if (ViewScale==Scale_Own)
        {
            ScaledVMem=ViewMem;
#ifndef NOASM
            V_ScaleBy2Ua();
#else
            V_ScaleBy2U();
#endif
            if ( SetDIBitsToDevice(    ViewDC, 0, 0, SCREENWIDTH*2, SCREENHEIGHT*2,
                                    0, 0, 0, SCREENHEIGHT*2,
                                    ScaledVMem, View_bmi, DIB_RGB_COLORS)
                == GDI_ERROR )
                I_Error("SetDIBitsToDevice failed");
        }
        GdiFlush();    
    }
}

void V_GetMessages (void)
{
  MSG msg;
  POINT point;
  static LONG prevX=0, prevY=0;
  static LONG prevDX=0, prevDY=0;
  static int hadMouse = 0;
  event_t event;
  RECT rectw;
#ifdef DIRECTX
  JOYINFO JoyInfo;
  int joyX,joyY;
#endif // DIRECTX

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
//    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

#ifdef DIRECTX
  if (usejoystick)
    if (joyGetPos(JOYSTICKID1,&JoyInfo)==JOYERR_NOERROR)
    {
      event.type = ev_joystick;
      event.data1 = 0;
      if (JoyInfo.wButtons&JOY_BUTTON1)
        event.data1 |= 1;
      if (JoyInfo.wButtons&JOY_BUTTON2)
        event.data1 |= 2;
      if (JoyInfo.wButtons&JOY_BUTTON3)
        event.data1 |= 4;
      if (JoyInfo.wButtons&JOY_BUTTON4)
        event.data1 |= 8;
      joyX=((int)JoyInfo.wXpos-32768)/2048;
      joyY=((int)JoyInfo.wYpos-32768)/2048;
      if (joyX < -4)
        event.data2 = -1;
      else if (joyX > 4)
        event.data2 = 1;
      else
        event.data2 = 0;
      if (joyY < -4)
        event.data3 = -1;
      else if (joyY > 4)
        event.data3 = 1;
      else
        event.data3 = 0;
      // post what you found
      D_PostEvent(&event);
    }
#endif // DIRECTX
    
  if (!noMouse)
  {
    if ( !GetCursorPos(&point) )
      I_Error("GetCursorPos() failed");
    if (hadMouse && fActive)
    {
      event.type = ev_mouse;
      event.data1 = MouseButtons;
      event.data2 = 0;
      event.data3 = 0;
      if ( (prevX != point.x) || (prevY != point.y) )
      {
        event.data2 = ((point.x - prevX)+prevDX)/2;
        event.data3 = ((prevY - point.y)+prevDY)/2;
        prevDX = (point.x - prevX);
        prevDY = (prevY - point.y);
        prevX = point.x;
        prevY = point.y;
      }
      D_PostEvent(&event);

      if ( grabMouse )
      {
        GetWindowRect(ghWnd, &rectw);
        prevX = (rectw.left + rectw.right) / 2;
        prevY = (rectw.top + rectw.bottom) / 2;
        if ( !SetCursorPos(prevX,prevY) )
          I_Error("SetCursorPos() failed");
      }
    }
    else
    {
      prevX = point.x;
      prevY = point.y;
      hadMouse = 1;
    }
  }
}
