// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details 
// Copyright (C) 2003 James Haley
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
//    fcvt from DJGPP libc and supporting functions, to solve
//    problem with psnprintf. ::SIGH::
//
//-----------------------------------------------------------------------------

#ifndef __M_FCVT_H__
#define __M_FCVT_H__

#ifdef NO_FCVT

char *M_Fcvt(double, int, int *, int *);
char *M_Fcvtbuf(double, int, int *, int *, char *);
void M_Ecvround(char *, char *, const char *, int *);
char *M_Ecvtbuf(double, int, int *, int *, char *);

#endif /* def NO_FCT */

#endif /* ndef __M_FCVT_H__ */

// EOF

