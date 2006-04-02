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
// 
// Shared intermission code
//
//-----------------------------------------------------------------------------

#ifndef IN_LUDE_H__
#define IN_LUDE_H__

// Intermission object struct
typedef struct interfns_s
{
   void (*Ticker)(void);         // called by IN_Ticker
   void (*DrawBackground)(void); // called various places
   void (*Drawer)(void);         // called by IN_Drawer
   void (*Start)(wbstartstruct_t *wbstartstruct); // called by IN_Start
} interfns_t;

// intercam
#define MAXCAMERAS 128

extern int intertime;
extern int acceleratestage;

extern MobjCollection camerathings;
extern mobj_t *wi_camera;

void IN_AddCameras(void);
void IN_slamBackground(void);
void IN_checkForAccelerate(void);
void IN_Ticker(void);
void IN_Drawer(void);
void IN_DrawBackground(void);
void IN_Start(wbstartstruct_t *wbstartstruct);

#endif

// EOF
