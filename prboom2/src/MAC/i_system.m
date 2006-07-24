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
#include "i_system.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSString.h>
#import <Foundation/NSFileManager.h>

static NSString *libraryDir(void)
{
  return [@"~/Library/Application Support/PrBoom-Plus" stringByExpandingTildeInPath];
}

static char *NSStringToCString(NSString *str)
{
  char *cStr = malloc([str lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1);
  strcpy(cStr, [str UTF8String]);
  return cStr;
}

static char *macExeDir = 0;
char *I_DoomExeDir(void)
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

  const char *retval = 0;
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
