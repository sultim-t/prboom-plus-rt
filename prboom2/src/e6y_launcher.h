#ifndef __E6Y_LAUNCHER__
#define __E6Y_LAUNCHER__

#include "doomdef.h"

#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)
typedef HRESULT (WINAPI *EnableThemeDialogTexturePROC)(HWND, DWORD);

#define FA_RDONLY	0x00000001
#define FA_HIDDEN	0x00000002
#define FA_SYSTEM	0x00000004
#define FA_DIREC	0x00000010
#define FA_ARCH		0x00000020
#define LAUNCHER_HISTORY_SIZE 10

#define LAUNCHER_CAPTION "Prboom-Plus Launcher"

typedef struct
{
  struct wadfile_info *wadfiles;
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

extern const int nstandard_iwads;
extern const char *const standard_iwads[];
void CheckIWAD(const char *iwadname,GameMode_t *gmode,boolean *hassec);

#endif
