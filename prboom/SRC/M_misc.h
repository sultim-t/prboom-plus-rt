// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: M_misc.h,v 1.1 2000/04/09 18:00:44 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
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
//
//    
//-----------------------------------------------------------------------------

#ifndef __M_MISC__
#define __M_MISC__

#include "doomtype.h"

//
// MISC
//

H_boolean M_WriteFile(const char *name, void *source, int length);
int M_ReadFile(const char *name, byte **buffer);
void M_ScreenShot(void);
void M_LoadDefaults(void);
void M_SaveDefaults(void);
int M_DrawText(int x,int y,H_boolean direct, char *string);
struct default_s *M_LookupDefault(const char *name);     // killough 11/98
H_boolean M_ParseOption(const char *name, H_boolean wad);    // killough 11/98
void M_LoadOptions(void);                                // killough 11/98

extern int screenshot_pcx;                               // killough 10/98

// phares 4/21/98:
// Moved from m_misc.c so m_menu.c could see it.
//
// killough 11/98: totally restructured

typedef struct default_s
{
  const char *const name;                   // name
  int  *const location;                     // default variable
  int  *const current;                      // possible nondefault variable
  int   const defaultvalue;                 // built-in default value
  struct {int min, max;} const limit;       // numerical limits
  enum {number, string} const isstr;        // number or string
  ss_types const setupscreen;               // setup screen this appears on
  enum {wad_no, wad_yes} const wad_allowed; // whether it's allowed in wads
  const char *const help;                   // description of parameter

  // internal fields (initialized implicitly to 0) follow

  struct default_s *first, *next;           // hash table pointers
  int modified;                             // Whether it's been modified
  int orig_default;                         // Original default, if modified
  struct setup_menu_s *setup_menu;          // Xref to setup menu item, if any
} default_t;

#define UL (-123456789) /* magic number for no min or max for parameter */

#endif

//----------------------------------------------------------------------------
//
// $Log: M_misc.h,v $
// Revision 1.1  2000/04/09 18:00:44  proff_fs
// Initial revision
//
// Revision 1.4  1998/05/05  19:56:06  phares
// Formatting and Doc changes
//
// Revision 1.3  1998/04/22  13:46:17  phares
// Added Setup screen Reset to Defaults
//
// Revision 1.2  1998/01/26  19:27:12  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
