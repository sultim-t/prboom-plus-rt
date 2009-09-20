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

#ifndef __E6Y__
#define __E6Y__

#include <stdarg.h>

#include "hu_lib.h"
#include "SDL_timer.h"
#include "SDL_events.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "r_demo.h"

#define HU_HUDADDX (HU_HUDX)
#define HU_HUDADDY (HU_HUDY+(-1)*HU_GAPY)
#define HU_CENTERMSGX (320/2)
#define HU_CENTERMSGY ((200-ST_HEIGHT)/2 - 1 - LittleShort(hu_font[0].height))

#define HU_HUDADDX_D (HU_HUDX_LL)
#define HU_HUDADDY_D (HU_HUDY_LL+(-1)*HU_GAPY)

#define HU_MSGCENTERTIMEOUT   (2*TICRATE)

#define STSTR_SECRETFOUND   "A secret is revealed!"

#define S_CANT_GL_ARB_MULTITEXTURE 0x10000000
#define S_CANT_GL_ARB_MULTISAMPLEFACTOR  0x20000000

#define GL_COMBINE_ARB                    0x8570
#define GL_RGB_SCALE_ARB                  0x8573

#define NO_INDEX ((unsigned short)-1)

#define FOV_CORRECTION_FACTOR (1.13776f)
#define FOV90 (90)

#define Pi 3.14159265358979323846f
#define DEG2RAD( a ) ( a * Pi ) / 180.0f
#define RAD2DEG( a ) ( a / Pi ) * 180.0f

typedef struct
{
  const char *wadname;
  int map;
  int address;
} buf_overrun_item_t;

typedef struct camera_s
{
  long x;
  long y;
  long z;
  long PrevX;
  long PrevY;
  long PrevZ;
  angle_t angle;
  angle_t pitch;
  angle_t PrevAngle;
  angle_t PrevPitch;
  int type;
} camera_t;

typedef enum { spriteclip_const, spriteclip_always, spriteclip_smart } spriteclipmode_t;
extern spriteclipmode_t gl_spriteclip;
extern const char *gl_spriteclipmodes[];
extern int gl_sprite_offset;

extern int REAL_SCREENWIDTH;
extern int REAL_SCREENHEIGHT;
extern int REAL_SCREENPITCH;

extern dboolean wasWiped;

extern int totalleveltimes;

extern int secretfound;
extern int messagecenter_counter;
extern int demo_skiptics;
extern int demo_tics_count;
extern int demo_curr_tic;
extern int demo_playerscount;
extern char demo_len_st[80];

extern int avi_shot_time;
extern int avi_shot_num;
extern const char *avi_shot_fname;
extern char avi_shot_curr_fname[PATH_MAX];

extern dboolean doSkip;
extern dboolean demo_stoponnext;
extern dboolean demo_stoponend;
extern dboolean demo_warp;

extern int key_speed_up;
extern int key_speed_down;
extern int key_speed_default;
extern int key_demo_jointogame;
extern int key_demo_nextlevel;
extern int key_demo_endlevel;
extern int speed_step;
extern int key_walkcamera;
extern int key_showalive;

extern int hudadd_gamespeed;
extern int hudadd_leveltime;
extern int hudadd_demotime;
extern int hudadd_secretarea;
extern int hudadd_smarttotals;
extern int hudadd_demoprogressbar;
extern int movement_strafe50;
extern int movement_strafe50onturns;
extern int movement_mouselook;
extern int movement_mouseinvert;
extern int movement_maxviewpitch;
extern int mouse_handler;
extern int mouse_doubleclick_as_use;
extern int render_usedetail;
extern int render_detailedwalls;
extern int render_detailedflats;
extern int render_multisampling;
extern int render_paperitems;
extern int render_wipescreen;
extern int mouse_acceleration;
extern int demo_overwriteexisting;

extern int render_fov;
extern int render_aspect;
extern float render_ratio;
extern float render_fovratio;
extern float render_fovy;
extern float render_multiplier;
void CheckRatio(int width, int height);
void M_ChangeAspectRatio(void);

extern int misc_fastexit;

extern char *sdl_videodriver;
extern int palette_ondamage;
extern int palette_onbonus;
extern int palette_onpowers;

extern camera_t walkcamera;

extern fixed_t sidemove_normal[2];
extern fixed_t sidemove_strafe50[2];

extern int PitchSign;
extern int mouseSensitivity_mlook;
extern angle_t viewpitch;
extern float skyscale;
extern float screen_skybox_zplane;
extern float maxNoPitch[];
extern float tan_pitch;
extern float skyUpAngle;
extern float skyUpShift;
extern float skyXShift;
extern float skyYShift;
extern dboolean mlook_or_fov;

extern hu_textline_t  w_hudadd;
extern hu_textline_t  w_centermsg;
extern hu_textline_t  w_precache;
extern char hud_add[80];
extern char hud_centermsg[80];

void e6y_assert(const char *format, ...);

void ParamsMatchingCheck();
void e6y_InitCommandLine(void);

void P_WalkTicker ();
void P_ResetWalkcam(dboolean ResetCoord, dboolean ResetSight);

extern dboolean sound_inited_once;
void e6y_I_uSleep(unsigned long usecs);
void G_SkipDemoStart(void);
void G_SkipDemoStop(void);
void G_SkipDemoCheck(void);

#ifdef GL_DOOM
void M_ChangeMouseLook(void);
void M_ChangeMaxViewPitch(void);
void M_ChangeMouseInvert(void);
void M_ChangeFOV(void);
void M_ChangeUseDetail(void);
void M_ChangeMultiSample(void);
void M_ChangeSpriteClip(void);
void M_ChangeAllowBoomColormaps(void);
void M_ChangeTextureUseHires(void);
void M_ChangeAllowFog(void);
void M_ChangeTextureHQResize(void);
#endif
void M_ChangeRenderPrecise(void);
void M_ChangeSpeed(void);
void M_ChangeScreenMultipleFactor(void);
void M_ChangeInterlacedScanning(void);
void M_MouseMLook(int choice);
void M_MouseAccel(int choice);
void M_ChangeCompTranslucency(void);
void CheckPitch(signed int *pitch);
void I_Init2(void);

#ifdef GL_DOOM
dboolean GetMouseLook(void);
dboolean HaveMouseLook(void);
#else
#define GetMouseLook() (0)
#define HaveMouseLook() (0)
#endif

extern float viewPitch;
extern dboolean transparentpresent;

#define MAPBITS 12
#define FRACTOMAPBITS (FRACBITS-MAPBITS)

void R_ClearClipSegs (void);
void R_RenderBSPNode(int bspnum);

void gld_MultisamplingSet(void);
void gld_MultisamplingCheck(void);
void gld_MultisamplingInit(void);

typedef struct prboom_comp_s
{
  unsigned int minver;
  unsigned int maxver;
  dboolean state;
  const char *cmd;
} prboom_comp_t;

enum
{
  PC_MONSTER_AVOID_HAZARDS,
  PC_REMOVE_SLIME_TRAILS,
  PC_NO_DROPOFF,
  PC_TRUNCATED_SECTOR_SPECIALS,
  PC_BOOM_BRAINAWAKE,
  PC_PRBOOM_FRICTION,
  PC_REJECT_PAD_WITH_FF,
  PC_FORCE_LXDOOM_DEMO_COMPATIBILITY,
  PC_ALLOW_SSG_DIRECT,
  PC_TREAT_NO_CLIPPING_THINGS_AS_NOT_BLOCKING,
  PC_FORCE_INCORRECT_PROCESSING_OF_RESPAWN_FRAME_ENTRY,
  PC_FORCE_CORRECT_CODE_FOR_3_KEYS_DOORS_IN_MBF,
  PC_UNINITIALIZE_CRUSH_FIELD_FOR_STAIRS,
  PC_FORCE_BOOM_FINDNEXTHIGHESTFLOOR,
  PC_ALLOW_SKY_TRANSFER_IN_BOOM,
  PC_APPLY_GREEN_ARMOR_CLASS_TO_ARMOR_BONUSES,
  PC_APPLY_BLUE_ARMOR_CLASS_TO_MEGASPHERE,
  PC_WRONG_FIXEDDIV,
  PC_FORCE_INCORRECT_BOBBING_IN_BOOM,
  PC_BOOM_DEH_PARSER,
  PC_MAX
};

extern prboom_comp_t prboom_comp[];

int StepwiseSum(int value, int direction, int step, int minval, int maxval, int defval);

enum
{
  TT_ALLKILL,
  TT_ALLITEM,
  TT_ALLSECRET,

  TT_TIME,
  TT_TOTALTIME,
  TT_TOTALKILL,
  TT_TOTALITEM,
  TT_TOTALSECRET,

  TT_MAX
};

typedef struct timetable_s
{
  char map[16];

  int kill[MAXPLAYERS];
  int item[MAXPLAYERS];
  int secret[MAXPLAYERS];
  
  int stat[TT_MAX];
} timetable_t;

#ifdef _WIN32
const char* WINError(void);
#endif

extern int stats_level;
void e6y_G_DoCompleted(void);
void e6y_WriteStats(void);

void e6y_G_DoWorldDone(void);

void I_ProcessWin32Mouse(void);
void I_StartWin32Mouse(void);
void I_EndWin32Mouse(void);
int AccelerateMouse(int val);
void MouseAccelChanging(void);

extern int mlooky;
extern int realtic_clock_rate;

extern dboolean IsDehMaxHealth;
extern dboolean IsDehMaxSoul;
extern dboolean IsDehMegaHealth;
extern dboolean DEH_mobjinfo_bits[NUMMOBJTYPES];

extern int deh_maxhealth;
extern int deh_max_soul;
extern int deh_mega_health;

extern int maxhealthbonus;

void e6y_G_Compatibility(void);

extern dboolean zerotag_manual;
extern int comperr_zerotag;
extern int comperr_passuse;
extern int comperr_hangsolid;

dboolean compbad_get(int *compbad);

dboolean ProcessNoTagLines(line_t* line, sector_t **sec, int *secnum);

#define I_FindName(a)	((a)->Name)
#define I_FindAttr(a)	((a)->Attribs)

typedef struct
{
	unsigned int Attribs;
	unsigned int Times[3*2];
	unsigned int Size[2];
	unsigned int Reserved[2];
	char Name[PATH_MAX];
	char AltName[14];
} findstate_t;

char* PathFindFileName(const char* pPath);
void NormalizeSlashes2(char *str);
unsigned int AfxGetFileName(const char* lpszPathName, char* lpszTitle, unsigned int nMax);
void AbbreviateName(char* lpszCanon, int cchMax, int bAtLeastName);

dboolean StrToInt(const char *s, long *l);

//extern int viewMaxY;

#define MAXTRACEITEMS 8

typedef struct
{
  int index;
  char value[16];
  int data1;
} traceitem_t;

typedef struct
{
  traceitem_t items[MAXTRACEITEMS];
  int count;
} trace_t;

typedef struct
{
  trace_t *trace;
  char *hudstr;
  char cmd[32];
  char prefix[32];
} traceslist_t;

extern trace_t things_health;
extern trace_t things_pickup;
extern trace_t lines_cross;

extern traceslist_t traces[];
extern dboolean traces_present;

extern hu_textline_t  w_traces[];

extern char hud_trace_things_health[80];
extern char hud_trace_things_pickup[80];
extern char hud_trace_lines_cross[80];

extern int clevfromrecord;

void CheckThingsPickupTracer(mobj_t *mobj);
void CheckThingsHealthTracer(mobj_t *mobj);
void CheckLinesCrossTracer(line_t *line);
void ClearLinesCrossTracer(void);

extern dboolean isskytexture;

void D_AddDehFile (const char *file, wad_source_t source);

extern int levelstarttic;

void I_AfterUpdateVideoMode(void);

extern int force_singletics_to;

dboolean HU_DrawDemoProgress(void);

#ifdef ALL_IN_ONE
unsigned char* GetAllInOneLumpHandle(void);
#endif

#ifdef _MSC_VER
int GetFullPath(const char* FileName, const char* ext, char *Buffer, size_t BufferLength);
#endif

void I_vWarning(const char *message, va_list argList);
void I_Warning(const char *message, ...);

#define PRB_MB_OK                       0x00000000
#define PRB_MB_OKCANCEL                 0x00000001
#define PRB_MB_ABORTRETRYIGNORE         0x00000002
#define PRB_MB_YESNOCANCEL              0x00000003
#define PRB_MB_YESNO                    0x00000004
#define PRB_MB_RETRYCANCEL              0x00000005
#define PRB_MB_DEFBUTTON1               0x00000000
#define PRB_MB_DEFBUTTON2               0x00000100
#define PRB_MB_DEFBUTTON3               0x00000200
#define PRB_IDOK                1
#define PRB_IDCANCEL            2
#define PRB_IDABORT             3
#define PRB_IDRETRY             4
#define PRB_IDIGNORE            5
#define PRB_IDYES               6
#define PRB_IDNO                7
int I_MessageBox(const char* text, unsigned int type);

dboolean SmoothEdges(unsigned char * buffer,int w, int h);

#endif
