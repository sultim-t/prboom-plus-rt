// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
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
//  Heretic Intermission screens.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "w_wad.h"
#include "doomstat.h"
#include "g_game.h"
#include "in_lude.h"
#include "v_video.h"
#include "mn_htic.h"
#include "s_sound.h"
#include "d_gi.h"

extern char **mapnamesh[];

// Macros

#define HIS_NOWENTERING "NOW ENTERING:"
#define HIS_FINISHED    "FINISHED"
#define HIS_KILLS       "KILLS"
#define HIS_ITEMS       "ITEMS"
#define HIS_SECRETS     "SECRETS"
#define HIS_TIME        "TIME"


// Private Data

typedef struct hipoint_s
{
   int x;
   int y;
} hipoint_t;

static hipoint_t hipoints[3][9] =
{
   {
      { 172, 78 },
      { 86,  90 },
      { 73,  66 },
      { 159, 95 },
      { 148, 126 },
      { 132, 54 },
      { 131, 74 },
      { 208, 138 },
      { 52,  101 }
   },
   {
      { 218, 57 },
      { 137, 81 },
      { 155, 124 },
      { 171, 68 },
      { 250, 86 },
      { 136, 98 },
      { 203, 90 },
      { 220, 140 },
      { 279, 106 }
   },
   {
      { 86,  99 },
      { 124, 103 },
      { 154, 79 },
      { 202, 83 },
      { 178, 59 },
      { 142, 58 },
      { 219, 66 },
      { 247, 57 },
      { 107, 80 }
   }
};

typedef enum
{
   INTR_NONE = -1,
   INTR_STATS,
   INTR_LEAVING,
   INTR_GOING,
   INTR_WAITING,
} interstate_e;

static interstate_e interstate;

static int statetime;             // time until next state
static int countdown;             // count down to end
static boolean flashtime = false;

static wbstartstruct_t hi_wbs;

// graphic patches
static patch_t *hi_interpic;
static patch_t *hi_in_x;
static patch_t *hi_in_yah;

static int hi_faces[4];
static int hi_dead_faces[4];

// Private functions

void HI_DrawBackground(void);

static void HI_loadData(void)
{
   int i;
   char mapname[9];

   memset(mapname, 0, 9);

   // load interpic
   hi_interpic = NULL;

   if(gameepisode <= 3)
   {
      sprintf(mapname, "MAPE%d", gameepisode);
      hi_interpic = W_CacheLumpName(mapname, PU_STATIC);
   }

   // load positional indicators
   hi_in_x   = W_CacheLumpName("IN_X", PU_STATIC);
   hi_in_yah = W_CacheLumpName("IN_YAH", PU_STATIC);

   // get lump numbers for faces
   for(i = 0; i < 4; i++)
   {
      char tempstr[9];

      memset(tempstr, 0, 9);

      sprintf(tempstr, "FACEA%.1d", i);
      hi_faces[i] = W_GetNumForName(tempstr);

      sprintf(tempstr, "FACEB%.1d", i);
      hi_dead_faces[i] = W_GetNumForName(tempstr);
   }

   // draw the background to the back buffer
   HI_DrawBackground();
}

static void HI_Stop(void)
{
   if(hi_interpic)
      Z_ChangeTag(hi_interpic, PU_CACHE);

   Z_ChangeTag(hi_in_x, PU_CACHE);
   Z_ChangeTag(hi_in_yah, PU_CACHE);
}

// Drawing functions

//
// HI_drawNewLevelName
//
// Draws "NOW ENTERING:" and the destination level name centered,
// starting at the given y coordinate.
//
static void HI_drawNewLevelName(int y)
{
   int x;
   char *thisLevelName;

   x = (SCREENWIDTH - V_StringWidth(HIS_NOWENTERING)) >> 1;
   V_WriteText(HIS_NOWENTERING, x, y);

   thisLevelName = *mapnamesh[hi_wbs.epsd * 9 + hi_wbs.next];

   // be safe about incrementing past the "ExMx:  "
   if(strlen(thisLevelName) > 7)
      thisLevelName += 7;

   x = (SCREENWIDTH - MN_HBStringWidth(thisLevelName)) >> 1;
   MN_HBWriteText(thisLevelName, x, y + 10);
}

//
// HI_drawOldLevelName
//
// Draws previous level name and "FINISHED" centered,
// starting at the given y coordinate.
//
static void HI_drawOldLevelName(int y)
{
   int x;
   char *oldLevelName;

   oldLevelName = *mapnamesh[hi_wbs.epsd * 9 + hi_wbs.last];

   // be safe about incrementing past the "ExMx:  "
   if(strlen(oldLevelName) > 7)
      oldLevelName += 7;

   x = (SCREENWIDTH - MN_HBStringWidth(oldLevelName)) >> 1;
   MN_HBWriteText(oldLevelName, x, y);

   x = (SCREENWIDTH - V_StringWidth(HIS_FINISHED)) >> 1;
   V_WriteText(HIS_FINISHED, x, y + 22);
}

//
// HI_drawGoing
//
// Drawer function for the final intermission stage where the
// "now entering" is shown, and a pointer blinks at the next
// stage on the map.
//
static void HI_drawGoing(void)
{
   int i, previous;

   if(gameepisode > 3)
      return;
 
   HI_drawNewLevelName(10);
   
   previous = hi_wbs.last;

   // handle secret level
   if(previous == 8)
   {
      previous = hi_wbs.next - 1;
   }
   
   // draw patches on levels visited
   for(i = 0; i <= previous; i++)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][i].x, 
                  hipoints[hi_wbs.epsd][i].y,
                  &vbscreen,
                  hi_in_x);
   }

   // draw patch on secret level
   if(hi_wbs.didsecret)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][8].x, 
                  hipoints[hi_wbs.epsd][8].y,
                  &vbscreen,
                  hi_in_x);
   }

   // blink destination arrow
   if(flashtime)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][hi_wbs.next].x,
                  hipoints[hi_wbs.epsd][hi_wbs.next].y, 
                  &vbscreen,
                  hi_in_yah);
   }
}

//
// HI_drawLeaving
//
// Drawer function for first map stage, where "leaving..." is
// shown and the previous levels are all X'd out
//
static void HI_drawLeaving(void)
{
   int i;
   int lastlevel, thislevel;
   boolean drawsecret;

   if(gameepisode > 3)
      return;

   HI_drawOldLevelName(3);

   if(hi_wbs.last == 8)
   {
      // leaving secret level, handle specially
      lastlevel = hi_wbs.next;
      thislevel = 8;
      drawsecret = false;
   }
   else
   {
      lastlevel = thislevel = hi_wbs.last;
      drawsecret = hi_wbs.didsecret;
   }

   // draw all the previous levels
   for(i = 0; i < lastlevel; i++)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][i].x,
                  hipoints[hi_wbs.epsd][i].y,
                  &vbscreen,
                  hi_in_x);
   }

   // draw the secret level if appropriate
   if(drawsecret)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][8].x, 
                  hipoints[hi_wbs.epsd][8].y,
                  &vbscreen,
                  hi_in_x);
   }

   // blink the level we're leaving
   if(flashtime)
   {
      V_DrawPatch(hipoints[hi_wbs.epsd][thislevel].x, 
                  hipoints[hi_wbs.epsd][thislevel].y,
                  &vbscreen,
                  hi_in_x);
   }   
}

//
// HI_drawLevelStat
//
// Draws a pair of "Font B" numbers separated by a slash
//
static void HI_drawLevelStat(int stat, int max, int x, int y)
{
   char str[16];

   sprintf(str, "%3d", stat);
   MN_HBWriteNumText(str, x, y);
   
   MN_HBWriteText("/", x + 37, y);

   sprintf(str, "%3d", max);
   MN_HBWriteNumText(str, x + 48, y);
}

//
// HI_drawTime
//
// Draws a "Font B" HH:MM:SS time indication. Nothing is
// shown for hours if h is zero, but minutes and seconds
// are always shown no matter what. It looks best this way.
//
static void HI_drawTime(int h, int m, int s, int x, int y)
{
   char timestr[16];

   if(h)
   {
      sprintf(timestr, "%02d", h);
      MN_HBWriteNumText(timestr, x, y);
      MN_HBWriteText(":", x + 26, y);
   }

   sprintf(timestr, "%02d", m);
   MN_HBWriteNumText(timestr, x + 34, y);
   MN_HBWriteText(":", x + 60, y);

   sprintf(timestr, "%02d", s);
   MN_HBWriteNumText(timestr, x + 68, y);
}

//
// HI_drawSingleStats
//
// Drawer function for single-player mode stats stage.
// Shows kills, items, secrets, and either time or the next
// level name (depending on whether we're in a SOSR episode
// or not). Note Heretic has no par times.
//
static void HI_drawSingleStats(void)
{
   static int statstage = 0;

   MN_HBWriteText(HIS_KILLS,   50,  65);
   MN_HBWriteText(HIS_ITEMS,   50,  90);
   MN_HBWriteText(HIS_SECRETS, 50, 115);

   HI_drawOldLevelName(3);

   // prior to tic 30: draw nothing
   if(intertime < 30)
   {
      statstage = 0;
      return;
   }

   // tics 30 to 60: add kill count
   if(intertime > 30)
   {
      if(statstage == 0)
      {
         S_StartSound(NULL, sfx_hdorcls);
         statstage = 1;
      }

      HI_drawLevelStat(players[consoleplayer].killcount,
                       hi_wbs.maxkills, 200, 65);
   }
   
   // tics 60 to 90: add item count
   if(intertime > 60)
   {
      if(statstage == 1)
      {
         S_StartSound(NULL, sfx_hdorcls);
         statstage = 2;
      }

      HI_drawLevelStat(players[consoleplayer].itemcount,
                       hi_wbs.maxitems, 200, 90);
   }
   
   // tics 90 to 150: add secret count
   if(intertime > 90)
   {
      if(statstage == 2)
      {
         S_StartSound(NULL, sfx_hdorcls);
         statstage = 3;
      }

      HI_drawLevelStat(players[consoleplayer].secretcount,
                       hi_wbs.maxsecret, 200, 115);
   }

   // 150 to end: show time or next level name
   // Note that hitting space earlier than 150 sets the ticker to
   // 150 so that we jump here.

   if(intertime > 150)
   {
      if(statstage == 3)
      {
         S_StartSound(NULL, sfx_hdorcls);
         statstage = 4;
      }

      if(gameepisode < 4)
      {         
         int time, hours, minutes, seconds;
         
         time = hi_wbs.plyr[consoleplayer].stime / TICRATE;
         
         hours = time / 3600;
         time -= hours * 3600;
         
         minutes = time / 60;
         time -= minutes * 60;
         
         seconds = time;

         MN_HBWriteText(HIS_TIME, 85, 160);
         HI_drawTime(hours, minutes, seconds, 155, 160);
      }
      else
      {
         HI_drawNewLevelName(160);
         acceleratestage = false;
      }
   }
}

// Public functions

void HI_Ticker(void)
{
   if(interstate == INTR_WAITING)
   {
      countdown--;
      
      if(!countdown)
      {
         HI_Stop();
         G_WorldDone();
      }

      return;
   }

   if(statetime < intertime)
   {
      interstate++;
      
      if(gameepisode > 3 && interstate > INTR_STATS)
      {
         // extended episodes have no map screens
         interstate = INTR_WAITING;
      }

      switch(interstate)
      {
      case INTR_STATS:
         statetime = intertime + ((gameepisode > 3) ? 1200 : 300);
         break;
      case INTR_LEAVING:
         statetime = intertime + 200;
         HI_DrawBackground();             // change the background
         break;
      case INTR_GOING:
         statetime = D_MAXINT;
         break;
      case INTR_WAITING:
         countdown = 10;
         break;
      default:
         break;
      }
   }

   // update flashing graphics state
   flashtime = !(intertime & 16) || (interstate == INTR_WAITING);

   // see if we should skip ahead
   if(acceleratestage)
   {
      if(interstate == INTR_STATS && intertime < 150)
      {
         intertime = 150;
      }
      else if(interstate < INTR_GOING && gameepisode < 4)
      {
         interstate = INTR_GOING;
         HI_DrawBackground(); // force a background change
         S_StartSound(NULL, sfx_hdorcls);
      }
      else
      {
         interstate = INTR_WAITING;
         countdown = 10;
         flashtime = true;

         S_StartSound(NULL, sfx_hdorcls);
      }

      // clear accelerate flag
      acceleratestage = 0;
   }
}

//
// HI_DrawBackground
//
// Called when the background needs to be changed.
// IN_slamBackground is called to swap this to the
// screen, saving some time.
//
void HI_DrawBackground(void)
{
   if(interstate > INTR_STATS && hi_interpic)
   {
      V_DrawPatch(0, 0, &backscreen1, hi_interpic);
   }
   else
   {
      // TODO: externalize flat name
      V_DrawBackground("FLOOR16", &backscreen1);
   }
}

void HI_Drawer(void)
{
   static interstate_e oldinterstate;

   if(interstate == INTR_WAITING)
   {
      return;
   }
   
   if(oldinterstate != INTR_GOING && interstate == INTR_GOING)
   {
      //S_StartSound(NULL, sfx_pstop);
   }

   // swap the background onto the screen
   IN_slamBackground();

   switch(interstate)
   {
   case INTR_STATS:
      switch(GameType)
      {
      case gt_single:
         HI_drawSingleStats();
         break;
      case gt_coop:
         // IN_DrawCoopStats();
         break;
      default:
         // IN_DrawDMStats();
         break;
      }
      break;

   case INTR_LEAVING:
      HI_drawLeaving();
      break;
   case INTR_GOING:
      HI_drawGoing();
      break;
   default:
      break;
   }

   oldinterstate = interstate;
}

void HI_Start(wbstartstruct_t *wbstartstruct)
{
   // hidden levels will have no intermission
   if(gameepisode >= gameModeInfo->numEpisodes)
   {
      G_WorldDone();
      return;
   }

   acceleratestage = 0;
   statetime = -1;
   intertime = 0;
   interstate = INTR_NONE;

   hi_wbs = *wbstartstruct;

   HI_loadData();
}

// EOF




