// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: D_main.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//  DOOM main program (D_DoomMain) and game loop, plus functions to
//  determine game mode (shareware, registered), parse command line
//  parameters, configure game parameters (turbo), and call the startup
//  functions.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: D_main.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $";

#ifdef _MSC_VER //proff
#include <direct.h>
#include <io.h>
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#define S_ISDIR(x) (((sbuf.st_mode & S_IFDIR)==S_IFDIR)?1:0)
#define FALSE false
#define TRUE true
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
// proff: The definition of mkdir is different in Visual C
#define mkdir(x,y) (mkdir)(x)
#else //_MSC_VER
#include <unistd.h>
// proff 11/01/98: This is for mingw32 and cygwin32
#if defined (__MINGW32__) || defined (__CYGWIN32__)
#define FALSE 0
#define TRUE !FALSE
#endif
// proff 07/04/98: Changed for compatibility to CYGWIN32
#if !defined(__CYGWIN32__)
#define mkdir(x,y) (mkdir)(x)
#endif
#endif //_MSC_VER
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef GL_DOOM
#include "gl_struct.h"
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"
#include "sounds.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_main.h"
#include "d_main.h"
#include "d_deh.h"  // Ty 04/08/98 - Externalizations
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

// DEHacked support - Ty 03/09/97
// killough 10/98:
// Add lump number as third argument, for use when filename==NULL
void ProcessDehFile(char *filename, char *outfilename, int lump);

// killough 10/98: support -dehout filename
static char *D_dehout(void)
{
  static char *s;      // cache results over multiple calls
  if (!s)
    {
      int p = M_CheckParm("-dehout");
      if (!p)
        p = M_CheckParm("-bexout");
      s = p && ++p < myargc ? myargv[p] : "-";
    }
  return s;
}

void GetFirstMap(int *ep, int *map); // Ty 08/29/98 - add "-warp x" functionality

char **wadfiles;
int *wadfilesource;  // Ty 08/29/98 - add source of lumps/files

// killough 10/98: preloaded files
#define MAXLOADFILES 2
char *wad_files[MAXLOADFILES], *deh_files[MAXLOADFILES];

H_boolean devparm;        // started game with -devparm

// jff 1/24/98 add new versions of these variables to remember command line
H_boolean clnomonsters;   // checkparm of -nomonsters
H_boolean clrespawnparm;  // checkparm of -respawn
H_boolean clfastparm;     // checkparm of -fast
// jff 1/24/98 end definition of command line version of play mode switches

H_boolean nomonsters;     // working -nomonsters
H_boolean respawnparm;    // working -respawn
H_boolean fastparm;       // working -fast

H_boolean singletics = false; // debug flag to cancel adaptiveness

//jff 1/22/98 parms for disabling music and sound
H_boolean nosfxparm;
H_boolean nomusicparm;

//jff 4/18/98
extern H_boolean inhelpscreens;

skill_t startskill;
int     startepisode;
int     startmap;
H_boolean autostart;
FILE    *debugfile;

H_boolean advancedemo;

extern H_boolean timingdemo, singledemo, demoplayback, fastdemo; // killough

char    wadfile[PATH_MAX+1];       // primary wad file
char    mapdir[PATH_MAX+1];        // directory of development maps
char    basedefault[PATH_MAX+1];   // default file
char    baseiwad[PATH_MAX+1];      // jff 3/23/98: iwad directory
char    basesavegame[PATH_MAX+1];  // killough 2/16/98: savegame directory

//jff 4/19/98 list of standard IWAD names
const char *const standard_iwads[]=
{
  "/doom2f.wad",
  "/doom2.wad",
  "/plutonia.wad",
  "/tnt.wad",
  "/doom.wad",
  "/doom1.wad",
};
static const int nstandard_iwads = sizeof standard_iwads/sizeof*standard_iwads;

void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//

event_t events[MAXEVENTS];
int eventhead, eventtail;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(event_t *ev)
{
  events[eventhead++] = *ev;
  eventhead &= MAXEVENTS-1;
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//

void D_ProcessEvents (void)
{
  // IF STORE DEMO, DO NOT ACCEPT INPUT
  if (gamemode != commercial || W_CheckNumForName("map01") >= 0)
    for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
      if (!M_Responder(events+eventtail))
        G_Responder(events+eventtail);
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t    wipegamestate = GS_DEMOSCREEN;
extern H_boolean setsizeneeded;
extern int     showMessages;
void           R_ExecuteSetViewSize(void);

void D_Display (void)
{
  static H_boolean viewactivestate = false;
  static H_boolean menuactivestate = false;
  static H_boolean inhelpscreensstate = false;
  static H_boolean fullscreen = false;
  static gamestate_t oldgamestate = -1;
  static int borderdrawcount;
  int wipestart;
  H_boolean done, wipe, redrawsbar;

  if (nodrawers)                    // for comparative timing / profiling
    return;

  redrawsbar = false;

  if (setsizeneeded)                // change the view size if needed
    {
      R_ExecuteSetViewSize();
      oldgamestate = -1;            // force background redraw
      borderdrawcount = 3;
    }

#ifdef GL_DOOM
  gld_Set2DMode();
  // proff 11/99: don't wipe in OpenGL
  wipe=false;
#else
  // save the current screen if about to wipe
  if ((wipe = gamestate != wipegamestate))
    wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
#endif

  if (gamestate == GS_LEVEL && gametic)
    HU_Erase();

  switch (gamestate)                // do buffered drawing
    {
    case GS_LEVEL:
      if (!gametic)
        break;
      if (automapactive)
        AM_Drawer();
// proff 08/18/98: Changed for high-res
      if (wipe || (viewheight != SCREENHEIGHT && fullscreen)
          || (inhelpscreensstate && !inhelpscreens))
	    { 
		    redrawsbar = true;  
		    R_DrawViewBorder(); // proff/nicolas 09/09/98 -- force redraw pattern
	    }                     // around status bar for high-res mode
      ST_Drawer(viewheight == SCREENHEIGHT, redrawsbar );
      fullscreen = viewheight == SCREENHEIGHT;
//      if (wipe || (viewheight != 200 && fullscreen)
//          || (inhelpscreensstate && !inhelpscreens))
//        redrawsbar = true;              // just put away the help screen
//      ST_Drawer(viewheight == 200, redrawsbar );
//      fullscreen = viewheight == 200;
      break;
    case GS_INTERMISSION:
      WI_Drawer();
      break;
    case GS_FINALE:
      F_Drawer();
      break;
    case GS_DEMOSCREEN:
      D_PageDrawer();
      break;
    }

  // draw buffered stuff to screen
  I_UpdateNoBlit();

  // draw the view directly
  if (gamestate == GS_LEVEL && !automapactive && gametic)
    R_RenderPlayerView (&players[displayplayer]);

  if (gamestate == GS_LEVEL && gametic)
    HU_Drawer ();

    // clean up border stuff
  if (gamestate != oldgamestate && gamestate != GS_LEVEL)
    I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));

  // see if the border needs to be initially drawn
  if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
      viewactivestate = false;        // view was not active
      R_FillBackScreen ();    // draw the pattern into the back screen
	    // proff/nicolas 09/04/98 -- make sure that pattern
	    // around the statusbar is drawn in high-res mode at
	    // startup when statusbar width is shorter then SCREENWIDTH
      R_DrawViewBorder();
    }

  // see if the border needs to be updated to the screen
// proff 08/17/98: Changed for high-res
  if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != SCREENWIDTH)
//  if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320)
    {
      if (menuactive || menuactivestate || !viewactivestate)
        borderdrawcount = 3;
      if (borderdrawcount)
        {
          R_DrawViewBorder ();    // erase old menu stuff
          borderdrawcount--;
        }
    }

  menuactivestate = menuactive;
  viewactivestate = viewactive;
  inhelpscreensstate = inhelpscreens;
  oldgamestate = wipegamestate = gamestate;

  // draw pause pic
  if (paused)
    {
      int y = 4;
      if (!automapactive)
        y += viewwindowy;
      // proff 11/27/98: Removed bug
      V_DrawPatchStretchedFromName((320-68)/2,(y*200)/SCREENHEIGHT,0,"M_PAUSE");
    }

  // menus go directly to the screen
  M_Drawer();          // menu is drawn even on top of everything
  NetUpdate();         // send out any new accumulation

  // normal update
  if (!wipe)
    {
      I_FinishUpdate ();              // page flip or blit buffer
      return;
    }

#ifndef GL_DOOM
// proff 11/99: not needed in OpenGL

  // wipe update
  wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

  wipestart = I_GetTime () - 1;

  do
    {
      int nowtime, tics;
      do
        {
          nowtime = I_GetTime();
          tics = nowtime - wipestart;
        }
      while (!tics);
      wipestart = nowtime;
      done = wipe_ScreenWipe(wipe_Melt,0,0,SCREENWIDTH,SCREENHEIGHT,tics);
      I_UpdateNoBlit();
      M_Drawer();                   // menu is drawn even on top of wipes
      I_FinishUpdate();             // page flip or blit buffer
    }
  while (!done);
#endif // GL_DOOM
}

//
//  DEMO LOOP
//

static int  demosequence;         // killough 5/2/98: made static
static int  pagetic;
static char *pagename;

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
  // killough 12/98: don't advance internal demos if a single one is 
  // being played. The only time this matters is when using -loadgame with
  // -fastdemo, -playdemo, or -timedemo, and a consistency error occurs.

  if (!singledemo && --pagetic < 0)
    D_AdvanceDemo();
}

//
// D_PageDrawer
//
// killough 11/98: add credits screen
//

void D_PageDrawer(void)
{
  if (pagename)
  {
    V_DrawPatchStretchedFromName(0, 0, 0, pagename);
  }
  else
    M_DrawCredits();
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
  advancedemo = true;
}

// killough 11/98: functions to perform demo sequences

static void D_SetPageName(char *name)
{
  pagename = name;
}

static void D_DrawTitle1(char *name)
{
  S_StartMusic(mus_intro);
  pagetic = (TICRATE*170)/35;
  D_SetPageName(name);
}

static void D_DrawTitle2(char *name)
{
  S_StartMusic(mus_dm2ttl);
  D_SetPageName(name);
}

// killough 11/98: tabulate demo sequences

static struct 
{
  void (*func)(char *);
  char *name;
} const demostates[][4] =
  {
    {
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle2, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
    },

    {
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
    },

    {
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
    },

    {
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "CREDIT"},
      {D_DrawTitle1,  "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {D_SetPageName, "CREDIT"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {G_DeferedPlayDemo, "demo4"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {NULL},
    }
  };

//
// This cycles through the demo sequences.
//
// killough 11/98: made table-driven

void D_DoAdvanceDemo(void)
{
  players[consoleplayer].playerstate = PST_LIVE;  // not reborn
  advancedemo = usergame = paused = false;
  gameaction = ga_nothing;

  pagetic = TICRATE * 11;         // killough 11/98: default behavior
  gamestate = GS_DEMOSCREEN;

  if (!demostates[++demosequence][gamemode].func)
    demosequence = 0;
  demostates[demosequence][gamemode].func
    (demostates[demosequence][gamemode].name);
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo();
}

// print title for every printed line
#ifdef _WIN32 // proff: I need this for the windowtitle
char title[128];
#else //_WIN32
static char title[128];
#endif //_WIN32

//
// D_AddFile
//
// Rewritten by Lee Killough
//
// Ty 08/29/98 - add source parm to indicate where this came from
// killough 11/98: remove limit on number of files
//

void D_AddFile (char *file, int source)
{
  static int numwadfiles, numwadfiles_alloc;

  if (numwadfiles >= numwadfiles_alloc)
  {
    wadfiles = realloc(wadfiles, (numwadfiles_alloc = numwadfiles_alloc ?
                                  numwadfiles_alloc * 2 : 8)*sizeof(*wadfiles));
    wadfilesource = realloc(wadfilesource, (numwadfiles_alloc = numwadfiles_alloc ?
                                  numwadfiles_alloc * 2 : 8)*sizeof(int));
  }
  wadfiles[numwadfiles] = !file ? NULL : strdup(file);
  wadfilesource[numwadfiles++] = source;
}

// Return the path where the executable lies -- Lee Killough
char *D_DoomExeDir(void)
{
  static char *base;
  if (!base)        // cache multiple requests
    {
      size_t len = strlen(*myargv);
      char *p = (base = malloc(len+1)) + len;
      strcpy(base,*myargv);
      while (p > base && *p!='/' && *p!='\\')
        *p--=0;
    }
  return base;
}

// killough 10/98: return the name of the program the exe was invoked as
char *D_DoomExeName(void)
{
  static char *name;    // cache multiple requests
  if (!name)
    {
      char *p = *myargv + strlen(*myargv);
      int i = 0;
      while (p > *myargv && p[-1] != '/' && p[-1] != '\\' && p[-1] != ':')
        p--;
      while (p[i] && p[i] != '.')
        i++;
      strncpy(name = malloc(i+1), p, i)[i] = 0;
    }
  return name;
}

//
// CheckIWAD
//
// Verify a file is indeed tagged as an IWAD
// Scan its lumps for levelnames and return gamemode as indicated
// Detect missing wolf levels in DOOM II
//
// The filename to check is passed in iwadname, the gamemode detected is
// returned in gmode, hassec returns the presence of secret levels
//
// jff 4/19/98 Add routine to test IWAD for validity and determine
// the gamemode from it. Also note if DOOM II, whether secret levels exist
//
// killough 11/98:
// Rewritten to considerably simplify
// Added Final Doom support (thanks to Joel Murdoch)
//

static void CheckIWAD(const char *iwadname,
                      GameMode_t *gmode,
                      GameMission_t *gmission,  // joel 10/17/98 Final DOOM fix
                      H_boolean *hassec)
{
  FILE *fp = fopen(iwadname, "rb");
  int ud, rg, sw, cm, sc, tnt, plut;
  filelump_t lump;
  wadinfo_t header;
  const char *n = lump.name;

  if (!fp)
    I_Error("Can't open IWAD: %s\n",iwadname);

  // read IWAD header
  if (fread(&header, 1, sizeof header, fp) != sizeof header ||
      header.identification[0] != 'I' || header.identification[1] != 'W' ||
      header.identification[2] != 'A' || header.identification[3] != 'D')
    I_Error("IWAD tag not present: %s\n",iwadname);

  fseek(fp, LONG(header.infotableofs), SEEK_SET);

  // Determine game mode from levels present
  // Must be a full set for whichever mode is present
  // Lack of wolf-3d levels also detected here

  for (ud=rg=sw=cm=sc=tnt=plut=0, header.numlumps = LONG(header.numlumps);
       header.numlumps && fread(&lump, sizeof lump, 1, fp); header.numlumps--)
    *n=='E' && n[2]=='M' && !n[4] ?
      n[1]=='4' ? ++ud : n[1]!='1' ? rg += n[1]=='3' || n[1]=='2' : ++sw :
    *n=='M' && n[1]=='A' && n[2]=='P' && !n[5] ?
      ++cm, sc += n[3]=='3' && (n[4]=='1' || n[4]=='2') :
    *n=='C' && n[1]=='A' && n[2]=='V' && !n[7] ? ++tnt :
    *n=='M' && n[1]=='C' && !n[3] && ++plut;

  fclose(fp);

  *gmission = doom;
  *hassec = false;
  *gmode =
    cm >= 30 ? (*gmission = tnt >= 4 ? pack_tnt :
                plut >= 8 ? pack_plut : doom2,
                *hassec = sc >= 2, commercial) :
    ud >= 9 ? retail :
    rg >= 18 ? registered :
    sw >= 9 ? shareware :
    indetermined;
}

// jff 4/19/98 Add routine to check a pathname for existence as
// a file or directory. If neither append .wad and check if it
// exists as a file then. Else return non-existent.

H_boolean WadFileStatus(char *filename,H_boolean *isdir)
{
  struct stat sbuf;
  int i;

  *isdir = false;                 //default is directory to false
  if (!filename || !*filename)    //if path NULL or empty, doesn't exist
    return false;

  if (!stat(filename,&sbuf))      //check for existence
  {
    *isdir=S_ISDIR(sbuf.st_mode); //if it does, set whether a dir or not
    return true;                  //return does exist
  }

  i = strlen(filename);           //get length of path
  if (i>=4)
    if(!strnicmp(filename+i-4,".wad",4))
      return false;               //if already ends in .wad, not found

  strcat(filename,".wad");        //try it with .wad added
  if (!stat(filename,&sbuf))      //if it exists then
  {
    if (S_ISDIR(sbuf.st_mode))    //but is a dir, then say we didn't find it
      return false;
    return true;                  //otherwise return file found, w/ .wad added
  }
  filename[i]=0;                  //remove .wad
  return false;                   //and report doesn't exist
}

//
// FindIWADFIle
//
// Search in all the usual places until an IWAD is found.
//
// The global baseiwad contains either a full IWAD file specification
// or a directory to look for an IWAD in, or the name of the IWAD desired.
//
// The global standard_iwads lists the standard IWAD names
//
// The result of search is returned in baseiwad, or set blank if none found
//
// IWAD search algorithm:
//
// Set customiwad blank
// If -iwad present set baseiwad to normalized path from -iwad parameter
//  If baseiwad is an existing file, thats it
//  If baseiwad is an existing dir, try appending all standard iwads
//  If haven't found it, and no : or / is in baseiwad,
//   append .wad if missing and set customiwad to baseiwad
//
// Look in . for customiwad if set, else all standard iwads
//
// Look in DoomExeDir. for customiwad if set, else all standard iwads
//
// If $DOOMWADDIR is an existing file
//  If customiwad is not set, thats it
//  else replace filename with customiwad, if exists thats it
// If $DOOMWADDIR is existing dir, try customiwad if set, else standard iwads
//
// If $HOME is an existing file
//  If customiwad is not set, thats it
//  else replace filename with customiwad, if exists thats it
// If $HOME is an existing dir, try customiwad if set, else standard iwads
//
// IWAD not found
//
// jff 4/19/98 Add routine to search for a standard or custom IWAD in one
// of the standard places. Returns a blank string if not found.
//
// killough 11/98: simplified, removed error-prone cut-n-pasted code
//

char *FindIWADFile(void)
{
  static const char *envvars[] = {"DOOMWADDIR", "HOME"};
  static char iwad[PATH_MAX+1], customiwad[PATH_MAX+1];
  H_boolean isdir=false;
  int i,j;
  char *p;

  *iwad = 0;       // default return filename to empty
  *customiwad = 0; // customiwad is blank

  //jff 3/24/98 get -iwad parm if specified else use .
  if ((i = M_CheckParm("-iwad")) && i < myargc-1)
    {
      NormalizeSlashes(strcpy(baseiwad,myargv[i+1]));
      if (WadFileStatus(strcpy(iwad,baseiwad),&isdir))
        if (!isdir)
          return iwad;
        else
          for (i=0;i<nstandard_iwads;i++)
            {
              int n = strlen(iwad);
              strcat(iwad,standard_iwads[i]);
              if (WadFileStatus(iwad,&isdir) && !isdir)
                return iwad;
              iwad[n] = 0; // reset iwad length to former
            }
      else
        if (!strchr(iwad,':') && !strchr(iwad,'/'))
          AddDefaultExtension(strcat(strcpy(customiwad, "/"), iwad), ".wad");
    }

  for (j=0; j<2; j++)
    {
      strcpy(iwad, j ? D_DoomExeDir() : ".");
      NormalizeSlashes(iwad);
      lprintf(LO_INFO, "Looking in %s\n",iwad);   // killough 8/8/98
      if (*customiwad)
        {
          strcat(iwad,customiwad);
          if (WadFileStatus(iwad,&isdir) && !isdir)
            return iwad;
        }
      else
        for (i=0;i<nstandard_iwads;i++)
          {
            int n = strlen(iwad);
            strcat(iwad,standard_iwads[i]);
            if (WadFileStatus(iwad,&isdir) && !isdir)
              return iwad;
            iwad[n] = 0; // reset iwad length to former
          }
    }

  for (i=0; i<sizeof envvars/sizeof *envvars;i++)
    if ((p = getenv(envvars[i])))
      {
        NormalizeSlashes(strcpy(iwad,p));
        if (WadFileStatus(iwad,&isdir))
          if (!isdir)
            {
              if (!*customiwad)
                return lprintf(LO_INFO, "Looking for %s\n",iwad), iwad; // killough 8/8/98
              else
                if ((p = strrchr(iwad,'/')))
                  {
                    *p=0;
                    strcat(iwad,customiwad);
                    lprintf(LO_INFO, "Looking for %s\n",iwad);  // killough 8/8/98
                    if (WadFileStatus(iwad,&isdir) && !isdir)
                      return iwad;
                  }
            }
          else
            {
              lprintf(LO_INFO, "Looking in %s\n",iwad);  // killough 8/8/98
              if (*customiwad)
                {
                  if (WadFileStatus(strcat(iwad,customiwad),&isdir) && !isdir)
                    return iwad;
                }
              else
                for (i=0;i<nstandard_iwads;i++)
                  {
                    int n = strlen(iwad);
                    strcat(iwad,standard_iwads[i]);
                    if (WadFileStatus(iwad,&isdir) && !isdir)
                      return iwad;
                    iwad[n] = 0; // reset iwad length to former
                  }
            }
      }

  *iwad = 0;
  return iwad;
}

//
// IdentifyVersion
//
// Set the location of the defaults file and the savegame root
// Locate and validate an IWAD file
// Determine gamemode from the IWAD
// 
// supports IWADs with custom names. Also allows the -iwad parameter to
// specify which iwad is being searched for if several exist in one dir.
// The -iwad parm may specify:
//
// 1) a specific pathname, which must exist (.wad optional)
// 2) or a directory, which must contain a standard IWAD,
// 3) or a filename, which must be found in one of the standard places:
//   a) current dir,
//   b) exe dir
//   c) $DOOMWADDIR
//   d) or $HOME
//
// jff 4/19/98 rewritten to use a more advanced search algorithm

void IdentifyVersion (void)
{
  int         i;    //jff 3/24/98 index of args on commandline
  struct stat sbuf; //jff 3/24/98 used to test save path for existence
  char *iwad;

  // get config file from same directory as executable
  // killough 10/98
  sprintf(basedefault,"%s/%s.cfg", D_DoomExeDir(), D_DoomExeName());

  // set save path to -save parm or current dir

  strcpy(basesavegame,".");       //jff 3/27/98 default to current dir
  if ((i=M_CheckParm("-save")) && i<myargc-1) //jff 3/24/98 if -save present
  {
    if (!stat(myargv[i+1],&sbuf) && S_ISDIR(sbuf.st_mode)) // and is a dir
    {
      strcpy(basesavegame,myargv[i+1]);  //jff 3/24/98 use that for savegame
      NormalizeSlashes(basesavegame);    //jff 9/22/98 fix c:\ not working
    }
    else
      //jff 9/3/98 use logical output routine
      lprintf(LO_ERROR,"Error: -save path does not exist, using current dir\n");
  }

  // locate the IWAD and determine game mode from it

  iwad = FindIWADFile();

#if (defined(GL_DOOM) && defined(_DEBUG))
  // proff 11/99: used for debugging
  {
    FILE *f;
    f=fopen("levelinfo.txt","w");
    if (f)
    {
      fprintf(f,"%s\n",iwad);
      fclose(f);
    }
  }
#endif

  if (iwad && *iwad)
  {
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"IWAD found: %s\n",iwad); //jff 4/20/98 print only if found

    CheckIWAD(iwad,
              &gamemode,
              &gamemission,   // joel 10/16/98 gamemission added
              &haswolflevels);

    switch(gamemode)
    {
      case retail:
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"Ultimate DOOM version\n");
        break;
      case registered:
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"DOOM Registered version\n");
        break;
      case shareware:
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"DOOM Shareware version\n");
        break;
      case commercial:
          // joel 10/16/98 Final DOOM fix
          switch (gamemission)
            {
            case pack_tnt:
              lprintf(LO_CONFIRM,"Final DOOM: TNT - Evilution version\n");
              break;

            case pack_plut:
              lprintf(LO_CONFIRM,"Final DOOM: The Plutonia Experiment version\n");
              break;

            case doom2:
            default:

              i = strlen(iwad);
              if (i>=10 && !strnicmp(iwad+i-10,"doom2f.wad",10))
                {
                  language=french;
                  lprintf(LO_CONFIRM,"DOOM II version, French language\n");  // killough 8/8/98
                }
              else
                lprintf(LO_CONFIRM, haswolflevels ? "DOOM II version\n" :  // killough 10/98
                     "DOOM II version, german edition, no wolf levels\n");
              break;
            }
          // joel 10/16/88 end Final DOOM fix

      default:
        break;
    }
    if (gamemode == indetermined)
      //jff 9/3/98 use logical output routine
      lprintf(LO_WARN,"Unknown Game Version, may not work\n");
    D_AddFile(iwad,source_iwad);
  }
  else
    I_Error("IWAD not found\n");
}

// killough 5/3/98: old code removed

//
// Find a Response File
//

#define MAXARGVS 100

void FindResponseFile (void)
{
  int i;

  for (i = 1;i < myargc;i++)
    if (myargv[i][0] == '@')
      {
        FILE *handle;
        int  size;
        int  k;
        int  index;
        int  indexinfile;
        char *infile;
        char *file;
        char *moreargs[MAXARGVS];
        char *firstargv;
        // proff 12/5/98: Added for searching responsefile
        char fname[PATH_MAX+1];

        // proff 12/5/98: Added for searching responsefile
        strcpy(fname,&myargv[i][1]);
        AddDefaultExtension(fname,".rsp");
        // READ THE RESPONSE FILE INTO MEMORY
        handle = fopen (fname,"rb");
        // proff 12/5/98: Added for searching responsefile
        if (!handle)
        {
          strcat(strcpy(fname,D_DoomExeDir()),&myargv[i][1]);
          AddDefaultExtension(fname,".rsp");
          handle = fopen (fname,"rb");
        }
        if (!handle)
          {
            //jff 9/3/98 use logical output routine
            // proff 12/8/98: Simply removed the exit(1);
            lprintf(LO_FATAL,"\nNo such response file!\n");
            // exit(1);
          }
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"Found response file %s!\n",&myargv[i][1]);
        fseek(handle,0,SEEK_END);
        size = ftell(handle);
        // proff 12/8/98: Added check for empty rsp file
        if (size<=0)
        {
          lprintf(LO_FATAL,"\nResponse file empty!\n");
          *myargv[i]='\0';
          fclose(handle);
          return;
        }
        fseek(handle,0,SEEK_SET);
        file = malloc (size);
        fread(file,size,1,handle);
        fclose(handle);

        // KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
        for (index = 0,k = i+1; k < myargc; k++)
          moreargs[index++] = myargv[k];

        firstargv = myargv[0];
        myargv = calloc(sizeof(char *),MAXARGVS);
        myargv[0] = firstargv;

        infile = file;
        indexinfile = k = 0;
        indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
        do
          {
            myargv[indexinfile++] = infile+k;
            while(k < size &&
                  ((*(infile+k)>= ' '+1) && (*(infile+k)<='z')))
              k++;
            *(infile+k) = 0;
            while(k < size &&
                  ((*(infile+k)<= ' ') || (*(infile+k)>'z')))
              k++;
          }
        while(k < size);

        for (k = 0;k < index;k++)
          myargv[indexinfile++] = moreargs[k];
        myargc = indexinfile;

        // DISPLAY ARGS
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"%d command-line args:\n",myargc);
        for (k=1;k<myargc;k++)
          //jff 9/3/98 use logical output routine
          lprintf(LO_CONFIRM,"%s\n",myargv[k]);
        break;
      }
}

//
// DoLooseFiles
//
// Take any file names on the command line before the first switch parm
// and insert the appropriate -file, -deh or -playdemo switch in front
// of them.
//
// Note that more than one -file, etc. entry on the command line won't
// work, so we have to go get all the valid ones if any that show up
// after the loose ones.  This means that boom fred.wad -file wilma
// will still load fred.wad and wilma.wad, in that order.
// The response file code kludges up its own version of myargv[] and 
// unfortunately we have to do the same here because that kludge only
// happens if there _is_ a response file.  Truth is, it's more likely
// that there will be a need to do one or the other so it probably 
// isn't important.  We'll point off to the original argv[], or the 
// area allocated in FindResponseFile, or our own areas from strdups.
//

static void DoLooseFiles(void)
{
  char *wads[MAXARGVS];  // store the respective loose filenames
  char *lmps[MAXARGVS];
  char *dehs[MAXARGVS];
  int wadcount = 0;      // count the loose filenames
  int lmpcount = 0;
  int dehcount = 0;
  int i,j,p;
  char **tmyargv;  // use these to recreate the argv array
  int tmyargc;

  for (i=1;i<myargc;i++)
  {
    if (*myargv[i] == '-') break;  // quit at first switch 

    // so now we must have a loose file.  Find out what kind and store it.
    j = strlen(myargv[i]);
    // proff 12/8/98: Added check for argv which can be created with rsp files
    if (j<=0)
      continue;
    if (!stricmp(&myargv[i][j-4],".wad")) 
      wads[wadcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".lmp")) 
      lmps[lmpcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".deh")) 
      dehs[dehcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".bex")) 
      dehs[dehcount++] = strdup(myargv[i]);
    if (myargv[i][j-4] != '.')  // assume wad if no extension
      wads[wadcount++] = strdup(myargv[i]);
    *myargv[i] = '\0'; // nuke that entry so it won't repeat later
  }

  // Now, if we didn't find any loose files, we can just leave.
  if (wadcount+lmpcount+dehcount == 0) return;  // ******* early return ****

  if ((p = M_CheckParm ("-file")))
  {
    *myargv[p] = '\0';    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      wads[wadcount++] = strdup(myargv[p]);
      *myargv[p] = '\0';  // null any we find and save
    }
  }

  if ((p = M_CheckParm ("-deh")))
  {
    *myargv[p] = '\0';    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      dehs[dehcount++] = strdup(myargv[p]);
      *myargv[p] = '\0';  // null any we find and save
    }
  }

  if ((p = M_CheckParm ("-playdemo")))
  {
    *myargv[p] = '\0';    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      lmps[lmpcount++] = strdup(myargv[p]);
      *myargv[p] = '\0';  // null any we find and save
    }
  }

  // Now go back and redo the whole myargv array with our stuff in it.
  // First, create a new myargv array to copy into
  tmyargv = calloc(sizeof(char *),MAXARGVS);
  tmyargv[0] = myargv[0]; // invocation 
  tmyargc = 1;

  // put our stuff into it
  if (wadcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-file"); // put the switch in
    for (i=0;i<wadcount;)
      tmyargv[tmyargc++] = wads[i++]; // allocated by strdup above
  }

  // for -deh
  if (dehcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-deh"); 
    for (i=0;i<dehcount;)
      tmyargv[tmyargc++] = dehs[i++]; 
  }

  // for -playdemo
  if (lmpcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-playdemo"); 
    for (i=0;i<lmpcount;)
      tmyargv[tmyargc++] = lmps[i++]; 
  }

  // then copy everything that's there now
  for (i=1;i<myargc;i++)
  {
    if (*myargv[i])  // skip any null entries
      tmyargv[tmyargc++] = myargv[i];  // pointers are still valid
  }
  // now make the global variables point to our array
  myargv = tmyargv;
  myargc = tmyargc;
}

// killough 10/98: moved code to separate function

static void D_ProcessDehCommandLine(void)
{
  // ty 03/09/98 do dehacked stuff
  // Note: do this before any other since it is expected by
  // the deh patch author that this is actually part of the EXE itself
  // Using -deh in BOOM, others use -dehacked.
  // Ty 03/18/98 also allow .bex extension.  .bex overrides if both exist.
  // killough 11/98: also allow -bex

  int p = M_CheckParm ("-deh");
  if (p || (p = M_CheckParm("-bex")))
    {
      // the parms after p are deh/bex file names,
      // until end of parms or another - preceded parm
      // Ty 04/11/98 - Allow multiple -deh files in a row
      // killough 11/98: allow multiple -deh parameters

      H_boolean deh = true;
      while (++p < myargc)
        if (*myargv[p] == '-')
          deh = !strcasecmp(myargv[p],"-deh") || !strcasecmp(myargv[p],"-bex");
        else
          if (deh)
            {
              char file[PATH_MAX+1];      // killough
              AddDefaultExtension(strcpy(file, myargv[p]), ".bex");
              if (access(file, F_OK))  // nope
                {
                  AddDefaultExtension(strcpy(file, myargv[p]), ".deh");
                  if (access(file, F_OK))  // still nope
                    I_Error("Cannot find .deh or .bex file named %s",
                            myargv[p]);
                }
              // during the beta we have debug output to dehout.txt
              // (apparently, this was never removed after Boom beta-killough)
              ProcessDehFile(file, D_dehout(), 0);  // killough 10/98
            }
    }
  // ty 03/09/98 end of do dehacked stuff
}

// killough 10/98: support preloaded wads

static void D_ProcessWadPreincludes(void)
{
  if (!M_CheckParm ("-noload"))
    {
      int i;
      char *s;
      for (i=0; i<MAXLOADFILES; i++)
        if ((s=wad_files[i]))
          {
            while (isspace(*s))
              s++;
            if (*s)
              {
                char file[PATH_MAX+1];
                AddDefaultExtension(strcpy(file, s), ".wad");
                if (!access(file, R_OK))
                  D_AddFile(file,source_lmp);
                else
                  lprintf(LO_WARN, "\nWarning: could not open %s\n", file);
              }
          }
    }
}

// killough 10/98: support preloaded deh/bex files

static void D_ProcessDehPreincludes(void)
{
  if (!M_CheckParm ("-noload"))
    {
      int i;
      char *s;
      for (i=0; i<MAXLOADFILES; i++)
        if ((s=deh_files[i]))
          {
            while (isspace(*s))
              s++;
            if (*s)
              {
                char file[PATH_MAX+1];
                AddDefaultExtension(strcpy(file, s), ".bex");
                if (!access(file, R_OK))
                  ProcessDehFile(file, D_dehout(), 0);
                else
                  {
                    AddDefaultExtension(strcpy(file, s), ".deh");
                    if (!access(file, R_OK))
                      ProcessDehFile(file, D_dehout(), 0);
                    else
                      lprintf(LO_WARN, "\nWarning: could not open %s .deh or .bex\n", s);
                  }
              }
          }
    }
}

// killough 10/98: support .deh from wads
//
// A lump named DEHACKED is treated as plaintext of a .deh file embedded in
// a wad (more portable than reading/writing info.c data directly in a wad).
//
// If there are multiple instances of "DEHACKED", we process each, in first
// to last order (we must reverse the order since they will be stored in
// last to first order in the chain). Passing NULL as first argument to
// ProcessDehFile() indicates that the data comes from the lump number
// indicated by the third argument, instead of from a file.

static void D_ProcessDehInWad(int i)
{
  if (i >= 0)
    {
      D_ProcessDehInWad(lumpinfo[i].next);
      if (!strncasecmp(lumpinfo[i].name, "dehacked", 8) &&
          lumpinfo[i].namespace == ns_global)
        ProcessDehFile(NULL, D_dehout(), i);
    }
}

#define D_ProcessDehInWads() D_ProcessDehInWad(lumpinfo[W_LumpNameHash \
                                                       ("dehacked") % (unsigned) numlumps].index);

//
// D_DoomMain
//

void D_DoomMain(void)
{
  int p, i, slot;
  char file[PATH_MAX+1];      // killough 3/22/98
  char *cena="ICWEFDA",*pos;  //jff 9/3/98 use this for parsing console masks

  //jff 9/3/98 get mask for console output filter
  if ((p = M_CheckParm ("-cout")))
    if (++p != myargc && *myargv[p] != '-')
      for (i=0,cons_output_mask=0;i<strlen(myargv[p]);i++)
        if ((pos = strchr(cena,toupper(myargv[p][i]))))
          cons_output_mask |= (1<<(pos-cena));

  //jff 9/3/98 get mask for redirected console error filter
  if ((p = M_CheckParm ("-cerr")))
    if (++p != myargc && *myargv[p] != '-')
      for (i=0,cons_error_mask=0;i<strlen(myargv[p]);i++)
        if ((pos = strchr(cena,toupper(myargv[p][i]))))
          cons_error_mask |= (1<<(pos-cena));

  setbuf(stdout,NULL);

  FindResponseFile();
  DoLooseFiles();  // Ty 08/29/98 - handle "loose" files on command line

  // killough 10/98: set default savename based on executable's name
  sprintf(savegamename = malloc(16), "%.4ssav", D_DoomExeName());

  IdentifyVersion();

  modifiedgame = false;

  // killough 10/98: process all command-line DEH's first
  D_ProcessDehCommandLine();

  // jff 1/24/98 set both working and command line value of play parms
  nomonsters = clnomonsters = M_CheckParm ("-nomonsters");
  respawnparm = clrespawnparm = M_CheckParm ("-respawn");
  fastparm = clfastparm = M_CheckParm ("-fast");
  // jff 1/24/98 end of set to both working and command line value

  devparm = M_CheckParm ("-devparm");

  if (M_CheckParm ("-altdeath"))
    deathmatch = 2;
  else
    if (M_CheckParm ("-deathmatch"))
      deathmatch = 1;

  switch ( gamemode )
    {
    case retail:
      sprintf (title,
               "                         "
               "The Ultimate DOOM Startup v%i.%02i"
               "                           ",
               VERSION/100,VERSION%100);
      break;
    case shareware:
      sprintf (title,
               "                            "
               "DOOM Shareware Startup v%i.%02i"
               "                           ",
               VERSION/100,VERSION%100);
      break;
    case registered:
      sprintf (title,
               "                            "
               "DOOM Registered Startup v%i.%02i"
               "                           ",
               VERSION/100,VERSION%100);
      break;
    case commercial:
      switch (gamemission)      // joel 10/16/98 Final DOOM fix
      {
        case pack_plut:
          sprintf (title,
                   "                   "
                   "DOOM 2: Plutonia Experiment v%i.%02i"
                   "                           ",
                   VERSION/100,VERSION%100);
          break;
        case pack_tnt:
          sprintf (title,
                   "                     "
                   "DOOM 2: TNT - Evilution v%i.%02i"
                   "                           ",
                   VERSION/100,VERSION%100);
          break;
        case doom2:
        default:
          sprintf (title,
                   "                         "
                   "DOOM 2: Hell on Earth v%i.%02i"
                   "                           ",
                   VERSION/100,VERSION%100);
          break;
      }
      break;
      // joel 10/16/98 end Final DOOM fix

    default:
      sprintf (title,
               "                     "
               "Public DOOM - v%i.%i"
               "                           ",
               VERSION/100,VERSION%100);
      break;
    }

  //jff 9/3/98 use logical output routine
  lprintf (LO_ALWAYS,"%s\nBuilt on %s\n", title, version_date);    // killough 2/1/98

  if (devparm)
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,D_DEVSTR);

  if (M_CheckParm("-cdrom"))
    {
      //jff 9/3/98 use logical output routine
      lprintf(LO_CONFIRM,D_CDROM);
      mkdir("c:/doomdata",0);
      // killough 10/98:
      sprintf(basedefault, "c:/doomdata/%s.cfg", D_DoomExeName());
    }

  // turbo option
  if ((p=M_CheckParm ("-turbo")))
    {
      int scale = 200;
      extern int forwardmove[2];
      extern int sidemove[2];

      if (p<myargc-1)
        scale = atoi(myargv[p+1]);
      if (scale < 10)
        scale = 10;
      if (scale > 400)
        scale = 400;
      //jff 9/3/98 use logical output routine
      lprintf (LO_CONFIRM,"turbo scale: %i%%\n",scale);
      forwardmove[0] = forwardmove[0]*scale/100;
      forwardmove[1] = forwardmove[1]*scale/100;
      sidemove[0] = sidemove[0]*scale/100;
      sidemove[1] = sidemove[1]*scale/100;
    }

#ifdef GL_DOOM
  // nicolas 11/99: for GL-specific switches
 	gld_InitCommandLine();
#endif

  // add any files specified on the command line with -file wadfile
  // to the wad list

  // killough 1/31/98, 5/2/98: reload hack removed, -wart same as -warp now.

  if ((p = M_CheckParm ("-file")))
    {
      // the parms after p are wadfile/lump names,
      // until end of parms or another - preceded parm
      // killough 11/98: allow multiple -file parameters

      H_boolean file = modifiedgame = true;            // homebrew levels
      while (++p < myargc)
        if (*myargv[p] == '-')
          file = !strcasecmp(myargv[p],"-file");
        else
          if (file)
            D_AddFile(myargv[p], source_pwad);
    }

  if (!(p = M_CheckParm("-playdemo")) || p >= myargc-1)    // killough
    if ((p = M_CheckParm ("-fastdemo")) && p < myargc-1)   // killough
      fastdemo = true;             // run at fastest speed possible
    else
      p = M_CheckParm ("-timedemo");

  if (p && p < myargc-1)
    {
      strcpy(file,myargv[p+1]);
      AddDefaultExtension(file,".lmp");     // killough
      D_AddFile (file,source_lmp);
      //jff 9/3/98 use logical output routine
      lprintf(LO_CONFIRM,"Playing demo %s\n",file);
    }

  // get skill / episode / map from parms

  startskill = sk_none; // jff 3/24/98 was sk_medium, just note not picked
  startepisode = 1;
  startmap = 1;
  autostart = false;

  if ((p = M_CheckParm ("-skill")) && p < myargc-1)
    {
      startskill = myargv[p+1][0]-'1';
      autostart = true;
    }

  if ((p = M_CheckParm ("-episode")) && p < myargc-1)
    {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
    }

  if ((p = M_CheckParm ("-timer")) && p < myargc-1 && deathmatch)
    {
      int time = atoi(myargv[p+1]);
      //jff 9/3/98 use logical output routine
      lprintf(LO_CONFIRM,"Levels will end after %d minute%s.\n", time, time>1 ? "s" : "");
    }

  if ((p = M_CheckParm ("-avg")) && p < myargc-1 && deathmatch)
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"Austin Virtual Gaming: Levels will end after 20 minutes\n");

  if ((p = M_CheckParm ("-warp")) ||      // killough 5/2/98
       (p = M_CheckParm ("-wart"))) 
       // Ty 08/29/98 - moved this check later so we can have -warp alone: && p < myargc-1)
  {
    startmap = 0; // Ty 08/29/98 - allow "-warp x" to go to first map in wad(s)
    autostart = true; // Ty 08/29/98 - move outside the decision tree
    if (gamemode == commercial)
    {
      if (p < myargc-1) 
        startmap = atoi(myargv[p+1]);   // Ty 08/29/98 - add test if last parm
    }
    else    // 1/25/98 killough: fix -warp xxx from crashing Doom 1 / UD
    {
      if (p < myargc-2)
      {
        startepisode = atoi(myargv[++p]);
        startmap = atoi(myargv[p+1]);
      }
    }
  }
  // Ty 08/29/98 - later we'll check for startmap=0 and autostart=true
  // as a special case that -warp * was used.  Actually -warp with any
  // non-numeric will do that but we'll only document "*"

  //jff 1/22/98 add command line parms to disable sound and music
  {
    int nosound = M_CheckParm("-nosound");
    nomusicparm = nosound || M_CheckParm("-nomusic");
    nosfxparm   = nosound || M_CheckParm("-nosfx");
  }
  //jff end of sound/music command line parms

  // killough 3/2/98: allow -nodraw -noblit generally
  nodrawers = M_CheckParm ("-nodraw");
  noblit = M_CheckParm ("-noblit");

  // jff 4/21/98 allow writing predefined lumps out as a wad
  if ((p = M_CheckParm("-dumplumps")) && p < myargc-1)
    WritePredefinedLumpWad(myargv[p+1]);

  //proff 11/22/98: Added setting of viewangleoffset
  p = M_CheckParm("-viewangle");
  if (p)
  {
    viewangleoffset = atoi(myargv[p+1]);
    viewangleoffset = viewangleoffset<0 ? 0 : (viewangleoffset>7 ? 7 : viewangleoffset);
    viewangleoffset = (8-viewangleoffset) * ANG45;
  }

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"M_LoadDefaults: Load system defaults.\n");
  M_LoadDefaults();              // load before initing other systems

// proff 12/2/98: Moved from WINSTUFF.C for high-res
    if (i=M_CheckParm("-width"))
    {
      SCREENWIDTH = atoi (myargv[i+1]);
      SCREENWIDTH >>= 4;
      SCREENWIDTH <<= 4;
    }
    if (i=M_CheckParm("-height"))
      SCREENHEIGHT = atoi (myargv[i+1]);
    if (SCREENWIDTH<320)
      SCREENWIDTH=320;
    if (SCREENWIDTH>MAX_SCREENWIDTH)
      SCREENWIDTH=MAX_SCREENWIDTH;
    if (SCREENHEIGHT<200)
      SCREENHEIGHT=200;
    if (SCREENHEIGHT>MAX_SCREENHEIGHT)
      SCREENHEIGHT=MAX_SCREENHEIGHT;

  bodyquesize = default_bodyquesize; // killough 10/98
#ifndef _WIN32
  snd_card = default_snd_card;
  mus_card = default_mus_card;
#endif

  // init subsystems
  //proff 12/2/98: Moved this down to allow SCREENWIDTH and SCREENHEIGHT to be in the cfg
  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"V_Init: allocate screens.\n");
  V_Init();

  G_ReloadDefaults();    // killough 3/4/98: set defaults just loaded.
  // jff 3/24/98 this sets startskill if it was -1

  // 1/18/98 killough: Z_Init() call moved to i_main.c

  D_ProcessWadPreincludes(); // killough 10/98: add preincluded wads at the end

  D_AddFile(NULL, -1);

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"W_Init: Init WADfiles.\n");
  W_InitMultipleFiles(wadfiles,wadfilesource);

  lprintf(LO_INFO,"\n");     // killough 3/6/98: add a newline, by popular demand :)

  D_ProcessDehInWads();      // killough 10/98: now process all deh in wads

  D_ProcessDehPreincludes(); // killough 10/98: process preincluded .deh files

  // Check for -file in shareware
  if (modifiedgame)
    {
      // These are the lumps that will be checked in IWAD,
      // if any one is not present, execution will be aborted.
      static const char name[23][8]= {
        "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
        "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
        "dphoof","bfgga0","heada1","cybra1","spida1d1" };
      int i;

      if (gamemode == shareware)
        I_Error("\nYou cannot -file with the shareware version. Register!");

      // Check for fake IWAD with right name,
      // but w/o all the lumps of the registered version.
      if (gamemode == registered)
        for (i = 0;i < 23; i++)
          if (W_CheckNumForName(name[i])<0 &&
              (W_CheckNumForName)(name[i],ns_sprites)<0) // killough 4/18/98
            I_Error("\nThis is not the registered version.");
    }

  V_InitColorTranslation(); //jff 4/24/98 load color translation lumps

  // killough 2/22/98: copyright / "modified game" / SPA banners removed

  // Ty 04/08/98 - Add 5 lines of misc. data, only if nonblank
  // The expectation is that these will be set in a .bex file
  //jff 9/3/98 use logical output routine
  if (*startup1) lprintf(LO_INFO,startup1);
  if (*startup2) lprintf(LO_INFO,startup2);
  if (*startup3) lprintf(LO_INFO,startup3);
  if (*startup4) lprintf(LO_INFO,startup4);
  if (*startup5) lprintf(LO_INFO,startup5);
  // End new startup strings

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"M_Init: Init miscellaneous info.\n");
  M_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"R_Init: Init DOOM refresh daemon - ");
  R_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"\nP_Init: Init Playloop state.\n");
  P_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"I_Init: Setting up machine state.\n");
  I_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"D_CheckNetGame: Checking network game status.\n");
  D_CheckNetGame();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"S_Init: Setting up sound.\n");
  S_Init(snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"HU_Init: Setting up heads up display.\n");
  HU_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"ST_Init: Init status bar.\n");
  ST_Init();

  idmusnum = -1; //jff 3/17/98 insure idmus number is blank

  // check for a driver that wants intermission stats
  if ((p = M_CheckParm ("-statcopy")) && p<myargc-1)
    {
      // for statistics driver
      extern  void* statcopy;

      // killough 5/2/98: this takes a memory
      // address as an integer on the command line!

      statcopy = (void*) atoi(myargv[p+1]);
      //jff 9/3/98 use logical output routine
      lprintf (LO_CONFIRM,"External statistics registered.\n");
    }

  // start the apropriate game based on parms

  // killough 12/98: 
  // Support -loadgame with -record and reimplement -recordfrom.

  if ((slot = M_CheckParm("-recordfrom")) && (p = slot+2) < myargc)
    G_RecordDemo(myargv[p]);
  else
  {
    slot = M_CheckParm("-loadgame");
    if ((p = M_CheckParm("-record")) && ++p < myargc)
  	{
      autostart = true;
      G_RecordDemo(myargv[p]);
    }
  }

  if ((p = M_CheckParm ("-fastdemo")) && ++p < myargc)
  {                                 // killough
    fastdemo = true;                // run at fastest speed possible
    timingdemo = true;              // show stats after quit
    G_DeferedPlayDemo(myargv[p]);
    singledemo = true;              // quit after one demo
  }
  else
    if ((p = M_CheckParm("-timedemo")) && ++p < myargc)
    {
    	singletics = true;
	    timingdemo = true;            // show stats after quit
	    G_DeferedPlayDemo(myargv[p]);
	    singledemo = true;            // quit after one demo
    }
    else
      if ((p = M_CheckParm("-playdemo")) && ++p < myargc)
    	{
    	  G_DeferedPlayDemo(myargv[p]);
    	  singledemo = true;          // quit after one demo
	    }

  if (slot && ++slot < myargc)
    {
      slot = atoi(myargv[slot]);        // killough 3/16/98: add slot info
      G_SaveGameName(file, slot);       // killough 3/22/98
      G_LoadGame(file, slot, true);     // killough 5/15/98: add command flag
    }
  else
    if (!singledemo)                    // killough 12/98
      if (autostart || netgame)
	    {
        GetFirstMap(&startepisode, &startmap);  // sets first map and first episode if unknown
	      G_InitNew(startskill, startepisode, startmap);
	      if (demorecording)
	        G_BeginRecording();
	    }
      else
  	    D_StartTitle();                 // start up intro loop

  // killough 12/98: inlined D_DoomLoop

  if (M_CheckParm ("-debugfile"))
    {
      char    filename[20];
      sprintf(filename,"debug%i.txt",consoleplayer);
      //jff 9/3/98 use logical output routine
      lprintf(LO_DEBUG,"debug output to: %s\n",filename);
      debugfile = fopen(filename,"w");
    }

  I_InitGraphics ();

  atexit(D_QuitNetGame);       // killough

  for (;;)
    {
      // frame syncronous IO operations
      I_StartFrame ();

      // process one or more tics
      if (singletics)
        {
          I_StartTic ();
          D_ProcessEvents ();
          G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
          if (advancedemo)
            D_DoAdvanceDemo ();
          M_Ticker ();
          G_Ticker ();
          gametic++;
          maketic++;
        }
      else
      {
        I_StartTic ();
        TryRunTics (); // will run at least one tic
      }

      // killough 3/16/98: change consoleplayer to displayplayer
      S_UpdateSounds(players[displayplayer].mo);// move positional sounds

      // Update display, next frame, with current state.
      D_Display();

#ifndef SNDSERV
      // Sound mixing for the buffer is snychronous.
      I_UpdateSound();
#endif

#ifndef SNDINTR
      // Synchronous sound output is explicitly called.
      // Update sound output.
      I_SubmitSound();
#endif
    }
}

//
// GetFirstMap
//
// Ty 08/29/98 - determine first available map from the loaded wads and run it
// 

void GetFirstMap(int *ep, int *map)
{
  int i,j; // used to generate map name
  H_boolean done = FALSE;  // Ty 09/13/98 - to exit inner loops
  char test[6];  // MAPxx or ExMx plus terminator for testing
  char name[6];  // MAPxx or ExMx plus terminator for display
  H_boolean newlevel = FALSE;  // Ty 10/04/98 - to test for new level
  int ix;  // index for lookup

  strcpy(name,""); // initialize
  if (*map == 0) // unknown so go search for first changed one
  {
    *ep = 1;
    *map = 1; // default E1M1 or MAP01
    if (gamemode == commercial)
    {
      for (i=1;!done && i<33;i++)  // Ty 09/13/98 - add use of !done
      {
        sprintf(test,"MAP%02d",i);
        ix = W_CheckNumForName(test);
        if (ix != -1)  // Ty 10/04/98 avoid -1 subscript
        {
          if (lumpinfo[ix].source == source_pwad)
          {
            *map = i;
            strcpy(name,test);  // Ty 10/04/98
            done = TRUE;  // Ty 09/13/98
            newlevel = TRUE; // Ty 10/04/98
          }
          else
          {
            if (!*name)  // found one, not pwad.  First default.
               strcpy(name,test);
          }
        }
      }
    }
    else // one of the others
    {
      strcpy(name,"E1M1");  // Ty 10/04/98 - default for display
      for (i=1;!done && i<5;i++)  // Ty 09/13/98 - add use of !done
      {
        for (j=1;!done && j<10;j++)  // Ty 09/13/98 - add use of !done
        {
          sprintf(test,"E%dM%d",i,j);
          ix = W_CheckNumForName(test);
          if (ix != -1)  // Ty 10/04/98 avoid -1 subscript
          {
            if (lumpinfo[ix].source == source_pwad)
            {
              *ep = i;
              *map = j;
              strcpy(name,test); // Ty 10/04/98
              done = TRUE;  // Ty 09/13/98
              newlevel = TRUE; // Ty 10/04/98
            }
            else
            {
              if (!*name)  // found one, not pwad.  First default.
                 strcpy(name,test);
            }
          }
        }
      }
    }   
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"Auto-warping to first %slevel: %s\n",
      newlevel ? "new " : "", name);  // Ty 10/04/98 - new level test
  }
}

//----------------------------------------------------------------------------
//
// $Log: D_main.c,v $
// Revision 1.2  2000/04/26 20:00:02  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.1.1.1  2000/04/09 18:05:20  proff_fs
// Initial login
//
// Revision 1.55  1998/11/20  23:15:49  phares
// New Fireline fix
//
// Revision 1.54  1998/10/04  13:20:32  thldrmn
// Fix autowarp message and subscript bug
//
// Revision 1.53  1998/09/24  19:02:41  jim
// Fixed -save parm for x:/
//
// Revision 1.52  1998/09/14  01:27:38  thldrmn
// Fixed autowarp message for DOOM1/UDOOM
//
// Revision 1.51  1998/09/07  20:18:09  jim
// Added logical output routine
//
// Revision 1.49  1998/08/29  22:57:45  thldrmn
// Updates to -warp and switchless filename handling
//
// Revision 1.48  1998/08/24  20:28:26  jim
// gamemission support in IdentifyVersion
//
// Revision 1.47  1998/05/16  09:16:51  killough
// Make loadgame checksum friendlier
//
// Revision 1.46  1998/05/12  10:32:42  jim
// remove LEESFIXES from d_main
//
// Revision 1.45  1998/05/06  15:15:46  jim
// Documented IWAD routines
//
// Revision 1.44  1998/05/03  22:26:31  killough
// beautification, declarations, headers
//
// Revision 1.43  1998/04/24  08:08:13  jim
// Make text translate tables lumps
//
// Revision 1.42  1998/04/21  23:46:01  jim
// Predefined lump dumper option
//
// Revision 1.39  1998/04/20  11:06:42  jim
// Fixed print of IWAD found
//
// Revision 1.37  1998/04/19  01:12:19  killough
// Fix registered check to work with new lump namespaces
//
// Revision 1.36  1998/04/16  18:12:50  jim
// Fixed leak
//
// Revision 1.35  1998/04/14  08:14:18  killough
// Remove obsolete adaptive_gametics code
//
// Revision 1.34  1998/04/12  22:54:41  phares
// Remaining 3 Setup screens
//
// Revision 1.33  1998/04/11  14:49:15  thldrmn
// Allow multiple deh/bex files
//
// Revision 1.32  1998/04/10  06:31:50  killough
// Add adaptive gametic timer
//
// Revision 1.31  1998/04/09  09:18:17  thldrmn
// Added generic startup strings for BEX use
//
// Revision 1.30  1998/04/06  04:52:29  killough
// Allow demo_insurance=2, fix fps regression wrt redrawsbar
//
// Revision 1.29  1998/03/31  01:08:11  phares
// Initial Setup screens and Extended HELP screens
//
// Revision 1.28  1998/03/28  15:49:37  jim
// Fixed merge glitches in d_main.c and g_game.c
//
// Revision 1.27  1998/03/27  21:26:16  jim
// Default save dir offically . now
//
// Revision 1.26  1998/03/25  18:14:21  jim
// Fixed duplicate IWAD search in .
//
// Revision 1.25  1998/03/24  16:16:00  jim
// Fixed looking for wads message
//
// Revision 1.23  1998/03/24  03:16:51  jim
// added -iwad and -save parms to command line
//
// Revision 1.22  1998/03/23  03:07:44  killough
// Use G_SaveGameName, fix some remaining default.cfg's
//
// Revision 1.21  1998/03/18  23:13:54  jim
// Deh text additions
//
// Revision 1.19  1998/03/16  12:27:44  killough
// Remember savegame slot when loading
//
// Revision 1.18  1998/03/10  07:14:58  jim
// Initial DEH support added, minus text
//
// Revision 1.17  1998/03/09  07:07:45  killough
// print newline after wad files
//
// Revision 1.16  1998/03/04  08:12:05  killough
// Correctly set defaults before recording demos
//
// Revision 1.15  1998/03/02  11:24:25  killough
// make -nodraw -noblit work generally, fix ENDOOM
//
// Revision 1.14  1998/02/23  04:13:55  killough
// My own fix for m_misc.c warning, plus lots more (Rand's can wait)
//
// Revision 1.11  1998/02/20  21:56:41  phares
// Preliminarey sprite translucency
//
// Revision 1.10  1998/02/20  00:09:00  killough
// change iwad search path order
//
// Revision 1.9  1998/02/17  06:09:35  killough
// Cache D_DoomExeDir and support basesavegame
//
// Revision 1.8  1998/02/02  13:20:03  killough
// Ultimate Doom, -fastdemo -nodraw -noblit support, default_compatibility
//
// Revision 1.7  1998/01/30  18:48:15  phares
// Changed textspeed and textwait to functions
//
// Revision 1.6  1998/01/30  16:08:59  phares
// Faster end-mission text display
//
// Revision 1.5  1998/01/26  19:23:04  phares
// First rev with no ^Ms
//
// Revision 1.4  1998/01/26  05:40:12  killough
// Fix Doom 1 crashes on -warp with too few args
//
// Revision 1.3  1998/01/24  21:03:04  jim
// Fixed disappearence of nomonsters, respawn, or fast mode after demo play or IDCLEV
//
// Revision 1.1.1.1  1998/01/19  14:02:53  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
