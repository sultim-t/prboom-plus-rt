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
 *  Copyright (C) 2006) by Neil Stevens
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
 *  Misc system stuff needed by Doom, implemented for Linux.
 *  Mainly timer handling, and ENDOOM/ENDBOOM.
 *
 *-----------------------------------------------------------------------------
 */

/* Mac path finding and WAD selection UI */

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "dstrings.h"
#include "d_main.h"
#include "m_fixed.h"
#include "i_system.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSString.h>
#import <Foundation/NSFileManager.h>

static NSString *libraryDir(void)
{
  return [@"~/Library/Application Support/PrBoom" stringByExpandingTildeInPath];
}

static char *NSStringToCString(NSString *str)
{
  char *cStr = malloc([str lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1);
  strcpy(cStr, [str UTF8String]);
  return cStr;
}

static char *macExeDir = 0;
const char *I_DoomExeDir(void)
{
  if(macExeDir)
    return macExeDir;

  NSString *exeDir = libraryDir();
  [[NSFileManager defaultManager] createDirectoryAtPath:exeDir attributes:nil];

  macExeDir = NSStringToCString(exeDir);
  return macExeDir;
}

char *I_FindFile(const char *wf_name, const char *ext)
{
  NSArray *paths = [NSArray arrayWithObject:libraryDir()];
  paths = [paths arrayByAddingObject: [[NSBundle mainBundle] resourcePath]];
  paths = [paths arrayByAddingObject: @""];

  char *retval = 0;
  int i;
  for(i = 0; !retval && (i < [paths count]); ++i)
  {
    NSString *path = [NSString stringWithFormat:@"%@/%@",
                      [paths objectAtIndex:i],
                      [NSString stringWithUTF8String:wf_name]];
    if([[NSFileManager defaultManager] isReadableFileAtPath:path])
      retval = NSStringToCString(path);
  }

  return retval;
}
