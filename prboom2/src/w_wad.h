/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: w_wad.h,v 1.2 2000/05/07 20:19:34 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
 * DESCRIPTION:
 *      WAD I/O functions.
 *
 *-----------------------------------------------------------------------------*/


#ifndef __W_WAD__
#define __W_WAD__

#ifdef __GNUG__
#pragma interface
#endif

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

#ifndef ALL_IN_ONE

// NO_PREDEFINED_LUMPS causes none of the predefined lumps in info.c to be 
// included, and removes all extra code which is only there for them
// Saves a little memory normally, lots if any were overridden, and makes 
// the executable smaller
#define NO_PREDEFINED_LUMPS

#endif

//
// WADFILE I/O related stuff.
//

// CPhipps - defined enum in wider scope
// Ty 08/29/98 - add source field to identify where this lump came from
typedef enum {
  // CPhipps - define elements in order of 'how new/unusual'
  source_iwad=0,    // iwad file load 
  source_pre,       // predefined lump
  source_auto_load, // lump auto-loaded by config file
  source_pwad,      // pwad file load
  source_lmp,       // lmp file load
  source_net        // CPhipps
} wad_source_t;

typedef struct
{
  // WARNING: order of some fields important (see info.c).

  char  name[8];
  int   size;
#ifndef NO_PREDEFINED_LUMPS
  const void *data;     // killough 1/31/98: points to predefined lump data
#endif

  // killough 1/31/98: hash table fields, used for ultra-fast hash table lookup
  int index, next;

  // killough 4/17/98: namespace tags, to prevent conflicts between resources
  enum {
    ns_global=0,
    ns_sprites,
    ns_flats,
    ns_colormaps
  } namespace;

  int handle;
  int position;
  unsigned int locks; // CPhipps - wad lump locking
  wad_source_t source;
} lumpinfo_t;

// killough 1/31/98: predefined lumps
extern const size_t num_predefined_lumps;
extern const lumpinfo_t predefined_lumps[];

extern void       **lumpcache;
extern lumpinfo_t *lumpinfo;
extern int        numlumps;

// CPhipps - changed wad init
// We _must_ have the wadfiles[] the same as those actually loaded, so there 
// is no point having these separate entities. This belongs here.
struct wadfile_info {
  const char* name;
  wad_source_t src;
};

extern struct wadfile_info *wadfiles;

extern size_t numwadfiles; // CPhipps - size of the wadfiles array

void W_Init(void); // CPhipps - uses the above array

// killough 4/17/98: if W_CheckNumForName() called with only
// one argument, pass ns_global as the default namespace

#define W_CheckNumForName(name) (W_CheckNumForName)(name, ns_global)
int     (W_CheckNumForName)(const char* name, int);   // killough 4/17/98
int     W_GetNumForName (const char* name);
int     W_LumpLength (int lump);
void    W_ReadLump (int lump, void *dest);
// CPhipps - modified for 'new' lump locking
const void* W_CacheLumpNum (int lump, unsigned short locks);
void    W_UnlockLumpNum(int lump, signed short unlocks);

// CPhipps - convenience macros
#define W_CacheLumpNum(num) (W_CacheLumpNum)((num),1)
#define W_CacheLumpName(name) W_CacheLumpNum (W_GetNumForName(name))

#define W_UnlockLumpNum(num) (W_UnlockLumpNum)((num),1)
#define W_UnlockLumpName(name) W_UnlockLumpNum (W_GetNumForName(name))

char *AddDefaultExtension(char *, const char *);  // killough 1/18/98
void ExtractFileBase(const char *, char *);       // killough
unsigned W_LumpNameHash(const char *s);           // killough 1/31/98

// Function to write all predefined lumps to a PWAD if requested
extern void WritePredefinedLumpWad(const char *filename); // jff 5/6/98

#endif

//----------------------------------------------------------------------------
//
// $Log: w_wad.h,v $
// Revision 1.2  2000/05/07 20:19:34  proff_fs
// changed use of colormaps from pointers to numbers.
// That's needed for OpenGL.
// The OpenGL part is slightly better now.
// Added some typedefs to reduce warnings in VisualC.
// Messages are also scaled now, because at 800x600 and
// above you can't read them even on a 21" monitor.
//
// Revision 1.1.1.1  2000/05/04 08:18:58  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.10  1999/10/27 18:35:50  cphipps
// Made W_CacheLump* return a const pointer
//
// Revision 1.9  1999/10/12 13:01:16  cphipps
// Changed header to GPL
//
// Revision 1.8  1999/04/01 21:58:24  cphipps
// Add new wad_source_t class, for pwads added by a network server
//
// Revision 1.7  1999/01/01 15:32:45  cphipps
// New wad lump locking system decls
//
// Revision 1.6  1998/12/22 20:56:26  cphipps
// Declare source enum separately
// New wadfiles array declared
// W_Init prototype changed
//
// Revision 1.5  1998/10/27 19:05:26  cphipps
// Boom v2.02 update: wad source tags
//
// Revision 1.4  1998/10/17 14:52:18  cphipps
// A couple of const's
//
// Revision 1.3  1998/10/13 14:16:22  cphipps
// Allow NO_PREDEFINED_LUMPS to be overridden from makefile
//
// Revision 1.2  1998/09/14 19:20:18  cphipps
// Removed predefined lumps
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
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
