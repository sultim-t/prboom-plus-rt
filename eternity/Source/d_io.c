// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
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
//----------------------------------------------------------------------------
//
// File/WAD Standard Input Routines
//
// This code was moved here from d_deh.c and generalized to make
// a consistent interface for emulating stdio functions on wad lumps.
// The structure here handles either external files or wad lumps,
// depending on how its initialized, and emulates stdio regardless.
//
// As with FILE pointers, DWFILE structures should be treated as
// opaque objects. All manipulation should be achieved through
// the global routines here.
//
// James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_keywds.h"
#include "doomtype.h"
#include "d_io.h"
#include "w_wad.h"

char *D_Fgets(char *buf, size_t n, DWFILE *fp)
{
   // If this is a real file, return regular fgets
   if(!fp->lump)
      return fgets(buf, n, (FILE *) fp->inp);
   
   // If no more characters
   if(!n || !*fp->inp || fp->size<=0)
      return NULL;
  
   if(n == 1)
   {
      fp->size--, *buf = *fp->inp++;
   }
   else
   {  // copy buffer
      char *p = buf;
      while(n > 1 && *fp->inp && fp->size &&
            (n--, fp->size--, *p++ = *fp->inp++) != '\n')
         ;
      *p = 0;
   }
   return buf; // Return buffer pointer
}

int D_Feof(DWFILE *fp)
{
   return !fp->lump ? feof((FILE *)fp->inp) : 
                      !*fp->inp || fp->size <= 0;
}

int D_Fgetc(DWFILE *fp)
{
   return !fp->lump ? fgetc((FILE *) fp->inp) : fp->size > 0 ?
      fp->size--, *fp->inp++ : EOF;
}


//
// D_Ungetc
//
// haleyjd 04/03/03: note that wad lump buffers will not be 
// written into by this function -- this is necessary to
// maintain cacheability.
//
int D_Ungetc(int c, DWFILE *fp)
{
   return !fp->lump ? ungetc(c, (FILE *)fp->inp) :
              fp->size < fp->origsize ?
                 fp->size++, *(--fp->inp) : EOF;
}

//
// D_OpenFile
//
// Open a file into the DWFILE structure. Uses standard
// fopen.
//
void D_OpenFile(DWFILE *infile, const char *filename, char *mode)
{
   infile->inp  = (byte *)fopen(filename, mode);
   infile->lump = NULL;
}

//
// D_OpenLump
//
// Open a wad lump into the DWFILE structure. The wad lump will
// be cached in zone memory at static allocation level.
//
void D_OpenLump(DWFILE *infile, int lumpnum)
{
   // haleyjd 04/03/03: added origsize field for D_Ungetc

   infile->size = infile->origsize = W_LumpLength(lumpnum);
   infile->inp = infile->lump = W_CacheLumpNum(lumpnum, PU_STATIC);
}

//
// D_Fclose
//
// Closes the file or wad lump.  Calls standard fclose for files;
// sets wad lumps to cache allocation level.
//
void D_Fclose(DWFILE *dwfile)
{
   if(!D_IsOpen(dwfile))
      return;

   if(dwfile->lump)
   {
      Z_ChangeTag(dwfile->lump, PU_CACHE);
   }
   else
   {
      fclose((FILE *)dwfile->inp);
   }

   dwfile->inp = dwfile->lump = NULL;
}

//
// D_Fread
//
// haleyjd
//
size_t D_Fread(void *dest, size_t size, size_t num, DWFILE *file)
{
   if(!file->lump)
      return fread(dest, size, num, (FILE *)(file->inp));
   else
   {
      size_t numbytes = size * num;
      size_t numbytesread = 0;
      byte *d = (byte *)dest;

      while(numbytesread < numbytes && file->size)
      {
         *d++ = *file->inp++;
         file->size--;
         numbytesread++;
      }

      return numbytesread;
   }
}

// EOF

