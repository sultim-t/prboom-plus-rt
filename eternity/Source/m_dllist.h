// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//  Generalized Double-linked List Routines
//
// haleyjd 08/05/05: This is Lee Killough's smart double-linked list
// implementation with pointer-to-pointer prev links, generalized to
// be able to work with any structure. This type of double-linked list 
// can only be traversed from head to tail, but it treats all nodes 
// uniformly even without the use of a dummy head node, and thus it is 
// very efficient. These routines are inlined for maximum speed.
//
// Just put an mdllistitem_t as the first item in a structure and you
// can then cast a pointer to that structure to mdllistitem_t * and
// pass it to these routines. You are responsible for defining the 
// pointer used as the head of the list.
//    
//-----------------------------------------------------------------------------

#ifndef M_DLLIST_H__
#define M_DLLIST_H__

#include "d_keywds.h"

typedef struct mdllistitem_s
{
   struct mdllistitem_s *next;
   struct mdllistitem_s **prev;
} mdllistitem_t;

d_inline static void M_DLListInsert(mdllistitem_t *item, mdllistitem_t **head)
{
   mdllistitem_t *next = *head;

   if((item->next = next))
      next->prev = &item->next;
   item->prev = head;
   *head = item;
}

d_inline static void M_DLListRemove(mdllistitem_t *item)
{
   mdllistitem_t **prev = item->prev;
   mdllistitem_t *next  = item->next;
   
   if((*prev = next))
      next->prev = prev;
}

#endif

// EOF

