/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2002 by
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
 *-----------------------------------------------------------------------------*/

/*
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
*/

#include "z_zone.h"
#include "doomstat.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_bsp.h"
#include "r_things.h"
#include "p_tick.h"
#include "i_system.h"
#include "r_draw.h"
#include "lprintf.h"
#include "r_patch.h"
#include "v_video.h"
#include <assert.h>

// posts are runs of non masked source pixels
typedef struct
{
  byte topdelta; // -1 is the last post in a column
  byte length;   // length data bytes follows
} post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;

//
// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//

typedef struct
{
  short width, height;  // bounding box size
  short leftoffset;     // pixels to the left of origin
  short topoffset;      // pixels below the origin
  int columnofs[8];     // only [width] used
} patch_t;

//---------------------------------------------------------------------------
// Re-engineered patch support
//---------------------------------------------------------------------------
static rpatch_t *patches = 0;

static rpatch_t *texture_composites = 0;

// indices of two duplicate PLAYPAL entries, second is -1 if none found
static int playpal_transparent, playpal_duplicate;

//---------------------------------------------------------------------------
void R_InitPatches(void) {
  if (!patches)
  {
    patches = malloc(numlumps * sizeof(rpatch_t));
    // clear out new patches to signal they're uninitialized
    memset(patches, 0, sizeof(rpatch_t)*numlumps);
  }
  if (!texture_composites)
  {
    texture_composites = malloc(numtextures * sizeof(rpatch_t));
    // clear out new patches to signal they're uninitialized
    memset(texture_composites, 0, sizeof(rpatch_t)*numtextures);
  }

  if (!playpal_duplicate)
  {
    int lump = W_GetNumForName("PLAYPAL");
    const byte *playpal = W_CacheLumpNum(lump);

    // find two duplicate palette entries. use one for transparency.
    // rewrite source pixels in patches to the other on composition.

    int i, j, found = 0;

    for (i = 0; i < 256; i++)
    {
      for (j = i+1; j < 256; j++)
      {
        if (playpal[3*i+0] == playpal[3*j+0] &&
            playpal[3*i+1] == playpal[3*j+1] &&
            playpal[3*i+2] == playpal[3*j+2])
        {
          found = 1;
          break;
        }
      }
      if (found)
        break;
    }

    if (found) { // found duplicate
      playpal_transparent = i;
      playpal_duplicate   = j;
    } else { // no duplicate: use 255 for transparency, as done previously
      playpal_transparent = 255;
      playpal_duplicate   = -1;
    }

    W_UnlockLumpNum(lump);
  }
}

//---------------------------------------------------------------------------
void R_FlushAllPatches(void) {
  int i;

  if (patches)
  {
    for (i=0; i < numlumps; i++)
      if (patches[i].locks > 0)
        I_Error("R_FlushAllPatches: patch number %i still locked",i);
    free(patches);
    patches = NULL;
  }
  if (texture_composites)
  {
    for (i=0; i<numtextures; i++)
      if (texture_composites[i].data)
        free(texture_composites[i].data);
    free(texture_composites);
    texture_composites = NULL;
  }
}

//---------------------------------------------------------------------------
int R_NumPatchWidth(int lump)
{
  const rpatch_t *patch = R_CachePatchNum(lump);
  int width = patch->width;
  R_UnlockPatchNum(lump);
  return width;
}

//---------------------------------------------------------------------------
int R_NumPatchHeight(int lump)
{
  const rpatch_t *patch = R_CachePatchNum(lump);
  int height = patch->height;
  R_UnlockPatchNum(lump);
  return height;
}

//---------------------------------------------------------------------------
static int getPatchIsNotTileable(const patch_t *patch) {
  int x=0, numPosts, lastColumnDelta = 0;
  const column_t *column;
  int cornerCount = 0;
  int hasAHole = 0;

  for (x=0; x<LittleShort(patch->width); x++) {
    column = (const column_t *)((const byte *)patch + LittleLong(patch->columnofs[x]));
    if (!x) lastColumnDelta = column->topdelta;
    else if (lastColumnDelta != column->topdelta) hasAHole = 1;

    numPosts = 0;
    while (column->topdelta != 0xff) {
      // check to see if a corner pixel filled
      if (x == 0 && column->topdelta == 0) cornerCount++;
      else if (x == 0 && column->topdelta + column->length >= LittleShort(patch->height)) cornerCount++;
      else if (x == LittleShort(patch->width)-1 && column->topdelta == 0) cornerCount++;
      else if (x == LittleShort(patch->width)-1 && column->topdelta + column->length >= LittleShort(patch->height)) cornerCount++;

      if (numPosts++) hasAHole = 1;
      column = (const column_t *)((const byte *)column + column->length + 4);
    }
  }

  if (cornerCount == 4) return 0;
  return hasAHole;
}

//---------------------------------------------------------------------------
static int getIsSolidAtSpot(const column_t *column, int spot) {
  if (!column) return 0;
  while (column->topdelta != 0xff) {
    if (spot < column->topdelta) return 0;
    if ((spot >= column->topdelta) && (spot <= column->topdelta + column->length)) return 1;
    column = (const column_t*)((const byte*)column + 3 + column->length + 1);
  }
  return 0;
}

//---------------------------------------------------------------------------
// Used to determine whether a column edge (top or bottom) should slope
// up or down for smoothed masked edges - POPE
//---------------------------------------------------------------------------
static int getColumnEdgeSlope(const column_t *prevcolumn, const column_t *nextcolumn, int spot) {
  int holeToLeft = !getIsSolidAtSpot(prevcolumn, spot);
  int holeToRight = !getIsSolidAtSpot(nextcolumn, spot);

  if (holeToLeft && !holeToRight) return 1;
  if (!holeToLeft && holeToRight) return -1;
  return 0;
}

//---------------------------------------------------------------------------
static void FillEmptySpace(rpatch_t *patch)
{
  int x, y, w, h, numpix, pass, transparent, has_holes;
  byte *orig, *copy, *src, *dest, *prev, *next;

  // loop over patch looking for transparent pixels next to solid ones
  // copy solid pixels into the spaces, dilating the patch outwards
  // repeat either a fixed number of times or until all space is filled
  // this eliminates artifacts surrounding weapon sprites (e.g. chainsaw)

  w = patch->width;
  h = patch->height;
  numpix = w * h;

  // alternate between two buffers to avoid "overlapping memcpy"-like symptoms
  orig = patch->pixels;
  copy = malloc(numpix);

  for (pass = 0; pass < 8; pass++) // arbitrarily chosen limit (must be even)
  {
    // copy src to dest, then switch them on next pass
    // (this requires an even number of passes)
    src  = ((pass & 1) == 0) ? orig : copy;
    dest = ((pass & 1) == 0) ? copy : orig;

    // previous and next columns adjacent to current column x
    // these pointers must not be dereferenced without appropriate checks
    prev = src - h; // only valid when x > 0
    next = src + h; // only valid when x < w-1

    has_holes = 0; // if the patch has any holes at all
    transparent = 0; // number of pixels this pass did not handle

    // detect transparent pixels on edges, copy solid colour into the space
    // the order of directions (up,down,left,right) is arbitrarily chosen
    for (x = 0; x < w; x++)
    {
      for (y = 0; y < h; y++)
      {
        if (*src == playpal_transparent) has_holes = 1;

        if (*src != playpal_transparent)
          *dest = *src; // already a solid pixel, just copy it over
        else if (y > 0 && *(src-1) != playpal_transparent)
          *dest = *(src - 1); // solid pixel above
        else if (y < h-1 && *(src+1) != playpal_transparent)
          *dest = *(src + 1); // solid pixel below
        else if (x > 0 && *prev != playpal_transparent)
          *dest = *prev; // solid pixel to left
        else if (x < w-1 && *next != playpal_transparent)
          *dest = *next; // solid pixel to right
        else // transparent pixel with no adjacent solid pixels
          *dest = *src, transparent++; // count unhandled pixels

        prev++, src++, next++, dest++;
      }
    }

    if (transparent == 0) // no more transparent pixels to fill
    {
      if ((pass & 1) == 0) // dest was copy, src was orig: orig needs update
        memcpy(orig, copy, numpix);
      break;
    }
    else if (transparent == numpix)
      break; // avoid infinite loop on entirely transparent patches (STBR127)
  }

  free(copy);

  // copy top row of patch into any space at bottom, and vice versa
  // a hack to fix erroneous row of pixels at top of firing chaingun

  for (x = 0, src = orig, dest = src + h-1; x < w; x++, src += h, dest += h)
  {
    if (*src != playpal_transparent && *dest == playpal_transparent)
      *dest = *src; // bottom transparent, top solid
    else if (*src == playpal_transparent && *dest != playpal_transparent)
      *src = *dest; // top transparent, bottom solid
  }

  if (has_holes)
    patch->flags |= PATCH_HASHOLES;
}

//==========================================================================
//
// Checks if the lump can be a Doom patch
//
//==========================================================================

static dboolean CheckIfPatch(int lump)
{
  int size;
  int width, height;
  const patch_t * patch;
  dboolean result;

  size = W_LumpLength(lump);
  
  // minimum length of a valid Doom patch
  if (size < 13)
    return false;

  patch = (const patch_t *)W_CacheLumpNum(lump);

  width = LittleShort(patch->width);
  height = LittleShort(patch->height);

  result = (height > 0 && height <= 16384 && width > 0 && width <= 16384 && width < size / 4);

  if (result)
  {
    // The dimensions seem like they might be valid for a patch, so
    // check the column directory for extra security. All columns 
    // must begin after the column directory, and none of them must
    // point past the end of the patch.
    int x;

    for (x = 0; x < width; x++)
    {
      unsigned int ofs = LittleLong(patch->columnofs[x]);

      // Need one byte for an empty column (but there's patches that don't know that!)
      if (ofs < (unsigned int)width * 4 + 8 || ofs >= (unsigned int)size)
      {
        result = false;
        break;
      }
    }
  }

  W_UnlockLumpNum(lump);
  return result;
}

//---------------------------------------------------------------------------
static void StorePixel(rpatch_t *patch, int x, int y, byte color)
{
  // write pixel to patch, substituting for playpal_transparent as needed
  if (color == playpal_transparent && playpal_duplicate >= 0)
    color = playpal_duplicate;
  patch->pixels[x * patch->height + y] = color;
}

//---------------------------------------------------------------------------
static void createPatch(int id) {
  rpatch_t *patch;
  const int patchNum = id;
  const patch_t *oldPatch;
  const column_t *oldColumn, *oldPrevColumn, *oldNextColumn;
  int x, y;
  int pixelDataSize;
  int columnsDataSize;
  int postsDataSize;
  int dataSize;
  int *numPostsInColumn;
  int numPostsTotal;
  const unsigned char *oldColumnPixelData;
  int numPostsUsedSoFar;
  int edgeSlope;

#ifdef RANGECHECK
  if (id >= numlumps)
    I_Error("createPatch: %i >= numlumps", id);
#endif

  if (!CheckIfPatch(patchNum))
  {
    I_Error("createPatch: Unknown patch format %s.",
      (patchNum < numlumps ? lumpinfo[patchNum].name : NULL));
  }

  oldPatch = (const patch_t*)W_CacheLumpNum(patchNum);

  patch = &patches[id];
  // proff - 2003-02-16 What about endianess?
  patch->width = LittleShort(oldPatch->width);
  patch->widthmask = 0;
  patch->height = LittleShort(oldPatch->height);
  patch->leftoffset = LittleShort(oldPatch->leftoffset);
  patch->topoffset = LittleShort(oldPatch->topoffset);
  patch->flags = 0;
  if (getPatchIsNotTileable(oldPatch))
    patch->flags |= PATCH_ISNOTTILEABLE;

#ifdef GL_DOOM
  // Width of M_THERMM patch is 9, but Doom interprets it as 8-columns lump
  // during drawing. It is not a problem for software mode and GL_NEAREST,
  // but looks wrong with filtering. So I need to patch it during loading.
  if (V_GetMode() == VID_MODEGL)
  {
    if (!strncasecmp(lumpinfo[id].name, "M_THERMM", 8) && patch->width > 8)
    {
      patch->width--;
    }
  }
#endif

  // work out how much memory we need to allocate for this patch's data
  pixelDataSize = (patch->width * patch->height + 4) & ~3;
  columnsDataSize = sizeof(rcolumn_t) * patch->width;

  // count the number of posts in each column
  numPostsInColumn = malloc(sizeof(int) * patch->width);
  numPostsTotal = 0;

  for (x=0; x<patch->width; x++) {
    oldColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x]));
    numPostsInColumn[x] = 0;
    while (oldColumn->topdelta != 0xff) {
      numPostsInColumn[x]++;
      numPostsTotal++;
      oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
    }
  }

  postsDataSize = numPostsTotal * sizeof(rpost_t);

  // allocate our data chunk
  dataSize = pixelDataSize + columnsDataSize + postsDataSize;
  patch->data = (unsigned char*)Z_Malloc(dataSize, PU_CACHE, (void **)&patch->data);
  memset(patch->data, 0, dataSize);

  // set out pixel, column, and post pointers into our data array
  patch->pixels = patch->data;
  patch->columns = (rcolumn_t*)((unsigned char*)patch->pixels + pixelDataSize);
  patch->posts = (rpost_t*)((unsigned char*)patch->columns + columnsDataSize);

  // sanity check that we've got all the memory allocated we need
  assert((((byte*)patch->posts  + numPostsTotal*sizeof(rpost_t)) - (byte*)patch->data) == dataSize);

  if (playpal_transparent != 0)
    memset(patch->pixels, playpal_transparent, (patch->width*patch->height));

  // fill in the pixels, posts, and columns
  numPostsUsedSoFar = 0;
  for (x=0; x<patch->width; x++) {
    int top = -1;

    oldColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x]));

    if (patch->flags&PATCH_ISNOTTILEABLE) {
      // non-tiling
      if (x == 0) oldPrevColumn = 0;
      else oldPrevColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x-1]));
      if (x == patch->width-1) oldNextColumn = 0;
      else oldNextColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x+1]));
    }
    else {
      // tiling
      int prevColumnIndex = x-1;
      int nextColumnIndex = x+1;
      while (prevColumnIndex < 0) prevColumnIndex += patch->width;
      while (nextColumnIndex >= patch->width) nextColumnIndex -= patch->width;
      oldPrevColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[prevColumnIndex]));
      oldNextColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[nextColumnIndex]));
    }

    // setup the column's data
    patch->columns[x].pixels = patch->pixels + (x*patch->height) + 0;
    patch->columns[x].numPosts = numPostsInColumn[x];
    patch->columns[x].posts = patch->posts + numPostsUsedSoFar;

    while (oldColumn->topdelta != 0xff) {
      int len = oldColumn->length;

      //e6y: support for DeePsea's true tall patches
      if (oldColumn->topdelta <= top)
      {
        top += oldColumn->topdelta;
      }
      else
      {
        top = oldColumn->topdelta;
      }

      // Clip posts that extend past the bottom
      if (top + oldColumn->length > patch->height)
      {
        len = patch->height - top;
      }

      if (len > 0)
      {
        // set up the post's data
        patch->posts[numPostsUsedSoFar].topdelta = top;
        patch->posts[numPostsUsedSoFar].length = len;
        patch->posts[numPostsUsedSoFar].slope = 0;

        edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, top);
        if (edgeSlope == 1) patch->posts[numPostsUsedSoFar].slope |= RDRAW_EDGESLOPE_TOP_UP;
        else if (edgeSlope == -1) patch->posts[numPostsUsedSoFar].slope |= RDRAW_EDGESLOPE_TOP_DOWN;

        edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, top+len);
        if (edgeSlope == 1) patch->posts[numPostsUsedSoFar].slope |= RDRAW_EDGESLOPE_BOT_UP;
        else if (edgeSlope == -1) patch->posts[numPostsUsedSoFar].slope |= RDRAW_EDGESLOPE_BOT_DOWN;

        // fill in the post's pixels
        oldColumnPixelData = (const byte *)oldColumn + 3;
        for (y=0; y<len; y++) {
          StorePixel(patch, x, top + y, oldColumnPixelData[y]);
        }
      }
      oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
      numPostsUsedSoFar++;
    }
  }

  FillEmptySpace(patch);

  W_UnlockLumpNum(patchNum);
  free(numPostsInColumn);
}

typedef struct {
  unsigned short patches;
  unsigned short posts;
  unsigned short posts_used;
} count_t;

static void switchPosts(rpost_t *post1, rpost_t *post2) {
  rpost_t dummy;

  dummy.topdelta = post1->topdelta;
  dummy.length = post1->length;
  dummy.slope = post1->slope;
  post1->topdelta = post2->topdelta;
  post1->length = post2->length;
  post1->slope = post2->slope;
  post2->topdelta = dummy.topdelta;
  post2->length = dummy.length;
  post2->slope = dummy.slope;
}

static void removePostFromColumn(rcolumn_t *column, int post) {
  int i;
#ifdef RANGECHECK
  if (post >= column->numPosts)
    I_Error("removePostFromColumn: invalid post index");
#endif
  if (post < column->numPosts)
    for (i=post; i<(column->numPosts-1); i++) {
      rpost_t *post1 = &column->posts[i];
      rpost_t *post2 = &column->posts[i+1];
      post1->topdelta = post2->topdelta;
      post1->length = post2->length;
      post1->slope = post2->slope;
    }
  column->numPosts--;
}

//---------------------------------------------------------------------------
static void createTextureCompositePatch(int id) {
  rpatch_t *composite_patch;
  texture_t *texture;
  texpatch_t *texpatch;
  int patchNum;
  const patch_t *oldPatch;
  const column_t *oldColumn, *oldPrevColumn, *oldNextColumn;
  int i, x, y;
  int oy, count;
  int pixelDataSize;
  int columnsDataSize;
  int postsDataSize;
  int dataSize;
  int numPostsTotal;
  const unsigned char *oldColumnPixelData;
  int numPostsUsedSoFar;
  int edgeSlope;
  count_t *countsInColumn;

#ifdef RANGECHECK
  if (id >= numtextures)
    I_Error("createTextureCompositePatch: %i >= numtextures", id);
#endif

  composite_patch = &texture_composites[id];

  texture = textures[id];

  composite_patch->width = texture->width;
  composite_patch->height = texture->height;
  composite_patch->widthmask = texture->widthmask;
  composite_patch->leftoffset = 0;
  composite_patch->topoffset = 0;
  composite_patch->flags = 0;

  // work out how much memory we need to allocate for this patch's data
  pixelDataSize = (composite_patch->width * composite_patch->height + 4) & ~3;
  columnsDataSize = sizeof(rcolumn_t) * composite_patch->width;

  // count the number of posts in each column
  countsInColumn = (count_t *)calloc(sizeof(count_t), composite_patch->width);
  numPostsTotal = 0;

  for (i=0; i<texture->patchcount; i++) {
    texpatch = &texture->patches[i];
    patchNum = texpatch->patch;
    oldPatch = (const patch_t*)W_CacheLumpNum(patchNum);

    for (x=0; x<LittleShort(oldPatch->width); x++) {
      int tx = texpatch->originx + x;

      if (tx < 0)
        continue;
      if (tx >= composite_patch->width)
        break;

      countsInColumn[tx].patches++;

      oldColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x]));
      while (oldColumn->topdelta != 0xff) {
        countsInColumn[tx].posts++;
        numPostsTotal++;
        oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
      }
    }

    W_UnlockLumpNum(patchNum);
  }

  postsDataSize = numPostsTotal * sizeof(rpost_t);

  // allocate our data chunk
  dataSize = pixelDataSize + columnsDataSize + postsDataSize;
  composite_patch->data = (unsigned char*)Z_Malloc(dataSize, PU_STATIC, (void **)&composite_patch->data);
  memset(composite_patch->data, 0, dataSize);

  // set out pixel, column, and post pointers into our data array
  composite_patch->pixels = composite_patch->data;
  composite_patch->columns = (rcolumn_t*)((unsigned char*)composite_patch->pixels + pixelDataSize);
  composite_patch->posts = (rpost_t*)((unsigned char*)composite_patch->columns + columnsDataSize);

  // sanity check that we've got all the memory allocated we need
  assert((((byte*)composite_patch->posts + numPostsTotal*sizeof(rpost_t)) - (byte*)composite_patch->data) == dataSize);

  if (playpal_transparent != 0)
    memset(composite_patch->pixels, playpal_transparent,
           (composite_patch->width*composite_patch->height));

  numPostsUsedSoFar = 0;

  for (x=0; x<texture->width; x++) {
      // setup the column's data
      composite_patch->columns[x].pixels = composite_patch->pixels + (x*composite_patch->height);
      composite_patch->columns[x].numPosts = countsInColumn[x].posts;
      composite_patch->columns[x].posts = composite_patch->posts + numPostsUsedSoFar;
      numPostsUsedSoFar += countsInColumn[x].posts;
  }

  // fill in the pixels, posts, and columns
  for (i=0; i<texture->patchcount; i++) {
    texpatch = &texture->patches[i];
    patchNum = texpatch->patch;
    oldPatch = (const patch_t*)W_CacheLumpNum(patchNum);

    for (x=0; x<LittleShort(oldPatch->width); x++) {
      int top = -1;
      int tx = texpatch->originx + x;

      if (tx < 0)
        continue;
      if (tx >= composite_patch->width)
        break;

      oldColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[x]));

      {
        // tiling
        int prevColumnIndex = x-1;
        int nextColumnIndex = x+1;
        while (prevColumnIndex < 0) prevColumnIndex += LittleShort(oldPatch->width);
        while (nextColumnIndex >= LittleShort(oldPatch->width)) nextColumnIndex -= LittleShort(oldPatch->width);
        oldPrevColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[prevColumnIndex]));
        oldNextColumn = (const column_t *)((const byte *)oldPatch + LittleLong(oldPatch->columnofs[nextColumnIndex]));
      }

      while (oldColumn->topdelta != 0xff) {
        rpost_t *post = &composite_patch->columns[tx].posts[countsInColumn[tx].posts_used];

        //e6y: support for DeePsea's true tall patches
        if (oldColumn->topdelta <= top)
        {
          top += oldColumn->topdelta;
        }
        else
        {
          top = oldColumn->topdelta;
        }

        oldColumnPixelData = (const byte *)oldColumn + 3;
        oy = texpatch->originy;
        count = oldColumn->length;
        // the original renderer had several bugs which we reproduce here
        if (countsInColumn[tx].patches > 1) {
          // when there are multiple patches, then we need to handle the
          // column differently
          if (i == 0) {
            // draw first patch at original position, it will be partly
            // overdrawn below
            for (y=0; y<count; y++) {
              int ty = oy + top + y;
              if (ty < 0)
                continue;
              if (ty >= composite_patch->height)
                break;
              StorePixel(composite_patch, tx, ty, oldColumnPixelData[y]);
            }
          }
          // do the buggy clipping
          if ((oy + top) < 0) {
            count += oy;
            oy = 0;
          }
        } else {
          // with a single patch only negative y origins are wrong
          oy = 0;
        }
        // set up the post's data
        post->topdelta = top + oy;
        post->length = count;
        if ((post->topdelta + post->length) > composite_patch->height) {
          if (post->topdelta > composite_patch->height)
            post->length = 0;
          else
            post->length = composite_patch->height - post->topdelta;
        }
        if (post->topdelta < 0) {
          if ((post->topdelta + post->length) <= 0)
            post->length = 0;
          else
            post->length -= post->topdelta;
          post->topdelta = 0;
        }
        post->slope = 0;

        edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, top);
        if (edgeSlope == 1) post->slope |= RDRAW_EDGESLOPE_TOP_UP;
        else if (edgeSlope == -1) post->slope |= RDRAW_EDGESLOPE_TOP_DOWN;

        edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, top+count);
        if (edgeSlope == 1) post->slope |= RDRAW_EDGESLOPE_BOT_UP;
        else if (edgeSlope == -1) post->slope |= RDRAW_EDGESLOPE_BOT_DOWN;

        // fill in the post's pixels
        for (y=0; y<count; y++) {
          int ty = oy + top + y;
          if (ty < 0)
            continue;
          if (ty >= composite_patch->height)
            break;
          StorePixel(composite_patch, tx, ty, oldColumnPixelData[y]);
        }

        oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
        countsInColumn[tx].posts_used++;
        assert(countsInColumn[tx].posts_used <= countsInColumn[tx].posts);
      }
    }

    W_UnlockLumpNum(patchNum);
  }

  for (x=0; x<texture->width; x++) {
    rcolumn_t *column;

    if (countsInColumn[x].patches <= 1)
      continue;

    // cleanup posts on multipatch columns
    column = &composite_patch->columns[x];

    i = 0;
    while (i<(column->numPosts-1)) {
      rpost_t *post1 = &column->posts[i];
      rpost_t *post2 = &column->posts[i+1];
      int length;

      if ((post2->topdelta - post1->topdelta) < 0)
        switchPosts(post1, post2);

      if ((post1->topdelta + post1->length) >= post2->topdelta) {
        length = (post1->length + post2->length) - ((post1->topdelta + post1->length) - post2->topdelta);
        if (post1->length < length) {
          post1->slope = post2->slope;
          post1->length = length;
        }
        removePostFromColumn(column, i+1);
        i = 0;
        continue;
      }
      i++;
    }
  }

  FillEmptySpace(composite_patch);

  free(countsInColumn);
}

//---------------------------------------------------------------------------
const rpatch_t *R_CachePatchNum(int id) {
  const int locks = 1;

  if (!patches)
    I_Error("R_CachePatchNum: Patches not initialized");

#ifdef RANGECHECK
  if (id >= numlumps)
    I_Error("createPatch: %i >= numlumps", id);
#endif

  if (!patches[id].data)
    createPatch(id);

  /* cph - if wasn't locked but now is, tell z_zone to hold it */
  if (!patches[id].locks && locks) {
    Z_ChangeTag(patches[id].data,PU_STATIC);
#ifdef TIMEDIAG
    patches[id].locktic = gametic;
#endif
  }
  patches[id].locks += locks;

#ifdef SIMPLECHECKS
  if (!((patches[id].locks+1) & 0xf))
    lprintf(LO_DEBUG, "R_CachePatchNum: High lock on %8s (%d)\n", 
	    lumpinfo[id].name, patches[id].locks);
#endif

  return &patches[id];
}

void R_UnlockPatchNum(int id)
{
  const int unlocks = 1;
#ifdef SIMPLECHECKS
  if ((signed short)patches[id].locks < unlocks)
    lprintf(LO_DEBUG, "R_UnlockPatchNum: Excess unlocks on %8s (%d-%d)\n", 
	    lumpinfo[id].name, patches[id].locks, unlocks);
#endif
  patches[id].locks -= unlocks;
  /* cph - Note: must only tell z_zone to make purgeable if currently locked, 
   * else it might already have been purged
   */
  if (unlocks && !patches[id].locks)
    Z_ChangeTag(patches[id].data, PU_CACHE);
}

//---------------------------------------------------------------------------
const rpatch_t *R_CacheTextureCompositePatchNum(int id) {
  const int locks = 1;

  if (!texture_composites)
    I_Error("R_CacheTextureCompositePatchNum: Composite patches not initialized");

#ifdef RANGECHECK
  if (id >= numtextures)
    I_Error("createTextureCompositePatch: %i >= numtextures", id);
#endif

  if (!texture_composites[id].data)
    createTextureCompositePatch(id);

  /* cph - if wasn't locked but now is, tell z_zone to hold it */
  if (!texture_composites[id].locks && locks) {
    Z_ChangeTag(texture_composites[id].data,PU_STATIC);
#ifdef TIMEDIAG
    texture_composites[id].locktic = gametic;
#endif
  }
  texture_composites[id].locks += locks;

#ifdef SIMPLECHECKS
  if (!((texture_composites[id].locks+1) & 0xf))
    lprintf(LO_DEBUG, "R_CacheTextureCompositePatchNum: High lock on %8s (%d)\n", 
	    textures[id]->name, texture_composites[id].locks);
#endif

  return &texture_composites[id];

}

void R_UnlockTextureCompositePatchNum(int id)
{
  const int unlocks = 1;
#ifdef SIMPLECHECKS
  if ((signed short)texture_composites[id].locks < unlocks)
    lprintf(LO_DEBUG, "R_UnlockTextureCompositePatchNum: Excess unlocks on %8s (%d-%d)\n", 
	    textures[id]->name, texture_composites[id].locks, unlocks);
#endif
  texture_composites[id].locks -= unlocks;
  /* cph - Note: must only tell z_zone to make purgeable if currently locked, 
   * else it might already have been purged
   */
  if (unlocks && !texture_composites[id].locks)
    Z_ChangeTag(texture_composites[id].data, PU_CACHE);
}

//---------------------------------------------------------------------------
const rcolumn_t *R_GetPatchColumnWrapped(const rpatch_t *patch, int columnIndex) {
  while (columnIndex < 0) columnIndex += patch->width;
  columnIndex %= patch->width;
  return &patch->columns[columnIndex];
}

//---------------------------------------------------------------------------
const rcolumn_t *R_GetPatchColumnClamped(const rpatch_t *patch, int columnIndex) {
  if (columnIndex < 0) columnIndex = 0;
  if (columnIndex >= patch->width) columnIndex = patch->width-1;
  return &patch->columns[columnIndex];
}

//---------------------------------------------------------------------------
const rcolumn_t *R_GetPatchColumn(const rpatch_t *patch, int columnIndex) {
  if (patch->flags&PATCH_ISNOTTILEABLE) return R_GetPatchColumnClamped(patch, columnIndex);
  else return R_GetPatchColumnWrapped(patch, columnIndex);
}

