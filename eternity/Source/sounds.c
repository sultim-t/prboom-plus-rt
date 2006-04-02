// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//
// DESCRIPTION:
//      Created by a sound utility.
//      Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: sounds.c,v 1.3 1998/05/03 22:44:25 killough Exp $";

// killough 5/3/98: reformatted

#include "doomtype.h"
#include "sounds.h"
#include "p_skin.h"

//
// Information about all the music
//

musicinfo_t S_music[] = {
  { 0 },
  { "e1m1" },
  { "e1m2" },
  { "e1m3" },
  { "e1m4" },
  { "e1m5" },
  { "e1m6" },
  { "e1m7" },
  { "e1m8" },
  { "e1m9" },
  { "e2m1" },
  { "e2m2" },
  { "e2m3" },
  { "e2m4" },
  { "e2m5" },
  { "e2m6" },
  { "e2m7" },
  { "e2m8" },
  { "e2m9" },
  { "e3m1" },
  { "e3m2" },
  { "e3m3" },
  { "e3m4" },
  { "e3m5" },
  { "e3m6" },
  { "e3m7" },
  { "e3m8" },
  { "e3m9" },
  { "inter" },
  { "intro" },
  { "bunny" },
  { "victor" },
  { "introa" },
  { "runnin" },
  { "stalks" },
  { "countd" },
  { "betwee" },
  { "doom" },
  { "the_da" },
  { "shawn" },
  { "ddtblu" },
  { "in_cit" },
  { "dead" },
  { "stlks2" },
  { "theda2" },
  { "doom2" },
  { "ddtbl2" },
  { "runni2" },
  { "dead2" },
  { "stlks3" },
  { "romero" },
  { "shawn2" },
  { "messag" },
  { "count2" },
  { "ddtbl3" },
  { "ampie" },
  { "theda3" },
  { "adrian" },
  { "messg2" },
  { "romer2" },
  { "tense" },
  { "shawn3" },
  { "openin" },
  { "evil" },
  { "ultima" },
  { "read_m" },
  { "dm2ttl" },
  { "dm2int" },
};

musicinfo_t H_music[] =
{
   { 0 },
   { "e1m1" }, // 1
   { "e1m2" }, // 2
   { "e1m3" }, // 3
   { "e1m4" }, // 4
   { "e1m5" }, // 5
   { "e1m6" }, // 6
   { "e1m7" }, // 7
   { "e1m8" }, // 8
   { "e1m9" }, // 9
   { "e2m1" }, // 10
   { "e2m2" }, // 11
   { "e2m3" }, // 12
   { "e2m4" }, // 13
   { "e2m6" }, // 14
   { "e2m7" }, // 15
   { "e2m8" }, // 16
   { "e2m9" }, // 17
   { "e3m2" }, // 18
   { "e3m3" }, // 19
   { "titl" }, // 20
   { "intr" }, // 21
   { "cptd" }, // 22
};

// haleyjd: a much more clever way of getting an index to the music
// above than what was used in Heretic
int H_Mus_Matrix[6][9] =
{
   {  1,  2,  3,  4,  5,  6,  7,  8,  9 }, // episode 1
   { 10, 11, 12, 13,  4, 14, 15, 16, 17 }, // episode 2
   {  1, 18, 19,  6,  3,  2,  5,  9, 14 }, // episode 3
   {  6,  2,  3,  4,  5,  1,  7,  8,  9 }, // episode 4 (sosr)
   { 10, 11, 12, 13,  4, 14, 15, 16, 17 }, // episode 5 (sosr)
   { 18, 19,  6, 20, 21, 22,  1,  1,  1 }, // hidden levels
};

//
// Information about all the sfx
//
// killough 12/98: 
// Reimplemented 'singularity' flag, adjusting many sounds below

// haleyjd 11/05/03: made dynamic with EDF
sfxinfo_t *S_sfx = NULL;
int NUMSFX = 0;

//----------------------------------------------------------------------------
//
// $Log: sounds.c,v $
// Revision 1.3  1998/05/03  22:44:25  killough
// beautification
//
// Revision 1.2  1998/01/26  19:24:54  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
