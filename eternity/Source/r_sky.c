// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//
// DESCRIPTION:
//  Sky rendering. The DOOM sky is a texture map like any
//  wall, wrapping around. A 1024 columns equal 360 degrees.
//  The default sky map is 256 columns and repeats 4 times
//  on a 320 screen?
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_sky.c,v 1.6 1998/05/03 23:01:06 killough Exp $";

#include "doomstat.h"
#include "r_sky.h"
#include "r_data.h"
#include "p_info.h"
#include "w_wad.h"
#include "d_gi.h"

//
// sky mapping
//

int skyflatnum;
int sky2flatnum;  // haleyjd: number of F_SKY2 flat for Hexen-style skies
int skytexture;
int sky2texture;
int stretchsky;

fixed_t Sky1ColumnOffset, Sky2ColumnOffset;

//
// R_StartSky
//
// Called when the level starts to load the appropriate sky.
//
void R_StartSky(void)
{
   Sky1ColumnOffset = Sky2ColumnOffset = 0;

   // haleyjd 07/18/04: init moved to MapInfo
  
   // Set the sky map.
   // First thing, we have a dummy sky texture name,
   //  a flat. The data is in the WAD only because
   //  we look for an actual index, instead of simply
   //  setting one.

   skyflatnum  = R_FlatNumForName(SKYFLATNAME);   
   sky2flatnum = R_FlatNumForName(SKY2FLATNAME); // haleyjd

   // haleyjd 01/22/04: added error checking
   
   if((skytexture = R_TextureNumForName(LevelInfo.skyName)) == -1)
      I_Error("R_StartSky: bad sky texture '%s'\n", LevelInfo.skyName);
      
   if((sky2texture = R_TextureNumForName(LevelInfo.sky2Name)) == -1)
      I_Error("R_StartSky: bad sky2 texture '%s'\n", LevelInfo.sky2Name);
}

//
// Sky texture information hash table stuff
// haleyjd 08/30/02: I need to store information about sky textures
// for use in the renderer, because each sky texture must be
// rendered differently depending on its size.
//

// the sky texture hash table
skytexture_t *skytextures[NUMSKYCHAINS];

#define skytexturekey(a) ((a) % NUMSKYCHAINS)

//
// R_AddSkyTexture
//
// Constructs a skytexture_t and adds it to the hash table
//
static skytexture_t *R_AddSkyTexture(int texturenum)
{
   skytexture_t *newSky;
   texpatch_t *texpatch;
   patch_t wpatch;
   int i, count, p_height, key, t_height;

   count = textures[texturenum]->patchcount;

   texpatch = &(textures[texturenum]->patches[0]);

   // 02/11/04: get height of texture
   t_height = textures[texturenum]->height;
   p_height = 0;

   newSky = Z_Malloc(sizeof(skytexture_t), PU_STATIC, NULL);

   // find the tallest patch in the texture
   for(i = 0; i < count; i++, texpatch++)
   {
      W_ReadLumpHeader(texpatch->patch, &wpatch, sizeof(patch_t));
      wpatch.height = SHORT(wpatch.height);

      if(wpatch.height > p_height)
         p_height = wpatch.height;
   }

   // 02/11/04: only if patch height is greater than texture height
   // should we use it
   newSky->texturenum = texturenum;
   newSky->height = p_height > t_height ? p_height : t_height;
   
   if(newSky->height >= 200)
      newSky->texturemid = 200*FRACUNIT;
   else
      newSky->texturemid = 100*FRACUNIT;

   key = skytexturekey(texturenum);

   // use head insertion
   newSky->next = skytextures[key];
   skytextures[key] = newSky;

   return newSky;
}

//
// R_GetSkyTexture
//
// Looks for the specified skytexture_t with the given texturenum
// in the hash table. If it doesn't exist, it'll be created now.
// 
skytexture_t *R_GetSkyTexture(int texturenum)
{
   int key;
   skytexture_t *target = NULL;

   key = skytexturekey(texturenum);

   if(skytextures[key])
   {
      // search in chain
      skytexture_t *rover = skytextures[key];

      while(rover)
      {
         if(rover->texturenum == texturenum)
         {
            target = rover;
            break;
         }

         rover = rover->next;
      }
   }

   return target ? target : R_AddSkyTexture(texturenum);
}

//
// R_ClearSkyTextures
//
// Must be called from R_InitData to clear out old texture numbers.
// Otherwise the data will be corrupt and meaningless.
//
void R_ClearSkyTextures(void)
{
   int i;

   for(i = 0; i < NUMSKYCHAINS; ++i)
   {
      if(skytextures[i])
      {
         skytexture_t *next;
         skytexture_t *rover = skytextures[i];

         while(rover)
         {
            next = rover->next;

            Z_Free(rover);

            rover = next;
         }
      }

      skytextures[i] = NULL;
   }
}

unsigned char egg[] =
{247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,14,14,15,79,79,79,15,15,78,110,111,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,
247,247,247,247,247,247,247,247,247,247,14,143,14,14,15,15,15,79,79,110,110,5,5,5,111,14,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,14,14,13,14,14,14,14,14,14,78,108,108,111,5,5,5,109,14,238,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,15,14,15,14,15,9,14,14,14,78,15,13,14,109,110,5,5,110,14,14,14,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,78,15,15,14,13,78,79,79,79,15,14,15,15,15,15,109,111,5,5,109,14,13,14,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,79,111,9,110,109,9,110,5,111,109,14,15,15,15,110,110,109,5,5,111,110,110,9,109,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,79,5,5,111,111,110,109,110,5,111,111,110,110,110,110,110,109,109,111,5,110,5,5,5,5,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,111,5,5,5,5,5,111,110,5,5,5,5,5,5,5,111,111,111,111,111,5,111,5,5,5,5,5,247,247,247,247,247,247,247,
247,247,247, /*** you ain't seen me, right ? ****/ 247,247,247,247,247,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,111,5,5,5,5,247,247,247,247,247,247,247,247,247,247,247,247,247,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,111,5,5,5,5,5,247,247,247,247,247,247,247,247,247,247,247,111,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,247,247,247,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,247,247,111,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,247,247,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,247,5,5,5,5,5,5,5,5,5,5,5,5,237,236,150,14,14,142,13,15,238,79,5,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,79,5,5,5,5,5,5,5,5,5,79,236,73,72,71,139,138,136,137,139,73,236,239,5,5,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,79,239,239,5,5,5,5,5,5,5,14,148,73,73,73,73,139,137,137,138,147,140,143,15,1,5,5,5,5,5,5,5,5,
5,5,247,247,247,247,247,247,247,239,237,13,239,5,5,5,5,5,15,76,73,73,237,238,238,238,238,14,149,141,73,74,236,238,5,5,5,5,5,5,5,5,5,5,247,247,247,247,247,247,247,239,14,140,13,239,5,1,5,142,138,73,73,72,75,238,239,1,1,2,239,238,76,76,76,237,15,79,5,6,5,5,5,5,5,5,247,247,247,247,247,247,247,79,15,136,138,237,239,239,76,139,69,71,72,70,147,149,13,79,1,2,2,2,239,238,238,237,237,15,79,1,5,5,5,5,5,5,247,247,247,247,247,247,247,239,1,139,139,75,76,150,73,71,69,69,70,69,71,148,14,239,2,2,2,1,239,239,238,239,239,79,1,5,6,5,5,5,5,5,247,247,247,247,247,247,247,2,2,15,139,73,74,73,72,70,69,69,70,69,70,69,73,237,239,239,239,238,15,14,238,1,2,2,6,6,6,6,5,5,5,247,247,247,247,247,247,247,247,2,2,1,238,76,73,71,71,70,69,70,70,69,70,146,69,72,74,149,236,150,73,140,238,2,2,6,6,6,7,5,5,5,5,247,247,247,247,247,247,247,247,7,2,2,2,15,72,70,70,70,70,70,70,69,70,70,69,70,71,71,72,74,72,140,238,2,6,6,6,6,6,5,6,6,247,247,247,247,247,247,247,247,6,2,6,2,1,15,72,70,69,70,70,70,70,71,71,72,73,73,73,71,71,70,134,139,239,2,2,7,7,7,7,6,
6,247,247,247,247,247,247,247,247,6,6,6,2,2,238,150,72,70,70,70,71,71,71,72,73,74,76,44,76,71,145,132,132,138,238,79,1,2,2,6,6,247,247,247,247,247,247,247,247,247,6,6,2,2,1,15,14,75,73,72,71,71,72,72,72,73,74,76,237,237,76,71,135,131,131,147,238,238,78,79,1,79,247,247,247,247,247,247,247,247,247,247,6,2,1,79,15,143,74,75,74,73,72,72,72,73,73,74,75,76,74,73,237,238,76,138,135,139,15,15,15,78,78,247,247,247,247,247,247,247,247,247,247,247,6,2,79,142,141,73,74,76,237,75,74,73,73,73,74,74,76,76,73,148,236,239,239,239,15,238,239,239,238,78,247,247,247,247,247,247,247,247,247,247,247,247,2,239,141,139,148,73,74,237,239,15,76,74,74,73,74,237,239,239,76,140,141,15,1,2,2,1,1,79,79,238,247,247,247,247,247,247,247,247,247,247,247,247,1,142,138,147,72,74,150,238,239,239,237,150,74,74,75,76,76,76,15,238,238,238,1,6,6,5,1,1,239,247,247,247,247,247,247,247,247,247,247,247,247,247,13,136,137,147,73,75,15,238,1,1,239,237,75,74,74,73,71,71,73,76,239,2,6,6,6,5,1,1,247,247,247,247,247,247,247,247,247,247,247,247,247,247,136,
135,146,139,149,76,15,79,1,2,1,239,76,74,72,71,70,71,74,237,238,1,2,7,6,6,2,79,247,247,247,247,247,247,247,247,247,247,247,247,247,247,136,146,147,73,150,237,78,79,1,2,2,1,238,75,71,69,69,69,73,15,239,1,2,6,6,6,1,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,137,137,139,74,151,77,79,79,1,6,6,2,1,238,74,147,145,145,146,73,237,239,1,6,6,6,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,140,139,149,76,15,238,79,1,1,6,6,2,2,1,238,236,139,137,147,71,73,237,79,6,7,6,2,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,238,142,13,15,238,79,79,1,6,6,2,6,6,2,2,239,238,237,150,150,236,15,1,7,7,6,6,2,247,247,247,247,247,247,247,247,247,247,247,247,247,247,2,111,15,78,79,1,1,6,6,6,6,6,6,6,6,2,2,1,1,1,1,1,2,6,6,6,6,2,5,247,247,247,247,247,247,247,247,247,247,247,247,247,6,2,1,79,1,1,6,6,6,6,6,6,6,6,6,6,6,6,2,2,2,2,6,6,6,6,6,6,2,247,247,247,247,247,247,247,247,247,247,247,247,247};


//----------------------------------------------------------------------------
//
// $Log: r_sky.c,v $
// Revision 1.6  1998/05/03  23:01:06  killough
// beautification
//
// Revision 1.5  1998/05/01  14:14:24  killough
// beautification
//
// Revision 1.4  1998/02/05  12:14:31  phares
// removed dummy comment
//
// Revision 1.3  1998/01/26  19:24:49  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/19  16:17:59  rand
// Added dummy line to be removed later.
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
