// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2001 James Haley
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
// Dialog Scripting System (DSS) Interpreter
//
// Reads lumps and drives character dialog.
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef __D_DIALOG_H__
#define __D_DIALOG_H__

#include "doomtype.h"

typedef struct runningdialog_s
{
   int  *ip;          // instruction pointer
   char *stringtable; // pointer to beginning of string table
	
   char lump[9];      // name of lump from which this sequence is loaded
   void *cache;       // cache to store lump
   
   int  playernum;    // number of player that triggered this sequence
   
   int  waittime;     // if non-zero, the ticker will not proceed
                      // but will count down to zero -- if
                      // negative, the ticker will wait 
                      // indefinitely (for the renderer)

   int  waittype;     // describes the reason the dialog is waiting

} runningdialog_t;

extern runningdialog_t *currentdialog;
extern boolean dlginit;

void DLG_Init(void);
void DLG_Start(int, const char *, const char *, const char *);
void DLG_Stop(void);
void DLG_SetBGTrans(boolean);
void DLG_SetTextColor(int);
void DLG_SetBGColor(int);
void DLG_Ticker(void);
void DLG_Drawer(void);

#endif

// EOF
