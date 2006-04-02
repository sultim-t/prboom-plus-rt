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
//      Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: w_wad.c,v 1.20 1998/05/06 11:32:00 jim Exp $";

#include "doomstat.h"
#include "d_io.h"  // SoM 3/12/2002: moved unistd stuff into d_io.h
#include <fcntl.h>
#include <sys/stat.h>

#include "c_io.h"
#include "s_sound.h"
#include "p_skin.h"
#include "w_wad.h"

//
// GLOBALS
//

// sf:
#ifndef O_BINARY
#define O_BINARY 0
#endif

// Location of each lump on disk.
lumpinfo_t **lumpinfo;     // sf : array of ptrs
int        numlumps;       // killough
int        iwadhandle;     // sf: the handle of the main iwad
int        firstWadHandle = -1; // haleyjd: track handle of first wad added

static int W_FileLength(int handle)
{
   struct stat fileinfo;
   if(fstat(handle, &fileinfo) == -1)
      I_Error("W_FileLength: failure in fstat\n");
   return fileinfo.st_size;
}

void ExtractFileBase(const char *path, char *dest)
{
   const char *src = path + strlen(path) - 1;
   int length;
   
   // back up until a \ or the start
   while(src != path && src[-1] != ':' // killough 3/22/98: allow c:filename
         && *(src-1) != '\\'
         && *(src-1) != '/')
      src--;

   // copy up to eight characters
   memset(dest, 0, 8);
   length = 0;

   while(*src && *src != '.')
      if(++length == 9)
         I_Error("Filename base of %s > 8 chars", path);
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
   while(*p++);
   while(p-- > path && *p != '/' && *p != '\\')
      if(*p == '.')
         return path;
   if(*ext != '.')
      strcat(path, ".");
   return strcat(path, ext);
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
   for(p = str; *p; p++)
   {
      if(*p == '\\')
         *p = '/';
   }

   // Remove trailing slashes
   while(p > str && *--p == '/')
      *p = 0;

   // Collapse multiple slashes
   for(p = str; (*str++ = *p); )
      if(*p++ == '/')
         while(*p == '/')
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

static int w_sound_update_type;

void D_NewWadLumps(int handle, int sound_update_type);

        // sf: made int
static int W_AddFile(const char *name) // killough 1/31/98: static, const
{
   wadinfo_t   header;
   lumpinfo_t* lump_p;
   unsigned    i;
   int         handle;
   int         length;
   int         startlump;
   filelump_t  *fileinfo, *fileinfo2free = NULL; //killough
   filelump_t  singleinfo;
   char        *filename = strdup(name);
   lumpinfo_t  *newlumps;
   boolean     isWad;     // haleyjd 05/23/04

   NormalizeSlashes(AddDefaultExtension(filename, ".wad"));  // killough 11/98

   // open the file and add to directory
   
   if((handle = open(filename, O_RDONLY | O_BINARY)) == -1)
   {
      if(strlen(name) > 4 && !strcasecmp(name+strlen(name)-4 , ".lmp" ))
      {
         free(filename);
         return false;         // sf: no errors
      }
      // killough 11/98: allow .lmp extension if none existed before
      NormalizeSlashes(AddDefaultExtension(strcpy(filename, name), ".lmp"));
      if((handle = open(filename,O_RDONLY | O_BINARY)) == -1)
      {
         if(in_textmode)
            I_Error("Error: couldn't open %s\n",name);  // killough
         else
         {
            C_Printf(FC_ERROR "couldn't open %s\n",name);
            return true;  // error
         }
      }
   }
  
   if(in_textmode)
      printf(" adding %s\n", filename);   // killough 8/8/98
   startlump = numlumps;

   // killough:
   if(strlen(filename) <= 4 || strcasecmp(filename+strlen(filename)-4, ".wad" ))
   {
      // single lump file
      isWad = false; // haleyjd 05/23/04
      fileinfo = &singleinfo;
      singleinfo.filepos = 0;
      singleinfo.size = LONG(W_FileLength(handle));
      ExtractFileBase(filename, singleinfo.name);
      numlumps++;
   }
   else
   {
      // WAD file
      isWad = true; // haleyjd 05/23/04
      read(handle, &header, sizeof(header));
      if(strncmp(header.identification,"IWAD",4) &&
         strncmp(header.identification,"PWAD",4))
         I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
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
   //sf :ptr to ptr
   lumpinfo = realloc(lumpinfo, (numlumps+2)*sizeof(lumpinfo_t*));

   // space for new lumps
   newlumps = malloc((numlumps-startlump) * sizeof(lumpinfo_t));
   lump_p = newlumps;
   //&lumpinfo[startlump];
   
   // update IWAD handle
   //
   // haleyjd 05/23/04: do NOT do this when dealing with single
   // files. "isWad" keeps track of this now.
   
   if(isWad)
   {
      // haleyjd 06/21/04: track handle of first wad added also
      if(firstWadHandle == -1)
         firstWadHandle = handle;

      if(!strncmp(header.identification, "IWAD", 4))
         iwadhandle = handle;
   }
  
   for(i = startlump; i < (unsigned)numlumps; i++, lump_p++, fileinfo++)
   {
      lumpinfo[i] = lump_p;
      lump_p->handle = handle;                    //  killough 4/25/98
      lump_p->position = LONG(fileinfo->filepos);
      lump_p->size = LONG(fileinfo->size);
      // sf:cache
      lump_p->data = lump_p->cache = NULL;         // killough 1/31/98
      lump_p->li_namespace = ns_global;              // killough 4/17/98
      
      memset(lump_p->name, 0, 9);
      strncpy(lump_p->name, fileinfo->name, 8);
   }
   
   free(fileinfo2free);      // killough
   
   D_NewWadLumps(handle, w_sound_update_type);
   
   return false;       // no error
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
                                     const char *end_marker, int li_namespace)
{
   lumpinfo_t **marked = malloc(sizeof(*marked) * numlumps);
   size_t i, num_marked = 0, num_unmarked = 0;
   int is_marked = 0, mark_end = 0;
   lumpinfo_t *lump = lumpinfo[0];
  
   for(i = 0; i < (unsigned)numlumps; i++)
   {
      lump = lumpinfo[i];
      
      // If this is the first start marker, add start marker to marked lumps
      if(IsMarker(start_marker, lump->name))       // start marker found
      {
         if(!num_marked)
         {
            marked[0] = lump;
            marked[0]->li_namespace = ns_global;        // killough 4/17/98
            num_marked = 1;
         }
         is_marked = 1;                            // start marking lumps
      }
      else if(IsMarker(end_marker, lump->name))    // end marker found
      {
         mark_end = 1;                           // add end marker below
         is_marked = 0;                          // stop marking lumps
      }
      else if(is_marked || lump->li_namespace == li_namespace)
      {
         // if we are marking lumps,
         // move lump to marked list
         // sf: check for namespace already set

         // sf 26/10/99:
         // ignore sprite lumps smaller than 8 bytes (the smallest possible)
         // in size -- this was used by some dmadds wads
         // as an 'empty' graphics resource
         if(li_namespace != ns_sprites || lump->size > 8)
         {
            marked[num_marked] = lump;
            marked[num_marked]->li_namespace = li_namespace;
            num_marked++;
         }
      }
      else
      {
         lumpinfo[num_unmarked] = lump;       // else move down THIS list
         num_unmarked++;
      }
   }
   
   // Append marked list to end of unmarked list
   memcpy(lumpinfo + num_unmarked, marked, num_marked * sizeof(lumpinfo_t *));

   free(marked);                                   // free marked list
   
   numlumps = num_unmarked + num_marked;           // new total number of lumps
   
   if(mark_end)                                    // add end marker
   {
      lumpinfo[numlumps] = malloc(sizeof(lumpinfo_t));
      lumpinfo[numlumps]->size = 0;  // killough 3/20/98: force size to be 0
      lumpinfo[numlumps]->li_namespace = ns_global;   // killough 4/17/98
      strncpy(lumpinfo[numlumps]->name, end_marker, 8);
      numlumps++;
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
int (W_CheckNumForName)(register const char *name, register int li_namespace)
{
   // Hash function maps the name to one of possibly numlump chains.
   // It has been tuned so that the average chain length never exceeds 2.
   
   register int i = lumpinfo[W_LumpNameHash(name) % (unsigned)numlumps]->index;

   // We search along the chain until end, looking for case-insensitive
   // matches which also match a namespace tag. Separate hash tables are
   // not used for each namespace, because the performance benefit is not
   // worth the overhead, considering namespace collisions are rare in
   // Doom wads.

   while(i >= 0 && (strncasecmp(lumpinfo[i]->name, name, 8) ||
         lumpinfo[i]->li_namespace != li_namespace))
      i = lumpinfo[i]->next;

   // Return the matching lump, or -1 if none found.   
   return i;
}

//
// W_InitLumpHash
//
// killough 1/31/98: Initialize lump hash table
//
void W_InitLumpHash(void)
{
   int i;
   
   for(i = 0; i < numlumps; i++)
      lumpinfo[i]->index = -1;                     // mark slots empty

   // Insert nodes to the beginning of each chain, in first-to-last
   // lump order, so that the last lump of a given name appears first
   // in any chain, observing pwad ordering rules. killough

   for(i = 0; i < numlumps; i++)
   {                                           // hash function:
      int j = W_LumpNameHash(lumpinfo[i]->name) % (unsigned)numlumps;
      lumpinfo[i]->next = lumpinfo[j]->index;     // Prepend to list
      lumpinfo[j]->index = i;
   }
}

// End of lump hashing -- killough 1/31/98

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(const char* name)     // killough -- const added
{
   int i = W_CheckNumForName(name);
   if(i == -1)
      I_Error("W_GetNumForName: %.8s not found!", name); // killough .8 added
   return i;
}

static void W_InitResources(void)          // sf
{
   //jff 1/23/98
   // get all the sprites and flats into one marked block each
   // killough 1/24/98: change interface to use M_START/M_END explicitly
   // killough 4/17/98: Add namespace tags to each entry
   
   W_CoalesceMarkedResource("S_START", "S_END", ns_sprites);
   
   W_CoalesceMarkedResource("F_START", "F_END", ns_flats);

   // killough 4/4/98: add colormap markers
   W_CoalesceMarkedResource("C_START", "C_END", ns_colormaps);
   
   // haleyjd 01/12/04: add translation markers
   W_CoalesceMarkedResource("T_START", "T_END", ns_translations);

   // set up caching
   // sf: caching now done in the lumpinfo_t's
   
   // killough 1/31/98: initialize lump hash table
   W_InitLumpHash();
}

//
// W_AddPredefines
//
// Predefined lumps are no longer in the executable.
// This now just initializes lumpinfo.
//
void W_AddPredefines(void)
{
   // predefined lumps removed now
   numlumps = 0;
   
   lumpinfo = Z_Malloc(1, PU_STATIC, 0);
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

void W_InitMultipleFiles(char *const *filenames)
{
   // killough 1/31/98: add predefined lumps first
   
   //sf : move predefine adding to a separate function
   W_AddPredefines();

   // haleyjd 09/13/03: set new sound lump parsing to deferred
   w_sound_update_type = 0;
   
   // open all the files, load headers, and count lumps
   while(*filenames)
      W_AddFile(*filenames++);
   
   if(!numlumps)
      I_Error("W_InitFiles: no files found");
   
   W_InitResources();
}

int W_AddNewFile(char *filename)
{
   // haleyjd 09/13/03: use S_UpdateSound here
   w_sound_update_type = 1;
   if(W_AddFile(filename)) 
      return true;
   W_InitResources();              // reinit lump lookups etc
   return false;
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
   if(lump < 0 || lump >= numlumps)
      I_Error("W_LumpLength: %i >= numlumps", lump);
   return lumpinfo[lump]->size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(int lump, void *dest)
{
   // haleyjd 08/30/02: access is potentially too early!
   lumpinfo_t *l; // = lumpinfo[lump];
   
   if(lump < 0 || lump >= numlumps)
      I_Error("W_ReadLump: %i >= numlumps",lump);

   // haleyjd: NOW we can do it
   l = lumpinfo[lump];
   
   if(l->data)     // killough 1/31/98: predefined lump data
      memcpy(dest, l->data, l->size);
   else
   {
      int c;
      
      // killough 1/31/98: Reload hack (-wart) removed
      // killough 10/98: Add flashing disk indicator
      I_BeginRead();
      lseek(l->handle, l->position, SEEK_SET);
      c = read(l->handle, dest, l->size);
      if(c < l->size)
         I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);
      I_EndRead();
   }
}

//
// W_ReadLumpHeader
//
// haleyjd 08/30/02: Inspired by DOOM Legacy, this function is
// for when you just need a small piece of data from the beginning
// of a lump. There's hardly any use reading in the whole thing
// in that case.
//
int W_ReadLumpHeader(int lump, void *dest, int size)
{
   lumpinfo_t *l;
   
   if(lump < 0 || lump >= numlumps)
      I_Error("W_ReadLumpHeader: %i >= numlumps",lump);
   
   l = lumpinfo[lump];

   if(l->size < size || l->size == 0)
      return 0;

   if(l->data)
   {
      memcpy(dest, l->data, size);
      return size;
   }
   else
   {
      int c;
      
      I_BeginRead();
      lseek(l->handle, l->position, SEEK_SET);
      c = read(l->handle, dest, size);
      if(c < size)
         I_Error("W_ReadLumpHeader: only read %i of %i on lump %i", c, size, lump);
      I_EndRead();

      return c;
   }
}

//
// W_CacheLumpNum
//
// killough 4/25/98: simplified
//
void *W_CacheLumpNum(int lump, int tag)
{
   // haleyjd 08/14/02: again, should not be RANGECHECK only
   if(lump < 0 || lump >= numlumps)
      I_Error("W_CacheLumpNum: %i >= numlumps", lump);
   
   if(!lumpinfo[lump]->cache)      // read the lump in
   {
      W_ReadLump(lump, Z_Malloc(W_LumpLength(lump), tag, &lumpinfo[lump]->cache));
   }
   else
   {
      // haleyjd: do not lower cache level and cause static
      // users to lose their data unexpectedly
      int oldtag = Z_CheckTag(lumpinfo[lump]->cache);

      if(tag < oldtag) 
         Z_ChangeTag(lumpinfo[lump]->cache, tag);
   }
   
   return lumpinfo[lump]->cache;
}

// W_CacheLumpName macroized in w_wad.h -- killough

// Predefined lumps removed -- sf

//
// W_LumpCheckSum
//
// sf
//
long W_LumpCheckSum(int lumpnum)
{
   int i, lumplength;
   char *lump;
   long checksum = 0;
   
   lump = W_CacheLumpNum(lumpnum, PU_CACHE);
   lumplength = W_LumpLength(lumpnum);
   
   for(i = 0; i < lumplength; i++)
      checksum += lump[i] * i;
   
   return checksum;
}

//----------------------------------------------------------------------------
//
// $Log: w_wad.c,v $
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

