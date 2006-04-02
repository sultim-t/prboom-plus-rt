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
//      WAD I/O functions.
//
//-----------------------------------------------------------------------------

#ifndef __W_WAD__
#define __W_WAD__

#include <stdio.h>

//
// TYPES
//

typedef struct
{
  char identification[4];                  // Should be "IWAD" or "PWAD".
  int  numlumps;
  int  infotableofs;
} wadinfo_t;

typedef struct
{
  int  filepos;
  int  size;
  char name[8];
} filelump_t;

//
// WADFILE I/O related stuff.
//

typedef struct
{
  // WARNING: order of some fields important (see info.c).

  char  name[9];
  int   size;
  const void *data;     // killough 1/31/98: points to predefined lump data

  // killough 1/31/98: hash table fields, used for ultra-fast hash table lookup
  int index, next;

  // killough 4/17/98: namespace tags, to prevent conflicts between resources
  enum {
    ns_global=0,
    ns_sprites,
    ns_flats,
    ns_colormaps,
    ns_translations
  } li_namespace; // haleyjd 05/21/02: renamed from "namespace"

  int handle;
  int position;
  void *cache;  //sf
} lumpinfo_t;

extern void       **lumpcache;
extern lumpinfo_t **lumpinfo;   //sf: ptr to ptr
extern int        numlumps;

void W_InitMultipleFiles(char *const*filenames);

// sf: add a new wad file after the game has already begun
int W_AddNewFile(char *filename);

// killough 4/17/98: if W_CheckNumForName() called with only
// one argument, pass ns_global as the default namespace

#define W_CheckNumForName(name) (W_CheckNumForName)(name, ns_global)
int     (W_CheckNumForName)(const char* name, int);   // killough 4/17/98
int     W_GetNumForName(const char* name);
int     W_LumpLength(int lump);
void    W_ReadLump(int lump, void *dest);
void*   W_CacheLumpNum(int lump, int tag);
long    W_LumpCheckSum(int lumpnum);
int     W_ReadLumpHeader(int lump, void *dest, int size);

#define W_CacheLumpName(name,tag) W_CacheLumpNum (W_GetNumForName(name),(tag))

void NormalizeSlashes(char *);                    // killough 11/98
char *AddDefaultExtension(char *, const char *);  // killough 1/18/98
void ExtractFileBase(const char *, char *);       // killough
unsigned W_LumpNameHash(const char *s);           // killough 1/31/98
void W_InitLumpHash(void);

void I_BeginRead(void), I_EndRead(void); // killough 10/98

extern int iwadhandle;
extern int firstWadHandle; // haleyjd 06/21/04

#endif

//----------------------------------------------------------------------------
//
// $Log: w_wad.h,v $
// Revision 1.10  1998/05/06  11:32:05  jim
// Moved predefined lump writer info->w_wad
//
// Revision 1.9  1998/05/03  22:43:45  killough
// remove unnecessary #includes
//
// Revision 1.8  1998/05/01  14:55:54  killough
// beautification
//
// Revision 1.7  1998/04/27  02:05:30  killough
// Program beautification
//
// Revision 1.6  1998/04/19  01:14:36  killough
// Reinstate separate namespaces
//
// Revision 1.5  1998/04/17  16:52:21  killough
// back out namespace changes temporarily
//
// Revision 1.4  1998/04/17  10:33:50  killough
// Macroize W_CheckNumForName(), add namespace parameter to functional version
//
// Revision 1.3  1998/02/02  13:35:13  killough
// Improve lump hashing, add predefine lumps
//
// Revision 1.2  1998/01/26  19:28:01  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
