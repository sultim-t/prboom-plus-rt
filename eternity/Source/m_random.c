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
//      Random number LUT.
//
// 1/19/98 killough: Rewrote random number generator for better randomness,
// while at the same time maintaining demo sync and backward compatibility.
//
// 2/16/98 killough: Made each RNG local to each control-equivalent block,
// to reduce the chances of demo sync problems.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: m_random.c,v 1.6 1998/05/03 23:13:18 killough Exp $";

#include "doomstat.h"
#include "m_random.h"
#include "a_small.h"

//
// M_Random
// Returns a 0-255 number
//
const unsigned char rndtable[256] = { // 1/19/98 killough -- made const
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

int demo_insurance=0, default_demo_insurance=0;   // killough 3/31/98

rng_t rng;     // the random number state

unsigned long rngseed = 1993;   // killough 3/26/98: The seed

int P_Random(pr_class_t pr_class)
{
  // killough 2/16/98:  We always update both sets of random number
  // generators, to ensure repeatability if the demo_compatibility
  // flag is changed while the program is running. Changing the
  // demo_compatibility flag does not change the sequences generated,
  // only which one is selected from.
  //
  // All of this RNG stuff is tricky as far as demo sync goes --
  // it's like playing with explosives :) Lee

  int compat;

  unsigned long boom;

  // killough 3/31/98:
  // If demo sync insurance is not requested, use
  // much more unstable method by putting everything
  // except pr_misc into pr_all_in_one


      compat= pr_class == pr_misc ?     // sf: moved here
            (rng.prndindex = (rng.prndindex + 1) & 255) :
            (rng.rndindex  = (rng.rndindex  + 1) & 255) ;

  if (pr_class != pr_misc && !demo_insurance)      // killough 3/31/98
    pr_class = pr_all_in_one;

  boom = rng.seed[pr_class];

  // killough 3/26/98: add pr_class*2 to addend

  rng.seed[pr_class] = boom * 1664525ul + 221297ul + pr_class*2;

  if (demo_compatibility)
    return rndtable[compat];

  boom >>= 20;

  // killough 3/30/98: use gametic-levelstarttic to shuffle RNG
  // killough 3/31/98: but only if demo insurance requested,
  // since it's unnecessary for random shuffling otherwise
  // killough 9/29/98: but use basetic now instead of levelstarttic

  if (demo_insurance)
    boom += (gametic-basetic)*7;

  return boom & 255;
}

//
// P_SubRandom
//
// haleyjd 08/05/04: This function eliminates the need to use
// temporary variables everywhere in order to subtract two random
// values without incurring order of evaluation problems.
//
int P_SubRandom(pr_class_t pr_class)
{
   int temp = P_Random(pr_class);

   return (temp - P_Random(pr_class));
}

// Initialize all the seeds
//
// This initialization method is critical to maintaining demo sync.
// Each seed is initialized according to its class, so if new classes
// are added they must be added to end of pr_class_t list. killough
//

void M_ClearRandom (void)
{
  int i;
  unsigned long seed = rngseed*2+1;    // add 3/26/98: add rngseed
  for (i=0; i<NUMPRCLASS; i++)         // go through each pr_class and set
    rng.seed[i] = seed *= 69069ul;     // each starting seed differently
  rng.prndindex = rng.rndindex = 0;    // clear two compatibility indices
}

static cell AMX_NATIVE_CALL sm_random(AMX *amx, cell *params)
{
   return P_Random(pr_script);
}

//
// sm_mrandom
//
// Implements M_Random()
// 
// This is strictly for non-demo-sync-critical stuff such as
// particle effects.
//
static cell AMX_NATIVE_CALL sm_mrandom(AMX *amx, cell *params)
{
   return M_Random();
}

AMX_NATIVE_INFO random_Natives[] =
{
   { "_P_Random",   sm_random },
   { "_M_Random", sm_mrandom },
   { NULL, NULL }
};

//----------------------------------------------------------------------------
//
// $Log: m_random.c,v $
// Revision 1.6  1998/05/03  23:13:18  killough
// Fix #include
//
// Revision 1.5  1998/03/31  10:43:05  killough
// Fix (supposed) RNG problems, add new demo_insurance
//
// Revision 1.4  1998/03/28  17:56:05  killough
// Improve RNG by adding external seed
//
// Revision 1.3  1998/02/17  05:40:08  killough
// Make RNGs local to each calling block, for demo sync
//
// Revision 1.2  1998/01/26  19:23:51  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
