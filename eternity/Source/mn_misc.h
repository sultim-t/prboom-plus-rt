// Emacs style mode select -*- C++ -*- 
//----------------------------------------------------------------------------
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
// Misc menu stuff
//
// By Simon Howard 
//
//----------------------------------------------------------------------------

#ifndef __MN_MISC_H__
#define __MN_MISC_H__

// pop-up messages

void MN_Alert(char *message, ...);
void MN_Question(char *message, char *command);
void MN_QuestionFunc(char *message, void (*handler)(void));

// help screens

void MN_StartHelpScreen();

// map colour selection

void MN_SelectColour(char *variable_name);

#endif /** __MN_MISC_H__ **/

// EOF
