#ifndef R_PATCH_H
#define R_PATCH_H

//---------------------------------------------------------------------------
typedef struct {
  int startY;
  int length;
  unsigned char edgeSloping;
} TPatchPost;

//---------------------------------------------------------------------------
typedef struct {
  int numPosts;
  TPatchPost *posts;
  unsigned char *pixels;
} TPatchColumn;

//---------------------------------------------------------------------------
typedef struct {
  int width;
  int height;
  unsigned  widthmask;
    
  unsigned char isNotTileable;
  
  int leftOffset;
  int topOffset;
  
  // this is the single malloc'ed/free'd array 
  // for this patch
  unsigned char *data;
  
  // these are pointers into the data array
  unsigned char *pixels;
  TPatchColumn *columns;
  TPatchPost *posts;

#ifdef TIMEDIAG
  int locktic;
#endif
  unsigned int locks;
} TPatch;

//---------------------------------------------------------------------------
const TPatch *R_CachePatchNum(int id);
void R_UnlockPatchNum(int id);
#define R_CachePatchName(name) R_CachePatchNum(W_GetNumForName(name))
#define R_UnlockPatchName(name) R_UnlockPatchNum(W_GetNumForName(name))

const TPatch *R_CacheTextureCompositePatchNum(int id);
void R_UnlockTextureCompositePatchNum(int id);

//---------------------------------------------------------------------------
// Size query funcs
int R_NumPatchWidth(int lump) ;
int R_NumPatchHeight(int lump);
#define R_NamePatchWidth(name) R_NumPatchWidth(W_GetNumForName(name))
#define R_NamePatchHeight(name) R_NumPatchHeight(W_GetNumForName(name))

//---------------------------------------------------------------------------
const TPatchColumn *R_GetPatchColumnWrapped(const TPatch *patch, int columnIndex);
const TPatchColumn *R_GetPatchColumnClamped(const TPatch *patch, int columnIndex);

//---------------------------------------------------------------------------
// returns R_GetPatchColumnWrapped for square, non-holed textures
// and R_GetPatchColumnClamped otherwise
const TPatchColumn *R_GetPatchColumn(const TPatch *patch, int columnIndex);

//---------------------------------------------------------------------------
void R_InitPatches();
void R_FlushAllPatches();

//---------------------------------------------------------------------------







#endif
