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

#ifndef M_QUEUE_H
#define M_QUEUE_H

typedef struct mqueueitem_s
{
   struct mqueueitem_s *next;
} mqueueitem_t;

typedef struct mqueue_s
{
   mqueueitem_t head;
   mqueueitem_t *tail;
   mqueueitem_t *rover;
} mqueue_t;

void M_QueueInit(mqueue_t *queue);
void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue);
mqueueitem_t *M_QueueIterator(mqueue_t *queue);
void M_QueueResetIterator(mqueue_t *queue);
void M_QueueFree(mqueue_t *queue);

#endif

// EOF

