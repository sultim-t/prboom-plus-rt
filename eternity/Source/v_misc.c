// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Misc Video stuff.
//
// Font. Loading box. FPS ticker, etc
//
// TODO: The font code is a candidate for generalization.
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include "c_io.h"
#include "c_runcmd.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_video.h"
#include "v_video.h"
#include "v_misc.h"
#include "w_wad.h"
#include "d_gi.h"
#include "r_main.h" // haleyjd
#include "v_font.h"

extern int gamma_correct;

/////////////////////////////////////////////////////////////////////////////
//
// Console Video Mode Commands
//
// non platform-specific stuff is here in v_misc.c
// platform-specific stuff is in i_video.c
// videomode_t is platform specific although it must
// contain a member of type char* called description:
// see i_video.c for more info

int v_mode = 0;
static int prevmode = 0;

int v_width = SCREENWIDTH;
int v_height = SCREENHEIGHT;
fixed_t globalxscale = FRACUNIT, globalyscale = FRACUNIT;
fixed_t globalixscale = FRACUNIT, globaliyscale = FRACUNIT;
int realxarray[321];
int realyarray[201];

//
// V_NumModes
//
// Counts the number of video modes for the build platform.
// Used various places, including the -v_mode code and the
// menus.
//
int V_NumModes(void)
{
   int count = 0;
   
   while(videomodes[count].description)
      ++count;
   
   return count;
}

//
// V_ResetMode
//
// Called after changing video mode
//
void V_ResetMode(void)
{
   // check for invalid mode
   
   if(v_mode >= V_NumModes() || v_mode < 0)
   {
      C_Printf(FC_ERROR"invalid mode %i\n", v_mode);
      v_mode = prevmode;
      return;
   }
   
   prevmode = v_mode;
   
   I_SetMode(v_mode);
}

patch_t *bgp[9];        // background for boxes

///////////////////////////////////////////////////////////////////////////
//
// Font
//

// haleyjd 01/14/05: big font defines
#define B_FONTSTART '!'
#define B_FONTEND   'Z'
#define B_FONTSIZE  (B_FONTEND - B_FONTSTART + 1)

patch_t *v_font[V_FONTSIZE]; // still used externally
static patch_t *b_font[B_FONTSIZE];

vfont_t small_font =
{
   V_FONTSTART, // first character
   V_FONTEND,   // last character
   V_FONTSIZE,  // number of characters

   // these differ in the small font between game modes
   0,           // cy: set below
   0,           // space: set below
   0,           // dw: set below
   0,           // absh: set below

   true,        // color enabled
   true,        // caps only
   false,       // no centering

   v_font,      // patch array
};

// haleyjd 01/14/05: big font -- TODO: not enabled in DOOM yet
static vfont_t big_font =
{
   B_FONTSTART, // first character
   B_FONTEND,   // last character
   B_FONTSIZE,  // number of characters

   20,          // cy
   8,           // space
   1,           // dw
   20,          // absh -- FIXME

   false,       // color enabled
   true,        // caps only
   false,       // no centering

   b_font,      // patch array
};

// same as big font, but used for drawing numbers
vfont_t big_num_font =
{
   B_FONTSTART, // first character
   B_FONTEND,   // last character
   B_FONTSIZE,  // number of characters

   20,          // cy
   12,          // space
   0,           // dw
   20,          // absh -- FIXME

   false,       // color enabled
   true,        // caps only
   true,        // centering on

   b_font,      // patch array

   12,          // cw
};

//
// V_LoadFont
//
// Loads the general small font used almost everywhere.
// This font differs between DOOM and Heretic, and in Heretic
// it is quite a mess.
//
static void V_LoadFont(void)
{
   int i, j;
   char tempstr[9];

   // haleyjd 01/14/05: populate the vfont object with the proper
   // values for the current game mode.
   small_font.cy    = gameModeInfo->vtextinfo->cy;
   small_font.space = gameModeInfo->vtextinfo->space;
   small_font.dw    = gameModeInfo->vtextinfo->dw;
   small_font.absh  = gameModeInfo->vtextinfo->absh;

   // init to NULL first
   for(i = 0; i < V_FONTSIZE; ++i)
      v_font[i] = NULL;

   for(i = 0, j = V_FONTSTART; i < V_FONTSIZE; i++, j++)
   {
      if(j > 96 && j != 121 && j != 123 && j != 124 && j != 125)
         continue;

      // haleyjd 08/16/02: heretic font support
      if(gameModeInfo->type == Game_Heretic)
      {
         switch(j)
         {
         case 91:
            strcpy(tempstr, "FONTA00"); // bit of a hack here
            break;
         case 95:
            strcpy(tempstr, "FONTA59"); // this one is numbered wrong
            break;
         default:
            sprintf(tempstr, "FONTA%.2d", j - 32);
            break;
         }
      }
      else
         sprintf(tempstr, "STCFN%.3d", j);

      v_font[i] = W_CacheLumpName(tempstr, PU_STATIC);
   }
}

static void V_LoadBigFont(void)
{
   int i, j;
   char tempstr[9];
   
   // init to NULL first
   for(i = 0; i < B_FONTSIZE; i++)
      b_font[i] = NULL;

   // FONTB may not exist, check to make sure
   if(W_CheckNumForName("FONTB01") == -1)
      return;
   
   for(i = 0, j = B_FONTSTART; i < B_FONTSIZE; i++, j++)
   {      
      sprintf(tempstr, "FONTB%.2d", j - 32);
      b_font[i] = W_CacheLumpName(tempstr, PU_STATIC);
   }
}

//
// V_WriteText
//
// sf: write a text line to x, y
// haleyjd 01/14/05: now uses vfont engine
//
void V_WriteText(const char *s, int x, int y)
{
   V_FontWriteText(&small_font, s, x, y);
}

//
// V_WriteTextBig
//
// haleyjd 01/14/05: big font support
//
void V_WriteTextBig(const char *s, int x, int y)
{
   V_FontWriteText(&big_font, s, x, y);
}

//
// V_WriteNumTextBig
//
// Uses a big font object specialized for drawing numbers.
//
void V_WriteNumTextBig(const char *s, int x, int y)
{
   V_FontWriteText(&big_num_font, s, x, y);
}

//
// V_WriteTextColoured
//
// write text in a particular colour
// haleyjd 03/27/03: rewritten
// haleyjd 01/14/05: now uses vfont engine
//
void V_WriteTextColoured(const char *s, int colour, int x, int y)
{
   V_FontWriteTextColored(&small_font, s, colour, x, y);
}

//
// V_StringHeight
//
// find height(in pixels) of a string 
// haleyjd 01/14/05: now uses vfont engine
//
int V_StringHeight(const unsigned char *s)
{
   return V_FontStringHeight(&small_font, s);
}

//
// V_StringHeightBig
//
// haleyjd 01/14/05: big font support
//
int V_StringHeightBig(const unsigned char *s)
{
   return V_FontStringHeight(&big_font, s);
}

//
// V_StringWidth
//
// haleyjd 01/14/05: now uses vfont engine
//
int V_StringWidth(const unsigned char *s)
{
   return V_FontStringWidth(&small_font, s);
}

//
// V_StringWidthBig
//
// haleyjd 01/14/05: big font support
//
int V_StringWidthBig(const unsigned char *s)
{
   return V_FontStringWidth(&big_font, s);
}


////////////////////////////////////////////////////////////////////////////
//
// Box Drawing
//
// Originally from the Boom heads up code
//

#define FG 0

void V_DrawBox(int x, int y, int w, int h)
{
   int xs = bgp[0]->width;
   int ys = bgp[0]->height;
   int i,j;
   
   // top rows
   V_DrawPatchDirect(x, y, &vbscreen, bgp[0]);    // ul
   for(j = x+xs; j < x+w-xs; j += xs)     // uc
      V_DrawPatchDirect(j, y, &vbscreen, bgp[1]);
   V_DrawPatchDirect(j, y, &vbscreen, bgp[2]);    // ur
   
   // middle rows
   for(i = y+ys; i < y+h-ys; i += ys)
   {
      V_DrawPatchDirect(x, i, &vbscreen, bgp[3]);    // cl
      for(j = x+xs; j < x+w-xs; j += xs)     // cc
         V_DrawPatchDirect(j, i, &vbscreen, bgp[4]);
      V_DrawPatchDirect(j, i, &vbscreen, bgp[5]);    // cr
   }
   
   // bottom row
   V_DrawPatchDirect(x, i, &vbscreen, bgp[6]);    // ll
   for(j = x+xs; j < x+w-xs; j += xs)     // lc
      V_DrawPatchDirect(j, i, &vbscreen, bgp[7]);
   V_DrawPatchDirect(j, i, &vbscreen, bgp[8]);    // lr
}

void V_InitBox(void)
{
   bgp[0] = (patch_t *) W_CacheLumpName("BOXUL", PU_STATIC);
   bgp[1] = (patch_t *) W_CacheLumpName("BOXUC", PU_STATIC);
   bgp[2] = (patch_t *) W_CacheLumpName("BOXUR", PU_STATIC);
   bgp[3] = (patch_t *) W_CacheLumpName("BOXCL", PU_STATIC);
   bgp[4] = (patch_t *) W_CacheLumpName("BOXCC", PU_STATIC);
   bgp[5] = (patch_t *) W_CacheLumpName("BOXCR", PU_STATIC);
   bgp[6] = (patch_t *) W_CacheLumpName("BOXLL", PU_STATIC);
   bgp[7] = (patch_t *) W_CacheLumpName("BOXLC", PU_STATIC);
   bgp[8] = (patch_t *) W_CacheLumpName("BOXLR", PU_STATIC);
}

//////////////////////////////////////////////////////////////////////////
//
// "Loading" Box
//

static int loading_amount = 0;
static int loading_total = -1;
static char *loading_message;

// SoM: ANYRES
void V_DrawLoading(void)
{
   int x, y, realx, realy, reallinelen, reallineend;
   char *dest;
   int linelen;
   fixed_t yfrac, ystep;

   // haleyjd 11/30/02: get palette indices from gameModeInfo
   int white = gameModeInfo->whiteIndex;
   int black = gameModeInfo->blackIndex;

   if(!loading_message)
      return;
  
   V_DrawBox((SCREENWIDTH/2)-50, (SCREENHEIGHT/2)-30, 100, 40);
   
   V_WriteText(loading_message, (SCREENWIDTH/2)-30, 
               (SCREENHEIGHT/2)-20);
  
   x = ((SCREENWIDTH/2)-45);
   y = (SCREENHEIGHT/2);
   realx = realxarray[x];
   realy = realyarray[y];
   dest = screens[0] + (realy*v_width) + realx;
   linelen = (90*loading_amount) / loading_total;
   reallinelen = realxarray[linelen];
   reallineend = realxarray[90 - linelen];

   // white line
   memset(dest, white, reallinelen);
   // black line (unfilled)

   if((globalyscale >> FRACBITS) > 1)
   {
      yfrac = 0;
      ystep = globaliyscale;

      while(yfrac < FRACUNIT)
      {
         dest += v_width;
         memset(dest, white, reallinelen);
         memset(dest + reallinelen, black, reallineend);
         yfrac += ystep;
      }
   }

   I_FinishUpdate();
}

void V_SetLoading(int total, char *mess)
{
  loading_total = total ? total : 1;
  loading_amount = 0;
  loading_message = mess;

  if(in_textmode)
    {
      int i;
      printf(" %s ", mess);
      putchar('[');
      for(i=0; i<total; i++) putchar(' ');     // gap
      putchar(']');
      for(i=0; i<=total; i++) putchar('\b');    // backspace
    }
  else
    V_DrawLoading();
}

void V_LoadingIncrease(void)
{
  loading_amount++;
  if(in_textmode)
    {
      putchar('.');
      if(loading_amount == loading_total) putchar('\n');
    }
  else
    V_DrawLoading();

  if(loading_amount == loading_total) loading_message = NULL;
}

void V_LoadingSetTo(int amount)
{
  loading_amount = amount;
  if(!in_textmode) V_DrawLoading();
}

/////////////////////////////////////////////////////////////////////////////
//
// Framerate Ticker
//
// show dots at the bottom of the screen which represent
// an approximation to the current fps of doom.
// moved from i_video.c to make it a bit more
// system non-specific

// haleyjd 11/30/02: altered BLACK, WHITE defines to use gameModeInfo
#define BLACK (gameModeInfo->blackIndex)
#define WHITE (gameModeInfo->whiteIndex)
#define FPS_HISTORY 80
#define CHART_HEIGHT 40
#define X_OFFSET 20
#define Y_OFFSET 20

int v_ticker = 0;
static int history[FPS_HISTORY];
int current_count = 0;

void V_ClassicFPSDrawer(void);
void V_TextFPSDrawer(void);

void V_FPSDrawer(void)
{
   int i;
   int x,y;          // screen x,y
   int cx, cy;       // chart x,y
   
   if(v_ticker == 2)
   {
      V_ClassicFPSDrawer();
      return;
   }

   if(v_ticker == 3)
   {
      V_TextFPSDrawer();
      return;
   }
  
   current_count++;
 
   // render the chart
   for(cx=0, x = X_OFFSET; cx<FPS_HISTORY; x++, cx++)
   {
      for(cy=0, y = Y_OFFSET; cy<CHART_HEIGHT; y++, cy++)
      {
         i = cy > (CHART_HEIGHT-history[cx]) ? BLACK : WHITE;
         screens[0][y*v_width +x] = i; // ANYRES
      }
   }
}

void V_FPSTicker(void)
{
   static int lasttic;
   int thistic;
   int i;
   
   thistic = I_GetTime() / 7;
   
   if(lasttic != thistic)
   {
      lasttic = thistic;
      
      for(i = 0; i < FPS_HISTORY - 1; i++)
         history[i] = history[i+1];
      
      history[FPS_HISTORY-1] = current_count;
      current_count = 0;
   }
}

// sf: classic fps ticker kept seperate

void V_ClassicFPSDrawer(void)
{
  static int lasttic;
  byte *s = screens[0];
  
  int i = I_GetTime();
  int tics = i - lasttic;
  lasttic = i;
  if (tics > 20)
    tics = 20;

   // SoM: ANYRES
   if(globalyscale > FRACUNIT)
   {
      int baseoffset = (v_height - (globalyscale >> FRACBITS)) * v_width;
      int offset;
      int x, y, w, h;

      w = globalxscale >> FRACBITS;
      h = globalyscale >> FRACBITS;

      for (i=0 ; i < tics * 2 ; i += 2)
      {
         offset = baseoffset;
         y = h;
         x = (i * globalxscale) >> FRACBITS;
         while(y--)
         {
            memset(s + offset + x, 0xff, w);
            offset += v_width;
         }
      }
      for ( ; i < 20 * 2 ; i += 2)
      {
         offset = baseoffset;
         y = h;
         x = (i * globalxscale) >> FRACBITS;
         while(y--)
         {
            memset(s + offset + x, 0x0, w);
            offset += v_width;
         }
      }
   }
   else
   {
      for (i=0 ; i<tics*2 ; i+=2)
         s[(SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
      for ( ; i<20*2 ; i+=2)
         s[(SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
   }
}

void V_TextFPSDrawer(void)
{
   static char fpsStr[16];
   static int  fhistory[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
   static int  lasttic = 0;
   
   float fps = 0;
   int   i, thistic, totaltics = 0;
   
   thistic = I_GetTime();
   
   for(i = 0; i < 9; i++)
   {
      fhistory[i] = fhistory[i+1];
      totaltics += fhistory[i];
   }
   
   fhistory[9] = thistic - lasttic;
   
   if(fhistory[9] == 0)
      fhistory[9] = 1;
   
   totaltics += fhistory[9];
   
   fps = (float)totaltics;
   fps /= 10;
   
   if(fps)
   {
      fps = (float)TICRATE / fps;
   }
   
   psnprintf(fpsStr, sizeof(fpsStr), FC_GRAY "FPS: %.2f", fps);
   
   lasttic = thistic;
      
   V_WriteText(fpsStr, 5, 10);
}

// haleyjd: VBuffer stuff

VBuffer vbscreen;
VBuffer backscreen1;

//
// V_InitScreenVBuffer
//
static void V_InitScreenVBuffer(void)
{
   int drawtype;

   // haleyjd: set up VBuffer for the screen
   vbscreen.data   = screens[0];
   vbscreen.width  = v_width;
   vbscreen.height = v_height;
   vbscreen.pitch  = v_width; // TODO: fix to allow direct drawing!
   vbscreen.xlookup = realxarray;
   vbscreen.ylookup = realyarray;
   vbscreen.ixscale = globalixscale;
   vbscreen.iyscale = globaliyscale;
   
   if(v_width == 320 && v_height == 200)
      drawtype = DRAWTYPE_UNSCALED;
   else if(v_width == 640 && v_height == 400)
      drawtype = DRAWTYPE_2XSCALED;
   else
      drawtype = DRAWTYPE_GENSCALED;

   V_SetupBufferFuncs(&vbscreen, drawtype);

   // copy most attributes to the first backscreen

   memcpy(&backscreen1, &vbscreen, sizeof(VBuffer));
   backscreen1.data = screens[1];
   backscreen1.pitch = backscreen1.width;
}

//
// V_Init
//
// Allocates the 4 full screen buffers in low DOS memory
// No return value
//
void V_Init(void)
{
   static byte *s;
   
   int size = v_width * v_height;

   // SoM: ANYRES is ganna have to work in DJGPP too
   // haleyjd: why isn't this in i_video.c, anyways? oh well.
#ifdef DJGPP
   if(s)
      free(s), destroy_bitmap(screens0_bitmap);
#else
   if(s)
   {
      free(s);
      free(screens[0]);
   }
#endif
   
   screens[3] = (screens[2] = (screens[1] = s = calloc(size,3)) + size) + size;
   
#ifdef DJGPP
   screens0_bitmap = create_bitmap_ex(8, v_width, v_height);
   memset(screens[0] = screens0_bitmap->line[0], 0, size);
#else
   screens[0] = malloc(size);
   memset(screens[0], 0, size);
#endif

   R_SetupViewScaling();
   
   V_InitScreenVBuffer(); // haleyjd
}

//
// V_DrawBackground
//
// Tiles a 64 x 64 flat over the entirety of the provided VBuffer
// surface. Used by menus, intermissions, finales, etc.
//
void V_DrawBackground(char *patchname, VBuffer *back_dest)
{
   byte *src = W_CacheLumpNum(firstflat + R_FlatNumForName(patchname),
                              PU_CACHE);

   back_dest->TileBlock64(back_dest, src);
}

char *R_DistortedFlat(int);

//
// V_DrawDistortedBackground
//
// As above, but uses the ultra-cool water warping effect.
// Created by fraggle.
//
void V_DrawDistortedBackground(char* patchname, VBuffer *back_dest)
{
   byte *src = R_DistortedFlat(R_FlatNumForName(patchname));
   
   back_dest->TileBlock64(back_dest, src);
}

////////////////////////////////////////////////////////////////////////////
//
// Init
//

void V_InitMisc(void)
{
   V_LoadFont();
   V_LoadBigFont(); // haleyjd 01/14/05
   V_InitBox();
}

//////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

VARIABLE_INT(v_mode,   NULL, 0, 11, NULL);

char *str_ticker[] = { "off", "chart", "classic", "text" };
VARIABLE_INT(v_ticker, NULL, 0, 3,  str_ticker);

CONSOLE_VARIABLE(v_mode, v_mode, cf_buffered)
{
   V_ResetMode();
}

CONSOLE_COMMAND(v_modelist, 0)
{
   videomode_t* videomode = videomodes;
   
   C_Printf(FC_HI "video modes:\n");
   
   while(videomode->description)
   {
      C_Printf("%i: %s\n",(int)(videomode-videomodes),
               videomode->description);
      ++videomode;
   }
}

CONSOLE_VARIABLE(v_ticker, v_ticker, 0) {}

void V_AddCommands(void)
{
   C_AddCommand(v_mode);
   C_AddCommand(v_modelist);
   C_AddCommand(v_ticker);
}

// EOF
