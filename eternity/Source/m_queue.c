// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
// General queue code
//
// By James Haley
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "m_queue.h"

//
// M_QueueInit
//
// Sets up a queue. Can be called again to reset a used queue
// structure.
//
void M_QueueInit(mqueue_t *queue)
{
   queue->head.next = NULL;
   queue->tail = &(queue->head);
   queue->rover = &(queue->head);
}

//
// M_QueueInsert
//
// Inserts the given item into the queue.
//
void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue)
{
   // link in at the tail (this works even for the first node!)
   queue->tail = queue->tail->next = item;
}

//
// M_QueueIterator
//
// Returns the next item in the queue each time it is called,
// or NULL once the end is reached. The iterator can be reset
// using M_QueueResetIterator.
//
mqueueitem_t *M_QueueIterator(mqueue_t *queue)
{
   if(queue->rover == NULL)
      return NULL;
      
   return (queue->rover = queue->rover->next);
}

//
// M_QueueResetIterator
//
// Returns the queue iterator to the beginning.
//
void M_QueueResetIterator(mqueue_t *queue)
{
   queue->rover = &(queue->head);
}

//
// M_QueueFree
//
// Frees all the elements in the queue
//
void M_QueueFree(mqueue_t *queue)
{
   mqueueitem_t *rover = queue->head.next;

   while(rover)
   {
      mqueueitem_t *next = rover->next;
      free(rover);

      rover = next;
   }

   M_QueueInit(queue);
}

// EOF

