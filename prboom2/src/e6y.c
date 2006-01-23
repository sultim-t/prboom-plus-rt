#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
#include "e6y.h"

#define Pi 3.14159265358979323846f

boolean wasWiped = false;

int secretfound;
int messagecenter_counter;
int demo_skiptics;
int demo_recordfromto = false;

int avi_shot_count;
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
int movement_mouselook;
int movement_mouseinvert;
int movement_strafe50;
int movement_strafe50onturns;
int movement_smooth;
int movement_altmousesupport;
int render_fov;
int render_usedetail;
int render_detailwalls;
int render_detailflats;
int render_detailsprites;
int render_detailanims;
int render_usedetailwalls;
int render_usedetailflats;
int render_usedetailsprites;
int render_multisampling;
int render_smartitemsclipping;
int render_wipescreen;
int mouse_acceleration;
int demo_smoothturns;
int demo_smoothturnsfactor;
int demo_overwriteexisting;
int misc_fixfirstmousemotion;
int misc_spechitoverrun_warn;
int misc_spechitoverrun_emulate;

int test_sky1;
int test_sky2;

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
boolean NewThinkerPresent = FALSE;

fixed_t PrevX;
fixed_t PrevY;
fixed_t PrevZ;

fixed_t oviewx;
fixed_t oviewy;
fixed_t oviewz;
angle_t oviewangle;
angle_t oviewpitch;

int PitchSign;
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
#endif
//--------------------------------------------------

void e6y_assert(const char *format, ...) 
{
  static FILE *f = NULL;
  va_list argptr;
  va_start(argptr,format);
  //if (!f)
    f = fopen("d:\\a.txt", "a+");
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
  if ((p = M_CheckParm("-framescapture")) && (p < myargc-2))
    if ((avi_shot_count = avi_shot_time = atoi(myargv[p+1])))
      avi_shot_fname = myargv[p+2];
  force_monster_avoid_hazards = M_CheckParm("-force_monster_avoid_hazards");
  stats_level = M_CheckParm("-levelstat");
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
  extern int sidemove_normal[2];
  extern int sidemove_strafe50[2];

  if(movement_strafe50)
  {
    sidemove[0] = sidemove_strafe50[0];
    sidemove[1] = sidemove_strafe50[1];
  }
  else
  {
    sidemove[0] = sidemove_normal[0];
    sidemove[1] = sidemove_normal[1];
  }
}

void M_ChangeMouseLook(void)
{
  viewpitch = 0;
}

boolean GetMouseLook(void)
{
#ifdef GL_DOOM
  boolean ret = (demoplayback||demorecording)&&walkcamera.type==0?false:movement_mouselook;
  if (!ret) viewpitch = 0;
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
  if(movement_mouseinvert)
    PitchSign = +1;
  else
    PitchSign = -1;
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
  extern setup_menu_t stat_settings3[];
  
  render_usedetail = render_usedetail;// && gl_arb_multitexture;

  render_usedetailwalls   = render_usedetail && render_detailwalls;
  render_usedetailflats   = render_usedetail && render_detailflats;
  render_usedetailsprites = render_usedetail && render_detailsprites;

  if (render_usedetail)
  {
    stat_settings3[5].m_flags &= ~(S_SKIP|S_SELECT);
    stat_settings3[6].m_flags &= ~(S_SKIP|S_SELECT);
//    stat_settings3[7].m_flags &= ~(S_SKIP|S_SELECT);
  }
  else
  {
    stat_settings3[5].m_flags |= (S_SKIP|S_SELECT);
    stat_settings3[6].m_flags |= (S_SKIP|S_SELECT);
//    stat_settings3[7].m_flags |= (S_SKIP|S_SELECT);
  }
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

extern int realtic_clock_rate;
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
int startofdynamicinterpolations = 0;
fixed_t oldipos[MAXINTERPOLATIONS][1];
fixed_t bakipos[MAXINTERPOLATIONS][1];
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
  }
}

void DoAnInterpolation (int i, fixed_t smoothratio)
{
  fixed_t *adr1, pos;

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
 default:
    return;
  }

  pos = bakipos[i][0] = *adr1;
  *adr1 = oldipos[i][0] + FixedMul (pos - oldipos[i][0], smoothratio);
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
  if (numinterpolations >= MAXINTERPOLATIONS) return;
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

  for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
  {
    if (curipos[i].Address == posptr && curipos[i].Type == type)
    {
      numinterpolations--;
      oldipos[i][0] = oldipos[numinterpolations][0];
      bakipos[i][0] = bakipos[numinterpolations][0];
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

  for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
  {
    numinterpolations--;
    oldipos[i][0] = oldipos[numinterpolations][0];
    bakipos[i][0] = bakipos[numinterpolations][0];
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

void SetInterpolationIfNew(thinker_t *th)
{
  void *posptr = NULL;
  EInterpType type;
  int i;

  if (!movement_smooth)
    return;

  if (th->function == T_MoveFloor)
  {
    type = INTERP_SectorFloor;
    posptr = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    type = INTERP_SectorFloor;
    posptr = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    type = INTERP_SectorCeiling;
    posptr = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    type = INTERP_SectorCeiling;
    posptr = ((vldoor_t *)th)->sector;
  }

  if(posptr)
  {
    for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
      if (curipos[i].Address == posptr)
        return;

    setinterpolation (type, posptr);
  }
}

void StopInterpolationIfNeeded(thinker_t *th)
{
  void *posptr = NULL;
  EInterpType type;

  if (!movement_smooth)
    return;

  if (th->function == T_MoveFloor)
  {
    type = INTERP_SectorFloor;
    posptr = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    type = INTERP_SectorFloor;
    posptr = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    type = INTERP_SectorCeiling;
    posptr = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    type = INTERP_SectorCeiling;
    posptr = ((vldoor_t *)th)->sector;
  }

  if(posptr)
  {
    stopinterpolation (type, posptr);
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

  ClearSmoothViewAngels();
}

void M_ChangeSpechitOverrun_Warn(void)
{
  ClearSmoothViewAngels();
}

void ClearSmoothViewAngels()
{
  if (demo_smoothturns && demoplayback)
  {
    if (players)
      demos_smoothangle = players[displayplayer].mo->angle;

    memset(demos_lastturns, 0, sizeof(demos_lastturns[0]) * MAX_DEMOS_SMOOTHFACTOR);
    demos_lastturnssum = 0;
    demos_lastturnsindex = 0;
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

void e6y_AfterTeleporting(void)
{
  R_ResetViewInterpolation();
  ClearSmoothViewAngels();
}

float viewPitch;
boolean WasRenderedInTryRunTics;
boolean trasparentpresent;

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

void I_Warning(const char *message, ...)
{
  char msg[1024];
  va_list argptr;
  va_start(argptr,message);
#ifdef HAVE_VSNPRINTF
  vsnprintf(msg,sizeof(msg),message,argptr);
#else
  vsprintf(msg,message,argptr);
#endif
  va_end(argptr);
  fprintf(stdout,"%s\n",msg);
#ifdef _MSC_VER
  {
    extern HWND con_hWnd;
    Init_ConsoleWin();
    MessageBox(con_hWnd,msg,"PrBoom-Plus",MB_OK | MB_TASKMODAL | MB_TOPMOST);
    SwitchToGameWindow();
  }
#endif
}

char* GetFileName (char *path)
{
  char *src = path + strlen(path) - 1;
  
  while (src != path && src[-1] != ':'
         && *(src-1) != '\\'
         && *(src-1) != '/')
    src--;

  return src;
}


boolean StrToInt(char *s, long *l)
{
/*  long val;
  char *p = NULL;
  boolean b;
  strcpy(s, "0");
  val = strtol(s, &p, 0);
  if (val==0)
    b = (sscanf(s, " %d", l) == 1);
  else
    b = true;

  return b;
  */
  return (
    (sscanf(s, " 0x%x", l) == 1) ||
    (sscanf(s, " 0X%x", l) == 1) ||
    (sscanf(s, " 0%o", l) == 1) ||
    (sscanf(s, " %d", l) == 1)
  );
}

void SwitchToGameWindow()
{
#ifdef _WIN32
  HWND hwnd = GetHWND();

  if (hwnd)
  {
    SetForegroundWindow(hwnd);

    {
      typedef BOOL (WINAPI *TSwitchToThisWindow) (HWND wnd, BOOL restore);
      static TSwitchToThisWindow SwitchToThisWindow = NULL;
      Sleep(100);
      
      if (!SwitchToThisWindow)
        SwitchToThisWindow = (TSwitchToThisWindow)GetProcAddress(GetModuleHandle("user32.dll"), "SwitchToThisWindow");
      
      if (SwitchToThisWindow)
        SwitchToThisWindow(hwnd, TRUE);
    }
  }
#endif
}

void ShowSpechitsOverrunningWarning(const char *msg)
{
  static char buffer[1024];
  static boolean OverrunPromted = false;
  extern line_t **spechit;

  if (!OverrunPromted)
  {
    OverrunPromted = true;

    sprintf(buffer,
      "%s The demo can be desync %s. "
#ifdef GL_DOOM
      "The list of LinesID leading to overrun: "
      "%d, %d, %d, %d, %d, %d, %d, %d, %d. "
#endif
      "You can disable this warning through: "
      "\\Options\\Setup\\Status Bar / HUD\\Warn on Spechits Overrun"
      ,msg, demoplayback?"soon":"on playback with vanilla doom2 engine"
#ifdef GL_DOOM
      ,spechit[0]->iLineID, spechit[1]->iLineID, spechit[2]->iLineID
      ,spechit[3]->iLineID, spechit[4]->iLineID, spechit[5]->iLineID
      ,spechit[6]->iLineID, spechit[7]->iLineID, spechit[8]->iLineID
#endif
      );
    I_Warning(buffer);
  }
}

void CheckForSpechitsOverrun(line_t* ld)
{
  extern int numspechit;
  extern fixed_t tmbbox[4];
  extern boolean crushchange, nofit;
  extern mobj_t *bombsource, *bombspot;
  extern int bombdamage;
  extern mobj_t* usething;
  extern int la_damage;
  extern fixed_t attackrange;

  if (numspechit>8 && demo_compatibility
    && (misc_spechitoverrun_warn || misc_spechitoverrun_emulate))
  {
    if (misc_spechitoverrun_warn)
      ShowSpechitsOverrunningWarning("Spechits overflow has been detected.");

    if (misc_spechitoverrun_emulate)
    {
      int addr = 0x01C09C98 + (ld - lines) * 0x3E;
    
      switch(numspechit)
      {
      case 9: 
      case 10:
      case 11:
      case 12:
        tmbbox[numspechit-9] = addr;
        break;
      case 13: crushchange = addr; break;
      case 14: nofit = addr; break;
      case 15: bombsource = (mobj_t*)addr; break;
      case 16: bombdamage = addr; break;
      case 17: bombspot = (mobj_t*)addr; break;
      case 18: usething = (mobj_t*)addr; break;
      case 19: attackrange = addr; break;
      case 20: la_damage = addr; break;

      default:
        ShowSpechitsOverrunningWarning("Too big spechits overflow for emulation was detected.");
        break;
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
    strcpy(str, "%%s - %%%dd:%%05.2f (%%%dd:%%02d)  K: %%%dd/%%-%dd%%%ds  I: %%%dd/%%-%dd%%%ds  S: %%%dd/%%-%dd %%%ds\n");

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
  RECT rect;
  HWND hwnd = GetHWND();
	
  GetWindowRect (hwnd, &rect);

  MousePrevX = (rect.left + rect.right) >> 1;
  MousePrevY = (rect.top + rect.bottom) >> 1;

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
