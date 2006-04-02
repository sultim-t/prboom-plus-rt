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
//   the automap code
//
//-----------------------------------------------------------------------------

static const char rcsid[] =
  "$Id: am_map.c,v 1.24 1998/05/10 12:05:24 jim Exp $";

#include "doomstat.h"
#include "d_main.h"
#include "st_stuff.h"
#include "r_main.h"
#include "p_setup.h"
#include "p_maputl.h"
#include "r_draw.h"
#include "w_wad.h"
#include "v_video.h"
#include "p_spec.h"
#include "am_map.h"
#include "dstrings.h"
#include "d_deh.h"    // Ty 03/27/98 - externalizations
#include "d_gi.h"
#include "g_bind.h"

//jff 1/7/98 default automap colors added
int mapcolor_back;    // map background
int mapcolor_grid;    // grid lines color
int mapcolor_wall;    // normal 1s wall color
int mapcolor_fchg;    // line at floor height change color
int mapcolor_cchg;    // line at ceiling height change color
int mapcolor_clsd;    // line at sector with floor=ceiling color
int mapcolor_rkey;    // red key color
int mapcolor_bkey;    // blue key color
int mapcolor_ykey;    // yellow key color
int mapcolor_rdor;    // red door color  (diff from keys to allow option)
int mapcolor_bdor;    // blue door color (of enabling one but not other )
int mapcolor_ydor;    // yellow door color
int mapcolor_tele;    // teleporter line color
int mapcolor_secr;    // secret sector boundary color
int mapcolor_exit;    // jff 4/23/98 add exit line color
int mapcolor_unsn;    // computer map unseen line color
int mapcolor_flat;    // line with no floor/ceiling changes
int mapcolor_sprt;    // general sprite color
int mapcolor_hair;    // crosshair color
int mapcolor_sngl;    // single player arrow color
int mapcolor_plyr[4]; // colors for player arrows in multiplayer
int mapcolor_frnd;    // colors for friends of player

//jff 3/9/98 add option to not show secret sectors until entered
int map_secret_after;
//jff 4/3/98 add symbols for "no-color" for disable and "black color" for black
#define NC 0
#define BC 247

// drawing stuff
#define FB    0

// haleyjd 07/07/04: removed key_map* variables

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC  4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (f_x + MTOF((x)-m_x))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-m_y)))

typedef struct
{
   int x, y;
} fpoint_t;

typedef struct
{
   fpoint_t a, b;
} fline_t;

typedef struct
{
   mpoint_t a, b;
} mline_t;

typedef struct
{
   fixed_t slp, islp;
} islope_t;

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] =
{
  { { -R+R/8, 0 }, { R, 0 } }, // -----
  { { R, 0 }, { R-R/2, R/4 } },  // ----->
  { { R, 0 }, { R-R/2, -R/4 } },
  { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
  { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
  { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
  { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] =
{ // killough 3/22/98: He's alive, Jim :)
  { { -R+R/8, 0 }, { R, 0 } }, // -----
  { { R, 0 }, { R-R/2, R/4 } },  // ----->
  { { R, 0 }, { R-R/2, -R/4 } },
  { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
  { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
  { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
  { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } },
  { { -R/10-R/6, R/4}, {-R/10-R/6, -R/4} },  // J
  { { -R/10-R/6, -R/4}, {-R/10-R/6-R/8, -R/4} },
  { { -R/10-R/6-R/8, -R/4}, {-R/10-R/6-R/8, -R/8} },
  { { -R/10, R/4}, {-R/10, -R/4}},           // F
  { { -R/10, R/4}, {-R/10+R/8, R/4}},
  { { -R/10+R/4, R/4}, {-R/10+R/4, -R/4}},   // F
  { { -R/10+R/4, R/4}, {-R/10+R/4+R/8, R/4}},
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

#define R (FRACUNIT)
// haleyjd 03/17/02: fixed to clean up type coercion warnings
#define TG_MINUS867 ((fixed_t)(-.867 * R))
#define TG_MINUSP5  ((fixed_t)(-.5 * R))
#define TG_P867     ((fixed_t)(.867 * R))

mline_t triangle_guy[] =
{
  { { TG_MINUS867, TG_MINUSP5 }, { TG_P867, TG_MINUSP5 } },
  { { TG_P867, TG_MINUSP5 } , { 0, R } },
  { { 0, R }, { TG_MINUS867, TG_MINUSP5 } }
};
#undef R
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))

//jff 1/5/98 new symbol for keys on automap
#define R (FRACUNIT)
mline_t cross_mark[] =
{
  { { -R, 0 }, { R, 0} },
  { { 0, -R }, { 0, R } },
};
#undef R
#define NUMCROSSMARKLINES (sizeof(cross_mark)/sizeof(mline_t))
//jff 1/5/98 end of new symbol

#define R (FRACUNIT)
#define TTG_MINUSP5 ((fixed_t)(-.5 * R))
#define TTG_MINUSP7 ((fixed_t)(-.7 * R))
#define TTG_P7      ((fixed_t)(.7 * R))
mline_t thintriangle_guy[] =
{
  { { TTG_MINUSP5, TTG_MINUSP7 }, { R, 0 } },
  { { R, 0 }, { TTG_MINUSP5, TTG_P7 } },
  { { TTG_MINUSP5, TTG_P7 }, { TTG_MINUSP5, TTG_MINUSP7 } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))

int ddt_cheating = 0;         // killough 2/7/98: make global, rename to ddt_*

int automap_grid = 0;

boolean automapactive = false;

// location of window on screen
static int  f_x;
static int  f_y;

// size of window on screen
static int  f_w;
static int  f_h;

static int  lightlev;        // used for funky strobing effect
static byte*  fb;            // pseudo-frame buffer
static int  amclock;

static mpoint_t m_paninc;    // how far the window pans each tic (map coords)
static fixed_t mtof_zoommul; // how far the window zooms each tic (map coords)
static fixed_t ftom_zoommul; // how far the window zooms each tic (fb coords)

static fixed_t m_x, m_y;     // LL x,y window location on the map (map coords)
static fixed_t m_x2, m_y2;   // UR x,y window location on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t  m_w;
static fixed_t  m_h;

// based on level size
static fixed_t  min_x;
static fixed_t  min_y; 
static fixed_t  max_x;
static fixed_t  max_y;

static fixed_t  max_w;          // max_x-min_x,
static fixed_t  max_h;          // max_y-min_y

// based on player size
static fixed_t  min_w;
static fixed_t  min_h;


static fixed_t  min_scale_mtof; // used to tell when to stop zooming out
static fixed_t  max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t)INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t *plr;           // the player represented by an arrow

static patch_t *marknums[10];   // numbers used for marking by the automap

// killough 2/22/98: Remove limit on automap marks,
// and make variables external for use in savegames.

mpoint_t *markpoints = NULL;    // where the points are
int markpointnum = 0; // next point to be assigned (also number of points now)
int markpointnum_max = 0;       // killough 2/22/98
int followplayer = 1; // specifies whether to follow the player around

static boolean stopped = true;

// haleyjd 12/22/02: Heretic stuff

// backdrop
static byte *am_backdrop = NULL;
static boolean am_usebackdrop = false;

//
// AM_getIslope()
//
// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.
//
// Passed the line slope is desired for and an islope_t structure for return
// Returns nothing
//
void AM_getIslope(mline_t *ml, islope_t *is )
{
   int dx, dy;
   
   dy = ml->a.y - ml->b.y;
   dx = ml->b.x - ml->a.x;
   if(!dy)
      is->islp = (dx < 0 ? -D_MAXINT : D_MAXINT);
   else
      is->islp = FixedDiv(dx, dy);
   if(!dx)
      is->slp = (dy < 0 ? -D_MAXINT : D_MAXINT);
   else
      is->slp = FixedDiv(dy, dx);
}

//
// AM_activateNewScale()
//
// Changes the map scale after zooming or translating
//
// Passed nothing, returns nothing
//
void AM_activateNewScale(void)
{
   m_x += m_w/2;
   m_y += m_h/2;
   m_w = FTOM(f_w);
   m_h = FTOM(f_h);
   m_x -= m_w/2;
   m_y -= m_h/2;
   m_x2 = m_x + m_w;
   m_y2 = m_y + m_h;
}

//
// AM_saveScaleAndLoc()
//
// Saves the current center and zoom
// Affects the variables that remember old scale and loc
//
// Passed nothing, returns nothing
//
void AM_saveScaleAndLoc(void)
{
   old_m_x = m_x;
   old_m_y = m_y;
   old_m_w = m_w;
   old_m_h = m_h;
}

//
// AM_restoreScaleAndLoc()
//
// restores the center and zoom from locally saved values
// Affects global variables for location and scale
//
// Passed nothing, returns nothing
//
void AM_restoreScaleAndLoc(void)
{
   m_w = old_m_w;
   m_h = old_m_h;
   if(!followplayer)
   {
      m_x = old_m_x;
      m_y = old_m_y;
   }
   else
   {
      m_x = plr->mo->x - m_w/2;
      m_y = plr->mo->y - m_h/2;
   }
   m_x2 = m_x + m_w;
   m_y2 = m_y + m_h;
   
   // Change the scaling multipliers
   scale_mtof = FixedDiv(f_w << FRACBITS, m_w);
   scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// AM_addMark()
//
// Adds a marker at the current location
// Affects global variables for marked points
//
// Passed nothing, returns nothing
//
void AM_addMark(void)
{
   // killough 2/22/98:
   // remove limit on automap marks
   
   if(markpointnum >= markpointnum_max)
      markpoints = realloc(markpoints,
                           (markpointnum_max = markpointnum_max ? 
                            markpointnum_max*2 : 16) * sizeof(*markpoints));

   markpoints[markpointnum].x = m_x + m_w/2;
   markpoints[markpointnum].y = m_y + m_h/2;
   markpointnum++;
}

//
// AM_findMinMaxBoundaries()
//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
// Passed nothing, returns nothing
//
void AM_findMinMaxBoundaries(void)
{
   int i;
   fixed_t a;
   fixed_t b;
   
   min_x = min_y =  D_MAXINT;
   max_x = max_y = -D_MAXINT;
   
   for(i = 0; i < numvertexes; ++i)
   {
      if (vertexes[i].x < min_x)
         min_x = vertexes[i].x;
      else if (vertexes[i].x > max_x)
         max_x = vertexes[i].x;
      
      if (vertexes[i].y < min_y)
         min_y = vertexes[i].y;
      else if (vertexes[i].y > max_y)
         max_y = vertexes[i].y;
   }
   
   max_w = max_x - min_x;
   max_h = max_y - min_y;
   
   min_w = 2*PLAYERRADIUS; // const? never changed?
   min_h = 2*PLAYERRADIUS;
   
   a = FixedDiv(f_w << FRACBITS, max_w);
   b = FixedDiv(f_h << FRACBITS, max_h);
   
   min_scale_mtof = a < b ? a : b;
   max_scale_mtof = FixedDiv(f_h << FRACBITS, 2*PLAYERRADIUS);
}

//
// AM_changeWindowLoc()
//
// Moves the map window by the global variables m_paninc.x, m_paninc.y
//
// Passed nothing, returns nothing
//
void AM_changeWindowLoc(void)
{
   if(m_paninc.x || m_paninc.y)
   {
      followplayer = 0;
      f_oldloc.x = D_MAXINT;
   }
   
   m_x += m_paninc.x;
   m_y += m_paninc.y;
   
   if(m_x + m_w/2 > max_x)
      m_x = max_x - m_w/2;
   else if(m_x + m_w/2 < min_x)
      m_x = min_x - m_w/2;
   
   if(m_y + m_h/2 > max_y)
      m_y = max_y - m_h/2;
   else if(m_y + m_h/2 < min_y)
      m_y = min_y - m_h/2;
   
   m_x2 = m_x + m_w;
   m_y2 = m_y + m_h;
}

extern void ST_AutomapEvent(int type);

//
// AM_initVariables()
//
// Initialize the variables for the automap
//
// Affects the automap global variables
// Status bar is notified that the automap has been entered
// Passed nothing, returns nothing
//
void AM_initVariables(void)
{
   int pnum;   
   
   automapactive = true;
   fb = screens[0];
   
   f_oldloc.x = D_MAXINT;
   amclock = 0;
   lightlev = 0;
   
   m_paninc.x = m_paninc.y = 0;
   ftom_zoommul = FRACUNIT;
   mtof_zoommul = FRACUNIT;
   
   m_w = FTOM(f_w);
   m_h = FTOM(f_h);
   
   // find player to center on initially
   if(!playeringame[pnum = consoleplayer])
   {
      for(pnum = 0; pnum < MAXPLAYERS; ++pnum)
         if(playeringame[pnum])
            break;
   }
         
   plr = &players[pnum];
   m_x = plr->mo->x - m_w/2;
   m_y = plr->mo->y - m_h/2;
   AM_changeWindowLoc();
         
   // for saving & restoring
   old_m_x = m_x;
   old_m_y = m_y;
   old_m_w = m_w;
   old_m_h = m_h;
   
   // inform the status bar of the change
   ST_AutomapEvent(AM_MSGENTERED);
}

//
// AM_loadPics()
// 
// Load the patches for the mark numbers
//
// Sets the marknums[i] variables to the patches for each digit
// Passed nothing, returns nothing;
//
void AM_loadPics(void)
{
   int i, lumpnum;
   char namebuf[9];
   
   for(i = 0; i < 10; ++i)
   {
      sprintf(namebuf, "AMMNUM%d", i);
      marknums[i] = W_CacheLumpName(namebuf, PU_STATIC);
   }

   // haleyjd 12/22/02: automap background support (raw format)
   if((lumpnum = W_CheckNumForName("AUTOPAGE")) != -1)
   {
      int size = W_LumpLength(lumpnum);
      byte *autopage = W_CacheLumpNum(lumpnum, PU_STATIC);
      int height = size / SCREENWIDTH;

      // allocate backdrop
      if(!am_backdrop)
         am_backdrop = Z_Malloc(SCREENWIDTH*SCREENHEIGHT, PU_STATIC, NULL);

      // must be at least 100 tall
      if(height < 100 || height > SCREENHEIGHT)
         I_Error("AM_loadPics: bad AUTOPAGE size\n");

      // use V_CacheBlock to construct an unscaled screen buffer
      V_CacheBlock(0, 0, SCREENWIDTH, height, autopage, am_backdrop);

      // background is allowed to be shorter than 200 pixels, so
      // tile the graphic through the difference
      if(height < SCREENHEIGHT)
      {
         V_CacheBlock(0, height, SCREENWIDTH, SCREENHEIGHT-height,
                      autopage, am_backdrop);
      }

      // set lump purgable
      Z_ChangeTag(autopage, PU_CACHE);

      am_usebackdrop = true;
   }
}

//
// AM_unloadPics()
//
// Makes the mark patches purgable
//
// Passed nothing, returns nothing
//
void AM_unloadPics(void)
{
   int i;
   
   for(i = 0; i < 10; ++i)
      Z_ChangeTag(marknums[i], PU_CACHE);
   
   // haleyjd 12/22/02: backdrop support
   if(am_backdrop)
   {
      Z_Free(am_backdrop);
      am_backdrop = NULL;
      am_usebackdrop = false;
   }
}

//
// AM_clearMarks()
//
// Sets the number of marks to 0, thereby clearing them from the display
//
// Affects the global variable markpointnum
// Passed nothing, returns nothing
//
void AM_clearMarks(void)
{
   markpointnum = 0;
}

//
// AM_LevelInit()
//
// Initialize the automap at the start of a new level
// should be called at the start of every level
//
// Passed nothing, returns nothing
// Affects automap's global variables
//
void AM_LevelInit(void)
{
   f_x = f_y = 0;
   
   // killough 2/7/98: get rid of finit_ vars
   // to allow runtime setting of width/height
   //
   
   // SoM 2-4-04: ANYRES
   f_w = v_width;
   f_h = v_height - ((gameModeInfo->StatusBar->height * globalyscale) >> FRACBITS);

   AM_findMinMaxBoundaries();
   scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7*FRACUNIT));
   if(scale_mtof > max_scale_mtof)
      scale_mtof = min_scale_mtof;
   scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// AM_Stop()
//
// Cease automap operations, unload patches, notify status bar
//
// Passed nothing, returns nothing
//
void AM_Stop(void)
{  
   AM_unloadPics();
   automapactive = false;
   ST_AutomapEvent(AM_MSGEXITED);
   stopped = true;
   redrawsbar = redrawborder = true;  // sf: need redraw
}

//
// AM_Start()
// 
// Start up automap operations, 
//  if a new level, or game start, (re)initialize level variables
//  init map variables
//  load mark patches
//
// Passed nothing, returns nothing
//
void AM_Start(void)
{
   static int lastlevel = -1, lastepisode = -1, 
              last_width = -1, last_height = -1;
   
   if(!stopped)
      AM_Stop();
   redrawsbar = redrawborder = true;  // sf: redraw needed
   stopped = false;
   
   // SoM: ANYRES
   if(lastlevel != gamemap || lastepisode != gameepisode || 
      last_width != v_width || last_height != v_height)
   {
      last_width = v_width;
      last_height = v_height;
      AM_LevelInit();
      lastlevel = gamemap;
      lastepisode = gameepisode;
   }
   AM_initVariables();
   AM_loadPics();
}

//
// AM_minOutWindowScale()
//
// Set the window scale to the maximum size
//
// Passed nothing, returns nothing
//
void AM_minOutWindowScale()
{
   scale_mtof = min_scale_mtof;
   scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
   AM_activateNewScale();
}

//
// AM_maxOutWindowScale(void)
//
// Set the window scale to the minimum size
//
// Passed nothing, returns nothing
//
void AM_maxOutWindowScale(void)
{
   scale_mtof = max_scale_mtof;
   scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
   AM_activateNewScale();
}

static boolean am_key_handled;

//
// AM_Responder()
//
// Handle events (user inputs) in automap mode
//
// Passed an input event, returns true if its handled
//
// haleyjd 07/07/04: rewritten to support new keybindings
//
boolean AM_Responder(event_t *ev)
{
   static int cheatstate=0;
   static int bigstate=0;
     
   // haleyjd 07/07/04: dynamic bindings
   am_key_handled = false;
   G_KeyResponder(ev, kac_map);

   if(ev->type == ev_keydown && !am_key_handled)
   {
      if(!automapactive)
      {
         if(action_map_toggle)
         {
            AM_Start();
            am_key_handled = true;
            action_map_toggle = 0;
         }
      }
      else
      {
         am_key_handled = true;

         if(action_map_toggle)
         {
            action_map_toggle = bigstate = 0;
            AM_Stop();
         }
         else if(action_map_gobig)
         {
            bigstate = !bigstate;
            if(bigstate)
            {
               AM_saveScaleAndLoc();
               AM_minOutWindowScale();
            }
            else
               AM_restoreScaleAndLoc();
            action_map_gobig = 0;
         }
         else if(action_map_follow)
         {
            followplayer = !followplayer;
            f_oldloc.x = D_MAXINT;
            // Ty 03/27/98 - externalized
            doom_printf(followplayer ? s_AMSTR_FOLLOWON : s_AMSTR_FOLLOWOFF);
            action_map_follow = 0;
         }
         else if(action_map_grid)
         {
            automap_grid = !automap_grid;      // killough 2/28/98
            // Ty 03/27/98 - *not* externalized
            doom_printf(automap_grid ? s_AMSTR_GRIDON : s_AMSTR_GRIDOFF);
            action_map_grid = 0;
         }
         else if(action_map_mark)
         {
            // Ty 03/27/98 - *not* externalized     
            // sf: fixed this (buffer at start, presumably from an old sprintf
            doom_printf("%s %d", s_AMSTR_MARKEDSPOT, markpointnum);
            AM_addMark();
            action_map_mark = 0;
         }
         else if(action_map_clear)
         {
            AM_clearMarks();  // Ty 03/27/98 - *not* externalized
            doom_printf(s_AMSTR_MARKSCLEARED);
            action_map_clear = 0;
         }
         else
         {
            cheatstate = 0;
            am_key_handled = false;
         }
      }
   }

   return am_key_handled;
}

//
// action_handler_right
//
// Registered as the handler for the "map_right" key binding.
//
static void action_handler_right(event_t *ev)
{
   if(automapactive && !followplayer)
   {
      if(ev->type == ev_keydown)
      {
         m_paninc.x = FTOM(F_PANINC);
         am_key_handled = true;
      }
      else
         m_paninc.x = 0;
   }
}

//
// action_handler_left
//
// Registered as the handler for the "map_left" key binding.
//
static void action_handler_left(event_t *ev)
{
   if(automapactive && !followplayer)
   {
      if(ev->type == ev_keydown)
      {
         m_paninc.x = -FTOM(F_PANINC);
         am_key_handled = true;
      }
      else
         m_paninc.x = 0;
   }
}

//
// action_handler_up
//
// Registered as the handler for the "map_up" key binding.
//
static void action_handler_up(event_t *ev)
{
   if(automapactive && !followplayer)
   {
      if(ev->type == ev_keydown)
      {
         m_paninc.y = FTOM(F_PANINC);
         am_key_handled = true;
      }
      else
         m_paninc.y = 0;
   }
}

//
// action_handler_down
//
// Registered as the handler for the "map_down" key binding.
//
static void action_handler_down(event_t *ev)
{
   if(automapactive && !followplayer)
   {
      if(ev->type == ev_keydown)
      {
         m_paninc.y = -FTOM(F_PANINC);
         am_key_handled = true;
      }
      else
         m_paninc.y = 0;
   }
}

//
// action_handler_zoomout
//
// Registered as the handler for the "map_zoomout" key binding.
//
static void action_handler_zoomout(event_t *ev)
{
   if(automapactive)
   {
      if(ev->type == ev_keydown)
      {
         mtof_zoommul = M_ZOOMOUT;
         ftom_zoommul = M_ZOOMIN;
         am_key_handled = true;
      }
      else
      {
         mtof_zoommul = FRACUNIT;
         ftom_zoommul = FRACUNIT;
      }
   }
}

//
// action_handler_zoomin
//
// Registered as the handler for the "map_zoomin" key binding.
//
static void action_handler_zoomin(event_t *ev)
{
   if(automapactive)
   {
      if(ev->type == ev_keydown)
      {
         mtof_zoommul = M_ZOOMIN;
         ftom_zoommul = M_ZOOMOUT;
         am_key_handled = true;
      }
      else
      {
         mtof_zoommul = FRACUNIT;
         ftom_zoommul = FRACUNIT;
      }
   }
}

//
// AM_Handlers
//
// Array of the above keybinding handler functions
//
binding_handler AM_Handlers[] =
{
   action_handler_right,
   action_handler_left,
   action_handler_up,
   action_handler_down,
   action_handler_zoomin,
   action_handler_zoomout,
};

//
// AM_changeWindowScale()
//
// Automap zooming
//
// Passed nothing, returns nothing
//
void AM_changeWindowScale(void)
{
   // Change the scaling multipliers
   scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
   scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
   
   if(scale_mtof < min_scale_mtof)
      AM_minOutWindowScale();
   else if(scale_mtof > max_scale_mtof)
      AM_maxOutWindowScale();
   else
      AM_activateNewScale();
}

//
// AM_doFollowPlayer()
//
// Turn on follow mode - the map scrolls opposite to player motion
//
// Passed nothing, returns nothing
//
void AM_doFollowPlayer(void)
{
   if(f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
   {
      m_x = FTOM(MTOF(plr->mo->x)) - m_w/2;
      m_y = FTOM(MTOF(plr->mo->y)) - m_h/2;
      m_x2 = m_x + m_w;
      m_y2 = m_y + m_h;
      f_oldloc.x = plr->mo->x;
      f_oldloc.y = plr->mo->y;
   }
}

//
// killough 10/98: return coordinates, to allow use of a non-follow-mode
// pointer. Allows map inspection without moving player to the location.
//

// haleyjd 07/01/02: reformatted for readability

int map_point_coordinates;

void AM_Coordinates(const mobj_t *mo, fixed_t *x, fixed_t *y, fixed_t *z)
{
   if(followplayer || !map_point_coordinates)
   {
      *x = mo->x; *y = mo->y; *z = mo->z;
   }
   else
   {
      *x = m_x + m_w / 2;
      *y = m_y + m_h / 2;
      *z = R_PointInSubsector(*x, *y)->sector->floorheight;
   }
}

//
// AM_Ticker()
//
// Updates on gametic - enter follow mode, zoom, or change map location
//
// Passed nothing, returns nothing
//
void AM_Ticker(void)
{
   if(!automapactive)
      return;
   
   amclock++;
   
   if(followplayer)
      AM_doFollowPlayer();
   
   // Change the zoom if necessary
   if(ftom_zoommul != FRACUNIT)
      AM_changeWindowScale();
   
   // Change x,y location
   if(m_paninc.x || m_paninc.y)
      AM_changeWindowLoc();
}


//
// Clear automap frame buffer.
//
void AM_clearFB(int color)
{
   // haleyjd 12/22/02: backdrop support
   if(am_usebackdrop && am_backdrop)
   {
      // SoM 2-4-04: ANYRES
      V_DrawBlock(0, 0, &vbscreen, 
                  SCREENWIDTH, (f_h << FRACBITS) / globalyscale, 
                  am_backdrop);
   }
   else
      memset(fb, color, f_w*f_h);
}

//
// AM_clipMline()
//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes. If the speed is needed,
// use a hash algorithm to handle the common cases.
//
// Passed the line's coordinates on map and in the frame buffer performs
// clipping on them in the lines frame coordinates.
// Returns true if any part of line was not clipped
//
boolean AM_clipMline(mline_t *ml, fline_t *fl)
{
   enum
   {
      LEFT   = 1,
      RIGHT  = 2,
      BOTTOM = 4,
      TOP    = 8
   };
   
   register int outcode1 = 0;
   register int outcode2 = 0;
   register int outside;

   fpoint_t  tmp;
   int   dx;
   int   dy;

    
#define DOOUTCODE(oc, mx, my) \
   (oc) = 0; \
   if ((my) < 0) (oc) |= TOP; \
   else if ((my) >= f_h) (oc) |= BOTTOM; \
   if ((mx) < 0) (oc) |= LEFT; \
   else if ((mx) >= f_w) (oc) |= RIGHT;

    
   // do trivial rejects and outcodes
   if(ml->a.y > m_y2)
      outcode1 = TOP;
   else if(ml->a.y < m_y)
      outcode1 = BOTTOM;

   if(ml->b.y > m_y2)
      outcode2 = TOP;
   else if(ml->b.y < m_y)
      outcode2 = BOTTOM;

   if(outcode1 & outcode2)
      return false; // trivially outside

   if(ml->a.x < m_x)
      outcode1 |= LEFT;
   else if(ml->a.x > m_x2)
      outcode1 |= RIGHT;

   if(ml->b.x < m_x)
      outcode2 |= LEFT;
   else if(ml->b.x > m_x2)
      outcode2 |= RIGHT;

   if(outcode1 & outcode2)
      return false; // trivially outside

   // transform to frame-buffer coordinates.
   fl->a.x = CXMTOF(ml->a.x);
   fl->a.y = CYMTOF(ml->a.y);
   fl->b.x = CXMTOF(ml->b.x);
   fl->b.y = CYMTOF(ml->b.y);
   
   DOOUTCODE(outcode1, fl->a.x, fl->a.y);
   DOOUTCODE(outcode2, fl->b.x, fl->b.y);

   if(outcode1 & outcode2)
      return false;

   while(outcode1 | outcode2)
   {
      // may be partially inside box
      // find an outside point
      if(outcode1)
         outside = outcode1;
      else
         outside = outcode2;

      // clip to each side
      if(outside & TOP)
      {
         dy = fl->a.y - fl->b.y;
         dx = fl->b.x - fl->a.x;
         tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
         tmp.y = 0;
      }
      else if(outside & BOTTOM)
      {
         dy = fl->a.y - fl->b.y;
         dx = fl->b.x - fl->a.x;
         tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
         tmp.y = f_h-1;
      }
      else if(outside & RIGHT)
      {
         dy = fl->b.y - fl->a.y;
         dx = fl->b.x - fl->a.x;
         tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
         tmp.x = f_w-1;
      }
      else if(outside & LEFT)
      {
         dy = fl->b.y - fl->a.y;
         dx = fl->b.x - fl->a.x;
         tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
         tmp.x = 0;
      }

      if(outside == outcode1)
      {
         fl->a = tmp;
         DOOUTCODE(outcode1, fl->a.x, fl->a.y);
      }
      else
      {
         fl->b = tmp;
         DOOUTCODE(outcode2, fl->b.x, fl->b.y);
      }
      
      if(outcode1 & outcode2)
         return false; // trivially outside
   }
   
   return true;
}
#undef DOOUTCODE

//
// AM_drawFline()
//
// Draw a line in the frame buffer.
// Classic Bresenham w/ whatever optimizations needed for speed
//
// Passed the frame coordinates of line, and the color to be drawn
// Returns nothing
//
void AM_drawFline(fline_t *fl, int color )
{
   register int x;
   register int y;
   register int dx;
   register int dy;
   register int sx;
   register int sy;
   register int ax;
   register int ay;
   register int d;

#ifdef RANGECHECK         // killough 2/22/98    
   //static int fuck = 0;
   
   // For debugging only
   if(   fl->a.x < 0 || fl->a.x >= f_w
      || fl->a.y < 0 || fl->a.y >= f_h
      || fl->b.x < 0 || fl->b.x >= f_w
      || fl->b.y < 0 || fl->b.y >= f_h
     )
   {
      //fprintf(stderr, "fuck %d \r", fuck++);
      return;
   }
#endif

#define PUTDOT(xx,yy,cc) fb[(yy)*f_w+(xx)]=(cc)

   dx = fl->b.x - fl->a.x;
   ax = 2 * (dx < 0 ? -dx : dx);
   sx = dx < 0 ? -1 : 1;
   
   dy = fl->b.y - fl->a.y;
   ay = 2 * (dy < 0 ? -dy : dy);
   sy = dy < 0 ? -1 : 1;

   x = fl->a.x;
   y = fl->a.y;
   
   if(ax > ay)
   {
      d = ay - ax/2;
      while(1)
      {
         PUTDOT(x, y, color);
         if(x == fl->b.x) return;
         if(d >= 0)
         {
            y += sy;
            d -= ax;
         }
         x += sx;
         d += ay;
      }
   }
   else
   {
      d = ax - ay/2;
      while(1)
      {
         PUTDOT(x, y, color);
         if(y == fl->b.y) return;
         if(d >= 0)
         {
            x += sx;
            d -= ay;
         }
         y += sy;
         d += ax;
      }
   }
}

//
// AM_drawMline()
//
// Clip lines, draw visible parts of lines.
//
// Passed the map coordinates of the line, and the color to draw it
// Color -1 is special and prevents drawing. Color 247 is special and
// is translated to black, allowing Color 0 to represent feature disable
// in the defaults file.
// Returns nothing.
//
void AM_drawMline(mline_t *ml, int color )
{
   static fline_t fl;
   
   if(color == -1)  // jff 4/3/98 allow not drawing any sort of line
      return;       // by setting its color to -1
   if(color == 247) // jff 4/3/98 if color is 247 (xparent), use black
      color=0;
   
   if(AM_clipMline(ml, &fl))
      AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}

//
// AM_drawGrid()
//
// Draws blockmap aligned grid lines.
//
// Passed the color to draw the grid lines
// Returns nothing
//
void AM_drawGrid(int color)
{
   fixed_t x, y;
   fixed_t start, end;
   mline_t ml;
   
   // Figure out start of vertical gridlines
   start = m_x;
   if((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS))
      start += (MAPBLOCKUNITS << FRACBITS)
                - ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS));
   end = m_x + m_w;

   // draw vertical gridlines
   ml.a.y = m_y;
   ml.b.y = m_y + m_h;
   for(x = start; x < end; x += (MAPBLOCKUNITS << FRACBITS))
   {
      ml.a.x = x;
      ml.b.x = x;
      AM_drawMline(&ml, color);
   }

   // Figure out start of horizontal gridlines
   start = m_y;
   if((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS))
      start += (MAPBLOCKUNITS << FRACBITS)
                - ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS));
   end = m_y + m_h;

   // draw horizontal gridlines
   ml.a.x = m_x;
   ml.b.x = m_x + m_w;
   for(y = start; y < end; y += (MAPBLOCKUNITS << FRACBITS))
   {
      ml.a.y = y;
      ml.b.y = y;
      AM_drawMline(&ml, color);
   }
}

//
// AM_DoorColor()
//
// Returns the 'color' or key needed for a door linedef type
//
// Passed the type of linedef, returns:
//   -1 if not a keyed door
//    0 if a red key required
//    1 if a blue key required
//    2 if a yellow key required
//    3 if a multiple keys required
//
// jff 4/3/98 add routine to get color of generalized keyed door
//
int AM_DoorColor(int type)
{
   if(GenLockedBase <= type && type < GenDoorBase)
   {
      type -= GenLockedBase;
      type = (type & LockedKey) >> LockedKeyShift;
      if(!type || type==7)
         return 3;  //any or all keys
      else 
         return (type - 1) % 3;
   }

   switch(type)  // closed keyed door
   {
   case 26: case 32: case 99: case 133:
      /*bluekey*/
      return 1;
   case 27: case 34: case 136: case 137:
      /*yellowkey*/
      return 2;
   case 28: case 33: case 134: case 135:
      /*redkey*/
      return 0;
   default:
      return -1; // not a keyed door
   }
   return -1;    // not a keyed door
}

// haleyjd 07/07/04: Support routines to clean up the horrible
// mess that was AM_drawWalls. Go look in the beta 7 or earlier
// source if you want to see what I now consider one of the largest
// messes in all +120k lines of this program's code. Some of the
// code in these functions was actually repeated many times over.

//
// AM_drawAsExitLine
//
// Returns true if line is an exit and the exit map color is
// defined; returns false otherwise.
//
d_inline static boolean AM_drawAsExitLine(line_t *line)
{
   return (mapcolor_exit &&
           (line->special==11  ||
            line->special==52  ||
            line->special==197 ||
            line->special==51  ||
            line->special==124 ||
            line->special==198));
}

//
// AM_drawAs1sSecret
//
// Returns true if a 1S line is or was secret and the secret line
// map color is defined; returns false otherwise.
//
d_inline static boolean AM_drawAs1sSecret(line_t *line)
{
   return (mapcolor_secr &&
           ((map_secret_after &&
             P_WasSecret(line->frontsector) &&
             !P_IsSecret(line->frontsector)) ||
           (!map_secret_after &&
            P_WasSecret(line->frontsector))));
}

//
// AM_drawAs2sSecret
//
// Returns true if a 2S line is or was secret and the secret line
// map color is defined; returns false otherwise.
//
d_inline static boolean AM_drawAs2sSecret(line_t *line)
{
   //jff 2/16/98 fixed bug: special was cleared after getting it
   
   //jff 3/9/98 add logic to not show secret til after entered 
   // if map_secret_after is true

   // haleyjd: this is STILL horrible, but oh well.
   
   return (mapcolor_secr &&       
           ((map_secret_after &&
             ((P_WasSecret(line->frontsector) && 
               !P_IsSecret(line->frontsector)) || 
              (P_WasSecret(line->backsector) && 
               !P_IsSecret(line->backsector)))) ||  
           (!map_secret_after &&
            (P_WasSecret(line->frontsector) ||
             P_WasSecret(line->backsector)))));
}

//
// AM_drawAtTeleporter
//
// Returns true if a line is a teleporter and the teleporter map
// color is defined; returns false otherwise.
//
d_inline static boolean AM_drawAsTeleporter(line_t *line)
{
   return (mapcolor_tele && !(line->flags & ML_SECRET) && 
           (line->special == 39  || line->special == 97 ||
            line->special == 125 || line->special == 126));
}

//
// AM_drawAsLockedDoor
//
// Returns true if a line is a locked door for which the corresponding
// map color is defined; returns false otherwise.
//
// FIXME / HTIC_TODO: Heretic support
//
d_inline static boolean AM_drawAsLockedDoor(line_t *line)
{
   return ((mapcolor_bdor || mapcolor_ydor || mapcolor_rdor) &&
           ((line->special >=  26 && line->special <=  28) ||
            (line->special >=  32 && line->special <=  34) ||
            (line->special >= 133 && line->special <= 137) ||
            line->special == 99 ||
            (line->special >= GenLockedBase && 
             line->special <  GenDoorBase)));
}

//
// AM_isDoorClosed
//
// Returns true if a door is closed, false otherwise.
//
d_inline static boolean AM_isDoorClosed(line_t *line)
{
   return ((line->backsector->floorheight  == line->backsector->ceilingheight) ||
           (line->frontsector->floorheight == line->frontsector->ceilingheight));
}

//
// AM_drawAsClosedDoor
//
// Returns true if a door is closed, not secret, and closed door
// map color is defined; returns false otherwise.
//
d_inline static boolean AM_drawAsClosedDoor(line_t *line)
{
   return (mapcolor_clsd &&  
           !(line->flags & ML_SECRET) &&    // non-secret closed door
           AM_isDoorClosed(line));
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
// jff 1/5/98 many changes in this routine
// backward compatibility not needed, so just changes, no ifs
// addition of clauses for:
//    doors opening, keyed door id, secret sectors,
//    teleports, exit lines, key things
// ability to suppress any of added features or lines with no height changes
//
// support for gamma correction in automap abandoned
//
// jff 4/3/98 changed mapcolor_xxxx=0 as control to disable feature
// jff 4/3/98 changed mapcolor_xxxx=-1 to disable drawing line completely
//
void AM_drawWalls(void)
{
   int i;
   static mline_t l;
   
   // draw the unclipped visible portions of all lines
   for(i = 0; i < numlines; ++i)
   {
      line_t *line = &lines[i];

      l.a.x = line->v1->x;
      l.a.y = line->v1->y;
      l.b.x = line->v2->x;
      l.b.y = line->v2->y;

      // if line has been seen or IDDT has been used
      if(ddt_cheating || (line->flags & ML_MAPPED))
      {
         // check for DONTDRAW flag; those lines are only visible
         // if using the IDDT cheat.
         if((line->flags & ML_DONTDRAW) && !ddt_cheating)
            continue;

         if(!line->backsector) // 1S lines
         {            
            if(AM_drawAsExitLine(line))
            {
               //jff 4/23/98 add exit lines to automap
               AM_drawMline(&l, mapcolor_exit); // exit line
            }            
            else if(AM_drawAs1sSecret(line))
            {
               // jff 1/10/98 add new color for 1S secret sector boundary
               AM_drawMline(&l, mapcolor_secr); // line bounding secret sector
            }
            else                                //jff 2/16/98 fixed bug
               AM_drawMline(&l, mapcolor_wall); // special was cleared
         }
         else // 2S lines
         {
            // jff 1/10/98 add color change for all teleporter types
            if(AM_drawAsTeleporter(line))
            { 
               // teleporters
               AM_drawMline(&l, mapcolor_tele);
            }
            else if(AM_drawAsExitLine(line))
            {
               //jff 4/23/98 add exit lines to automap
               AM_drawMline(&l, mapcolor_exit);
            }
            else if(AM_drawAsLockedDoor(line))
            {
               //jff 1/5/98 this clause implements showing keyed doors
               if(AM_isDoorClosed(line))
               {
                  switch(AM_DoorColor(line->special)) // closed keyed door
                  {
                  case 1:
                     /*bluekey*/
                     AM_drawMline(&l,
                        mapcolor_bdor ? mapcolor_bdor : mapcolor_cchg);
                     break;
                  case 2:
                     /*yellowkey*/
                     AM_drawMline(&l,
                        mapcolor_ydor ? mapcolor_ydor : mapcolor_cchg);
                     break;
                  case 0:
                     /*redkey*/
                     AM_drawMline(&l,
                        mapcolor_rdor ? mapcolor_rdor : mapcolor_cchg);
                     break;
                  case 3:
                     /*any or all*/
                     AM_drawMline(&l,
                        mapcolor_clsd ? mapcolor_clsd : mapcolor_cchg);
                     break;
                  }
               }
               else
                  AM_drawMline(&l, mapcolor_cchg); // open keyed door
            }
            else if(line->flags & ML_SECRET)    // secret door
            {
               AM_drawMline(&l, mapcolor_wall);      // wall color
            }
            else if(AM_drawAsClosedDoor(line))
            {
               AM_drawMline(&l, mapcolor_clsd); // non-secret closed door
            } 
            else if(AM_drawAs2sSecret(line))
            {
               AM_drawMline(&l, mapcolor_secr); // line bounding secret sector
            } 
            else if(line->backsector->floorheight !=
                    line->frontsector->floorheight)
            {
               AM_drawMline(&l, mapcolor_fchg); // floor level change
            }
            else if(line->backsector->ceilingheight !=
                    line->frontsector->ceilingheight)
            {
               AM_drawMline(&l, mapcolor_cchg); // ceiling level change
            }
            else if(mapcolor_flat && ddt_cheating)
            { 
               AM_drawMline(&l, mapcolor_flat); // 2S lines that appear only in IDDT
            }
         }
      } 
      else if(plr->powers[pw_allmap]) // computermap visible lines
      {
         // now draw the lines only visible because the player has computermap
         if(!(line->flags & ML_DONTDRAW)) // invisible flag lines do not show
         {
            if(mapcolor_flat ||
               !line->backsector ||
               line->backsector->floorheight != line->frontsector->floorheight ||
               line->backsector->ceilingheight != line->frontsector->ceilingheight
              )
            {
               AM_drawMline(&l, mapcolor_unsn);
            }
         }
      } // end else if
   } // end for
}

//
// AM_rotate()
//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
// Passed the coordinates of a point, and an angle
// Returns the coordinates rotated by the angle
//
// haleyjd 01/24/03: made static
//
static void AM_rotate(fixed_t *x, fixed_t *y, angle_t a)
{
   fixed_t tmpx;

   tmpx = FixedMul(*x, finecosine[a >> ANGLETOFINESHIFT])
           - FixedMul(*y, finesine[a >> ANGLETOFINESHIFT]);

   *y   = FixedMul(*x, finesine[a >> ANGLETOFINESHIFT])
           + FixedMul(*y, finecosine[a >> ANGLETOFINESHIFT]);

   *x = tmpx;
}

//
// AM_drawLineCharacter()
//
// Draws a vector graphic according to numerous parameters
//
// Passed the structure defining the vector graphic shape, the number
// of vectors in it, the scale to draw it at, the angle to draw it at,
// the color to draw it with, and the map coordinates to draw it at.
// Returns nothing
//
void AM_drawLineCharacter
( mline_t*  lineguy,
  int   lineguylines,
  fixed_t scale,
  angle_t angle,
  int   color,
  fixed_t x,
  fixed_t y )
{
   int   i;
   mline_t l;
   
   for(i = 0; i < lineguylines; ++i)
   {
      l.a.x = lineguy[i].a.x;
      l.a.y = lineguy[i].a.y;

      if(scale)
      {
         l.a.x = FixedMul(scale, l.a.x);
         l.a.y = FixedMul(scale, l.a.y);
      }

      if(angle)
         AM_rotate(&l.a.x, &l.a.y, angle);

      l.a.x += x;
      l.a.y += y;
      
      l.b.x = lineguy[i].b.x;
      l.b.y = lineguy[i].b.y;
      
      if(scale)
      {
         l.b.x = FixedMul(scale, l.b.x);
         l.b.y = FixedMul(scale, l.b.y);
      }
      
      if(angle)
         AM_rotate(&l.b.x, &l.b.y, angle);

      l.b.x += x;
      l.b.y += y;
      
      AM_drawMline(&l, color);
   }
}

//
// AM_drawPlayers()
//
// Draws the player arrow in single player, 
// or all the player arrows in a netgame.
//
// Passed nothing, returns nothing
//
void AM_drawPlayers(void)
{
   int   i;
   player_t* p;
   int   their_color = -1;
   int   color;

   if(!netgame)
   {
      if(ddt_cheating)
      {
         AM_drawLineCharacter
          (
            cheat_player_arrow,
            NUMCHEATPLYRLINES,
            0,
            plr->mo->angle,
            mapcolor_sngl,      //jff color
            plr->mo->x,
            plr->mo->y
          );
      }
      else
      {
         AM_drawLineCharacter
          (
            player_arrow,
            NUMPLYRLINES,
            0,
            plr->mo->angle,
            mapcolor_sngl,      //jff color
            plr->mo->x,
            plr->mo->y
          );
      }
      return;
   }
  
   for(i = 0; i < MAXPLAYERS; ++i)
   {
      their_color = players[i].colormap;
      p = &players[i];
      
      // killough 9/29/98: use !demoplayback so internal demos are no different
      if((GameType == gt_dm && !demoplayback) && p != plr)
         continue;

      if(!playeringame[i])
         continue;
      
      // haleyjd: add total invisibility
      
      if(p->powers[pw_invisibility] || p->powers[pw_totalinvis])
         color = 246; // *close* to black
      else
      {
         // sf: extended colour range
#define GREEN 112
         if(their_color == 0)
         {
            color = GREEN;
         }
         else
         {
            // haleyjd 01/12/04: rewritten
            byte *transtbl = translationtables[their_color - 1];

            color = transtbl[GREEN];
         }
      }
      
      AM_drawLineCharacter
       (
         player_arrow,
         NUMPLYRLINES,
         0,
         p->mo->angle,
         color,
         p->mo->x,
         p->mo->y
       );
   }
}

//
// AM_drawThings()
//
// Draws the things on the automap in double IDDT cheat mode
//
// Passed colors and colorrange, no longer used
// Returns nothing
//
void AM_drawThings(int colors, int colorrange)
{
   int     i;
   mobj_t *t;
   
   // for all sectors
   for(i = 0; i < numsectors; ++i)
   {
      t = sectors[i].thinglist;
      while(t) // for all things in that sector
      {
         // FIXME / HTIC_TODO: Heretic support and EDF editing?

         //jff 1/5/98 case over doomednum of thing being drawn
         if(mapcolor_rkey || mapcolor_ykey || mapcolor_bkey)
         {
            switch(t->info->doomednum)
            {
               //jff 1/5/98 treat keys special
            case 38: case 13: //jff  red key
               AM_drawLineCharacter
                  (
                   cross_mark,
                   NUMCROSSMARKLINES,
                   16<<FRACBITS,
                   t->angle,
                   mapcolor_rkey!=-1? mapcolor_rkey : mapcolor_sprt,
                   t->x,
                   t->y
                  );
               t = t->snext;
               continue;
            case 39: case 6: //jff yellow key
               AM_drawLineCharacter
                  (
                   cross_mark,
                   NUMCROSSMARKLINES,
                   16<<FRACBITS,
                   t->angle,
                   mapcolor_ykey!=-1? mapcolor_ykey : mapcolor_sprt,
                   t->x,
                   t->y
                  );
               t = t->snext;
               continue;
            case 40: case 5: //jff blue key
               AM_drawLineCharacter
                  (
                   cross_mark,
                   NUMCROSSMARKLINES,
                   16<<FRACBITS,
                   t->angle,
                   mapcolor_bkey!=-1? mapcolor_bkey : mapcolor_sprt,
                   t->x,
                   t->y
                  );
               t = t->snext;
               continue;
            default:
               break;
            }
         }

         //jff 1/5/98 end added code for keys
         //jff previously entire code
         AM_drawLineCharacter
            (
             thintriangle_guy,
             NUMTHINTRIANGLEGUYLINES,
             16<<FRACBITS,
             t->angle,
             // killough 8/8/98: mark friends specially
             t->flags & MF_FRIEND && !t->player ? mapcolor_frnd : mapcolor_sprt,
             t->x,
             t->y
            );
         t = t->snext;
      } // end if
   } // end for
}

//
// AM_drawMarks()
//
// Draw the marked locations on the automap
//
// Passed nothing, returns nothing
//
// killough 2/22/98:
// Rewrote AM_drawMarks(). Removed limit on marks.
//
// SoM: ANYRES support
//
void AM_drawMarks(void)
{
   int i;
   for(i = 0; i < markpointnum; ++i) // killough 2/22/98: remove automap mark limit
   {
      if(markpoints[i].x != -1)
      {
         int w  = (5 * globalxscale) >> FRACBITS;
         int h  = (6 * globalyscale) >> FRACBITS;
         int fx = CXMTOF(markpoints[i].x);
         int fy = CYMTOF(markpoints[i].y);
         int j  = i;
         
         do
         {
            int d = j % 10;
            
            if(d == 1)          // killough 2/22/98: less spacing for '1'
               fx += (globalxscale >> FRACBITS);
            
            if(fx >= f_x && fx < f_w - w && fy >= f_y && fy < f_h - h)
            {
               V_DrawPatch((fx<<FRACBITS)/globalxscale, 
                           (fy<<FRACBITS)/globalyscale, 
                           &vbscreen, 
                           marknums[d]);
            }
            
            fx -= w - (globalxscale >> FRACBITS); // killough 2/22/98: 1 space backwards
            
            j /= 10;

         } while(j > 0);
      }
   }
}

//
// AM_drawCrosshair()
//
// Draw the single point crosshair representing map center
//
// Passed the color to draw the pixel with
// Returns nothing
// haleyjd: made inline static
//
d_inline static void AM_drawCrosshair(int color)
{
   fb[(f_w*(f_h+1))/2] = color; // single point for now
}

//
// AM_Drawer()
//
// Draws the entire automap
//
// Passed nothing, returns nothing
//
void AM_Drawer(void)
{
   if(!automapactive)
      return;

   AM_clearFB(mapcolor_back);       //jff 1/5/98 background default color
   
   if(automap_grid)                 // killough 2/28/98: change var name
      AM_drawGrid(mapcolor_grid);   //jff 1/7/98 grid default color
   
   AM_drawWalls();
   AM_drawPlayers();
   
   if(ddt_cheating == 2)
      AM_drawThings(mapcolor_sprt, 0); //jff 1/5/98 default double IDDT sprite

   AM_drawCrosshair(mapcolor_hair); //jff 1/7/98 default crosshair color   
   AM_drawMarks();
}

//----------------------------------------------------------------------------
//
// $Log: am_map.c,v $
// Revision 1.24  1998/05/10  12:05:24  jim
// formatted/documented am_map
//
// Revision 1.23  1998/05/03  22:13:49  killough
// Provide minimal headers at top; no other changes
//
// Revision 1.22  1998/04/23  13:06:53  jim
// Add exit line to automap
//
// Revision 1.21  1998/04/16  16:16:56  jim
// Fixed disappearing marks after new level
//
// Revision 1.20  1998/04/03  14:45:17  jim
// Fixed automap disables at 0, mouse sens unbounded
//
// Revision 1.19  1998/03/28  05:31:40  jim
// Text enabling changes for DEH
//
// Revision 1.18  1998/03/23  03:06:22  killough
// I wonder
//
// Revision 1.17  1998/03/15  14:36:46  jim
// fixed secrets transfer bug in automap
//
// Revision 1.16  1998/03/10  07:06:21  jim
// Added secrets on automap after found only option
//
// Revision 1.15  1998/03/09  18:29:22  phares
// Created separately bound automap and menu keys
//
// Revision 1.14  1998/03/02  11:22:30  killough
// change grid to automap_grid and make external
//
// Revision 1.13  1998/02/23  04:08:11  killough
// Remove limit on automap marks, save them in savegame
//
// Revision 1.12  1998/02/17  22:58:40  jim
// Fixed bug of vanishinb secret sectors in automap
//
// Revision 1.11  1998/02/15  03:12:42  phares
// Jim's previous comment: Fixed bug in automap from mistaking framebuffer index for mark color
//
// Revision 1.10  1998/02/15  02:47:33  phares
// User-defined keys
//
// Revision 1.8  1998/02/09  02:50:13  killough
// move ddt cheat to st_stuff.c and some cleanup
//
// Revision 1.7  1998/02/02  22:16:31  jim
// Fixed bug in automap that showed secret lines
//
// Revision 1.6  1998/01/26  20:57:54  phares
// Second test of checkin/checkout
//
// Revision 1.5  1998/01/26  20:28:15  phares
// First checkin/checkout script test
//
// Revision 1.4  1998/01/26  19:23:00  phares
// First rev with no ^Ms
//
// Revision 1.3  1998/01/24  11:21:25  jim
// Changed disables in automap to -1 and -2 (nodraw)
//
// Revision 1.1.1.1  1998/01/19  14:02:53  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------

