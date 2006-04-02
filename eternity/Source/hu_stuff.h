// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"

enum
{
   WIDGET_MISC,
   WIDGET_PATCH,
   WIDGET_TEXT,
};

// haleyjd 06/04/05: HUD rewrite

typedef struct hu_widget_s
{
   // overridable functions (virtuals in a sense)

   void (*ticker)(struct hu_widget_s *); // ticker: called each gametic
   void (*drawer)(struct hu_widget_s *); // drawer: called when drawn
   void (*eraser)(struct hu_widget_s *); // eraser: called when erased
   void (*clear) (struct hu_widget_s *); // clear : called on reinit

   // id data
   int type;                 // widget type
   char name[33];            // name of this widget
   struct hu_widget_s *next; // next in hash chain
   boolean disabled;         // disable flag
   boolean prevdisabled;     // previous state of disable flag
} hu_widget_t;

extern int show_vpo;
extern boolean chat_on;
extern int obituaries;
extern int obcolour;       // the colour of death messages
extern int showMessages;   // Show messages has default, 0 = off, 1 = on
extern int mess_colour;    // the colour of normal messages
extern char *chat_macros[10];

void HU_Init(void);
void HU_Drawer(void);
void HU_Ticker(void);
boolean HU_Responder(event_t *ev);

void HU_Start(void);

void HU_WriteText(const char *s, int x, int y);
void HU_PlayerMsg(const char *s);
void HU_CenterMessage(const char *s);
void HU_Erase(void);

#define CROSSHAIRS 3
extern int crosshairnum;       // 0= none
extern boolean crosshair_hilite;

#endif

// EOF
