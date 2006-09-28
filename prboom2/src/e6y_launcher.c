/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *
 *-----------------------------------------------------------------------------
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <commctrl.h>

#include "doomtype.h"
#include "w_wad.h"
#include "doomstat.h"
#include "lprintf.h"
#include "d_main.h"
#include "m_misc.h"
#include "i_system.h"
#include "m_argv.h"
#include "i_main.h"
#include ".\..\ICONS\resource.h"
#include "e6y.h"
#include "e6y_launcher.h"

#pragma comment( lib, "comctl32.lib" )
#pragma comment( lib, "advapi32.lib" )

#define PCRE_STATIC 1
#include "pcreposix.h"
#ifdef _DEBUG
#pragma comment( lib, "pcred.lib" )
#pragma comment( lib, "pcreposixd.lib" )
#else
#pragma comment( lib, "pcre.lib" )
#pragma comment( lib, "pcreposix.lib" )
#endif

#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)
typedef HRESULT (WINAPI *EnableThemeDialogTexturePROC)(HWND, DWORD);

#define FA_DIREC	0x00000010
#define LAUNCHER_HISTORY_SIZE 10

#define LAUNCHER_CAPTION "Prboom-Plus Launcher"

typedef struct
{
  wadfile_info_t *wadfiles;
  size_t numwadfiles;
} wadfiles_t;

typedef struct
{
  char name[PATH_MAX];
  wad_source_t source;
  boolean doom1;
  boolean doom2;
} fileitem_t;

typedef struct
{
  HWND HWNDServer;
  HWND HWNDClient;
  HWND listIWAD;
  HWND listPWAD;
  HWND listHistory;
  HWND listCMD;
  HWND staticFileName;
  fileitem_t *files;
  size_t filescount;
  fileitem_t *cache;
  size_t cachesize;
} launcher_t;

launcher_t launcher;

int launcher_enable;
char *launcher_history[LAUNCHER_HISTORY_SIZE];

static char launchercachefile[PATH_MAX];

//global
void CheckIWAD(const char *iwadname,GameMode_t *gmode,boolean *hassec);
void ProcessDehFile(const char *filename, const char *outfilename, int lumpnum);
const char *D_dehout(void);

//common
void *I_FindFirst (const char *filespec, findstate_t *fileinfo);
int I_FindNext (void *handle, findstate_t *fileinfo);
int I_FindClose (void *handle);

//events
static void L_GameOnChange(void);
static void L_FilesOnChange(void);
static void L_HistoryOnChange(void);
static void L_CommandOnChange(void);

static void L_FillGameList(void);
static void L_FillFilesList(fileitem_t *iwad);
static void L_FillHistoryList(void);

static char* L_HistoryGetStr(wadfiles_t *data);
static wadfiles_t* L_HistoryGetData(const char *str);
static void L_FreeWadFiles(wadfiles_t *data);

static boolean L_DemoAutoDetect(void);

static void L_ReadCacheData(void);
static void L_AddItemToCache(fileitem_t *item);

static boolean L_GetFileType(const char *filename, fileitem_t *item);
static boolean L_PrepareToLaunch(void);

static boolean L_GUISelect(wadfile_info_t *wadfiles, size_t numwadfiles);
static boolean L_LauncherIsNeeded(void);

static void L_FillFilesList(fileitem_t *iwad);
static void L_AddItemToCache(fileitem_t *item);

char* e6y_I_FindFile(const char* ext);
int GetFullPath(const char* FileName, char *Buffer, size_t BufferLength);

//common
void *I_FindFirst (const char *filespec, findstate_t *fileinfo)
{
	return FindFirstFileA(filespec, (LPWIN32_FIND_DATAA)fileinfo);
}

int I_FindNext (void *handle, findstate_t *fileinfo)
{
	return !FindNextFileA((HANDLE)handle, (LPWIN32_FIND_DATAA)fileinfo);
}

int I_FindClose (void *handle)
{
	return FindClose((HANDLE)handle);
}

//events
static void L_GameOnChange(void)
{
  int index;

  index = SendMessage(launcher.listIWAD, CB_GETCURSEL, 0, 0);
  if (index != CB_ERR)
  {
    index = SendMessage(launcher.listIWAD, CB_GETITEMDATA, index, 0);
    if (index != CB_ERR)
    {
      L_FillFilesList(&launcher.files[index]);
    }
  }
}

static void L_FilesOnChange(void)
{
  int index = SendMessage(launcher.listPWAD, LB_GETCURSEL, 0, 0);
  if (index != LB_ERR)
  {
    index = SendMessage(launcher.listPWAD, LB_GETITEMDATA, index, 0);
    if (index != LB_ERR)
    {
      char path[PATH_MAX];
      size_t count;
      RECT rect;
      HFONT font, oldfont;
      HDC hdc;

      strcpy(path, launcher.files[index].name);
      NormalizeSlashes2(path);
      strlwr(path);

      hdc = GetDC(launcher.staticFileName);
      GetWindowRect(launcher.staticFileName, &rect);

      font = (HFONT)SendMessage(launcher.staticFileName, WM_GETFONT, 0, 0);
      oldfont = SelectObject(hdc, font);
      
      for (count = strlen(path); count > 0 ; count--)
      {
        char tmppath[PATH_MAX];
        SIZE size = {0, 0};
        strcpy(tmppath, path);
        AbbreviateName(tmppath, count, false);
        if (GetTextExtentPoint32(hdc, tmppath, count, &size))
        {
          if (size.cx < rect.right - rect.left)
          {
            SendMessage(launcher.staticFileName, WM_SETTEXT, 0, (LPARAM)tmppath);
            break;
          }
        }
      }
      
      SelectObject(hdc, oldfont);
    }
  }
}

static void L_HistoryOnChange(void)
{
  int index;

  index = SendMessage(launcher.listHistory, CB_GETCURSEL, 0, 0);
  if (index >= 0)
  {
    wadfiles_t *wadfiles;
    wadfiles = (wadfiles_t*)SendMessage(launcher.listHistory, CB_GETITEMDATA, index, 0);
    if ((int)wadfiles != CB_ERR)
    {
      if (!L_GUISelect(wadfiles->wadfiles, wadfiles->numwadfiles))
      {
        SendMessage(launcher.listHistory, CB_SETCURSEL, -1, 0);
      }
    }
  }
}

static void L_CommandOnChange(void)
{
  int index;

  index = SendMessage(launcher.listCMD, CB_GETCURSEL, 0, 0);
  
  switch (index)
  {
  case 0:
    remove(launchercachefile);
    
    SendMessage(launcher.listPWAD, LB_RESETCONTENT, 0, 0);
    SendMessage(launcher.listHistory, CB_SETCURSEL, -1, 0);
    
    if (launcher.files)
    {
      free(launcher.files);
      launcher.files = NULL;
    }
    launcher.filescount = 0;

    if (launcher.cache)
    {
      free(launcher.cache);
      launcher.cache = NULL;
    }
    launcher.cachesize = 0;

    e6y_I_FindFile("*.wad");
    e6y_I_FindFile("*.deh");

    L_GameOnChange();

    MessageBox(launcher.HWNDServer, "The cache has been successfully rebuilt", LAUNCHER_CAPTION, MB_OK|MB_ICONEXCLAMATION);
    break;
  case 1:
    {
      size_t i;
      for (i = 0; i < sizeof(launcher_history)/sizeof(launcher_history[0]); i++)
      {
        char str[32];
        default_t *history;

        sprintf(str, "launcher_history%d", i);
        history = M_LookupDefault(str);
        
        strcpy((char*)history->location.ppsz[0], "");
      }
      M_SaveDefaults();
      L_FillHistoryList();
      SendMessage(launcher.listHistory, CB_SETCURSEL, -1, 0);
  
      MessageBox(launcher.HWNDServer, "The history has been successfully cleared", LAUNCHER_CAPTION, MB_OK|MB_ICONEXCLAMATION);
    }
    break;

  case 2:
    {
      HKEY hKey;
      DWORD Disposition;
      if (RegCreateKeyEx(HKEY_CLASSES_ROOT, ".lmp\\shell\\open\\command", 0, NULL, 0, KEY_WRITE, NULL, &hKey, &Disposition) == ERROR_SUCCESS)
      {
        char str[PATH_MAX];
        sprintf(str, "\"%s\" \"%%1\"", *myargv);
        
        RegSetValueEx(hKey, NULL, 0, REG_SZ, str, strlen(str) + 1);
        RegCloseKey(hKey);
      }
      MessageBox(launcher.HWNDServer, "Succesfully Installed", LAUNCHER_CAPTION, MB_OK|MB_ICONEXCLAMATION);
    }
    break;

  case 3:
    {
      char buf[128];
      sprintf(buf, "Do you really want to %s the Launcher?", (launcher_enable ? "disable" : "enable"));
      if (MessageBox(launcher.HWNDServer, buf, LAUNCHER_CAPTION, MB_YESNO|MB_ICONQUESTION) == IDYES)
      {
        launcher_enable = !launcher_enable;
        M_SaveDefaults();
        sprintf(buf, "Successfully %s", (launcher_enable ? "enabled" : "disabled"));
        MessageBox(launcher.HWNDServer, buf, LAUNCHER_CAPTION, MB_OK|MB_ICONEXCLAMATION);
      }
    }
    break;
  }
  
  SendMessage(launcher.listCMD, CB_SETCURSEL, -1, 0);
}

static boolean L_GetFileType(const char *filename, fileitem_t *item)
{
  size_t i, len;
  wadinfo_t header;
  FILE *f;

  item->source = source_err;
  item->doom1 = false;
  item->doom2 = false;
  strcpy(item->name, filename);
  
  len = strlen(filename);

  if (!strcasecmp(&filename[len-4],".deh"))
  {
    item->source = source_deh;
    return true;
  }
  
  for (i = 0; i < launcher.cachesize; i++)
  {
    if (!strcasecmp(filename, launcher.cache[i].name))
    {
      strcpy(item->name, launcher.cache[launcher.cachesize].name);
      item->source = launcher.cache[i].source;
      item->doom1 = launcher.cache[i].doom1;
      item->doom2 = launcher.cache[i].doom2;
      return true;
    }
  }

  if ( (f = fopen (filename, "rb")) )
  {
    fread (&header, sizeof(header), 1, f);
    if (!strncmp(header.identification, "IWAD", 4))
    {
      item->source = source_iwad;
    }
    else if (!strncmp(header.identification, "PWAD", 4))
    {
      item->source = source_pwad;
    }
    if (item->source != source_err)
    {
      header.numlumps = LONG(header.numlumps);
      if (0 == fseek(f, LONG(header.infotableofs), SEEK_SET))
      {
        for (i = 0; !item->doom1 && !item->doom2 && i < (size_t)header.numlumps; i++)
        {
          filelump_t lump;
          
          if (0 == fread (&lump, sizeof(lump), 1, f))
            break;

          if (strlen(lump.name) == 4)
          {
            if ((lump.name[0] == 'E' && lump.name[2] == 'M') &&
              (lump.name[1] >= '1' && lump.name[1] <= '4') &&
              (lump.name[3] >= '1' && lump.name[3] <= '9'))
              item->doom1 = true;
          }

          if (strlen(lump.name) == 5)
          {
            if (!strncmp(lump.name, "MAP", 3) &&
              (lump.name[3] >= '0' && lump.name[3] <= '9') &&
              (lump.name[4] >= '0' && lump.name[4] <= '9'))
              item->doom2 = true;
          }

        }
        L_AddItemToCache(item);
      }
    }
    fclose(f);
    return true;
  }
  return false;
}

static boolean L_GUISelect(wadfile_info_t *wadfiles, size_t numwadfiles)
{
  int i, j;
  size_t k;
  int topindex;
  boolean processed = false;
  int listIWADCount, listPWADCount;
  char fullpath[PATH_MAX];
  
  if (!wadfiles)
    return false;

  listIWADCount = SendMessage(launcher.listIWAD, CB_GETCOUNT, 0, 0);
  SendMessage(launcher.listIWAD, CB_SETCURSEL, -1, 0);

  for (k=0; !processed && k < numwadfiles; k++)
  {
    if (GetFullPath(wadfiles[k].name, fullpath, PATH_MAX))
    {
      switch (wadfiles[k].src)
      {
      case source_iwad:
        for (i=0; !processed && (size_t)i<launcher.filescount; i++)
        {
          if (launcher.files[i].source == source_iwad &&
              !strcasecmp(launcher.files[i].name, fullpath))
          {
            for (j=0; !processed && j < listIWADCount; j++)
            {
              if (SendMessage(launcher.listIWAD, CB_GETITEMDATA, j, 0)==i)
              {
                if (SendMessage(launcher.listIWAD, CB_SETCURSEL, j, 0) != CB_ERR)
                {
                  processed = true;
                  L_GameOnChange();
                }
              }
            }
          }
        }
        break;
      }
    }
  }

  if (!processed)
    return false;

  listPWADCount = SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);
  for (i = 0; i < listPWADCount; i++)
    SendMessage(launcher.listPWAD, LB_SETSEL, false, i);

  topindex = -1;

  for (k=0; k < numwadfiles; k++)
  {
    if (GetFullPath(wadfiles[k].name, fullpath, PATH_MAX))
    {
      switch (wadfiles[k].src)
      {
      case source_deh:
      case source_pwad:
        processed = false;
        for (j=0; !processed && j < listPWADCount; j++)
        {
          int index = SendMessage(launcher.listPWAD, LB_GETITEMDATA, j, 0);
          if (index != LB_ERR)
          {
            if (!strcasecmp(launcher.files[index].name, fullpath))
              if (SendMessage(launcher.listPWAD, LB_SETSEL, true, j) != CB_ERR)
              {
                if (topindex == -1)
                  topindex = j;
                processed = true;
              }
          }
        }
        if (!processed)
          return false;
        break;
      }
    }
    else
      return false;
  }
  
  if (topindex == -1)
    topindex = 0;
  SendMessage(launcher.listPWAD, LB_SETTOPINDEX, topindex, 0);

  return true;
}

static boolean L_PrepareToLaunch(void)
{
  int i, index, listPWADCount;
  char *history = NULL;
  wadfile_info_t *new_wadfiles=NULL;
  size_t new_numwadfiles = 0;

  new_numwadfiles = numwadfiles;
  new_wadfiles = malloc(sizeof(*wadfiles) * numwadfiles);
  memcpy(new_wadfiles, wadfiles, sizeof(*wadfiles) * numwadfiles);
  numwadfiles = 0;
  free(wadfiles);
  wadfiles = NULL;
  
  listPWADCount = SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);
  
  index = SendMessage(launcher.listIWAD, CB_GETCURSEL, 0, 0);
  if (index != CB_ERR)
  {
    index = SendMessage(launcher.listIWAD, CB_GETITEMDATA, index, 0);
    if (index != CB_ERR)
    {
      char *iwadname = PathFindFileName(launcher.files[index].name);
      char *realiwad = I_FindFile(iwadname, ".wad");
      if (realiwad && *realiwad)
      {
        history = malloc(strlen(iwadname) + 8);
        strcpy(history, iwadname);
        D_AddFile(realiwad,source_iwad);
        free(realiwad);
      }
      //L_DoGameReplace(iwadname);
    }
  }

  if (numwadfiles == 0)
    return false;

  for (i = 0; (size_t)i < new_numwadfiles; i++)
  {
    if (new_wadfiles[i].src == source_auto_load)
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = strdup(new_wadfiles[i].name);
      wadfiles[numwadfiles].src = new_wadfiles[i].src;
      wadfiles[numwadfiles].handle = new_wadfiles[i].handle;
      numwadfiles++;
    }
  }

  for (i=0; i < listPWADCount; i++)
  {
    if (SendMessage(launcher.listPWAD, LB_GETSEL, i, 0) > 0)
    {
      int index;
      fileitem_t *item;
      index = SendMessage(launcher.listPWAD, LB_GETITEMDATA, i, 0);
      item = &launcher.files[index];
      
      if (item->source == source_pwad || item->source == source_iwad)
        D_AddFile(item->name,source_pwad);
      
      if (item->source == source_deh)
        ProcessDehFile(item->name,D_dehout(),0);

      history = realloc(history, strlen(history) + strlen(item->name) + 8);
      strcat(history, "|");
      strcat(history, item->name);
    }
  }
  for (i = 0; (size_t)i < new_numwadfiles; i++)
  {
    if (new_wadfiles[i].src == source_lmp || new_wadfiles[i].src == source_net)
      D_AddFile(new_wadfiles[i].name, new_wadfiles[i].src);
    if (new_wadfiles[i].name)
      free((char*)new_wadfiles[i].name);
  }
  free(new_wadfiles);

  if (history)
  {
    size_t i;
    char str[32];
    default_t *history1, *history2;
    size_t historycount = sizeof(launcher_history)/sizeof(launcher_history[0]);
    size_t shiftfrom = historycount - 1;

    for (i = 0; i < historycount; i++)
    {
      sprintf(str, "launcher_history%d", i);
      history1 = M_LookupDefault(str);

      if (!strcasecmp(history1->location.ppsz[0], history))
      {
        shiftfrom = i;
        break;
      }
    }

    for (i = shiftfrom; i > 0; i--)
    {
      sprintf(str, "launcher_history%d", i);
      history1 = M_LookupDefault(str);
      sprintf(str, "launcher_history%d", i-1);
      history2 = M_LookupDefault(str);

      if (i == shiftfrom)
        free((char*)history1->location.ppsz[0]);
      history1->location.ppsz[0] = history2->location.ppsz[0];
    }
    if (shiftfrom > 0)
    {
      history1 = M_LookupDefault("launcher_history0");
      history1->location.ppsz[0] = history;
    }
  }
  return true;
}

static void L_AddItemToCache(fileitem_t *item)
{
  FILE *fcache;

  if ( (fcache = fopen(launchercachefile, "at")) )
  {
    fprintf(fcache, "%s = %d, %d, %d\n",item->name, item->source, item->doom1, item->doom2);
    fclose(fcache);
  }
}

static void L_ReadCacheData(void)
{
  FILE *fcache;

  if ( (fcache = fopen(launchercachefile, "rt")) )
  {
    fileitem_t item;
    char name[PATH_MAX];

    while (4 == fscanf(fcache, "%s = %d, %d, %d",name, &item.source, &item.doom1, &item.doom2))
    {
      launcher.cache = realloc(launcher.cache, sizeof(*launcher.cache) * (launcher.cachesize + 1));
      
      strcpy(launcher.cache[launcher.cachesize].name, strlwr(name));
      launcher.cache[launcher.cachesize].source = item.source;
      launcher.cache[launcher.cachesize].doom1 = item.doom1;
      launcher.cache[launcher.cachesize].doom2 = item.doom2;
      
      launcher.cachesize++;
    }
    fclose(fcache);
  }
}

static void L_FillGameList(void)
{
  extern const int nstandard_iwads;
  extern const char *const standard_iwads[];

  int i, j;
  
  //"doom2f.wad", "doom2.wad", "plutonia.wad", "tnt.wad", "doom.wad", "doom1.wad", "doomu.wad", "freedoom.wad"
  const char *IWADTypeNames[] =
  {
    "DOOM 2: French Version",
    "DOOM 2: Hell on Earth",
    "DOOM 2: Plutonia Experiment",
    "DOOM 2: TNT - Evilution",
    "DOOM Registered",
    "DOOM Shareware",
    "The Ultimate DOOM",
    "Freedoom",
  };
  
  for (i = 0; (size_t)i < launcher.filescount; i++)
  {
    fileitem_t *item = &launcher.files[i];
    if (item->source == source_iwad)
    {
      for (j=0; j < nstandard_iwads; j++)
      {
        if (!strcasecmp(PathFindFileName(item->name), standard_iwads[j]))
        {
          char iwadname[128];
          int index;
          sprintf(iwadname, "%s (%s)", IWADTypeNames[j], standard_iwads[j]);
          index = SendMessage(launcher.listIWAD, CB_ADDSTRING, 0, (LPARAM)iwadname);
          if (index >= 0)
            SendMessage(launcher.listIWAD, CB_SETITEMDATA, index, (LPARAM)i);
        }
      }
    }
  }
}

static void L_FillFilesList(fileitem_t *iwad)
{
  int index;
  size_t i;
  fileitem_t *item;

  SendMessage(launcher.listPWAD, LB_RESETCONTENT, 0, 0);

  for (i = 0; i < launcher.filescount; i++)
  {
    item = &launcher.files[i];
    if (iwad->doom1 && item->doom1 || iwad->doom2 && item->doom2 ||
      (!item->doom1 && !item->doom2) ||
      item->source == source_deh)
    {
      index = SendMessage(launcher.listPWAD, LB_ADDSTRING, 0, (LPARAM)strlwr(PathFindFileName(item->name)));
      if (index >= 0)
        SendMessage(launcher.listPWAD, LB_SETITEMDATA, index, i);
    }

  }
}

int GetFullPath(const char* FileName, char *Buffer, size_t BufferLength)
{
  int i, Result;
  char *p;
  char dir[PATH_MAX];
  
  for (i=0; i<3; i++)
  {
    switch(i)
    {
    case 0:
      getcwd(dir, sizeof(dir));
      break;
    case 1:
      if (!getenv("DOOMWADDIR"))
        continue;
      strcpy(dir, getenv("DOOMWADDIR"));
      break;
    case 2:
      strcpy(dir, I_DoomExeDir());
      break;
    }

    Result = SearchPath(dir,FileName,NULL,BufferLength,Buffer,&p);
    if (Result)
      return Result;
  }

  return false;
}

char* e6y_I_FindFile(const char* ext)
{
  int i;
  /* Precalculate a length we will need in the loop */
  size_t  pl = strlen(ext) + 4;

  for (i=0; i<8; i++) {
    char  * p;
    char d[PATH_MAX];
    const char  * s = NULL;
    strcpy(d, "");
    /* Each entry in the switch sets d to the directory to look in,
     * and optionally s to a subdirectory of d */
    switch(i) {
    case 1:
      if (!getenv("DOOMWADDIR"))
        continue;
      strcpy(d, getenv("DOOMWADDIR"));
      break;
    case 0:
      getcwd(d, sizeof(d));
      break;
    case 2:
      strcpy(d, DOOMWADDIR);
      break;
    case 4:
      strcpy(d, "/usr/share/games/doom");
      break;
    case 5:
      strcpy(d, "/usr/local/share/games/doom");
      break;
    case 6:
      strcpy(d, I_DoomExeDir());
      break;
    case 3:
      s = "doom";
    case 7:
      if (!getenv("HOME"))
        continue;
      strcpy(d, getenv("HOME"));
      break;
    }

    p = malloc(strlen(d) + (s ? strlen(s) : 0) + pl);
    sprintf(p, "%s%s%s%s", d, (d && !HasTrailingSlash(d)) ? "\\" : "",
                             s ? s : "", (s && !HasTrailingSlash(s)) ? "\\" : "");

    {
      void *handle;
      findstate_t findstate;
      char fullmask[PATH_MAX];

      sprintf(fullmask, "%s%s", (p?p:""), ext);
      
      if ((handle = I_FindFirst(fullmask, &findstate)) != (void *)-1)
      {
        do
        {
          if (!(I_FindAttr (&findstate) & FA_DIREC))
          {
            fileitem_t item;
            char fullpath[PATH_MAX];
            
            sprintf(fullpath, "%s%s", (p?p:""), I_FindName(&findstate));
            
            if (L_GetFileType(fullpath, &item))
            {
              if (item.source != source_err)
              {
                size_t j;
                boolean present = false;
                for (j = 0; !present && j < launcher.filescount; j++)
                  present = !strcasecmp(launcher.files[j].name, fullpath);

                if (!present)
                {
                  launcher.files = realloc(launcher.files, sizeof(*launcher.files) * (launcher.filescount + 1));
                  
                  strcpy(launcher.files[launcher.filescount].name, fullpath);
                  launcher.files[launcher.filescount].source = item.source;
                  launcher.files[launcher.filescount].doom1 = item.doom1;
                  launcher.files[launcher.filescount].doom2 = item.doom2;
                  launcher.filescount++;
                }
              }
            }
          }
        }
        while (I_FindNext (handle, &findstate) == 0);
        I_FindClose (handle);
      }
    }

    free(p);
  }
  return NULL;
}

static char* L_HistoryGetStr(wadfiles_t *data)
{
  size_t i;
  char *iwad = NULL;
  char *pwad = NULL;
  char *deh = NULL;
  char **str;
  char *result;
  int len;

  for (i = 0; i < data->numwadfiles; i++)
  {
    str = NULL;
    switch (data->wadfiles[i].src)
    {
    case source_iwad: str = &iwad; break;
    case source_pwad: str = &pwad; break;
    case source_deh:  str = &deh;  break;
    }
    if (*str)
    {
      *str = realloc(*str, strlen(*str) + strlen(data->wadfiles[i].name) + 8);
      strcat(*str, " + ");
      strcat(*str, data->wadfiles[i].name);
    }
    else
    {
      *str = malloc(strlen(data->wadfiles[i].name) + 8);
      strcpy(*str, data->wadfiles[i].name);
    }
  }

  len = 0;
  if (iwad) len += strlen(iwad);
  if (pwad) len += strlen(pwad);
  if (deh)  len += strlen(deh);
  
  result = malloc(len + 16);
  strcpy(result, "");
  
  if (pwad)
  {
    strcat(result, strlwr(PathFindFileName(pwad)));
    if (deh)
      strcat(result, " + ");
    free(pwad);
  }
  if (deh)
  {
    strcat(result, strlwr(PathFindFileName(deh)));
    free(deh);
  }
  if (iwad)
  {
    strcat(result, " @ ");
    strcat(result, strupr(PathFindFileName(iwad)));
    free(iwad);
  }

  return result;
}

static wadfiles_t* L_HistoryGetData(const char *str)
{
  wadfile_info_t *wadfiles = NULL;
  wadfiles_t *data;
  size_t numwadfiles = 0;
  char *pStr = strdup(str);
  char *pToken = pStr;

  for (;(pToken = strtok(pToken,"|"));pToken = NULL)
  {
    wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));

    wadfiles[numwadfiles].name =
      AddDefaultExtension(strcpy(malloc(strlen(pToken)+5), pToken), ".wad");

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
      if (!strcasecmp(&p[len-4],".deh"))
        wadfiles[numwadfiles].src = source_deh;
    }
    numwadfiles++;
  }

  data = malloc(sizeof(*data));
  data->wadfiles = wadfiles;
  data->numwadfiles = numwadfiles;

  free(pStr);

  return data;
}

static void L_FreeWadFiles(wadfiles_t *wadfiles)
{
  if (wadfiles)
  {
    if (wadfiles->wadfiles)
    {
      free(wadfiles->wadfiles);
      wadfiles->wadfiles = NULL;
    }
    free(wadfiles);
    wadfiles = NULL;
  }
}

static void L_FillHistoryList(void)
{
  int i, count;
  char *p = NULL;

  count = SendMessage(launcher.listHistory, CB_GETCOUNT, 0, 0);

  if (count != CB_ERR)
  {
    for (i = 0; i < count; i++)
    {
      wadfiles_t *data;
      data = (wadfiles_t*)SendMessage(launcher.listHistory, CB_GETITEMDATA, i, 0);
      if (data)
      {
        if (data->wadfiles)
          free(data->wadfiles);
        free(data);
      }
    }
  }
  
  SendMessage(launcher.listHistory, CB_RESETCONTENT, 0, 0);

  for (i = 0; i < sizeof(launcher_history)/sizeof(launcher_history[0]); i++)
  {
    if (strlen(launcher_history[i]) > 0)
    {
      int index;
      wadfiles_t *data;
      char *str = strdup(launcher_history[i]);

      data = L_HistoryGetData(str);
      p = L_HistoryGetStr(data);

      if (p)
      {
        index = SendMessage(launcher.listHistory, CB_ADDSTRING, 0, (LPARAM)p);
        if (index >= 0)
          SendMessage(launcher.listHistory, CB_SETITEMDATA, index, (LPARAM)data);
        
        free(p);
        p = NULL;
      }

      free(str);
    }
  }
}

static boolean L_DemoAutoDetect(void)
{
  boolean res = false;
  int demoindex;
  for (demoindex = 0; (size_t)demoindex < numwadfiles; demoindex++)
  {
    if (wadfiles[demoindex].src == source_lmp)
    {
      int i;
      unsigned int maxlen = 0;
      char *pattern;
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
            result = regexec(&preg, wadfiles[demoindex].name, 1, &demo_match[0], REG_NOTBOL);
            if (result == 0)
            {
              char path[MAX_PATH];
              wadfiles_t *data = L_HistoryGetData(buf + pmatch[3].rm_so);
              L_GUISelect(data->wadfiles, data->numwadfiles);
              len = min(pmatch[1].rm_eo - pmatch[1].rm_so, sizeof(path) - sizeof(path[0]));
              strncpy(path, buf, len);
              path[len] = '\0';
              SendMessage(launcher.staticFileName, WM_SETTEXT, 0, (LPARAM)path);
              L_FreeWadFiles(data);
              res = true;
              break;
            }
          }
          regfree(&preg);
        }
      }
      free(pattern);
      break;
    }
  }
  return res;
}

BOOL CALLBACK LauncherClientCallback (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
  case WM_INITDIALOG:
    {
      HMODULE hMod;

      launcher.HWNDClient = hDlg;
      launcher.listIWAD = GetDlgItem(launcher.HWNDClient, IDC_IWADCOMBO);
      launcher.listPWAD = GetDlgItem(launcher.HWNDClient, IDC_PWADLIST);
      launcher.listHistory = GetDlgItem(launcher.HWNDClient, IDC_HISTORYCOMBO);
      launcher.listCMD = GetDlgItem(launcher.HWNDClient, IDC_COMMANDCOMBO);
      launcher.staticFileName = GetDlgItem(launcher.HWNDClient, IDC_FULLFILENAMESTATIC);
      
      hMod = LoadLibrary("uxtheme.dll");
      if (hMod)
      {
        EnableThemeDialogTexturePROC pEnableThemeDialogTexture;
        pEnableThemeDialogTexture = (EnableThemeDialogTexturePROC)GetProcAddress(hMod, "EnableThemeDialogTexture");
        if (pEnableThemeDialogTexture)
          pEnableThemeDialogTexture(hDlg, ETDT_ENABLETAB);
        FreeLibrary(hMod);
      }

      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Rebuild the PrBoom-Plus cache");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Clear all Launcher's history");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Associate the current EXE with DOOM demos");

      {
        char buf[128];
        strcpy(buf, (launcher_enable ? "Disable" : "Enable"));
        strcat(buf, " this Launcher for future use");
        SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)buf);
      }

      SendMessage(launcher.listCMD, CB_SETCURSEL, -1, 0);
      L_CommandOnChange();

      L_ReadCacheData();
      
      e6y_I_FindFile("*.wad");
      e6y_I_FindFile("*.deh");

      L_FillGameList();
      L_FillHistoryList();

      if (L_DemoAutoDetect())
      {
        // do nothing
      }
      else if (SendMessage(launcher.listHistory, CB_SETCURSEL, 0, 0) != CB_ERR)
      {
        L_HistoryOnChange();
        SetFocus(launcher.listHistory);
      }
      else if (SendMessage(launcher.listIWAD, CB_SETCURSEL, 0, 0) != CB_ERR)
      {
        L_GameOnChange();
        SetFocus(launcher.listPWAD);
      }
    }
		break;

  case WM_COMMAND:
    {
      int wmId    = LOWORD(wParam);
      int wmEvent = HIWORD(wParam);

      if (wmId == IDC_PWADLIST && wmEvent == LBN_DBLCLK)
      {
        if (L_PrepareToLaunch())
          EndDialog (launcher.HWNDServer, 1);
      }
      
      if (wmId == IDC_HISTORYCOMBO && wmEvent == CBN_SELCHANGE)
        L_HistoryOnChange();
      
      if (wmId == IDC_IWADCOMBO && wmEvent == CBN_SELCHANGE)
        L_GameOnChange();
      
      if (wmId == IDC_PWADLIST && wmEvent == LBN_SELCHANGE)
        L_FilesOnChange();

      if ((wmId == IDC_IWADCOMBO && wmEvent == CBN_SELCHANGE) ||
        (wmId == IDC_PWADLIST && wmEvent == LBN_SELCHANGE))
        SendMessage(launcher.listHistory, CB_SETCURSEL, -1, 0);

      if (wmId == IDC_COMMANDCOMBO && wmEvent == CBN_SELCHANGE)
        L_CommandOnChange();
    }
    break;
	}
	return FALSE;
  }

BOOL CALLBACK LauncherServerCallback (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  
  switch (message)
  {
  case WM_COMMAND:
    wmId    = LOWORD(wParam);
    wmEvent = HIWORD(wParam);
    switch (wmId)
    {
    case IDCANCEL:
      EndDialog (hWnd, 0);
      break;
    case IDOK:
      if (L_PrepareToLaunch())
        EndDialog (hWnd, 1);
      break;
    }
    break;
    
  case WM_INITDIALOG:
      launcher.HWNDServer = hWnd;
      CreateDialogParam(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(IDD_LAUNCHERCLIENTDIALOG), 
        launcher.HWNDServer,
        LauncherClientCallback, 0);
      break;
      
  //case WM_DESTROY:
  //  PostQuitMessage(0);
  //  break;
  }
  return 0;
}

static boolean L_LauncherIsNeeded(void)
{
  int i;
  boolean pwad = false;
  char *iwad = NULL;


//  SHIFT for invert
//  if (GetAsyncKeyState(VK_SHIFT) ? launcher_enable : !launcher_enable)
//    return false;

  if (!launcher_enable && !GetAsyncKeyState(VK_SHIFT))
    return false;

  i = M_CheckParm("-iwad");
  if (i && (++i < myargc))
    iwad = I_FindFile(myargv[i], ".wad");

  for (i=0; !pwad && i < (int)numwadfiles; i++)
    pwad = wadfiles[i].src == source_pwad;

  return (!iwad && !pwad);
}

void LauncherShow(void)
{
  int result;
  int nCmdShow = SW_SHOW;

  if (!L_LauncherIsNeeded())
    return;

  InitCommonControls();
  sprintf(launchercachefile,"%s/prboom-plus.cache", I_DoomExeDir());

  result = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LAUNCHERSERVERDIALOG), NULL, (DLGPROC)LauncherServerCallback);

  switch (result)
  {
  case 0:
    I_SafeExit(-1);
    break;
  case 1:
    M_SaveDefaults();
    break;
  }
}

#endif // _WIN32
