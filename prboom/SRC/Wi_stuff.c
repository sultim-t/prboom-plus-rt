// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: Wi_stuff.c,v 1.1 2000/04/09 18:15:35 proff_fs Exp $
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
//  Intermission screens.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: Wi_stuff.c,v 1.1 2000/04/09 18:15:35 proff_fs Exp $";

#include "doomstat.h"
#include "m_random.h"
#include "w_wad.h"
#include "g_game.h"
#include "r_main.h"
#include "v_video.h"
#include "wi_stuff.h"
#include "s_sound.h"
#include "sounds.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

// Ty 03/17/98: flag that new par times have been loaded in d_deh
extern H_boolean deh_pars;  

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES 4
#define NUMMAPS     9


// Not used
// in tics
//U #define PAUSELEN    (TICRATE*2) 
//U #define SCORESTEP    100
//U #define ANIMPERIOD    32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST  10 
//U #define WK 1


// GLOBAL LOCATIONS
#define WI_TITLEY      2
#define WI_SPACINGY   33

// SINGLE-PLAYER STUFF
#define SP_STATSX     50
#define SP_STATSY     50

#define SP_TIMEX      16
// proff/nicolas 09/20/98 -- changed for hi-res
#define SP_TIMEY      168
//#define SP_TIMEY      (SCREENHEIGHT-32)


// NET GAME STUFF
#define NG_STATSY     50
#define NG_STATSX     (32 + SHORT(star->width)/2 + 32*!dofrags)

#define NG_SPACINGX   64


// Used to display the frags matrix at endgame
// DEATHMATCH STUFF
#define DM_MATRIXX    42
#define DM_MATRIXY    68

#define DM_SPACINGX   40

#define DM_TOTALSX   269

#define DM_KILLERSX   10
#define DM_KILLERSY  100
#define DM_VICTIMSX    5
#define DM_VICTIMSY   50


// These animation variables, structures, etc. are used for the
// DOOM/Ultimate DOOM intermission screen animations.  This is
// totally different from any sprite or texture/flat animations
typedef enum
{
  ANIM_ALWAYS,   // determined by patch entry
  ANIM_RANDOM,   // occasional
  ANIM_LEVEL     // continuous
} animenum_t;

typedef struct
{
  int   x;       // x/y coordinate pair structure
  int   y;
} point_t;


//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
  animenum_t  type;

  // period in tics between animations
  int   period;

  // number of animation frames
  int   nanims;

  // location of animation
  point_t loc;

  // ALWAYS: n/a,
  // RANDOM: period deviation (<256),
  // LEVEL: level
  int   data1;

  // ALWAYS: n/a,
  // RANDOM: random base period,
  // LEVEL: n/a
  int   data2; 

  // actual graphics for frames of animations
  patch_t*  p[3]; 

  // following must be initialized to zero before use!

  // next value of bcnt (used in conjunction with period)
  int   nexttic;

  // last drawn animation frame
  int   lastdrawn;

  // next frame number to animate
  int   ctr;
  
  // used by RANDOM and LEVEL when animating
  int   state;  
} anim_t;


static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
  // Episode 0 World Map
  {
    { 185, 164 }, // location of level 0 (CJ)
    { 148, 143 }, // location of level 1 (CJ)
    { 69, 122 },  // location of level 2 (CJ)
    { 209, 102 }, // location of level 3 (CJ)
    { 116, 89 },  // location of level 4 (CJ)
    { 166, 55 },  // location of level 5 (CJ)
    { 71, 56 },   // location of level 6 (CJ)
    { 135, 29 },  // location of level 7 (CJ)
    { 71, 24 }    // location of level 8 (CJ)
  },
  
  // Episode 1 World Map should go here
  {
    { 254, 25 },  // location of level 0 (CJ)
    { 97, 50 },   // location of level 1 (CJ)
    { 188, 64 },  // location of level 2 (CJ)
    { 128, 78 },  // location of level 3 (CJ)
    { 214, 92 },  // location of level 4 (CJ)
    { 133, 130 }, // location of level 5 (CJ)
    { 208, 136 }, // location of level 6 (CJ)
    { 148, 140 }, // location of level 7 (CJ)
    { 235, 158 }  // location of level 8 (CJ)
  },
  
  // Episode 2 World Map should go here
  {
    { 156, 168 }, // location of level 0 (CJ)
    { 48, 154 },  // location of level 1 (CJ)
    { 174, 95 },  // location of level 2 (CJ)
    { 265, 75 },  // location of level 3 (CJ)
    { 130, 48 },  // location of level 4 (CJ)
    { 279, 23 },  // location of level 5 (CJ)
    { 198, 48 },  // location of level 6 (CJ)
    { 140, 25 },  // location of level 7 (CJ)
    { 281, 136 }  // location of level 8 (CJ)
  }
};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_t epsd1animinfo[] =
{
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 1 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 2 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 3 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 4 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 5 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 6 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 7 },
  { ANIM_LEVEL,  TICRATE/3, 3, { 192, 144 }, 8 },
  { ANIM_LEVEL,  TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_t epsd2animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
  { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static int NUMANIMS[NUMEPISODES] =
{
  sizeof(epsd0animinfo)/sizeof(anim_t),
  sizeof(epsd1animinfo)/sizeof(anim_t),
  sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[NUMEPISODES] =
{
  epsd0animinfo,
  epsd1animinfo,
  epsd2animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0


// States for single-player
#define SP_KILLS    0
#define SP_ITEMS    2
#define SP_SECRET   4
#define SP_FRAGS    6 
#define SP_TIME     8 
#define SP_PAR      ST_TIME

#define SP_PAUSE    1

// in seconds
#define SHOWNEXTLOCDELAY  4
//#define SHOWLASTLOCDELAY  SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
int   acceleratestage;           // killough 3/28/98: made global

// wbs->pnum
static int    me;

 // specifies current state
static stateenum_t  state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static wbplayerstruct_t* plrs;  // wbs->plyr[]

// used for general timing
static int    cnt;  

// used for timing of background animation
static int    bcnt;

// signals to refresh everything for one frame
static int    firstrefresh; 

static int    cnt_kills[MAXPLAYERS];
static int    cnt_items[MAXPLAYERS];
static int    cnt_secret[MAXPLAYERS];
static int    cnt_time;
static int    cnt_par;
static int    cnt_pause;

// # of commercial levels
static int    NUMCMAPS; 


//
//  GRAPHICS
//

// You Are Here graphic
static patch_t*   yah[2]; 

// splat
static patch_t*   splat;

// %, : graphics
static patch_t*   percent;
static patch_t*   colon;

// 0-9 graphic
static patch_t*   num[10];

// minus sign
static patch_t*   wiminus;

// "Finished!" graphics
static patch_t*   finished;

// "Entering" graphic
static patch_t*   entering; 

// "secret"
static patch_t*   sp_secret;

// "Kills", "Scrt", "Items", "Frags"
static patch_t*   kills;
static patch_t*   secret;
static patch_t*   items;
static patch_t*   frags;

// Time sucks.
static patch_t*   time;
static patch_t*   par;
static patch_t*   sucks;

// "killers", "victims"
static patch_t*   killers;
static patch_t*   victims; 

// "Total", your face, your dead face
static patch_t*   total;
static patch_t*   star;
static patch_t*   bstar;

// "red P[1..MAXPLAYERS]"
static patch_t*   p[MAXPLAYERS];

// "gray P[1..MAXPLAYERS]"
static patch_t*   bp[MAXPLAYERS];

// Name graphics of each level (centered)
static patch_t**  lnames;


//
// CODE
//


// ====================================================================
// WI_slamBackground
// Purpose: Put the full-screen background up prior to patches
// Args:    none
// Returns: void
//
static void WI_slamBackground(void)
{
  WI_DrawBackground();
//  V_CopyRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0); // killough 11/98
}


// ====================================================================
// WI_Responder
// Purpose: Draw animations on intermission background screen
// Args:    ev    -- event pointer, not actually used here.
// Returns: False -- dummy routine
//
// The ticker is used to detect keys
//  because of timing issues in netgames.
H_boolean WI_Responder(event_t* ev)
{
  return false;
}


// ====================================================================
// WI_drawLF
// Purpose: Draw the "Finished" level name before showing stats
// Args:    none
// Returns: void
//
static void WI_drawLF(void)
{
  int y = WI_TITLEY;

  // draw <LevelName> 
  V_DrawPatchStretchedFromMem((320 - SHORT(lnames[wbs->last]->width))/2,
  y, FB, lnames[wbs->last]);

  // draw "Finished!"
  y += (5*SHORT(lnames[wbs->last]->height))/4;
  
  V_DrawPatchStretchedFromMem((320 - SHORT(finished->width))/2,
  y, FB, finished);
}


// ====================================================================
// WI_drawEL
// Purpose: Draw introductory "Entering" and level name
// Args:    none
// Returns: void
//
static void WI_drawEL(void)
{
  int y = WI_TITLEY;

  // draw "Entering"
  V_DrawPatchStretchedFromMem((320 - SHORT(entering->width))/2,
  y, FB, entering);

  // draw level
  y += (5*SHORT(lnames[wbs->next]->height))/4;

  V_DrawPatchStretchedFromMem((320 - SHORT(lnames[wbs->next]->width))/2,
  y, FB, lnames[wbs->next]);
}


// ====================================================================
// WI_drawOnLnode
// Purpose: Draw patches at a location based on episode/map
// Args:    n   -- index to map# within episode
//          c[] -- array of patches to be drawn
// Returns: void
//
static void
WI_drawOnLnode  // draw stuff at a location by episode/map#
( int   n,
  patch_t*  c[] )
{
  int   i;
  int   left;
  int   top;
  int   right;
  int   bottom;
  H_boolean fits = false;

  i = 0;
  do
  {
    left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
    top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
    right = left + SHORT(c[i]->width);
    bottom = top + SHORT(c[i]->height);
  
    if (left >= 0
       && right < SCREENWIDTH
       && top >= 0
       && bottom < SCREENHEIGHT)
    {
      fits = true;
    }
    else
    {
      i++;
    }
  } while (!fits && i!=2);

  if (fits && i<2)
  {
    V_DrawPatchStretchedFromMem(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y,
      FB, c[i]);
  }
  else
  {
    // DEBUG
    //jff 8/3/98 use logical output routine
    lprintf(LO_DEBUG,"Could not place patch on level %d", n+1);
  }
}


// ====================================================================
// WI_initAnimatedBack
// Purpose: Initialize pointers and styles for background animation
// Args:    none
// Returns: void
//
static void WI_initAnimatedBack(void)
{
  int   i;
  anim_t* a;

  if (gamemode == commercial)  // no animation for DOOM2
    return;

  if (wbs->epsd > 2)
    return;

  for (i=0;i<NUMANIMS[wbs->epsd];i++)
  {
    a = &anims[wbs->epsd][i];

    // init variables
    a->ctr = -1;
  
    // specify the next time to draw it
    if (a->type == ANIM_ALWAYS)
      a->nexttic = bcnt + 1 + (M_Random()%a->period);
    else 
      if (a->type == ANIM_RANDOM)
        a->nexttic = bcnt + 1 + a->data2+(M_Random()%a->data1);
      else 
        if (a->type == ANIM_LEVEL)
          a->nexttic = bcnt + 1;
  }
}


// ====================================================================
// WI_updateAnimatedBack
// Purpose: Figure out what animation we do on this iteration
// Args:    none
// Returns: void
//
static void WI_updateAnimatedBack(void)
{
  int   i;
  anim_t* a;

  if (gamemode == commercial)
    return;

  if (wbs->epsd > 2)
    return;

  for (i=0;i<NUMANIMS[wbs->epsd];i++)
  {
    a = &anims[wbs->epsd][i];

    if (bcnt == a->nexttic)
    {
      switch (a->type)
      {
        case ANIM_ALWAYS:
             if (++a->ctr >= a->nanims) a->ctr = 0;
             a->nexttic = bcnt + a->period;
             break;

        case ANIM_RANDOM:
             a->ctr++;
             if (a->ctr == a->nanims)
             {
               a->ctr = -1;
               a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
             }
             else 
               a->nexttic = bcnt + a->period;
             break;
    
        case ANIM_LEVEL:
             // gawd-awful hack for level anims
             if (!(state == StatCount && i == 7)
                && wbs->next == a->data1)
             {
               a->ctr++;
               if (a->ctr == a->nanims) a->ctr--;
               a->nexttic = bcnt + a->period;
             }
             break;
      }
    }
  }
}


// ====================================================================
// WI_drawAnimatedBack
// Purpose: Actually do the animation (whew!)
// Args:    none
// Returns: void
//
static void WI_drawAnimatedBack(void)
{
  int     i;
  anim_t*   a;

  if (gamemode==commercial) //jff 4/25/98 Someone forgot commercial an enum
    return;

  if (wbs->epsd > 2)
    return;

  for (i=0 ; i<NUMANIMS[wbs->epsd] ; i++)
  {
    a = &anims[wbs->epsd][i];

    if (a->ctr >= 0)
      V_DrawPatchStretchedFromMem(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
  }
}


// ====================================================================
// WI_drawNum
// Purpose: Draws a number.  If digits > 0, then use that many digits
//          minimum, otherwise only use as many as necessary
// Args:    x, y   -- location
//          n      -- the number to be drawn
//          digits -- number of digits minimum or zero
// Returns: new x position after drawing (note we are going to the left)
//
static int
WI_drawNum
( int   x,
  int   y,
  int   n,
  int   digits )
{
  int   fontwidth = SHORT(num[0]->width);
  int   neg;
  int   temp;

  neg = n < 0; // killough 11/98: move up to here, for /= 10 division below
  if (neg)
    n = -n;

  if (digits < 0)
  {
    if (!n)
    {
      // make variable-length zeros 1 digit long
      digits = 1;
    }
    else
    {
      // figure out # of digits in #
      digits = 0;
      temp = n;

      while (temp)
      {
        temp /= 10;
        digits++;
      }
    }
  }

  // if non-number, do not draw it
  if (n == 1994)
    return 0;

  // draw the new number
  while (digits--)
  {
    x -= fontwidth;
    V_DrawPatchStretchedFromMem(x, y, FB, num[ n % 10 ]);
    n /= 10;
  }

  // draw a minus sign if necessary
  if (neg)
    V_DrawPatchStretchedFromMem(x-=8, y, FB, wiminus);

  return x;
}


// ====================================================================
// WI_drawPercent
// Purpose: Draws a percentage, really just a call to WI_drawNum 
//          after putting a percent sign out there
// Args:    x, y   -- location
//          p      -- the percentage value to be drawn, no negatives
// Returns: void
//
static void
WI_drawPercent
( int   x,
  int   y,
  int   p )
{
  if (p < 0)
    return;

  V_DrawPatchStretchedFromMem(x, y, FB, percent);
  WI_drawNum(x, y, p, -1);
}


// ====================================================================
// WI_drawTime
// Purpose: Draws the level completion time or par time, or "Sucks"
//          if 1 hour or more
// Args:    x, y   -- location
//          t      -- the time value to be drawn
// Returns: void
//
static void
WI_drawTime
( int   x,
  int   y,
  int   t )
{
  
  int   div;
  int   n;

  if (t<0)
    return;

  if (t <= 61*59)  // otherwise known as 60*60 -1 == 3599
  {
    div = 1;

    do
    {
      n = (t / div) % 60;
      x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
      div *= 60;

      // draw
      if (div==60 || t / div)
        V_DrawPatchStretchedFromMem(x, y, FB, colon);
      
    } while (t / div);
  }
  else
  {
    // "sucks"
    V_DrawPatchStretchedFromMem(x - SHORT(sucks->width), y, FB, sucks); 
  }
}


// ====================================================================
// WI_unloadData
// Purpose: Free up the space allocated during WI_loadData
// Args:    none
// Returns: void
//
static void WI_unloadData(void)
{
  int   i;
  int   j;

  Z_ChangeTag(wiminus, PU_CACHE);

  for (i=0 ; i<10 ; i++)
    Z_ChangeTag(num[i], PU_CACHE);
    
  if (gamemode == commercial)
  {
    for (i=0 ; i<NUMCMAPS ; i++)
      Z_ChangeTag(lnames[i], PU_CACHE);
  }
  else
  {
    Z_ChangeTag(yah[0], PU_CACHE);
    Z_ChangeTag(yah[1], PU_CACHE);

    Z_ChangeTag(splat, PU_CACHE);

    for (i=0 ; i<NUMMAPS ; i++)
      Z_ChangeTag(lnames[i], PU_CACHE);
  
    if (wbs->epsd < 3)
    {
      for (j=0;j<NUMANIMS[wbs->epsd];j++)
      {
        if (wbs->epsd != 1 || j != 8)
          for (i=0;i<anims[wbs->epsd][j].nanims;i++)
            Z_ChangeTag(anims[wbs->epsd][j].p[i], PU_CACHE);
      }
    }
  }
    
  //  Z_Free(lnames);    // killough 4/26/98: this free is too early!!!

  Z_ChangeTag(percent, PU_CACHE);
  Z_ChangeTag(colon, PU_CACHE);
  Z_ChangeTag(finished, PU_CACHE);
  Z_ChangeTag(entering, PU_CACHE);
  Z_ChangeTag(kills, PU_CACHE);
  Z_ChangeTag(secret, PU_CACHE);
  Z_ChangeTag(sp_secret, PU_CACHE);
  Z_ChangeTag(items, PU_CACHE);
  Z_ChangeTag(frags, PU_CACHE);
  Z_ChangeTag(time, PU_CACHE);
  Z_ChangeTag(sucks, PU_CACHE);
  Z_ChangeTag(par, PU_CACHE);

  Z_ChangeTag(victims, PU_CACHE);
  Z_ChangeTag(killers, PU_CACHE);
  Z_ChangeTag(total, PU_CACHE);
  //  Z_ChangeTag(star, PU_CACHE);
  //  Z_ChangeTag(bstar, PU_CACHE);
  
  for (i=0 ; i<MAXPLAYERS ; i++)
    Z_ChangeTag(p[i], PU_CACHE);

  for (i=0 ; i<MAXPLAYERS ; i++)
    Z_ChangeTag(bp[i], PU_CACHE);
}


// ====================================================================
// WI_End
// Purpose: Unloads data structures (inverse of WI_Start)
// Args:    none
// Returns: void
//
static void WI_End(void)
{
  WI_unloadData();
}


// ====================================================================
// WI_initNoState
// Purpose: Clear state, ready for end of level activity
// Args:    none
// Returns: void
//
static void WI_initNoState(void)
{
  state = NoState;
  acceleratestage = 0;
  cnt = 10;
}


// ====================================================================
// WI_updateNoState
// Purpose: Cycle until end of level activity is done
// Args:    none
// Returns: void
//
static void WI_updateNoState(void) 
{

  WI_updateAnimatedBack();

  if (!--cnt)
  {
    WI_End();
    G_WorldDone();
  }
}

static H_boolean    snl_pointeron = false;


// ====================================================================
// WI_initShowNextLoc
// Purpose: Prepare to show the next level's location 
// Args:    none
// Returns: void
//
static void WI_initShowNextLoc(void)
{
  state = ShowNextLoc;
  acceleratestage = 0;
  cnt = SHOWNEXTLOCDELAY * TICRATE;

  WI_initAnimatedBack();
}


// ====================================================================
// WI_updateShowNextLoc
// Purpose: Prepare to show the next level's location
// Args:    none
// Returns: void
//
static void WI_updateShowNextLoc(void)
{
  WI_updateAnimatedBack();

  if (!--cnt || acceleratestage)
    WI_initNoState();
  else
    snl_pointeron = (cnt & 31) < 20;
}


// ====================================================================
// WI_drawShowNextLoc
// Purpose: Show the next level's location on animated backgrounds
// Args:    none
// Returns: void
//
static void WI_drawShowNextLoc(void)
{
  int   i;
  int   last;

  WI_slamBackground();

  // draw animated background
  WI_drawAnimatedBack(); 

  if ( gamemode != commercial)
  {
    if (wbs->epsd > 2)
    {
      WI_drawEL();  // "Entering..." if not E1 or E2
      return;
    }
  
    last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

    // draw a splat on taken cities.
    for (i=0 ; i<=last ; i++)
      WI_drawOnLnode(i, &splat);

    // splat the secret level?
    if (wbs->didsecret)
      WI_drawOnLnode(8, &splat);

    // draw flashing ptr
    if (snl_pointeron)
      WI_drawOnLnode(wbs->next, yah); 
  }

  // draws which level you are entering..
  if ( (gamemode != commercial)
     || wbs->next != 30)  // check for MAP30 end game
  WI_drawEL();  
}

// ====================================================================
// WI_drawNoState
// Purpose: Draw the pointer and next location
// Args:    none
// Returns: void
//
static void WI_drawNoState(void)
{
  snl_pointeron = true;
  WI_drawShowNextLoc();
}

// ====================================================================
// WI_fragSum
// Purpose: Calculate frags for this player based on the current totals
//          of all the other players.  Subtract self-frags.
// Args:    playernum -- the player to be calculated
// Returns: the total frags for this player
//
static int WI_fragSum(int playernum)
{
  int   i;
  int   frags = 0;
    
  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i]  // is this player playing?
       && i!=playernum) // and it's not the player we're calculating
    {
      frags += plrs[playernum].frags[i];  
    }
  }

  
  // JDC hack - negative frags.
  frags -= plrs[playernum].frags[playernum];
  // UNUSED if (frags < 0)
  //  frags = 0;

  return frags;
}

static int    dm_state;
static int    dm_frags[MAXPLAYERS][MAXPLAYERS];  // frags matrix
static int    dm_totals[MAXPLAYERS];  // totals by player

// ====================================================================
// WI_initDeathmatchStats
// Purpose: Set up to display DM stats at end of level.  Calculate 
//          frags for all players.
// Args:    none
// Returns: void
//
static void WI_initDeathmatchStats(void)
{

  int   i; // looping variables
  int   j;

  state = StatCount;  // We're doing stats
  acceleratestage = 0;
  dm_state = 1;  // count how many times we've done a complete stat

  cnt_pause = TICRATE;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i])
    { 
      for (j=0 ; j<MAXPLAYERS ; j++)
        if (playeringame[j])
          dm_frags[i][j] = 0;  // set all counts to zero

      dm_totals[i] = 0;
    }
  }
  WI_initAnimatedBack();
}


// ====================================================================
// WI_initDeathmatchStats
// Purpose: Set up to display DM stats at end of level.  Calculate 
//          frags for all players.  Lots of noise and drama around
//          the presentation.
// Args:    none
// Returns: void
//
static void WI_updateDeathmatchStats(void)
{
  int   i;
  int   j;
    
  H_boolean stillticking;
  
  WI_updateAnimatedBack();
  
  if (acceleratestage && dm_state != 4)  // still ticking
  {
    acceleratestage = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i])
      {
        for (j=0 ; j<MAXPLAYERS ; j++)
          if (playeringame[j])
            dm_frags[i][j] = plrs[i].frags[j];

        dm_totals[i] = WI_fragSum(i);
      }
    }
  

    S_StartSound(0, sfx_barexp);  // bang
    dm_state = 4;  // we're done with all 4 (or all we have to do)
  }

    
  if (dm_state == 2)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);  // noise while counting
  
    stillticking = false;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i])
      {
        for (j=0 ; j<MAXPLAYERS ; j++)
        {
          if (playeringame[j]
             && dm_frags[i][j] != plrs[i].frags[j])
          {
            if (plrs[i].frags[j] < 0)
              dm_frags[i][j]--;
            else
              dm_frags[i][j]++;

            if (dm_frags[i][j] > 999) // Ty 03/17/98 3-digit frag count
              dm_frags[i][j] = 999;

            if (dm_frags[i][j] < -999)
              dm_frags[i][j] = -999;
      
            stillticking = true;
          }
        }
        dm_totals[i] = WI_fragSum(i);

        if (dm_totals[i] > 999)
          dm_totals[i] = 999;
    
        if (dm_totals[i] < -999)
          dm_totals[i] = -999;  // Ty 03/17/98 end 3-digit frag count
      }
    }

    if (!stillticking)
    {
      S_StartSound(0, sfx_barexp);
      dm_state++;
    }
  }
  else if (dm_state == 4)
  {
    if (acceleratestage)
    {   
      S_StartSound(0, sfx_slop);

      if ( gamemode == commercial)
        WI_initNoState();
      else
        WI_initShowNextLoc();
    }
  }
  else if (dm_state & 1)
  {
    if (!--cnt_pause)
    {
      dm_state++;
      cnt_pause = TICRATE;
    }
  }
}


// ====================================================================
// WI_drawDeathmatchStats
// Purpose: Draw the stats on the screen in a matrix
// Args:    none
// Returns: void
//
static void WI_drawDeathmatchStats(void)
{
  int   i;
  int   j;
  int   x;
  int   y;
  int   w;
  
  int   lh; // line height

  lh = WI_SPACINGY;

  WI_slamBackground();
  
  // draw animated background
  WI_drawAnimatedBack(); 
  WI_drawLF();

  // draw stat titles (top line)
  V_DrawPatchStretchedFromMem(DM_TOTALSX-SHORT(total->width)/2,
    DM_MATRIXY-WI_SPACINGY+10,
    FB,
    total);
  
  V_DrawPatchStretchedFromMem(DM_KILLERSX, DM_KILLERSY, FB, killers);
  V_DrawPatchStretchedFromMem(DM_VICTIMSX, DM_VICTIMSY, FB, victims);

  // draw P?
  x = DM_MATRIXX + DM_SPACINGX;
  y = DM_MATRIXY;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i])
    {
      V_DrawPatchStretchedFromMem(x-SHORT(p[i]->width)/2,
        DM_MATRIXY - WI_SPACINGY,
        FB,
        p[i]);
      
      V_DrawPatchStretchedFromMem(DM_MATRIXX-SHORT(p[i]->width)/2,
        y,
        FB,
        p[i]);

      if (i == me)
      {
        V_DrawPatchStretchedFromMem(x-SHORT(p[i]->width)/2,
          DM_MATRIXY - WI_SPACINGY,
          FB,
          bstar);

        V_DrawPatchStretchedFromMem(DM_MATRIXX-SHORT(p[i]->width)/2,
          y,
          FB,
          star);
      }
    }
    else
    {
      // V_DrawPatch(x-SHORT(bp[i]->width)/2,
      //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
      // V_DrawPatch(DM_MATRIXX-SHORT(bp[i]->width)/2,
      //   y, FB, bp[i]);
    }
    x += DM_SPACINGX;
    y += WI_SPACINGY;
  }

  // draw stats
  y = DM_MATRIXY+10;
  w = SHORT(num[0]->width);

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    x = DM_MATRIXX + DM_SPACINGX;

    if (playeringame[i])
    {
      for (j=0 ; j<MAXPLAYERS ; j++)
      {
        if (playeringame[j])
          WI_drawNum(x+w, y, dm_frags[i][j], 2);

        x += DM_SPACINGX;
      }
      WI_drawNum(DM_TOTALSX+w, y, dm_totals[i], 2);
    }
    y += WI_SPACINGY;
  }
}


//
// Note: The term "Netgame" means a coop game
// (Or sometimes a DM game too, depending on context -- killough)
//
static int  cnt_frags[MAXPLAYERS];
static int  dofrags;
static int  ng_state;


// ====================================================================
// WI_initNetgameStats
// Purpose: Prepare for coop game stats
// Args:    none
// Returns: void
//
static void WI_initNetgameStats(void)
{
  int i;

  state = StatCount;
  acceleratestage = 0;
  ng_state = 1;

  cnt_pause = TICRATE;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (!playeringame[i])
      continue;

    cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

    dofrags += WI_fragSum(i);
  }

  dofrags = !!dofrags; // set to true or false - did we have frags?

  WI_initAnimatedBack();
}


// ====================================================================
// WI_updateNetgameStats
// Purpose: Calculate coop stats as we display them with noise and fury
// Args:    none
// Returns: void
// Comment: This stuff sure is complicated for what it does
//
static void WI_updateNetgameStats(void)
{
  int   i;
  int   fsum;
    
  H_boolean stillticking;

  WI_updateAnimatedBack();

  if (acceleratestage && ng_state != 10)
  {
    acceleratestage = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (!playeringame[i])
        continue;

      cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
      cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;

      // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
      cnt_secret[i] = wbs->maxsecret ? 
                      (plrs[i].ssecret * 100) / wbs->maxsecret : 100;
      if (dofrags)
        cnt_frags[i] = WI_fragSum(i);  // we had frags
    }
    S_StartSound(0, sfx_barexp);  // bang
    ng_state = 10;
  }

  if (ng_state == 2)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);  // pop

    stillticking = false;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (!playeringame[i])
        continue;

      cnt_kills[i] += 2;

      if (cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
        cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
      else
        stillticking = true; // still got stuff to tally
    }
  
    if (!stillticking)
    {
      S_StartSound(0, sfx_barexp); 
      ng_state++;
    }
  }
  else if (ng_state == 4)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);
  
    stillticking = false;
  
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (!playeringame[i])
        continue;
  
      cnt_items[i] += 2;
      if (cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
        cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
      else
        stillticking = true;
    }
  
    if (!stillticking)
    {
      S_StartSound(0, sfx_barexp);
      ng_state++;
    }
  }
  else if (ng_state == 6)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    stillticking = false;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (!playeringame[i])
        continue;

      cnt_secret[i] += 2;

      // killough 2/22/98: Make secrets = 100% if maxsecret = 0:

      if (cnt_secret[i] >= (wbs->maxsecret ? (plrs[i].ssecret * 100) / wbs->maxsecret : 100))
        cnt_secret[i] = wbs->maxsecret ? (plrs[i].ssecret * 100) / wbs->maxsecret : 100;
      else
        stillticking = true;
    }
  
    if (!stillticking)
    {
      S_StartSound(0, sfx_barexp);
      ng_state += 1 + 2*!dofrags;
    }
  }
  else if (ng_state == 8)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    stillticking = false;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (!playeringame[i])
        continue;

      cnt_frags[i] += 1;

      if (cnt_frags[i] >= (fsum = WI_fragSum(i)))
        cnt_frags[i] = fsum;
      else
        stillticking = true;
    }
      
    if (!stillticking)
    {
      S_StartSound(0, sfx_pldeth);
      ng_state++;
    }
  }
  else if (ng_state == 10)
  {
    if (acceleratestage)
    {
      S_StartSound(0, sfx_sgcock);
      if ( gamemode == commercial )
        WI_initNoState();
      else
        WI_initShowNextLoc();
    }
  }
  else if (ng_state & 1)
  {
    if (!--cnt_pause)
    {
      ng_state++;
      cnt_pause = TICRATE;
    }
  }
}


// ====================================================================
// WI_drawNetgameStats
// Purpose: Put the coop stats on the screen
// Args:    none
// Returns: void
//
static void WI_drawNetgameStats(void)
{
  int   i;
  int   x;
  int   y;
  int   pwidth = SHORT(percent->width);

  WI_slamBackground();
  
  // draw animated background
  WI_drawAnimatedBack(); 

  WI_drawLF();

  // draw stat titles (top line)
  V_DrawPatchStretchedFromMem(NG_STATSX+NG_SPACINGX-SHORT(kills->width),
  NG_STATSY, FB, kills);

  V_DrawPatchStretchedFromMem(NG_STATSX+2*NG_SPACINGX-SHORT(items->width),
  NG_STATSY, FB, items);

  V_DrawPatchStretchedFromMem(NG_STATSX+3*NG_SPACINGX-SHORT(secret->width),
  NG_STATSY, FB, secret);
  
  if (dofrags)
    V_DrawPatchStretchedFromMem(NG_STATSX+4*NG_SPACINGX-SHORT(frags->width),
      NG_STATSY, FB, frags);

  // draw stats
  y = NG_STATSY + SHORT(kills->height);

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (!playeringame[i])
      continue;

    x = NG_STATSX;
    V_DrawPatchStretchedFromMem(x-SHORT(p[i]->width), y, FB, p[i]);

    if (i == me)
      V_DrawPatchStretchedFromMem(x-SHORT(p[i]->width), y, FB, star);

    x += NG_SPACINGX;
    WI_drawPercent(x-pwidth, y+10, cnt_kills[i]); x += NG_SPACINGX;
    WI_drawPercent(x-pwidth, y+10, cnt_items[i]); x += NG_SPACINGX;
    WI_drawPercent(x-pwidth, y+10, cnt_secret[i]);  x += NG_SPACINGX;

    if (dofrags)
      WI_drawNum(x, y+10, cnt_frags[i], -1);

    y += WI_SPACINGY;
  }
}

static int  sp_state;

// ====================================================================
// WI_initStats
// Purpose: Get ready for single player stats
// Args:    none
// Returns: void
// Comment: Seems like we could do all these stats in a more generic
//          set of routines that weren't duplicated for dm, coop, sp
//
static void WI_initStats(void)
{
  state = StatCount;
  acceleratestage = 0;
  sp_state = 1;
  cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
  cnt_time = cnt_par = -1;
  cnt_pause = TICRATE;

  WI_initAnimatedBack();
}

// ====================================================================
// WI_updateStats
// Purpose: Calculate solo stats
// Args:    none
// Returns: void
//
static void WI_updateStats(void)
{
  WI_updateAnimatedBack();

  if (acceleratestage && sp_state != 10)
  {
    acceleratestage = 0;
    cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
    cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;

    // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
    cnt_secret[0] = (wbs->maxsecret ? 
      (plrs[me].ssecret * 100) / wbs->maxsecret : 100);

    cnt_time = plrs[me].stime / TICRATE;
    cnt_par = wbs->partime / TICRATE;
    S_StartSound(0, sfx_barexp);
    sp_state = 10;
  }

  if (sp_state == 2)
  {
    cnt_kills[0] += 2;

    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
    {
      cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
      S_StartSound(0, sfx_barexp);
      sp_state++;
    }
  }
  else if (sp_state == 4)
  {
    cnt_items[0] += 2;

    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
    {
      cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
      S_StartSound(0, sfx_barexp);
      sp_state++;
    }
  }
  else if (sp_state == 6)
  {
    cnt_secret[0] += 2;

    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
    if (cnt_secret[0] >= (wbs->maxsecret ? 
      (plrs[me].ssecret * 100) / wbs->maxsecret : 100))
    {
      cnt_secret[0] = (wbs->maxsecret ? 
        (plrs[me].ssecret * 100) / wbs->maxsecret : 100);
      S_StartSound(0, sfx_barexp);
      sp_state++;
    }
  }
  else if (sp_state == 8)
  {
    if (!(bcnt&3))
      S_StartSound(0, sfx_pistol);

    cnt_time += 3;

    if (cnt_time >= plrs[me].stime / TICRATE)
      cnt_time = plrs[me].stime / TICRATE;

    cnt_par += 3;

    if (cnt_par >= wbs->partime / TICRATE)
    {
      cnt_par = wbs->partime / TICRATE;

      if (cnt_time >= plrs[me].stime / TICRATE)
      {
        S_StartSound(0, sfx_barexp);
        sp_state++;
      }
    }
  }
  else if (sp_state == 10)
  {
    if (acceleratestage)
    {
      S_StartSound(0, sfx_sgcock);

      if (gamemode == commercial)
        WI_initNoState();
      else
        WI_initShowNextLoc();
    }
  }
  else if (sp_state & 1)
  {
    if (!--cnt_pause)
    {
      sp_state++;
      cnt_pause = TICRATE;
    }
  }
}

// ====================================================================
// WI_drawStats
// Purpose: Put the solo stats on the screen
// Args:    none
// Returns: void
//
static void WI_drawStats(void)
{
  // line height
  int lh; 

  lh = (3*SHORT(num[0]->height))/2;

  WI_slamBackground();

  // draw animated background
  WI_drawAnimatedBack();
  
  WI_drawLF();

  V_DrawPatchStretchedFromMem(SP_STATSX, SP_STATSY, FB, kills);
  WI_drawPercent(320 - SP_STATSX, SP_STATSY, cnt_kills[0]);

  V_DrawPatchStretchedFromMem(SP_STATSX, SP_STATSY+lh, FB, items);
  WI_drawPercent(320 - SP_STATSX, SP_STATSY+lh, cnt_items[0]);

  V_DrawPatchStretchedFromMem(SP_STATSX, SP_STATSY+2*lh, FB, sp_secret);
  WI_drawPercent(320 - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);

  V_DrawPatchStretchedFromMem(SP_TIMEX, SP_TIMEY, FB, time);
  WI_drawTime(320/2 - SP_TIMEX, SP_TIMEY, cnt_time);

  // Ty 04/11/98: redid logic: should skip only if with pwad but 
  // without deh patch
  // killough 2/22/98: skip drawing par times on pwads
  // Ty 03/17/98: unless pars changed with deh patch

  if (!modifiedgame || deh_pars)
  {
    if (wbs->epsd < 3)
    {
      V_DrawPatchStretchedFromMem(320/2 + SP_TIMEX, SP_TIMEY, FB, par);
      WI_drawTime(320 - SP_TIMEX, SP_TIMEY, cnt_par);
    }
  }
}

// ====================================================================
// WI_checkForAccelerate
// Purpose: See if the player has hit either the attack or use key
//          or mouse button.  If so we set acceleratestage to 1 and
//          all those display routines above jump right to the end.
// Args:    none
// Returns: void
//
void WI_checkForAccelerate(void)
{
  int   i;
  player_t  *player;

  // check for button presses to skip delays
  for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
  {
    if (playeringame[i])
    {
      if (player->cmd.buttons & BT_ATTACK)
      {
        if (!player->attackdown)
          acceleratestage = 1;
        player->attackdown = true;
      }
      else
        player->attackdown = false;

      if (player->cmd.buttons & BT_USE)
      {
        if (!player->usedown)
          acceleratestage = 1;
        player->usedown = true;
      }
      else
        player->usedown = false;
    }
  }
}

// ====================================================================
// WI_Ticker
// Purpose: Do various updates every gametic, for stats, animation,
//          checking that intermission music is running, etc.
// Args:    none
// Returns: void
//
void WI_Ticker(void)
{
  // counter for general background animation
  bcnt++;  

  if (bcnt == 1)
  {
    // intermission music
    if ( gamemode == commercial )
      S_ChangeMusic(mus_dm2int, true);
    else
      S_ChangeMusic(mus_inter, true); 
  }

  WI_checkForAccelerate();

  switch (state)
  {
    case StatCount:
         if (deathmatch) WI_updateDeathmatchStats();
         else if (netgame) WI_updateNetgameStats();
         else WI_updateStats();
         break;
  
    case ShowNextLoc:
         WI_updateShowNextLoc();
         break;
  
    case NoState:
         WI_updateNoState();
         break;
  }
}

// killough 11/98:
// Moved to separate function so that i_video.c could call it

void WI_DrawBackground(void)
{
  char  name[9];  // limited to 8 characters

  if (gamemode == commercial || (gamemode == retail && wbs->epsd == 3))
    strcpy(name, "INTERPIC");
  else 
    sprintf(name, "WIMAP%d", wbs->epsd);

  // background
  V_DrawPatchStretchedFromName(0, 0, 0, name);
}

// ====================================================================
// WI_loadData
// Purpose: Initialize intermission data such as background graphics,
//          patches, map names, etc.
// Args:    none
// Returns: void
//
static void WI_loadData(void)
{
  int   i,j;
  char  name[9];

  WI_DrawBackground();         // killough 11/98

  // killough 4/26/98: free lnames here (it was freed too early in Doom)
  Z_Free(lnames);

  if (gamemode == commercial)
  {
    NUMCMAPS = 32;                
    lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMCMAPS,
      PU_STATIC, 0);
    for (i=0 ; i<NUMCMAPS ; i++)
    { 
      sprintf(name, "CWILV%2.2d", i);
      lnames[i] = W_CacheLumpName(name, PU_STATIC);
    }         
  }
  else
  {
    lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMMAPS,
      PU_STATIC, 0);
    for (i=0 ; i<NUMMAPS ; i++)
    {
      sprintf(name, "WILV%d%d", wbs->epsd, i);
      lnames[i] = W_CacheLumpName(name, PU_STATIC);
    }

    // you are here
    yah[0] = W_CacheLumpName("WIURH0", PU_STATIC);

    // you are here (alt.)
    yah[1] = W_CacheLumpName("WIURH1", PU_STATIC);

    // splat
    splat = W_CacheLumpName("WISPLAT", PU_STATIC); 
  
    if (wbs->epsd < 3)
    {
      for (j=0;j<NUMANIMS[wbs->epsd];j++)
      {
	      anim_t *a = &anims[wbs->epsd][j];
        for (i=0;i<a->nanims;i++)
        {
          // MONDO HACK!
          if (wbs->epsd != 1 || j != 8) 
          {
            // animations
            sprintf(name, "WIA%d%.2d%.2d", wbs->epsd, j, i);  
            a->p[i] = W_CacheLumpName(name, PU_STATIC);
          }
          else
          {
            // HACK ALERT!
            a->p[i] = anims[1][4].p[i]; 
          }
        }
      }
    }
  }

  // More hacks on minus sign.
  wiminus = W_CacheLumpName("WIMINUS", PU_STATIC); 

  for (i=0;i<10;i++)
  {
    // numbers 0-9
    sprintf(name, "WINUM%d", i);     
    num[i] = W_CacheLumpName(name, PU_STATIC);
  }

  // percent sign
  percent = W_CacheLumpName("WIPCNT", PU_STATIC);

  // "finished"
  finished = W_CacheLumpName("WIF", PU_STATIC);

  // "entering"
  entering = W_CacheLumpName("WIENTER", PU_STATIC);

  // "kills"
  kills = W_CacheLumpName("WIOSTK", PU_STATIC);   

  // "scrt"
  secret = W_CacheLumpName("WIOSTS", PU_STATIC);

  // "secret"
  sp_secret = W_CacheLumpName("WISCRT2", PU_STATIC);

  // Yuck. // Ty 03/27/98 - got that right :)  
  // french is an enum=1 always true.
  //    if (french)
  //    {
  //  // "items"
  //  if (netgame && !deathmatch)
  //      items = W_CacheLumpName("WIOBJ", PU_STATIC);    
  //    else
  //      items = W_CacheLumpName("WIOSTI", PU_STATIC);
  //    } else

  items = W_CacheLumpName("WIOSTI", PU_STATIC);

  // "frgs"
  frags = W_CacheLumpName("WIFRGS", PU_STATIC);    

  // ":"
  colon = W_CacheLumpName("WICOLON", PU_STATIC); 

  // "time"
  time = W_CacheLumpName("WITIME", PU_STATIC);   

  // "sucks"
  sucks = W_CacheLumpName("WISUCKS", PU_STATIC);  

  // "par"
  par = W_CacheLumpName("WIPAR", PU_STATIC);   

  // "killers" (vertical)
  killers = W_CacheLumpName("WIKILRS", PU_STATIC);
  
  // "victims" (horiz)
  victims = W_CacheLumpName("WIVCTMS", PU_STATIC);

  // "total"
  total = W_CacheLumpName("WIMSTT", PU_STATIC);   

  // your face
  star = W_CacheLumpName("STFST01", PU_STATIC);

  // dead face
  bstar = W_CacheLumpName("STFDEAD0", PU_STATIC);    

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    // "1,2,3,4"
    sprintf(name, "STPB%d", i);      
    p[i] = W_CacheLumpName(name, PU_STATIC);

    // "1,2,3,4"
    sprintf(name, "WIBP%d", i+1);     
    bp[i] = W_CacheLumpName(name, PU_STATIC);
  }
}

// ====================================================================
// WI_Drawer
// Purpose: Call the appropriate stats drawing routine depending on
//          what kind of game is being played (DM, coop, solo)
// Args:    none
// Returns: void
//
void WI_Drawer (void)
{
  switch (state)
  {
    case StatCount:
         if (deathmatch)
           WI_drawDeathmatchStats();
         else if (netgame)
           WI_drawNetgameStats();
         else
           WI_drawStats();
         break;
  
    case ShowNextLoc:
         WI_drawShowNextLoc();
         break;
  
    case NoState:
         WI_drawNoState();
         break;
  }
}


// ====================================================================
// WI_initVariables
// Purpose: Initialize the intermission information structure
//          Note: wbstartstruct_t is defined in d_player.h
// Args:    wbstartstruct -- pointer to the structure with the data
// Returns: void
//
static void WI_initVariables(wbstartstruct_t* wbstartstruct)
{

  wbs = wbstartstruct;

#ifdef RANGECHECKING
  if (gamemode != commercial)
  {
    if ( gamemode == retail )
      RNGCHECK(wbs->epsd, 0, 3);
    else
      RNGCHECK(wbs->epsd, 0, 2);
  }
  else
  {
    RNGCHECK(wbs->last, 0, 8);
    RNGCHECK(wbs->next, 0, 8);
  }
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

  acceleratestage = 0;
  cnt = bcnt = 0;
  firstrefresh = 1;
  me = wbs->pnum;
  plrs = wbs->plyr;

  if (!wbs->maxkills)
    wbs->maxkills = 1;  // probably only useful in MAP30

  if (!wbs->maxitems)
    wbs->maxitems = 1;

  // killough 2/22/98: Keep maxsecret=0 if it's zero, so
  // we can detect 0/0 as as a special case and print 100%.
  //
  //    if (!wbs->maxsecret)
  //  wbs->maxsecret = 1;

  if ( gamemode != retail )
    if (wbs->epsd > 2)
      wbs->epsd -= 3;
}

// ====================================================================
// WI_Start
// Purpose: Call the various init routines
//          Note: wbstartstruct_t is defined in d_player.h
// Args:    wbstartstruct -- pointer to the structure with the 
//          intermission data
// Returns: void
//
void WI_Start(wbstartstruct_t* wbstartstruct)
{
  WI_initVariables(wbstartstruct);
  WI_loadData();

  if (deathmatch)
    WI_initDeathmatchStats();
  else if (netgame)
    WI_initNetgameStats();
  else
    WI_initStats();
}


//----------------------------------------------------------------------------
//
// $Log: Wi_stuff.c,v $
// Revision 1.1  2000/04/09 18:15:35  proff_fs
// Initial revision
//
// Revision 1.12  1998/09/07  20:10:39  jim
// Logical output routine added
//
// Revision 1.11  1998/05/04  21:36:02  thldrmn
// commenting and reformatting
//
// Revision 1.10  1998/05/03  22:45:35  killough
// Provide minimal correct #include's at top; nothing else
//
// Revision 1.9  1998/04/27  02:11:44  killough
// Fix lnames being freed too early causing crashes
//
// Revision 1.8  1998/04/26  14:55:38  jim
// Fixed animated back bug
//
// Revision 1.7  1998/04/11  14:49:52  thldrmn
// Fixed par display logic
//
// Revision 1.6  1998/03/28  18:12:03  killough
// Make acceleratestage external so it can be used for teletype
//
// Revision 1.5  1998/03/28  05:33:12  jim
// Text enabling changes for DEH
//
// Revision 1.4  1998/03/18  23:14:14  jim
// Deh text additions
//
// Revision 1.3  1998/02/23  05:00:19  killough
// Fix Secret percentage, avoid par times on pwads
//
// Revision 1.2  1998/01/26  19:25:12  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:05  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
