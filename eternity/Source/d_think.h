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
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//
//-----------------------------------------------------------------------------

#ifndef __D_THINK__
#define __D_THINK__

// killough 11/98: convert back to C instead of C++
typedef  void (*actionf_t)();

// Historically, "think_t" is yet another function 
// pointer to a routine to handle an actor.
typedef actionf_t think_t;

// Doubly linked list of actors.
typedef struct thinker_s
{
  struct thinker_s *prev, *next;
  think_t function;
  
  // killough 8/29/98: we maintain thinkers in several equivalence classes,
  // according to various criteria, so as to allow quicker searches.

  struct thinker_s *cnext, *cprev; // Next, previous thinkers in same class

  // killough 11/98: count of how many other objects reference
  // this one using pointers. Used for garbage collection.
  unsigned references;
} thinker_t;

#endif

//----------------------------------------------------------------------------
//
// $Log: d_think.h,v $
// Revision 1.3  1998/05/04  21:34:20  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:34  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
