// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: W_wad.c,v 1.1 2000/04/09 18:13:04 proff_fs Exp $
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
//      Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: W_wad.c,v 1.1 2000/04/09 18:13:04 proff_fs Exp $";

#include "doomstat.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf
#ifdef _MSC_VER // proff
#include <io.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else //_MSC_VER
#include <unistd.h>
#endif //_MSC_VER
#include <fcntl.h>
#include <sys/stat.h>

#include "w_wad.h"

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t *lumpinfo;
int        numlumps;         // killough
void       **lumpcache;      // killough

// proff 07/04/98: Changed from _WIN32 to _MSC_VER for CYGWIN32 compatibility
// proff: This is defined in <io.h>
#if !defined(_MSC_VER) && !defined(__MINGW32__)
static int filelength(int handle)
{
  struct stat   fileinfo;
  if (fstat(handle,&fileinfo) == -1)
    I_Error("Error fstating");
  return fileinfo.st_size;
}
#endif //_MSC_VER

void ExtractFileBase (const char *path, char *dest)
{
  const char *src = path + strlen(path) - 1;
  int length;

  // back up until a \ or the start
  while (src != path && src[-1] != ':' // killough 3/22/98: allow c:filename
         && *(src-1) != '\\'
         && *(src-1) != '/')
    src--;

  // copy up to eight characters
  memset(dest,0,8);
  length = 0;

  while (*src && *src != '.')
    if (++length == 9)
      I_Error ("Filename base of %s >8 chars",path);
    else
      *dest++ = toupper(*src++);
}

//
// 1/18/98 killough: adds a default extension to a path
// Note: Backslashes are treated specially, for MS-DOS.
//

char *AddDefaultExtension(char *path, const char *ext)
{
  char *p = path;
  while (*p++);
  while (p-->path && *p!='/' && *p!='\\')
    if (*p=='.')
      return path;
  if (*ext!='.')
    strcat(path,".");
  return strcat(path,ext);
}

// NormalizeSlashes
//
// Remove trailing slashes, translate backslashes to slashes
// The string to normalize is passed and returned in str
//
// killough 11/98: rewritten

void NormalizeSlashes(char *str)
{
  char *p;

  // Convert backslashes to slashes
  for (p = str; *p; p++)
    if (*p == '\\')
      *p = '/';

  // Remove trailing slashes
  while (p > str && *--p == '/')
    *p = 0;

  // Collapse multiple slashes
  for (p = str; (*str++ = *p);)
    if (*p++ == '/')
      while (*p == '/')
	p++;
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// Reload hack removed by Lee Killough
//
// Ty 08/29/98 - added source parm to indicate iwad, pwad or lmp loaded file

static void W_AddFile(const char *name,int source) // killough 1/31/98: static, const
{
  wadinfo_t   header;
  lumpinfo_t* lump_p;
  unsigned    i;
  int         handle;
  int         length;
  int         startlump;
  filelump_t  *fileinfo, *fileinfo2free=NULL; //killough
  filelump_t  singleinfo;
  char        *filename;

  filename=strcpy(malloc(strlen(name)+5), name);

  NormalizeSlashes(AddDefaultExtension(filename, ".wad")); // killough 11/98
  // open the file and add to directory

  if ((handle = open(filename,O_RDONLY | O_BINARY)) == -1)
    {
      if (strlen(name) > 4 && !strcasecmp(name+strlen(name)-4 , ".lmp" ))
    	{
    	  free(filename);
    	  return;
	    }
      // killough 11/98: allow .lmp extension if none existed before
      NormalizeSlashes(AddDefaultExtension(strcpy(filename, name), ".lmp"));
      if ((handle = open(filename,O_RDONLY | O_BINARY)) == -1)
	      I_Error("Error: couldn't open %s\n",name);  // killough
    }

  //jff 8/3/98 use logical output routine
  lprintf (LO_INFO," adding %s\n",filename);
  startlump = numlumps;

  // killough:
  if (strlen(filename)<=4 || strcasecmp(filename+strlen(filename)-4, ".wad" ))
    {
      // single lump file
      fileinfo = &singleinfo;
      singleinfo.filepos = 0;
      singleinfo.size = LONG(filelength(handle));
      ExtractFileBase(filename, singleinfo.name);
      numlumps++;
    }
  else
    {
      // WAD file
      read(handle, &header, sizeof(header));
      if (strncmp(header.identification,"IWAD",4) &&
          strncmp(header.identification,"PWAD",4))
        I_Error ("Wad file %s doesn't have IWAD or PWAD id\n", filename);
      header.numlumps = LONG(header.numlumps);
      header.infotableofs = LONG(header.infotableofs);
      length = header.numlumps*sizeof(filelump_t);
      fileinfo2free = fileinfo = malloc(length);    // killough
      lseek(handle, header.infotableofs, SEEK_SET);
      read(handle, fileinfo, length);
      numlumps += header.numlumps;
    }

    free(filename);           // killough 11/98

    // Fill in lumpinfo
    lumpinfo = realloc(lumpinfo, numlumps*sizeof(lumpinfo_t));

    lump_p = &lumpinfo[startlump];

    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
      {
        lump_p->handle = handle;                    //  killough 4/25/98
        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
        lump_p->data = NULL;                        // killough 1/31/98
        lump_p->namespace = ns_global;              // killough 4/17/98
        lump_p->source = source;                    // Ty 08/29/98
        strncpy (lump_p->name, fileinfo->name, 8);
      }

    free(fileinfo2free);      // killough
}

// jff 1/23/98 Create routines to reorder the master directory
// putting all flats into one marked block, and all sprites into another.
// This will allow loading of sprites and flats from a PWAD with no
// other changes to code, particularly fast hashes of the lumps.
//
// killough 1/24/98 modified routines to be a little faster and smaller

static int IsMarker(const char *marker, const char *name)
{
  return !strncasecmp(name, marker, 8) ||
    (*name == *marker && !strncasecmp(name+1, marker, 7));
}

// killough 4/17/98: add namespace tags

static void W_CoalesceMarkedResource(const char *start_marker,
                                     const char *end_marker, int namespace)
{
  lumpinfo_t *marked = malloc(sizeof(*marked) * numlumps);
  size_t i, num_marked = 0, num_unmarked = 0;
  int is_marked = 0, mark_end = 0;
  lumpinfo_t *lump = lumpinfo;

  for (i=numlumps; i--; lump++)
    if (IsMarker(start_marker, lump->name))       // start marker found
      { // If this is the first start marker, add start marker to marked lumps
        if (!num_marked)
          {
            strncpy(marked->name, start_marker, 8);
            marked->size = 0;  // killough 3/20/98: force size to be 0
            marked->namespace = ns_global;        // killough 4/17/98
            num_marked = 1;
          }
        is_marked = 1;                            // start marking lumps
      }
    else
      if (IsMarker(end_marker, lump->name))       // end marker found
        {
          mark_end = 1;                           // add end marker below
          is_marked = 0;                          // stop marking lumps
        }
      else
        if (is_marked)                            // if we are marking lumps,
          {                                       // move lump to marked list
            marked[num_marked] = *lump;
            marked[num_marked++].namespace = namespace;  // killough 4/17/98
          }
        else
          lumpinfo[num_unmarked++] = *lump;       // else move down THIS list

  // Append marked list to end of unmarked list
  memcpy(lumpinfo + num_unmarked, marked, num_marked * sizeof(*marked));

  free(marked);                                   // free marked list

  numlumps = num_unmarked + num_marked;           // new total number of lumps

  if (mark_end)                                   // add end marker
    {
      lumpinfo[numlumps].size = 0;  // killough 3/20/98: force size to be 0
      lumpinfo[numlumps].namespace = ns_global;   // killough 4/17/98
      strncpy(lumpinfo[numlumps++].name, end_marker, 8);
    }
}

// Hash function used for lump names.
// Must be mod'ed with table size.
// Can be used for any 8-character names.
// by Lee Killough

unsigned W_LumpNameHash(const char *s)
{
  unsigned hash;
  (void) ((hash =        toupper(s[0]), s[1]) &&
          (hash = hash*3+toupper(s[1]), s[2]) &&
          (hash = hash*2+toupper(s[2]), s[3]) &&
          (hash = hash*2+toupper(s[3]), s[4]) &&
          (hash = hash*2+toupper(s[4]), s[5]) &&
          (hash = hash*2+toupper(s[5]), s[6]) &&
          (hash = hash*2+toupper(s[6]),
           hash = hash*2+toupper(s[7]))
         );
  return hash;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//
// Rewritten by Lee Killough to use hash table for performance. Significantly
// cuts down on time -- increases Doom performance over 300%. This is the
// single most important optimization of the original Doom sources, because
// lump name lookup is used so often, and the original Doom used a sequential
// search. For large wads with > 1000 lumps this meant an average of over
// 500 were probed during every search. Now the average is under 2 probes per
// search. There is no significant benefit to packing the names into longwords
// with this new hashing algorithm, because the work to do the packing is
// just as much work as simply doing the string comparisons with the new
// algorithm, which minimizes the expected number of comparisons to under 2.
//
// killough 4/17/98: add namespace parameter to prevent collisions
// between different resources such as flats, sprites, colormaps
//

int (W_CheckNumForName)(register const char *name, register int namespace)
{
  // Hash function maps the name to one of possibly numlump chains.
  // It has been tuned so that the average chain length never exceeds 2.

  register int i = lumpinfo[W_LumpNameHash(name) % (unsigned) numlumps].index;

  // We search along the chain until end, looking for case-insensitive
  // matches which also match a namespace tag. Separate hash tables are
  // not used for each namespace, because the performance benefit is not
  // worth the overhead, considering namespace collisions are rare in
  // Doom wads.

  while (i >= 0 && (strncasecmp(lumpinfo[i].name, name, 8) ||
                    lumpinfo[i].namespace != namespace))
    i = lumpinfo[i].next;

  // Return the matching lump, or -1 if none found.

  return i;
}

//
// killough 1/31/98: Initialize lump hash table
//

static void W_InitLumpHash(void)
{
  int i;

  for (i=0; i<numlumps; i++)
    lumpinfo[i].index = -1;                     // mark slots empty

  // Insert nodes to the beginning of each chain, in first-to-last
  // lump order, so that the last lump of a given name appears first
  // in any chain, observing pwad ordering rules. killough

  for (i=0; i<numlumps; i++)
    {                                           // hash function:
      int j = W_LumpNameHash(lumpinfo[i].name) % (unsigned) numlumps;
      lumpinfo[i].next = lumpinfo[j].index;     // Prepend to list
      lumpinfo[j].index = i;
    }
}

// End of lump hashing -- killough 1/31/98

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//

int W_GetNumForName (const char* name)     // killough -- const added
{
  int i = W_CheckNumForName (name);
  if (i == -1)
    I_Error ("W_GetNumForName: %.8s not found!", name); // killough .8 added
  return i;
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//

void W_InitMultipleFiles(char *const *filenames, int *const pfilesource)
{
  int *filesource = pfilesource;  // to iterate with

  // killough 1/31/98: add predefined lumps first

  numlumps = num_predefined_lumps;

  // lumpinfo will be realloced as lumps are added
  lumpinfo = malloc(numlumps*sizeof(*lumpinfo));

  memcpy(lumpinfo, predefined_lumps, numlumps*sizeof(*lumpinfo));
  // Ty 08/29/98 - add source flag to the predefined lumps
  {
    int i;
    for (i=0;i<numlumps;i++)
      lumpinfo[i].source = source_pre;
  }

  // open all the files, load headers, and count lumps
  while (*filenames)
    W_AddFile(*filenames++,*filesource++);

  if (!numlumps)
    I_Error ("W_InitFiles: no files found");

  //jff 1/23/98
  // get all the sprites and flats into one marked block each
  // killough 1/24/98: change interface to use M_START/M_END explicitly
  // killough 4/17/98: Add namespace tags to each entry

  W_CoalesceMarkedResource("S_START", "S_END", ns_sprites);
  W_CoalesceMarkedResource("F_START", "F_END", ns_flats);

  // killough 4/4/98: add colormap markers
  W_CoalesceMarkedResource("C_START", "C_END", ns_colormaps);

  // set up caching
  lumpcache = calloc(sizeof *lumpcache, numlumps); // killough

  if (!lumpcache)
    I_Error ("Couldn't allocate lumpcache");

  // killough 1/31/98: initialize lump hash table
  W_InitLumpHash();
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
  if (lump >= numlumps)
    I_Error ("W_LumpLength: %i >= numlumps",lump);
  return lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//

void W_ReadLump(int lump, void *dest)
{
  lumpinfo_t *l = lumpinfo + lump;

#ifdef RANGECHECK
  if (lump >= numlumps)
    I_Error ("W_ReadLump: %i >= numlumps",lump);
#endif

  if (l->data)     // killough 1/31/98: predefined lump data
    memcpy(dest, l->data, l->size);
  else
    {
      int c;

      // killough 1/31/98: Reload hack (-wart) removed

      lseek(l->handle, l->position, SEEK_SET);
      c = read(l->handle, dest, l->size);
      if (c < l->size)
        I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);
    }
}

//
// W_CacheLumpNum
//
// killough 4/25/98: simplified

void *W_CacheLumpNum (int lump, int tag)
{
#ifdef RANGECHECK
  if ((unsigned)lump >= numlumps)
    I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
#endif

  if (!lumpcache[lump])      // read the lump in
    W_ReadLump(lump, Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]));
  else
    Z_ChangeTag(lumpcache[lump],tag);

  return lumpcache[lump];
}

// W_CacheLumpName macroized in w_wad.h -- killough

// WritePredefinedLumpWad
// Args: Filename - string with filename to write to
// Returns: void
//
// If the user puts a -dumplumps switch on the command line, we will
// write all those predefined lumps above out into a pwad.  User
// supplies the pwad name.
//
// killough 4/22/98: make endian-independent, remove tab chars
void WritePredefinedLumpWad(const char *filename)
{
  int handle;         // for file open
  char filenam[256];  // we may have to add ".wad" to the name they pass

  if (!filename || !*filename)  // check for null pointer or empty name
    return;  // early return

  AddDefaultExtension(strcpy(filenam, filename), ".wad");

  // The following code writes a PWAD from the predefined lumps array
  // How to write a PWAD will not be explained here.
#ifdef _MSC_VER // proff: In Visual C open is defined a bit different
  if ( (handle = open (filenam, O_RDWR | O_CREAT | O_BINARY, _S_IWRITE|_S_IREAD)) != -1)
#else //_MSC_VER
  if ( (handle = open (filenam, O_RDWR | O_CREAT | O_BINARY, S_IWUSR|S_IRUSR)) != -1)
#endif //_MSC_VER
  {
    wadinfo_t header = {"PWAD"};
    size_t filepos = sizeof(wadinfo_t) + num_predefined_lumps * sizeof(filelump_t);
    int i;

    header.numlumps = LONG(num_predefined_lumps);
    header.infotableofs = LONG(sizeof(header));

    // write header
    write(handle, &header, sizeof(header));

    // write directory
    for (i=0;i<num_predefined_lumps;i++)
    {
      filelump_t fileinfo = {0};
      fileinfo.filepos = LONG(filepos);
      fileinfo.size = LONG(predefined_lumps[i].size);
      strncpy(fileinfo.name, predefined_lumps[i].name, 8);
      write(handle, &fileinfo, sizeof(fileinfo));
      filepos += predefined_lumps[i].size;
    }

    // write lumps
    for (i=0;i<num_predefined_lumps;i++)
      write(handle, predefined_lumps[i].data, predefined_lumps[i].size);

    close(handle);
    I_Error("Predefined lumps wad, %s written, exiting\n", filename);
  }
 I_Error("Cannot open predefined lumps wad %s for output\n", filename);
}

//----------------------------------------------------------------------------
//
// $Log: W_wad.c,v $
// Revision 1.1  2000/04/09 18:13:04  proff_fs
// Initial revision
//
// Revision 1.22  1998/09/07  20:10:30  jim
// Logical output routine added
//
// Revision 1.21  1998/08/29  22:59:55  thldrmn
// Lump source field logic etc.
//
// Revision 1.20  1998/05/06  11:32:00  jim
// Moved predefined lump writer info->w_wad
//
// Revision 1.19  1998/05/03  22:43:09  killough
// beautification, header #includes
//
// Revision 1.18  1998/05/01  14:53:59  killough
// beautification
//
// Revision 1.17  1998/04/27  02:06:41  killough
// Program beautification
//
// Revision 1.16  1998/04/17  10:34:53  killough
// Tag lumps with namespace tags to resolve collisions
//
// Revision 1.15  1998/04/06  04:43:59  killough
// Add C_START/C_END support, remove non-standard C code
//
// Revision 1.14  1998/03/23  03:42:59  killough
// Fix drive-letter bug and force marker lumps to 0-size
//
// Revision 1.12  1998/02/23  04:59:18  killough
// Move TRANMAP init code to r_data.c
//
// Revision 1.11  1998/02/20  23:32:30  phares
// Added external tranmap
//
// Revision 1.10  1998/02/20  22:53:25  phares
// Moved TRANMAP initialization to w_wad.c
//
// Revision 1.9  1998/02/17  06:25:07  killough
// Make numlumps static add #ifdef RANGECHECK for perf
//
// Revision 1.8  1998/02/09  03:20:16  killough
// Fix garbage printed in lump error message
//
// Revision 1.7  1998/02/02  13:21:04  killough
// improve hashing, add predef lumps, fix err handling
//
// Revision 1.6  1998/01/26  19:25:10  phares
// First rev with no ^Ms
//
// Revision 1.5  1998/01/26  06:30:50  killough
// Rewrite merge routine to use simpler, robust algorithm
//
// Revision 1.3  1998/01/23  20:28:11  jim
// Basic sprite/flat functionality in PWAD added
//
// Revision 1.2  1998/01/22  05:55:58  killough
// Improve hashing algorithm
//
//----------------------------------------------------------------------------

