/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_misc.c,v 1.43 2002/02/10 21:03:46 proff_fs Exp $
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
 *  Main loop menu stuff.
 *  Default Config File.
 *  PCX Screenshots.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_misc.c,v 1.43 2002/02/10 21:03:46 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _MSC_VER
#include <io.h>
#endif
#ifdef DREAMCAST
#define remove fs_unlink
#else
//#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include "doomstat.h"
#include "m_argv.h"
#include "g_game.h"
#include "mn_engin.h"
#include "am_map.h"
#include "w_wad.h"
#include "i_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "dstrings.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "lprintf.h"
#include "d_main.h"

/* cph - disk icon not implemented */
static inline void I_BeginRead(void) {}
static inline void I_EndRead(void) {}

/*
 * M_WriteFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

boolean M_WriteFile(char const *name, void *source, int length)
{
  FILE *fp;

  //errno = 0;
  
  if (!(fp = fopen(name, "wb")))       // Try opening file
    return 0;                          // Could not open file for writing

  I_BeginRead();                       // Disk icon on
  length = fwrite(source, 1, length, fp) == (size_t)length;   // Write data
  fclose(fp);
  I_EndRead();                         // Disk icon off

  if (!length)                         // Remove partially written file
    remove(name);

  return length;
}

/*
 * M_ReadFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

int M_ReadFile(char const *name, byte **buffer)
{
  FILE *fp;

  //errno = 0;

  if ((fp = fopen(name, "rb")))
    {
      size_t length;

      I_BeginRead();
      fseek(fp, 0, SEEK_END);
      length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      *buffer = malloc(length);
      if (fread(*buffer, 1, length, fp) == length)
        {
          fclose(fp);
          I_EndRead();
          return length;
        }
      fclose(fp);
    }

/*
  I_Error("Couldn't read file %s: %s", name, 
	  errno ? strerror(errno) : "(Unknown Error)");
*/
  return 0;
}

//
// SCREEN SHOTS
//

// CPhipps - nasty but better than nothing
static boolean screenshot_write_error;

// jff 3/30/98 types and data structures for BMP output of screenshots
//
// killough 5/2/98:
// Changed type names to avoid conflicts with endianess functions

#define BI_RGB 0L

typedef unsigned long dword_t;
typedef long     long_t;
typedef unsigned char ubyte_t;

#ifdef _MSC_VER
#define GCC_PACKED
#else //_MSC_VER
/* we might want to make this sensitive to whether configure has detected
   a build with GCC */
#define GCC_PACKED __attribute__((packed))
#endif //_MSC_VER

#ifdef _MSC_VER // proff: This is the same as __attribute__ ((packed)) in GNUC
#pragma pack(push)
#pragma pack(1)
#endif //_MSC_VER

#if defined(__MWERKS__)
#pragma options align=packed
#endif

typedef struct tagBITMAPFILEHEADER
  {
  unsigned short  bfType GCC_PACKED;
  dword_t bfSize GCC_PACKED;
  unsigned short  bfReserved1 GCC_PACKED;
  unsigned short  bfReserved2 GCC_PACKED;
  dword_t bfOffBits GCC_PACKED;
  } GCC_PACKED BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
  {
  dword_t biSize GCC_PACKED;
  long_t  biWidth GCC_PACKED;
  long_t  biHeight GCC_PACKED;
  unsigned short  biPlanes GCC_PACKED;
  unsigned short  biBitCount GCC_PACKED;
  dword_t biCompression GCC_PACKED;
  dword_t biSizeImage GCC_PACKED;
  long_t  biXPelsPerMeter GCC_PACKED;
  long_t  biYPelsPerMeter GCC_PACKED;
  dword_t biClrUsed GCC_PACKED;
  dword_t biClrImportant GCC_PACKED;
  } GCC_PACKED BITMAPINFOHEADER;

#if defined(__MWERKS__)
#pragma options align=reset
#endif
  
#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER

// jff 3/30/98 binary file write with error detection
// CPhipps - static, const on parameter
static void SafeWrite(const void *data, size_t size, size_t number, FILE *st)
{
  if (fwrite(data,size,number,st)<number)
    screenshot_write_error = true; // CPhipps - made non-fatal
}

#ifndef GL_DOOM
//
// WriteBMPfile
// jff 3/30/98 Add capability to write a .BMP file (256 color uncompressed)
//

// CPhipps - static, const on parameters
static void WriteBMPfile(const char* filename, const byte* data, 
			 const int width, const int height, const byte* palette)
{
  int i,wid;
  BITMAPFILEHEADER bmfh;
  BITMAPINFOHEADER bmih;
  int fhsiz,ihsiz;
  FILE *st;
  char zero=0;
  ubyte_t c;

  fhsiz = sizeof(BITMAPFILEHEADER);
  ihsiz = sizeof(BITMAPINFOHEADER);
  wid = 4*((width+3)/4);
  //jff 4/22/98 add endian macros
  bmfh.bfType = SHORT(19778);
  bmfh.bfSize = LONG(fhsiz+ihsiz+256L*4+width*height);
  bmfh.bfReserved1 = SHORT(0);
  bmfh.bfReserved2 = SHORT(0);
  bmfh.bfOffBits = LONG(fhsiz+ihsiz+256L*4);

  bmih.biSize = LONG(ihsiz);
  bmih.biWidth = LONG(width);
  bmih.biHeight = LONG(height);
  bmih.biPlanes = SHORT(1);
  bmih.biBitCount = SHORT(8);
  bmih.biCompression = LONG(BI_RGB);
  bmih.biSizeImage = LONG(wid*height);
  bmih.biXPelsPerMeter = LONG(0);
  bmih.biYPelsPerMeter = LONG(0);
  bmih.biClrUsed = LONG(256);
  bmih.biClrImportant = LONG(256);

  st = fopen(filename,"wb");
  if (st!=NULL) {
    int gtlump = (W_CheckNumForName)("GAMMATBL",ns_prboom);
    register const byte * const gtable = (const byte *)W_CacheLumpNum(gtlump) + 256*usegamma;

    // write the header
    SafeWrite(&bmfh.bfType,sizeof(bmfh.bfType),1,st);
    SafeWrite(&bmfh.bfSize,sizeof(bmfh.bfSize),1,st);
    SafeWrite(&bmfh.bfReserved1,sizeof(bmfh.bfReserved1),1,st);
    SafeWrite(&bmfh.bfReserved2,sizeof(bmfh.bfReserved2),1,st);
    SafeWrite(&bmfh.bfOffBits,sizeof(bmfh.bfOffBits),1,st);

    SafeWrite(&bmih.biSize,sizeof(bmih.biSize),1,st);
    SafeWrite(&bmih.biWidth,sizeof(bmih.biWidth),1,st);
    SafeWrite(&bmih.biHeight,sizeof(bmih.biHeight),1,st);
    SafeWrite(&bmih.biPlanes,sizeof(bmih.biPlanes),1,st);
    SafeWrite(&bmih.biBitCount,sizeof(bmih.biBitCount),1,st);
    SafeWrite(&bmih.biCompression,sizeof(bmih.biCompression),1,st);
    SafeWrite(&bmih.biSizeImage,sizeof(bmih.biSizeImage),1,st);
    SafeWrite(&bmih.biXPelsPerMeter,sizeof(bmih.biXPelsPerMeter),1,st);
    SafeWrite(&bmih.biYPelsPerMeter,sizeof(bmih.biYPelsPerMeter),1,st);
    SafeWrite(&bmih.biClrUsed,sizeof(bmih.biClrUsed),1,st);
    SafeWrite(&bmih.biClrImportant,sizeof(bmih.biClrImportant),1,st);

    // write the palette, in blue-green-red order, gamma corrected
    for (i=0;i<768;i+=3) {
      c=gtable[palette[i+2]];
      SafeWrite(&c,sizeof(char),1,st);
      c=gtable[palette[i+1]];
      SafeWrite(&c,sizeof(char),1,st);
      c=gtable[palette[i+0]];
      SafeWrite(&c,sizeof(char),1,st);
      SafeWrite(&zero,sizeof(char),1,st);
    }

    for (i = 0 ; i < height ; i++)
      SafeWrite(data+(height-1-i)*width,sizeof(byte),wid,st);

    fclose(st);
    W_UnlockLumpNum(gtlump);
  }
}

#else /* GL_DOOM */

//
// WriteTGAfile
// proff 05/15/2000 Add capability to write a .TGA file (24bit color uncompressed)
//

// CPhipps - static, const on parameters
static void WriteTGAfile(const char* filename, const byte* data, 
			 const int width, const int height)
{
  unsigned char c;
  unsigned short s;
  int i;
  FILE *st;

  st = fopen(filename,"wb");
  if (st!=NULL) {
    // write the header
    // id_length
    c=0; SafeWrite(&c,sizeof(c),1,st);
    // colormap_type
    c=0; SafeWrite(&c,sizeof(c),1,st);
    // image_type
    c=2; SafeWrite(&c,sizeof(c),1,st);
    // colormap_index
    s=0; SafeWrite(&s,sizeof(s),1,st);
    // colormap_length
    s=0; SafeWrite(&s,sizeof(s),1,st);
    // colormap_size
    c=0; SafeWrite(&c,sizeof(c),1,st);
    // x_origin
    s=0; SafeWrite(&s,sizeof(s),1,st);
    // y_origin
    s=0; SafeWrite(&s,sizeof(s),1,st);
    // width
    s=SHORT(width); SafeWrite(&s,sizeof(s),1,st);
    // height
    s=SHORT(height); SafeWrite(&s,sizeof(s),1,st);
    // bits_per_pixel
    c=24; SafeWrite(&c,sizeof(c),1,st);
    // attributes
    c=0; SafeWrite(&c,sizeof(c),1,st);

    for (i=0; i<width*height*3; i+=3)
    {
      SafeWrite(&data[i+2],sizeof(byte),1,st);
      SafeWrite(&data[i+1],sizeof(byte),1,st);
      SafeWrite(&data[i+0],sizeof(byte),1,st);
    }

    fclose(st);
  }
}
#endif /* GL_DOOM */

//
// M_ScreenShot
//
// Modified by Lee Killough so that any number of shots can be taken,
// the code is faster, and no annoying "screenshot" message appears.

// CPhipps - modified to use its own buffer for the image
//         - checks for the case where no file can be created (doesn't occur on POSIX systems, would on DOS)
//         - track errors better
//         - split into 2 functions

//
// M_DoScreenShot
// Takes a screenshot into the names file

void M_DoScreenShot (const char* fname)
{
  byte       *linear;
#ifndef GL_DOOM
  const byte *pal;
  int        pplump = W_GetNumForName("PLAYPAL");
#endif

  screenshot_write_error = false;

#ifdef GL_DOOM
  // munge planar buffer to linear
  // CPhipps - use a malloc()ed buffer instead of screens[2]
  gld_ReadScreen(linear = malloc(SCREENWIDTH * SCREENHEIGHT * 3));

  // save the bmp file

  WriteTGAfile
    (fname, linear, SCREENWIDTH, SCREENHEIGHT);
#else
  // munge planar buffer to linear
  // CPhipps - use a malloc()ed buffer instead of screens[2]
  I_ReadScreen(linear = malloc(SCREENWIDTH * SCREENHEIGHT));

  // killough 4/18/98: make palette stay around (PU_CACHE could cause crash)
  pal = W_CacheLumpNum (pplump);
    
  // save the bmp file

  WriteBMPfile
    (fname, linear, SCREENWIDTH, SCREENHEIGHT, pal);

  // cph - free the palette
  W_UnlockLumpNum(pplump);
#endif
  free(linear);
  // 1/18/98 killough: replace "SCREEN SHOT" acknowledgement with sfx

  if (screenshot_write_error)
    doom_printf("M_ScreenShot: Error writing screenshot");
}

void M_ScreenShot(void)
{
  static int shot;
  char       lbmname[32];
  int        startshot;
  
  screenshot_write_error = false;

  if (access(".",2)) screenshot_write_error = true;

  startshot = shot; // CPhipps - prevent infinite loop
    
  do
#ifdef GL_DOOM
    sprintf(lbmname,"DOOM%02d.TGA", shot++);
#else // GL_DOOM
    sprintf(lbmname,"DOOM%02d.BMP", shot++);
#endif // GL_DOOM
  while (!access(lbmname,0) && (shot != startshot) && (shot < 10000));

  if (!access(lbmname,0)) screenshot_write_error = true;

  if (screenshot_write_error) {
#ifdef GL_DOOM
    doom_printf ("M_ScreenShot: Couldn't create a TGA"); 
#else // GL_DOOM
    doom_printf ("M_ScreenShot: Couldn't create a BMP"); 
#endif // GL_DOOM
    // killough 4/18/98
    return;
  }

  M_DoScreenShot(lbmname); // cph
  S_StartSound(NULL,gamemode==commercial ? sfx_radio : sfx_tink); 
}
