// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: St_lib.c,v 1.1 2000/04/09 18:18:58 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      The status bar widget code.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: St_lib.c,v 1.1 2000/04/09 18:18:58 proff_fs Exp $";

#include "doomdef.h"
#include "doomstat.h"
#include "v_video.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "st_lib.h"
#include "r_main.h"

int sts_always_red;      //jff 2/18/98 control to disable status color changes
int sts_pct_always_gray; // killough 2/21/98: always gray %'s? bug or feature?

patchnum_t sttminus;

//
// STlib_init()
//
// Hack display negative frags. Loads and store the stminus lump.
//
// Passed nothing, returns nothing
//
void STlib_init(void)
{
  R_SetPatchNum(&sttminus, "STTMINUS");
}

//
// STlib_initNum()
//
// Initializes an st_number_t widget
//
// Passed the widget, its position, the patches for the digits, a pointer
// to the value displayed, a pointer to the on/off control, and the width
// Returns nothing
//
void STlib_initNum
( st_number_t* n,
  int x,
  int y,
  patchnum_t* pl,
  int* num,
  H_boolean* on,
  int     width )
{
  n->x  = x;
  n->y  = y;
  n->oldnum = 0;
  n->width  = width;
  n->num  = num;
  n->on = on;
  n->p  = pl;
}

//
// STlib_drawNum()
// 
// A fairly efficient way to draw a number based on differences from the 
// old number.
//
// Passed a st_number_t widget, a color range for output, and a flag
// indicating whether refresh is needed.
// Returns nothing
//
void STlib_drawNum
( st_number_t*  n,
  int cm,        //jff 2/16/98 add color translation to digit output
  H_boolean refresh )
{

  int   numdigits = n->width;
  int   num = *n->num;

  int   w = SHORT(n->p[0].width);
  int   h = SHORT(n->p[0].height);
  int   x = n->x;

  int   neg;

  n->oldnum = *n->num;

  neg = num < 0;

  if (neg)
  {
    if (numdigits == 2 && num < -9)
      num = -9;
    else if (numdigits == 3 && num < -99)
      num = -99;

    num = -num;
  }

  // clear the area
  x = n->x - numdigits*w;

  if (n->y - ST_Y < 0)
    I_Error("drawNum: n->y - ST_Y < 0");

  V_CopyRectStretched(x, n->y - ST_Y, BG, w*numdigits, h, x, n->y, FG);

  // if non-number, do not draw it
  if (num == 1994)
    return;

  x = n->x;

  //jff 2/16/98 add color translation to digit output
  // in the special case of 0, you draw 0
  if (!num)
    if ((cm!=CR_RED) && !sts_always_red)
      V_DrawPatchTransStretchedFromNum(x - w, n->y, FG, n->p[ 0 ].lumpnum,cm);
    else //jff 2/18/98 allow use of faster draw routine from config
      V_DrawPatchStretchedFromNum(x - w, n->y, FG, n->p[ 0 ].lumpnum);

  // draw the new number
  //jff 2/16/98 add color translation to digit output
  while (num && numdigits--)
  {
    x -= w;
    if ((cm!=CR_RED) && !sts_always_red)
      V_DrawPatchTransStretchedFromNum(x, n->y, FG, n->p[ num % 10 ].lumpnum,cm);
    else //jff 2/18/98 allow use of faster draw routine from config
      V_DrawPatchStretchedFromNum(x, n->y, FG, n->p[ num % 10 ].lumpnum);
    num /= 10;
  }

  // draw a minus sign if necessary
  //jff 2/16/98 add color translation to digit output
  if (neg)
    if ((cm!=CR_RED) && !sts_always_red)
      V_DrawPatchTransStretchedFromNum(x - 8, n->y, FG, sttminus.lumpnum,cm);
    else //jff 2/18/98 allow use of faster draw routine from config
      V_DrawPatchStretchedFromNum(x - 8, n->y, FG, sttminus.lumpnum);
}

//
// STlib_updateNum()
//
// Draws a number conditionally based on the widget's enable
//
// Passed a number widget, the output color range, and a refresh flag
// Returns nothing
//
void STlib_updateNum
( st_number_t*    n,
  int cm, //jff 2/16/98 add color translation to digit output
  H_boolean   refresh )
{
  if (*n->on) STlib_drawNum(n, cm, refresh);
}

//
// STlib_initPercent()
//
// Initialize a st_percent_t number with percent sign widget
//
// Passed a st_percent_t widget, the position, the digit patches, a pointer
// to the number to display, a pointer to the enable flag, and patch
// for the percent sign.
// Returns nothing.
//
void STlib_initPercent
( st_percent_t* p,
  int x,
  int y,
  patchnum_t* pl,
  int* num,
  H_boolean* on,
  patchnum_t* percent )
{
  STlib_initNum(&p->n, x, y, pl, num, on, 3);
  p->p = percent;
}

//
// STlib_updatePercent()
//
// Draws a number/percent conditionally based on the widget's enable
//
// Passed a precent widget, the output color range, and a refresh flag
// Returns nothing
//
void STlib_updatePercent
( st_percent_t*   per,
  int cm,             //jff 2/16/98 add color translation to digit output
  int refresh )
{
  if (refresh || *per->n.on) // killough 2/21/98: fix percents not updated;
    if (!sts_always_red)     // also support gray-only percents
      V_DrawPatchTransStretchedFromNum(per->n.x,per->n.y,FG,per->p->lumpnum,
                                       sts_pct_always_gray ? CR_GRAY : cm);
    else   //jff 2/18/98 allow use of faster draw routine from config
      V_DrawPatchStretchedFromNum(per->n.x, per->n.y, FG, per->p->lumpnum);
  
  STlib_updateNum(&per->n, cm, refresh);
}

//
// STlib_initMultIcon()
//
// Initialize a st_multicon_t widget, used for a multigraphic display
// like the status bar's keys.
//
// Passed a st_multicon_t widget, the position, the graphic patches, a pointer
// to the numbers representing what to display, and pointer to the enable flag
// Returns nothing.
//
void STlib_initMultIcon
( st_multicon_t* i,
  int x,
  int y,
  patchnum_t il[],
  int* inum,
  H_boolean* on )
{
  i->x  = x;
  i->y  = y;
  i->oldinum  = -1;
  i->inum = inum;
  i->on = on;
  i->p  = il;
}

//
// STlib_updateMultIcon()
//
// Draw a st_multicon_t widget, used for a multigraphic display
// like the status bar's keys. Displays each when the control
// numbers change or refresh is true
//
// Passed a st_multicon_t widget, and a refresh flag
// Returns nothing.
//
void STlib_updateMultIcon
( st_multicon_t*  mi,
  H_boolean   refresh )
{
  int w;
  int h;
  int x;
  int y;

  if (*mi->on && (mi->oldinum != *mi->inum || refresh))
  {
    if (mi->oldinum != -1)
    {
      x = mi->x - SHORT(mi->p[mi->oldinum].leftoffset);
      y = mi->y - SHORT(mi->p[mi->oldinum].topoffset);
      w = SHORT(mi->p[mi->oldinum].width);
      h = SHORT(mi->p[mi->oldinum].height);

      if (y - ST_Y < 0)
        I_Error("updateMultIcon: y - ST_Y < 0");

      V_CopyRectStretched(x, y-ST_Y, BG, w, h, x, y, FG);
    }
    if (*mi->inum != -1)  // killough 2/16/98: redraw only if != -1
      V_DrawPatchStretchedFromNum(mi->x, mi->y, FG, mi->p[*mi->inum].lumpnum);
    mi->oldinum = *mi->inum;
  }
}

//
// STlib_initBinIcon()
//
// Initialize a st_binicon_t widget, used for a multinumber display
// like the status bar's weapons, that are present or not.
//
// Passed a st_binicon_t widget, the position, the digit patches, a pointer
// to the flags representing what is displayed, and pointer to the enable flag
// Returns nothing.
//
void STlib_initBinIcon
( st_binicon_t* b,
  int x,
  int y,
  patchnum_t* i,
  H_boolean* val,
  H_boolean* on )
{
  b->x  = x;
  b->y  = y;
  b->oldval = 0;
  b->val  = val;
  b->on = on;
  b->p  = i;
}

//
// STlib_updateBinIcon()
//
// DInitialize a st_binicon_t widget, used for a multinumber display
// like the status bar's weapons, that are present or not.
//
// Draw a st_binicon_t widget, used for a multinumber display
// like the status bar's weapons that are present or not. Displays each
// when the control flag changes or refresh is true
//
// Passed a st_binicon_t widget, and a refresh flag
// Returns nothing.
//
void STlib_updateBinIcon
( st_binicon_t*   bi,
  H_boolean   refresh )
{
  int     x;
  int     y;
  int     w;
  int     h;

  if (*bi->on && (bi->oldval != *bi->val || refresh))
  {
    x = bi->x - SHORT(bi->p->leftoffset);
    y = bi->y - SHORT(bi->p->topoffset);
    w = SHORT(bi->p->width);
    h = SHORT(bi->p->height);

    if (y - ST_Y < 0)
      I_Error("updateBinIcon: y - ST_Y < 0");

    if (*bi->val)
      V_DrawPatchStretchedFromNum(bi->x, bi->y, FG, bi->p->lumpnum);
    else
      V_CopyRectStretched(x, y-ST_Y, BG, w, h, x, y, FG);

    bi->oldval = *bi->val;
  }
}

//----------------------------------------------------------------------------
//
// $Log: St_lib.c,v $
// Revision 1.1  2000/04/09 18:18:58  proff_fs
// Initial revision
//
// Revision 1.8  1998/05/11  10:44:42  jim
// formatted/documented st_lib
//
// Revision 1.7  1998/05/03  22:58:17  killough
// Fix header #includes at top, nothing else
//
// Revision 1.6  1998/02/23  04:56:34  killough
// Fix percent sign problems
//
// Revision 1.5  1998/02/19  16:55:09  jim
// Optimized HUD and made more configurable
//
// Revision 1.4  1998/02/18  00:59:13  jim
// Addition of HUD
//
// Revision 1.3  1998/02/17  06:17:03  killough
// Add support for erasing keys in status bar
//
// Revision 1.2  1998/01/26  19:24:56  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------

