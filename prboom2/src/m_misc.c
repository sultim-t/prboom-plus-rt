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
 *  Main loop menu stuff.
 *  Default Config File.
 *  PCX Screenshots.
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
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

  /* cph 2002/08/10 - this used to return 0 on error, but that's ambiguous,
   * because we could have a legit 0-length file. So make it -1. */
  return -1;
}

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
  if (I_ScreenShot(fname) != 0)
    doom_printf("M_ScreenShot: Error writing screenshot");
}

void M_ScreenShot(void)
{
  static int shot;
  char       lbmname[32];
  int        startshot;
  boolean    screenshot_write_error = false;

  if (access(".",2)) screenshot_write_error = true;

  startshot = shot; // CPhipps - prevent infinite loop
    
  do
    sprintf(lbmname,"DOOM%02d.BMP", shot++);
  while (!access(lbmname,0) && (shot != startshot) && (shot < 10000));

  if (!access(lbmname,0)) screenshot_write_error = true;

  if (screenshot_write_error) {
    doom_printf ("M_ScreenShot: Couldn't create a BMP"); 
    // killough 4/18/98
    return;
  }

  M_DoScreenShot(lbmname); // cph
  S_StartSound(NULL,gamemode==commercial ? sfx_radio : sfx_tink); 
}
