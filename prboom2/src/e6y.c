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
#include "i_sound.h"
#include "m_menu.h"
#include "lprintf.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_maputl.h"
#include "p_map.h"
#include "i_video.h"
#include "info.h"
#include "r_main.h"
#include "r_things.h"
#include "r_sky.h"
#include "am_map.h"
#include "hu_tracers.h"
#ifdef GL_DOOM
#include "gl_struct.h"
#include "gl_intern.h"
#endif
#include "g_game.h"
#include "r_demo.h"
#include "d_deh.h"
#include "e6y.h"

dboolean wasWiped = false;

int secretfound;
int demo_skiptics;
int demo_playerscount;
int demo_tics_count;
int demo_curr_tic;
char demo_len_st[80];

int avi_shot_time;
int avi_shot_num;
const char *avi_shot_fname;

dboolean doSkip;
dboolean demo_stoponnext;
dboolean demo_stoponend;
dboolean demo_warp;

int key_speed_up;
int key_speed_down;
int key_speed_default;
int speed_step;
int key_level_restart;
int key_nextlevel;
int key_demo_jointogame;
int key_demo_endlevel;
int key_demo_skip;
int key_walkcamera;
int key_showalive;

int hudadd_gamespeed;
int hudadd_leveltime;
int hudadd_demotime;
int hudadd_secretarea;
int hudadd_smarttotals;
int hudadd_demoprogressbar;
int hudadd_crosshair;
int hudadd_crosshair_scale;
int hudadd_crosshair_color;
int hudadd_crosshair_health;
int hudadd_crosshair_target;
int hudadd_crosshair_target_color;
int hudadd_crosshair_lock_target;
int movement_strafe50;
int movement_shorttics;
int movement_mouselook;
int movement_mouseinvert;
int movement_maxviewpitch;
int mouse_handler;
int mouse_doubleclick_as_use;
int render_fov = 90;
int render_multisampling;
int render_paperitems;
int render_wipescreen;
int mouse_acceleration;
int demo_overwriteexisting;

int showendoom;

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

int mouseSensitivity_mlook;
angle_t viewpitch;
float skyscale;
float screen_skybox_zplane;
float tan_pitch;
float skyUpAngle;
float skyUpShift;
float skyXShift;
float skyYShift;
dboolean mlook_or_fov;

int maxViewPitch;
int minViewPitch;

#ifdef _WIN32
const char* WINError(void)
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
  dboolean recording_attempt = 
    M_CheckParm("-record") || 
    M_CheckParm("-recordfrom") ||
    M_CheckParm("-recordfromto");
  
  dboolean playbacking_attempt = 
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
  {0x02020200, 0x02050003, 0, "-force_incorrect_bobbing_in_boom"},
  {0xffffffff, 0x00000000, 0, "-boom_deh_parser"},
  {0x00000000, 0x02050007, 0, "-mbf_remove_thinker_in_killmobj"},
  {0x00000000, 0x02050007, 0, "-do_not_inherit_friendlyness_flag_on_spawn"},
  {0x00000000, 0x02050007, 0, "-do_not_use_misc12_frame_parameters_in_a_mushroom"},
  {0x00000000, 0x02050102, 0, "-apply_mbf_codepointers_to_any_complevel"},
  {0x00000000, 0x02050104, 0, "-reset_monsterspawner_params_after_loading"},
};

void e6y_InitCommandLine(void)
{
  int p;

  if ((p = M_CheckParm("-skipsec")) && (p < myargc-1))
  {
    float min, sec;

    if (sscanf(myargv[p+1], "%f:%f", &min, &sec) == 2)
      demo_skiptics = (int) ((60 * min + sec) * TICRATE);
    else if (sscanf(myargv[p+1], "%f", &sec) == 1)
      demo_skiptics = (int) (sec * TICRATE);
  }

  if ((IsDemoPlayback() || IsDemoContinue()) && (startmap > 1 || demo_skiptics))
    G_SkipDemoStart();
  if ((p = M_CheckParm("-avidemo")) && (p < myargc-1))
    avi_shot_fname = myargv[p + 1];
  stats_level = M_CheckParm("-levelstat");

  // TAS-tracers
  InitTracers();

  shorttics = movement_shorttics || M_CheckParm("-shorttics");
}

static dboolean saved_fastdemo;
static dboolean saved_nodrawers;
static dboolean saved_nosfxparm;
static dboolean saved_nomusicparm;

void G_SkipDemoStart(void)
{
  saved_fastdemo = fastdemo;
  saved_nodrawers = nodrawers;
  saved_nosfxparm = nosfxparm;
  saved_nomusicparm = nomusicparm;

  paused = false;
  
  doSkip = true;

  S_StopMusic();
  fastdemo = true;
  nodrawers = true;
  nosfxparm = true;
  nomusicparm = true;

  I_Init2();
}

dboolean sound_inited_once = false;

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
    I_InitSound();
  }
  S_Init(snd_SfxVolume, snd_MusicVolume);
  S_Stop();
  S_RestartMusic();

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

int G_ReloadLevel(void)
{
  int result = false;

  if ((gamestate == GS_LEVEL) &&
      !deathmatch && !netgame &&
      !demorecording && !demoplayback &&
      !menuactive)
  {
    G_DeferedInitNew(gameskill, gameepisode, gamemap);
    result = true;
  }

  return result;
}

int G_GotoNextLevel(void)
{
	static byte doom2_next[33] = {
	  2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	  12, 13, 14, 15, 31, 17, 18, 19, 20, 21,
	  22, 23, 24, 25, 26, 27, 28, 29, 30, 1,
	  32, 16, 3
	};
	static byte doom_next[4][9] = {
	  {12, 13, 19, 15, 16, 17, 18, 21, 14},
	  {22, 23, 24, 25, 29, 27, 28, 31, 26},
	  {32, 33, 34, 35, 36, 39, 38, 41, 37},
	  {42, 49, 44, 45, 46, 47, 48, 11, 43}
	};
	int epsd;
	int map = -1;

	int changed = false;
	if (gamemapinfo != NULL)
	{
		const char *n;
		if (gamemapinfo->nextsecret[0]) n = gamemapinfo->nextsecret;
		else n = gamemapinfo->nextmap;
		G_ValidateMapName(n, &epsd, &map);
	}

	if (map == -1)
	{
		// secret level
		doom2_next[14] = (haswolflevels ? 31 : 16);

		if (bfgedition && singleplayer)
		{
			if (gamemission == pack_nerve)
			{
				doom2_next[3] = 9;
				doom2_next[7] = 1;
				doom2_next[8] = 5;
			}
			else
				doom2_next[1] = 33;
		}

		// shareware doom has only episode 1
		doom_next[0][7] = (gamemode == shareware ? 11 : 21);

		doom_next[2][7] = ((gamemode == registered) ||
			// the fourth episode for pre-ultimate complevels is not allowed.
			(compatibility_level < ultdoom_compatibility) ?
			11 : 41);

		if ((gamestate == GS_LEVEL) &&
			!deathmatch && !netgame &&
			!demorecording && !demoplayback &&
			!menuactive)
		{
			//doom2_next and doom_next are 0 based, unlike gameepisode and gamemap
			epsd = gameepisode - 1;
			map = gamemap - 1;

			if (gamemode == commercial)
			{
				epsd = 1;
				map = doom2_next[BETWEEN(0, 32, map)];
			}
			else
			{
				int next = doom_next[BETWEEN(0, 3, epsd)][BETWEEN(0, 9, map)];
				epsd = next / 10;
				map = next % 10;
			}
		}
	}
	G_DeferedInitNew(gameskill, epsd, map);
	changed = true;

	return changed;
}

void M_ChangeSpeed(void)
{
  G_SetSpeed();
}

void M_ChangeMouseLook(void)
{
  viewpitch = 0;

  R_InitSkyMap();

#ifdef GL_DOOM
  if (gl_skymode == skytype_auto)
    gl_drawskys = (movement_mouselook ? skytype_skydome : skytype_standard);
  else
    gl_drawskys = gl_skymode;
#endif // GL_DOOM
}

void M_ChangeMouseInvert(void)
{
}

void M_ChangeMaxViewPitch(void)
{
  int max_up, max_dn, angle_up, angle_dn;
  
  if (V_GetMode() == VID_MODEGL)
  {
    max_up = movement_maxviewpitch;
    max_dn = movement_maxviewpitch;
  }
  else
  {
    max_up = MIN(movement_maxviewpitch, 56);
    max_dn = MIN(movement_maxviewpitch, 32);
  }

  angle_up = (int)((float)max_up / 45.0f * ANG45);
  angle_dn = (int)((float)max_dn / 45.0f * ANG45);

  maxViewPitch = (+angle_up - (1<<ANGLETOFINESHIFT));
  minViewPitch = (-angle_dn + (1<<ANGLETOFINESHIFT));

  viewpitch = 0;
}

void M_ChangeScreenMultipleFactor(void)
{
  V_ChangeScreenResolution();
}

dboolean GetMouseLook(void)
{
  return movement_mouselook;
}
dboolean HaveMouseLook(void)
{
  return (viewpitch != 0);
}

void CheckPitch(signed int *pitch)
{
  if(*pitch > maxViewPitch)
    *pitch = maxViewPitch;
  if(*pitch < minViewPitch)
    *pitch = minViewPitch;

  (*pitch) >>= 16;
  (*pitch) <<= 16;
}

int render_aspect;
float render_ratio;
float render_fovratio;
float render_fovy = FOV90;
float render_multiplier;

void M_ChangeAspectRatio(void)
{
  extern int screenblocks;

  M_ChangeFOV();

  R_SetViewSize(screenblocks);
}

void M_ChangeStretch(void)
{
  extern int screenblocks;

  render_stretch_hud = render_stretch_hud_default;

  R_SetViewSize(screenblocks);
}

void M_ChangeFOV(void)
{
  float f1, f2;
  int p;
  int render_aspect_width, render_aspect_height;

  if ((p = M_CheckParm("-aspect")) && (p+1 < myargc) && (strlen(myargv[p+1]) <= 21) &&
     (2 == sscanf(myargv[p+1], "%dx%d", &render_aspect_width, &render_aspect_height)))
  {
    SetRatio(SCREENWIDTH, SCREENHEIGHT);
    render_fovratio = (float)render_aspect_width / (float)render_aspect_height;
    render_ratio = RMUL * render_fovratio;
    render_multiplier = 64.0f / render_fovratio / RMUL;
  }
  else
  {
    SetRatio(SCREENWIDTH, SCREENHEIGHT);
    render_ratio = gl_ratio;
    render_multiplier = (float)ratio_multiplier;
    if (!tallscreen)
    {
      render_fovratio = 1.6f;
    }
    else
    {
      render_fovratio = render_ratio;
    }
  }

  render_fovy = (float)(2 * RAD2DEG(atan(tan(DEG2RAD(render_fov) / 2) / render_fovratio)));

  screen_skybox_zplane = 320.0f/2.0f/(float)tan(DEG2RAD(render_fov/2));

  f1 = (float)(320.0f / 200.0f * (float)render_fov / (float)FOV90 - 0.2f);
  f2 = (float)tan(DEG2RAD(render_fovy)/2.0f);
  if (f1-f2<1)
    skyUpAngle = (float)-RAD2DEG(asin(f1-f2));
  else
    skyUpAngle = -90.0f;

  skyUpShift = (float)tan(DEG2RAD(render_fovy)/2.0f);

  skyscale = 1.0f / (float)tan(DEG2RAD(render_fov / 2));
}

#ifdef GL_DOOM
void M_ChangeMultiSample(void)
{
}

void M_ChangeSpriteClip(void)
{
  gl_sprite_offset = (gl_spriteclip != spriteclip_const ? 0 : (.01f * (float)gl_sprite_offset_default));
  gl_spriteclip_threshold_f = (float)gl_spriteclip_threshold / MAP_COEFF;
}

void ResolveColormapsHiresConflict(dboolean prefer_colormap)
{
  gl_boom_colormaps = !r_have_internal_hires && !gl_texture_external_hires;
#if 0
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
#endif
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
    gl_boom_colormaps = gl_boom_colormaps_default;
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

float viewPitch;
dboolean transparentpresent;

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
  doom_vsnprintf(msg,sizeof(msg),message,argList);
  lprintf(LO_ERROR, "%s\n", msg);
#ifdef _MSC_VER
  I_MessageBox(msg, PRB_MB_OK);
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
    result = MessageBox(GetDesktopWindow(), text, PACKAGE_NAME, type|MB_TASKMODAL|MB_TOPMOST);
    I_SwitchToWindow(current_hwnd);
    return result;
  }
#endif

#if 0
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
#else
  return PRB_IDCANCEL;
#endif
}

int stats_level;
int numlevels = 0;
int levels_max = 0;
timetable_t *stats = NULL;

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
        char strtmp[200];
        strcpy(str, tmp.kill[0] == '\0' ? "%s%d" : "%s+%d");

        doom_snprintf(strtmp, sizeof(strtmp), str, tmp.kill, stats[level].kill[i]);
        strcpy(tmp.kill, strtmp);
        
        doom_snprintf(strtmp, sizeof(strtmp), str, tmp.item, stats[level].item[i]);
        strcpy(tmp.item, strtmp);
        
        doom_snprintf(strtmp, sizeof(strtmp), str, tmp.secret, stats[level].secret[i]);
        strcpy(tmp.secret, strtmp);
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
    doom_snprintf(str, 200, "%d", max.stat[i]);
    max.stat[i] = strlen(str);
  }

  for (level=0;level<numlevels;level++)
  {
    sprintf(str,
      "%%s - %%%dd:%%05.2f (%%%dd:%%02d)  K: %%%dd/%%-%dd%%%ds  I: %%%dd/%%-%dd%%%ds  S: %%%dd/%%-%dd %%%ds\r\n",
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
    int episode = 0;
    int map = 0;
    int p;

    if ((p = M_CheckParm ("-warp")))
    {
      if (gamemode == commercial)
      {
        if (p < myargc - 1)
          map = atoi(myargv[p + 1]);
      }
      else
      {
        if (p < myargc - 2)
        {
          episode = atoi(myargv[++p]);
          map = atoi(myargv[p + 1]);
        }
      }
    }

    demo_warp = demo_stoponnext ||
      (gamemode == commercial ? (map == gamemap) : (episode == gameepisode && map == gamemap));
    
    if (demo_warp && demo_skiptics == 0 && !firstmap)
      G_SkipDemoStop();

    firstmap = 0;
  }
}

//--------------------------------------------------

int AccelerateMouse(int val)
{
  if (!mouse_acceleration)
    return val;

  if (val < 0)
    return -AccelerateMouse(-val);

  return M_DoubleToInt(pow((double)val, (double)mouse_accelfactor));
}

int mlooky = 0;

void e6y_G_Compatibility(void)
{
  deh_applyCompatibility();

  if (IsDemoPlayback())
  {
    int i, p;

    //"2.4.8.2" -> 0x02040802
    if ((p = M_CheckParm("-emulate")) && (p < myargc - 1))
    {
      unsigned int emulated_version = 0;
      int b[4], k = 1;
      memset(b, 0, sizeof(b));
      sscanf(myargv[p + 1], "%d.%d.%d.%d", &b[0], &b[1], &b[2], &b[3]);
      for (i = 3; i >= 0; i--, k *= 256)
      {
#ifdef RANGECHECK
        if (b[i] >= 256)
          I_Error("Wrong version number of package: %s", PACKAGE_VERSION);
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

  P_CrossSubsector = P_CrossSubsector_PrBoom;
  if (!prboom_comp[PC_FORCE_LXDOOM_DEMO_COMPATIBILITY].state)
  {
    if (demo_compatibility)
      P_CrossSubsector = P_CrossSubsector_Doom;

    switch (compatibility_level)
    {
    case boom_compatibility_compatibility:
    case boom_201_compatibility:
    case boom_202_compatibility:
    case mbf_compatibility:
      P_CrossSubsector = P_CrossSubsector_Boom;
    break;
    }
  }
}

dboolean zerotag_manual;

dboolean ProcessNoTagLines(line_t* line, sector_t **sec, int *secnum)
{
  zerotag_manual = false;
  if (line->tag == 0 && comperr(comperr_zerotag))
  {
    if (!(*sec=line->backsector))
      return true;
    *secnum = (*sec)->iSectorID;
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
  size_t l;

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

int levelstarttic;

int force_singletics_to = 0;

int HU_DrawDemoProgress(int force)
{
  static unsigned int last_update = 0;
  static int prev_len = -1;

  int len, tics_count, diff;
  unsigned int tick, max_period;
  
  if (gamestate == GS_DEMOSCREEN || (!demoplayback && !democontinue) || !hudadd_demoprogressbar)
    return false;

  tics_count = ((doSkip && demo_skiptics > 0) ? MIN(demo_skiptics, demo_tics_count) : demo_tics_count) * demo_playerscount;
  len = MIN(SCREENWIDTH, (int)((int_64_t)SCREENWIDTH * demo_curr_tic / tics_count));

  if (!force)
  {
    max_period = ((tics_count - demo_curr_tic > 35 * demo_playerscount) ? 500 : 15);

    // Unnecessary updates of progress bar
    // can slow down demo skipping and playback
    tick = SDL_GetTicks();
    if (tick - last_update < max_period)
      return false;
    last_update = tick;

    // Do not update progress bar if difference is small
    diff = len - prev_len;
    if (diff == 0 || diff == 1) // because of static prev_len
      return false;
  }

  prev_len = len;
  
  V_FillRect(0, 0, SCREENHEIGHT - 4, len - 0, 4, 4);
  if (len > 4)
    V_FillRect(0, 2, SCREENHEIGHT - 3, len - 4, 2, 0);

  return true;
}

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

#ifdef _WIN32
#include <Mmsystem.h>
#pragma comment( lib, "winmm.lib" )
int mus_extend_volume;
void I_midiOutSetVolumes(int volume)
{
  // NSM changed to work on the 0-15 volume scale,
  // and to check mus_extend_volume itself.
  
  MMRESULT result;
  int calcVolume;
  MIDIOUTCAPS capabilities;
  unsigned int i;

  if (volume > 15)
    volume = 15;
  if (volume < 0)
    volume = 0;
  calcVolume = (65535 * volume / 15);

  //SDL_LockAudio(); // this function doesn't touch anything the audio callback touches

  //Device loop
  for (i = 0; i < midiOutGetNumDevs(); i++)
  {
    //Get device capabilities
    result = midiOutGetDevCaps(i, &capabilities, sizeof(capabilities));
    if (result == MMSYSERR_NOERROR)
    {
      //Adjust volume on this candidate
      if ((capabilities.dwSupport & MIDICAPS_VOLUME))
      {
        midiOutSetVolume((HMIDIOUT)i, MAKELONG(calcVolume, calcVolume));
      }
    }
  }

  //SDL_UnlockAudio();
}
#endif

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

#define CHKPIX(ofs) (l1[(ofs)*4+MSB]==255 ? (( ((unsigned int*)l1)[0] = ((unsigned int*)l1)[ofs]&SOME_MASK), trans=true ) : false)

dboolean SmoothEdges(unsigned char * buffer,int w, int h)
{
  int x,y;
  dboolean trans=buffer[MSB]==0; // If I set this to false here the code won't detect textures 
                                // that only contain transparent pixels.
  unsigned char * l1;

  // makes (a) no sense and (b) doesn't work with this code!
  // if (h<=1 || w<=1)
  // e6y: Do not smooth small patches.
  // Makes sense for HUD small digits
  // 2 and 7 still look ugly
  if (h<=8 || w<=8)
    return false;

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
