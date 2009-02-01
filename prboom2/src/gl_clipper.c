/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

/*
*
** gl_clipper.cpp
**
** Handles visibility checks.
** Loosely based on the JDoom clipper.
**
**---------------------------------------------------------------------------
** Copyright 2003 Tim Stump
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <SDL_opengl.h>
#include <math.h>
#include "v_video.h"
#include "gl_intern.h"
#include "e6y.h"

#define ANGLE_MAX   (0xffffffff)
#define ANG1        (ANG45/45)

typedef struct clipnode_s
{
  struct clipnode_s *prev, *next;
  angle_t start, end;
} clipnode_t;

clipnode_t *freelist;
clipnode_t *clipnodes;
clipnode_t *cliphead;

static clipnode_t * gld_clipnode_GetNew(void);
static clipnode_t * gld_clipnode_NewRange(angle_t start, angle_t end);
static dboolean gld_clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle);
static void gld_clipper_AddClipRange(angle_t start, angle_t end);
static void gld_clipper_RemoveRange(clipnode_t * range);
static void gld_clipnode_Free(clipnode_t *node);

static clipnode_t * gld_clipnode_GetNew(void)
{
  if (freelist)
  {
    clipnode_t * p = freelist;
    freelist = p->next;
    return p;
  }
  else
  {
    return malloc(sizeof(clipnode_t));
  }
}

static clipnode_t * gld_clipnode_NewRange(angle_t start, angle_t end)
{
  clipnode_t * c = gld_clipnode_GetNew();
  c->start = start;
  c->end = end;
  c->next = c->prev=NULL;
  return c;
}

dboolean gld_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle)
{
  if(startAngle > endAngle)
  {
    return (gld_clipper_IsRangeVisible(startAngle, ANGLE_MAX) || gld_clipper_IsRangeVisible(0, endAngle));
  }

  return gld_clipper_IsRangeVisible(startAngle, endAngle);
}

static dboolean gld_clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle)
{
  clipnode_t *ci;
  ci = cliphead;

  if (endAngle == 0 && ci && ci->start == 0)
    return false;

  while (ci != NULL && ci->start < endAngle)
  {
    if (startAngle >= ci->start && endAngle <= ci->end)
    {
      return false;
    }
    ci = ci->next;
  }

  return true;
}

static void gld_clipnode_Free(clipnode_t *node)
{
  node->next = freelist;
  freelist = node;
}

static void gld_clipper_RemoveRange(clipnode_t *range)
{
  if (range == cliphead)
  {
    cliphead = cliphead->next;
  }
  else
  {
    if (range->prev)
    {
      range->prev->next = range->next;
    }
    if (range->next)
    {
      range->next->prev = range->prev;
    }
  }

  gld_clipnode_Free(range);
}

void gld_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle)
{
  if(startangle > endangle)
  {
    // The range has to added in two parts.
    gld_clipper_AddClipRange(startangle, ANGLE_MAX);
    gld_clipper_AddClipRange(0, endangle);
  }
  else
  {
    // Add the range as usual.
    gld_clipper_AddClipRange(startangle, endangle);
  }
}

static void gld_clipper_AddClipRange(angle_t start, angle_t end)
{
  clipnode_t *node, *temp, *prevNode;
  if (cliphead)
  {
    //check to see if range contains any old ranges
    node = cliphead;
    while (node != NULL && node->start < end)
    {
      if (node->start >= start && node->end <= end)
      {
        temp = node;
        node = node->next;
        gld_clipper_RemoveRange(temp);
      }
      else
      {
        if (node->start <= start && node->end >= end)
        {
          return;
        }
        else
        {
          node = node->next;
        }
      }
    }
    //check to see if range overlaps a range (or possibly 2)
    node = cliphead;
    while (node != NULL)
    {
      if (node->start >= start && node->start <= end)
      {
        node->start = start;
        return;
      }
      if (node->end >= start && node->end <= end)
      {
        // check for possible merger
        if (node->next && node->next->start <= end)
        {
          node->end = node->next->end;
          gld_clipper_RemoveRange(node->next);
        }
        else
        {
          node->end = end;
        }

        return;
      }
      node = node->next;
    }
    //just add range
    node = cliphead;
    prevNode = NULL;
    temp = gld_clipnode_NewRange(start, end);
    while (node != NULL && node->start < end)
    {
      prevNode = node;
      node = node->next;
    }
    temp->next = node;
    if (node == NULL)
    {
      temp->prev = prevNode;
      if (prevNode)
      {
        prevNode->next = temp;
      }
      if (!cliphead)
      {
        cliphead = temp;
      }
    }
    else
    {
      if (node == cliphead)
      {
        cliphead->prev = temp;
        cliphead = temp;
      }
      else
      {
        temp->prev = prevNode;
        prevNode->next = temp;
        node->prev = temp;
      }
    }
  }
  else
  {
    temp = gld_clipnode_NewRange(start, end);
    cliphead = temp;
    return;
  }
}

void gld_clipper_Clear(void)
{
  clipnode_t *node = cliphead;
  clipnode_t *temp;

  while (node != NULL)
  {
    temp = node;
    node = node->next;
    gld_clipnode_Free(temp);
  }

  cliphead = NULL;
}

angle_t gld_FrustumAngle(void)
{
  double floatangle;
  angle_t a1;

  float tilt = (float)fabs(((double)(int)viewpitch) / ANG1);
  if (tilt > 90.0f)
  {
    tilt = 90.0f;
  }

  // If the pitch is larger than this you can look all around at a FOV of 90
  if (D_abs(viewpitch) > 46 * ANG1)
    return 0xffffffff;

  // ok, this is a gross hack that barely works...
  // but at least it doesn't overestimate too much...
  floatangle = 2.0 + (45.0 + (tilt / 1.9)) * render_fov * 48.0 / 48.0 / 90.0;
  a1 = ANG1 * (int)floatangle;
  if (a1 >= ANG180)
    return 0xffffffff;
  return a1;
}
