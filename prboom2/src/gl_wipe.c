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

#include <SDL_opengl.h>
#include "v_video.h"
#include "gl_intern.h"
#include "m_random.h"
#include "lprintf.h"
#include "e6y.h"

static GLuint wipe_scr_start_tex = 0;
static GLuint wipe_scr_end_tex = 0;
static int cur_wipe_progress;

static GLuint CaptureScreenAsTexID(void)
{
  int total_w, total_h;
  int x, y, id;
  
  byte *pixels;
  byte *line_buf;
  
  cur_wipe_progress =  0;
  
  total_w = gld_GetTexDimension(SCREENWIDTH);
  total_h = gld_GetTexDimension(SCREENHEIGHT);
  
  pixels = Z_Malloc(total_w * total_h * 4, PU_STATIC, 0);
  
  line_buf = Z_Malloc(SCREENWIDTH * 4, PU_STATIC, 0);
  
  // read pixels from screen
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  for (y=0; y < total_h; y++)
  {
    int px;
    int py = y;
    
    glReadPixels(0, py, SCREENWIDTH, 1, GL_RGB, GL_UNSIGNED_BYTE, line_buf);
    
    for (x=0; x < total_w; x++)
    {
      byte *dest_p = pixels + ((y * total_w + x) * 3);
      
      px = x;
      
      dest_p[0] = line_buf[px*3 + 0];
      dest_p[1] = line_buf[px*3 + 1];
      dest_p[2] = line_buf[px*3 + 2];
    }
  }
  
  glEnable(GL_TEXTURE_2D);
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  
  glTexImage2D(GL_TEXTURE_2D, 0, 3, total_w, total_h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
  
  //glDisable(GL_TEXTURE_2D);
  
  Z_Free(line_buf);
  Z_Free(pixels);

  return id;
}

int gld_wipe_doMelt(int ticks, int *y_lookup)
{
  int i;
  int total_w, total_h;
  float fU1,fU2,fV1,fV2;

  total_w = gld_GetTexDimension(SCREENWIDTH);
  total_h = gld_GetTexDimension(SCREENHEIGHT);

  fU1=0.0f;
  fV1=(float)SCREENHEIGHT / (float)total_h;
  fU2=(float)SCREENWIDTH / (float)total_w;
  fV2=0.0f;
  
  glEnable(GL_TEXTURE_2D);
  
  glBindTexture(GL_TEXTURE_2D, wipe_scr_end_tex);
  glColor3f(1.0f, 1.0f, 1.0f);

  glBegin(GL_TRIANGLE_STRIP);
  {
    glTexCoord2f(fU1, fV1); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(fU1, fV2); glVertex2f(0.0f, (float)SCREENHEIGHT);
    glTexCoord2f(fU2, fV1); glVertex2f((float)SCREENWIDTH, 0.0f);
    glTexCoord2f(fU2, fV2); glVertex2f((float)SCREENWIDTH, (float)SCREENHEIGHT);
  }
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D, wipe_scr_start_tex);
  glColor3f(1.0f, 1.0f, 1.0f);
  
  glBegin(GL_QUAD_STRIP);
  
  for (i=0; i <= SCREENWIDTH; i++)
  {
    int yoffs = MAX(0, y_lookup[i]);
    
    float tx = (float) i / total_w;
    float sx = (float) i;
    float sy = (float) yoffs * SCREENHEIGHT / 200.0f;
    
    glTexCoord2f(tx, fV1); glVertex2f(sx, sy);
    glTexCoord2f(tx, fV2); glVertex2f(sx, sy + (float)SCREENHEIGHT);
  }
  
  glEnd();
  
  return 0;
}

int gld_wipe_exitMelt(int ticks)
{
  if (wipe_scr_start_tex != 0)
  {
    glDeleteTextures(1, &wipe_scr_start_tex);
    wipe_scr_start_tex = 0;
  }
  if (wipe_scr_end_tex != 0)
  {
    glDeleteTextures(1, &wipe_scr_end_tex);
    wipe_scr_end_tex = 0;
  }

  return 0;
}

int gld_wipe_StartScreen(void)
{
  wipe_scr_start_tex = CaptureScreenAsTexID();

  return 0;
}

int gld_wipe_EndScreen(void)
{
  wipe_scr_end_tex = CaptureScreenAsTexID();

  return 0;
}
