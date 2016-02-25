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
#include "pcreposix.h"
#include "r_demo.h"
#include "e6y.h"
#include "e6y_launcher.h"

#pragma comment( lib, "comctl32.lib" )
#pragma comment( lib, "advapi32.lib" )

#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)
typedef HRESULT (WINAPI *EnableThemeDialogTexturePROC)(HWND, DWORD);

#define FA_DIREC	0x00000010
#define LAUNCHER_HISTORY_SIZE 10

#define LAUNCHER_CAPTION PACKAGE_NAME" Launcher"

typedef struct
{
  char name[PATH_MAX];
  wad_source_t source;
  dboolean doom1;
  dboolean doom2;
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
  
  int *selection;
  size_t selectioncount;
} launcher_t;

launcher_t launcher;

launcher_enable_t launcher_enable;
const char *launcher_enable_states[launcher_enable_count] = {"never", "smart", "always"};
char *launcher_history[LAUNCHER_HISTORY_SIZE];

static char launchercachefile[PATH_MAX];

unsigned int launcher_params;

//global
void CheckIWAD(const char *iwadname,GameMode_t *gmode,dboolean *hassec);
void ProcessDehFile(const char *filename, const char *outfilename, int lumpnum);
const char *D_dehout(void);

//tooltip
HWND g_hwndTT;
HHOOK g_hhk;
BOOL DoCreateDialogTooltip(void);
BOOL CALLBACK EnumChildProc(HWND hwndCtrl, LPARAM lParam);
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID OnWMNotify(LPARAM lParam) ;

//common
void *I_FindFirst (const char *filespec, findstate_t *fileinfo);
int I_FindNext (void *handle, findstate_t *fileinfo);
int I_FindClose (void *handle);
char *strrtrm (char *Str);

//events
static void L_GameOnChange(void);
static void L_FilesOnChange(void);
static void L_HistoryOnChange(void);
static void L_CommandOnChange(void);

static void L_FillGameList(void);
static void L_FillFilesList(fileitem_t *iwad);
static void L_FillHistoryList(void);

static char* L_HistoryGetStr(waddata_t *data);
static void L_HistoryFreeData(void);

static void L_ReadCacheData(void);

//selection
static void L_SelAdd(int index);
static void L_SelClearAndFree(void);
static int L_SelGetList(int **list);

static dboolean L_GetFileType(const char *filename, fileitem_t *item);
static dboolean L_PrepareToLaunch(void);

static dboolean L_GUISelect(waddata_t *waddata);
static dboolean L_LauncherIsNeeded(void);

static void L_FillFilesList(fileitem_t *iwad);
static void L_AddItemToCache(fileitem_t *item);

char* e6y_I_FindFile(const char* ext);

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

#define prb_isspace(c) ((c) == 0x20)
char *strrtrm (char *str)
{
  if (str)
  {
    char *p = str + strlen (str)-1;
    while (p >= str && prb_isspace((unsigned char) *p))
      p--;
    *++p = 0;
  }
  return str;
}
#undef prb_isspace


//events
static void L_GameOnChange(void)
{
  int index;

  index = (int)SendMessage(launcher.listIWAD, CB_GETCURSEL, 0, 0);
  if (index != CB_ERR)
  {
    index = (int)SendMessage(launcher.listIWAD, CB_GETITEMDATA, index, 0);
    if (index != CB_ERR)
    {
      L_FillFilesList(&launcher.files[index]);
    }
  }
}

static void L_FilesOnChange(void)
{
  int index;
  int i, start, end;

  // блядь, как заебал этот винапи...
  start = (int)SendMessage(launcher.listPWAD, LB_GETANCHORINDEX, 0, 0);
  end = (int)SendMessage(launcher.listPWAD, LB_GETCARETINDEX, 0, 0);
  
  for (i = start; (start<end?(i<=end):(i>=end)); (start<end?i++:i--))
  {
    if (SendMessage(launcher.listPWAD, LB_GETSEL, i, 0) > 0)
    {
      index = (int)SendMessage(launcher.listPWAD, LB_GETITEMDATA, i, 0);
      if (index != LB_ERR)
      {
        L_SelAdd(index);
      }
    }
  }
  
  index = (int)SendMessage(launcher.listPWAD, LB_GETCURSEL, 0, 0);
  if (index != LB_ERR)
  {
    index = (int)SendMessage(launcher.listPWAD, LB_GETITEMDATA, index, 0);
    if (index != LB_ERR)
    {
      char path[PATH_MAX];
      size_t count;
      RECT rect;
      HFONT font, oldfont;
      HDC hdc;

      strcpy(path, launcher.files[index].name);
      NormalizeSlashes2(path);
      M_Strlwr(path);

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

  index = (int)SendMessage(launcher.listHistory, CB_GETCURSEL, 0, 0);
  if (index >= 0)
  {
    waddata_t *waddata;
    waddata = (waddata_t*)SendMessage(launcher.listHistory, CB_GETITEMDATA, index, 0);
    if ((int)waddata != CB_ERR)
    {
      if (!L_GUISelect(waddata))
      {
        SendMessage(launcher.listHistory, CB_SETCURSEL, -1, 0);
      }
    }
  }
}

static DWORD L_Associate(const char *Name, const char *Ext, const char *cmdline)
{
  HKEY hKeyRoot, hKey;
  DWORD result;

  hKeyRoot = HKEY_CLASSES_ROOT;

  // This creates a Root entry called 'Name'
  result = RegCreateKey(hKeyRoot, Name, &hKey);
  if (result != ERROR_SUCCESS) return result;
  result = RegSetValue(hKey, "", REG_SZ, "PrBoom-Plus", 0);
  if (result != ERROR_SUCCESS) return result;
  RegCloseKey(hKey);

  // This creates a Root entry called 'Ext' associated with 'Name'
  result = RegCreateKey(hKeyRoot, Ext, &hKey);
  if (result != ERROR_SUCCESS) return result;
  result = RegSetValue(hKey, "", REG_SZ, Name, 0);
  if (result != ERROR_SUCCESS) return result;
  RegCloseKey(hKey);

  // This sets the command line for 'Name'
  result = RegCreateKey(hKeyRoot, Name, &hKey);
  if (result != ERROR_SUCCESS) return result;
  result = RegSetValue(hKey, "shell\\open\\command", REG_SZ, cmdline, strlen(cmdline) + 1);
  if (result != ERROR_SUCCESS) return result;
  RegCloseKey(hKey);

  return result;
}
static void L_CommandOnChange(void)
{
  int index;

  index = (int)SendMessage(launcher.listCMD, CB_GETCURSEL, 0, 0);
  
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
    e6y_I_FindFile("*.bex");

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
  case 3:
  case 4:
    {
      DWORD result;
      char *msg;
      char *cmdline;

      cmdline = malloc(strlen(*myargv) + 100);

      if (cmdline)
      {
        sprintf(cmdline, "\"%s\" \"%%1\"", *myargv);

        result = 0;
        if (index == 2)
          result = L_Associate("PrBoomPlusWadFiles", ".wad", cmdline);
        if (index == 3)
          result = L_Associate("PrBoomPlusLmpFiles", ".lmp", cmdline);
        if (index == 4)
        {
          strcat(cmdline, " -auto");
          result = L_Associate("PrBoomPlusLmpFiles", ".lmp", cmdline);
        }

        free(cmdline);

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&msg, 512, NULL))
        {
          MessageBox(launcher.HWNDServer, msg, LAUNCHER_CAPTION,
            MB_OK | (result == ERROR_SUCCESS ? MB_ICONASTERISK : MB_ICONEXCLAMATION));
          LocalFree(msg);
        }
      }
    }
    break;

  case 5:
    {
      char buf[128], next_mode[100];
      launcher_enable_t launcher_next_mode = (launcher_enable + 1) % launcher_enable_count;
      
      if (launcher_next_mode == launcher_enable_never)
        strcpy(next_mode, "disable");
      if (launcher_next_mode == launcher_enable_smart)
        strcpy(next_mode, "enable ('smart' mode)");
      if (launcher_next_mode == launcher_enable_always)
        strcpy(next_mode, "enable ('always' mode)");

      sprintf(buf, "Do you really want to %s the Launcher?", next_mode);
      if (MessageBox(launcher.HWNDServer, buf, LAUNCHER_CAPTION, MB_YESNO|MB_ICONQUESTION) == IDYES)
      {
        launcher_enable = launcher_next_mode;

        SendMessage(launcher.listCMD, CB_DELETESTRING, index, (LPARAM)buf);
        strcpy(buf, ((launcher_enable + 1) % launcher_enable_count == launcher_enable_never ? "Disable" : "Enable"));
        strcat(buf, " this Launcher for future use");
        SendMessage(launcher.listCMD, CB_INSERTSTRING, index, (LPARAM)buf);

        M_SaveDefaults();
        sprintf(buf, "Successfully %s", (launcher_enable != launcher_enable_never ? "enabled" : "disabled"));
        MessageBox(launcher.HWNDServer, buf, LAUNCHER_CAPTION, MB_OK|MB_ICONEXCLAMATION);
      }
    }
    break;
  }
  
  SendMessage(launcher.listCMD, CB_SETCURSEL, -1, 0);
}

static dboolean L_GetFileType(const char *filename, fileitem_t *item)
{
  size_t i, len;
  wadinfo_t header;
  FILE *f;

  item->source = source_err;
  item->doom1 = false;
  item->doom2 = false;
  strcpy(item->name, filename);
  
  len = strlen(filename);

  if (!strcasecmp(&filename[len-4],".deh") || !strcasecmp(&filename[len-4],".bex"))
  {
    item->source = source_deh;
    return true;
  }
  
  for (i = 0; i < launcher.cachesize; i++)
  {
    if (!strcasecmp(filename, launcher.cache[i].name))
    {
      strcpy(item->name, launcher.cache[i].name);
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
      header.numlumps = LittleLong(header.numlumps);
      if (0 == fseek(f, LittleLong(header.infotableofs), SEEK_SET))
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

static dboolean L_GUISelect(waddata_t *waddata)
{
  int i, j;
  size_t k;
  int topindex;
  dboolean processed = false;
  int listIWADCount, listPWADCount;
  char fullpath[PATH_MAX];
  
  if (!waddata->wadfiles)
    return false;

  listIWADCount = (int)SendMessage(launcher.listIWAD, CB_GETCOUNT, 0, 0);
  SendMessage(launcher.listIWAD, CB_SETCURSEL, -1, 0);

  for (k=0; !processed && k < waddata->numwadfiles; k++)
  {
    if (GetFullPath(waddata->wadfiles[k].name, NULL, fullpath, PATH_MAX))
    {
      switch (waddata->wadfiles[k].src)
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

  //no iwad?
  if (!processed)
    return false;

  listPWADCount = (int)SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);
  for (i = 0; i < listPWADCount; i++)
    SendMessage(launcher.listPWAD, LB_SETSEL, false, i);

  topindex = -1;

  for (k=0; k < waddata->numwadfiles; k++)
  {
    if (GetFullPath(waddata->wadfiles[k].name, NULL, fullpath, PATH_MAX))
    {
      switch (waddata->wadfiles[k].src)
      {
      case source_deh:
      case source_pwad:
        processed = false;
        for (j=0; !processed && j < listPWADCount; j++)
        {
          int index = (int)SendMessage(launcher.listPWAD, LB_GETITEMDATA, j, 0);
          if (index != LB_ERR)
          {
            if (!strcasecmp(launcher.files[index].name, fullpath))
              if (SendMessage(launcher.listPWAD, LB_SETSEL, true, j) != CB_ERR)
              {
                if (topindex == -1)
                  topindex = j;
                L_SelAdd(index);
                processed = true;
              }
          }
        }
        if (!processed)
          return false;
        break;
      }
    }
    //else
    //  return false;
  }
  
  if (topindex == -1)
    topindex = 0;
  SendMessage(launcher.listPWAD, LB_SETTOPINDEX, topindex, 0);

  return true;
}

static dboolean L_PrepareToLaunch(void)
{
  int i, index, listPWADCount;
  char *history = NULL;
  wadfile_info_t *new_wadfiles=NULL;
  size_t new_numwadfiles = 0;
  int *selection = NULL;
  int selectioncount = 0;

  new_numwadfiles = numwadfiles;
  new_wadfiles = malloc(sizeof(*wadfiles) * numwadfiles);
  memcpy(new_wadfiles, wadfiles, sizeof(*wadfiles) * numwadfiles);
  numwadfiles = 0;
  free(wadfiles);
  wadfiles = NULL;
  
  listPWADCount = (int)SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);
  
  index = (int)SendMessage(launcher.listIWAD, CB_GETCURSEL, 0, 0);
  if (index != CB_ERR)
  {
    index = (int)SendMessage(launcher.listIWAD, CB_GETITEMDATA, index, 0);
    if (index != CB_ERR)
    {
      char *iwadname = PathFindFileName(launcher.files[index].name);
      history = malloc(strlen(iwadname) + 8);
      strcpy(history, iwadname);
      AddIWAD(launcher.files[index].name);
    }
  }

  if (numwadfiles == 0)
    return false;

  for (i = 0; (size_t)i < new_numwadfiles; i++)
  {
    if (new_wadfiles[i].src == source_auto_load || new_wadfiles[i].src == source_pre)
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = strdup(new_wadfiles[i].name);
      wadfiles[numwadfiles].src = new_wadfiles[i].src;
      wadfiles[numwadfiles].handle = new_wadfiles[i].handle;
      numwadfiles++;
    }
  }

  selectioncount = L_SelGetList(&selection);

  for (i=0; i < selectioncount; i++)
  {
    int index = selection[i];
    fileitem_t *item = &launcher.files[index];

    if (item->source == source_pwad || item->source == source_iwad)
    {
      D_AddFile(item->name, source_pwad);
      modifiedgame = true;
    }

    if (item->source == source_deh)
      ProcessDehFile(item->name, D_dehout(),0);

    history = realloc(history, strlen(history) + strlen(item->name) + 8);
    strcat(history, "|");
    strcat(history, item->name);
  }
 
  free(selection);
  L_SelClearAndFree();

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

    while (fgets(name, sizeof(name), fcache))
    {
      char *p = strrchr(name, '=');
      if (p)
      {
        *p = 0;
        if (3 == sscanf(p + 1, "%d, %d, %d", &item.source, &item.doom1, &item.doom2))
        {
          launcher.cache = realloc(launcher.cache, sizeof(*launcher.cache) * (launcher.cachesize + 1));
          strcpy(launcher.cache[launcher.cachesize].name, M_Strlwr(strrtrm(name)));
          launcher.cache[launcher.cachesize].source = item.source;
          launcher.cache[launcher.cachesize].doom1 = item.doom1;
          launcher.cache[launcher.cachesize].doom2 = item.doom2;
          
          launcher.cachesize++;
        }
      }
    }
    fclose(fcache);
  }
}

static void L_SelAdd(int index)
{
  launcher.selection = realloc(launcher.selection, 
    sizeof(launcher.selection[0]) * (launcher.selectioncount + 1));
  launcher.selection[launcher.selectioncount] = index;
  launcher.selectioncount++;
}

static void L_SelClearAndFree(void)
{
  free(launcher.selection);
  launcher.selection = NULL;
  launcher.selectioncount = 0;
}

static int L_SelGetList(int **list)
{
  int i, j, count = 0;
  int listPWADCount = (int)SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);

  *list = NULL;

  for (i = launcher.selectioncount - 1; i >= 0; i--)
  {
    dboolean present = false;
    for (j = 0; j < count && !present; j++)
    {
      present = (*list)[j] == launcher.selection[i];
    }
    
    if (!present)
    {
      for (j=0; j < listPWADCount; j++)
      {
        int index = launcher.selection[i];
        if (SendMessage(launcher.listPWAD, LB_GETITEMDATA, j, 0) == index)
        {
          if (SendMessage(launcher.listPWAD, LB_GETSEL, j, 0) > 0)
          {
            *list = realloc(*list, sizeof(int) * (count + 1));
            (*list)[count++] = launcher.selection[i];
          }
        }
      }
    }
  }

  for (i = 0; i < count / 2; i++)
  {
    int tmp = (*list)[i];
    (*list)[i] = (*list)[count - 1 - i];
    (*list)[count - 1 - i] = tmp;
  }

  return count;
}

static void L_FillGameList(void)
{
  extern const int nstandard_iwads;
  extern const char *const standard_iwads[];

  int i, j;
  
  // "doom2f.wad", "doom2.wad", "plutonia.wad", "tnt.wad",
  // "doom.wad", "doom1.wad", "doomu.wad",
  // "freedoom2.wad", "freedoom1.wad", "freedm.wad"
  // "hacx.wad", "chex.wad"
  // "bfgdoom2.wad", "bfgdoom.wad"
  const char *IWADTypeNames[] =
  {
    "DOOM 2: French Version",
    "DOOM 2: Hell on Earth",
    "DOOM 2: Plutonia Experiment",
    "DOOM 2: TNT - Evilution",

    "DOOM Registered",
    "DOOM Shareware",
    "The Ultimate DOOM",

    "Freedoom: Phase 2",
    "Freedoom: Phase 1",
    "FreeDM",

    "HACX - Twitch 'n Kill",
    "Chex(R) Quest",

    "DOOM 2: BFG Edition",
    "DOOM 1: BFG Edition",
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
          index = (int)SendMessage(launcher.listIWAD, CB_ADDSTRING, 0, (LPARAM)iwadname);
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
      index = (int)SendMessage(launcher.listPWAD, LB_ADDSTRING, 0, (LPARAM)M_Strlwr(PathFindFileName(item->name)));
      if (index >= 0)
      {
        SendMessage(launcher.listPWAD, LB_SETITEMDATA, index, i);
      }
    }

  }
}

char* e6y_I_FindFile(const char* ext)
{
  int i;
  /* Precalculate a length we will need in the loop */
  size_t  pl = strlen(ext) + 4;

  for (i=0; i<3; i++) {
    char  * p;
    char d[PATH_MAX];
    const char  * s = NULL;
    strcpy(d, "");
    switch(i) {
    case 0:
      getcwd(d, sizeof(d));
      break;
    case 1:
      if (!getenv("DOOMWADDIR"))
        continue;
      strcpy(d, getenv("DOOMWADDIR"));
      break;
    case 2:
      strcpy(d, I_DoomExeDir());
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
                dboolean present = false;
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

static char* L_HistoryGetStr(waddata_t *data)
{
  size_t i;
  char *iwad = NULL;
  char *pwad = NULL;
  char *deh = NULL;
  char **str;
  char *result;
  size_t len;

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
      strcat(*str, PathFindFileName(data->wadfiles[i].name));
    }
    else
    {
      *str = malloc(strlen(data->wadfiles[i].name) + 8);
      strcpy(*str, PathFindFileName(data->wadfiles[i].name));
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
    strcat(result, M_Strlwr(pwad));
    if (deh)
      strcat(result, " + ");
    free(pwad);
  }
  if (deh)
  {
    strcat(result, M_Strlwr(deh));
    free(deh);
  }
  if (iwad)
  {
    strcat(result, " @ ");
    strcat(result, M_Strupr(iwad));
    free(iwad);
  }

  return result;
}

static void L_HistoryFreeData(void)
{
  int i, count;
  count = (int)SendMessage(launcher.listHistory, CB_GETCOUNT, 0, 0);
  if (count != CB_ERR)
  {
    for (i = 0; i < count; i++)
    {
      waddata_t *waddata = (waddata_t*)SendMessage(launcher.listHistory, CB_GETITEMDATA, i, 0);
      if ((int)waddata != CB_ERR)
      {
        WadDataFree(waddata);
      }
    }
  }
}

static void L_FillHistoryList(void)
{
  int i;
  char *p = NULL;

  L_HistoryFreeData();
  
  SendMessage(launcher.listHistory, CB_RESETCONTENT, 0, 0);

  for (i = 0; i < sizeof(launcher_history)/sizeof(launcher_history[0]); i++)
  {
    if (strlen(launcher_history[i]) > 0)
    {
      int index;
      char *str = strdup(launcher_history[i]);
      waddata_t *waddata = malloc(sizeof(*waddata));
      memset(waddata, 0, sizeof(*waddata));

      ParseDemoPattern(str, waddata, NULL, false);
      p = L_HistoryGetStr(waddata);

      if (p)
      {
        index = (int)SendMessage(launcher.listHistory, CB_ADDSTRING, 0, (LPARAM)p);
        if (index >= 0)
          SendMessage(launcher.listHistory, CB_SETITEMDATA, index, (LPARAM)waddata);
        
        free(p);
        p = NULL;
      }

      free(str);
    }
  }
}

BOOL CALLBACK LauncherClientCallback (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
  case WM_INITDIALOG:
    {
      int i;
      HMODULE hMod;
      waddata_t data;

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

      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Rebuild the "PACKAGE_NAME" cache");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Clear all Launcher's history");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"Associate the current EXE with DOOM wads");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"... with DOOM demos");
      SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)"... with DOOM demos (-auto mode)");

      {
        char buf[128];
        strcpy(buf, ((launcher_enable + 1) % launcher_enable_count == launcher_enable_never ? "Disable" : "Enable"));
        strcat(buf, " this Launcher for future use");
        SendMessage(launcher.listCMD, CB_ADDSTRING, 0, (LPARAM)buf);
      }

      DoCreateDialogTooltip();

      SendMessage(launcher.listCMD, CB_SETCURSEL, -1, 0);
      L_CommandOnChange();

      L_ReadCacheData();
      
      e6y_I_FindFile("*.wad");
      e6y_I_FindFile("*.deh");
      e6y_I_FindFile("*.bex");

      L_FillGameList();
      L_FillHistoryList();

      i = -1;
      if (launcher_params)
      {
        WadDataInit(&data);
        WadFilesToWadData(&data);
        L_GUISelect(&data);
      }
      else
      {
#ifdef HAVE_LIBPCREPOSIX
        for (i = 0; (size_t)i < numwadfiles; i++)
        {
          if (wadfiles[i].src == source_lmp)
          {
            patterndata_t patterndata;
            memset(&patterndata, 0, sizeof(patterndata));

            if (DemoNameToWadData(wadfiles[i].name, &data, &patterndata))
            {
              L_GUISelect(&data);
              SendMessage(launcher.staticFileName, WM_SETTEXT, 0, (LPARAM)patterndata.pattern_name);
              WadDataFree(&data);
              break;
            }
            free(patterndata.missed);
          }
        }
#endif
      }
      
      if ((size_t)i == numwadfiles)
      {
        if (SendMessage(launcher.listHistory, CB_SETCURSEL, 0, 0) != CB_ERR)
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
    }
		break;

  case WM_NOTIFY:
    OnWMNotify(lParam);
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
      {
        EndDialog (hWnd, 1);
      }
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
      
  case WM_DESTROY:
    L_HistoryFreeData();
    break;
  }
  return 0;
}

// DoCreateDialogTooltip - creates a tooltip control for a dialog box,
//   enumerates the child control windows, and installs a hook
//   procedure to monitor the message stream for mouse messages posted
//   to the control windows.
// Returns TRUE if successful or FALSE otherwise.
//
// Global variables
// g_hwndTT - handle of the tooltip control
// g_hhk - handle of the hook procedure

BOOL DoCreateDialogTooltip(void)
{
  // Ensure that the common control DLL is loaded, and create a tooltip control.
  g_hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPSTR) NULL,
    TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, launcher.HWNDClient, (HMENU) NULL, GetModuleHandle(NULL), NULL);

  if (g_hwndTT == NULL)
    return FALSE;

  // Enumerate the child windows to register them with the tooltip control.
  if (!EnumChildWindows(launcher.HWNDClient, (WNDENUMPROC) EnumChildProc, 0))
    return FALSE;
 
  // Install a hook procedure to monitor the message stream for mouse
  // messages intended for the controls in the dialog box.
  g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
    (HINSTANCE)NULL, GetCurrentThreadId());

  if (g_hhk == (HHOOK) NULL)
    return FALSE;

  return TRUE;
}

// EmumChildProc - registers control windows with a tooltip control by
// using the TTM_ADDTOOL message to pass the address of a TOOLINFO structure.
// Returns TRUE if successful or FALSE otherwise.
// hwndCtrl - handle of a control window
// lParam - application-defined value (not used)
BOOL CALLBACK EnumChildProc(HWND hwndCtrl, LPARAM lParam)
{
  TOOLINFO ti;
  char szClass[64];

  // Skip static controls.
  GetClassName(hwndCtrl, szClass, sizeof(szClass));
  if (strcmp(szClass, "STATIC"))
  {
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND;

    ti.hwnd = launcher.HWNDClient;
    ti.uId = (UINT) hwndCtrl;
    ti.hinst = 0;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage(g_hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
  }
  return TRUE;
}

// GetMsgProc - monitors the message stream for mouse messages intended
// for a control window in the dialog box.
// Returns a message-dependent value.
// nCode - hook code
// wParam - message flag (not used)
// lParam - address of an MSG structure
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  MSG *lpmsg;

  lpmsg = (MSG *) lParam;
  if (nCode < 0 || !(IsChild(launcher.HWNDClient, lpmsg->hwnd)))
    return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
 
  switch (lpmsg->message)
  {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      if (g_hwndTT != NULL)
      {
        MSG msg;

        msg.lParam = lpmsg->lParam;
        msg.wParam = lpmsg->wParam;
        msg.message = lpmsg->message;
        msg.hwnd = lpmsg->hwnd;
        SendMessage(g_hwndTT, TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) &msg);
      }
      break;
    default:
      break;
  }
  return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
}

// OnWMNotify - provides the tooltip control with the appropriate text
// to display for a control window. This function is called by
// the dialog box procedure in response to a WM_NOTIFY message.
// lParam - second message parameter of the WM_NOTIFY message
VOID OnWMNotify(LPARAM lParam)
{
  static char *tooltip_str = NULL;
  static int   tooltip_maxlen = 0;

  LPTOOLTIPTEXT lpttt;
  int idCtrl;

  if ((((LPNMHDR) lParam)->code) == TTN_NEEDTEXT)
  {
    idCtrl = GetDlgCtrlID((HWND) ((LPNMHDR) lParam)->idFrom);
    lpttt = (LPTOOLTIPTEXT) lParam;

    switch (idCtrl)
    {
    case IDC_HISTORYCOMBO:
    case IDC_PWADLIST:
      {
        int i;
        int count = 0;
        int *selection = NULL;
        int selectioncount = 0;

        SendMessage(launcher.listPWAD, LB_GETCOUNT, 0, 0);

        selectioncount = L_SelGetList(&selection);

        for (i=0; i < selectioncount; i++)
        {
          int index = selection[i];
          char *line = PathFindFileName(launcher.files[index].name);
          int needlen = (tooltip_str?strlen(tooltip_str):0) + strlen(line) + sizeof(char) * 8;
          if (needlen > tooltip_maxlen)
          {
            tooltip_str = realloc(tooltip_str, needlen);
            tooltip_maxlen = needlen;
          }

          if (count++ > 0)
            strcat(strcat(tooltip_str, ", "), line);
          else
            strcpy(tooltip_str, line);
        }

        free(selection);

        lpttt->lpszText = tooltip_str;
      }
      break;
    }
  }
  return;
}

static dboolean L_LauncherIsNeeded(void)
{
  int i;
  dboolean pwad = false;
  char *iwad = NULL;

//  SHIFT for invert
//  if (GetAsyncKeyState(VK_SHIFT) ? launcher_enable : !launcher_enable)
//    return false;

  if ((GetKeyState(VK_SHIFT) & 0x8000))
    return true;

  if (launcher_enable == launcher_enable_always)
    return true;

  if (launcher_enable == launcher_enable_never)
    return false;

  i = M_CheckParm("-iwad");
  if (i && (++i < myargc))
    iwad = I_FindFile(myargv[i], ".wad");

  for (i=0; !pwad && i < (int)numwadfiles; i++)
    pwad = wadfiles[i].src == source_pwad;

  return (!iwad && !pwad && !M_CheckParm("-auto"));
}

void LauncherShow(unsigned int params)
{
  int result;

  if (!L_LauncherIsNeeded())
    return;

  launcher_params = params;

  InitCommonControls();
  sprintf(launchercachefile,"%s/"PACKAGE_TARNAME".cache", I_DoomExeDir());

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
