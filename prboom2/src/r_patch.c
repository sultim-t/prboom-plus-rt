#include "r_patch.h"
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
#include <assert.h>

//---------------------------------------------------------------------------
// Legacy patch format, now private to this file
//---------------------------------------------------------------------------
typedef struct {
  short swidth, sheight;  // bounding box size
  short sleftoffset;     // pixels to the left of origin
  short stopoffset;      // pixels below the origin
  int lcolumnofs[8];     // only [width] used
} patch_t;
// posts are runs of non masked source pixels
typedef struct {
  byte topdelta; // -1 is the last post in a column
  byte length;   // length data bytes follows
} post_t;
// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;

//---------------------------------------------------------------------------
// Re-engineered patch support
//---------------------------------------------------------------------------
static TPatch *patches = 0;
int numPatches = 0;

//---------------------------------------------------------------------------
void R_InitPatches() {
  int i;
  for (i=0; i<numPatches; i++) {
    if (patches[i].data) free(patches[i].data);
  }
  free(patches);
  patches = 0;
  numPatches = 0;
}

//---------------------------------------------------------------------------
void R_FlushAllPatches() {
  R_InitPatches();
}

//---------------------------------------------------------------------------
int R_NumPatchWidth(int lump) { return R_GetPatch(lump)->width; }
int R_NumPatchHeight(int lump) { return R_GetPatch(lump)->height; }
int R_NamePatchWidth(const char *n) { return R_NumPatchWidth(W_GetNumForName(n)); }
int R_NamePatchHeight(const char *n) { return R_NumPatchHeight(W_GetNumForName(n)); }

//---------------------------------------------------------------------------
static int getPatchIsNotTileable(const patch_t *patch) {
  int x=0, numPosts, lastColumnDelta = 0;
  const column_t *column;
  int cornerCount = 0;
  int hasAHole = 0;

  for (x=0; x<SHORT(patch->swidth); x++) {
    column = (const column_t *)((const byte *)patch + LONG(patch->lcolumnofs[x]));
    if (!x) lastColumnDelta = column->topdelta;
    else if (lastColumnDelta != column->topdelta) hasAHole = 1;

    numPosts = 0;
    while (column->topdelta != 0xff) {
      // check to see if a corner pixel filled
      if (x == 0 && column->topdelta == 0) cornerCount++;
      else if (x == 0 && column->topdelta + column->length >= SHORT(patch->sheight)) cornerCount++;
      else if (x == SHORT(patch->swidth)-1 && column->topdelta == 0) cornerCount++;
      else if (x == SHORT(patch->swidth)-1 && column->topdelta + column->length >= SHORT(patch->sheight)) cornerCount++;

      if (numPosts++) hasAHole = 1;
      column = (const column_t *)((byte *)column + column->length + 4);
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
static void createPatch(int id) {
  TPatch *patch;
  const int patchNum = id;
  const patch_t *oldPatch = (const patch_t*)W_CacheLumpNum(patchNum);
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
  const TPatchColumn *column, *prevColumn;

  if (id >= numPatches) {
    // create room for this patch
    int prevNumPatches = numPatches;
    numPatches = (id + 256);
    patches = (TPatch*)realloc(patches,  numPatches * sizeof(TPatch));
    // clear out new patches to signal they're uninitialized
    memset(patches + prevNumPatches, 0, sizeof(TPatch)*(numPatches-prevNumPatches));
  }

  patch = &patches[id];
  // proff - 2003-02-16 What about endianess?
  patch->width = SHORT(oldPatch->swidth);
  patch->height = SHORT(oldPatch->sheight);
  patch->leftOffset = SHORT(oldPatch->sleftoffset);
  patch->topOffset = SHORT(oldPatch->stopoffset);
  patch->isNotTileable = getPatchIsNotTileable(oldPatch);

  // work out how much memory we need to allocate for this patch's data
  pixelDataSize = (patch->width * patch->height + 4) & ~3;
  columnsDataSize = sizeof(TPatchColumn) * patch->width;

  // count the number of posts in each column
  numPostsInColumn = (int*)malloc(sizeof(int) * patch->width);
  numPostsTotal = 0;

  for (x=0; x<patch->width; x++) {
    oldColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[x]));
    numPostsInColumn[x] = 0;
    while (oldColumn->topdelta != 0xff) {
      numPostsInColumn[x]++;
      numPostsTotal++;
      oldColumn = (const column_t *)((byte *)oldColumn + oldColumn->length + 4);
    }
  }

  postsDataSize = numPostsTotal * sizeof(TPatchPost);

  // allocate our data chunk
  dataSize = pixelDataSize + columnsDataSize + postsDataSize;
  patch->data = (unsigned char*)malloc(dataSize);
  memset(patch->data, 0, dataSize);

  // set out pixel, column, and post pointers into our data array
  patch->pixels = patch->data;
  patch->columns = (TPatchColumn*)((unsigned char*)patch->pixels + pixelDataSize);
  patch->posts = (TPatchPost*)((unsigned char*)patch->columns + columnsDataSize);

  // sanity check that we've got all the memory allocated we need
  assert(((int)patch->posts + (int)(numPostsTotal*sizeof(TPatchPost)) - (int)patch->data) == dataSize);

  memset(patch->pixels, 0xff, (patch->width*patch->height));

  // fill in the pixels, posts, and columns
  numPostsUsedSoFar = 0;
  for (x=0; x<patch->width; x++) {

    oldColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[x]));

    if (patch->isNotTileable) {
      // non-tiling
      if (x == 0) oldPrevColumn = 0;
      else oldPrevColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[x-1]));
      if (x == patch->width-1) oldNextColumn = 0;
      else oldNextColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[x+1]));
    }
    else {
      // tiling
      int prevColumnIndex = x-1;
      int nextColumnIndex = x+1;
      while (prevColumnIndex < 0) prevColumnIndex += patch->width;
      while (nextColumnIndex >= patch->width) nextColumnIndex -= patch->width;
      oldPrevColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[prevColumnIndex]));
      oldNextColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->lcolumnofs[nextColumnIndex]));
    }

    // setup the column's data
    patch->columns[x].pixels = patch->pixels + (x*patch->height) + 0;
    patch->columns[x].numPosts = numPostsInColumn[x];
    patch->columns[x].posts = patch->posts + numPostsUsedSoFar;

    while (oldColumn->topdelta != 0xff) {
      // set up the post's data
      patch->posts[numPostsUsedSoFar].startY = oldColumn->topdelta;
      patch->posts[numPostsUsedSoFar].length = oldColumn->length;
      patch->posts[numPostsUsedSoFar].edgeSloping = 0;

      edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, oldColumn->topdelta);
      if (edgeSlope == 1) patch->posts[numPostsUsedSoFar].edgeSloping |= RDRAW_EDGESLOPE_TOP_UP;
      else if (edgeSlope == -1) patch->posts[numPostsUsedSoFar].edgeSloping |= RDRAW_EDGESLOPE_TOP_DOWN;

      edgeSlope = getColumnEdgeSlope(oldPrevColumn, oldNextColumn, oldColumn->topdelta+oldColumn->length);
      if (edgeSlope == 1) patch->posts[numPostsUsedSoFar].edgeSloping |= RDRAW_EDGESLOPE_BOT_UP;
      else if (edgeSlope == -1) patch->posts[numPostsUsedSoFar].edgeSloping |= RDRAW_EDGESLOPE_BOT_DOWN;

      // fill in the post's pixels
      oldColumnPixelData = (const byte *)oldColumn + 3;
      for (y=0; y<oldColumn->length; y++) {
        patch->pixels[x * patch->height + oldColumn->topdelta + y] = oldColumnPixelData[y];
      }

      oldColumn = (const column_t *)((byte *)oldColumn + oldColumn->length + 4);
      numPostsUsedSoFar++;
    }
  }

  if (1 || patch->isNotTileable) {
    // copy the patch image down and to the right where there are
    // holes to eliminate the black halo from bilinear filtering
    for (x=0; x<patch->width; x++) {
      //oldColumn = (const column_t *)((const byte *)oldPatch + oldPatch->columnofs[x]);

      column = R_GetPatchColumnClamped(patch, x);
      prevColumn = R_GetPatchColumnClamped(patch, x-1);

      if (column->pixels[0] == 0xff) {
        // force the first pixel (which is a hole), to use
        // the color from the next solid spot in the column
        for (y=0; y<patch->height; y++) {
          if (column->pixels[y] != 0xff) {
            column->pixels[0] = column->pixels[y];
            break;
          }
        }
      }

      // copy from above or to the left
      for (y=1; y<patch->height; y++) {
        //if (getIsSolidAtSpot(oldColumn, y)) continue;
        if (column->pixels[y] != 0xff) continue;

        // this pixel is a hole

        if (x && prevColumn->pixels[y-1] != 0xff) {
          // copy the color from the left
          column->pixels[y] = prevColumn->pixels[y];
        }
        else {
          // copy the color from above
          column->pixels[y] = column->pixels[y-1];
        }
      }
    }

    // verify that the patch truly is non-rectangular since
    // this determines tiling later on
  }

  W_UnlockLumpNum(patchNum);
  free(numPostsInColumn);
}

//---------------------------------------------------------------------------
const TPatch *R_GetPatch(int id) {
  if (id >= numPatches || !patches[id].data) createPatch(id);
  return &patches[id];
}

//---------------------------------------------------------------------------
const TPatchColumn *R_GetPatchColumnWrapped(const TPatch *patch, int columnIndex) {
  while (columnIndex < 0) columnIndex += patch->width;
  columnIndex %= patch->width;
  return &patch->columns[columnIndex];
}

//---------------------------------------------------------------------------
const TPatchColumn *R_GetPatchColumnClamped(const TPatch *patch, int columnIndex) {
  if (columnIndex < 0) columnIndex = 0;
  if (columnIndex >= patch->width) columnIndex = patch->width-1;
  return &patch->columns[columnIndex];
}

//---------------------------------------------------------------------------
const TPatchColumn *R_GetPatchColumn(const TPatch *patch, int columnIndex) {
  if (patch->isNotTileable) return R_GetPatchColumnClamped(patch, columnIndex);
  else return R_GetPatchColumnWrapped(patch, columnIndex);
}

