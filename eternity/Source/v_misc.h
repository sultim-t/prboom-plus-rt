// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------

#ifndef __V_MISC_H__
#define __V_MISC_H__

#include "v_patch.h"
#include "v_block.h"

void V_InitMisc(void);

/////////////////////////////////////////////////////////////////////////////
//
// Mode setting
//

void V_Mode(void);
void V_ModeList(void);
void V_ResetMode(void);
int  V_NumModes(void); // haleyjd: made global
extern int v_mode;

/////////////////////////////////////////////////////////////////////////////
//
// Font
//

#define V_FONTSTART    '!'     // the first font character
#define V_FONTEND      (0x7f) // jff 2/16/98 '_' the last font characters
// Calculate # of glyphs in font.
#define V_FONTSIZE     (V_FONTEND - V_FONTSTART + 1) 

extern patch_t *v_font[V_FONTSIZE];

        // font colours
#define FC_BRICK        "\x80"
#define FC_TAN          "\x81"
#define FC_GRAY         "\x82"
#define FC_GREEN        "\x83"
#define FC_BROWN        "\x84"
#define FC_GOLD         "\x85"
#define FC_RED          "\x86"
#define FC_BLUE         "\x87"
#define FC_ORANGE       "\x88"
#define FC_YELLOW       "\x89"
// haleyjd: translucent text support
#define FC_TRANS	"\x8a"
// haleyjd 08/20/02: new characters for internal color usage
#define FC_NORMAL       "\x8b"
#define FC_HI           "\x8c"
#define FC_ERROR        "\x8d"

void V_WriteText(const char *s, int x, int y);
void V_WriteTextColoured(const char *s, int colour, int x, int y);
int  V_StringWidth(const unsigned char *s);
int  V_StringHeight(const unsigned char *s);

void V_WriteTextBig(const char *s, int x, int y);
void V_WriteNumTextBig(const char *s, int x, int y);
int  V_StringWidthBig(const unsigned char *s);
int  V_StringHeightBig(const unsigned char *s);

///////////////////////////////////////////////////////////////////////////
//
// Box Drawing
//

void V_DrawBox(int, int, int, int);

///////////////////////////////////////////////////////////////////////////
//
// Loading box
//

void V_DrawLoading();
void V_SetLoading(int total, char *mess);
void V_LoadingIncrease();
void V_LoadingSetTo(int amount);

///////////////////////////////////////////////////////////////////////////
//
// FPS ticker
//

void V_FPSDrawer();
void V_FPSTicker();
extern int v_ticker;

///////////////////////////////////////////////////////////////////////////
//
// Background 'tile' fill
//

void V_DrawBackground(char *patchname, VBuffer *back_dest);
void V_DrawDistortedBackground(char* patchname, VBuffer *back_dest);

#endif

// EOF
