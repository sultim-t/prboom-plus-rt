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

int V_NumModes(void)
{
   int count=0;
   
   while(videomodes[count].description)
      count++;
   
   return count;
}

// v_resetmode is called after changing vid mode

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

///////////////////////////////////////////////////////////////////////////
//
// Font
//

patch_t* v_font[V_FONTSIZE];
patch_t *bgp[9];        // background for boxes

void V_LoadFont(void)
{
   int i, j;
   char tempstr[10];

   // init to NULL first
   for(i = 0; i < V_FONTSIZE; i++)
      v_font[i] = NULL;

   for(i = 0, j = V_FONTSTART; i < V_FONTSIZE; i++, j++)
   {
      if(j > 96 && j != 121 && j != 123 && j != 124 && j != 125)
         continue;

      // haleyjd 08/16/02: heretic font support
      if(gameModeInfo->flags & GIF_HERETIC)
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
      {
         sprintf(tempstr, "STCFN%.3d", j);
      }

      v_font[i] = W_CacheLumpName(tempstr, PU_STATIC);
   }
}

static boolean fixedColor = false;
static int fixedColNum = 0;

 // sf: write a text line to x, y
void V_WriteText(const char *s, int x, int y)
{
   int   w, h;
   const unsigned char *ch;
   char *colour;
   unsigned int c;
   boolean translucent = false; // haleyjd: trans. text support
   int   cx;
   int   cy;
   patch_t *patch;

   // haleyjd: get v-font metrics
   gitextmetric_t *fontmetrics = gameModeInfo->vtextinfo;

   if(fixedColor)
   {
      // haleyjd 03/27/03: use fixedColor if it was set by 
      // another routine
      colour = colrngs[fixedColNum];
      fixedColor = false;
   }
   else
   {
      // haleyjd: get default text color from gamemode info
      colour = *(gameModeInfo->defTextTrans); // pointer to pointer!
   }
   
   ch = (const unsigned char *)s;
   cx = x;
   cy = y;
   
   while(1)
   {
      c = *ch++;
      if(!c)
         break;
      if(c >= 128)     // new colour
      {
         if(c == 138)
         {
            translucent = !translucent;
         }
         else
         {
            int colnum;
            
            // haleyjd: allow use of gamemode-dependent defaults
            switch(c)
            {
            case 139:
               colnum = gameModeInfo->colorNormal;
               break;
            case 140:
               colnum = gameModeInfo->colorHigh;
               break;
            case 141:
               colnum = gameModeInfo->colorError;
               break;
            default:
               colnum = c - 128;
               break;
            }
            
            // haleyjd: added error checking from SMMU v3.30
            if(colnum < 0 || colnum >= CR_LIMIT)
               I_Error("V_WriteText: invalid colour %i\n", colnum);
            else
               colour = colrngs[colnum];
         }
         continue;
      }
      if(c == '\t')
      {
         cx = (cx/40)+1;
         cx = cx*40;
         continue; // haleyjd: missing continue for \t
      }
      if(c == '\n')
      {
         cx = x;
         cy += fontmetrics->cy;
         continue;
      }
      
      c = toupper(c) - V_FONTSTART;
      if(c < 0 || c >= V_FONTSIZE)
      {
         cx += fontmetrics->space;
         continue;
      }
      
      patch = v_font[c];
      
      // haleyjd: need to add 4 to cx for NULL characters
      // or else V_StringWidth is lying about their length
      if(!patch)
      {
         cx += fontmetrics->space;
         continue;
      }
      
      // haleyjd: was no cx<0 check
      
      w = SHORT (patch->width);
      if(cx < 0 || cx+w > SCREENWIDTH)
         break;
      
      // haleyjd: was no y checking at all!
      
      h = SHORT(patch->height);
      if(cy < 0 || cy+h > SCREENHEIGHT)
         break;
      
      if(translucent)
         V_DrawPatchTL(cx, cy, &vbscreen, patch, colour, 32768);
      else
         V_DrawPatchTranslated(cx, cy, &vbscreen, patch, colour, 0);
      
      cx += (w - fontmetrics->dw);
   }
}

// write text in a particular colour
// haleyjd 03/27/03: rewritten
void V_WriteTextColoured(const char *s, int colour, int x, int y)
{
   if(colour < 0 || colour >= CR_LIMIT)
      I_Error("V_WriteTextColoured: invalid colour %i\n", colour);

   fixedColor  = true;
   fixedColNum = colour;

   V_WriteText(s, x, y);
}

// find height(in pixels) of a string 

int V_StringHeight(const unsigned char *s)
{
   int height;
   gitextmetric_t *fontmetrics = gameModeInfo->vtextinfo;
   
   height = fontmetrics->cy;  // always at least cy 
   
   // add an extra cy for each newline found
   
   while(*s)
   {
      if(*s == '\n')
         height += fontmetrics->cy;
      s++;
   }
   
   return height;
}

int V_StringWidth(const unsigned char *s)
{
   int length = 0; // current line width
   int longest_width = 0; // line with longest width so far
   unsigned char c;
   gitextmetric_t *fontmetrics = gameModeInfo->vtextinfo;
   
   for(; *s; s++)
   {
      c = *s;
      if(c >= 128)         // colour
         continue;
      if(c == '\n')        // newline
      {
         if(length > longest_width) 
            longest_width = length;
         length = 0; // next line;
         continue;	  
      }
      c = toupper(c) - V_FONTSTART;

      if(c >= V_FONTSIZE || !v_font[c])
         length += fontmetrics->space;
      else
         length += (SHORT(v_font[c]->width) - fontmetrics->dw);
   }
   
   if(length > longest_width)
      longest_width = length; // check last line
   
   return longest_width;
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
  for (j = x+xs; j < x+w-xs; j += xs)     // uc
    V_DrawPatchDirect(j, y, &vbscreen, bgp[1]);
  V_DrawPatchDirect(j, y, &vbscreen, bgp[2]);    // ur

  // middle rows
  for (i=y+ys;i<y+h-ys;i+=ys)
    {
      V_DrawPatchDirect(x, i, &vbscreen, bgp[3]);    // cl
      for (j = x+xs; j < x+w-xs; j += xs)     // cc
        V_DrawPatchDirect(j, i, &vbscreen, bgp[4]);
      V_DrawPatchDirect(j, i, &vbscreen, bgp[5]);    // cr
    }

  // bottom row
  V_DrawPatchDirect(x, i, &vbscreen, bgp[6]);    // ll
  for (j = x+xs; j < x+w-xs; j += xs)     // lc
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
#ifdef DJGPP
  if (s)
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

void V_DrawBackground(char *patchname, VBuffer *back_dest)
{
   byte *src = W_CacheLumpNum(firstflat + R_FlatNumForName(patchname),
                              PU_CACHE);

   back_dest->TileBlock64(back_dest, src);
}

// sf:

char *R_DistortedFlat(int);

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
  V_InitBox();
}

//////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

VARIABLE_INT(v_mode, NULL,              0, 11, NULL);

char *str_ticker[]={"off", "chart", "classic", "text"};
VARIABLE_INT(v_ticker, NULL,            0, 3, str_ticker);

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
      videomode++;
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
