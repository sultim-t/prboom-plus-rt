// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: gl_struct.h,v 1.3 2000/05/09 20:49:32 proff_fs Exp $
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

#ifndef _GL_STRUCT_H
#define _GL_STRUCT_H

#include "doomtype.h"
#include "r_data.h"

void gld_Init(int width, int height);
void gld_InitCommandLine();

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags);
void gld_DrawPatchFromMem(int x, int y, const patch_t *patch, int cm, enum patch_translation_e flags);
void gld_DrawBackground(const char* name);
void gld_DrawLine(int x0, int y0, int x1, int y1, byte BaseColor);
void gld_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel);
void gld_FillBlock(int x, int y, int width, int height, int col);
void gld_SetPalette(int palette);

void gld_CleanMemory(void);
void gld_PreprocessLevel(void);

void gld_AddSpriteToRenderQueue(mobj_t *pSpr,int lump, boolean flip);

void gld_Set2DMode();
void gld_InitDrawScene(void);
void gld_StartDrawScene(void);
void gld_DrawScene(player_t *player);
void gld_EndDrawScene(void);
void gld_Finish();

#endif // _GL_STRUCT_H

//-----------------------------------------------------------------------------
//
// $Log: gl_struct.h,v $
// Revision 1.3  2000/05/09 20:49:32  proff_fs
// reorganised the gl-stuff a little bit and made it ready for Linux
//
// Revision 1.2  2000/05/07 20:19:33  proff_fs
// changed use of colormaps from pointers to numbers.
// That's needed for OpenGL.
// The OpenGL part is slightly better now.
// Added some typedefs to reduce warnings in VisualC.
// Messages are also scaled now, because at 800x600 and
// above you can't read them even on a 21" monitor.
//
// Revision 1.1  2000/05/04 16:40:00  proff_fs
// added OpenGL stuff. Not complete yet.
// Only the playerview is rendered.
// The normal output is displayed in a small window.
// The level is only drawn in debugmode to the window.
//
// Revision 1.2  2000/04/10 21:13:13  proff_fs
// added Log to OpenGL files
//
// Revision 1.1.1.1  2000/04/09 18:21:44  proff_fs
// Initial login
//
//-----------------------------------------------------------------------------
