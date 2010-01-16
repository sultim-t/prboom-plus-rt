/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *      Demo stuff
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>

#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "r_demo.h"
#include "r_fps.h"
#include "lprintf.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "m_argv.h"
#include "w_wad.h"
#include "d_main.h"
#include "d_deh.h"
#include "g_game.h"
#include "p_map.h"
#include "hu_stuff.h"
#include "g_overflow.h"
#include "e6y.h"

int IsDemoPlayback(void)
{
  int p;

  if ((p = M_CheckParm("-playdemo")) && (p < myargc - 1))
    return p;
  if ((p = M_CheckParm("-timedemo")) && (p < myargc - 1))
    return p;
  if ((p = M_CheckParm("-fastdemo")) && (p < myargc - 1))
    return p;

  return 0;
}

int IsDemoContinue(void)
{
  int p;

  if ((p = M_CheckParm("-recordfromto")) && (p < myargc - 2) &&
    I_FindFile(myargv[p + 1], ".lmp"))
  {
    return p;
  }

  return 0;
}

//
// Smooth playing stuff
//

int demo_smoothturns = false;
int demo_smoothturnsfactor = 6;

static int smooth_playing_turns[SMOOTH_PLAYING_MAXFACTOR];
static int_64_t smooth_playing_sum;
static int smooth_playing_index;
static angle_t smooth_playing_angle;

void R_SmoothPlaying_Reset(player_t *player)
{
  if (demo_smoothturns && demoplayback)
  {
    if (!player)
      player = &players[displayplayer];

    if (player==&players[displayplayer])
    {
      if (players[displayplayer].mo)
      {
        smooth_playing_angle = players[displayplayer].mo->angle;
        memset(smooth_playing_turns, 0, sizeof(smooth_playing_turns[0]) * SMOOTH_PLAYING_MAXFACTOR);
        smooth_playing_sum = 0;
        smooth_playing_index = 0;
      }
    }
  }
}

void R_SmoothPlaying_Add(int delta)
{
  if (demo_smoothturns && demoplayback)
  {
    smooth_playing_sum -= smooth_playing_turns[smooth_playing_index];
    smooth_playing_turns[smooth_playing_index] = delta;
    smooth_playing_index = (smooth_playing_index + 1)%(demo_smoothturnsfactor);
    smooth_playing_sum += delta;
    smooth_playing_angle += (int)(smooth_playing_sum/(demo_smoothturnsfactor));
  }
}

angle_t R_SmoothPlaying_Get(angle_t defangle)
{
  if (demo_smoothturns && demoplayback)
    return smooth_playing_angle;
  else
    return defangle;
}

void R_ResetAfterTeleport(player_t *player)
{
  R_ResetViewInterpolation();
  R_SmoothPlaying_Reset(player);
}

//
// DemoEx stuff
//

#ifndef _MSC_VER
#ifdef HAVE_LIBPCREPOSIX
#include "pcreposix.h"
#endif
#else // _MSC_VER
#define HAVE_LIBPCREPOSIX 1
#define PCRE_STATIC 1
#include "pcreposix.h"
#ifdef _DEBUG
#pragma comment( lib, "pcred.lib" )
#pragma comment( lib, "pcreposixd.lib" )
#else
#pragma comment( lib, "pcre.lib" )
#pragma comment( lib, "pcreposix.lib" )
#endif
#endif

#define PWAD_SIGNATURE "PWAD"
#define DEMOEX_VERSION "2"

#define DEMOEX_VERSION_LUMPNAME "VERSION"
#define DEMOEX_PORTNAME_LUMPNAME "PORTNAME"
#define DEMOEX_PARAMS_LUMPNAME "CMDLINE"
#define DEMOEX_MLOOK_LUMPNAME "MLOOK"
#define DEMOEX_COMMENT_LUMPNAME "COMMENT"

#define DEMOEX_SEPARATOR       "\n"

// patterns
int demo_patterns_count;
char *demo_patterns_mask;
char **demo_patterns_list;
char *demo_patterns_list_def[9];

// demo ex
int demo_extendedformat = -1;
int demo_extendedformat_default;
dboolean use_demoex_info = false;

char demoex_filename[PATH_MAX];
char *demo_demoex_filename;
//wadtbl_t demoex;

typedef struct
{
  const char name[9];
  short *data;
  int lump;
  size_t maxtick;
  size_t tick;
} mlooklump_t;

mlooklump_t mlook_lump = {DEMOEX_MLOOK_LUMPNAME, NULL, -2, 0, 0};

int AddString(char **str, const char *val);

static void R_DemoEx_AddParams(wadtbl_t *wadtbl);
static int R_DemoEx_GetVersion(void);
static void R_DemoEx_GetParams(const byte *pwad_p, waddata_t *waddata);
static void R_DemoEx_AddMouseLookData(wadtbl_t *wadtbl);

static int G_ReadDemoFooter(const char *filename);

int AddString(char **str, const char *val)
{
  int size = 0;

  if (!str || !val)
    return 0;

  if (*str)
  {
    size = strlen(*str) + strlen(val) + 1;
    *str = realloc(*str, size);
    strcat(*str, val);
  }
  else
  {
    size = strlen(val) + 1;
    *str = malloc(size);
    strcpy(*str, val);
  }

  return size;
}

void M_ChangeDemoExtendedFormat(void)
{
  if (demo_extendedformat == -1)
  {
    demo_extendedformat = demo_extendedformat_default;
  }

  use_demoex_info = demo_extendedformat || M_CheckParm("-auto");
}

void W_InitPWADTable(wadtbl_t *wadtbl)
{
  //init header signature and lookup table offset and size
  strncpy(wadtbl->header.identification, PWAD_SIGNATURE, 4);
  wadtbl->header.infotableofs = sizeof(wadtbl->header);
  wadtbl->header.numlumps = 0;

  //clear PWAD lookup table
  wadtbl->lumps = NULL;
  
  //clear PWAD data
  wadtbl->data = NULL;
  wadtbl->datasize = 0;
}

void W_FreePWADTable(wadtbl_t *wadtbl)
{
  //clear PWAD lookup table
  free(wadtbl->lumps);
  
  //clear PWAD data
  free(wadtbl->data);
}

wadtbl_t* W_CreatePWADTable(const byte* buffer, size_t size)
{
  int i;
  char *pwad_p;
  wadinfo_t *header_p;
  filelump_t* filelump_p;
  wadtbl_t *wadtbl;

  if (!buffer)
    return NULL;

  wadtbl = calloc(sizeof(*wadtbl), 1);

  W_InitPWADTable(wadtbl);

  pwad_p = (char*)buffer;
  header_p = (wadinfo_t*)pwad_p;

  if (strncmp(header_p->identification, PWAD_SIGNATURE, 4))
    I_Error("W_CreatePWADTable: Wad buffer doesn't have PWAD id");

  header_p->numlumps = LittleLong(header_p->numlumps);
  header_p->infotableofs = LittleLong(header_p->infotableofs);

  filelump_p = (filelump_t*)((char*)buffer + header_p->infotableofs);
  for (i = 0; i < header_p->numlumps; i++, filelump_p++)
  {
    char lumpname[9];
    strncpy(lumpname, filelump_p->name, 8);
    lumpname[8] = 0;
    W_AddLump(wadtbl, lumpname, (const byte*)(pwad_p + filelump_p->filepos), filelump_p->size);
  }

  return wadtbl;
}

void W_AddLump(wadtbl_t *wadtbl, const char *name, const byte* data, size_t size)
{
  int lumpnum;

  if (wadtbl && (name && strlen(name) > 8))
  {
    I_Error("W_AddLump: wrong parameters.");
    return;
  }

  lumpnum = wadtbl->header.numlumps;
  
  if (name)
  {
    wadtbl->lumps = realloc(wadtbl->lumps, (lumpnum + 1) * sizeof(wadtbl->lumps[0]));

    strncpy(wadtbl->lumps[lumpnum].name, name, 8);
    wadtbl->lumps[lumpnum].size = size;
    wadtbl->lumps[lumpnum].filepos = wadtbl->header.infotableofs;

    wadtbl->header.numlumps++;
  }

  if (data && size > 0)
  {
    wadtbl->data = realloc(wadtbl->data, wadtbl->datasize + size);

    memcpy(wadtbl->data + wadtbl->datasize, data, size);
    wadtbl->datasize += size;

    wadtbl->header.infotableofs += size;
  }
}

void R_DemoEx_ShowComment(void)
{
  extern patchnum_t hu_font[];

  int         lump;
  int         cx = 10;
  int         cy = 10;
  const char* ch;
  int         count;
  int         w;

  if (!use_demoex_info)
    return;

  lump = W_CheckNumForName(DEMOEX_COMMENT_LUMPNAME);
  if (lump == -1)
    return;

  count = W_LumpLength(lump);

  if (count <= 0)
    return;

  ch = W_CacheLumpNum(lump);

  for ( ; count ; count-- )
  {
    int c = *ch++;

    if (!c)
      break;
    if (c == '\n')
    {
      cx = 10;
      cy += 11;
      continue;
    }

    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }

    w = hu_font[c].width;
    if (cx + w > SCREENWIDTH)
      break;

    V_DrawNumPatch(cx, cy, 0, hu_font[c].lumpnum, CR_DEFAULT, VPT_STRETCH);
    cx += w;
  }

  W_UnlockLumpNum(lump);
}

angle_t R_DemoEx_ReadMLook(void)
{
  angle_t pitch;

  if (!use_demoex_info || !(demoplayback || democontinue))
    return 0;

  // mlook data must be initialised here
  if ((mlook_lump.lump == -2))
  {
    if (R_DemoEx_GetVersion() < 2)
    {
      // unsupported format
      mlook_lump.lump = -1;
    }
    else
    {
      mlook_lump.lump = W_CheckNumForName(mlook_lump.name);
      if (mlook_lump.lump != -1)
      {
        const unsigned char *data = W_CacheLumpName(mlook_lump.name);
        int size = W_LumpLength(mlook_lump.lump);

        mlook_lump.maxtick = size / sizeof(mlook_lump.data[0]);
        mlook_lump.data = malloc(size);
        memcpy(mlook_lump.data, data, size);
      }
    }
  }

  pitch = 0;
  if (mlook_lump.data && mlook_lump.tick < mlook_lump.maxtick &&
    consoleplayer == displayplayer && !walkcamera.type)
  {
    pitch = mlook_lump.data[mlook_lump.tick];
  }
  mlook_lump.tick++;

  return (pitch << 16);
}

void R_DemoEx_WriteMLook(angle_t pitch)
{
  if (!use_demoex_info || !demorecording)
    return;

  if (mlook_lump.tick >= mlook_lump.maxtick)
  {
    int ticks = mlook_lump.maxtick;
    mlook_lump.maxtick = (mlook_lump.maxtick ? mlook_lump.maxtick * 2 : 8192);
    if (mlook_lump.tick >= mlook_lump.maxtick)
      mlook_lump.maxtick = mlook_lump.tick * 2; 
    mlook_lump.data = realloc(mlook_lump.data, mlook_lump.maxtick * sizeof(mlook_lump.data[0]));
    memset(mlook_lump.data + ticks, 0, (mlook_lump.maxtick - ticks) * sizeof(mlook_lump.data[0]));
  }

  mlook_lump.data[mlook_lump.tick] = (short)(pitch >> 16);
  mlook_lump.tick++;
}

static int R_DemoEx_GetVersion(void)
{
  int result = -1;

  int lump, ver;
  unsigned int size;
  const char *data;
  char str_ver[32];

  lump = W_CheckNumForName(DEMOEX_VERSION_LUMPNAME);
  if (lump != -1)
  {
    size = W_LumpLength(lump);
    if (size > 0)
    {
      data = W_CacheLumpNum(lump);
      strncpy(str_ver, data, MIN(size, sizeof(str_ver)));

      if (sscanf(str_ver, "%d", &ver) == 1)
      {
        result = ver;
      }
    }
    W_UnlockLumpNum(lump);
  }

  return result;
}

static void R_DemoEx_GetParams(const byte *pwad_p, waddata_t *waddata)
{
  int lump;
  size_t size;
  char *str;
  const char *data;
  char **params;
  int i, p, paramscount;
  
  lump = W_CheckNumForName(DEMOEX_PARAMS_LUMPNAME);
  if (lump == -1)
    return;

  size = W_LumpLength(lump);
  if (size <= 0)
    return;

  str = calloc(size + 1, 1);
  if (!str)
    return;

  data = W_CacheLumpNum(lump);
  strncpy(str, data, size);

  M_ParseCmdLine(str, NULL, NULL, &paramscount, &i);

  params = malloc(paramscount * sizeof(char*) + i * sizeof(char) + 1);
  if (params)
  {
    struct {
      const char *param;
      wad_source_t source;
    } files[] = {
      {"-iwad" , source_iwad},
      {"-file" , source_pwad},
      {"-deh"  , source_deh},
      {NULL}
    };

    M_ParseCmdLine(str, params, ((char*)params) + sizeof(char*) * paramscount, &paramscount, &i);
  
    if (!M_CheckParm("-iwad") && !M_CheckParm("-file"))
    {
      i = 0;
      while (files[i].param)
      {
        p = M_CheckParmEx(files[i].param, params, paramscount);
        if (p >= 0)
        {
          while (++p != paramscount && *params[p] != '-')
          {
            char *filename;
            //something is wrong here
            filename = I_FindFile(params[p], ".wad");
            if (!filename)
            {
              filename = strdup(params[p]);
            }
            WadDataAddItem(waddata, filename, files[i].source, 0);
            free(filename);
          }
        }
        i++;
      }
    }

    if (!M_CheckParm("-complevel"))
    {
      p = M_CheckParmEx("-complevel", params, paramscount);
      if (p >= 0 && p < (int)paramscount - 1)
      {
        M_AddParam("-complevel");
        M_AddParam(strdup(params[p + 1]));
      }
    }

    // for doom 1.2
    if (!M_CheckParm("-respawn"))
    {
      p = M_CheckParmEx("-respawn", params, paramscount);
      if (p >= 0)
      {
        M_AddParam("-respawn");
      }
    }

    // for doom 1.2
    if (!M_CheckParm("-fast"))
    {
      p = M_CheckParmEx("-fast", params, paramscount);
      if (p >= 0)
      {
        M_AddParam("-fast");
      }
    }

    // for doom 1.2
    if (!M_CheckParm("-nomonsters"))
    {
      p = M_CheckParmEx("-nomonsters", params, paramscount);
      if (p >= 0)
      {
        M_AddParam("-nomonsters");
      }
    }

    p = M_CheckParmEx("-spechit", params, paramscount);
    if (p >= 0 && p < (int)paramscount - 1)
    {
      spechit_baseaddr = atoi(params[p + 1]);
    }

    //overflows
    {
      overrun_list_t overflow;
      for (overflow = 0; overflow < OVERFLOW_MAX; overflow++)
      {
        int value;
        char *pstr, *mask;

        mask = malloc(strlen(overflow_cfgname[overflow]) + 16);
        if (mask)
        {
          sprintf(mask, "-set %s", overflow_cfgname[overflow]);
          pstr = strstr(str, mask);

          if (pstr)
          {
            strcat(mask, " = %d");
            if (sscanf(pstr, mask, &value) == 1)
            {
              overflows[overflow].tmp_emulate = value;
            }
          }
          free(mask);
        }
      }
    }

    free(params);
  }

  W_UnlockLumpNum(lump);
  free(str);
}

static void R_DemoEx_AddParams(wadtbl_t *wadtbl)
{
  size_t i;
  int p;
  char buf[200];

  char* filename_p;
  char* fileext_p;

  char *files = NULL;
  char *iwad  = NULL;
  char *pwads = NULL;
  char *dehs  = NULL;
  char **item;

  //iwad and pwads
  for (i = 0; i < numwadfiles; i++)
  {
    filename_p = PathFindFileName(wadfiles[i].name);
    fileext_p = filename_p + strlen(filename_p) - 1;
    while (fileext_p != filename_p && *(fileext_p - 1) != '.')
      fileext_p--;
    if (fileext_p == filename_p)
      continue;

    item = NULL;

    if (wadfiles[i].src == source_iwad && !iwad && !strcasecmp(fileext_p, "wad"))
      item = &iwad;

    if (wadfiles[i].src == source_pwad && !strcasecmp(fileext_p, "wad"))
      item = &pwads;

    if (item)
    {
      AddString(item, "\"");
      AddString(item, filename_p);
      AddString(item, "\" ");
    }
  }

  //dehs
  p = M_CheckParm ("-deh");
  if (p)
  {
    while (++p != myargc && *myargv[p] != '-')
    {
      char *file = NULL;
      if ((file = I_FindFile(myargv[p], ".bex")) ||
          (file = I_FindFile(myargv[p], ".deh")))
      {
        filename_p = PathFindFileName(file);
        AddString(&dehs, "\"");
        AddString(&dehs, filename_p);
        AddString(&dehs, "\" ");
        free(file);
      }
    }
  }

  if (iwad)
  {
    AddString(&files, "-iwad ");
    AddString(&files, iwad);
  }
  
  if (pwads)
  {
    AddString(&files, "-file ");
    AddString(&files, pwads);
  }

  if (dehs)
  {
    AddString(&files, "-deh ");
    AddString(&files, dehs);
  }

  //add complevel for formats which do not have it in header
  if (demo_compatibility)
  {
    sprintf(buf, "-complevel %d ", compatibility_level);
    AddString(&files, buf);
  }

  // doom 1.2 does not store these params in header
  if (compatibility_level == doom_12_compatibility)
  {
    if (M_CheckParm("-respawn"))
    {
      sprintf(buf, "-respawn ");
      AddString(&files, buf);
    }
    if (M_CheckParm("-fast"))
    {
      sprintf(buf, "-fast ");
      AddString(&files, buf);
    }
    if (M_CheckParm("-nomonsters"))
    {
      sprintf(buf, "-nomonsters ");
      AddString(&files, buf);
    }
  }

  if (spechit_baseaddr != 0 && spechit_baseaddr != DEFAULT_SPECHIT_MAGIC)
  {
    sprintf(buf, "-spechit %d ", spechit_baseaddr);
    AddString(&files, buf);
  }

  //overflows
  {
    overrun_list_t overflow;
    for (overflow = 0; overflow < OVERFLOW_MAX; overflow++)
    {
      if (overflows[overflow].shit_happens)
      {
        sprintf(buf, "-set %s=%d ", overflow_cfgname[overflow], overflows[overflow].emulate);
        AddString(&files, buf);
      }
    }
  }

  if (files)
  {
    W_AddLump(wadtbl, DEMOEX_PARAMS_LUMPNAME, (const byte*)files, strlen(files));
  }
}

static void R_DemoEx_AddMouseLookData(wadtbl_t *wadtbl)
{
  int i = 0;

  if (!mlook_lump.data)
    return;

  // search for at least one tic with a nonzero pitch
  while (i < (int)mlook_lump.tick)
  {
    if (mlook_lump.data[i] != 0)
    {
      W_AddLump(wadtbl, mlook_lump.name, 
        (const byte*)mlook_lump.data, mlook_lump.tick * sizeof(mlook_lump.data[0]));
      break;
    }
    i++;
  }
}

void I_DemoExShutdown(void)
{
  W_ReleaseAllWads();

  if (demoex_filename[0] && !(demo_demoex_filename && *demo_demoex_filename))
  {
    lprintf(LO_DEBUG, "I_DemoExShutdown: removing %s\n", demoex_filename);
    if (unlink(demoex_filename) != 0)
    {
      lprintf(LO_DEBUG, "I_DemoExShutdown: %s\n", strerror(errno));
    }
  }
}

byte* G_GetDemoFooter(const char *filename, byte** footer, size_t *size)
{
  byte* result = NULL;

  FILE *hfile;
  byte *buffer = NULL;
  byte* p;
  size_t file_size;

  hfile = fopen(filename, "rb");

  if (!hfile)
    return result;

  //get demo size in bytes
  fseek(hfile, 0, SEEK_END);
  file_size = ftell(hfile);
  fseek(hfile, 0, SEEK_SET);

  buffer = malloc(file_size);

  if (fread(buffer, file_size, 1, hfile) == 1)
  {
    //skip demo header
    p = (byte*)G_ReadDemoHeaderEx(buffer, file_size, RDH_SKIP_HEADER);

    //skip demo data
    while (p < buffer + file_size && *((byte*)p) != DEMOMARKER)
    {
      p += bytes_per_tic;
    }

    if (*((byte*)p) == DEMOMARKER)
    {
      //skip DEMOMARKER
      p++;

      //seach for the "PWAD" signature after ENDDEMOMARKER
      while (p - buffer + sizeof(wadinfo_t) < file_size)
      {
        if (!memcmp(p, PWAD_SIGNATURE, strlen(PWAD_SIGNATURE)))
        {
          //got it!
          //the demo has an additional information itself
          int demoex_size = file_size - (p - buffer);
          
          result = buffer;

          if (footer)
          {
            *footer = p;
          }

          if (size)
          {
            *size = demoex_size;
          }

          break;
        }
        p++;
      }
    }
  }

  fclose(hfile);

  return result;
}

void G_SetDemoFooter(const char *filename, wadtbl_t *wadtbl)
{
  FILE *hfile;
  byte *buffer = NULL;
  byte *demoex_p = NULL;
  size_t size;

  buffer = G_GetDemoFooter(filename, &demoex_p, &size);
  if (buffer)
  {
    char newfilename[PATH_MAX];

    strncpy(newfilename, filename, sizeof(newfilename) - 5);
    strcat(newfilename, ".out");

    hfile = fopen(newfilename, "wb");
    if (hfile)
    {
      int demosize = (demoex_p - buffer);
      int headersize = sizeof(wadtbl->header);
      int datasize = wadtbl->datasize;
      int lumpssize = wadtbl->header.numlumps * sizeof(wadtbl->lumps[0]);
  
      fwrite(buffer, demosize, 1, hfile);

      //write pwad header, all data and lookup table to the end of a demo
      if (
        fwrite(&wadtbl->header, headersize, 1, hfile) != 1 ||
        fwrite(wadtbl->data, datasize, 1, hfile) != 1 ||
        fwrite(wadtbl->lumps, lumpssize, 1, hfile) != 1 ||
        false)
      {
        I_Error("G_SetDemoFooter: error writing");
      }

      fclose(hfile);
    }
    free(buffer);
  }
}

static int G_ReadDemoFooter(const char *filename)
{
  int result = false;

  byte *buffer = NULL;
  byte *demoex_p = NULL;
  size_t size;

  M_ChangeDemoExtendedFormat();

  if (!use_demoex_info)
    return result;

  demoex_filename[0] = 0;

  if (demo_demoex_filename && *demo_demoex_filename)
  {
    strncpy(demoex_filename, demo_demoex_filename, PATH_MAX);
  }
  else
  {
    const char* tmp_dir;
    char* tmp_path = NULL;
    const char* template_format = "%sprboom-plus-demoex-XXXXXX";

    tmp_dir = I_GetTempDir();
    if (tmp_dir && strlen(tmp_dir) > 0)
    {
      tmp_path = malloc(strlen(tmp_dir) + 2);
      strcpy(tmp_path, tmp_dir);
      if (!HasTrailingSlash(tmp_dir))
      {
        strcat(tmp_path, "/");
      }

      SNPRINTF(demoex_filename, sizeof(demoex_filename), template_format, tmp_path);
      mktemp(demoex_filename);

      free(tmp_path);
    }
  }

  if (!demoex_filename[0])
  {
    lprintf(LO_ERROR, "G_ReadDemoFooter: failed to create demoex temp file");
  }
  else
  {
    AddDefaultExtension(demoex_filename, ".wad");

    buffer = G_GetDemoFooter(filename, &demoex_p, &size);
    if (buffer)
    {
      //the demo has an additional information itself
      size_t i;
      waddata_t waddata;

      //write an additional info from a demo to demoex.wad
      if (!M_WriteFile(demoex_filename, (void*)demoex_p, size))
      {
        lprintf(LO_ERROR, "G_ReadDemoFooter: failed to create demoex temp file %s", demoex_filename);
      }
      else
      {
        //add demoex.wad to the wads list
        D_AddFile(demoex_filename, source_auto_load);

        //cache demoex.wad for immediately getting its data with W_CacheLumpName
        W_Init();

        WadDataInit(&waddata);

        //enumerate and save all auto-loaded files and demo for future use
        for (i = 0; i < numwadfiles; i++)
        {
          if (
            wadfiles[i].src == source_auto_load ||
            wadfiles[i].src == source_pre ||
            wadfiles[i].src == source_lmp)
          {
            WadDataAddItem(&waddata, wadfiles[i].name, wadfiles[i].src, 0);
          }
        }

        //get needed wads and dehs from demoex.wad
        //restore all critical params like -spechit x
        R_DemoEx_GetParams(buffer, &waddata);

        //replace old wadfiles with the new ones
        if (waddata.numwadfiles)
        {
          for (i = 0; (size_t)i < waddata.numwadfiles; i++)
          {
            if (waddata.wadfiles[i].src == source_iwad)
            {
              W_ReleaseAllWads();
              WadDataToWadFiles(&waddata);
              result = true;
              break;
            }
          }
        }
        WadDataFree(&waddata);
      }
      free(buffer);
    }
    else
    {
      demoex_filename[0] = 0;
    }
  }

  return result;
}

void G_WriteDemoFooter(FILE *file)
{
  wadtbl_t demoex;

  if (!use_demoex_info)
    return;

  //init PWAD header
  W_InitPWADTable(&demoex);

  //
  //write all the data
  //

  // separators for eye-friendly looking
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));

  //process format version
  W_AddLump(&demoex, DEMOEX_VERSION_LUMPNAME, (const byte*)DEMOEX_VERSION, strlen(DEMOEX_VERSION));
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));

  //process mlook
  R_DemoEx_AddMouseLookData(&demoex);
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));

  //process port name
  W_AddLump(&demoex, DEMOEX_PORTNAME_LUMPNAME,
    (const byte*)(PACKAGE_TITLE" "VERSION), strlen(PACKAGE_TITLE" "VERSION));
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));

  //process iwad, pwads, dehs and critical for demos params like -spechit, etc
  R_DemoEx_AddParams(&demoex);
  W_AddLump(&demoex, NULL, (const byte*)DEMOEX_SEPARATOR, strlen(DEMOEX_SEPARATOR));

  //write pwad header, all data and lookup table to the end of a demo
  if (
    fwrite(&demoex.header, sizeof(demoex.header), 1, file) != 1 ||
    fwrite(demoex.data, demoex.datasize, 1, file) != 1 ||
    fwrite(demoex.lumps, demoex.header.numlumps * sizeof(demoex.lumps[0]), 1, file) != 1 ||
    false)
  {
    I_Error("G_WriteDemoFooter: error writing");
  }
}

int WadDataInit(waddata_t *waddata)
{
  if (!waddata)
    return false;

  memset(waddata, 0, sizeof(*waddata));
  return true;
}

void WadDataFree(waddata_t *waddata)
{
  if (waddata)
  {
    if (waddata->wadfiles)
    {
      int i;
      for (i = 0; i < (int)waddata->numwadfiles; i++)
      {
        if (waddata->wadfiles[i].name)
        {
          free(waddata->wadfiles[i].name);
          waddata->wadfiles[i].name = NULL;
        }
      }
      free(waddata->wadfiles);
      waddata->wadfiles = NULL;
    }
  }
}

int WadDataAddItem(waddata_t *waddata, const char *filename, wad_source_t source, int handle)
{
  if (!waddata || !filename)
    return false;

  waddata->wadfiles = realloc(waddata->wadfiles, sizeof(*wadfiles) * (waddata->numwadfiles + 1));
  waddata->wadfiles[waddata->numwadfiles].name = strdup(filename);
  waddata->wadfiles[waddata->numwadfiles].src = source;
  waddata->wadfiles[waddata->numwadfiles].handle = handle;

  waddata->numwadfiles++;

  return true;
}

int ParseDemoPattern(const char *str, waddata_t* waddata, char **missed, dboolean trytodownload)
{
  int processed = 0;
  wadfile_info_t *wadfiles = NULL;
  size_t numwadfiles = 0;
  char *pStr = strdup(str);
  char *pToken = pStr;
  
  if (missed)
  {
    *missed = NULL;
  }

  for (;(pToken = strtok(pToken,"|"));pToken = NULL)
  {
    char *token = NULL;
    processed++;

    if (trytodownload && !I_FindFile2(pToken, ".wad"))
    {
      D_TryGetWad(pToken);
    }
#ifdef _MSC_VER
    token = malloc(PATH_MAX);
    if (GetFullPath(pToken, ".wad", token, PATH_MAX))
#else
    if ((token = I_FindFile(pToken, ".wad")))
#endif
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = token;
      wadfiles[numwadfiles].handle = 0;
      
      if (pToken == pStr)
      {
        wadfiles[numwadfiles].src = source_iwad;
      }
      else
      {
        char *p = (char*)wadfiles[numwadfiles].name;
        int len = strlen(p);
        if (!strcasecmp(&p[len-4],".wad"))
          wadfiles[numwadfiles].src = source_pwad;
        if (!strcasecmp(&p[len-4],".deh") || !strcasecmp(&p[len-4],".bex"))
          wadfiles[numwadfiles].src = source_deh;
      }
      numwadfiles++;
    }
    else
    {
      if (missed)
      {
        int len = (*missed ? strlen(*missed) : 0);
        *missed = realloc(*missed, len + strlen(pToken) + 100);
        sprintf(*missed + len, " %s not found\n", pToken);
      }
    }
  }

  WadDataFree(waddata);

  waddata->wadfiles = wadfiles;
  waddata->numwadfiles = numwadfiles;

  free(pStr);

  return processed;
}

#ifdef HAVE_LIBPCREPOSIX
int DemoNameToWadData(const char * demoname, waddata_t *waddata, patterndata_t *patterndata)
{
  int numwadfiles_required = 0;
  int i;
  size_t maxlen = 0;
  char *pattern;

  char *demofilename = PathFindFileName(demoname);
  
  WadDataInit(waddata);

  for (i = 0; i < demo_patterns_count; i++)
  {
    if (strlen(demo_patterns_list[i]) > maxlen)
      maxlen = strlen(demo_patterns_list[i]);
  }

  pattern = malloc(maxlen + sizeof(char));
  for (i = 0; i < demo_patterns_count; i++)
  {
    int result;
    regex_t preg;
    regmatch_t pmatch[4];
    char errbuf[256];
    char *buf = demo_patterns_list[i];

    regcomp(&preg, "(.*?)\\/(.*)\\/(.+)", REG_ICASE);
    result = regexec(&preg, buf, 4, &pmatch[0], REG_NOTBOL);
    regerror(result, &preg, errbuf, sizeof(errbuf));
    regfree(&preg);

    if (result != 0)
    {
      lprintf(LO_WARN, "Incorrect format of the <%s%d = \"%s\"> config entry\n", demo_patterns_mask, i, buf);
    }
    else
    {
      regmatch_t demo_match[2];
      int len = pmatch[2].rm_eo - pmatch[2].rm_so;

      strncpy(pattern, buf + pmatch[2].rm_so, len);
      pattern[len] = '\0';
      result = regcomp(&preg, pattern, REG_ICASE);
      if (result != 0)
      {
        regerror(result, &preg, errbuf, sizeof(errbuf));
        lprintf(LO_WARN, "Incorrect regular expressions in the <%s%d = \"%s\"> config entry - %s\n", demo_patterns_mask, i, buf, errbuf);
      }
      else
      {
        result = regexec(&preg, demofilename, 1, &demo_match[0], 0);
        if (result == 0 && demo_match[0].rm_so == 0 && demo_match[0].rm_eo == (int)strlen(demofilename))
        {
          numwadfiles_required = ParseDemoPattern(buf + pmatch[3].rm_so, waddata,
            (patterndata ? &patterndata->missed : NULL), true);

          waddata->wadfiles = realloc(waddata->wadfiles, sizeof(*wadfiles)*(waddata->numwadfiles+1));
          waddata->wadfiles[waddata->numwadfiles].name = strdup(demoname);
          waddata->wadfiles[waddata->numwadfiles].src = source_lmp;
          waddata->wadfiles[waddata->numwadfiles].handle = 0;
          waddata->numwadfiles++;

          if (patterndata)
          {
            len = MIN(pmatch[1].rm_eo - pmatch[1].rm_so, sizeof(patterndata->pattern_name) - 1);
            strncpy(patterndata->pattern_name, buf, len);
            patterndata->pattern_name[len] = '\0';

            patterndata->pattern_num = i;
          }

          break;
        }
      }
      regfree(&preg);
    }
  }
  free(pattern);

  return numwadfiles_required;
}
#endif // HAVE_LIBPCREPOSIX

void WadDataToWadFiles(waddata_t *waddata)
{
  void ProcessDehFile(const char *filename, const char *outfilename, int lumpnum);
  const char *D_dehout(void);

  int i, iwadindex = -1;

  wadfile_info_t *old_wadfiles=NULL;
  size_t old_numwadfiles = numwadfiles;

  old_numwadfiles = numwadfiles;
  old_wadfiles = malloc(sizeof(*(wadfiles)) * numwadfiles);
  memcpy(old_wadfiles, wadfiles, sizeof(*(wadfiles)) * numwadfiles);

  free(wadfiles);
  wadfiles = NULL;
  numwadfiles = 0;

  for (i = 0; (size_t)i < waddata->numwadfiles; i++)
  {
    if (waddata->wadfiles[i].src == source_iwad)
    {
      AddIWAD(I_FindFile(waddata->wadfiles[i].name, ".wad"));
      iwadindex = i;
      break;
    }
  }

  if (iwadindex == -1)
  {
    I_Error("WadDataToWadFiles: IWAD not found\n");
  }

  for (i = 0; (size_t)i < old_numwadfiles; i++)
  {
    if (old_wadfiles[i].src == source_auto_load || old_wadfiles[i].src == source_pre)
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = strdup(old_wadfiles[i].name);
      wadfiles[numwadfiles].src = old_wadfiles[i].src;
      wadfiles[numwadfiles].handle = old_wadfiles[i].handle;
      numwadfiles++;
    }
  }

  for (i = 0; (size_t)i < waddata->numwadfiles; i++)
  {
    if (waddata->wadfiles[i].src == source_auto_load)
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = strdup(waddata->wadfiles[i].name);
      wadfiles[numwadfiles].src = waddata->wadfiles[i].src;
      wadfiles[numwadfiles].handle = waddata->wadfiles[i].handle;
      numwadfiles++;
    }
  }

  for (i = 0; (size_t)i < waddata->numwadfiles; i++)
  {
    if (waddata->wadfiles[i].src == source_iwad && i != iwadindex)
    {
      D_AddFile(waddata->wadfiles[i].name, source_pwad);
      modifiedgame = true;
    }
    if (waddata->wadfiles[i].src == source_pwad)
    {
      char *file = I_FindFile(waddata->wadfiles[i].name, ".wad");
      if (!file && D_TryGetWad(waddata->wadfiles[i].name))
      {
        file = I_FindFile(waddata->wadfiles[i].name, ".wad");
      }
      if (file)
      {
        D_AddFile(waddata->wadfiles[i].name, source_pwad);
        modifiedgame = true;
      }
    }
    if (waddata->wadfiles[i].src == source_deh)
    {
      ProcessDehFile(waddata->wadfiles[i].name, D_dehout(), 0);
    }
  }

  for (i = 0; (size_t)i < waddata->numwadfiles; i++)
  {
    if (waddata->wadfiles[i].src == source_lmp || waddata->wadfiles[i].src == source_net)
      D_AddFile(waddata->wadfiles[i].name, waddata->wadfiles[i].src);
  }

  free(old_wadfiles);
}

void WadFilesToWadData(waddata_t *waddata)
{
  int i;

  if (!waddata)
    return;

  for (i = 0; i < (int)numwadfiles; i++)
  {
    WadDataAddItem(waddata, wadfiles[i].name, wadfiles[i].src, wadfiles[i].handle);
  }
}

int CheckDemoExDemo(void)
{
  int result = false;
  int p;

  M_ChangeDemoExtendedFormat();

  p = IsDemoPlayback();
  if (!p)
  {
    p = IsDemoContinue();
  }

  if (p)
  {
    char *demoname, *filename;

    filename = malloc(strlen(myargv[p + 1]) + 16);
    strcpy(filename, myargv[p + 1]);
    AddDefaultExtension(filename, ".lmp");

    demoname = I_FindFile(filename, NULL);
    if (demoname)
    {
      result = G_ReadDemoFooter(demoname);
      free(demoname);
    }

    free(filename);
  }

  return result;
}

int CheckAutoDemo(void)
{
  int result = false;
  if (M_CheckParm("-auto"))
#ifndef HAVE_LIBPCREPOSIX
    I_Error("Cannot process -auto - "
        PACKAGE " was compiled without LIBPCRE support");
#else
  {
    int i;
    waddata_t waddata;

    for (i = 0; (size_t)i < numwadfiles; i++)
    {
      if (wadfiles[i].src == source_lmp)
      {
        int numwadfiles_required;
        
        patterndata_t patterndata;
        memset(&patterndata, 0, sizeof(patterndata));

        numwadfiles_required = DemoNameToWadData(wadfiles[i].name, &waddata, &patterndata);
        
        if (waddata.numwadfiles)
        {
          result = true;
          if ((size_t)numwadfiles_required + 1 != waddata.numwadfiles && patterndata.missed)
          {
            I_Warning(
              "DataAutoload: pattern #%i is used\n"
              "%s not all required files are found, may not work\n",
              patterndata.pattern_num, patterndata.missed);
          }
          else
          {
            lprintf(LO_WARN,"DataAutoload: pattern #%i is used\n", patterndata.pattern_num);
          }
          WadDataToWadFiles(&waddata);
        }
        free(patterndata.missed);
        WadDataFree(&waddata);
        break;
      }
    }
  }
#endif // HAVE_LIBPCREPOSIX

  return result;
}

char *getwad_cmdline = NULL;

dboolean D_TryGetWad(const char* name)
{
  dboolean result = false;

  char wadname[PATH_MAX];
  char* cmdline = NULL;
  char* wadname_p = NULL;
  char* msg = NULL;
  const char* format =
    "The necessary wad has not been found\n"
    "Do you want to search for \'%s\'?\n\n"
    "Command line:\n%s\n\n"
    "Be careful! Execution of an unknown program is unsafe.";

  if (!getwad_cmdline || !name || !(*getwad_cmdline) || !(*name))
    return false;

  strncpy(wadname, PathFindFileName(name), sizeof(wadname) - 4);
  AddDefaultExtension(wadname, ".wad");

  cmdline = malloc(strlen(getwad_cmdline) + strlen(wadname) + 2);
  wadname_p = strstr(getwad_cmdline, "%wadname%");
  if (wadname_p)
  {
    strncpy(cmdline, getwad_cmdline, wadname_p - getwad_cmdline);
    strcat(cmdline, wadname);
    strcat(cmdline, wadname_p + strlen("%wadname%"));
  }
  else
  {
    sprintf(cmdline, "%s %s", getwad_cmdline, wadname);
  }

  msg = malloc(strlen(format) + strlen(wadname) + strlen(cmdline));
  sprintf(msg, format, wadname, cmdline);

  if (PRB_IDYES == I_MessageBox(msg, PRB_MB_DEFBUTTON2 | PRB_MB_YESNO))
  {
    int ret;

    lprintf(LO_INFO, "D_TryGetWad: Trying to get %s from somewhere\n", name);

    ret = system(cmdline);

    if (ret != 0)
    {
      lprintf(LO_ERROR, "D_TryGetWad: Execution failed - %s\n", strerror(errno));
    }
    else
    {
      char *str = I_FindFile(name, ".wad");
      if (str)
      {
        lprintf(LO_INFO, "D_TryGetWad: Successfully received\n");
        free(str);
        result = true;
      }
    }
  }

  free(msg);
  free(cmdline);

  return result;
}
