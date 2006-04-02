// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
// EDF Strings Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef E_STRING_H__
#define E_STRING_H__

#include "m_dllist.h"

typedef struct edf_string_s
{
   mdllistitem_t numlinks;    // next, prev links for numeric hash chain
   struct edf_string_s *next; // next in mnemonic hash chain

   char *string;       // string value
   char key[33];       // mnemonic for hashing
   int  numkey;        // number for hashing
   
} edf_string_t;

edf_string_t *E_CreateString(const char *, const char *, int);
edf_string_t *E_StringForName(const char *key);
edf_string_t *E_GetStringForName(const char *key);
edf_string_t *E_StringForNum(int num);
edf_string_t *E_GetStringForNum(int num);

#ifdef NEED_EDF_DEFINITIONS
#define EDF_SEC_STRING "string"
extern cfg_opt_t edf_string_opts[];

void E_ProcessStrings(cfg_t *cfg);
#endif

#endif

// EOF

