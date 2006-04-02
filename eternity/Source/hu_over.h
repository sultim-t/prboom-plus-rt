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

#ifndef __HU_OVER_H__
#define __HU_OVER_H__

    /*************** heads up font **************/

void HU_LoadFont(void);
void HU_WriteText(const char *s, int x, int y);
int  HU_StringWidth(const unsigned char *s);
int  HU_StringHeight(const unsigned char *s);

    /************** overlay drawing ***************/

typedef struct overlay_s overlay_t;

struct overlay_s
{
  int x, y;
  void (*drawer)(int x, int y);
};

enum
{
  ol_health,
  ol_ammo,
  ol_armor,
  ol_weap,
  ol_key,
  ol_frag,
  ol_status,
  NUMOVERLAY
};

extern overlay_t overlay[NUMOVERLAY];

void HU_OverlayDraw(void);
void HU_ToggleHUD(void);
void HU_DisableHUD(void);

extern int hud_overlaystyle;
extern int hud_enabled;
extern int hud_hidestatus;

#endif

// EOF
