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
 *-----------------------------------------------------------------------------
 */

#ifndef __C_IO_H__
#define __C_IO_H__

#include "doomstat.h"
#include "d_event.h"
        // for text colours:
#include "v_video.h"

#define INPUTLENGTH 512
#define LINELENGTH 96

void C_InitBackdrop();
void C_Init();
void C_Ticker();
void C_Drawer();
int C_Responder(event_t* ev);
void C_Update();

void C_Puts(const char *s);
void C_Printf(const char *s, ...);

void C_Seperator();

void C_SetConsole();
void C_Popup();
void C_InstaPopup();

        // sf 9/99: made a #define
#define consoleactive (current_height || gamestate==GS_CONSOLE)

extern int c_height;     // the height of the console
extern int c_speed;       // pixels/tic it moves
extern int current_height;
extern int current_target;
#define c_moving (current_height != current_target)
extern boolean c_showprompt;

#endif
