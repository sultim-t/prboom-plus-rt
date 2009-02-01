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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <winreg.h>
#endif
#ifdef GL_DOOM
#include <SDL_opengl.h>
#endif
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "SDL.h"
#ifdef _WIN32
#include <SDL_syswm.h>
#endif

#include "hu_lib.h"

#include "doomtype.h"
#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "i_system.h"
#include "i_main.h"
#include "m_menu.h"
#include "lprintf.h"
#include "d_think.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_maputl.h"
#include "i_video.h"
#include "info.h"
#include "i_simd.h"
#include "r_screenmultiply.h"
#include "r_main.h"
#ifdef GL_DOOM
#include "gl_struct.h"
#include "gl_intern.h"
#endif
#include "g_game.h"
#include "r_demo.h"
#include "e6y.h"
#include "./../ICONS/resource.h"

spriteclipmode_t gl_spriteclip;
const char *gl_spriteclipmodes[] = {"constant","always", "smart"};
int gl_sprite_offset;

int REAL_SCREENWIDTH;
int REAL_SCREENHEIGHT;
int REAL_SCREENPITCH;

DOOM_BOOL wasWiped = false;

int secretfound;
int messagecenter_counter;
int demo_skiptics;
int demo_playerscount;
int demo_tics_count;
int demo_curr_tic;
char demo_len_st[80];

int avi_shot_time;
int avi_shot_num;
const char *avi_shot_fname;
char avi_shot_curr_fname[PATH_MAX];

FILE    *_demofp;
DOOM_BOOL doSkip;
DOOM_BOOL demo_stoponnext;
DOOM_BOOL demo_stoponend;
DOOM_BOOL demo_warp;

int key_speed_up;
int key_speed_down;
int key_speed_default;
int key_demo_jointogame;
int key_demo_nextlevel;
int key_demo_endlevel;
int speed_step;
int key_walkcamera;

int hudadd_gamespeed;
int hudadd_leveltime;
int hudadd_demotime;
int hudadd_secretarea;
int hudadd_smarttotals;
int hudadd_demoprogressbar;
int movement_strafe50;
int movement_strafe50onturns;
int movement_mouselook;
int movement_mouseinvert;
int movement_maxviewpitch;
int mouse_handler;
int mouse_doubleclick_as_use;
int render_fov;
int render_usedetail;
int render_detailedwalls;
int render_detailedflats;
int render_multisampling;
int render_paperitems;
int render_wipescreen;
int mouse_acceleration;
int demo_overwriteexisting;
int overrun_spechit_warn;
int overrun_spechit_emulate;
int overrun_reject_warn;
int overrun_reject_emulate;
int overrun_intercept_warn;
int overrun_intercept_emulate;
int overrun_playeringame_warn;
int overrun_playeringame_emulate;

int overrun_spechit_promted = false;
int overrun_reject_promted = false;
int overrun_intercept_promted = false;
int overrun_playeringame_promted = false;

DOOM_BOOL was_aspected;
int render_aspect_width;
int render_aspect_height;
float render_aspect_ratio;

int misc_fastexit;

char *sdl_videodriver;
int palette_ondamage;
int palette_onbonus;
int palette_onpowers;

float mouse_accelfactor;

camera_t walkcamera;

hu_textline_t  w_hudadd;
hu_textline_t  w_centermsg;
hu_textline_t  w_precache;
char hud_add[80];
char hud_centermsg[80];

fixed_t sidemove_normal[2]    = {0x18, 0x28};
fixed_t sidemove_strafe50[2]    = {0x19, 0x32};

int mouseSensitivity_mlook;
angle_t viewpitch;
float fovscale;
float tan_pitch;
float skyUpAngle;
float skyUpShift;
float skyXShift;
float skyYShift;
DOOM_BOOL mlook_or_fov;

float internal_render_fov = FOV90;

int maxViewPitch;
int minViewPitch;

#ifdef _WIN32
char* WINError(void)
{
  static char *WinEBuff = NULL;
  DWORD err = GetLastError();
  char *ch;

  if (WinEBuff)
  {
    LocalFree(WinEBuff);
  }

  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &WinEBuff, 0, NULL) == 0)
  {
    return "Unknown error";
  }

  if ((ch = strchr(WinEBuff, '\n')) != 0)
    *ch = 0;
  if ((ch = strchr(WinEBuff, '\r')) != 0)
    *ch = 0;

  return WinEBuff;
}
#endif

//--------------------------------------------------
#ifdef _WIN32
HWND WIN32_GetHWND(void);
//static void I_CenterMouse(void);
#endif
//--------------------------------------------------

void e6y_assert(const char *format, ...) 
{
  static FILE *f = NULL;
  va_list argptr;
  va_start(argptr,format);
  //if (!f)
    f = fopen("d:\\a.txt", "ab+");
  vfprintf(f, format, argptr);
  fclose(f);
  va_end(argptr);
}

/* ParamsMatchingCheck
 * Conflicting command-line parameters could cause the engine to be confused 
 * in some cases. Added checks to prevent this.
 * Example: glboom.exe -record mydemo -playdemo demoname
 */
void ParamsMatchingCheck()
{
  DOOM_BOOL recording_attempt = 
    M_CheckParm("-record") || 
    M_CheckParm("-recordfrom") ||
    M_CheckParm("-recordfromto");
  
  DOOM_BOOL playbacking_attempt = 
    M_CheckParm("-playdemo") || 
    M_CheckParm("-timedemo") ||
    M_CheckParm("-fastdemo");

  if (recording_attempt && playbacking_attempt)
    I_Error("Params are not matching: Can not being played back and recorded at the same time.");
}

prboom_comp_t prboom_comp[PC_MAX] = {
  {0xffffffff, 0x02020615, 0, "-force_monster_avoid_hazards"},
  {0x00000000, 0x02040601, 0, "-force_remove_slime_trails"},
  {0x02020200, 0x02040801, 0, "-force_no_dropoff"},
  {0x00000000, 0x02040801, 0, "-force_truncated_sector_specials"},
  {0x00000000, 0x02040802, 0, "-force_boom_brainawake"},
  {0x00000000, 0x02040802, 0, "-force_prboom_friction"},
  {0x02020500, 0x02040000, 0, "-reject_pad_with_ff"},
  {0xffffffff, 0x02040802, 0, "-force_lxdoom_demo_compatibility"},
  {0x00000000, 0x0202061b, 0, "-allow_ssg_direct"},
  {0x00000000, 0x02040601, 0, "-treat_no_clipping_things_as_not_blocking"},
  {0x00000000, 0x02040803, 0, "-force_incorrect_processing_of_respawn_frame_entry"},
  {0x00000000, 0x02040601, 0, "-force_correct_code_for_3_keys_doors_in_mbf"},
  {0x00000000, 0x02040601, 0, "-uninitialize_crush_field_for_stairs"},
  {0x00000000, 0x02040802, 0, "-force_boom_findnexthighestfloor"},
  {0x00000000, 0x02040802, 0, "-allow_sky_transfer_in_boom"},
  {0x00000000, 0x02040803, 0, "-apply_green_armor_class_to_armor_bonuses"},
  {0x00000000, 0x02040803, 0, "-apply_blue_armor_class_to_megasphere"},
};

void e6y_InitCommandLine(void)
{
  int i, p;

  //-recordfromto x y
  {
    char demoname[PATH_MAX];
    DOOM_BOOL bDemoContinue = false;
    democontinue = false;

    if ((p = M_CheckParm("-recordfromto")) && (p < myargc - 2))
    {
      bDemoContinue = true;
      AddDefaultExtension(strcpy(demoname, myargv[p + 1]), ".lmp");
      AddDefaultExtension(strcpy(democontinuename, myargv[p + 2]), ".lmp");
    }

    if (bDemoContinue && (_demofp = fopen(demoname, "rb")))
    {
      democontinue = true;
    }
  }

  if ((p = M_CheckParm("-skipsec")) && (p < myargc-1))
    demo_skiptics = (int)(atof(myargv[p + 1]) * 35);
  if ((IsDemoPlayback() || democontinue) && (startmap > 1 || demo_skiptics))
    G_SkipDemoStart();
  if ((p = M_CheckParm("-avidemo")) && (p < myargc-1))
    avi_shot_fname = myargv[p + 1];
  stats_level = M_CheckParm("-levelstat");

  if (IsDemoPlayback())
  {
    //"2.4.8.2" -> 0x02040802
    if ((p = M_CheckParm("-emulate")) && (p < myargc - 1))
    {
      unsigned int emulated_version = 0;
      int b[4], k = 1;
      memset(b, 0, sizeof(b));
      sscanf(myargv[p+1], "%d.%d.%d.%d", &b[0], &b[1], &b[2], &b[3]);
      for (i = 3; i >= 0; i--, k *= 256)
      {
#ifdef RANGECHECK
        if (b[i] >= 256)
          I_Error("Wrong version number of package: %s", VERSION);
#endif
        emulated_version += b[i] * k;
      }
      
      for (i = 0; i < PC_MAX; i++)
      {
        prboom_comp[i].state = 
          (emulated_version >= prboom_comp[i].minver && 
           emulated_version <  prboom_comp[i].maxver);
      }
    }

    for (i = 0; i < PC_MAX; i++)
    {
      if (M_CheckParm(prboom_comp[i].cmd))
        prboom_comp[i].state = true;
    }
  }

  // TAS-tracers
  {
    long value, count;
    traces_present = false;

    for (i=0;i<3;i++)
    {
      count = 0;
      traces[i].trace->count = 0;
      if ((p = M_CheckParm(traces[i].cmd)) && (p < myargc-1))
      {
        while (count < 3 && p + count < myargc-1 && StrToInt(myargv[p+1+count], &value))
        {
          sprintf(traces[i].trace->items[count].value, "\x1b\x36%ld\x1b\x33 0", value);
          traces[i].trace->items[count].index = value;
          traces_present = true;
          count++;
        }
        traces[i].trace->count = count;
      }
    }
  }

  shorttics = M_CheckParm("-shorttics");

  // What hacked exe are we emulating?
  game_exe = EXE_NONE;
  if ((p = M_CheckParm("-exe")) && (p < myargc-1))
  {
    if (!strcasecmp(myargv[p + 1], "chex") || !strcasecmp(myargv[p + 1], "chex.exe"))
    {
      game_exe = EXE_CHEX;
    }
  }

  // demoex
  if ((p = M_CheckParm("-addlump")) && (p < myargc - 3))
  {
    char *demoname = NULL;
    char *filename = NULL;
    const char *lumpname;
    int result = 0;

    demoname = I_FindFile(myargv[p + 1], ".lmp");
    filename = I_FindFile(myargv[p + 2], ".txt");
    lumpname = myargv[p + 3];
    if (demoname && filename && lumpname && strlen(lumpname) <= 8)
    {
      byte *buffer = NULL;
      byte *demoex_p = NULL;
      size_t size;

      buffer = G_GetDemoFooter(demoname, &demoex_p, &size);
      if (buffer && demoex_p)
      {
        wadtbl_t* demoex;
        size_t lump_size = 0;
        byte *lump_buffer = NULL;

        demoex = W_CreatePWADTable(demoex_p, size);
        if (demoex)
        {
          if (I_FileToBuffer(filename, &lump_buffer, &lump_size))
          {
            W_AddLump(demoex, lumpname, lump_buffer, lump_size);
            G_SetDemoFooter(demoname, demoex);

            free(lump_buffer);
          }
          W_FreePWADTable(demoex);
        }

        free(buffer);
      }
    }
    free(demoname);
    free(filename);
  }
}

static DOOM_BOOL saved_fastdemo;
static DOOM_BOOL saved_nodrawers;
static DOOM_BOOL saved_nosfxparm;
static DOOM_BOOL saved_nomusicparm;
static int saved_render_precise;

void G_SkipDemoStart(void)
{
  saved_fastdemo = fastdemo;
  saved_nodrawers = nodrawers;
  saved_nosfxparm = nosfxparm;
  saved_nomusicparm = nomusicparm;
  saved_render_precise = render_precise;
  
  doSkip = true;

  S_StopMusic();
  fastdemo = true;
  nodrawers = true;
  nosfxparm = true;
  nomusicparm = true;

  render_precise = false;
  M_ChangeRenderPrecise();

  I_Init2();
}

DOOM_BOOL sound_inited_once = false;

void G_SkipDemoStop(void)
{
  fastdemo = saved_fastdemo;
  nodrawers = saved_nodrawers;
  nosfxparm = saved_nosfxparm;
  nomusicparm = saved_nomusicparm;
  
  render_precise = saved_render_precise;
  M_ChangeRenderPrecise();

  demo_stoponnext = false;
  demo_stoponend = false;
  demo_warp = false;
  doSkip = false;
  demo_skiptics = 0;
  startmap = 0;

  I_Init2();
  if (!sound_inited_once && !(nomusicparm && nosfxparm))
  {
    void I_InitSound(void);
    I_InitSound();
  }
  S_Init(snd_SfxVolume, snd_MusicVolume);
  S_Start();

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL) {
    gld_PreprocessLevel();
  }
#endif
}

void G_SkipDemoCheck(void)
{
  if (doSkip && gametic > 0)
  {
    if (((startmap <= 1) && 
         (gametic > demo_skiptics + (demo_skiptics > 0 ? 0 : demo_tics_count))) ||
        (demo_warp && gametic - levelstarttic > demo_skiptics))
     {
       G_SkipDemoStop();
     }
  }
}

void M_ChangeSpeed(void)
{
  extern int sidemove[2];

  if(movement_strafe50)
  {
    sidemove[0] = sidemove_strafe50[0];
    sidemove[1] = sidemove_strafe50[1];
  }
  else
  {
    movement_strafe50onturns = false;
    sidemove[0] = sidemove_normal[0];
    sidemove[1] = sidemove_normal[1];
  }
}

#ifdef GL_DOOM
void M_ChangeMouseLook(void)
{
  viewpitch = 0;
}

void M_ChangeMouseInvert(void)
{
}

void M_ChangeMaxViewPitch(void)
{
  int angle = (int)((float)movement_maxviewpitch / 45.0f * ANG45);
  maxViewPitch = (angle - (1<<ANGLETOFINESHIFT));
  minViewPitch = (-angle + (1<<ANGLETOFINESHIFT));

  viewpitch = 0;
}
#endif // GL_DOOM

void M_ChangeRenderPrecise(void)
{
#ifdef GL_DOOM
  if (V_GetMode() != VID_MODEGL)
  {
    gl_seamless = false;
    //render_segs = false;
    return;
  }
  else
  {
    if (render_precise)
    {
      gl_seamless = true;
      gld_InitVertexData();
    }
    else
    {
      gl_seamless = false;
      gld_CleanVertexData();
    }

    //render_segs = false;
  }
#endif // GL_DOOM
}

void M_ChangeScreenMultipleFactor(void)
{
}

void M_ChangeInterlacedScanning(void)
{
  if (render_interlaced_scanning)
    interlaced_scanning_requires_clearing = 1;
}

DOOM_BOOL GetMouseLook(void)
{
  return movement_mouselook;
}

void CheckPitch(signed int *pitch)
{
  if (V_GetMode() == VID_MODEGL)
  {
    if(*pitch > maxViewPitch)
      *pitch = maxViewPitch;
    if(*pitch < minViewPitch)
      *pitch = minViewPitch;
  }
}

#ifdef GL_DOOM
void M_ChangeMultiSample(void)
{
}

void M_ChangeFOV(void)
{
  float f1, f2;
  int p;
  int render_aspect_width, render_aspect_height;

  if ((p = M_CheckParm("-aspect")) && (p+1 < myargc) && (strlen(myargv[p+1]) <= 21) &&
    (2 == sscanf(myargv[p+1], "%dx%d", &render_aspect_width, &render_aspect_height)))
  {
    render_aspect_ratio = (float)render_aspect_width/(float)render_aspect_height;
  }
  else
  {
    render_aspect_ratio = 1.6f;
    /*if (desired_fullscreen)
      render_aspect_ratio = 1.6f;
    else
      render_aspect_ratio = (float)REAL_SCREENWIDTH/(float)REAL_SCREENHEIGHT;*/
  }
  was_aspected = (float)render_aspect_ratio != 1.6f;

  internal_render_fov = (float)(2 * RAD2DEG(atan(tan(DEG2RAD(render_fov) / 2) / render_aspect_ratio)));

  fovscale = FOV90/(float)render_fov;//*(render_aspect_ratio/1.6f);
  //fovscale = render_aspect_ratio/1.6f;

  f1 = (float)(320.0f/200.0f/fovscale-0.2f);
  f2 = (float)tan(DEG2RAD(internal_render_fov)/2.0f);
  if (f1-f2<1)
    skyUpAngle = (float)-RAD2DEG(asin(f1-f2));
  else
    skyUpAngle = -90.0f;

  skyUpShift = (float)tan(DEG2RAD(internal_render_fov)/2.0f);
}

void M_ChangeUseDetail(void)
{
  if (V_GetMode() == VID_MODEGL)
    render_usedetail = (render_canusedetail) && (render_detailedflats || render_detailedwalls);
}

void M_ChangeSpriteClip(void)
{
}

void ResolveColormapsHiresConflict(DOOM_BOOL prefer_colormap)
{
  if (prefer_colormap)
  {
    if (gl_boom_colormaps_default)
    {
      gl_texture_external_hires = false;
    }
    else if (gl_texture_external_hires)
    {
      gl_boom_colormaps = false;
      gl_boom_colormaps_default = false;
    }
  }
  else
  {
    if (gl_texture_external_hires)
    {
      gl_boom_colormaps = false;
      gl_boom_colormaps_default = false;
    }
    else if (gl_boom_colormaps_default)
    {
      gl_texture_external_hires = false;
    }
  }
}

void M_ChangeAllowBoomColormaps(void)
{
  if (gl_boom_colormaps == -1)
  {
    gl_boom_colormaps = gl_boom_colormaps_default;
    ResolveColormapsHiresConflict(true);
  }
  else
  {
    ResolveColormapsHiresConflict(true);
    gld_FlushTextures();
    gld_Precache();
  }
}

void M_ChangeTextureUseHires(void)
{
  ResolveColormapsHiresConflict(false);

  gld_FlushTextures();
  gld_Precache();
}

void M_ChangeLightMode(void)
{
  if (gl_lightmode == gl_lightmode_glboom)
  {
    gld_SetGammaRamp(-1);
    gld_FlushTextures();
  }

  if (gl_lightmode == gl_lightmode_gzdoom)
  {
    gld_SetGammaRamp(useglgamma);
  }

  if (gl_lightmode == gl_lightmode_mixed)
  {
    gld_SetGammaRamp(-1);
    gld_FlushTextures();
  }
}

void M_ChangeTextureHQResize(void)
{
  gld_FlushTextures();
}
#endif //GL_DOOM

void M_Mouse(int choice, int *sens);
void M_MouseMLook(int choice)
{
  M_Mouse(choice, &mouseSensitivity_mlook);
}

void M_MouseAccel(int choice)
{
  M_Mouse(choice, &mouse_acceleration);
  MouseAccelChanging();
}

void MouseAccelChanging(void)
{
  mouse_accelfactor = (float)mouse_acceleration/100.0f+1.0f;
}

void M_DemosBrowse(void)
{
}

float viewPitch;
DOOM_BOOL transparentpresent;

void e6y_MultisamplingCheck(void)
{
#ifdef GL_DOOM
  if (render_multisampling)
  {
    int test = -1;
    SDL_GL_GetAttribute (SDL_GL_MULTISAMPLESAMPLES, &test);
    if (test!=render_multisampling)
    {
      void M_SaveDefaults (void);
      void I_Error(const char *error, ...);
      int i=render_multisampling;
      render_multisampling = 0;
      M_SaveDefaults ();
      I_Error("Couldn't set %dX multisamples for %dx%d video mode", i, SCREENWIDTH, SCREENHEIGHT);
    }
  }
#endif //GL_DOOM
}

void e6y_MultisamplingSet(void)
{
#ifdef GL_DOOM
  if (render_multisampling)
  {
    extern int gl_colorbuffer_bits;
    extern int gl_depthbuffer_bits;
    
    gl_colorbuffer_bits = 32;
    SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, gl_colorbuffer_bits );
  
    if (gl_depthbuffer_bits!=8 && gl_depthbuffer_bits!=16 && gl_depthbuffer_bits!=24)
      gl_depthbuffer_bits = 16;
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, gl_depthbuffer_bits );

    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, render_multisampling );
    SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 1 );
  }
#endif //GL_DOOM
}

int StepwiseSum(int value, int direction, int step, int minval, int maxval, int defval)
{
  static int prev_value = 0;
  static int prev_direction = 0;

  int newvalue;
  int val = (direction > 0 ? value : value - 1);
  
  if (direction == 0)
    return defval;

  direction = (direction > 0 ? 1 : -1);
  
  if (step != 0)
    newvalue = (prev_direction * direction < 0 ? prev_value : value + direction * step);
  else
  {
    int exp = 1;
    while (exp * 10 <= val)
      exp *= 10;
    newvalue = direction * (val < exp * 5 && exp > 1 ? exp / 2 : exp);
    newvalue = (value + newvalue) / newvalue * newvalue;
  }

  if (newvalue > maxval) newvalue = maxval;
  if (newvalue < minval) newvalue = minval;

  if ((value < defval && newvalue > defval) || (value > defval && newvalue < defval))
    newvalue = defval;

  if (newvalue != value)
  {
    prev_value = value;
    prev_direction = direction;
  }

  return newvalue;
}

void I_vWarning(const char *message, va_list argList)
{
  char msg[1024];
#ifdef HAVE_VSNPRINTF
  vsnprintf(msg,sizeof(msg),message,argList);
#else
  vsprintf(msg,message,argList);
#endif
  lprintf(LO_ERROR, "%s\n", msg);
#ifdef _MSC_VER
  I_MessageBox(msg, PRB_MB_OK);
  //I_SwitchToWindow(WIN32_GetHWND());
#endif
}

void I_Warning(const char *message, ...)
{
  va_list argptr;
  va_start(argptr,message);
  I_vWarning(message, argptr);
  va_end(argptr);
}

int I_MessageBox(const char* text, unsigned int type)
{
  int result = PRB_IDCANCEL;

#ifdef _WIN32
  {
    HWND current_hwnd = GetForegroundWindow();
    //I_SwitchToWindow(GetDesktopWindow());
    result = MessageBox(GetDesktopWindow(), text, PACKAGE_TITLE, type|MB_TASKMODAL|MB_TOPMOST);
    I_SwitchToWindow(current_hwnd);
    return result;
  }
#endif

#ifdef RjY
  {
    typedef struct mb_hotkeys_s
    {
      int type;
      char *hotkeys_str;
    } mb_hotkeys_t;

    mb_hotkeys_t mb_hotkeys[] = {
      {PRB_MB_OK               , "(press <enter> to continue)"},
      {PRB_MB_OKCANCEL         , "(press <enter> to continue or <esc> to cancel)"},
      {PRB_MB_ABORTRETRYIGNORE , "(a - abort, r - retry, i - ignore)"},
      {PRB_MB_YESNOCANCEL      , "(y - yes, n - no, esc - cancel"},
      {PRB_MB_YESNO            , "(y - yes, n - no)"},
      {PRB_MB_RETRYCANCEL      , "(r - retry, <esc> - cancel)"},
      {0, NULL}
    };

    int i, c;
    char* hotkeys_str = NULL;
    
    type &= 0x000000ff;

    i = 0;
    while (mb_hotkeys[i].hotkeys_str)
    {
      if (mb_hotkeys[i].type == type)
      {
        hotkeys_str = mb_hotkeys[i].hotkeys_str;
        break;
      }
      i++;
    }

    if (hotkeys_str)
    {
      lprintf(LO_CONFIRM, "%s\n%s\n", text, hotkeys_str);

      result = -1;
      do
      {
        I_uSleep(1000);

        c = tolower(getchar());

        if (c == 'y') result = PRB_IDYES;
        else if (c == 'n') result = PRB_IDNO;
        else if (c == 'a') result = PRB_IDABORT;
        else if (c == 'r') result = PRB_IDRETRY;
        else if (c == 'i') result = PRB_IDIGNORE;
        else if (c == 'o' || c == 13) result = PRB_IDOK;
        else if (c == 'c' || c == 27) result = PRB_IDCANCEL;
      }
      while (result == EOF);

      return result;
    }

    return result;
  }
#else // RjY
  return PRB_IDCANCEL;
#endif // RjY
}

void ShowOverflowWarning(int emulate, int *promted, DOOM_BOOL fatal, const char *name, const char *params, ...)
{
  if (!(*promted))
  {
    va_list argptr;
    char buffer[1024];
    
    char str1[] =
      "Too big or not supported %s overflow has been detected. "
      "Desync or crash can occur soon "
      "or during playback with the vanilla engine in case you're recording demo.%s%s";
    
    char str2[] = 
      "%s overflow has been detected.%s%s";

    char str3[] = 
      "%s overflow has been detected. "
      "The option responsible for emulation of this overflow is switched off "
      "hence desync or crash can occur soon "
      "or during playback with the vanilla engine in case you're recording demo.%s%s";

    *promted = true;

    sprintf(buffer, (fatal?str1:(emulate?str2:str3)), 
      name, "\nYou can change PrBoom behaviour for this overflow through in-game menu.", params);
    
    va_start(argptr,params);
    I_vWarning(buffer, argptr);
    va_end(argptr);
  }
}

int stats_level;
int numlevels = 0;
int levels_max = 0;
timetable_t *stats = NULL;

void e6y_G_CheckDemoStatus(void)
{
  if (doSkip && (demo_stoponend || demo_stoponnext))
    G_SkipDemoStop();
}

void e6y_G_DoCompleted(void)
{
  int i;

  if (doSkip && (demo_stoponend || demo_warp))
    G_SkipDemoStop();

  if(!stats_level)
    return;

  if (numlevels >= levels_max)
  {
    levels_max = levels_max ? levels_max*2 : 32;
    stats = realloc(stats,sizeof(*stats)*levels_max);
  }

  memset(&stats[numlevels], 0, sizeof(timetable_t));

  if (gamemode==commercial)
    sprintf(stats[numlevels].map,"MAP%02i",gamemap);
  else
    sprintf(stats[numlevels].map,"E%iM%i",gameepisode,gamemap);

  stats[numlevels].stat[TT_TIME]        = leveltime;
  stats[numlevels].stat[TT_TOTALTIME]   = totalleveltimes;
  stats[numlevels].stat[TT_TOTALKILL]   = totalkills;
  stats[numlevels].stat[TT_TOTALITEM]   = totalitems;
  stats[numlevels].stat[TT_TOTALSECRET] = totalsecret;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i])
    {
      stats[numlevels].kill[i]   = players[i].killcount - players[i].resurectedkillcount;
      stats[numlevels].item[i]   = players[i].itemcount;
      stats[numlevels].secret[i] = players[i].secretcount;
      
      stats[numlevels].stat[TT_ALLKILL]   += stats[numlevels].kill[i];
      stats[numlevels].stat[TT_ALLITEM]   += stats[numlevels].item[i];
      stats[numlevels].stat[TT_ALLSECRET] += stats[numlevels].secret[i];
    }
  }

  numlevels++;

  e6y_WriteStats();
}

typedef struct tmpdata_s
{
  char kill[200];
  char item[200];
  char secret[200];
} tmpdata_t;

void e6y_WriteStats(void)
{
  FILE *f;
  char str[200];
  int i, level, playerscount;
  timetable_t max;
  tmpdata_t tmp;
  tmpdata_t all[32];
  size_t allkills_len=0, allitems_len=0, allsecrets_len=0;

  f = fopen("levelstat.txt", "wb");
  
  memset(&max, 0, sizeof(timetable_t));

  playerscount = 0;
  for (i=0; i<MAXPLAYERS; i++)
    if (playeringame[i])
      playerscount++;

  for (level=0;level<numlevels;level++)
  {
    memset(&tmp, 0, sizeof(tmpdata_t));
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i])
      {
        strcpy(str, strlen(tmp.kill)==0?"%s%d":"%s+%d");
        
        sprintf(tmp.kill,   str, tmp.kill,   stats[level].kill[i]  );
        sprintf(tmp.item,   str, tmp.item,   stats[level].item[i]  );
        sprintf(tmp.secret, str, tmp.secret, stats[level].secret[i]);
      }
    }
    if (playerscount<2)
      memset(&all[level], 0, sizeof(tmpdata_t));
    else
    {
      sprintf(all[level].kill,   " (%s)", tmp.kill  );
      sprintf(all[level].item,   " (%s)", tmp.item  );
      sprintf(all[level].secret, " (%s)", tmp.secret);
    }

    if (strlen(all[level].kill) > allkills_len)
      allkills_len = strlen(all[level].kill);
    if (strlen(all[level].item) > allitems_len)
      allitems_len = strlen(all[level].item);
    if (strlen(all[level].secret) > allsecrets_len)
      allsecrets_len = strlen(all[level].secret);

    for(i=0; i<TT_MAX; i++)
      if (stats[level].stat[i] > max.stat[i])
        max.stat[i] = stats[level].stat[i];
  }
  max.stat[TT_TIME] = max.stat[TT_TIME]/TICRATE/60;
  max.stat[TT_TOTALTIME] = max.stat[TT_TOTALTIME]/TICRATE/60;
  
  for(i=0; i<TT_MAX; i++) {
    SNPRINTF(str, 200, "%d", max.stat[i]);
    max.stat[i] = strlen(str);
  }

  for (level=0;level<numlevels;level++)
  {
    strcpy(str, "%%s - %%%dd:%%05.2f (%%%dd:%%02d)  K: %%%dd/%%-%dd%%%ds  I: %%%dd/%%-%dd%%%ds  S: %%%dd/%%-%dd %%%ds\r\n");

    sprintf(str, str,
      max.stat[TT_TIME],      max.stat[TT_TOTALTIME],
      max.stat[TT_ALLKILL],   max.stat[TT_TOTALKILL],   allkills_len,
      max.stat[TT_ALLITEM],   max.stat[TT_TOTALITEM],   allitems_len,
      max.stat[TT_ALLSECRET], max.stat[TT_TOTALSECRET], allsecrets_len);
    
    fprintf(f, str, stats[level].map, 
      stats[level].stat[TT_TIME]/TICRATE/60,
      (float)(stats[level].stat[TT_TIME]%(60*TICRATE))/TICRATE,
      (stats[level].stat[TT_TOTALTIME])/TICRATE/60, 
      (stats[level].stat[TT_TOTALTIME]%(60*TICRATE))/TICRATE,
      stats[level].stat[TT_ALLKILL],  stats[level].stat[TT_TOTALKILL],   all[level].kill,
      stats[level].stat[TT_ALLITEM],  stats[level].stat[TT_TOTALITEM],   all[level].item,
      stats[level].stat[TT_ALLSECRET],stats[level].stat[TT_TOTALSECRET], all[level].secret
      );
    
  }
  
  fclose(f);
}

void e6y_G_DoWorldDone(void)
{
  if (doSkip)
  {
    static int firstmap = 1;
    demo_warp =
      demo_stoponnext ||
      ((gamemode==commercial)?
        (startmap == gamemap):
        (startepisode==gameepisode && startmap==gamemap));
    if (demo_warp && demo_skiptics==0 && !firstmap)
      G_SkipDemoStop();
    if (firstmap) firstmap = 0;
  }
}

//--------------------------------------------------

#ifdef _WIN32
HWND WIN32_GetHWND(void)
{
  static HWND Window = NULL; 
  if(!Window)
  {
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWMInfo(&wminfo);
    Window = wminfo.window;
  }
  return Window;
}
#endif

int AccelerateMouse(int val)
{
  if (!mouse_acceleration)
    return val;

  if (val < 0)
    return -AccelerateMouse(-val);
  return (int)(pow((double)val, (double)mouse_accelfactor));
}

int mlooky;

DOOM_BOOL IsDehMaxHealth = false;
DOOM_BOOL IsDehMaxSoul = false;
DOOM_BOOL IsDehMegaHealth = false;
DOOM_BOOL DEH_mobjinfo_bits[NUMMOBJTYPES] = {0};

int deh_maxhealth;
int deh_max_soul;
int deh_mega_health;

int maxhealthbonus;

void M_ChangeCompTranslucency(void)
{
  int i;
  int predefined_translucency[] = {
    MT_FIRE, MT_SMOKE, MT_FATSHOT, MT_BRUISERSHOT, MT_SPAWNFIRE,
    MT_TROOPSHOT, MT_HEADSHOT, MT_PLASMA, MT_BFG, MT_ARACHPLAZ, MT_PUFF, 
    MT_TFOG, MT_IFOG, MT_MISC12, MT_INV, MT_INS, MT_MEGA
  };
  
  for(i = 0; (size_t)i < sizeof(predefined_translucency)/sizeof(predefined_translucency[0]); i++)
  {
    if (!DEH_mobjinfo_bits[predefined_translucency[i]])
    {
      // Transparent sprites are not visible behind transparent walls in OpenGL.
      // It needs much work.
#ifdef GL_DOOM
      if (V_GetMode() == VID_MODEGL)
      {
        // Disabling transparency in OpenGL for original sprites
        // which are not changed by dehacked, because it's buggy for now.
        // Global sorting of transparent sprites and walls is needed
        mobjinfo[predefined_translucency[i]].flags &= ~MF_TRANSLUCENT;
      }
      else
#endif
      if (comp[comp_translucency]) 
        mobjinfo[predefined_translucency[i]].flags &= ~MF_TRANSLUCENT;
      else 
        mobjinfo[predefined_translucency[i]].flags |= MF_TRANSLUCENT;
    }
  }
}

void e6y_G_Compatibility(void)
{
  extern int maxhealth;
  extern int max_soul;
  extern int mega_health;

  int comp_max = (compatibility_level == doom_12_compatibility ? 199 : 200);

  max_soul = (IsDehMaxSoul ? deh_max_soul : comp_max);
  mega_health = (IsDehMegaHealth ? deh_mega_health : comp_max);

  if (comp[comp_maxhealth]) 
  {
    maxhealth = 100;
    maxhealthbonus = (IsDehMaxHealth ? deh_maxhealth : comp_max);
  }
  else 
  {
    maxhealth = (IsDehMaxHealth ? deh_maxhealth : 100);
    maxhealthbonus = maxhealth * 2;
  }

  if (!DEH_mobjinfo_bits[MT_SKULL])
  {
    if (compatibility_level == doom_12_compatibility)
      mobjinfo[MT_SKULL].flags |= (MF_COUNTKILL);
    else
      mobjinfo[MT_SKULL].flags &= ~(MF_COUNTKILL);
  }

  M_ChangeCompTranslucency();
}

DOOM_BOOL zerotag_manual;
int comperr_zerotag;
int comperr_passuse;
int comperr_hangsolid;

DOOM_BOOL compbad_get(int *compbad)
{
  return !demo_compatibility && (*compbad) && !demorecording && !demoplayback;
}

DOOM_BOOL ProcessNoTagLines(line_t* line, sector_t **sec, int *secnum)
{
  zerotag_manual = false;
  if (line->tag == 0 && compbad_get(&comperr_zerotag))
  {
    if (!(*sec=line->backsector))
      return true;
    *secnum = *sec-sectors;
    zerotag_manual = true;
    return true;
  }
  return false;
}

char* PathFindFileName(const char* pPath)
{
  const char* pT = pPath;
  
  if (pPath)
  {
    for ( ; *pPath; pPath++)
    {
      if ((pPath[0] == '\\' || pPath[0] == ':' || pPath[0] == '/')
        && pPath[1] &&  pPath[1] != '\\'  &&   pPath[1] != '/')
        pT = pPath + 1;
    }
  }
  
  return (char*)pT;
}

void NormalizeSlashes2(char *str)
{
  int l;

  if (!str || !(l = strlen(str)))
    return;
  if (str[--l]=='\\' || str[l]=='/')
    str[l]=0;
  while (l--)
    if (str[l]=='/')
      str[l]='\\';
}

unsigned int AfxGetFileName(const char* lpszPathName, char* lpszTitle, unsigned int nMax)
{
  char* lpszTemp = PathFindFileName(lpszPathName);
  
  if (lpszTitle == NULL)
    return strlen(lpszTemp)+1;
  
  strncpy(lpszTitle, lpszTemp, nMax-1);
  return 0;
}

void AbbreviateName(char* lpszCanon, int cchMax, int bAtLeastName)
{
  int cchFullPath, cchFileName, cchVolName;
  const char* lpszCur;
  const char* lpszBase;
  const char* lpszFileName;
  
  lpszBase = lpszCanon;
  cchFullPath = strlen(lpszCanon);
  
  cchFileName = AfxGetFileName(lpszCanon, NULL, 0) - 1;
  lpszFileName = lpszBase + (cchFullPath-cchFileName);
  
  if (cchMax >= cchFullPath)
    return;
  
  if (cchMax < cchFileName)
  {
    strcpy(lpszCanon, (bAtLeastName) ? lpszFileName : "");
    return;
  }
  
  lpszCur = lpszBase + 2;
  
  if (lpszBase[0] == '\\' && lpszBase[1] == '\\')
  {
    while (*lpszCur != '\\')
      lpszCur++;
  }
  
  if (cchFullPath - cchFileName > 3)
  {
    lpszCur++;
    while (*lpszCur != '\\')
      lpszCur++;
  }
  
  cchVolName = (int)(lpszCur - lpszBase);
  if (cchMax < cchVolName + 5 + cchFileName)
  {
    strcpy(lpszCanon, lpszFileName);
    return;
  }
  
  while (cchVolName + 4 + (int)strlen(lpszCur) > cchMax)
  {
    do
    {
      lpszCur++;
    }
    while (*lpszCur != '\\');
  }
  
  lpszCanon[cchVolName] = '\0';
  strcat(lpszCanon, "\\...");
  strcat(lpszCanon, lpszCur);
}

DOOM_BOOL PlayeringameOverrun(const mapthing_t* mthing)
{
  if (mthing->type==0
    && (overrun_playeringame_warn || overrun_playeringame_emulate))
  {
    if (overrun_playeringame_warn)
      ShowOverflowWarning(overrun_playeringame_emulate, &overrun_playeringame_promted, players[4].didsecret, "PLAYERINGAME", "");

    if (overrun_playeringame_emulate)
    {
      return true;
    }
  }
  return false;
}

trace_t things_health;
trace_t things_pickup;
trace_t lines_cross;

hu_textline_t  w_traces[3];

char hud_trace_things_health[80];
char hud_trace_things_pickup[80];
char hud_trace_lines_cross[80];

int clevfromrecord = false;

char hud_trace_things_health[80];
char hud_trace_things_pickup[80];
char hud_trace_lines_cross[80];

DOOM_BOOL traces_present;
traceslist_t traces[3] = {
  {&things_health, hud_trace_things_health, "-trace_thingshealth", "\x1b\x31health "},
  {&things_pickup, hud_trace_things_pickup, "-trace_thingspickup", "\x1b\x31pickup "},
  {&lines_cross,   hud_trace_lines_cross,   "-trace_linescross"  , "\x1b\x31lcross "},
};

void CheckThingsPickupTracer(mobj_t *mobj)
{
  if (things_pickup.count)
  {
    int i;
    for (i=0;i<things_pickup.count;i++)
    {
      if (mobj->index == things_pickup.items[i].index)
        sprintf(things_pickup.items[i].value, "\x1b\x36%d \x1b\x33%05.2f", things_pickup.items[i].index, (float)(leveltime)/35);
    }
  }
}

void CheckThingsHealthTracer(mobj_t *mobj)
{
   if (things_health.count)
  {
    int i;
    for (i=0;i<things_health.count;i++)
    {
      if (mobj->index == things_health.items[i].index)
        sprintf(things_health.items[i].value, "\x1b\x36%d \x1b\x33%d", mobj->index, mobj->health);
    }
  }
}

int crossed_lines_count = 0;
void CheckLinesCrossTracer(line_t *line)
{
  if (lines_cross.count)
  {
    int i;
    crossed_lines_count++;
    for (i=0;i<lines_cross.count;i++)
    {
      if (line->iLineID == lines_cross.items[i].index)
      {
        if (!lines_cross.items[i].data1)
        {
          sprintf(lines_cross.items[i].value, "\x1b\x36%d \x1b\x33%05.2f", lines_cross.items[i].index, (float)(leveltime)/35);
          lines_cross.items[i].data1 = 1;
        }
      }
    }
  }
}

void ClearLinesCrossTracer(void)
{
  if (lines_cross.count)
  {
    if (!crossed_lines_count)
    {
      int i;
      for (i=0;i<lines_cross.count;i++)
      {
        lines_cross.items[i].data1 = 0;
      }
    }
    crossed_lines_count = 0;
  }
}

float paperitems_pitch;

int levelstarttic;

void I_AfterUpdateVideoMode(void)
{
#ifdef _WIN32
  // Move the window to the screen center if SDL create it in not working area.
  if (!desired_fullscreen)
  {
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    
    if(SDL_GetWMInfo(&wmInfo))
    {
      int SDL_width, SDL_height;
      RECT rectSDL, rectSCR;
      HWND hwndSDL = wmInfo.window;
      
      GetWindowRect(hwndSDL, &rectSDL);
      SDL_width = rectSDL.right - rectSDL.left;
      SDL_height = rectSDL.bottom - rectSDL.top;
      
      SystemParametersInfo(SPI_GETWORKAREA, 0, &rectSCR, 0);
      
      if (rectSDL.left < rectSCR.left || rectSDL.right > rectSCR.right || 
        rectSDL.top < rectSCR.top  || rectSDL.bottom > rectSCR.bottom)
      {
        MoveWindow(hwndSDL,
          MAX(rectSCR.left, rectSCR.left + (rectSCR.right - rectSCR.left - SDL_width) / 2),
          MAX(rectSCR.top, rectSCR.top + (rectSCR.bottom - rectSCR.top - SDL_height) / 2),
          SDL_width, SDL_height, TRUE);
      }
    }
  }

  I_SwitchToWindow(WIN32_GetHWND());
#endif

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL) {
    M_ChangeFOV();
    M_ChangeRenderPrecise();
    M_ChangeCompTranslucency();
  }
#endif
}

int force_singletics_to = 0;

DOOM_BOOL HU_DrawDemoProgress(void)
{
  int len;
  
  if (gamestate == GS_DEMOSCREEN || !demoplayback || !hudadd_demoprogressbar)
    return false;
  
  len = MIN(SCREENWIDTH, 
    (int)((int_64_t)SCREENWIDTH * demo_curr_tic / (demo_tics_count * demo_playerscount)));
    
  V_FillRect(0, 0, SCREENHEIGHT - 4, len - 0, 4, 4);
  if (len > 4)
    V_FillRect(0, 2, SCREENHEIGHT - 3, len - 4, 2, 0);

  return true;
}

#ifdef ALL_IN_ONE
unsigned char* GetAllInOneLumpHandle(void)
{
  static unsigned char* AllInOneLumpHandle = NULL;

  if (!AllInOneLumpHandle)
  {
    HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(IDR_ALL_IN_ONE_LUMP), RT_RCDATA);
    if (hrsrc)
    {
      HGLOBAL hglobal = LoadResource(NULL, hrsrc);
      if (hglobal)
      {
        AllInOneLumpHandle = LockResource(hglobal);
      }
    }
  }
  
  if (!AllInOneLumpHandle)
    I_Error("Can't load internal data.");

  return AllInOneLumpHandle;
}
#endif

#ifdef _MSC_VER
int GetFullPath(const char* FileName, const char* ext, char *Buffer, size_t BufferLength)
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

    Result = SearchPath(dir,FileName,ext,BufferLength,Buffer,&p);
    if (Result)
      return Result;
  }

  return false;
}
#endif

int IsDemoPlayback()
{
  int p =
    (((p = M_CheckParm("-fastdemo")) ||
      (p = M_CheckParm("-timedemo")) ||
      (p = M_CheckParm("-playdemo")))
     && p < myargc - 1);
  return p;
}

//Begin of GZDoom code
/*
**---------------------------------------------------------------------------
** Copyright 2004-2005 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of GZDoom or a GZDoom derivative, this code will be
**    covered by the terms of the GNU Lesser General Public License as published
**    by the Free Software Foundation; either version 2.1 of the License, or (at
**    your option) any later version.
*/

//===========================================================================
// 
// smooth the edges of transparent fields in the texture
// returns false when nothing is manipulated to save the work on further
// levels

// 28/10/2003: major optimization: This function was far too pedantic.
// taking the value of one of the neighboring pixels is fully sufficient
//
//===========================================================================

#ifdef WORDS_BIGENDIAN
#define MSB 0
#define SOME_MASK 0xffffff00
#else
#define MSB 3
#define SOME_MASK 0x00ffffff
#endif

#define CHKPIX(ofs) (l1[(ofs)*4+MSB]==255 ? (( ((long*)l1)[0] = ((long*)l1)[ofs]&SOME_MASK), trans=true ) : false)

DOOM_BOOL SmoothEdges(unsigned char * buffer,int w, int h)
{
  int x,y;
  DOOM_BOOL trans=buffer[MSB]==0; // If I set this to false here the code won't detect textures 
                                // that only contain transparent pixels.
  unsigned char * l1;

  if (h<=1 || w<=1) return false;  // makes (a) no sense and (b) doesn't work with this code!

  l1=buffer;


  if (l1[MSB]==0 && !CHKPIX(1)) CHKPIX(w);
  l1+=4;
  for(x=1;x<w-1;x++, l1+=4)
  {
    if (l1[MSB]==0 &&  !CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
  }
  if (l1[MSB]==0 && !CHKPIX(-1)) CHKPIX(w);
  l1+=4;

  for(y=1;y<h-1;y++)
  {
    if (l1[MSB]==0 && !CHKPIX(-w) && !CHKPIX(1)) CHKPIX(w);
    l1+=4;
    for(x=1;x<w-1;x++, l1+=4)
    {
      if (l1[MSB]==0 &&  !CHKPIX(-w) && !CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
    }
    if (l1[MSB]==0 && !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(w);
    l1+=4;
  }

  if (l1[MSB]==0 && !CHKPIX(-w)) CHKPIX(1);
  l1+=4;
  for(x=1;x<w-1;x++, l1+=4)
  {
    if (l1[MSB]==0 &&  !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(1);
  }
  if (l1[MSB]==0 && !CHKPIX(-w)) CHKPIX(-1);

  return trans;
}

#undef MSB
#undef SOME_MASK
//End of GZDoom code
