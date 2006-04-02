// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
//  Macros to resolve compiler-dependent keywords and language
//  extensions.
//
//-----------------------------------------------------------------------------

#ifndef D_KEYWDS_H__
#define D_KEYWDS_H__

// attribute hints are available only under GNU C

#ifndef __GNUC__
#define __attribute__(x)
#endif

// function inlining is available on most platforms, however,
// the GNU C __inline__ is too common and conflicts with a 
// definition in SDL, so it needs to be factored out into a 
// custom macro definition

#if defined(__GNUC__)
  #define d_inline __inline__
#elif defined(_MSC_VER)
  #define d_inline __inline
#else
  #define d_inline
#endif

//
// Non-standard function availability defines
//
// These control the presence of custom code for some common
// non-ANSI libc functions that are in m_misc.c -- in some cases
// its not worth having the code when its already available in 
// the platform's libc
//

// HAVE_ITOA -- define this if your platform has ITOA
// ITOA_NAME -- name of your platform's itoa function
// #define HAVE_ITOA
// #define ITOA_NAME itoa

// known platforms with itoa -- you can add yours here, but it
// isn't absolutely necessary
#ifndef HAVE_ITOA
  #ifdef DJGPP
    #define HAVE_ITOA
    #define ITOA_NAME itoa
  #endif
  #ifdef _MSC_VER
    #define HAVE_ITOA
    #define ITOA_NAME _itoa
  #endif
#endif

#endif // D_KEYWDS_H__

// EOF
