/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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
 * DESCRIPTION:  Head up display
 *
 *-----------------------------------------------------------------------------*/

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "v_video.h"

#define MAXHUDMESSAGES 16

#define MAXWIDGETS 16

typedef struct textwidget_s textwidget_t;

struct textwidget_s
{
  int x, y;       // co-ords on screen
  int font;       // 0 = normal red text 1 = heads up font
  char *message;
  void (*handler)();      // controller function
  int cleartic;   // gametic in which to clear the widget (0=never)
};

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
boolean HU_Responder(event_t* ev);
char HU_dequeueChatChar(void);

void HU_Start(void);
void HU_End(void);

void HU_WriteText(unsigned char *s, int x, int y);
void HU_PlayerMsg(char *s);
void HU_CentreMsg(char *s);
void HU_Erase(void);

#endif
