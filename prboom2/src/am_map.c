/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *   the automap code
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "doomstat.h"
#include "st_stuff.h"
#include "d_main.h"
#include "r_main.h"
#include "p_setup.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "v_video.h"
#include "p_spec.h"
#include "am_map.h"
#include "dstrings.h"
#include "d_deh.h"    // Ty 03/27/98 - externalizations
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf
#include "g_game.h"
#include "c_io.h"
#include "c_runcmd.h"

//jff 1/7/98 default automap colors added
int mapcolor_back = 247;    // map background
CONSOLE_INT(mapcolor_back, mapcolor_back, NULL, 0, 255, NULL, 0) {}

int mapcolor_grid = 104;    // grid lines color
CONSOLE_INT(mapcolor_grid, mapcolor_grid, NULL, 0, 255, NULL, 0) {}

int mapcolor_wall = 23;    // normal 1s wall color
CONSOLE_INT(mapcolor_wall, mapcolor_wall, NULL, 0, 255, NULL, 0) {}

int mapcolor_fchg = 55;    // line at floor height change color
CONSOLE_INT(mapcolor_fchg, mapcolor_fchg, NULL, 0, 255, NULL, 0) {}

int mapcolor_cchg = 215;    // line at ceiling height change color
CONSOLE_INT(mapcolor_cchg, mapcolor_cchg, NULL, 0, 255, NULL, 0) {}

int mapcolor_clsd = 208;    // line at sector with floor=ceiling color
CONSOLE_INT(mapcolor_clsd, mapcolor_clsd, NULL, 0, 255, NULL, 0) {}

int mapcolor_rkey = 175;    // red key color
CONSOLE_INT(mapcolor_rkey, mapcolor_rkey, NULL, 0, 255, NULL, 0) {}

int mapcolor_bkey = 204;    // blue key color
CONSOLE_INT(mapcolor_bkey, mapcolor_bkey, NULL, 0, 255, NULL, 0) {}

int mapcolor_ykey = 231;    // yellow key color
CONSOLE_INT(mapcolor_ykey, mapcolor_ykey, NULL, 0, 255, NULL, 0) {}

int mapcolor_rdor = 175;    // red door color  (diff from keys to allow option)
CONSOLE_INT(mapcolor_rdor, mapcolor_rdor, NULL, 0, 255, NULL, 0) {}

int mapcolor_bdor = 204;    // blue door color (of enabling one but not other )
CONSOLE_INT(mapcolor_bdor, mapcolor_bdor, NULL, 0, 255, NULL, 0) {}

int mapcolor_ydor = 231;    // yellow door color
CONSOLE_INT(mapcolor_ydor, mapcolor_ydor, NULL, 0, 255, NULL, 0) {}

int mapcolor_tele = 119;    // teleporter line color
CONSOLE_INT(mapcolor_tele, mapcolor_tele, NULL, 0, 255, NULL, 0) {}

int mapcolor_secr = 252;    // secret sector boundary color
CONSOLE_INT(mapcolor_secr, mapcolor_secr, NULL, 0, 255, NULL, 0) {}

int mapcolor_exit = 0;    // jff 4/23/98 add exit line color
CONSOLE_INT(mapcolor_exit, mapcolor_exit, NULL, 0, 255, NULL, 0) {}

int mapcolor_unsn = 104;    // computer map unseen line color
CONSOLE_INT(mapcolor_unsn, mapcolor_unsn, NULL, 0, 255, NULL, 0) {}

int mapcolor_flat = 88;    // line with no floor/ceiling changes
CONSOLE_INT(mapcolor_flat, mapcolor_flat, NULL, 0, 255, NULL, 0) {}

int mapcolor_sprt = 112;    // general sprite color
CONSOLE_INT(mapcolor_sprt, mapcolor_sprt, NULL, 0, 255, NULL, 0) {}

int mapcolor_hair = 208;    // crosshair color
CONSOLE_INT(mapcolor_hair, mapcolor_hair, NULL, 0, 255, NULL, 0) {}

int mapcolor_sngl = 208;    // single player arrow color
CONSOLE_INT(mapcolor_sngl, mapcolor_sngl, NULL, 0, 255, NULL, 0) {}

int mapcolor_frnd = 112;    // colors for friends of player
CONSOLE_INT(mapcolor_frnd, mapcolor_frnd, NULL, 0, 255, NULL, 0) {}

int mapcolor_me = 112;    // cph
CONSOLE_INT(mapcolor_me, mapcolor_me, NULL, 0, 255, NULL, 0) {}

int mapcolor_plyr[4];  // colors for players in multiplayer

void AM_AddColors()
{
  C_AddCommand(mapcolor_back);
  C_AddCommand(mapcolor_wall);
  C_AddCommand(mapcolor_fchg);
  C_AddCommand(mapcolor_grid);
  C_AddCommand(mapcolor_cchg);
  C_AddCommand(mapcolor_clsd);
  C_AddCommand(mapcolor_rkey);
  C_AddCommand(mapcolor_ykey);
  C_AddCommand(mapcolor_bkey);
  C_AddCommand(mapcolor_rdor);
  C_AddCommand(mapcolor_ydor);
  C_AddCommand(mapcolor_bdor);
  C_AddCommand(mapcolor_tele);
  C_AddCommand(mapcolor_secr);
  C_AddCommand(mapcolor_exit);
  C_AddCommand(mapcolor_unsn);
  C_AddCommand(mapcolor_sprt);
  C_AddCommand(mapcolor_hair);
  C_AddCommand(mapcolor_sngl);
  C_AddCommand(mapcolor_frnd);
  C_AddCommand(mapcolor_me);
}

//jff 3/9/98 add option to not show secret sectors until entered
int map_secret_after = 0;
//jff 4/3/98 add symbols for "no-color" for disable and "black color" for black
#define NC 0
#define BC 247

// drawing stuff
#define FB    0

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
mline_t triangle_guy[] =
{
{ { (fixed_t)(-.867*R), (fixed_t)(-.5*R) }, { (fixed_t)( .867*R), (fixed_t)(-.5*R) } },
{ { (fixed_t)( .867*R), (fixed_t)(-.5*R) }, { (fixed_t)(0      ), (fixed_t)(    R) } },
{ { (fixed_t)(0      ), (fixed_t)(    R) }, { (fixed_t)(-.867*R), (fixed_t)(-.5*R) } }
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
mline_t thintriangle_guy[] =
{
{ { (fixed_t)(-.5*R), (fixed_t)(-.7*R) }, { (fixed_t)(    R), (fixed_t)(    0) } },
{ { (fixed_t)(    R), (fixed_t)(    0) }, { (fixed_t)(-.5*R), (fixed_t)( .7*R) } },
{ { (fixed_t)(-.5*R), (fixed_t)( .7*R) }, { (fixed_t)(-.5*R), (fixed_t)(-.7*R) } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))

int ddt_cheating = 0;         // killough 2/7/98: make global, rename to ddt_*

static int leveljuststarted = 1;       // kluge until AM_LevelInit() is called

enum automapmode_e automapmode = 0; // Mode that the automap is in

// location of window on screen
static int  f_x;
static int  f_y;

// size of window on screen
static int  f_w;
static int  f_h;

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

// killough 2/22/98: Remove limit on automap marks,
// and make variables external for use in savegames.

mpoint_t *markpoints = NULL;    // where the points are
int markpointnum = 0; // next point to be assigned (also number of points now)
int markpointnum_max = 0;       // killough 2/22/98

static boolean stopped = true;

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
void AM_getIslope
( mline_t*  ml,
  islope_t* is )
{
  int dx, dy;

  dy = ml->a.y - ml->b.y;
  dx = ml->b.x - ml->a.x;
  if (!dy)
    is->islp = (dx<0?-INT_MAX:INT_MAX);
  else
    is->islp = FixedDiv(dx, dy);
  if (!dx)
    is->slp = (dy<0?-INT_MAX:INT_MAX);
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
  if (!(automapmode & am_follow))
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
  scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
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

  if (markpointnum >= markpointnum_max)
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

  min_x = min_y =  INT_MAX;
  max_x = max_y = -INT_MAX;

  for (i=0;i<numvertexes;i++)
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

  a = FixedDiv(f_w<<FRACBITS, max_w);
  b = FixedDiv(f_h<<FRACBITS, max_h);

  min_scale_mtof = a < b ? a : b;
  max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);
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
  if (m_paninc.x || m_paninc.y)
  {
    automapmode &= ~am_follow;
    f_oldloc.x = INT_MAX;
  }

  m_x += m_paninc.x;
  m_y += m_paninc.y;

  if (m_x + m_w/2 > max_x)
    m_x = max_x - m_w/2;
  else if (m_x + m_w/2 < min_x)
    m_x = min_x - m_w/2;

  if (m_y + m_h/2 > max_y)
    m_y = max_y - m_h/2;
  else if (m_y + m_h/2 < min_y)
    m_y = min_y - m_h/2;

  m_x2 = m_x + m_w;
  m_y2 = m_y + m_h;
}


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
  static event_t st_notify = { ev_keyup, AM_MSGENTERED };

  automapmode |= am_active;

  f_oldloc.x = INT_MAX;

  m_paninc.x = m_paninc.y = 0;
  ftom_zoommul = FRACUNIT;
  mtof_zoommul = FRACUNIT;

  m_w = FTOM(f_w);
  m_h = FTOM(f_h);

  // find player to center on initially
  if (!playeringame[pnum = consoleplayer])
  for (pnum=0;pnum<MAXPLAYERS;pnum++)
    if (playeringame[pnum])
  break;

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
  ST_Responder(&st_notify);
}

//
// AM_loadPics()
// 
void AM_loadPics(void)
{
  // cph - mark numbers no longer needed cached
}

//
// AM_unloadPics()
//
void AM_unloadPics(void)
{
  // cph - mark numbers no longer needed cached
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
// CPhipps - get status bar height from status bar code
void AM_LevelInit(void)
{
  leveljuststarted = 0;

  f_x = f_y = 0;
  f_w = SCREENWIDTH;           // killough 2/7/98: get rid of finit_ vars
  f_h = SCREENHEIGHT-ST_SCALED_HEIGHT;// to allow runtime setting of width/height

  AM_findMinMaxBoundaries();
  scale_mtof = FixedDiv(min_scale_mtof, (int) (0.7*FRACUNIT));
  if (scale_mtof > max_scale_mtof)
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
void AM_Stop (void)
{
  static event_t st_notify = { 0, ev_keyup, AM_MSGEXITED };

  AM_unloadPics();
  automapmode &= ~am_active;
  ST_Responder(&st_notify);
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
void AM_Start()
{
  static int lastlevel = -1, lastepisode = -1;

  if (!stopped)
    AM_Stop();
  redrawsbar = redrawborder = true;  // sf: redraw needed
  stopped = false;
  if (lastlevel != gamemap || lastepisode != gameepisode)
  {
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

//
// AM_Responder()
//
// Handle events (user inputs) in automap mode
//
// Passed an input event, returns true if its handled
//
boolean AM_Responder
( event_t*  ev )
{
  int rc;
  static int cheatstate=0;
  static int bigstate=0;
  int ch;                                                       // phares

  rc = false;

  if (!(automapmode & am_active))
  {
    if (ev->type == ev_keydown && ev->data1 == key_map)         // phares
    {
      AM_Start ();
      rc = true;
    }
  }
  else if (ev->type == ev_keydown)
  {
    rc = true;
    ch = ev->data1;                                             // phares
    if (ch == key_map_right)                                    //    |
      if (!(automapmode & am_follow))                           //    V
        m_paninc.x = FTOM(F_PANINC);
      else
        rc = false;
    else if (ch == key_map_left)
      if (!(automapmode & am_follow))
          m_paninc.x = -FTOM(F_PANINC);
      else
          rc = false;
    else if (ch == key_map_up)
      if (!(automapmode & am_follow))
          m_paninc.y = FTOM(F_PANINC);
      else
          rc = false;
    else if (ch == key_map_down)
      if (!(automapmode & am_follow))
          m_paninc.y = -FTOM(F_PANINC);
      else
          rc = false;
    else if (ch == key_map_zoomout)
    {
      mtof_zoommul = M_ZOOMOUT;
      ftom_zoommul = M_ZOOMIN;
    }
    else if (ch == key_map_zoomin)
    {
      mtof_zoommul = M_ZOOMIN;
      ftom_zoommul = M_ZOOMOUT;
    }
    else if (ch == key_map)
    {
      bigstate = 0;
      AM_Stop ();
    }
    else if (ch == key_map_gobig)
    {
      bigstate = !bigstate;
      if (bigstate)
      {
        AM_saveScaleAndLoc();
        AM_minOutWindowScale();
      }
      else
        AM_restoreScaleAndLoc();
    }
    else if (ch == key_map_follow)
    {
      automapmode ^= am_follow;     // CPhipps - put all automap mode stuff into one enum
      f_oldloc.x = INT_MAX;
      // Ty 03/27/98 - externalized
      doom_printf((automapmode & am_follow) ? s_AMSTR_FOLLOWON : s_AMSTR_FOLLOWOFF);
    }
    else if (ch == key_map_grid)
    {
      automapmode ^= am_grid;      // CPhipps
      // Ty 03/27/98 - *not* externalized
      doom_printf((automapmode & am_grid) ? s_AMSTR_GRIDON : s_AMSTR_GRIDOFF);
    }
    else if (ch == key_map_mark)
    {
      /* Ty 03/27/98 - *not* externalized     
       * cph 2001/11/20 - use doom_printf so we don't have our own buffer */
      doom_printf("%s %d", s_AMSTR_MARKEDSPOT, markpointnum);
      AM_addMark();
    }
    else if (ch == key_map_clear)
    {
      AM_clearMarks();  // Ty 03/27/98 - *not* externalized
      doom_printf(s_AMSTR_MARKSCLEARED);                        //    ^
    }                                                           //    |
    else if (ch == key_map_rotate) {
      automapmode ^= am_rotate;
      doom_printf((automapmode & am_rotate) ? s_AMSTR_ROTATEON : s_AMSTR_ROTATEOFF);
    }
    else if (ch == key_map_overlay) {
      automapmode ^= am_overlay;
      doom_printf((automapmode & am_overlay) ? s_AMSTR_OVERLAYON : s_AMSTR_OVERLAYOFF);
    }
    else                                                        // phares
    {
      cheatstate=0;
      rc = false;
    }
  }
  else if (ev->type == ev_keyup)
  {
    rc = false;
    ch = ev->data1;
    if (ch == key_map_right)
    {
      if (!(automapmode & am_follow))
          m_paninc.x = 0;
    }
    else if (ch == key_map_left)
    {
      if (!(automapmode & am_follow))
          m_paninc.x = 0;
    }
    else if (ch == key_map_up)
    {
      if (!(automapmode & am_follow))
          m_paninc.y = 0;
    }
    else if (ch == key_map_down)
    {
      if (!(automapmode & am_follow))
          m_paninc.y = 0;
    }
    else if ((ch == key_map_zoomout) || (ch == key_map_zoomin))
    {
      mtof_zoommul = FRACUNIT;
      ftom_zoommul = FRACUNIT;
    }
  }
  return rc;
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
// CPhipps - made static & enhanced for automap rotation

static void AM_rotate(fixed_t* x,  fixed_t* y, angle_t a, fixed_t xorig, fixed_t yorig)
{
  fixed_t tmpx;

  tmpx =
    FixedMul(*x - xorig,finecosine[a>>ANGLETOFINESHIFT])
      - FixedMul(*y - yorig,finesine[a>>ANGLETOFINESHIFT]);

  *y   = yorig + 
    FixedMul(*x - xorig,finesine[a>>ANGLETOFINESHIFT])
      + FixedMul(*y - yorig,finecosine[a>>ANGLETOFINESHIFT]);

  *x = tmpx + xorig;
}

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

  if (scale_mtof < min_scale_mtof)
    AM_minOutWindowScale();
  else if (scale_mtof > max_scale_mtof)
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
  if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
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
// AM_Ticker()
//
// Updates on gametic - enter follow mode, zoom, or change map location
//
// Passed nothing, returns nothing
//
void AM_Ticker (void)
{
  if (!(automapmode & am_active))
    return;

  if (automapmode & am_follow)
    AM_doFollowPlayer();

  // Change the zoom if necessary
  if (ftom_zoommul != FRACUNIT)
    AM_changeWindowScale();

  // Change x,y location
  if (m_paninc.x || m_paninc.y)
    AM_changeWindowLoc();
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
boolean AM_clipMline
( mline_t*  ml,
  fline_t*  fl )
{
  enum
  {
    LEFT    =1,
    RIGHT   =2,
    BOTTOM  =4,
    TOP     =8
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
  if (ml->a.y > m_y2)
  outcode1 = TOP;
  else if (ml->a.y < m_y)
  outcode1 = BOTTOM;

  if (ml->b.y > m_y2)
  outcode2 = TOP;
  else if (ml->b.y < m_y)
  outcode2 = BOTTOM;

  if (outcode1 & outcode2)
  return false; // trivially outside

  if (ml->a.x < m_x)
  outcode1 |= LEFT;
  else if (ml->a.x > m_x2)
  outcode1 |= RIGHT;

  if (ml->b.x < m_x)
  outcode2 |= LEFT;
  else if (ml->b.x > m_x2)
  outcode2 |= RIGHT;

  if (outcode1 & outcode2)
  return false; // trivially outside

  // transform to frame-buffer coordinates.
  fl->a.x = CXMTOF(ml->a.x);
  fl->a.y = CYMTOF(ml->a.y);
  fl->b.x = CXMTOF(ml->b.x);
  fl->b.y = CYMTOF(ml->b.y);

  DOOUTCODE(outcode1, fl->a.x, fl->a.y);
  DOOUTCODE(outcode2, fl->b.x, fl->b.y);

  if (outcode1 & outcode2)
  return false;

  while (outcode1 | outcode2)
  {
    // may be partially inside box
    // find an outside point
    if (outcode1)
      outside = outcode1;
    else
      outside = outcode2;

    // clip to each side
    if (outside & TOP)
    {
      dy = fl->a.y - fl->b.y;
      dx = fl->b.x - fl->a.x;
      tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
      tmp.y = 0;
    }
    else if (outside & BOTTOM)
    {
      dy = fl->a.y - fl->b.y;
      dx = fl->b.x - fl->a.x;
      tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
      tmp.y = f_h-1;
    }
    else if (outside & RIGHT)
    {
      dy = fl->b.y - fl->a.y;
      dx = fl->b.x - fl->a.x;
      tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
      tmp.x = f_w-1;
    }
    else if (outside & LEFT)
    {
      dy = fl->b.y - fl->a.y;
      dx = fl->b.x - fl->a.x;
      tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
      tmp.x = 0;
    }

    if (outside == outcode1)
    {
      fl->a = tmp;
      DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    }
    else
    {
      fl->b = tmp;
      DOOUTCODE(outcode2, fl->b.x, fl->b.y);
    }

    if (outcode1 & outcode2)
      return false; // trivially outside
  }

  return true;
}
#undef DOOUTCODE

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
void AM_drawMline
( mline_t*  ml,
  int   color )
{
  static fline_t fl;

  if (color==-1)  // jff 4/3/98 allow not drawing any sort of line
    return;       // by setting its color to -1
  if (color==247) // jff 4/3/98 if color is 247 (xparent), use black
    color=0;

  if (AM_clipMline(ml, &fl))
    V_DrawLine(&fl, color); // draws it on frame buffer using fb coords
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
  if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
    start += (MAPBLOCKUNITS<<FRACBITS)
      - ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
  end = m_x + m_w;

  // draw vertical gridlines
  ml.a.y = m_y;
  ml.b.y = m_y+m_h;
  for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
  {
    ml.a.x = x;
    ml.b.x = x;
    AM_drawMline(&ml, color);
  }

  // Figure out start of horizontal gridlines
  start = m_y;
  if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
    start += (MAPBLOCKUNITS<<FRACBITS)
      - ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
  end = m_y + m_h;

  // draw horizontal gridlines
  ml.a.x = m_x;
  ml.b.x = m_x + m_w;
  for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
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
  if (GenLockedBase <= type && type< GenDoorBase)
  {
    type -= GenLockedBase;
    type = (type & LockedKey) >> LockedKeyShift;
    if (!type || type==7)
      return 3;  //any or all keys
    else return (type-1)%3;
  }
  switch (type)  // closed keyed door
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
      return -1; //not a keyed door
  }
  return -1;     //not a keyed door
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
  for (i=0;i<numlines;i++)
  {
    l.a.x = lines[i].v1->x;
    l.a.y = lines[i].v1->y;
    l.b.x = lines[i].v2->x;
    l.b.y = lines[i].v2->y;

    if (automapmode & am_rotate) {
      AM_rotate(&l.a.x, &l.a.y, ANG90-plr->mo->angle, plr->mo->x, plr->mo->y);
      AM_rotate(&l.b.x, &l.b.y, ANG90-plr->mo->angle, plr->mo->x, plr->mo->y);
    }

    // if line has been seen or IDDT has been used
    if (ddt_cheating || (lines[i].flags & ML_MAPPED))
    {
      if ((lines[i].flags & ML_DONTDRAW) && !ddt_cheating)
        continue;
      {
        /* cph - show keyed doors and lines */
        int amd;
        if ((mapcolor_bdor || mapcolor_ydor || mapcolor_rdor) &&
            !(lines[i].flags & ML_SECRET) &&    /* non-secret */
          (amd = AM_DoorColor(lines[i].special)) != -1
        )
        {
          {
            switch (amd) /* closed keyed door */
            {
              case 1:
                /*bluekey*/
                AM_drawMline(&l,
                  mapcolor_bdor? mapcolor_bdor : mapcolor_cchg);
                continue;
              case 2:
                /*yellowkey*/
                AM_drawMline(&l,
                  mapcolor_ydor? mapcolor_ydor : mapcolor_cchg);
                continue;
              case 0:
                /*redkey*/
                AM_drawMline(&l,
                  mapcolor_rdor? mapcolor_rdor : mapcolor_cchg);
                continue;
              case 3:
                /*any or all*/
                AM_drawMline(&l,
                  mapcolor_clsd? mapcolor_clsd : mapcolor_cchg);
                continue;
            }
          }
        }
      }
      if /* jff 4/23/98 add exit lines to automap */
        (
          mapcolor_exit &&
          (
            lines[i].special==11 ||
            lines[i].special==52 ||
            lines[i].special==197 ||
            lines[i].special==51  ||
            lines[i].special==124 ||
            lines[i].special==198
          )
        ) {
          AM_drawMline(&l, mapcolor_exit); /* exit line */
          continue;
        }

      if (!lines[i].backsector)
      {
        // jff 1/10/98 add new color for 1S secret sector boundary
        if (mapcolor_secr && //jff 4/3/98 0 is disable
            (
             (
              map_secret_after &&
              P_WasSecret(lines[i].frontsector) &&
              !P_IsSecret(lines[i].frontsector)
             )
             ||
             (
              !map_secret_after &&
              P_WasSecret(lines[i].frontsector)
             )
            )
          )
          AM_drawMline(&l, mapcolor_secr); // line bounding secret sector
        else                               //jff 2/16/98 fixed bug
          AM_drawMline(&l, mapcolor_wall); // special was cleared
      }
      else /* now for 2S lines */
      {
        // jff 1/10/98 add color change for all teleporter types
        if
        (
            mapcolor_tele && !(lines[i].flags & ML_SECRET) && 
            (lines[i].special == 39 || lines[i].special == 97 ||
            lines[i].special == 125 || lines[i].special == 126)
        )
        { // teleporters
          AM_drawMline(&l, mapcolor_tele);
        }
        else if (lines[i].flags & ML_SECRET)    // secret door
        {
          AM_drawMline(&l, mapcolor_wall);      // wall color
        }
        else if
        (
            mapcolor_clsd &&  
            !(lines[i].flags & ML_SECRET) &&    // non-secret closed door
            ((lines[i].backsector->floorheight==lines[i].backsector->ceilingheight) ||
            (lines[i].frontsector->floorheight==lines[i].frontsector->ceilingheight))
        )
        {
          AM_drawMline(&l, mapcolor_clsd);      // non-secret closed door
        } //jff 1/6/98 show secret sector 2S lines
        else if
        (
            mapcolor_secr && //jff 2/16/98 fixed bug
            (                    // special was cleared after getting it
              (map_secret_after &&
               (
                (P_WasSecret(lines[i].frontsector)
                 && !P_IsSecret(lines[i].frontsector)) || 
                (P_WasSecret(lines[i].backsector)
                 && !P_IsSecret(lines[i].backsector))
               )
              )
              ||  //jff 3/9/98 add logic to not show secret til after entered
              (   // if map_secret_after is true
                !map_secret_after &&
                 (P_WasSecret(lines[i].frontsector) ||
                  P_WasSecret(lines[i].backsector))
              )
            )
        )
        {
          AM_drawMline(&l, mapcolor_secr); // line bounding secret sector
        } //jff 1/6/98 end secret sector line change
        else if (lines[i].backsector->floorheight !=
                  lines[i].frontsector->floorheight)
        {
          AM_drawMline(&l, mapcolor_fchg); // floor level change
        }
        else if (lines[i].backsector->ceilingheight !=
                  lines[i].frontsector->ceilingheight)
        {
          AM_drawMline(&l, mapcolor_cchg); // ceiling level change
        }
        else if (mapcolor_flat && ddt_cheating)
        { 
          AM_drawMline(&l, mapcolor_flat); //2S lines that appear only in IDDT  
        }
      }
    } // now draw the lines only visible because the player has computermap
    else if (plr->powers[pw_allmap]) // computermap visible lines
    {
      if (!(lines[i].flags & ML_DONTDRAW)) // invisible flag lines do not show
      {
        if
        (
          mapcolor_flat
          ||
          !lines[i].backsector
          ||
          lines[i].backsector->floorheight
          != lines[i].frontsector->floorheight
          ||
          lines[i].backsector->ceilingheight
          != lines[i].frontsector->ceilingheight
        )
          AM_drawMline(&l, mapcolor_unsn);
      }
    }
  }
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

  if (automapmode & am_rotate) angle -= plr->mo->angle - ANG90; // cph

  for (i=0;i<lineguylines;i++)
  {
    l.a.x = lineguy[i].a.x;
    l.a.y = lineguy[i].a.y;

    if (scale)
    {
      l.a.x = FixedMul(scale, l.a.x);
      l.a.y = FixedMul(scale, l.a.y);
    }

    if (angle)
      AM_rotate(&l.a.x, &l.a.y, angle, 0, 0);

    l.a.x += x;
    l.a.y += y;

    l.b.x = lineguy[i].b.x;
    l.b.y = lineguy[i].b.y;

    if (scale)
    {
      l.b.x = FixedMul(scale, l.b.x);
      l.b.y = FixedMul(scale, l.b.y);
    }

    if (angle)
      AM_rotate(&l.b.x, &l.b.y, angle, 0, 0);

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

  if (!netgame)
  {
    if (ddt_cheating)
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
    else
      AM_drawLineCharacter
      (
        player_arrow,
        NUMPLYRLINES,
        0,
        plr->mo->angle,
        mapcolor_sngl,      //jff color
        plr->mo->x,
        plr->mo->y);        
    return;
  }

  for (i=0;i<MAXPLAYERS;i++) {
    player_t* p = &players[i];
    
    if ( (deathmatch && !singledemo) && p != plr)
      continue;

    if (playeringame[i]) {
      fixed_t x = p->mo->x, y = p->mo->y;
      if (automapmode & am_rotate)
	AM_rotate(&x, &y, ANG90-plr->mo->angle, plr->mo->x, plr->mo->y);

      AM_drawLineCharacter (player_arrow, NUMPLYRLINES, 0, p->mo->angle,
			    p->powers[pw_invisibility] ? 246 /* *close* to black */ 
			    : mapcolor_plyr[i], //jff 1/6/98 use default color
			    x, y);
    }
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
void AM_drawThings
( int colors,
  int  colorrange)
{
  int   i;
  mobj_t* t;

  // for all sectors
  for (i=0;i<numsectors;i++)
  {
    t = sectors[i].thinglist;
    while (t) // for all things in that sector
    {
      fixed_t x = t->x, y = t->y;

      if (automapmode & am_rotate)
	AM_rotate(&x, &y, ANG90-plr->mo->angle, plr->mo->x, plr->mo->y);

      //jff 1/5/98 case over doomednum of thing being drawn
      if (mapcolor_rkey || mapcolor_ykey || mapcolor_bkey)
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
              x, y
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
              x, y
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
              x, y
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
        t->flags & MF_FRIEND && !t->player ? mapcolor_frnd : mapcolor_sprt,
        x, y
      );
      t = t->snext;
    }
  }
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
void AM_drawMarks(void)
{
  int i;
  for (i=0;i<markpointnum;i++) // killough 2/22/98: remove automap mark limit
    if (markpoints[i].x != -1)
    {
      int w = 5;
      int h = 6;
      int fx = markpoints[i].x;
      int fy = markpoints[i].y;
      int j = i;

      if (automapmode & am_rotate)
        AM_rotate(&fx, &fy, ANG90-plr->mo->angle, plr->mo->x, plr->mo->y);

      fx = CXMTOF(fx); fy = CYMTOF(fy);

      do
      {
        int d = j % 10;
        if (d==1)           // killough 2/22/98: less spacing for '1'
          fx++;

        if (fx >= f_x && fx < f_w - w && fy >= f_y && fy < f_h - h) {
	  // cph - construct patch name and draw marker
	  char namebuf[] = { 'A', 'M', 'M', 'N', 'U', 'M', '0'+d, 0 };
	  
          V_DrawNamePatch(fx, fy, FB, namebuf, CR_DEFAULT, VPT_NONE);
	}
        fx -= w-1;          // killough 2/22/98: 1 space backwards
        j /= 10;
      }
      while (j>0);
    }
}

//
// AM_drawCrosshair()
//
// Draw the single point crosshair representing map center
//
// Passed the color to draw the pixel with
// Returns nothing
//
// CPhipps - made static inline, and use the general pixel plotter function

inline static void AM_drawCrosshair(int color)
{
  // single point for now
  V_PlotPixel(FB, f_w/2, f_h/2, (byte)color);
}

//
// AM_Drawer()
//
// Draws the entire automap
//
// Passed nothing, returns nothing
//
void AM_Drawer (void)
{
  // CPhipps - all automap modes put into one enum
  if (!(automapmode & am_active)) return;

  if (!(automapmode & am_overlay)) // cph - If not overlay mode, clear background for the automap
    V_FillRect(FB, f_x, f_y, f_w, f_h, (byte)mapcolor_back); //jff 1/5/98 background default color
  if (automapmode & am_grid)
    AM_drawGrid(mapcolor_grid);      //jff 1/7/98 grid default color
  AM_drawWalls();
  AM_drawPlayers();
  if (ddt_cheating==2)
    AM_drawThings(mapcolor_sprt, 0); //jff 1/5/98 default double IDDT sprite
  AM_drawCrosshair(mapcolor_hair);   //jff 1/7/98 default crosshair color

  AM_drawMarks();
}

//============================================================================
//
// Console Commands
//
//============================================================================

CONSOLE_COMMAND(togglemap, 0)
{
  if(automapmode & am_active)
    AM_Stop();
  else
    AM_Start();
}

void AM_AddCommands()
{
  C_AddCommand(togglemap);

  AM_AddColors();
}

