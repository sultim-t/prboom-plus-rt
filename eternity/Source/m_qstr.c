// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley
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
// Reallocating string structure
//
// What this "class" guarantees:
// * The string will always be null-terminated
// * Indexing functions always check array bounds
// * Insertion functions always reallocate when needed
//
// Of course, using M_QStrBuffer can negate these, so avoid it
// except for passing a char * to read op functions.
//
// By James Haley
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "i_system.h"
#include "m_qstr.h"
#include "m_misc.h" // for M_Strupr/M_Strlwr

// 32 bytes is the minimum block size currently used by the zone 
// allocator, so it makes sense to use it as the base default 
// string size too.

#define QSTR_BASESIZE 32

//
// M_QStrInitCreate
//
// Initializes a qstring struct to all zeroes, then calls
// M_QStrCreate. This is for safety the first time a qstring
// is created (if qstr->buffer is uninitialized, realloc will
// crash).
//
qstring_t *M_QStrInitCreate(qstring_t *qstr)
{
   memset(qstr, 0, sizeof(*qstr));

   return M_QStrCreate(qstr);
}

//
// M_QStrCreateSize
//
// Creates a qstring with a given initial size, which helps prevent
// unnecessary initial reallocations. Resets insertion point to zero.
// This is safe to call on an existing qstring to reinitialize it.
//
qstring_t *M_QStrCreateSize(qstring_t *qstr, unsigned int size)
{
   qstr->buffer = realloc(qstr->buffer, size);
   qstr->size   = size;
   qstr->index  = 0;
   memset(qstr->buffer, 0, QSTR_BASESIZE);

   return qstr;
}

//
// M_QStrCreate
//
// Gives the qstring a buffer of the default size and initializes
// it to zero. Resets insertion point to zero. This is safe to call
// on an existing qstring to reinitialize it.
//
qstring_t *M_QStrCreate(qstring_t *qstr)
{
   return M_QStrCreateSize(qstr, QSTR_BASESIZE);
}

//
// M_QStrLen
//
// Calls strlen on the internal buffer. Convenience method.
//
unsigned int M_QStrLen(qstring_t *qstr)
{
   return strlen(qstr->buffer);
}

//
// M_QStrSize
//
// Returns the amount of size allocated for this qstring
// (will be >= strlen). You are allowed to index into the
// qstring up to size - 1, although any bytes beyond the
// strlen will be zero.
//
unsigned int M_QStrSize(qstring_t *qstr)
{
   return qstr->size;
}

//
// M_QStrBuffer
//
// Retrieves a pointer to the internal buffer. This pointer
// shouldn't be cached, and is not meant for writing into
// (although it is safe to do so, it circumvents the
// encapsulation and security of this structure).
//
char *M_QStrBuffer(qstring_t *qstr)
{
   return qstr->buffer;
}

//
// M_QStrGrow
//
// Grows the qstring's buffer by the indicated amount.
// This is automatically called by other qstring methods,
// so there is generally no need to call it yourself.
//
qstring_t *M_QStrGrow(qstring_t *qstr, unsigned int len)
{   
   int newsize = qstr->size + len;

   qstr->buffer = realloc(qstr->buffer, newsize);
   memset(qstr->buffer + qstr->size, 0, len);
   qstr->size += len;
   
   return qstr;
}

//
// M_QStrClear
//
// Sets the entire qstring buffer to zero, and resets the
// insertion index. Does not reallocate the buffer.
//
qstring_t *M_QStrClear(qstring_t *qstr)
{
   memset(qstr->buffer, 0, qstr->size);
   qstr->index = 0;

   return qstr;
}

//
// M_QStrFree
//
// Frees the qstring object. It should not be used after this,
// unless M_QStrCreate is called on it. You don't have to free
// a qstring before recreating it, however, since it uses realloc.
//
void M_QStrFree(qstring_t *qstr)
{
   free(qstr->buffer);
   qstr->buffer = NULL;
   qstr->index = qstr->size = 0;
}

//
// M_QStrCharAt
//
// Indexing function to access a character in a qstring.
// This is slower but more secure than using M_QStrBuffer
// with array indexing.
//
char M_QStrCharAt(qstring_t *qstr, unsigned int idx)
{
   if(idx >= qstr->size)
      I_Error("M_QStrCharAt: index out of range\n");

   return qstr->buffer[idx];
}

//
// M_QStrPutc
//
// Adds a character to the end of the qstring, reallocating
// via buffer doubling if necessary.
//
qstring_t *M_QStrPutc(qstring_t *qstr, char ch)
{
   if(qstr->index >= qstr->size - 1) // leave room for \0
      M_QStrGrow(qstr, qstr->size);  // double buffer size

   qstr->buffer[qstr->index++] = ch;

   return qstr;
}

//
// M_QStrCat
//
// Concatenates a C string onto the end of a qstring, expanding
// the buffer if necessary.
//
qstring_t *M_QStrCat(qstring_t *qstr, const char *str)
{
   unsigned int cursize = qstr->size;
   unsigned int newsize = strlen(qstr->buffer) + strlen(str) + 1;

   if(newsize > cursize)
      M_QStrGrow(qstr, newsize - cursize);

   strcat(qstr->buffer, str);

   qstr->index = newsize - 1;

   return qstr;
}

//
// M_QStrUpr
//
// Converts the string to uppercase.
//
qstring_t *M_QStrUpr(qstring_t *qstr)
{
   M_Strupr(qstr->buffer);
   return qstr;
}

//
// M_QStrLwr
//
// Converts the string to lowercase.
//
qstring_t *M_QStrLwr(qstring_t *qstr)
{
   M_Strlwr(qstr->buffer);
   return qstr;
}

// EOF

