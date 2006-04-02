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
//  DOOM Intermission screens.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: wi_stuff.c,v 1.11 1998/05/04 21:36:02 thldrmn Exp $";

#include "doomstat.h"
#include "m_random.h"
#include "w_wad.h"
#include "g_game.h"
#include "r_main.h"
#include "p_info.h"
#include "r_main.h"
#include "v_video.h"
#include "wi_stuff.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_mobj.h"
#include "d_gi.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "am_map.h"
#include "p_tick.h"
#include "in_lude.h"
#include "c_io.h"
#include "d_deh.h"
#include "e_string.h"

extern char gamemapname[9];

// Ty 03/17/98: flag that new par times have been loaded in d_deh
// haleyjd: moved to header

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

//
// Different between registered DOOM (1994) and
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
#define SP_TIMEY      (SCREENHEIGHT-32)

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
   
   // next value of intertime (used in conjunction with period)
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

// wbs->pnum
static int    me;

// specifies current state
static stateenum_t  state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static wbplayerstruct_t* plrs;  // wbs->plyr[]

// used for general timing
static int    cnt;  

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

// background (map of levels).
static patch_t*   bg;

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

// haleyjd: counter based on wi_pause_time
static int cur_pause_time;

// haleyjd: whether decision to fade bg graphics has been made yet
static boolean fade_applied = false;

// haleyjd 03/27/05: EDF-defined intermission map names
static edf_string_t *mapName;
static edf_string_t *nextMapName;

// globals

// haleyjd 02/02/05: intermission pause time -- see EDF
int wi_pause_time = 0;

// haleyjd 02/02/05: color fade
// Allows darkening the background after optional pause expires.
// This was inspired by Contra III and is used by CQIII. See EDF.
int     wi_fade_color = -1;
fixed_t wi_tl_level   =  0;

// forward declaration
static void WI_OverlayBackground(void);

//
// CODE
//

// ====================================================================
// WI_drawLF
// Purpose: Draw the "Finished" level name before showing stats
// Args:    none
// Returns: void
//
static void WI_drawLF(void)
{
   int y = WI_TITLEY;
   patch_t *patch = NULL;
   
   // haleyjd 07/08/04: fixed to work for any map
   // haleyjd 03/27/05: added string functionality

   if(LevelInfo.levelPic)
      patch = W_CacheLumpName(LevelInfo.levelPic, PU_CACHE);
   else
   {
      // haleyjd: do not index lnames out of bounds
      if(wbs->last >= 0 &&
         ((gamemode == commercial && wbs->last < NUMCMAPS) ||
          wbs->last < NUMMAPS))
      {
         patch = lnames[wbs->last];
      }
   }

   if(patch || mapName)
   {
      // draw <LevelName> 
      if(mapName)
      {
         V_WriteTextBig(mapName->string, 
            (SCREENWIDTH - V_StringWidthBig(mapName->string)) / 2,
            y);
         y += (5 * V_StringHeightBig(mapName->string)) / 4;
      }
      else
      {
         V_DrawPatch((SCREENWIDTH - SHORT(patch->width))/2,
                     y, &vbscreen, patch);
         y += (5 * SHORT(patch->height)) / 4;
      }
      
      // draw "Finished!"
      V_DrawPatch((SCREENWIDTH - SHORT(finished->width))/2,
                  y, &vbscreen, finished);
   }
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
   patch_t *patch = NULL;

   if(wbs->next >= 0 &&
      ((gamemode == commercial && wbs->next < NUMCMAPS) ||
      wbs->next < NUMMAPS))
   {
      patch = lnames[wbs->next];
   }

   if(patch || nextMapName)
   {
      // draw "Entering"
      V_DrawPatch((SCREENWIDTH - SHORT(entering->width))/2,
                  y, &vbscreen, entering);

      // haleyjd: corrected to use height of entering, not map name
      y += (5*SHORT(entering->height))/4;

      // draw level
      if(nextMapName)
      {
         V_WriteTextBig(nextMapName->string,
            (SCREENWIDTH - V_StringWidthBig(nextMapName->string)) / 2,
            y);
      }
      else
      {
         V_DrawPatch((SCREENWIDTH - SHORT(patch->width))/2,
                     y, &vbscreen, patch);
      }
   }
}


// ====================================================================
// WI_drawOnLnode
// Purpose: Draw patches at a location based on episode/map
// Args:    n   -- index to map# within episode
//          c[] -- array of patches to be drawn
//          numpatches -- haleyjd 04/12/03: bug fix - number of patches
// Returns: void
//
// draw stuff at a location by episode/map#
//
static void WI_drawOnLnode(int n, patch_t *c[], int numpatches)
{
   int   i;
   int   left;
   int   top;
   int   right;
   int   bottom;
   boolean fits = false;
   
   i = 0;
   do
   {
      left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
      top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
      right = left + SHORT(c[i]->width);
      bottom = top + SHORT(c[i]->height);
      
      if(left >= 0
         && right < SCREENWIDTH
         && top >= 0
         && bottom < SCREENHEIGHT)
         fits = true;
      else
         i++;
   } 
   while(!fits && i != numpatches); // haleyjd: bug fix

   if(fits && i < numpatches) // haleyjd: bug fix
   {
      V_DrawPatch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y,
                  &vbscreen, c[i]);
   }
   else
   {
      // haleyjd: changed printf to C_Printf
     
      // DEBUG
      C_Printf(FC_ERROR "Could not place patch on level %d\n", 
               n+1);
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

   if(gamemode == commercial)  // no animation for DOOM2
      return;

   if(wbs->epsd > 2)
      return;

   for(i = 0; i < NUMANIMS[wbs->epsd]; ++i)
   {
      a = &anims[wbs->epsd][i];
      
      // init variables
      a->ctr = -1;
  
      // specify the next time to draw it
      switch(a->type)
      {
      case ANIM_ALWAYS:
         a->nexttic = intertime + 1 + (M_Random() % a->period);
         break;
      case ANIM_RANDOM:
         a->nexttic = intertime + 1 + a->data2 + (M_Random() % a->data1);
         break;
      case ANIM_LEVEL:
         a->nexttic = intertime + 1;
         break;
      }
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
   int     i;
   anim_t *a;

   if(gamemode == commercial)
      return;

   if(wbs->epsd > 2)
      return;

   for(i = 0; i < NUMANIMS[wbs->epsd]; ++i)
   {
      a = &anims[wbs->epsd][i];
      
      if(intertime == a->nexttic)
      {
         switch(a->type)
         {
         case ANIM_ALWAYS:
            if(++a->ctr >= a->nanims) 
               a->ctr = 0;
            a->nexttic = intertime + a->period;
            break;

         case ANIM_RANDOM:
            a->ctr++;
            if(a->ctr == a->nanims)
            {
               a->ctr = -1;
               a->nexttic = intertime + a->data2 + (M_Random() % a->data1);
            }
            else 
               a->nexttic = intertime + a->period;
            break;
    
         case ANIM_LEVEL:
            // gawd-awful hack for level anims
            if(!(state == StatCount && i == 7)
               && wbs->next == a->data1)
            {
               a->ctr++;
               if(a->ctr == a->nanims)
                  a->ctr--;
               a->nexttic = intertime + a->period;
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
   anim_t *a;

   if(gamemode == commercial) //jff 4/25/98 Someone forgot commercial an enum
      return;

   if(wbs->epsd > 2)
      return;
   
   for(i = 0; i < NUMANIMS[wbs->epsd]; ++i)
   {
      a = &anims[wbs->epsd][i];
      
      if(a->ctr >= 0)
         V_DrawPatch(a->loc.x, a->loc.y, &vbscreen, a->p[a->ctr]);
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
static int WI_drawNum(int x, int y, int n, int digits)
{
   int neg, temp, fontwidth = SHORT(num[0]->width);

   neg = n < 0;    // killough 11/98: move up to here, for /= 10 division below
   if(neg)
      n = -n;

   if(digits < 0)
   {
      if(!n)
      {
         // make variable-length zeros 1 digit long
         digits = 1;
      }
      else
      {
         // figure out # of digits in #
         digits = 0;
         temp = n;
         
         while(temp)
         {
            temp /= 10;
            ++digits;
         }
      }
   }

   // if non-number, do not draw it
   if(n == 1994)
      return 0;

   // draw the new number
   while(digits--)
   {
      x -= fontwidth;
      V_DrawPatch(x, y, &vbscreen, num[ n % 10 ]);
      n /= 10;
   }

   // draw a minus sign if necessary
   if(neg)
      V_DrawPatch(x -= 8, y, &vbscreen, wiminus);
   
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
static void WI_drawPercent(int x, int y, int p )
{
   if(p < 0)
      return;
   
   V_DrawPatch(x, y, &vbscreen, percent);
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
static void WI_drawTime(int x, int y, int t)
{  
   int div, n;
   
   if(t < 0)
      return;
   
   if(t <= 61*59)  // otherwise known as 60*60 -1 == 3599
   {
      div = 1;
      
      do
      {
         n = (t / div) % 60;
         x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
         div *= 60;
         
         // draw
         if(div == 60 || t / div)
            V_DrawPatch(x, y, &vbscreen, colon);
         
      } while(t / div);
   }
   else
   {
      // "sucks"
      V_DrawPatch(x - SHORT(sucks->width), y, &vbscreen, sucks); 
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
   int i, j;
   
   Z_ChangeTag(wiminus, PU_CACHE);
   
   for(i = 0; i < 10; ++i)
      Z_ChangeTag(num[i], PU_CACHE);
   
   if(gamemode == commercial)
   {
      for(i = 0; i < NUMCMAPS; ++i)
         Z_ChangeTag(lnames[i], PU_CACHE);
   }
   else
   {
      Z_ChangeTag(yah[0], PU_CACHE);
      Z_ChangeTag(yah[1], PU_CACHE);
      
      Z_ChangeTag(splat, PU_CACHE);
      
      for(i = 0; i < NUMMAPS; ++i)
         Z_ChangeTag(lnames[i], PU_CACHE);
      
      if(wbs->epsd < 3)
      {
         for(j = 0; j < NUMANIMS[wbs->epsd]; ++j)
         {
            if(wbs->epsd != 1 || j != 8)
               for(i = 0; i < anims[wbs->epsd][j].nanims; ++i)
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
  
   for(i = 0; i < MAXPLAYERS; ++i)
      Z_ChangeTag(p[i], PU_CACHE);

   for(i = 0; i < MAXPLAYERS; ++i)
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
   
   if(!--cnt)
   {
      WI_End();
      G_WorldDone();
   }
}

static boolean snl_pointeron = false;


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
   
   if(!--cnt || acceleratestage)
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
   int   i, last;

   IN_slamBackground();
   
   // draw animated background
   WI_drawAnimatedBack(); 

   if(gamemode != commercial)
   {
      if(wbs->epsd > 2)
      {
         WI_drawEL();  // "Entering..." if not E1 or E2 or E3
         return;
      }
  
      last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;
      
      // draw a splat on taken cities.
      for(i = 0; i <= last; ++i)
         WI_drawOnLnode(i, &splat, 1); // haleyjd 04/12/03: bug fix

      // splat the secret level?
      if(wbs->didsecret)
         WI_drawOnLnode(8, &splat, 1);

      // draw flashing ptr
      if(snl_pointeron)
         WI_drawOnLnode(wbs->next, yah, 2); 
   }

   // draws which level you are entering..
   // check for end of game -- haleyjd 07/08/04: use map info
   if((gamemode != commercial) || !LevelInfo.endOfGame)
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
   int i, frags = 0;
    
   for(i = 0; i < MAXPLAYERS; ++i)
   {
      if(playeringame[i]  // is this player playing?
         && i != playernum) // and it's not the player we're calculating
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

static int dm_state;
static int dm_frags[MAXPLAYERS][MAXPLAYERS];  // frags matrix
static int dm_totals[MAXPLAYERS];  // totals by player

// ====================================================================
// WI_initDeathmatchStats
// Purpose: Set up to display DM stats at end of level.  Calculate 
//          frags for all players.
// Args:    none
// Returns: void
//
static void WI_initDeathmatchStats(void)
{
   int i, j; // looping variables
   
   state = StatCount;  // We're doing stats
   acceleratestage = 0;
   dm_state = 1;  // count how many times we've done a complete stat

   cnt_pause = TICRATE;
   
   for(i = 0; i < MAXPLAYERS; ++i)
   {
      if(playeringame[i])
      { 
         for(j = 0; j < MAXPLAYERS; ++j)
         {
            if(playeringame[j])
               dm_frags[i][j] = 0;  // set all counts to zero
         }
         
         dm_totals[i] = 0;
      }
   }
   WI_initAnimatedBack();
}


// ====================================================================
// WI_updateDeathmatchStats
// Purpose: Update numbers for deathmatch intermission. Lots of noise 
//          and drama around the presentation.
// Args:    none
// Returns: void
//
static void WI_updateDeathmatchStats(void)
{
   int     i, j;    
   boolean stillticking;
   
   WI_updateAnimatedBack();

   if(cur_pause_time > 0)
   {
      if(acceleratestage)
      {
         cur_pause_time = 0;
         acceleratestage = 0;
      }
      else
      {
         --cur_pause_time;
         return;
      }
   }
   
   if(acceleratestage && dm_state != 4)  // still ticking
   {
      acceleratestage = 0;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(playeringame[i])
         {
            for(j = 0; j < MAXPLAYERS; ++j)
            {
               if(playeringame[j])
                  dm_frags[i][j] = plrs[i].frags[j];
            }
            
            dm_totals[i] = WI_fragSum(i);
         }
      }
  
      S_StartSound(NULL, sfx_barexp);  // bang
      dm_state = 4;  // we're done with all 4 (or all we have to do)
   }
    
   if(dm_state == 2)
   {
      if(!(intertime&3))
         S_StartSound(NULL, sfx_pistol);  // noise while counting
  
      stillticking = false;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(playeringame[i])
         {
            for(j = 0; j < MAXPLAYERS; ++j)
            {
               if(playeringame[j] && dm_frags[i][j] != plrs[i].frags[j])
               {
                  if(plrs[i].frags[j] < 0)
                     dm_frags[i][j]--;
                  else
                     dm_frags[i][j]++;

                  if(dm_frags[i][j] > 999) // Ty 03/17/98 3-digit frag count
                     dm_frags[i][j] = 999;
                  
                  if(dm_frags[i][j] < -999)
                     dm_frags[i][j] = -999;
                  
                  stillticking = true;
               }
            }
            dm_totals[i] = WI_fragSum(i);
            
            if(dm_totals[i] > 999)
               dm_totals[i] = 999;
            
            if(dm_totals[i] < -999)
               dm_totals[i] = -999;  // Ty 03/17/98 end 3-digit frag count
         }
      }

      if(!stillticking)
      {
         S_StartSound(NULL, sfx_barexp);
         dm_state++;
      }
   }
   else if(dm_state == 4)
   {
      if(acceleratestage)
      {   
         S_StartSound(NULL, sfx_slop);

         if(gamemode == commercial)
            WI_initNoState();
         else
            WI_initShowNextLoc();
      }
   }
   else if(dm_state & 1)
   {
      if(!--cnt_pause)
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
   int   i, j, x, y, w;  
   int   lh; // line height
   
   lh = WI_SPACINGY;

   if(!(fade_applied || cur_pause_time))
   {
      if(wi_fade_color != -1)
         WI_OverlayBackground();
      fade_applied = true;
   }
   
   IN_slamBackground();
  
   // draw animated background
   WI_drawAnimatedBack();

   if(cur_pause_time > 0)
      return;

   WI_drawLF();

   // draw stat titles (top line)
   V_DrawPatch(DM_TOTALSX-SHORT(total->width)/2,
               DM_MATRIXY-WI_SPACINGY+10,
               &vbscreen,
               total);
  
   V_DrawPatch(DM_KILLERSX, DM_KILLERSY, &vbscreen, killers);
   V_DrawPatch(DM_VICTIMSX, DM_VICTIMSY, &vbscreen, victims);

   // draw P?
   x = DM_MATRIXX + DM_SPACINGX;
   y = DM_MATRIXY;

   for(i = 0; i < MAXPLAYERS; ++i)
   {
      if (playeringame[i])
      {
         V_DrawPatch(x-SHORT(p[i]->width)/2,
                     DM_MATRIXY - WI_SPACINGY,
                     &vbscreen,
                     p[i]);
      
         V_DrawPatch(DM_MATRIXX-SHORT(p[i]->width)/2,
                     y,
                     &vbscreen,
                     p[i]);

         if(i == me)
         {
            V_DrawPatch(x-SHORT(p[i]->width)/2,
                        DM_MATRIXY - WI_SPACINGY,
                        &vbscreen,
                        bstar);

            V_DrawPatch(DM_MATRIXX-SHORT(p[i]->width)/2,
                        y,
                        &vbscreen,
                        star);
         }
      }
      x += DM_SPACINGX;
      y += WI_SPACINGY;
   }

   // draw stats
   y = DM_MATRIXY + 10;
   w = SHORT(num[0]->width);

   for(i = 0; i < MAXPLAYERS; ++i)
   {
      x = DM_MATRIXX + DM_SPACINGX;
      
      if(playeringame[i])
      {
         for(j = 0; j < MAXPLAYERS; ++j)
         {
            if(playeringame[j])
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
// (haleyjd: not any more since I cleaned that up using GameType)
//
static int cnt_frags[MAXPLAYERS];
static int dofrags;
static int ng_state;


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
   
   for(i = 0; i < MAXPLAYERS; ++i)
   {
      if(!playeringame[i])
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
   int i, fsum;    
   boolean stillticking;
   
   WI_updateAnimatedBack();

   if(cur_pause_time > 0)
   {
      if(acceleratestage)
      {
         cur_pause_time = 0;
         acceleratestage = 0;
      }
      else
      {
         --cur_pause_time;
         return;
      }
   }
   
   if(acceleratestage && ng_state != 10)
   {
      acceleratestage = 0;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(!playeringame[i])
            continue;
         
         cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
         cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
         
         // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
         cnt_secret[i] = wbs->maxsecret ? 
            (plrs[i].ssecret * 100) / wbs->maxsecret : 100;
         if(dofrags)
            cnt_frags[i] = WI_fragSum(i);  // we had frags
      }
      S_StartSound(NULL, sfx_barexp);  // bang
      ng_state = 10;
   }
   
   if(ng_state == 2)
   {
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);  // pop
      
      stillticking = false;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(!playeringame[i])
            continue;
         
         cnt_kills[i] += 2;
         
         if(cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
            cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
         else
            stillticking = true; // still got stuff to tally
      }
      
      if(!stillticking)
      {
         S_StartSound(NULL, sfx_barexp); 
         ng_state++;
      }
   }
   else if(ng_state == 4)
   {
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);
      
      stillticking = false;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(!playeringame[i])
            continue;
         
         cnt_items[i] += 2;
         if(cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
            cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
         else
            stillticking = true;
      }
      
      if(!stillticking)
      {
         S_StartSound(NULL, sfx_barexp);
         ng_state++;
      }
   }
   else if(ng_state == 6)
   {
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);
      
      stillticking = false;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(!playeringame[i])
            continue;
         
         cnt_secret[i] += 2;
         
         // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
         
         if(cnt_secret[i] >= (wbs->maxsecret ? (plrs[i].ssecret * 100) / wbs->maxsecret : 100))
            cnt_secret[i] = wbs->maxsecret ? (plrs[i].ssecret * 100) / wbs->maxsecret : 100;
         else
            stillticking = true;
      }
      
      if(!stillticking)
      {
         S_StartSound(NULL, sfx_barexp);
         ng_state += 1 + 2*!dofrags;
      }
   }
   else if(ng_state == 8)
   {
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);
      
      stillticking = false;
      
      for(i = 0; i < MAXPLAYERS; ++i)
      {
         if(!playeringame[i])
            continue;
         
         cnt_frags[i] += 1;
         
         if(cnt_frags[i] >= (fsum = WI_fragSum(i)))
            cnt_frags[i] = fsum;
         else
            stillticking = true;
      }
      
      if(!stillticking)
      {
         S_StartSound(NULL, sfx_pldeth);
         ng_state++;
      }
   }
   else if(ng_state == 10)
   {
      if(acceleratestage)
      {
         S_StartSound(NULL, sfx_sgcock);
         if(gamemode == commercial)
            WI_initNoState();
         else
            WI_initShowNextLoc();
      }
   }
   else if(ng_state & 1)
   {
      if(!--cnt_pause)
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
   int i, x, y, pwidth = SHORT(percent->width);

   if(!(fade_applied || cur_pause_time))
   {
      if(wi_fade_color != -1)
         WI_OverlayBackground();
      fade_applied = true;
   }

   IN_slamBackground();
   
   // draw animated background
   WI_drawAnimatedBack(); 

   if(cur_pause_time > 0)
      return;
   
   WI_drawLF();

   // draw stat titles (top line)
   V_DrawPatch(NG_STATSX+NG_SPACINGX-SHORT(kills->width),
               NG_STATSY, &vbscreen, kills);

   V_DrawPatch(NG_STATSX+2*NG_SPACINGX-SHORT(items->width),
               NG_STATSY, &vbscreen, items);

   V_DrawPatch(NG_STATSX+3*NG_SPACINGX-SHORT(secret->width),
               NG_STATSY, &vbscreen, secret);
  
   if(dofrags)
      V_DrawPatch(NG_STATSX+4*NG_SPACINGX-SHORT(frags->width),
                  NG_STATSY, &vbscreen, frags);

   // draw stats
   y = NG_STATSY + SHORT(kills->height);

   for(i = 0; i < MAXPLAYERS; ++i)
   {
      if(!playeringame[i])
         continue;
      
      x = NG_STATSX;
      V_DrawPatch(x-SHORT(p[i]->width), y, &vbscreen, p[i]);
      
      if(i == me)
         V_DrawPatch(x-SHORT(p[i]->width), y, &vbscreen, star);
      
      x += NG_SPACINGX;
      WI_drawPercent(x-pwidth, y+10, cnt_kills[i]); x += NG_SPACINGX;
      WI_drawPercent(x-pwidth, y+10, cnt_items[i]); x += NG_SPACINGX;
      WI_drawPercent(x-pwidth, y+10, cnt_secret[i]);  x += NG_SPACINGX;

      if(dofrags)
         WI_drawNum(x, y+10, cnt_frags[i], -1);
      
      y += WI_SPACINGY;
   }
}

static int sp_state;

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

   // haleyjd 02/02/05: allow an initial intermission pause
   if(cur_pause_time > 0)
   {
      if(acceleratestage)
      {
         cur_pause_time = 0;
         acceleratestage = 0;
      }
      else
      {
         --cur_pause_time;
         return;
      }
   }
   
   if(acceleratestage && sp_state != 10)
   {
      acceleratestage = 0;
      cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
      cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
      
      // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
      cnt_secret[0] = (wbs->maxsecret ? 
                       (plrs[me].ssecret * 100) / wbs->maxsecret : 100);

      cnt_time = plrs[me].stime / TICRATE;
      cnt_par = wbs->partime==-1 ? 0 : wbs->partime/TICRATE;
      S_StartSound(NULL, sfx_barexp);
      sp_state = 10;
   }

   if(sp_state == 2)
   {
      cnt_kills[0] += 2;
      
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);

      if(cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
      {
         cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
         S_StartSound(NULL, sfx_barexp);
         sp_state++;
      }
   }
   else if(sp_state == 4)
   {
      cnt_items[0] += 2;
      
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);
      
      if(cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
      {
         cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
         S_StartSound(NULL, sfx_barexp);
         sp_state++;
      }
   }
   else if(sp_state == 6)
   {
      cnt_secret[0] += 2;
      
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);

      // killough 2/22/98: Make secrets = 100% if maxsecret = 0:
      if(cnt_secret[0] >= (wbs->maxsecret ? 
         (plrs[me].ssecret * 100) / wbs->maxsecret : 100))
      {
         cnt_secret[0] = (wbs->maxsecret ? 
            (plrs[me].ssecret * 100) / wbs->maxsecret : 100);
         S_StartSound(NULL, sfx_barexp);
         sp_state++;
      }
   }
   else if(sp_state == 8)
   {
      if(!(intertime & 3))
         S_StartSound(NULL, sfx_pistol);
      
      cnt_time += 3;
      
      if(cnt_time >= plrs[me].stime / TICRATE)
         cnt_time = plrs[me].stime / TICRATE;
      
      cnt_par += 3;
      
      if(cnt_par >= wbs->partime / TICRATE)
      {
         cnt_par = wbs->partime / TICRATE;
         
         if (cnt_time >= plrs[me].stime / TICRATE)
         {
            S_StartSound(NULL, sfx_barexp);
            sp_state++;
         }
      }
      if(wbs->partime == -1) 
         cnt_par = 0;
   }
   else if(sp_state == 10)
   {
      if(acceleratestage)
      {
         S_StartSound(NULL, sfx_sgcock);
         
         if(gamemode == commercial)
            WI_initNoState();
         else
            WI_initShowNextLoc();
      }
   }
   else if(sp_state & 1)
   {
      if(!--cnt_pause)
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

   if(!(fade_applied || cur_pause_time))
   {
      if(wi_fade_color != -1)
         WI_OverlayBackground();
      fade_applied = true;
   }

   IN_slamBackground();
   
   // draw animated background
   WI_drawAnimatedBack();

   // haleyjd 02/02/05: intermission pause
   if(cur_pause_time > 0)
      return;
   
   WI_drawLF();

   V_DrawPatch(SP_STATSX, SP_STATSY, &vbscreen, kills);
   WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);
   
   V_DrawPatch(SP_STATSX, SP_STATSY+lh, &vbscreen, items);
   WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);
   
   V_DrawPatch(SP_STATSX, SP_STATSY+2*lh, &vbscreen, sp_secret);
   WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);
   
   V_DrawPatch(SP_TIMEX, SP_TIMEY, &vbscreen, time);
   WI_drawTime(SCREENWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);

   // Ty 04/11/98: redid logic: should skip only if with pwad but 
   // without deh patch
   // killough 2/22/98: skip drawing par times on pwads
   // Ty 03/17/98: unless pars changed with deh patch
   // sf: cleverer: only skips on _new_ non-iwad levels
   //   new logic in g_game.c

   if(wbs->partime != -1)
   {
      if(wbs->epsd < 3)
      {
         V_DrawPatch(SCREENWIDTH/2 + SP_TIMEX, SP_TIMEY, &vbscreen, par);
         WI_drawTime(SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
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
static void WI_Ticker(void)
{
   switch(state)
   {
   case StatCount:
      if(GameType == gt_dm) 
         WI_updateDeathmatchStats();
      else if(GameType == gt_coop) 
         WI_updateNetgameStats();
      else 
         WI_updateStats();
      break;
      
   case ShowNextLoc:
      WI_updateShowNextLoc();
      break;
      
   case NoState:
      WI_updateNoState();
      break;
   }
}

extern void V_ColorBlockTL(VBuffer *, byte, int, int, int, int, int);

//
// WI_OverlayBackground
//
// haleyjd 02/02/05: function to allow the background to be overlaid
// with a translucent color. Assumes WI_DrawBackground already called.
//
static void WI_OverlayBackground(void)
{
   V_ColorBlockTL(&backscreen1, (byte)wi_fade_color, 
                  0, 0, backscreen1.width, backscreen1.height, 
                  wi_tl_level);
}

// killough 11/98:
// Moved to separate function so that i_video.c could call it
// haleyjd: i_video now calls IN_DrawBackground, which calls the
//          appropriate gamemode's bg drawer.

static void WI_DrawBackground(void)
{
   char  name[9];  // limited to 8 characters
   
   if(gamemode == commercial || (gamemode == retail && wbs->epsd == 3))
      strcpy(name, LevelInfo.interPic);
   else 
      sprintf(name, "WIMAP%d", wbs->epsd);

   // background
   bg = W_CacheLumpName(name, PU_CACHE);    
   V_DrawPatch(0, 0, &backscreen1, bg);

   // re-fade if we were called due to video mode reset
   if(fade_applied && wi_fade_color != -1)
      WI_OverlayBackground();
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
   int   i, j;
   char name[9];

   WI_DrawBackground();         // killough 11/98

#if 0
   // UNUSED
   if(gamemode == commercial)
   {   // darken the background image
      unsigned char *pic = screens[1];
      for(; pic != screens[1] + SCREENHEIGHT*SCREENWIDTH; ++pic)
         *pic = colormaps[256*25 + *pic];
   }
#endif

   // killough 4/26/98: free lnames here (it was freed too early in Doom)
   Z_Free(lnames);

   if(gamemode == commercial)
   {
      NUMCMAPS = 32;
      
      lnames = (patch_t **)Z_Malloc(sizeof(patch_t*) * NUMCMAPS, PU_STATIC, 0);
      for(i = 0; i < NUMCMAPS; ++i)
      { 
         sprintf(name, "CWILV%2.2d", i);
         lnames[i] = W_CacheLumpName(name, PU_STATIC);
      }         
   }
   else
   {
      lnames = (patch_t **)Z_Malloc(sizeof(patch_t*) * NUMMAPS, PU_STATIC, 0);
      for(i = 0; i < NUMMAPS; ++i)
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
      
      if(wbs->epsd < 3)
      {
         for(j = 0; j < NUMANIMS[wbs->epsd]; ++j)
         {
            anim_t *a = &anims[wbs->epsd][j];
            for(i = 0; i < a->nanims; ++i)
            {
               // MONDO HACK!
               if(wbs->epsd != 1 || j != 8) 
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

   for(i = 0; i < 10; ++i)
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
   // haleyjd: removed old crap

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

   for(i = 0; i < MAXPLAYERS; ++i)
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
static void WI_Drawer(void)
{
   switch(state)
   {
   case StatCount:
      if(GameType == gt_dm)
         WI_drawDeathmatchStats();
      else if(GameType == gt_coop)
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
static void WI_initVariables(wbstartstruct_t *wbstartstruct)
{
   wbs = wbstartstruct;

   // haleyjd 02/02/05: pause and fade features
   cur_pause_time = wi_pause_time;
   fade_applied   = false;

   acceleratestage = 0;
   cnt = intertime = 0;
   firstrefresh = 1;
   me = wbs->pnum;
   plrs = wbs->plyr;

   if(!wbs->maxkills)
      wbs->maxkills = 1;  // probably only useful in MAP30
   
   if(!wbs->maxitems)
      wbs->maxitems = 1;

   // killough 2/22/98: Keep maxsecret=0 if it's zero, so
   // we can detect 0/0 as as a special case and print 100%.
   //
   //    if (!wbs->maxsecret)
   //  wbs->maxsecret = 1;

   if(gamemode != retail)
   {
      if(wbs->epsd > 2)
         wbs->epsd -= 3;
   }

   // haleyjd 03/27/05: EDF-defined intermission map names
   mapName = NULL;
   nextMapName = NULL;

   if(LevelInfo.useEDFInterName)
   {
      char nameBuffer[24];
      char *basename;

      // set current map
      psnprintf(nameBuffer, 24, "_IN_NAME_%s", gamemapname);
      mapName = E_StringForName(nameBuffer);

      // are we going to a secret level?
      basename = wbs->gotosecret ? LevelInfo.nextSecret : LevelInfo.nextLevel;

      // set next map
      if(*basename)
      {
         psnprintf(nameBuffer, 24, "_IN_NAME_%s", basename);

         nextMapName = E_StringForName(nameBuffer);
      }
      else
      {
         // try ExMy and MAPxy defaults for normally-named maps
         if(isExMy(gamemapname))
         {
            psnprintf(nameBuffer, 24, "_IN_NAME_E%01dM%01d", 
                      wbs->epsd + 1, wbs->next + 1);
            nextMapName = E_StringForName(nameBuffer);
         }
         else if(isMAPxy(gamemapname))
         {
            psnprintf(nameBuffer, 24, "_IN_NAME_MAP%02d", wbs->next + 1);
            nextMapName = E_StringForName(nameBuffer);
         }
      }
   }
}

// ====================================================================
// WI_Start
// Purpose: Call the various init routines
//          Note: wbstartstruct_t is defined in d_player.h
// Args:    wbstartstruct -- pointer to the structure with the 
//          intermission data
// Returns: void
//
static void WI_Start(wbstartstruct_t *wbstartstruct)
{
   WI_initVariables(wbstartstruct);
   WI_loadData();
   
   if(GameType == gt_dm)
      WI_initDeathmatchStats();
   else if(GameType == gt_coop)
      WI_initNetgameStats();
   else
      WI_initStats();
}

// haleyjd: DOOM intermission object

interfns_t DoomIntermission =
{
   WI_Ticker,
   WI_DrawBackground,
   WI_Drawer,
   WI_Start,
};

//----------------------------------------------------------------------------
//
// $Log: wi_stuff.c,v $
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
//----------------------------------------------------------------------------
