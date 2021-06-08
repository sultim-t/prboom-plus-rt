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
//      unicode paths for fopen() on Windows

#ifndef __WIN_D_FOPEN__
#define __WIN_D_FOPEN__

#ifdef _WIN32
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <direct.h>

FILE* D_fopen(const char *filename, const char *mode);
int D_remove(const char *path);
int D_stat(const char *path, struct stat *buffer);
int D_open(const char *filename, int oflag);
int D_access(const char *path, int mode);
int D_mkdir(const char *dirname);

#undef  fopen
#define fopen(n, m) D_fopen(n, m)

#undef  remove
#define remove(p) D_remove(p)

#undef  stat
#define stat(p, b) D_stat(p, b)

#undef  open
#define open(n, of) D_open(n, of)

#undef  access
#define access(p, m) D_access(p, m)

#undef  mkdir
#define mkdir(d) D_mkdir(d)
#endif

#endif
