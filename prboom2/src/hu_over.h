/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __HU_OVER_H__
#define __HU_OVER_H__

//---------------------------------------------------------------------------
//
// Heads up font
//

// copied from v_misc.h
#define HU_FONTSTART    '!'     /* the first font characters */
#define HU_FONTEND      (0x7f) /*jff 2/16/98 '_' the last font characters */

// Calculate # of glyphs in font.
#define HU_FONTSIZE     (HU_FONTEND - HU_FONTSTART + 1) 

void HU_LoadFont();
void HU_WriteText(unsigned char *s, int x, int y);
void HU_WriteTextColoured(unsigned char *s, int colour, int x, int y);
int HU_StringWidth(unsigned char *s);

//--------------------------------------------------------------------------
//
// Overlay drawing
//

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

void HU_OverlayDraw();
void HU_OverlayStyle();
void HU_ToggleHUD();

#endif
