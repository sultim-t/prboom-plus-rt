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
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __PREPRO_H__
#define __PREPRO_H__

typedef struct section_s section_t;
typedef struct label_s label_t;
#define SECTIONSLOTS 17
#define LABELSLOTS 17

#include "t_parse.h"

void preprocess(script_t *script);

/***** {} sections **********/

section_t *find_section_start(char *brace);
section_t *find_section_end(char *brace);

struct section_s
{
  char *start;    // offset of starting brace {
    char *end;      // offset of ending brace   }
  int type;       // section type: for() loop, while() loop etc
  
  union
  {
    struct
    {
      char *loopstart;  // positioned before the while()
    } data_loop;
  } data; // data for section
  
  section_t *next;        // for hashing
};

enum    // section types
{
  st_empty,       // none: empty {} braces
  st_if,          // if() statement             
  st_loop,        // loop
};

/****** goto labels ***********/

label_t *labelforname(char *labelname);

#endif
