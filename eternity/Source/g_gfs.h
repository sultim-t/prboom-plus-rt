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
//
// GFS -- Game File Scripts
//
// This is a radical new way for DOOM editing projects to provide
// file information to Eternity. A single GFS script, when provided
// via the -gfs command-line parameter, will affect the loading of
// an arbitrary number of WADs, DEH/BEX's, EDF files, and console
// scripts.
//
// By James Haley
//
//--------------------------------------------------------------------------

#ifndef __G_GFS_H__
#define __G_GFS_H__

#include "doomtype.h"
#include "d_io.h"

typedef struct gfs_s
{
   char **wadnames;
   char **dehnames;
   char **cscnames;

   char iwad[PATH_MAX + 1];
   char filepath[PATH_MAX + 1];
   char edf[PATH_MAX + 1];

   int numwads;
   int numdehs;
   int numcsc;
   boolean hasIWAD;
   boolean hasEDF;
} gfs_t;

gfs_t *G_LoadGFS(const char *filename);
void G_FreeGFS(gfs_t *gfs);
const char *G_GFSCheckIWAD(void);
const char *G_GFSCheckEDF(void);

#endif

// EOF

