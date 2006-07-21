#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <winreg.h>
#endif
#include <GL/gl.h>
#include <string.h>
#include <math.h>
#include <SDL_opengl.h>

#include <stdio.h>

#include "SDL.h"
#include <SDL_syswm.h>

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
#include "p_mobj.h"
#include "e6y.h"

#define Pi 3.14159265358979323846f

boolean wasWiped = false;

int secretfound;
int messagecenter_counter;
int demo_skiptics;
int demo_recordfromto = false;
int demo_lastheaderlen;

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
int hudadd_secretarea;
int hudadd_smarttotals;
int movement_strafe50;
int movement_strafe50onturns;
int movement_smooth;
int movement_altmousesupport;
int movement_mouselook;
int movement_mouseinvert;
int render_fov;
int render_usedetail;
int render_detailedwalls;
int render_detailedflats;
int render_multisampling;
int render_paperitems;
int render_smartitemsclipping;
int render_wipescreen;
int mouse_acceleration;
int demo_smoothturns;
int demo_smoothturnsfactor;
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

int test_sky1;
int test_sky2;
int test_dots;

int palette_ondamage;
int palette_onbonus;
int palette_onpowers;

float mouse_accelfactor;

camera_t walkcamera;
mobj_t *oviewer;

hu_textline_t  w_hudadd;
hu_textline_t  w_centermsg;
char hud_timestr[80];
char hud_centermsg[80];

fixed_t sidemove_normal[2]    = {0x18, 0x28};
fixed_t sidemove_strafe50[2]    = {0x18, 0x32};

fixed_t	r_TicFrac;
int otic;
boolean NewThinkerPresent = false;

fixed_t PrevX;
fixed_t PrevY;
fixed_t PrevZ;

fixed_t oviewx;
fixed_t oviewy;
fixed_t oviewz;
angle_t oviewangle;
angle_t oviewpitch;

int mouseSensitivity_mlook;
angle_t viewpitch;
float fovscale;
float tan_pitch;
float skyUpAngle;
float skyUpShift;
float skyXShift;
float skyYShift;

boolean SkyDrawed;

boolean isExtraDDisplay = false;
boolean skipDDisplay = false;
unsigned int DDisplayTime;


float internal_render_fov = FOV90;

int idDetail;
boolean gl_arb_multitexture;
PFNGLACTIVETEXTUREARBPROC        glActiveTextureARB       = NULL;
PFNGLMULTITEXCOORD2FVARBPROC     glMultiTexCoord2fvARB    = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC  glClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC      glMultiTexCoord2fARB     = NULL;

static boolean saved_fastdemo;
static boolean saved_nodrawers;
static boolean saved_nosfxparm;
static boolean saved_nomusicparm;

//--------------------------------------------------
#ifdef _WIN32
static HWND GetHWND(void);
void SwitchToWindow(HWND hwnd);
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

void e6y_D_DoomMainSetup(void)
{
  void G_RecordDemo (const char* name);
  void G_BeginRecording (void);

  int p;
  
  if ((p = M_CheckParm("-recordfromto")) && (p < myargc - 2))
  {
    _demofp = fopen(myargv[p+1], "rb");
    if (_demofp)
    {
      byte buf[200];
      byte *demo_p = buf;
      size_t len;
      fread(buf, 1, sizeof(buf), _demofp);
      len = G_ReadDemoHeader(buf) - buf;
      fseek(_demofp, len, SEEK_SET);
      if (demo_compatibility)
      {
        demo_recordfromto = true;
        singledemo = true;
        autostart = true;
        G_RecordDemo(myargv[p+2]);
        G_BeginRecording();
      }
      else
      {
        fclose(_demofp);
      }
    }
  }

  if ((p = M_CheckParm("-skipsec")) && (p < myargc-1))
    demo_skiptics = (int)(atof(myargv[p+1]) * 35);
  if ((gameaction == ga_playdemo||demo_recordfromto) && (startmap > 1 || demo_skiptics))
    G_SkipDemoStart();
  if ((p = M_CheckParm("-avidemo")) && (p < myargc-1))
    avi_shot_fname = myargv[p+1];
  force_monster_avoid_hazards = M_CheckParm("-force_monster_avoid_hazards");
  stats_level = M_CheckParm("-levelstat");

  {
    int i, value, count;

    for (i=0;i<3;i++)
    {
      count = 0;
      traces[i].trace->count = 0;
      if ((p = M_CheckParm(traces[i].cmd)) && (p < myargc-1))
      {
        while (count < 3 && p + count < myargc-1 && StrToInt(myargv[p+1+count], &value))
        {
          sprintf(traces[i].trace->items[count].value, "\x1b\x36%d\x1b\x33 0", value);
          traces[i].trace->items[count].index = value;
          count++;
        }
        traces[i].trace->count = count;
      }
    }
  }
}

void G_SkipDemoStart(void)
{
  saved_fastdemo = fastdemo;
  saved_nodrawers = nodrawers;
  saved_nosfxparm = nosfxparm;
  saved_nomusicparm = nomusicparm;
  
  doSkip = true;
  fastdemo = true;
  nodrawers = true;
  nosfxparm = true;
  nomusicparm = true;
  I_Init();
}

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
  I_Init();
  S_Init(snd_SfxVolume, snd_MusicVolume);
  S_Start();
}

void M_ChangeAltMouseHandling(void)
{
  if (movement_altmousesupport)
  {
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    GrabMouse_Win32();
  }
  else
  {
    SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
    SDL_WM_GrabInput(SDL_GRAB_ON);
    UngrabMouse_Win32();
  }
}

void M_ChangeSmooth(void)
{
}

void M_ChangeSpeed(void)
{
  extern int sidemove[2];
  extern setup_menu_t stat_settings2[];

  if(movement_strafe50)
  {
    stat_settings2[13].m_flags &= ~(S_SKIP|S_SELECT);
    sidemove[0] = sidemove_strafe50[0];
    sidemove[1] = sidemove_strafe50[1];
  }
  else
  {
    stat_settings2[13].m_flags |= (S_SKIP|S_SELECT);
    movement_strafe50onturns = false;
    sidemove[0] = sidemove_normal[0];
    sidemove[1] = sidemove_normal[1];
  }
}

void M_ChangeMouseLook(void)
{
#ifdef GL_DOOM
  extern setup_menu_t stat_settings3[];
  viewpitch = 0;
  if(movement_mouselook)
  {
    stat_settings3[12].m_flags &= ~(S_SKIP|S_SELECT);
    stat_settings3[13].m_flags &= ~(S_SKIP|S_SELECT);
  }
  else
  {
    stat_settings3[12].m_flags |= (S_SKIP|S_SELECT);
    stat_settings3[13].m_flags |= (S_SKIP|S_SELECT);
  }
#endif
}

boolean GetMouseLook(void)
{
#ifdef GL_DOOM
//  boolean ret = (demoplayback||demorecording)&&walkcamera.type==0?false:movement_mouselook;
  boolean ret = (demoplayback)&&walkcamera.type==0?false:movement_mouselook;
  if (!ret) 
    viewpitch = 0;
  return ret;
#else
  return false;
#endif
}

void CheckPitch(signed int *pitch)
{
#define maxAngle (ANG90 - (1<<ANGLETOFINESHIFT))
#define minAngle (-ANG90 + (1<<ANGLETOFINESHIFT))

  if(*pitch > maxAngle)
    *pitch = maxAngle;
  if(*pitch < minAngle)
    *pitch = minAngle;
}

void M_ChangeMouseInvert(void)
{
}

void M_ChangeFOV(void)
{
  float f1, f2;

#ifdef GL_DOOM
  internal_render_fov = (float)render_fov;
#else
  internal_render_fov = (float)FOV90;
#endif

  internal_render_fov = internal_render_fov/1.6f*FOV_CORRECTION_FACTOR;
  fovscale = FOV90/(float)render_fov;

  f1 = (float)(320.0f/200.0f/fovscale-0.2f);
  f2 = (float)tan(internal_render_fov/2.0f*Pi/180.0f);
  if (f1-f2<1)
    skyUpAngle = (float)-asin(f1-f2)*180.0f/Pi;
  else
    skyUpAngle = -90.0f;

  skyUpShift = (float)tan((internal_render_fov/2.0f)*Pi/180.0f);
}

void M_ChangeUseDetail(void)
{
#ifdef GL_DOOM
  render_usedetail = render_detailedflats || render_detailedwalls;
#endif
}

void M_ChangeMultiSample(void)
{
#ifdef GL_DOOM
#endif
}

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

unsigned int TicStart;
unsigned int TicNext;
unsigned int TicStep;
float TicksInMSec;

extern int_64_t I_GetTime_Scale;

void e6y_I_Init(void)
{
  TicksInMSec = realtic_clock_rate * TICRATE / 100000.0f;
}

void Extra_D_Display(void)
{
#ifdef GL_DOOM
  if (movement_smooth)
#else
    if (movement_smooth && gamestate==wipegamestate)
#endif
  {
    isExtraDDisplay = true;
    D_Display();
    isExtraDDisplay = false;
  }
}

fixed_t I_GetTimeFrac (void)
{
  unsigned long now;
  fixed_t frac;

  now = SDL_GetTicks();

  if (TicStep == 0)
    return FRACUNIT;
  else
  {
    frac = (fixed_t)((now - TicStart + DDisplayTime) * FRACUNIT / TicStep);
    if (frac < 0)
      frac = 0;
    if (frac > FRACUNIT)
      frac = FRACUNIT;
    return frac;
  }
}

void I_GetTime_SaveMS(void)
{
  if (!movement_smooth)
    return;

  TicStart = SDL_GetTicks();
  TicNext = (unsigned int) ((TicStart * TicksInMSec + 1.0f) / TicksInMSec);
  TicStep = TicNext - TicStart;
}

//------------

int numinterpolations = 0;
fixed_t oldipos[MAXINTERPOLATIONS][2];
fixed_t bakipos[MAXINTERPOLATIONS][2];
FActiveInterpolation curipos[MAXINTERPOLATIONS];
boolean NoInterpolateView;
boolean r_NoInterpolate;
static boolean didInterp;

void R_InterpolateView (player_t *player, fixed_t frac)
{
  if (movement_smooth)
  {
    if (NoInterpolateView)
    {
      NoInterpolateView = false;
      oviewx = player->mo->x;
      oviewy = player->mo->y;
      oviewz = player->viewz;

      oviewangle = player->mo->angle + viewangleoffset;
      oviewpitch = player->mo->pitch;

      if(walkcamera.type)
      {
        walkcamera.PrevX = walkcamera.x;
        walkcamera.PrevY = walkcamera.y;
        walkcamera.PrevZ = walkcamera.z;
        walkcamera.PrevAngle = walkcamera.angle;
        walkcamera.PrevPitch = walkcamera.pitch;
      }
    }

    if (walkcamera.type != 2)
    {
      viewx = oviewx + FixedMul (frac, player->mo->x - oviewx);
      viewy = oviewy + FixedMul (frac, player->mo->y - oviewy);
      viewz = oviewz + FixedMul (frac, player->viewz - oviewz);
    }
    else
    {
      viewx = walkcamera.PrevX + FixedMul (frac, walkcamera.x - walkcamera.PrevX);
      viewy = walkcamera.PrevY + FixedMul (frac, walkcamera.y - walkcamera.PrevY);
      viewz = walkcamera.PrevZ + FixedMul (frac, walkcamera.z - walkcamera.PrevZ);
    }

    if (walkcamera.type)
    {
      viewangle = walkcamera.PrevAngle + FixedMul (frac, walkcamera.angle - walkcamera.PrevAngle);
      viewpitch = walkcamera.PrevPitch + FixedMul (frac, walkcamera.pitch - walkcamera.PrevPitch);
    }
    else
    {
      viewangle = oviewangle + FixedMul (frac, GetSmoothViewAngel(player->mo->angle) + viewangleoffset - oviewangle);
      viewpitch = oviewpitch + FixedMul (frac, player->mo->pitch /*+ viewangleoffset*/ - oviewpitch);
    }
  }
  else
  {
    if (walkcamera.type != 2)
    {
      viewx = player->mo->x;
      viewy = player->mo->y;
      viewz = player->viewz;
    }
    else
    {
      viewx = walkcamera.x;
      viewy = walkcamera.y;
      viewz = walkcamera.z;
    }
    if (walkcamera.type)
    {
      viewangle = walkcamera.angle;
      viewpitch = walkcamera.pitch;
    }
    else
    {
      viewangle = GetSmoothViewAngel(player->mo->angle);
      //viewangle = player->mo->angle + viewangleoffset;
      viewpitch = player->mo->pitch;// + viewangleoffset;
    }
  }
}

void R_ResetViewInterpolation ()
{
  NoInterpolateView = true;
}

void CopyInterpToOld (int i)
{
  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->ceilingheight;
    break;
  case INTERP_Vertex:
    oldipos[i][0] = ((vertex_t*)curipos[i].Address)->x;
    oldipos[i][1] = ((vertex_t*)curipos[i].Address)->y;
    break;
  case INTERP_WallPanning:
    oldipos[i][0] = ((side_t*)curipos[i].Address)->rowoffset;
    oldipos[i][1] = ((side_t*)curipos[i].Address)->textureoffset;
    break;
  case INTERP_FloorPanning:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->floor_xoffs;
    oldipos[i][1] = ((sector_t*)curipos[i].Address)->floor_yoffs;
    break;
  case INTERP_CeilingPanning:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->ceiling_xoffs;
    oldipos[i][1] = ((sector_t*)curipos[i].Address)->ceiling_yoffs;
    break;
  }
}

void CopyBakToInterp (int i)
{
  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    ((sector_t*)curipos[i].Address)->floorheight = bakipos[i][0];
    break;
  case INTERP_SectorCeiling:
    ((sector_t*)curipos[i].Address)->ceilingheight = bakipos[i][0];
    break;
  case INTERP_Vertex:
    ((vertex_t*)curipos[i].Address)->x = bakipos[i][0];
    ((vertex_t*)curipos[i].Address)->y = bakipos[i][1];
    break;
  case INTERP_WallPanning:
    ((side_t*)curipos[i].Address)->rowoffset = bakipos[i][0];
    ((side_t*)curipos[i].Address)->textureoffset = bakipos[i][1];
    break;
  case INTERP_FloorPanning:
    ((sector_t*)curipos[i].Address)->floor_xoffs = bakipos[i][0];
    ((sector_t*)curipos[i].Address)->floor_yoffs = bakipos[i][1];
    break;
  case INTERP_CeilingPanning:
    ((sector_t*)curipos[i].Address)->ceiling_xoffs = bakipos[i][0];
    ((sector_t*)curipos[i].Address)->ceiling_yoffs = bakipos[i][1];
    break;
  }
}

void DoAnInterpolation (int i, fixed_t smoothratio)
{
  fixed_t pos;
  fixed_t *adr1 = NULL;
  fixed_t *adr2 = NULL;

  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    adr1 = &((sector_t*)curipos[i].Address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    adr1 = &((sector_t*)curipos[i].Address)->ceilingheight;
    break;
  case INTERP_Vertex:
    adr1 = &((vertex_t*)curipos[i].Address)->x;
////    adr2 = &((vertex_t*)curipos[i].Address)->y;
    break;
  case INTERP_WallPanning:
    adr1 = &((side_t*)curipos[i].Address)->rowoffset;
    adr2 = &((side_t*)curipos[i].Address)->textureoffset;
    break;
  case INTERP_FloorPanning:
    adr1 = &((sector_t*)curipos[i].Address)->floor_xoffs;
    adr2 = &((sector_t*)curipos[i].Address)->floor_yoffs;
    break;
  case INTERP_CeilingPanning:
    adr1 = &((sector_t*)curipos[i].Address)->ceiling_xoffs;
    adr2 = &((sector_t*)curipos[i].Address)->ceiling_yoffs;
    break;

 default:
    return;
  }

  if (adr1)
  {
    pos = bakipos[i][0] = *adr1;
    *adr1 = oldipos[i][0] + FixedMul (pos - oldipos[i][0], smoothratio);
  }

  if (adr2)
  {
    pos = bakipos[i][1] = *adr2;
    *adr2 = oldipos[i][1] + FixedMul (pos - oldipos[i][1], smoothratio);
  }
  /*{
    static FILE *f = NULL;
    if (!f) f = fopen("d:\\a.txt", "wb");
    fprintf(f, "%.10d:%.10d:%.10d %.10d-%.10d, %f-%f\n", gametic, smoothratio, DDisplayTime, oldipos[i][0], *adr1, oldipos[i][0]/65536.0f, *adr1/65536.0f);
  }*/
}

void updateinterpolations()
{
  int i;
  if (!movement_smooth)
    return;
  for (i = numinterpolations-1; i >= 0; --i)
    CopyInterpToOld (i);
}

void setinterpolation(EInterpType type, void *posptr)
{
  int i;
  if (!movement_smooth)
    return;
  if (numinterpolations >= MAXINTERPOLATIONS)
    return;
  for(i = numinterpolations-1; i >= 0; i--)
    if (curipos[i].Address == posptr && curipos[i].Type == type)
      return;
  curipos[numinterpolations].Address = posptr;
  curipos[numinterpolations].Type = type;
  CopyInterpToOld (numinterpolations);
  numinterpolations++;
} 

void stopinterpolation(EInterpType type, void *posptr)
{
  int i;

  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= 0; --i)
  {
    if (curipos[i].Address == posptr && curipos[i].Type == type)
    {
      numinterpolations--;
      oldipos[i][0] = oldipos[numinterpolations][0];
      oldipos[i][1] = oldipos[numinterpolations][1];
      bakipos[i][0] = bakipos[numinterpolations][0];
      bakipos[i][1] = bakipos[numinterpolations][1];
      curipos[i] = curipos[numinterpolations];
      break;
    }
  }
}

void stopallinterpolation(void)
{
  int i;
  
  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= 0; --i)
  {
    numinterpolations--;
    oldipos[i][0] = oldipos[numinterpolations][0];
    oldipos[i][1] = oldipos[numinterpolations][1];
    bakipos[i][0] = bakipos[numinterpolations][0];
    bakipos[i][1] = bakipos[numinterpolations][1];
    curipos[i] = curipos[numinterpolations];
  }
}

void dointerpolations(fixed_t smoothratio)
{
  int i;
  if (!movement_smooth)
    return;
  if (smoothratio == FRACUNIT)
  {
    didInterp = false;
    return;
  }

  didInterp = true;

  for (i = numinterpolations-1; i >= 0; --i)
  {
    DoAnInterpolation (i, smoothratio);
  }
}

void restoreinterpolations()
{
  int i;
  
  if (!movement_smooth)
    return;

  if (didInterp)
  {
    didInterp = false;
    for (i = numinterpolations-1; i >= 0; --i)
    {
      CopyBakToInterp (i);
    }
  }
}

void P_ActivateAllInterpolations()
{
  int i;
  sector_t     *sec;

  if (!movement_smooth)
    return;

  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
  {
    if (sec->floordata)
    {
      setinterpolation (INTERP_SectorFloor, sec);
    }
    if (sec->ceilingdata)
    {
      setinterpolation (INTERP_SectorCeiling, sec);
    }
  }
}

void InterpolationGetData(thinker_t *th,
  EInterpType *type1,EInterpType *type2,
  void **posptr1, void **posptr2)
{
  *posptr1 = NULL;
  *posptr2 = NULL;

  if (th->function == T_MoveFloor)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    *type1 = INTERP_SectorCeiling;
    *posptr1 = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    *type1 = INTERP_SectorCeiling;
    *posptr1 = ((vldoor_t *)th)->sector;
  }
  else
  if (th->function == T_MoveElevator)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((elevator_t *)th)->sector;
    *type2 = INTERP_SectorCeiling;
    *posptr2 = ((elevator_t *)th)->sector;
  }
  else
  if (th->function == T_Scroll)
  {
    switch (((scroll_t *)th)->type)
    {
      case sc_side:
        *type1 = INTERP_WallPanning;
        *posptr1 = sides + ((scroll_t *)th)->affectee;
        break;
      case sc_floor:
        *type1 = INTERP_FloorPanning;
        *posptr1 = sectors + ((scroll_t *)th)->affectee;
        break;
      case sc_ceiling:
        *type1 = INTERP_CeilingPanning;
        *posptr1 = sectors + ((scroll_t *)th)->affectee;
        break;
    }
  }
}

void SetInterpolationIfNew(thinker_t *th)
{
  void *posptr1;
  void *posptr2;
  EInterpType type1, type2;

  if (!movement_smooth)
    return;

  InterpolationGetData(th, &type1, &type2, &posptr1, &posptr2);

  if(posptr1)
  {
    setinterpolation (type1, posptr1);
    
    if(posptr2)
      setinterpolation (type2, posptr2);
  }
}

void StopInterpolationIfNeeded(thinker_t *th)
{
  void *posptr1;
  void *posptr2;
  EInterpType type1, type2;

  if (!movement_smooth)
    return;

  InterpolationGetData(th, &type1, &type2, &posptr1, &posptr2);

  if(posptr1)
  {
    stopinterpolation (type1, posptr1);
    if(posptr2)
      stopinterpolation (type2, posptr2);
  }
}

#ifdef GL_DOOM

float xCamera,yCamera;
TAnimItemParam *anim_flats = NULL;
TAnimItemParam *anim_textures = NULL;

void e6y_PreprocessLevel(void)
{
  if (gl_arb_multitexture)
  {
    extern void *gld_texcoords;

    glClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }
}

void e6y_InitExtensions(void)
{
#define isExtensionSupported(ext) strstr(extensions, ext)
  static int imageformats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

  extern int gl_tex_filter;
  extern int gl_mipmap_filter;
  extern int gl_texture_filter_anisotropic;
  extern int gl_tex_format;

  char *extensions = (char*)glGetString(GL_EXTENSIONS);

  gl_arb_multitexture = isExtensionSupported("GL_ARB_multitexture") != NULL;

  if (gl_arb_multitexture)
  {
    glActiveTextureARB       = SDL_GL_GetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");
    glMultiTexCoord2fvARB    = SDL_GL_GetProcAddress("glMultiTexCoord2fvARB");
    glMultiTexCoord2fARB     = SDL_GL_GetProcAddress("glMultiTexCoord2fARB");

    if (!glActiveTextureARB    || !glClientActiveTextureARB ||
        !glMultiTexCoord2fvARB || !glMultiTexCoord2fARB)
      gl_arb_multitexture = false;
  }
  //gl_arb_multitexture = false;

  //if (gl_arb_multitexture)
  {
    HRSRC hDetail;
    void *memDetail;
    SDL_PixelFormat fmt;
    SDL_Surface *surf = NULL;

    hDetail = FindResource(NULL, MAKEINTRESOURCE(115), RT_RCDATA);
    memDetail = LockResource(LoadResource(NULL, hDetail));

    surf = SDL_LoadBMP_RW(SDL_RWFromMem(memDetail, SizeofResource(NULL, hDetail)), 1);
    fmt = *surf->format;
    fmt.BitsPerPixel = 24;
    fmt.BytesPerPixel = 3;
    surf = SDL_ConvertSurface(surf, &fmt, surf->flags);
    if (surf)
    {
      if (gl_arb_multitexture)
        glActiveTextureARB(GL_TEXTURE1_ARB);
      glGenTextures(1, &idDetail);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, idDetail);
      
      gluBuild2DMipmaps(GL_TEXTURE_2D, 
        surf->format->BytesPerPixel, 
        surf->w, surf->h, 
        imageformats[surf->format->BytesPerPixel], 
        GL_UNSIGNED_BYTE, surf->pixels);
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
      //if (gl_texture_filter_anisotropic)
      //  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0);

      if (gl_arb_multitexture)
        glActiveTextureARB(GL_TEXTURE0_ARB);
      
      SDL_FreeSurface(surf);
    }
    else
      render_usedetail = false;
  }
  M_ChangeUseDetail();
  if (gl_arb_multitexture)
    lprintf(LO_INFO,"e6y: using GL_ARB_multitexture\n",glGetString(GL_VENDOR));
}

float distance2piece(float x0, float y0, float x1, float y1, float x2, float y2)
{
  float t, w;
  
  float x01 = x0-x1;
  float x02 = x0-x2;
  float x21 = x2-x1;
  float y01 = y0-y1;
  float y02 = y0-y2;
  float y21 = y2-y1;

  if((x01*x21+y01*y21)*(x02*x21+y02*y21)>0.0001f)
  {
    t = x01*x01 + y01*y01;
    w = x02*x02 + y02*y02;
    if (w < t) t = w;
  }
  else
  {
    float i1 = x01*y21-y01*x21;
    float i2 = x21*x21+y21*y21;
    t = (i1*i1)/i2;
  }
  return t;
}

#endif //GL_DOOM

//int demos_lastturns[MAX_DEMOS_SMOOTHFACTOR*2+1];
int demos_lastturns[MAX_DEMOS_SMOOTHFACTOR];
int_64_t demos_lastturnssum;
int demos_lastturnsindex;
angle_t demos_smoothangle;
const byte *demo_p_end;
int playerscount;

void GetCurrentTurnsSum(void);

void e6y_ProcessDemoHeader(void)
{
  int i;
  playerscount = 0;
  for (i=0; i < MAXPLAYERS; i++)
    if (playeringame[i])
      playerscount++;
}

void M_ChangeDemoSmoothTurns(void)
{
  extern setup_menu_t stat_settings2[];

  if (demo_smoothturns)
    stat_settings2[8].m_flags &= ~(S_SKIP|S_SELECT);
  else
    stat_settings2[8].m_flags |= (S_SKIP|S_SELECT);

  ClearSmoothViewAngels(NULL);
}

void ClearSmoothViewAngels(player_t *player)
{
  if (demo_smoothturns && demoplayback && players)
  {
    if (!player)
      player = &players[displayplayer];

    if (player==&players[displayplayer])
    {
      demos_smoothangle = players[displayplayer].mo->angle;

      memset(demos_lastturns, 0, sizeof(demos_lastturns[0]) * MAX_DEMOS_SMOOTHFACTOR);
      demos_lastturnssum = 0;
      demos_lastturnsindex = 0;
    }
  }
}

void AddSmoothViewAngel(int delta)
{
  if (demo_smoothturns && demoplayback)
  {
    demos_lastturnssum -= demos_lastturns[demos_lastturnsindex];
    demos_lastturns[demos_lastturnsindex] = delta;
//    demos_lastturnsindex = (demos_lastturnsindex + 1)%(demo_smoothturnsfactor*2+1);
    demos_lastturnsindex = (demos_lastturnsindex + 1)%(demo_smoothturnsfactor);
    demos_lastturnssum += delta;

//    demos_smoothangle += (int)(demos_lastturnssum/(demo_smoothturnsfactor*2+1));
    demos_smoothangle += (int)(demos_lastturnssum/(demo_smoothturnsfactor));
  }
}

angle_t GetSmoothViewAngel(angle_t defangle)
{
  if (demo_smoothturns && demoplayback)
    return demos_smoothangle;
  else
    return defangle;
}

void e6y_AfterTeleporting(player_t *player)
{
  R_ResetViewInterpolation();
  ClearSmoothViewAngels(player);
}

float viewPitch;
boolean WasRenderedInTryRunTics;
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
    extern int use_fullscreen;
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

int force_monster_avoid_hazards = false;

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

  if (value < defval && newvalue > defval || value > defval && newvalue < defval)
    newvalue = defval;

  if (newvalue != value)
  {
    prev_value = value;
    prev_direction = direction;
  }

  return newvalue;
}

void I_Warning(const char *message, va_list argList)
{
  char msg[1024];
#ifdef HAVE_VSNPRINTF
  vsnprintf(msg,sizeof(msg),message,argList);
#else
  vsprintf(msg,message,argList);
#endif
  fprintf(stdout,"%s\n",msg);
#ifdef _MSC_VER
  {
    extern HWND con_hWnd;
    SwitchToWindow(GetDesktopWindow());
    Init_ConsoleWin();
    if (con_hWnd) SwitchToWindow(con_hWnd);
    MessageBox(con_hWnd,msg,"PrBoom-Plus",MB_OK | MB_TASKMODAL | MB_TOPMOST);
    SwitchToWindow(GetHWND());
  }
#endif
}

void ShowOverflowWarning(int emulate, int *promted, boolean fatal, char *name, char *params, ...)
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
    I_Warning(buffer, argptr);
    va_end(argptr);
  }
}

void SpechitOverrun(line_t* ld)
{
  extern int numspechit;
  extern line_t **spechit;

  if (numspechit>8 && demo_compatibility)
  {
    if (overrun_spechit_warn)
      ShowOverflowWarning(overrun_spechit_emulate, &overrun_spechit_promted, numspechit > 20, "SPECHITS",
        "\n\nThe list of LinesID leading to overrun:\n%d, %d, %d, %d, %d, %d, %d, %d, %d.",
        spechit[0]->iLineID, spechit[1]->iLineID, spechit[2]->iLineID,
        spechit[3]->iLineID, spechit[4]->iLineID, spechit[5]->iLineID,
        spechit[6]->iLineID, spechit[7]->iLineID, spechit[8]->iLineID);

    if (overrun_spechit_emulate)
    {
      int addr = 0x01C09C98 + (ld - lines) * 0x3E;
      if (compatibility_level == dosdoom_compatibility || compatibility_level == tasdoom_compatibility)
      {
        extern fixed_t   tmfloorz;
        extern fixed_t   tmceilingz;

        switch(numspechit)
        {
        case 9: 
          tmfloorz = addr;
          break;
        case 10:
          tmceilingz = addr;
          break;
          
        default:
          fprintf(stderr, "SpechitOverrun: Warning: unable to emulate"
            "an overrun where numspechit=%i\n",
            numspechit);
          break;
        }
      }
      else
      {
        extern fixed_t tmbbox[4];
        extern boolean crushchange, nofit;

        switch(numspechit)
        {
        case 9: 
        case 10:
        case 11:
        case 12:
          tmbbox[numspechit-9] = addr;
          break;
        case 13:
          nofit = addr;
          break;
        case 14:
          crushchange = addr;
          break;

        default:
          fprintf(stderr, "SpechitOverrun: Warning: unable to emulate"
                          "an overrun where numspechit=%i\n",
                           numspechit);
          break;
        }
      }
    }
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

  if (doSkip && demo_stoponend)
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
  
  for(i=0; i<TT_MAX; i++)
     max.stat[i] = strlen(itoa( max.stat[i], str, 10));

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
static long MousePrevX, MousePrevY;
static boolean MakeMouseEvents;
HWND GetHWND(void)
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

void CenterMouse_Win32(long x, long y)
{
#ifdef _WIN32
  //RECT rect;
  //HWND hwnd = GetHWND();
	
  //GetWindowRect (hwnd, &rect);

  //MousePrevX = (rect.left + rect.right) >> 1;
  //MousePrevY = (rect.top + rect.bottom) >> 1;
  MousePrevX = SCREENWIDTH/2;
  MousePrevY = SCREENHEIGHT/2;

  if (MousePrevX != x || MousePrevY != y)
  {
    SetCursorPos (MousePrevX, MousePrevY);
  }
#endif
}

void e6y_I_GetEvent(void)
{
#ifdef _WIN32
  extern int usemouse;
  int I_SDLtoDoomMouseState(Uint8 buttonstate);

  if (movement_altmousesupport && usemouse && MakeMouseEvents)
  {
    POINT pos;
    int x, y;
    GetCursorPos (&pos);

    x = pos.x - MousePrevX;
    y = MousePrevY - pos.y;

    if (x | y)
    {
      event_t event;
      
      CenterMouse_Win32(pos.x, pos.y);

      event.type = ev_mouse;
      event.data1 = I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
      event.data2 = x << 5;
      event.data3 = y << 5;
      D_PostEvent (&event);
    }
  }
#endif
}

void GrabMouse_Win32(void)
{
#ifdef _WIN32
  RECT rect;
  HWND hwnd = GetHWND();

  ClipCursor (NULL);
  GetClientRect (hwnd, &rect);

  ClientToScreen (hwnd, (LPPOINT)&rect.left);
  ClientToScreen (hwnd, (LPPOINT)&rect.right);

  ClipCursor (&rect);
  CenterMouse_Win32(-1, -1);
  MakeMouseEvents = true;
#endif
}

void UngrabMouse_Win32(void)
{
#ifdef _WIN32
  ClipCursor(NULL);
  MakeMouseEvents = false;
#endif
}

void e6y_I_InitInputs(void)
{
//  SDL_WarpMouse((unsigned short)(desired_screenwidth/2), (unsigned short)(desired_screenheight/2));
  SDL_WarpMouse((unsigned short)(SCREENWIDTH/2), (unsigned short)(SCREENHEIGHT/2));
  M_ChangeAltMouseHandling();
  MouseAccelChanging();
}

int AccelerateMouse(int val)
{
  if (!mouse_acceleration)
    return val;

  if (val < 0)
    return -AccelerateMouse(-val);
  return (int) pow(val, mouse_accelfactor);
}

int rjreq, rjlen;

void AddIntForRejectOverflow(int k)
{
  extern byte *rejectmatrix;
  int i = 0;

  if (rjlen < rjreq && demo_compatibility
    && (overrun_reject_warn || overrun_reject_emulate))
  {
    if (overrun_reject_warn)
      ShowOverflowWarning(overrun_reject_emulate, &overrun_reject_promted, rjreq - rjlen > 16, "REJECT", "");
    
    if (overrun_reject_emulate)
    {
      while (rjlen < rjreq)
      {
        rejectmatrix[rjlen++] = (k & 0x000000ff);
        k >>= 8;
        if ((++i)==4) break;
      }
    }
  }
}

int mlooky;

boolean IsDehMaxHealth = false;
int deh_maxhealth;
int maxhealthbonus;

void M_ChangeCompTranslucency(void)
{
//  static boolean predefined_translucency_fixed = false;
//  static int predefined_translucency_level = -1;
  int i;
  int predefined_translucency[] = {
    MT_FIRE, MT_SMOKE, MT_FATSHOT, MT_BRUISERSHOT, MT_SPAWNFIRE,
    MT_TROOPSHOT, MT_HEADSHOT, MT_PLASMA, MT_BFG, MT_ARACHPLAZ, MT_PUFF, 
    MT_TFOG, MT_IFOG, MT_MISC12, MT_INV, MT_INS, MT_MEGA
  };
  
//  if (!predefined_translucency_fixed || predefined_translucency_level != compatibility_level)
  {
//    predefined_translucency_fixed = true;
//    predefined_translucency_level = compatibility_level;
    for(i = 0; i < sizeof(predefined_translucency)/sizeof(predefined_translucency[0]); i++)
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
  {
    extern int maxhealth;
    if (comp[comp_maxhealth]) 
    {
      maxhealth = 100;
      maxhealthbonus = (IsDehMaxHealth?deh_maxhealth:200);
    }
    else 
    {
      maxhealth = (IsDehMaxHealth?deh_maxhealth:100);
      maxhealthbonus = maxhealth * 2;
    }
  }
  M_ChangeCompTranslucency();
}

boolean zerotag_manual;
int comperr_zerotag;
int comperr_passuse;

boolean compbad_get(int *compbad)
{
  return !demo_compatibility && (*compbad) && !demorecording && !demoplayback;
}

/*boolean CompErrZeroTag(void)
{
  return !demo_compatibility && comperr_zerotag && !demorecording && !demoplayback;
}

boolean CompErrPassUse(void)
{
  return !demo_compatibility && comperr_passuse && !demorecording && !demoplayback;
}*/

int G_GetOriginalDoomCompatLevel(int ver)
{
  {
    int lev;
    int i = M_CheckParm("-complevel");
    if (i && (i+1 < myargc))
    {
      lev = atoi(myargv[i+1]);
      if (lev>=0)
        return lev;
    }
  }
  return (ver < 107 ? doom_1666_compatibility :
      (gamemode == retail) ? ultdoom_compatibility :
      (gamemission >= pack_tnt) ? finaldoom_compatibility :
      doom2_19_compatibility);
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

void SwitchToWindow(HWND hwnd)
{
#ifdef _WIN32
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
#endif
}

boolean StrToInt(const char *s, long *l)
{      
  return (
    (sscanf(s, " 0x%x", l) == 1) ||
    (sscanf(s, " 0X%x", l) == 1) ||
    (sscanf(s, " 0%o", l) == 1) ||
    (sscanf(s, " %d", l) == 1)
  );
}

boolean PlayeringameOverrun(mapthing_t* mthing)
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
boolean isskytexture = false;