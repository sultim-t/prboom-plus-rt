/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *  Provides a logical console output routine that allows what is
 *  output to console normally and when output is redirected to
 *  be controlled..
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef _MSC_VER
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "doomtype.h"
#include "lprintf.h"
#include "i_main.h"
#include "m_argv.h"

int cons_error_mask = -1-LO_INFO; /* all but LO_INFO when redir'd */
int cons_output_mask = -1;        /* all output enabled */

/* cphipps - enlarged message buffer and made non-static
 * We still have to be careful here, this function can be called after exit
 */
#define MAX_MESSAGE_SIZE 2048

#ifdef _WIN32
// Variables for the console
HWND con_hWnd=0;
HFONT OemFont;
LONG OemWidth, OemHeight;
int ConWidth,ConHeight;
char szConName[] = "PrBoomConWinClass";
char Lines[(80+2)*25+1];
char *Last = NULL;
boolean console_inited=FALSE;
static boolean should_exit = 0;

static CALLBACK ConWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT paint;
  HDC dc;

  switch (iMsg) {
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE)
      should_exit = 1;
    break;
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
    break;
  }
  return(DefWindowProc(hwnd,iMsg,wParam,lParam));
}

static void I_PrintStr (int xp, const char *cp, int count, BOOL scroll) {
  RECT rect;
  HDC conDC;

  if ((!con_hWnd) || (!console_inited))
    return;
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

static int I_ConPrintString (const char *outline)
{
  const char *cp, *newcp;
  static int xp = 0;
  int newxp;
  BOOL scroll;

  if (!console_inited)
    return 0;
  cp = outline;
  while (*cp) {
    for (newcp = cp, newxp = xp;
      *newcp != '\n' && *newcp != '\0' && newxp < 80;
       newcp++) {
      if (*newcp == '\x08') {
        newxp--;
        break;
      }
      else if (*newcp == '\t') {
        newxp = ((newxp + 8) / 8) * 8;
        break;
      }
      else
        newxp++;
    }

    if (*cp) {
      const char *poop;
      int x;

      for (x = xp, poop = cp; poop < newcp; poop++, x++) {
        Last[x+2] = ((*poop) < 32) ? 32 : (*poop);
      }

      if (*newcp == '\t')
        for (x = xp; x < newxp; x++)
          Last[x+2] = ' ';

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
        scroll = TRUE;
      } else {
        scroll = FALSE;
      }
      I_PrintStr (xp, cp, newcp - cp, scroll);

      xp = newxp;

      if ((*newcp == '\n') || (*newcp == '\x08') || (*newcp == '\t'))
        cp = newcp + 1;
      else
        cp = newcp;
    }
  }

  return strlen (outline);
}

void I_ConTextAttr(unsigned char a)
{
  int r,g,b,col;
  HDC conDC;

  if (!console_inited)
    return;
  conDC=GetDC(con_hWnd);
  r=0; g=0; b=0;
  if (a & FOREGROUND_INTENSITY) col=255;
  else col=128;
  if (a & FOREGROUND_RED) r=col;
  if (a & FOREGROUND_GREEN) g=col;
  if (a & FOREGROUND_BLUE) b=col;
  SetTextColor(conDC, PALETTERGB(r,g,b));
  r=0; g=0; b=0;
  if (a & BACKGROUND_INTENSITY) col=255;
  else col=128;
  if (a & BACKGROUND_RED) r=col;
  if (a & BACKGROUND_GREEN) g=col;
  if (a & BACKGROUND_BLUE) b=col;
  SetBkColor(conDC, PALETTERGB(r,g,b));
  ReleaseDC(con_hWnd,conDC);
}

void I_UpdateConsole(void)
{
  MSG msg;

  UpdateWindow(con_hWnd);
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (should_exit)
    exit(0);
}

static void Init_Console(void)
{
  memset(Lines,0,25*(80+2)+1);
  Last = Lines + (25 - 1) * (80 + 2);
  console_inited=TRUE;
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
    char titlebuffer[2048];

    if (con_hWnd)
      return TRUE;
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
    strcpy(titlebuffer,PACKAGE);
    strcat(titlebuffer," ");
    strcat(titlebuffer,VERSION);
    strcat(titlebuffer," console");
    con_hWnd = CreateWindow(szConName, titlebuffer,
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
    return TRUE;
}

void Done_ConsoleWin(void)
{
  if (con_hWnd)
    DestroyWindow(con_hWnd);
  UnregisterClass(szConName,GetModuleHandle(NULL));
  con_hWnd=0;
}
#endif

int lprintf(OutputLevels pri, const char *s, ...)
{
  int r=0;
  char msg[MAX_MESSAGE_SIZE];
  int lvl=pri;

  va_list v;
  va_start(v,s);
#ifdef HAVE_VSNPRINTF
  vsnprintf(msg,sizeof(msg),s,v);         /* print message in buffer  */
#else
  vsprintf(msg,s,v);
#endif
  va_end(v);

  if (lvl&cons_output_mask)               /* mask output as specified */
  {
    r=fprintf(stdout,"%s",msg);
#ifdef _WIN32
    I_ConPrintString(msg);
#endif
  }
  if (!isatty(1) && lvl&cons_error_mask)  /* if stdout redirected     */
    r=fprintf(stderr,"%s",msg);           /* select output at console */

  return r;
}

/*
 * I_Error
 *
 * cphipps - moved out of i_* headers, to minimise source files that depend on
 * the low-level headers. All this does is print the error, then call the
 * low-level safe exit function.
 * killough 3/20/98: add const
 */

void I_Error(const char *error, ...)
{
  char errmsg[MAX_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr,error);
#ifdef HAVE_VSNPRINTF
  vsnprintf(errmsg,sizeof(errmsg),error,argptr);
#else
  vsprintf(errmsg,error,argptr);
#endif
  va_end(argptr);
  lprintf(LO_ERROR, "%s\n", errmsg);
#ifdef _MSC_VER
  if (!M_CheckParm ("-nodraw")) {
    //Init_ConsoleWin();
    MessageBox(con_hWnd,errmsg,"PrBoom",MB_OK | MB_TASKMODAL | MB_TOPMOST);
  }
#endif
  I_SafeExit(-1);
}
