//--------------------------------------------------------------------------
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

#ifndef __P_SKIN_H__
#define __P_SKIN_H__

typedef struct skin_s skin_t;

#include "info.h"
#include "sounds.h"
#include "st_stuff.h"
#include "r_defs.h"

enum
{
   sk_plpain,
   sk_pdiehi,
   sk_oof,
   sk_slop,
   sk_punch,
   sk_radio,
   sk_pldeth,
   sk_plfall, // haleyjd: new sounds -- breaks compatibility
   sk_plfeet,
   sk_fallht,
   NUMSKINSOUNDS
};

// haleyjd 09/26/04: skin type enumeration

typedef enum
{
   SKIN_PLAYER,
   SKIN_MONSTER,
   SKIN_NUMTYPES
} skintype_e;

struct skin_s
{
   skintype_e  type;          // haleyjd: type of skin
   char        *spritename;   // 4 chars
   char        *skinname;     // name of the skin: eg 'marine'
   spritenum_t sprite;        // set by initskins
   char        *sounds[NUMSKINSOUNDS];
   char        *facename;     // statusbar face
   patch_t     **faces;
};

extern skin_t marine;
extern skin_t **skins;
extern char **spritelist;       // new spritelist, same format as sprnames
        // in info.c, but includes skins sprites.

void P_InitSkins(void);

void P_ListSkins(void);
void P_ChangeSkin(void);
void P_ParseSkin(int lumpnum);
void P_SetSkin(skin_t *skin, int playernum);

skin_t *P_GetMonsterSkin(spritenum_t sprnum);

#endif

// EOF
