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
// DESCRIPTION:
//
// Heretic Status Bar
//
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "m_random.h"
#include "p_mobj.h"
#include "v_video.h"
#include "w_wad.h"
#include "st_stuff.h"

//
// Defines
//
#define ST_HBARHEIGHT 42
#define plyr (&players[displayplayer])

//
// Static variables
//

// cached patches
static patch_t *barback;       // basic background
static patch_t *ltfacetop;     // left face top
static patch_t *rtfacetop;     // right face top
static patch_t *chain;         // the chain itself
static patch_t *lifegem;       // lifegem patch for chain
static patch_t *ltface;        // replacement part for left face
static patch_t *rtface;        // replacement part for right face
static patch_t *dmbar;         // deathmatch status bar
static patch_t *lifebar;       // normal status bar
static patch_t *statbar;       // points to one of the two above
static patch_t *invnums[10];   // inventory numbers
static patch_t *invneg;        // minus sign for inventory numbers

// current state variables
static int chainhealth;        // current position of the gem
static int chainwiggle;        // small randomized addend for chain y coord.

//
// ST_HticInit
//
// Initializes the Heretic status bar:
// * Caches most patch graphics used throughout
//
static void ST_HticInit(void)
{
   int i;

   barback   = W_CacheLumpName("BARBACK",  PU_STATIC);
   ltfacetop = W_CacheLumpName("LTFCTOP",  PU_STATIC);
   rtfacetop = W_CacheLumpName("RTFCTOP",  PU_STATIC);
   chain     = W_CacheLumpName("CHAIN",    PU_STATIC);
   ltface    = W_CacheLumpName("LTFACE",   PU_STATIC);
   rtface    = W_CacheLumpName("RTFACE",   PU_STATIC);
   
   // TODO: fix life gem for multiplayer modes
   lifegem   = W_CacheLumpName("LIFEGEM2", PU_STATIC);

   // load inventory numbers
   for(i = 0; i < 10; ++i)
   {
      char lumpname[9];

      memset(lumpname, 0, 9);
      sprintf(lumpname, "IN%d", i);

      invnums[i] = W_CacheLumpName(lumpname, PU_STATIC);
   }
   // load minus sign
   invneg = W_CacheLumpName("NEGNUM", PU_STATIC);

   // FIXME: these need to be loaded for the HUD, but we need
   // to add appropriate resources to eterhtic.wad for them
   for(i = 0; i < NUMCARDS+3; ++i)  //jff 2/23/98 show both keys too
   {
      extern patch_t *keys[];
      char namebuf[9];
      sprintf(namebuf, "STKEYS%d", i);
      keys[i] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);
   }
}

//
// ST_HticStart
//
static void ST_HticStart(void)
{
   // update the status bar patch for the appropriate game mode
   switch(GameType)
   {
   case gt_dm:
      if(!dmbar)
         dmbar = W_CacheLumpName("STATBAR", PU_STATIC);
      statbar = dmbar;
      break;
   default:
      if(!lifebar)
         lifebar = W_CacheLumpName("LIFEBAR", PU_STATIC);
      statbar = lifebar;
      break;
   }
}

//
// ST_HticTicker
//
// Processing code for Heretic status bar
//
static void ST_HticTicker(void)
{
   int playerHealth;

   // update the chain health value

   playerHealth = plyr->health;

   if(playerHealth != chainhealth)
   {
      int max, min, diff, sgn = 1;

      // set lower bound to zero
      if(playerHealth < 0)
         playerHealth = 0;

      // determine the max and min of the two values, and whether or
      // not to add or subtract from the chain position w/sgn
      if(playerHealth > chainhealth)
      {
         max = playerHealth; min = chainhealth;
      }
      else
      {
         sgn = -1; max = chainhealth; min = playerHealth;
      }

      // consider 1/4th of the difference
      diff = (max - min) >> 2;

      // don't move less than 1 or more than 8 units per tic
      if(diff < 1)
         diff = 1;
      else if(diff > 8)
         diff = 8;

      chainhealth += (sgn * diff);
   }

   // update chain wiggle value
   if(leveltime & 1)
      chainwiggle = M_Random() & 1;
}

static void ST_drawInvNum(int num, int x, int y)
{
   int numdigits = 3;
   boolean neg;

   neg = (num < 0);

   if(neg)
   {
      if(num < -9)
      {
         V_DrawPatch(x - 26, y + 1, &vbscreen, W_CacheLumpName("LAME", PU_CACHE));
         return;
      }
         
      num = -num;
   }

   if(!num)
      V_DrawPatch(x - 9, y, &vbscreen, invnums[0]);

   while(num && numdigits--)
   {
      x -= 9;
      V_DrawPatch(x, y, &vbscreen, invnums[num%10]);
      num /= 10;
   }
   
   if(neg)
      V_DrawPatch(x - 18, y, &vbscreen, invneg);
}

//
// ST_drawBackground
//
// Draws the basic status bar background
//
static void ST_drawBackground(void)
{
   // draw the background
   V_DrawPatch(0, 158, &vbscreen, barback);
   
   // patch the face eyes with the GOD graphics if the player
   // is in god mode
   if(plyr->cheats & CF_GODMODE)
   {
      V_DrawPatch(16,  167, &vbscreen, W_CacheLumpName("GOD1", PU_CACHE));
      V_DrawPatch(287, 167, &vbscreen, W_CacheLumpName("GOD2", PU_CACHE));
   }
   
   // draw the tops of the faces
   V_DrawPatch(0,   148, &vbscreen, ltfacetop);
   V_DrawPatch(290, 148, &vbscreen, rtfacetop);
}

//
// ST_shadowLine
//
// Can be used to form a horizontal shadowed line over part of 
// the screen via use of the normal light-fading colormaps.
// This version is for 320x200 resolution.
//
static void ST_shadowLine(int x, int y, int len,
                          int startmap, int mapstep)
{
   byte *dest, *colormap;
   int mapnum, i;

   // start at origin of line
   dest = screens[0] + y*SCREENWIDTH + x;

   // step in x direction for len pixels
   for(i = x, mapnum = startmap; i < x + len; ++i)
   {
      // get pointer to colormap
      colormap = colormaps[0] + mapnum*256;

      // remap the color currently on-screen
      *dest = colormap[*dest];

      ++dest;

      // move colormap level up or down after even-numbered pixels
      if(~i&1)
         mapnum += mapstep;
   }
}

//
// ST_shadowLineHi
//
// Can be used to form a horizontal shadowed line over part of 
// the screen via use of the normal light-fading colormaps.
// This version is for 640x400 resolution.
//
// ANYRES generalize for any resolution 
static void ST_shadowLineHi(int x, int y, int len,
                            int startmap, int mapstep)
{
   byte *dest, *colormap;
   int mapnum, p, realx, realy;

   // start at origin of line
   realx = realxarray[x];
   realy = realyarray[y];
   dest = screens[0] + realy * v_width + realx;
   len = realxarray[x + len] - realx;
   mapnum = startmap << FRACBITS;
   mapstep *= globaliyscale >> 1;

   // step in x direction for len scaled pixels
   for(x = realx; x < realx + len; ++x)
   {
      // get pointer to colormap
      colormap = colormaps[0] + (mapnum >> FRACBITS) * 256;
      mapnum += mapstep;

      // remap all four pixels in this scaled pixel's area
      dest = screens[0] + realy * v_width + x;
      for(p = globalyscale >> FRACBITS; p; p--)
      {
         *dest = colormap[*dest];
         dest += v_width;
      }
   }
}

//
// ST_chainShadow
//
// Draws two 16x10 shaded areas on the ends of the life chain, using
// the light-fading colormaps to darken what's already been drawn.
//
static void ST_chainShadow(void)
{
   int i;
   void (*linefunc)(int, int, int, int, int);

   // choose a drawing function depending on resolution, for
   // maximum speed
   // SoM: ANYRES
   if(globalyscale > FRACUNIT)
      linefunc = ST_shadowLineHi;
   else
      linefunc = ST_shadowLine;

   // draw 10 16-pixel shadow lines on each end of the life chain
   // by remapping the colors using odd colormap levels from 9 to
   // 23 (note the two regions fade in opposite directions).
   for(i = 0; i < 10; ++i)
   {
      linefunc(277, 190+i, 16,  9,  2); // right side (fades ->)
      linefunc(19,  190+i, 16, 23, -2); // left side  (fades <-)
   }
}

//
// ST_drawLifeChain
//
// Draws the chain & gem health indicator at the bottom
//
static void ST_drawLifeChain(void)
{
   int y = 191;
   int chainpos = chainhealth;
   
   // bound chainpos between 0 and 100
   if(chainpos < 0)
      chainpos = 0;
   if(chainpos > 100)
      chainpos = 100;
   
   // the total length between the left- and right-most gem
   // positions is 256 pixels, so scale the chainpos by that
   // amount (gem can range from 17 to 273)
   chainpos = (chainpos << 8) / 100;
      
   // jiggle y coordinate when chain is moving
   if(plyr->health != chainhealth)
      y += chainwiggle;

   // draw the chain -- links repeat every 17 pixels, so we
   // wrap the chain back to the starting position every 17
   V_DrawPatch(2 + (chainpos%17), y, &vbscreen, chain);
   
   // draw the gem (17 is the far left pos., 273 is max)
   V_DrawPatch(17 + chainpos, y, &vbscreen, lifegem);
   
   // draw face patches to cover over spare ends of chain
   V_DrawPatch(0,   190, &vbscreen, ltface);
   V_DrawPatch(276, 190, &vbscreen, rtface);
   
   // use the colormap to shadow the ends of the chain
   ST_chainShadow();
}

//
// ST_drawStatBar
//
// Draws the main status bar, shown when the inventory is not
// active.
//
static void ST_drawStatBar(void)
{
   int temp;

   V_DrawPatch(34, 160, &vbscreen, statbar);

   // TODO: inventory stuff

   // draw frags or health
   if(GameType == gt_dm)
   {
      ST_drawInvNum(plyr->totalfrags, 88, 170);
   }
   else
   {
      temp = chainhealth;

      if(temp < 0)
         temp = 0;

      ST_drawInvNum(temp, 88, 170);
   }

   // draw armor
   ST_drawInvNum(plyr->armorpoints, 255, 170);

   // draw key icons
   if(plyr->cards[it_yellowcard])
      V_DrawPatch(153, 164, &vbscreen, W_CacheLumpName("YKEYICON", PU_CACHE));
   if(plyr->cards[it_redcard])
      V_DrawPatch(153, 172, &vbscreen, W_CacheLumpName("GKEYICON", PU_CACHE));
   if(plyr->cards[it_bluecard])
      V_DrawPatch(153, 180, &vbscreen, W_CacheLumpName("BKEYICON", PU_CACHE));

   // TODO: ammo icon stuff
   // draw ammo amount
   temp = plyr->readyweapon;
   temp = weaponinfo[temp].ammo;
   if(temp < NUMAMMO)
   {
      temp = plyr->ammo[temp];
      ST_drawInvNum(temp, 136, 162);
   }
}

//
// ST_HticDrawer
//
// Draws the Heretic status bar
//
static void ST_HticDrawer(void)
{
   ST_drawBackground();
   ST_drawLifeChain();

   // TODO: choose whether to draw statbar or inventory bar here
   // based on whether the inventory is active
   ST_drawStatBar();
}

//
// ST_HticFSDrawer
//
// Draws the Heretic fullscreen hud/status information.
//
static void ST_HticFSDrawer(void)
{
}

//
// Status Bar Object for gameModeInfo
//
stbarfns_t HticStatusBar =
{
   ST_HBARHEIGHT,

   ST_HticTicker,
   ST_HticDrawer,
   ST_HticFSDrawer,
   ST_HticStart,
   ST_HticInit,
};

// EOF

