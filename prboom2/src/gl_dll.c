/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_dll.c,v 1.2 2001/06/17 18:49:50 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *
 *---------------------------------------------------------------------
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gl_dll.h"

DLL_HANDLE DLL_LoadLibrary(const char *name)
{
  return LoadLibrary(name);
}

void * DLL_GetProcAddress(DLL_HANDLE handle, const char *symbol)
{
  return GetProcAddress(handle, symbol);
}

char * DLL_ErrorMessage(void)
{
  return "unknown error";
}

#else

#include <dlfcn.h>
#include <stdio.h>
#include "gl_dll.h"

DLL_HANDLE DLL_LoadLibrary(const char *name)
{
  return dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
}

void * DLL_GetProcAddress(DLL_HANDLE handle, const char *symbol)
{
  return dlsym(handle, symbol);
}

char * DLL_ErrorMessage(void)
{
  return dlerror();
}

#endif
