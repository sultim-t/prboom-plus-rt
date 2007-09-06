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
#include "i_main.h"
#include "m_menu.h"
#include "p_spec.h"
#include "lprintf.h"
#include "d_think.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_maputl.h"
#include "i_video.h"
#include "info.h"
#include "i_simd.h"
#include "gl_struct.h"
#include "g_game.h"
#include "e6y.h"
#include "./../ICONS/resource.h"

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

#define DEFAULT_SPECHIT_MAGIC (0x01C09C98)
//#define DEFAULT_SPECHIT_MAGIC (0x84000000)

spriteclipmode_t gl_spriteclip;
const char *gl_spriteclipmodes[] = {"constant","always", "smart"};
int gl_sprite_offset;

int REAL_SCREENWIDTH;
int REAL_SCREENHEIGHT;
int REAL_SCREENPITCH;

boolean wasWiped = false;

int secretfound;
int messagecenter_counter;
int demo_skiptics;
int democontinue = false;
int demo_playerscount;
int demo_tics_count;
int demo_curr_tic;
char demo_len_st[80];

int avi_shot_time;
int avi_shot_num;
const char *avi_shot_fname;
char avi_shot_curr_fname[PATH_MAX];

FILE    *_demofp;
boolean doSkip;
boolean demo_stoponnext;
boolean demo_stoponend;
boolean demo_warp;

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
int render_screen_multiply;
int screen_multiply;
int render_interlaced_scanning;
int interlaced_scanning_requires_clearing;
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

boolean was_aspected;
int render_aspect_width;
int render_aspect_height;
float render_aspect_ratio;

int misc_fastexit;

int test_skybox;

unsigned int spechit_magic;

char *sdl_videodriver;
int palette_ondamage;
int palette_onbonus;
int palette_onpowers;

float mouse_accelfactor;

camera_t walkcamera;

hu_textline_t  w_hudadd;
hu_textline_t  w_centermsg;
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
boolean mlook_or_fov;
int real_viewangle;

float internal_render_fov = FOV90;

int maxViewPitch;
int minViewPitch;

static boolean saved_fastdemo;
static boolean saved_nodrawers;
static boolean saved_nosfxparm;
static boolean saved_nomusicparm;

//--------------------------------------------------
#ifdef _WIN32
static HWND WIN32_GetHWND(void);
static void SwitchToWindow(HWND hwnd);
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
  boolean recording_attempt = 
    M_CheckParm("-record") || 
    M_CheckParm("-recordfrom") ||
    M_CheckParm("-recordfromto");
  
  boolean playbacking_attempt = 
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
  {0x00000000, 0x02040601, 0, "-force_incorrect_processing_of_respawn_frame_entry"},
  {0x00000000, 0x02040601, 0, "-force_correct_code_for_3_keys_doors_in_mbf"},
  {0x00000000, 0x02040601, 0, "-uninitialize_crush_field_for_stairs"},
  {0x00000000, 0x02040802, 0, "-force_boom_findnexthighestfloor"},
  {0x00000000, 0x02040802, 0, "-allow_sky_transfer_in_boom"},
};

char democontinuename[PATH_MAX];

void e6y_InitCommandLine(void)
{
  int i, p;

  //-recordfromto x y
  {
    char demoname[PATH_MAX];
    boolean bDemoContinue = false;
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

  // spechit magic
  spechit_magic = DEFAULT_SPECHIT_MAGIC;
  if ((p = M_CheckParm("-spechit")) && (p < myargc-1))
  {
    if (!StrToInt(myargv[p+1], (long*)&spechit_magic))
      spechit_magic = DEFAULT_SPECHIT_MAGIC;
  }

  shorttics = M_CheckParm("-shorttics");
}

void G_CheckDemoContinue(void)
{
  if (democontinue)
  {
    void G_RecordDemo (const char* name);
    void G_BeginRecording (void);

    byte buf[512];
    byte *demo_p = buf;
    size_t len = fread(buf, 1, sizeof(buf), _demofp);
    len = G_ReadDemoHeader(buf, len, true) - buf;
    fseek(_demofp, len, SEEK_SET);

    singledemo = true;
    autostart = true;
    G_RecordDemo(democontinuename);
    G_BeginRecording();
  }
}

void G_SkipDemoStart(void)
{
  saved_fastdemo = fastdemo;
  saved_nodrawers = nodrawers;
  saved_nosfxparm = nosfxparm;
  saved_nomusicparm = nomusicparm;
  
  doSkip = true;

  S_StopMusic();
  fastdemo = true;
  nodrawers = true;
  nosfxparm = true;
  nomusicparm = true;
  I_Init2();
}

boolean sound_inited_once = false;

void G_SkipDemoStop(void)
{
  fastdemo = saved_fastdemo;
  nodrawers = saved_nodrawers;
  nosfxparm = saved_nosfxparm;
  nomusicparm = saved_nomusicparm;

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
}

void M_ChangeSpeed(void)
{
  extern int sidemove[2];
  extern setup_menu_t gen_settings4[];

  if(movement_strafe50)
  {
    gen_settings4[6].m_flags &= ~(S_SKIP|S_DISABLE);
    sidemove[0] = sidemove_strafe50[0];
    sidemove[1] = sidemove_strafe50[1];
  }
  else
  {
    gen_settings4[6].m_flags |= (S_SKIP|S_DISABLE);
    movement_strafe50onturns = false;
    sidemove[0] = sidemove_normal[0];
    sidemove[1] = sidemove_normal[1];
  }
}

#ifdef GL_DOOM
void M_ChangeMouseLook(void)
{
  extern setup_menu_t gen_settings5[];
  viewpitch = 0;
  if(movement_mouselook)
  {
    gen_settings5[4].m_flags &= ~(S_SKIP|S_DISABLE);
    gen_settings5[5].m_flags &= ~(S_SKIP|S_DISABLE);
  }
  else
  {
    gen_settings5[4].m_flags |= (S_SKIP|S_DISABLE);
    gen_settings5[5].m_flags |= (S_SKIP|S_DISABLE);
  }
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

void M_ChangeGLRenderPrecise(void)
{
  if (V_GetMode() != VID_MODEGL)
  {
    gl_seamless = false;
    return;
  }

  if (gl_render_precise)
  {
    gl_seamless = true;
    gld_InitVertexData();
  }
  else
  {
    gl_seamless = false;
    gld_CleanVertexData();
  }
}
#endif

void M_ChangeScreenMultipleFactor(void)
{
  extern setup_menu_t gen_settings6[];
  if(render_screen_multiply != 1)
  {
    gen_settings6[2].m_flags &= ~(S_SKIP|S_DISABLE);
  }
  else
  {
    gen_settings6[2].m_flags |= (S_SKIP|S_DISABLE);
  }
}

void M_ChangeInterlacedScanning(void)
{
  if (render_interlaced_scanning)
    interlaced_scanning_requires_clearing = 1;
}

boolean GetMouseLook(void)
{
  if (V_GetMode() == VID_MODEGL)
  {
    boolean ret = (demoplayback)&&walkcamera.type==0?false:movement_mouselook;
    if (!ret) 
      viewpitch = 0;
    return ret;
  }
  else
    return false;
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
  extern setup_menu_t gen_settings6[];

  if(gl_spriteclip == spriteclip_const)
  {
    gen_settings6[10].m_flags &= ~(S_SKIP|S_DISABLE);
  }
  else
  {
    gen_settings6[10].m_flags |= (S_SKIP|S_DISABLE);
  }
}

void M_ChangeAllowBoomColormaps(void)
{
  if (gl_boom_colormaps == -1)
    gl_boom_colormaps = gl_boom_colormaps_default;
  
  if (gl_boom_colormaps_default)
  {
    gl_texture_usehires_default = false;
    gl_patch_usehires_default = false;
  }
}

void M_ChangeTextureUseHires(void)
{
  extern setup_menu_t gen_settings6[];

  if (gl_texture_usehires == -1)
    gl_texture_usehires = gl_texture_usehires_default;

  if (gl_patch_usehires == -1)
    gl_patch_usehires = gl_patch_usehires_default;

  if (gl_texture_usehires_default || gl_patch_usehires_default)
    gl_boom_colormaps_default = false;
}
#endif

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
boolean transparentpresent;

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

void e6y_MultisamplingPrint(void)
{
  int temp;
  SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &temp );
  lprintf(LO_INFO,"    SDL_GL_MULTISAMPLESAMPLES: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &temp );
  lprintf(LO_INFO,"    SDL_GL_MULTISAMPLEBUFFERS: %i\n",temp);
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
  {
    extern HWND con_hWnd;
    SwitchToWindow(GetDesktopWindow());
    Init_ConsoleWin();
    if (con_hWnd) SwitchToWindow(con_hWnd);
    MessageBox(con_hWnd,msg,"PrBoom-Plus",MB_OK | MB_TASKMODAL | MB_TOPMOST);
    SwitchToWindow(WIN32_GetHWND());
  }
#endif
}

void I_Warning(const char *message, ...)
{
  va_list argptr;
  va_start(argptr,message);
  I_vWarning(message, argptr);
  va_end(argptr);
}

void ShowOverflowWarning(int emulate, int *promted, boolean fatal, const char *name, const char *params, ...)
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
#ifdef HAVE_SNPRINTF
    snprintf (str, 200, "%d", max.stat[i]);
#else
    sprintf (str, "%d", max.stat[i]);
#endif
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

boolean IsDehMaxHealth = false;
boolean IsDehMaxSoul = false;
boolean IsDehMegaHealth = false;
boolean DEH_mobjinfo_bits[NUMMOBJTYPES] = {0};

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

boolean zerotag_manual;
int comperr_zerotag;
int comperr_passuse;
int comperr_hangsolid;

boolean compbad_get(int *compbad)
{
  return !demo_compatibility && (*compbad) && !demorecording && !demoplayback;
}

boolean ProcessNoTagLines(line_t* line, sector_t **sec, int *secnum)
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

#ifdef _WIN32
void SwitchToWindow(HWND hwnd)
{
  typedef BOOL (WINAPI *TSwitchToThisWindow) (HWND wnd, BOOL restore);
  TSwitchToThisWindow SwitchToThisWindow = NULL;

  if (!SwitchToThisWindow)
    SwitchToThisWindow = (TSwitchToThisWindow)GetProcAddress(GetModuleHandle("user32.dll"), "SwitchToThisWindow");
  
  if (SwitchToThisWindow)
  {
    HWND hwndLastActive = GetLastActivePopup(hwnd);

    if (IsWindowVisible(hwndLastActive))
      hwnd = hwndLastActive;

    SetForegroundWindow(hwnd);
    Sleep(100);
    SwitchToThisWindow(hwnd, TRUE);
  }
}
#endif

boolean PlayeringameOverrun(const mapthing_t* mthing)
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

typedef struct
{
  int len;
  int* adr;
} intercepts_overrun_t;

void InterceptsOverrun(size_t num_intercepts, intercept_t *intercept)
{
  if (num_intercepts>128 && demo_compatibility
    && (overrun_intercept_warn || overrun_intercept_emulate))
  {
    if (overrun_intercept_warn)
      ShowOverflowWarning(overrun_intercept_emulate, &overrun_intercept_promted, false, "INTERCEPTS", "");

    if (overrun_intercept_emulate)
    {
      
      extern fixed_t bulletslope;
      extern mobj_t **blocklinks;
      extern int bmapwidth, bmapheight;
      extern long *blockmap;
      extern fixed_t bmaporgx, bmaporgy;
      extern long *blockmaplump;
      
      intercepts_overrun_t overrun[] = 
      {
        {4, (int*)0},
        {4, (int*)0},//&earlyout},
        {4, (int*)0},//&intercept_p},
        {4, (int*)&lowfloor},
        {4, (int*)&openbottom},
        {4, (int*)&opentop},
        {4, (int*)&openrange},
        {4, (int*)0},
        {120, (int*)0},//&activeplats},
        {8, (int*)0},
        {4, (int*)&bulletslope},
        {4, (int*)0},//&swingx},
        {4, (int*)0},//&swingy},
        {4, (int*)0},
        {40, (int*)&playerstarts},
        {4, (int*)0},//&blocklinks},
        {4, (int*)&bmapwidth},
        {4, (int*)0},//&blockmap},
        {4, (int*)&bmaporgx},
        {4, (int*)&bmaporgy},
        {4, (int*)0},//&blockmaplump},
        {4, (int*)&bmapheight},
        {0, (int*)0}
      };
      
      int i, j, offset;
      int count = (num_intercepts - 128) * 12 - 12;
      int* value = (int*)intercept;
      
      for (j = 0; j < 3; j++, value++, count+=4)
      {
        i = 0;
        offset = 0;
        while (overrun[i].len)
        {
          if (offset + overrun[i].len > count)
          {
            if (overrun[i].adr)
              *(overrun[i].adr+(count-offset)/4) = *value;
            break;
          }
          offset += overrun[i++].len;
        }
      }
    }
  }
}

char hud_trace_things_health[80];
char hud_trace_things_pickup[80];
char hud_trace_lines_cross[80];

boolean traces_present;
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

static void R_ProcessScreenMultiplyBlock2x(byte* pixels_src, byte* pixels_dest, 
                                         int pitch_src, int pitch_dest,
                                         int ytop, int ybottom,
                                         int interlaced)
{
  int x, y;

  if (interlaced)
  {
    for (y = ytop; y <= ybottom; y++)
    {
      unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 2));
      byte *psrc = pixels_src + y * pitch_src;
      x = SCREENWIDTH / 2;
      while (x > 0)
      {
        unsigned int data_dest = *psrc | (*(psrc + 1) << 16);
        data_dest |= data_dest << 8;
        psrc += 2;
        x--;
        *pdest++ = data_dest;
      }
    }
  }
  else
  {
    for (y = ytop; y <= ybottom; y++)
    {
      unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 2));
      byte *pdest_saved = (byte*)pdest;
      byte *psrc = pixels_src + y * pitch_src;
      x = SCREENWIDTH / 2;
      while (x > 0)
      {
        unsigned int data_dest = *psrc | (*(psrc + 1) << 16);
        data_dest |= data_dest << 8;
        psrc += 2;
        x--;
        *pdest++ = data_dest;
      }
      memcpy_fast(pdest_saved + pitch_dest, pdest_saved, pitch_dest);
    }
  }
}

static void R_ProcessScreenMultiplyBlock4x(byte* pixels_src, byte* pixels_dest, 
                                         int pitch_src, int pitch_dest,
                                         int ytop, int ybottom,
                                         int interlaced)
{
  int i, x, y;

  for (y = ytop; y <= ybottom; y++)
  {
    unsigned int *pdest = (unsigned int *)(pixels_dest + y * (pitch_dest * 4));
    byte *pdest_saved = (byte*)pdest;
    byte *psrc = pixels_src + y * pitch_src;
    x = SCREENWIDTH;
    while (x > 0)
    {
      unsigned int data_dest = *psrc | (*psrc << 16);
      data_dest |= data_dest << 8;
      psrc++;
      x--;
      *pdest++ = data_dest;
    }
    if (!interlaced)
    {
      for (i = 1; i < screen_multiply; i++)
        memcpy_fast(pdest_saved + i * pitch_dest, pdest_saved, pitch_dest);
    }
  }
}

static void R_ProcessScreenMultiplyBlock(byte* pixels_src, byte* pixels_dest, 
                                         int pitch_src, int pitch_dest,
                                         int ytop, int ybottom,
                                         int interlaced)
{
  int x, y, i;
  byte *psrc = pixels_src + pitch_src * ybottom;
  byte *pdest = pixels_dest + pitch_dest * (ybottom * screen_multiply);
  byte *pdest_saved = pdest;
  byte *data_src;

  if (screen_multiply == 2)
  {
    for (y = ybottom; y >= ytop; y--)
    {
      data_src = psrc;
      psrc -= pitch_src; 
      for (x = 0; x < SCREENWIDTH / 2 ; x++, data_src += 2)
      {
        unsigned int data_dest = *data_src | ((*(data_src + 1)) << 16);
        data_dest |= data_dest << 8;
        *((unsigned int*)pdest) = data_dest;
        pdest += sizeof(unsigned int);
      }
      if (!interlaced)
        memcpy_fast(pdest_saved + pitch_dest, pdest_saved, pitch_dest);
      pdest_saved = pdest = pdest_saved - pitch_dest * 2;
    }
  }
  else
  {
    for (y = ybottom; y >= ytop; y--)
    {
      data_src = psrc;
      psrc -= pitch_src; 
      for (x = 0; x < SCREENWIDTH ; x++, data_src++)
      {
        for (i = 0; i < screen_multiply; i++)
          *pdest++ = *data_src;
      }
      if (!render_interlaced_scanning)
      {
        for (i = 1; i < screen_multiply; i++)
          memcpy_fast(pdest_saved + i * pitch_dest, pdest_saved, pitch_dest);
      }
      pdest_saved = pdest = pdest_saved - pitch_dest * screen_multiply;
    }
  }
}

void R_ProcessScreenMultiply(byte* pixels_src, byte* pixels_dest, int pitch_src, int pitch_dest)
{
  if (screen_multiply > 1)
  {
    // there is no necessity to do it each tic
    if (interlaced_scanning_requires_clearing)
    {
      // needed for "directx" video driver with double buffering
      // two pages should be cleared
      interlaced_scanning_requires_clearing = (interlaced_scanning_requires_clearing + 1)%3;

      memset_fast(pixels_dest, 0, pitch_dest * screen_multiply * SCREENHEIGHT);
    }

    switch (screen_multiply)
    {
    case 2:
      R_ProcessScreenMultiplyBlock2x(pixels_src, pixels_dest, 
        pitch_src, pitch_dest, 0, SCREENHEIGHT - 1, render_interlaced_scanning);
      break;
    case 4:
      R_ProcessScreenMultiplyBlock4x(pixels_src, pixels_dest, 
        pitch_src, pitch_dest, 0, SCREENHEIGHT - 1, render_interlaced_scanning);
      break;
    default:
      {
        // e6y: The following code works correctly even if pixels_src == pixels_dest
        boolean same = (pixels_src == pixels_dest);
        R_ProcessScreenMultiplyBlock(pixels_src, pixels_dest, 
          pitch_src, pitch_dest, (same ? 1 : 0), SCREENHEIGHT - 1, render_interlaced_scanning);
        if (same)
        {
          // never happens after SDL_LockSurface()
          static byte *tmpbuf = NULL;
          if (!tmpbuf)
            tmpbuf = malloc(pitch_src);
          memcpy_fast(tmpbuf, pixels_src, pitch_src);
          R_ProcessScreenMultiplyBlock(tmpbuf, pixels_dest, 
            pitch_src, pitch_dest, 0, 0, render_interlaced_scanning);
        }
        break;
      }
    }
  }
}

int demo_patterns_count;
char *demo_patterns_mask;
char **demo_patterns_list;
char *demo_patterns_list_def[9];

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
#endif

#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL) {
    M_ChangeFOV();
    M_ChangeGLRenderPrecise();
  }
#endif
}

int force_singletics_to = 0;

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
          free((char*)waddata->wadfiles[i].name);
          waddata->wadfiles[i].name = NULL;
        }
      }
      free(waddata->wadfiles);
      waddata->wadfiles = NULL;
    }
  }
}

int ParseDemoPattern(const char *str, waddata_t* waddata, char **missed)
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
#ifdef _MSC_VER
    token = malloc(PATH_MAX);
    if (GetFullPath(pToken, ".wad", token, PATH_MAX))
#else
    if ((token = I_FindFile(pToken, ".wad")))
#endif
    {
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = token;
      
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
  
  memset(waddata, 0, sizeof(*waddata));

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
            (patterndata ? &patterndata->missed : NULL));

          waddata->wadfiles = realloc(waddata->wadfiles, sizeof(*wadfiles)*(waddata->numwadfiles+1));
          waddata->wadfiles[waddata->numwadfiles].name = strdup(demoname);
          waddata->wadfiles[waddata->numwadfiles].src = source_lmp;
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
      ProcessNewIWAD(waddata->wadfiles[i].name);
      iwadindex = i;
      break;
    }
  }

  if (iwadindex == -1)
  {
    I_Error("IdentifyVersion: IWAD not found\n");
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
      D_AddFile(waddata->wadfiles[i].name, source_pwad);
      modifiedgame = true;
    }
    if (waddata->wadfiles[i].src == source_deh)
      ProcessDehFile(waddata->wadfiles[i].name, D_dehout(), 0);
  }

  for (i = 0; (size_t)i < waddata->numwadfiles; i++)
  {
    if (waddata->wadfiles[i].src == source_lmp || waddata->wadfiles[i].src == source_net)
      D_AddFile(waddata->wadfiles[i].name, waddata->wadfiles[i].src);
  }

  free(old_wadfiles);
}

void CheckAutoDemo(void)
{
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
}

void ProcessNewIWAD(const char *iwad)
{
  extern boolean haswolflevels;
  void CheckIWAD(const char *iwadname,GameMode_t *gmode,boolean *hassec);

  int i;

  if (iwad && *iwad)
  {
    CheckIWAD(iwad,&gamemode,&haswolflevels);
    
    switch(gamemode)
    {
    case retail:
    case registered:
    case shareware:
      gamemission = doom;
      break;
    case commercial:
      i = strlen(iwad);
      gamemission = doom2;
      if (i>=10 && !strnicmp(iwad+i-10,"doom2f.wad",10))
        language=french;
      else if (i>=7 && !strnicmp(iwad+i-7,"tnt.wad",7))
        gamemission = pack_tnt;
      else if (i>=12 && !strnicmp(iwad+i-12,"plutonia.wad",12))
        gamemission = pack_plut;
      break;
    default:
      gamemission = none;
      break;
    }
    if (gamemode == indetermined)
      lprintf(LO_WARN,"Unknown Game Version, may not work\n");

    D_AddFile(iwad,source_iwad);
  }
}

boolean HU_DrawDemoProgress(void)
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

#define CHKPIX(ofs) \
	(l1[(ofs)*4+3]==255 ? (( ((long*)l1)[0] = ((long*)l1)[ofs]&0xffffff), trans=true ) : false)

boolean SmoothEdges(unsigned char * buffer,int w, int h)
{
	int x,y;
	boolean trans=buffer[3]==0; // If I set this to false here the code won't detect textures 
							 // that only contain transparent pixels.
	unsigned char * l1;

	if (h<=1 || w<=1) return false;	// makes (a) no sense and (b) doesn't work with this code!

	l1=buffer;


	if (l1[3]==0 && !CHKPIX(1)) CHKPIX(w);
	l1+=4;
	for(x=1;x<w-1;x++, l1+=4)
	{
		if (l1[3]==0 &&	!CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
	}
	if (l1[3]==0 && !CHKPIX(-1)) CHKPIX(w);
	l1+=4;

	for(y=1;y<h-1;y++)
	{
		if (l1[3]==0 && !CHKPIX(-w) && !CHKPIX(1)) CHKPIX(w);
		l1+=4;
		for(x=1;x<w-1;x++, l1+=4)
		{
			if (l1[3]==0 &&	!CHKPIX(-w) && !CHKPIX(-1) && !CHKPIX(1)) CHKPIX(w);
		}
		if (l1[3]==0 && !CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(w);
		l1+=4;
	}

	if (l1[3]==0 && !CHKPIX(-w)) CHKPIX(1);
	l1+=4;
	for(x=1;x<w-1;x++, l1+=4)
	{
		if (l1[3]==0 &&	!CHKPIX(-w) && !CHKPIX(-1)) CHKPIX(1);
	}
	if (l1[3]==0 && !CHKPIX(-w)) CHKPIX(-1);

	return trans;
}
//End of GZDoom code
