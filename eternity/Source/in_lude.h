// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
// 
// Shared intermission code
//
//-----------------------------------------------------------------------------

#ifndef __IN_LUDE_H__
#define __IN_LUDE_H__

// intercam
#define MAXCAMERAS 128

extern int intertime;
extern int acceleratestage;

extern mobj_t **camerathings;
extern mobj_t *wi_camera;
extern int numcameras;

void IN_AddCameras(void);
void IN_slamBackground(void);
void IN_checkForAccelerate(void);
void IN_Ticker(void);
void IN_Drawer(void);
void IN_Start(wbstartstruct_t *wbstartstruct);

#endif

// EOF
