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
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: st_stuff.c,v 1.46 1998/05/06 16:05:40 jim Exp $";

#include "doomdef.h"
#include "doomstat.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "m_random.h"
#include "i_video.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "st_lib.h"
#include "r_main.h"
#include "am_map.h"
#include "m_cheat.h"
#include "s_sound.h"
#include "sounds.h"
#include "dstrings.h"
#include "d_gi.h" // haleyjd
#include "hu_over.h" // haleyjd

//
// STATUS BAR DATA
//

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS            1
#define STARTBONUSPALS          9
#define NUMREDPALS              8
#define NUMBONUSPALS            4
// Radiation suit, green shift.
#define RADIATIONPAL            13

// Location of status bar
#define ST_X                    0
#define ST_X2                   104

#define ST_FX                   143
#define ST_FY                   169

// Should be set to patch width
//  for tall numbers later on
// haleyjd: unused
//#define ST_TALLNUMWIDTH         (tallnum[0]->width)

#define ST_TURNOFFSET           (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET           (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET       (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET        (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE              (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE             (ST_GODFACE+1)

#define ST_FACESX               143
#define ST_FACESY               168

#define ST_EVILGRINCOUNT        (2*TICRATE)
#define ST_STRAIGHTFACECOUNT    (TICRATE/2)
#define ST_TURNCOUNT            (1*TICRATE)
#define ST_OUCHCOUNT            (1*TICRATE)
#define ST_RAMPAGEDELAY         (2*TICRATE)

#define ST_MUCHPAIN             20

// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?
// I dunno, why don't you go and find out!!!  killough

// AMMO number pos.
#define ST_AMMOWIDTH            3
#define ST_AMMOX                44
#define ST_AMMOY                171

// HEALTH number pos.
#define ST_HEALTHWIDTH          3
#define ST_HEALTHX              90
#define ST_HEALTHY              171

// Weapon pos.
#define ST_ARMSX                111
#define ST_ARMSY                172
#define ST_ARMSBGX              104
#define ST_ARMSBGY              168
#define ST_ARMSXSPACE           12
#define ST_ARMSYSPACE           10

// Frags pos.
#define ST_FRAGSX               138
#define ST_FRAGSY               171
#define ST_FRAGSWIDTH           2

// ARMOR number pos.
#define ST_ARMORWIDTH           3
#define ST_ARMORX               221
#define ST_ARMORY               171

// Key icon positions.
#define ST_KEY0WIDTH            8
#define ST_KEY0HEIGHT           5
#define ST_KEY0X                239
#define ST_KEY0Y                171
#define ST_KEY1WIDTH            ST_KEY0WIDTH
#define ST_KEY1X                239
#define ST_KEY1Y                181
#define ST_KEY2WIDTH            ST_KEY0WIDTH
#define ST_KEY2X                239
#define ST_KEY2Y                191

// Ammunition counter.
#define ST_AMMO0WIDTH           3
#define ST_AMMO0HEIGHT          6
#define ST_AMMO0X               288
#define ST_AMMO0Y               173
#define ST_AMMO1WIDTH           ST_AMMO0WIDTH
#define ST_AMMO1X               288
#define ST_AMMO1Y               179
#define ST_AMMO2WIDTH           ST_AMMO0WIDTH
#define ST_AMMO2X               288
#define ST_AMMO2Y               191
#define ST_AMMO3WIDTH           ST_AMMO0WIDTH
#define ST_AMMO3X               288
#define ST_AMMO3Y               185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH        3
#define ST_MAXAMMO0HEIGHT       5
#define ST_MAXAMMO0X            314
#define ST_MAXAMMO0Y            173
#define ST_MAXAMMO1WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X            314
#define ST_MAXAMMO1Y            179
#define ST_MAXAMMO2WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X            314
#define ST_MAXAMMO2Y            191
#define ST_MAXAMMO3WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X            314
#define ST_MAXAMMO3Y            185

// killough 2/8/98: weapon info position macros UNUSED, removed here

// main player in game

#define plyr (&players[displayplayer])

// haleyjd 10/12/03: DOOM's status bar object

static void ST_DoomTicker(void);
static void ST_DoomDrawer(void);
static void ST_DoomFSDrawer(void);
static void ST_DoomStart(void);
static void ST_DoomInit(void);

stbarfns_t DoomStatusBar =
{
   ST_HEIGHT,

   ST_DoomTicker,
   ST_DoomDrawer,
   ST_DoomFSDrawer,
   ST_DoomStart,
   ST_DoomInit
};

// sf 15/10/99: removed a load of useless stuff left over from
//              when the messages were displayed in the statusbar!
//              amazing that this code from the _betas_ was still here

// ST_Start() has just been called
static boolean st_firsttime;

// lump number for PLAYPAL
static int lu_palette;

// used for timing
static unsigned int st_clock;

// whether in automap or first-person
static st_stateenum_t st_gamestate;

// whether left-side main status bar is active
static boolean st_statusbaron;

// haleyjd: whether status bar background is on (for fullscreen hud)
static boolean st_backgroundon;

// !deathmatch
static boolean st_notdeathmatch;

// !deathmatch && st_statusbaron
static boolean st_armson;

// !deathmatch
static boolean st_fragson;

// main bar left
static patch_t *sbar;

// 0-9, tall numbers
static patch_t *tallnum[10];

// tall % sign
static patch_t *tallpercent;

// 0-9, short, yellow (,different!) numbers
static patch_t *shortnum[10];

// 3 key-cards, 3 skulls, 3 card/skull combos
// jff 2/24/98 extend number of patches by three skull/card combos
// sf: unstaticed for overlay 
patch_t *keys[NUMCARDS+3];

// face status patches
patch_t *default_faces[ST_NUMFACES];

// face background
static patch_t *faceback; // sf: change to use one and colormap

 // main bar right
static patch_t *armsbg;

// weapon ownership patches
static patch_t *arms[6][2];

// haleyjd: fullscreen patches
static patch_t *fs_health;
static patch_t *fs_armorg;
static patch_t *fs_armorb;
static patch_t *fs_ammo[4];

// ready-weapon widget
static st_number_t w_ready;

//jff 2/16/98 status color change levels
int ammo_red;      // ammo percent less than which status is red
int ammo_yellow;   // ammo percent less is yellow more green
int health_red;    // health amount less than which status is red
int health_yellow; // health amount less than which status is yellow
int health_green;  // health amount above is blue, below is green
int armor_red;     // armor amount less than which status is red
int armor_yellow;  // armor amount less than which status is yellow
int armor_green;   // armor amount above is blue, below is green

 // in deathmatch only, summary of frags stats
static st_number_t w_frags;

// health widget
static st_percent_t w_health;

// arms background
static st_binicon_t  w_armsbg;

// weapon ownership widgets
static st_multicon_t w_arms[6];

// face status widget
static st_multicon_t w_faces;

// keycard widgets
static st_multicon_t w_keyboxes[3];

// armor widget
static st_percent_t  w_armor;

// ammo widgets
static st_number_t   w_ammo[4];

// max ammo widgets
static st_number_t   w_maxammo[4];

 // number of frags so far in deathmatch
static int      st_fragscount;

// used to use appopriately pained face
static int      st_oldhealth = -1;

// used for evil grin
static boolean  oldweaponsowned[NUMWEAPONS];

 // count until face changes
static int      st_facecount = 0;

// current face index, used by w_faces
static int      st_faceindex = 0;

// holds key-type for each key box on bar
static int      keyboxes[3];

extern char     *mapnames[];
extern byte     **translationtables;

//
// STATUS BAR CODE
//

extern VBuffer backscreen4;

static void ST_refreshBackground(void)
{
   if(st_statusbaron)
   {
      V_DrawPatch(ST_X, 0, &backscreen4, sbar);

      // killough 3/7/98: make face background change with displayplayer
      // haleyjd 01/12/04: changed translation handling
      if(GameType != gt_single)
      {
         V_DrawPatchTranslated(ST_FX, 0, &backscreen4, faceback,
            plyr->colormap ?
               (char *)translationtables[(plyr->colormap - 1)] :
               NULL, 
            false);
      }

      V_CopyRect(ST_X, 0, BG, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, FG);
      
      // faces
      STlib_initMultIcon(&w_faces,  ST_FACESX, ST_FACESY, //default_faces,
         players[displayplayer].skin->faces, &st_faceindex, &st_statusbaron);
   }
}

//
// ST_AutomapEvent
//
// haleyjd 09/29/04: Replaces the weird hack Dave Taylor put into
// ST_Responder to toggle the status bar when the automap is toggled.
// The automap now calls this function instead of sending fake events
// to ST_Responder, allowing that function to be minimized.
//
void ST_AutomapEvent(int type)
{
   switch(type)
   {
   case AM_MSGENTERED:
      st_gamestate = AutomapState;
      st_firsttime = true;
      break;
   case AM_MSGEXITED:
      st_gamestate = FirstPersonState;
      break;
   }
}

//
// ST_Responder
//
// Respond to keyboard input events, intercept cheats.
// This code is shared by all status bars.
//
boolean ST_Responder(event_t *ev)
{
   // TODO: allow cheat input to be disabled
   // if a user keypress...
   if(ev->type == ev_keydown)         // Try cheat responder in m_cheat.c
      return M_FindCheats(ev->data1); // killough 4/17/98, 5/2/98
   return false;
}

static int ST_calcPainOffset(void)
{
   static int lastcalc;
   static int oldhealth = -1;
   int health = plyr->health > 100 ? 100 : plyr->health;
   
   if(health != oldhealth)
   {
      lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
      oldhealth = health;
   }
   return lastcalc;
}

//
// ST_updateFaceWidget
//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
static void ST_updateFaceWidget(void)
{
   int         i;
   angle_t     badguyangle;
   angle_t     diffang;
   static int  lastattackdown = -1;
   static int  priority = 0;
   boolean     doevilgrin;
   
   if(priority < 10)
   {
      // dead
      if(!plyr->health)
      {
         priority = 9;
         st_faceindex = ST_DEADFACE;
         st_facecount = 1;
      }
   }

   if(priority < 9)
   {
      if(plyr->bonuscount)
      {
         // picking up bonus
         doevilgrin = false;

         for(i = 0; i < NUMWEAPONS; i++)
         {
            if (oldweaponsowned[i] != plyr->weaponowned[i])
            {
               doevilgrin = true;
               oldweaponsowned[i] = plyr->weaponowned[i];
            }
         }
         if(doevilgrin)
         {
            // evil grin if just picked up weapon
            priority = 8;
            st_facecount = ST_EVILGRINCOUNT;
            st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
         }
      }
   }

   if(priority < 8)
   {
      if(plyr->damagecount && plyr->attacker && plyr->attacker != plyr->mo)
      {
         // being attacked
         priority = 7;

         // haleyjd 10/12/03: classic DOOM problem of missing OUCH face
         // was due to inversion of this test:
         // if(plyr->health - st_oldhealth > ST_MUCHPAIN)
         if(st_oldhealth - plyr->health > ST_MUCHPAIN)
         {
            st_facecount = ST_TURNCOUNT;
            st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
         }
         else
         {
            badguyangle = R_PointToAngle2(plyr->mo->x,
                                          plyr->mo->y,
                                          plyr->attacker->x,
                                          plyr->attacker->y);

            if(badguyangle > plyr->mo->angle)
            {
               // whether right or left
               diffang = badguyangle - plyr->mo->angle;
               i = diffang > ANG180;
            }
            else
            {
               // whether left or right
               diffang = plyr->mo->angle - badguyangle;
               i = diffang <= ANG180;
            } // confusing, aint it?

            st_facecount = ST_TURNCOUNT;
            st_faceindex = ST_calcPainOffset();

            if(diffang < ANG45)
            {
               // head-on
               st_faceindex += ST_RAMPAGEOFFSET;
            }
            else if(i)
            {
               // turn face right
               st_faceindex += ST_TURNOFFSET;
            }
            else
            {
               // turn face left
               st_faceindex += ST_TURNOFFSET+1;
            }
         }
      }
   }

   if(priority < 7)
   {
      // getting hurt because of your own damn stupidity
      if(plyr->damagecount)
      {
         // haleyjd 10/12/03: classic DOOM problem of missing OUCH face
         // was due to inversion of this test:
         // if(plyr->health - st_oldhealth > ST_MUCHPAIN)
         if(st_oldhealth - plyr->health > ST_MUCHPAIN)
         {
            priority = 7;
            st_facecount = ST_TURNCOUNT;
            st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
         }
         else
         {
            priority = 6;
            st_facecount = ST_TURNCOUNT;
            st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
         }
      }
   }

   if(priority < 6)
   {
      // rapid firing
      if(plyr->attackdown)
      {
         if(lastattackdown==-1)
            lastattackdown = ST_RAMPAGEDELAY;
         else if(!--lastattackdown)
         {
            priority = 5;
            st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            st_facecount = 1;
            lastattackdown = 1;
         }
      }
      else
         lastattackdown = -1;
   }

   if(priority < 5)
   {
      // invulnerability
      if((plyr->cheats & CF_GODMODE) || plyr->powers[pw_invulnerability])
      {
         priority = 4;
         
         st_faceindex = ST_GODFACE;
         st_facecount = 1;
      }
   }

   // look left or look right if the facecount has timed out
   if(!st_facecount)
   {
      // sf: remove st_randomnumber
      st_faceindex = ST_calcPainOffset() + (M_Random() % 3);
      st_facecount = ST_STRAIGHTFACECOUNT;
      priority = 0;
   }
   
   st_facecount--;
}

int sts_traditional_keys; // killough 2/28/98: traditional status bar keys

static void ST_updateWidgets(void)
{
   static int  largeammo = 1994; // means "n/a"
   int         i;

   // must redirect the pointer if the ready weapon has changed.
   //  if (w_ready.data != plyr->readyweapon)
   //  {
   if(weaponinfo[plyr->readyweapon].ammo == am_noammo)
      w_ready.num = &largeammo;
   else
      w_ready.num = &plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
   //{
   // static int tic=0;
   // static int dir=-1;
   // if (!(tic&15))
   //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo]+=dir;
   // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
   //   dir = 1;
   // tic++;
   // }
   w_ready.data = plyr->readyweapon;

   // if (*w_ready.on)
   //  STlib_updateNum(&w_ready, true);
   // refresh weapon change
   //  }

   // update keycard multiple widgets
   for(i = 0; i < 3; i++)
   {
      keyboxes[i] = plyr->cards[i] ? i : -1;
      
      //jff 2/24/98 select double key
      //killough 2/28/98: preserve traditional keys by config option
      
      if(plyr->cards[i+3])
         keyboxes[i] = keyboxes[i]==-1 || sts_traditional_keys ? i+3 : i+6;
   }

   // refresh everything if this is him coming back to life
   ST_updateFaceWidget();

   // used by the w_armsbg widget
   st_notdeathmatch = (GameType != gt_dm);

   // used by w_arms[] widgets
   st_armson = st_statusbaron && GameType != gt_dm;

   // used by w_frags widget
   st_fragson = st_statusbaron && GameType == gt_dm;

   st_fragscount = plyr->totalfrags;     // sf 15/10/99 use totalfrags
}

//
// ST_DoomTicker
//
// Performs ticker actions for DOOM's status bar.
//
static void ST_DoomTicker(void)
{
   st_clock++;
   ST_updateWidgets();
   st_oldhealth = plyr->health;
}

//
// ST_Ticker
//
// Calls the current gamemode's status bar ticker function.
//
void ST_Ticker(void)
{
   gameModeInfo->StatusBar->Ticker();
}

static int st_palette = 0;

//
// ST_doPaletteStuff
//
// Performs player palette flashes for damage, item pickups, etc.
// This code is shared by all status bars.
//
static void ST_doPaletteStuff(void)
{
   int  palette;
   byte *pal;
   int  cnt = plyr->damagecount;

   if(plyr->powers[pw_strength])
   {
      // slowly fade the berzerk out
      int bzc = 12 - (plyr->powers[pw_strength]>>6);
      if(bzc > cnt)
         cnt = bzc;
   }

   if(cnt)
   {
      palette = (cnt+7)>>3;
      if(palette >= NUMREDPALS)
         palette = NUMREDPALS-1;
      palette += STARTREDPALS;
   }
   else if(plyr->bonuscount)
   {
      palette = (plyr->bonuscount+7)>>3;
      if(palette >= NUMBONUSPALS)
         palette = NUMBONUSPALS-1;
      palette += STARTBONUSPALS;
   }
   else if(plyr->powers[pw_ironfeet] > 4*32 || 
           plyr->powers[pw_ironfeet] & 8)
      palette = RADIATIONPAL;
   else
      palette = 0;

   if(camera) 
      palette = 0;     //sf
  
   if(palette != st_palette)
   {
      st_palette = palette;
      pal = (byte *)W_CacheLumpNum(lu_palette, PU_CACHE) + palette*768;
      I_SetPalette(pal);
   }
}

static void ST_drawCommonWidgets(boolean refresh)
{
   //jff 2/16/98 make color of ammo depend on amount
   if(*w_ready.num*100 < ammo_red*plyr->maxammo[weaponinfo[w_ready.data].ammo])
      STlib_updateNum(&w_ready, cr_red, refresh);
   else
    if(*w_ready.num*100 <
       ammo_yellow*plyr->maxammo[weaponinfo[w_ready.data].ammo])
      STlib_updateNum(&w_ready, cr_gold, refresh);
    else
      STlib_updateNum(&w_ready, cr_green, refresh);

   //jff 2/16/98 make color of health depend on amount
   if(*w_health.n.num < health_red)
      STlib_updatePercent(&w_health, cr_red, refresh);
   else if(*w_health.n.num < health_yellow)
      STlib_updatePercent(&w_health, cr_gold, refresh);
   else if(*w_health.n.num <= health_green)
      STlib_updatePercent(&w_health, cr_green, refresh);
   else
      STlib_updatePercent(&w_health, cr_blue_status, refresh); //killough 2/28/98

   //jff 2/16/98 make color of armor depend on amount
   if(*w_armor.n.num < armor_red)
      STlib_updatePercent(&w_armor, cr_red, refresh);
   else if (*w_armor.n.num < armor_yellow)
      STlib_updatePercent(&w_armor, cr_gold, refresh);
   else if (*w_armor.n.num <= armor_green)
      STlib_updatePercent(&w_armor, cr_green, refresh);
   else
      STlib_updatePercent(&w_armor, cr_blue_status, refresh); //killough 2/28/98
}

static void ST_drawWidgets(boolean refresh)
{
   int i;

   // used by w_arms[] widgets
   st_armson = st_statusbaron && !(GameType == gt_dm);

   // used by w_frags widget
   st_fragson = GameType == gt_dm && st_statusbaron;

   // haleyjd: draw widgets common to status bar and fullscreen 
   ST_drawCommonWidgets(refresh);

   for(i = 0; i < 4; i++)
   {
      STlib_updateNum(&w_ammo[i], NULL, refresh);   //jff 2/16/98 no xlation
      STlib_updateNum(&w_maxammo[i], NULL, refresh);
   }

   STlib_updateBinIcon(&w_armsbg, refresh);

   for(i = 0; i < 6; i++)
      STlib_updateMultIcon(&w_arms[i], refresh);

   STlib_updateMultIcon(&w_faces, refresh);

   for(i = 0; i < 3; i++)
      STlib_updateMultIcon(&w_keyboxes[i], refresh);

   STlib_updateNum(&w_frags, NULL, refresh);
}

static void ST_doRefresh(void)
{
   st_firsttime = false;
   
   // draw status bar background to off-screen buffer
   ST_refreshBackground();
   
   // and refresh all widgets
   ST_drawWidgets(true);
}

static void ST_diffDraw(void)
{
   // update all widgets
   ST_drawWidgets(false);
}

#define ST_FS_X 85
#define ST_FS_HEALTHY 154
#define ST_FS_BY 176
#define ST_FS_AMMOX 315

#define ST_FSGFX_X 5

static void ST_moveWidgets(boolean fs)
{
   if(fs)
   {
      if(w_health.n.x != ST_FS_X)
      {
         // set fullscreen graphical hud positions
         w_health.n.x = ST_FS_X;
         w_health.n.y = ST_FS_HEALTHY;
         w_armor.n.x  = ST_FS_X;
         w_armor.n.y  = ST_FS_BY;
         w_ready.x    = ST_FS_AMMOX;
         w_ready.y    = ST_FS_BY;
         
         // must refresh all widgets when moving them
         st_firsttime = true;
      }
   }
   else
   {
      if(w_health.n.x != ST_HEALTHX)
      {
         // restore normal status bar positions
         w_health.n.x = ST_HEALTHX;
         w_health.n.y = ST_HEALTHY;
         w_armor.n.x  = ST_ARMORX;
         w_armor.n.y  = ST_ARMORY;
         w_ready.x    = ST_AMMOX;
         w_ready.y    = ST_AMMOY;
         
         // must refresh all widgets when moving them
         st_firsttime = true;
      }
   }
}

//
// ST_DoomDrawer
//
// Draws stuff specific to the DOOM status bar.
//
static void ST_DoomDrawer(void)
{
   // possibly update widget positions
   ST_moveWidgets(false);

   if(st_firsttime)
      ST_doRefresh();     // If just after ST_Start(), refresh all
   else
      ST_diffDraw();      // Otherwise, update as little as possible
}

//
// ST_DoomFSDrawer
//
// Draws DOOM's new fullscreen hud/status information.
//
static void ST_DoomFSDrawer(void)
{
   ammotype_t ammo;

   // possibly update widget positions
   ST_moveWidgets(true);

   // draw graphics

   // health
   V_DrawPatchGeneral(ST_FSGFX_X, 152, &vbscreen, fs_health, false);
   
   // armor
   if(plyr->armortype == 2)
      V_DrawPatchGeneral(ST_FSGFX_X, ST_FS_BY, &vbscreen, fs_armorb, false);
   else
      V_DrawPatchGeneral(ST_FSGFX_X, ST_FS_BY, &vbscreen, fs_armorg, false);

   // ammo
   if((ammo = weaponinfo[w_ready.data].ammo) < NUMAMMO)
      V_DrawPatchGeneral(256, ST_FS_BY, &vbscreen, fs_ammo[ammo], false);

   // draw common number widgets (always refresh since no background)
   ST_drawCommonWidgets(true);
}

//
// ST_Drawer
//
// Performs player palette flashes and draws the current gamemode's
// status bar if appropriate.
//
void ST_Drawer(boolean fullscreen, boolean refresh)
{
   stbarfns_t *StatusBar = gameModeInfo->StatusBar;

   // haleyjd: test whether fullscreen graphical hud is enabled
   boolean fshud = hud_enabled && hud_overlaystyle == 4;

   st_statusbaron  = !fullscreen || automapactive || fshud;
   st_backgroundon = !fullscreen || automapactive;
   st_firsttime = st_firsttime || refresh;

   ST_doPaletteStuff();  // Do red-/gold-shifts from damage/items

   // sf: draw nothing in fullscreen
   // tiny bit faster and also removes the problem of status bar
   // percent '%' signs being drawn in fullscreen
   if(fullscreen && !automapactive)
   {
      // haleyjd: call game mode's fullscreen drawer when 
      // hud is enabled and hud_overlaystyle is "graphical"
      if(fshud)
         StatusBar->FSDrawer();
      return;
   }
   
   StatusBar->Drawer();
}

static void ST_loadGraphics(void)
{
   int  i;
   char namebuf[9];

   // Load the numbers, tall and short
   for(i = 0; i < 10; i++)
   {
      sprintf(namebuf, "STTNUM%d", i);
      tallnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
      sprintf(namebuf, "STYSNUM%d", i);
      shortnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
   }

   // Load percent key.
   //Note: why not load STMINUS here, too?
   tallpercent = (patch_t *) W_CacheLumpName("STTPRCNT", PU_STATIC);

   // key cards
   for(i = 0; i < NUMCARDS+3; i++)  //jff 2/23/98 show both keys too
   {
      sprintf(namebuf, "STKEYS%d", i);
      keys[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
   }

   // arms background
   armsbg = (patch_t *) W_CacheLumpName("STARMS", PU_STATIC);

   // arms ownership widgets
   for(i = 0; i < 6; i++)
   {
      sprintf(namebuf, "STGNUM%d", i+2);

      // gray #
      arms[i][0] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

      // yellow #
      arms[i][1] = shortnum[i+2];
   }

   // face backgrounds for different color players
   // killough 3/7/98: add better support for spy mode by loading all
   // player face backgrounds and using displayplayer to choose them:
   faceback = (patch_t *) W_CacheLumpName("STFB0", PU_STATIC);

   // status bar background bits
   sbar = (patch_t *) W_CacheLumpName("STBAR", PU_STATIC);

   // haleyjd: fullscreen graphics
   fs_health = (patch_t *)W_CacheLumpName("HU_FHLTH", PU_STATIC);
   fs_armorg = (patch_t *)W_CacheLumpName("HU_FARMR", PU_STATIC);
   fs_armorb = (patch_t *)W_CacheLumpName("HU_FARM2", PU_STATIC);
   for(i = 0; i < 4; ++i)
   {
      sprintf(namebuf, "HU_FAMM%d", i);
      fs_ammo[i] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);
   }

   ST_CacheFaces(default_faces, "STF");
}

//
// ST_CacheFaces
//
// Sets up the DOOM status bar face patches for a particular player
// skin.  Called by the skin code when creating a player skin.
//
void ST_CacheFaces(patch_t **faces, char *facename)
{
   int i, facenum;
   char namebuf[9];

   // haleyjd 10/11/03: not in other gamemodes
   if(gameModeInfo->type != Game_DOOM)
      return;

   // face states
   facenum = 0;
   for(i = 0; i < ST_NUMPAINFACES; i++)
   {
      int j;
      for(j = 0; j < ST_NUMSTRAIGHTFACES; j++)
      {
         sprintf(namebuf, "%sST%d%d",facename, i, j);
         faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
      }
      sprintf(namebuf, "%sTR%d0", facename, i);        // turn right
      faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
      sprintf(namebuf, "%sTL%d0", facename, i);        // turn left
      faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
      sprintf(namebuf, "%sOUCH%d", facename, i);       // ouch!
      faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
      sprintf(namebuf, "%sEVL%d", facename, i);        // evil grin ;)
      faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
      sprintf(namebuf, "%sKILL%d", facename, i);       // pissed off
      faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
   }
   sprintf(namebuf, "%sGOD0",facename);
   faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
   sprintf(namebuf, "%sDEAD0",facename);
   faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
}

static void ST_loadData(void)
{
   ST_loadGraphics();
}

#if 0

// haleyjd FIXME: the following two functions are never called -- ???
// they look like they should be, so this needs investigation.

static void ST_unloadGraphics(void)
{
   int i;

   // unload the numbers, tall and short
   for(i = 0; i < 10; i++)
   {
      Z_ChangeTag(tallnum[i], PU_CACHE);
      Z_ChangeTag(shortnum[i], PU_CACHE);
   }

   // unload tall percent
   Z_ChangeTag(tallpercent, PU_CACHE);

   // unload arms background
   Z_ChangeTag(armsbg, PU_CACHE);

   // unload gray #'s
   for(i = 0; i < 6; i++)
      Z_ChangeTag(arms[i][0], PU_CACHE);

   // unload the key cards
   for(i = 0; i < NUMCARDS+3; i++)  //jff 2/23/98 unload double key patches too
      Z_ChangeTag(keys[i], PU_CACHE);

   Z_ChangeTag(sbar, PU_CACHE);

   // killough 3/7/98: free each face background color
   for(i = 0; i < MAXPLAYERS; i++)
      Z_ChangeTag(faceback, PU_CACHE);

   for(i = 0; i < ST_NUMFACES; i++)
      Z_ChangeTag(default_faces[i], PU_CACHE);

   // Note: nobody ain't seen no unloading of stminus yet. Dude.
}

void ST_unloadData(void)
{
  ST_unloadGraphics();
}

#endif

static void ST_initData(void)
{
   int i;

   st_firsttime = true;

   st_clock = 0;
   st_gamestate = FirstPersonState;

   st_statusbaron = true;

   st_faceindex = 0;
   st_palette = -1;

   st_oldhealth = -1;

   for(i = 0; i < NUMWEAPONS; i++)
      oldweaponsowned[i] = plyr->weaponowned[i];

   for(i = 0; i < 3; i++)
      keyboxes[i] = -1;

   STlib_init();
}

static void ST_createWidgets(void)
{
   int i;

   // ready weapon ammo
   STlib_initNum(&w_ready,
                 ST_AMMOX,
                 ST_AMMOY,
                 tallnum,
                 &plyr->ammo[weaponinfo[plyr->readyweapon].ammo],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_AMMOWIDTH );

   // the last weapon type
   w_ready.data = plyr->readyweapon;

   // health percentage
   STlib_initPercent(&w_health,
                     ST_HEALTHX,
                     ST_HEALTHY,
                     tallnum,
                     &plyr->health,
                     &st_statusbaron,
                     &st_backgroundon,
                     tallpercent);

   // arms background
   STlib_initBinIcon(&w_armsbg,
                     ST_ARMSBGX,
                     ST_ARMSBGY,
                     armsbg,
                     &st_notdeathmatch,
                     &st_statusbaron);

   // weapons owned
   for(i = 0; i < 6; i++)
   {
      STlib_initMultIcon(&w_arms[i],
                         ST_ARMSX+(i%3)*ST_ARMSXSPACE,
                         ST_ARMSY+(i/3)*ST_ARMSYSPACE,
                         arms[i], (int *) &plyr->weaponowned[i+1],
                         &st_armson);
   }

   // frags sum
   STlib_initNum(&w_frags,
                 ST_FRAGSX,
                 ST_FRAGSY,
                 tallnum,
                 &st_fragscount,
                 &st_fragson,
                 &st_backgroundon,
                 ST_FRAGSWIDTH);

   // faces
   STlib_initMultIcon(&w_faces,
                      ST_FACESX,
                      ST_FACESY,
                      default_faces,
                      &st_faceindex,
                      &st_statusbaron);

   // armor percentage - should be colored later
   STlib_initPercent(&w_armor,
                     ST_ARMORX,
                     ST_ARMORY,
                     tallnum,
                     &plyr->armorpoints,
                     &st_statusbaron,
                     &st_backgroundon,
                     tallpercent);

   // keyboxes 0-2
   STlib_initMultIcon(&w_keyboxes[0],
                      ST_KEY0X,
                      ST_KEY0Y,
                      keys,
                      &keyboxes[0],
                      &st_statusbaron);

   STlib_initMultIcon(&w_keyboxes[1],
                      ST_KEY1X,
                      ST_KEY1Y,
                      keys,
                      &keyboxes[1],
                      &st_statusbaron);

   STlib_initMultIcon(&w_keyboxes[2],
                      ST_KEY2X,
                      ST_KEY2Y,
                      keys,
                      &keyboxes[2],
                      &st_statusbaron);

   // ammo count (all four kinds)
   STlib_initNum(&w_ammo[0],
                 ST_AMMO0X,
                 ST_AMMO0Y,
                 shortnum,
                 &plyr->ammo[0],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_AMMO0WIDTH);

   STlib_initNum(&w_ammo[1],
                 ST_AMMO1X,
                 ST_AMMO1Y,
                 shortnum,
                 &plyr->ammo[1],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_AMMO1WIDTH);

   STlib_initNum(&w_ammo[2],
                 ST_AMMO2X,
                 ST_AMMO2Y,
                 shortnum,
                 &plyr->ammo[2],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_AMMO2WIDTH);

   STlib_initNum(&w_ammo[3],
                 ST_AMMO3X,
                 ST_AMMO3Y,
                 shortnum,
                 &plyr->ammo[3],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_AMMO3WIDTH);

   // max ammo count (all four kinds)
   STlib_initNum(&w_maxammo[0],
                 ST_MAXAMMO0X,
                 ST_MAXAMMO0Y,
                 shortnum,
                 &plyr->maxammo[0],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_MAXAMMO0WIDTH);

   STlib_initNum(&w_maxammo[1],
                 ST_MAXAMMO1X,
                 ST_MAXAMMO1Y,
                 shortnum,
                 &plyr->maxammo[1],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_MAXAMMO1WIDTH);

   STlib_initNum(&w_maxammo[2],
                 ST_MAXAMMO2X,
                 ST_MAXAMMO2Y,
                 shortnum,
                 &plyr->maxammo[2],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_MAXAMMO2WIDTH);

   STlib_initNum(&w_maxammo[3],
                 ST_MAXAMMO3X,
                 ST_MAXAMMO3Y,
                 shortnum,
                 &plyr->maxammo[3],
                 &st_statusbaron,
                 &st_backgroundon,
                 ST_MAXAMMO3WIDTH);
}

VBuffer backscreen4;

//
// ST_DoomStart
//
// Startup function for the DOOM status bar.
//
static void ST_DoomStart(void)
{
   ST_initData();
   ST_createWidgets();

   memcpy(&backscreen4, &vbscreen, sizeof(VBuffer));
   backscreen4.data  = screens[4];
   backscreen4.pitch = backscreen4.width;
}

static boolean st_stopped = true;

//
// ST_Stop
//
// Status bar shut-off function. This code is shared by all
// status bars.
// haleyjd: moved up and made static.
//
static void ST_Stop(void)
{
   if(st_stopped)
      return;
   I_SetPalette(W_CacheLumpNum(lu_palette, PU_CACHE));   
   st_stopped = true;
}

//
// ST_Start
//
// Main startup function. Calls the current gamemode's
// status bar startup function as well.
//
void ST_Start(void)
{
   if(!st_stopped)
      ST_Stop();
   gameModeInfo->StatusBar->Start();
   st_stopped = false;
}

//
// ST_DoomInit
//
// Initializes the DOOM gamemode status bar
//
static void ST_DoomInit(void)
{
   ST_loadData();
   
   // SoM: allocate enough for ANYRES
   screens[4] = Z_Malloc(MAX_SCREENWIDTH * 150, PU_STATIC, 0);
}

//
// ST_Init
//
// Global status bar initialization function.
// Calls the current gamemode's status bar initialization
// function as well.
//
void ST_Init(void)
{
   // haleyjd: moved palette initialization here, because all
   // game modes will need it.
   lu_palette = W_GetNumForName("PLAYPAL");

   gameModeInfo->StatusBar->Init();
}

/***********************
        CONSOLE COMMANDS
 ***********************/

VARIABLE_INT(ammo_red, NULL,               0, 100, NULL);
VARIABLE_INT(ammo_yellow, NULL,            0, 100, NULL);
VARIABLE_INT(health_red, NULL,             0, 200, NULL);
VARIABLE_INT(health_yellow, NULL,          0, 200, NULL);
VARIABLE_INT(health_green, NULL,           0, 200, NULL);
VARIABLE_INT(armor_red, NULL,              0, 200, NULL);
VARIABLE_INT(armor_yellow, NULL,           0, 200, NULL);
VARIABLE_INT(armor_green, NULL,            0, 200, NULL);

VARIABLE_BOOLEAN(sts_pct_always_gray,      NULL,   yesno);
VARIABLE_BOOLEAN(sts_always_red,           NULL,   yesno);
VARIABLE_BOOLEAN(sts_traditional_keys,     NULL,   yesno);

CONSOLE_VARIABLE(ammo_red, ammo_red, 0) { }
CONSOLE_VARIABLE(ammo_yellow, ammo_yellow, 0) { }
CONSOLE_VARIABLE(health_red, health_red, 0) { }
CONSOLE_VARIABLE(health_yellow, health_yellow, 0) { }
CONSOLE_VARIABLE(health_green, health_green, 0) { }
CONSOLE_VARIABLE(armor_red, armor_red, 0) { }
CONSOLE_VARIABLE(armor_yellow, armor_yellow, 0) { }
CONSOLE_VARIABLE(armor_green, armor_green, 0) { }

CONSOLE_VARIABLE(st_graypct, sts_pct_always_gray, 0) {}
CONSOLE_VARIABLE(st_rednum, sts_always_red, 0) {}
CONSOLE_VARIABLE(st_singlekey, sts_traditional_keys, 0) {}

void ST_AddCommands(void)
{
   C_AddCommand(ammo_red);
   C_AddCommand(ammo_yellow);
   
   C_AddCommand(health_red);
   C_AddCommand(health_yellow);
   C_AddCommand(health_green);
   
   C_AddCommand(armor_red);
   C_AddCommand(armor_yellow);
   C_AddCommand(armor_green);
   
   C_AddCommand(st_graypct);
   C_AddCommand(st_rednum);
   C_AddCommand(st_singlekey);
}

//----------------------------------------------------------------------------
//
// $Log: st_stuff.c,v $
// Revision 1.46  1998/05/06  16:05:40  jim
// formatting and documenting
//
// Revision 1.45  1998/05/03  22:50:58  killough
// beautification, move external declarations, remove cheats
//
// Revision 1.44  1998/04/27  17:30:39  jim
// Fix DM demo/newgame status, remove IDK (again)
//
// Revision 1.43  1998/04/27  02:30:12  killough
// fuck you
//
// Revision 1.42  1998/04/24  23:52:31  thldrmn
// Removed idk cheat
//
// Revision 1.41  1998/04/24  11:39:23  killough
// Fix cheats while demo is played back
//
// Revision 1.40  1998/04/19  01:10:19  killough
// Generalize cheat engine to add deh support
//
// Revision 1.39  1998/04/16  06:26:06  killough
// Prevent cheats from working inside menu
//
// Revision 1.38  1998/04/12  10:58:24  jim
// IDMUSxy for DOOM 1 fix
//
// Revision 1.37  1998/04/12  10:23:52  jim
// IDMUS00 ok in DOOM 1
//
// Revision 1.36  1998/04/12  02:00:39  killough
// Change tranmap to main_tranmap
//
// Revision 1.35  1998/04/12  01:08:51  jim
// Fixed IDMUS00 crash
//
// Revision 1.34  1998/04/11  14:48:11  thldrmn
// Replaced IDK with TNTKA cheat
//
// Revision 1.33  1998/04/10  06:36:45  killough
// Fix -fast parameter bugs
//
// Revision 1.32  1998/03/31  10:37:17  killough
// comment clarification
//
// Revision 1.31  1998/03/28  18:09:19  killough
// Fix deh-cheat self-annihilation bug, make iddt closer to Doom
//
// Revision 1.30  1998/03/28  05:33:02  jim
// Text enabling changes for DEH
//
// Revision 1.29  1998/03/23  15:24:54  phares
// Changed pushers to linedef control
//
// Revision 1.28  1998/03/23  06:43:26  jim
// linedefs reference initial version
//
// Revision 1.27  1998/03/23  03:40:46  killough
// Fix idclip bug, make monster kills message smart
//
// Revision 1.26  1998/03/20  00:30:37  phares
// Changed friction to linedef control
//
// Revision 1.25  1998/03/17  20:44:32  jim
// fixed idmus non-restore, space bug
//
// Revision 1.24  1998/03/12  14:35:01  phares
// New cheat codes
//
// Revision 1.23  1998/03/10  07:14:38  jim
// Initial DEH support added, minus text
//
// Revision 1.22  1998/03/09  07:31:48  killough
// Fix spy mode to display player correctly, add TNTFAST
//
// Revision 1.21  1998/03/06  05:31:02  killough
// PEst control, from the TNT'EM man
//
// Revision 1.20  1998/03/02  15:35:03  jim
// Enabled Lee's status changes, added new types to common.cfg
//
// Revision 1.19  1998/03/02  12:09:18  killough
// blue status bar color, monsters_remember, traditional_keys
//
// Revision 1.18  1998/02/27  11:00:58  phares
// Can't own weapons that don't exist
//
// Revision 1.17  1998/02/26  22:57:45  jim
// Added message review display to HUD
//
// Revision 1.16  1998/02/24  08:46:45  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.15  1998/02/24  04:14:19  jim
// Added double keys to status
//
// Revision 1.14  1998/02/23  04:57:29  killough
// Fix TNTEM cheat again, add new cheats
//
// Revision 1.13  1998/02/20  21:57:07  phares
// Preliminarey sprite translucency
//
// Revision 1.12  1998/02/19  23:15:52  killough
// Add TNTAMMO in addition to TNTAMO
//
// Revision 1.11  1998/02/19  16:55:22  jim
// Optimized HUD and made more configurable
//
// Revision 1.10  1998/02/18  00:59:20  jim
// Addition of HUD
//
// Revision 1.9  1998/02/17  06:15:48  killough
// Add TNTKEYxx, TNTAMOx, TNTWEAPx cheats, and cheat engine support for them.
//
// Revision 1.8  1998/02/15  02:48:01  phares
// User-defined keys
//
// Revision 1.7  1998/02/09  03:19:04  killough
// Rewrite cheat code engine, add IDK and TNTHOM
//
// Revision 1.6  1998/02/02  22:19:01  jim
// Added TNTEM cheat to kill every monster alive
//
// Revision 1.5  1998/01/30  18:48:10  phares
// Changed textspeed and textwait to functions
//
// Revision 1.4  1998/01/30  16:09:03  phares
// Faster end-mission text display
//
// Revision 1.3  1998/01/28  12:23:05  phares
// TNTCOMP cheat code added
//
// Revision 1.2  1998/01/26  19:24:58  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
