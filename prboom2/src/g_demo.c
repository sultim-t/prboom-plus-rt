/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2001 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 * DESCRIPTION: Demo stuff
 *
 *-----------------------------------------------------------------------------
 */

#if ((defined _MSC_VER) || (defined DREAMCAST))
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#endif
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "z_zone.h"

#include "c_io.h"
#include "c_runcmd.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "w_wad.h"
#include "m_argv.h"
#include "m_random.h"
#include "mn_menus.h"
#include "i_system.h"
#include "lprintf.h"

#ifdef COMPILE_VIDD
#include "vidd/vidd.h"
#endif

boolean         demorecording;
boolean         demoplayback;
boolean         singledemo;           // quit after playing a demo from cmdline

extern boolean netdemo;
static const byte *demobuffer;   /* cph - only used for playback */
static FILE    *demofp; /* cph - record straight to file */
static const byte *demo_p;

static boolean timedemo_menuscreen;
static int starttime;     // for comparative timing purposes
static int startgametic;
int longtics;

static const byte* G_ReadDemoHeader(const byte* demo_p);

const char * comp_lev_str[] = 
{ "doom v1.2", "doom v1.666", "doom2 v1.9",
  "ultimate doom v1.9", "final doom/doom95", 
  "early DosDoom", "TASDoom",
  "\"boom compatibility\"", "boom v2.01", "boom v2.02", "lxdoom v1.3.2+", 
  "MBF", "PrBoom 2.03beta", "PrBoom v2.1.0-2.1.1", "PrBoom v2.1.2-v2.2.3",
  "Current PrBoom"  };

//
// DEMO RECORDING
//

#define DEMOMARKER    0x80

void G_ReadDemoTiccmd (ticcmd_t* cmd)
{
  if (*demo_p == DEMOMARKER)
    G_CheckDemoStatus();      // end of demo data stream
  else
    {
      cmd->forwardmove = ((signed char)*demo_p++);
      cmd->sidemove = ((signed char)*demo_p++);
      if (!longtics) {
        cmd->angleturn = ((unsigned char)*demo_p++)<<8;
      } else {
        unsigned int lowbyte = (unsigned char)*demo_p++;
	cmd->angleturn = (((signed int)(*demo_p++))<<8) + lowbyte;
      }
      cmd->buttons = (unsigned char)*demo_p++;
    }
}

/* Demo limits removed -- killough
 * cph - record straight to file
 */
static inline signed char fudge(signed char b)
{
  b |= 1; if (b>2) b-=2;
  return b;
}

void G_WriteDemoTiccmd (ticcmd_t* cmd)
{
  char buf[5];

  buf[0] = (cmd->forwardmove && demo_compatibility) ?
    fudge(cmd->forwardmove) : cmd->forwardmove;
  buf[1] = cmd->sidemove;
  if (!longtics) {
    buf[2] = (cmd->angleturn+128)>>8;
  } else {
    buf[2] = cmd->angleturn & 0xff;
    buf[3] = (cmd->angleturn >> 8) & 0xff;
  }
  buf[longtics ? 4 : 3] = cmd->buttons;
  if (fwrite(buf, longtics ? 5 : 4, 1, demofp) != 1)
    I_Error("G_WriteDemoTiccmd: error writing demo"); // FIXME

  /* cph - alias demo_p to it so we can read it back */
  demo_p = buf;
  G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same
}

//
// G_RecordDemo
//

void G_RecordDemo (const char* name)
{
  char     demoname[PATH_MAX];
  usergame = false;  
  AddDefaultExtension(strcpy(demoname, name), ".lmp");  // 1/18/98 killough
  demorecording = true;
  /* cph - Record demos straight to file
   * If file already exists, try to continue existing demo
   */
  if (access(demoname, F_OK)) {
    demofp = fopen(demoname, "wb");
  } else {
    demofp = fopen(demoname, "r+");
    if (demofp) {
      int slot = -1;
      int rc;
      {
	byte buf[200];
	size_t len;
	fread(buf, 1, sizeof(buf), demofp);
	
	len = G_ReadDemoHeader(buf) - buf;
	fseek(demofp, len, SEEK_SET);
      }
      do {
	byte buf[4];
	rc = fread(buf, 1, sizeof(buf), demofp);
	if (buf[0] == DEMOMARKER) break;
	if (buf[3] & BT_SPECIAL)
	  if ((buf[3] & BT_SPECIALMASK) == BTS_SAVEGAME)
	    slot = (buf[3] & BTS_SAVEMASK)>>BTS_SAVESHIFT;
      } while (rc == /* sizeof(buf) is out of scope here */ 4 );
      if (slot == -1) I_Error("G_RecordDemo: No save in demo, can't continue");
      fseek(demofp, -rc, SEEK_CUR);
      G_LoadGame(slot, false);
      autostart = false;
    }
  }
  if (!demofp)
  	I_Error("G_RecordDemo: failed to open %s", name);
}

// These functions are used to read and write game-specific options in demos
// and savegames so that demo sync is preserved and savegame restoration is
// complete. Not all options (for example "compatibility"), however, should
// be loaded and saved here. It is extremely important to use the same
// positions as before for the variables, so if one becomes obsolete, the
// byte(s) should still be skipped over or padded with 0's.
// Lee Killough 3/1/98

extern int forceOldBsp;

byte *G_WriteOptions(byte *demo_p)
{
  byte *target = demo_p + GAME_OPTION_SIZE;

  *demo_p++ = monsters_remember;  // part of monster AI

  *demo_p++ = variable_friction;  // ice & mud

  *demo_p++ = weapon_recoil;      // weapon recoil

  *demo_p++ = allow_pushers;      // MT_PUSH Things

  *demo_p++ = 0;

  *demo_p++ = player_bobbing;  // whether player bobs or not

  // killough 3/6/98: add parameters to savegame, move around some in demos
  *demo_p++ = respawnparm;
  *demo_p++ = fastparm;
  *demo_p++ = nomonsters;

  *demo_p++ = demo_insurance;        // killough 3/31/98

  // killough 3/26/98: Added rngseed. 3/31/98: moved here
  *demo_p++ = (byte)((rngseed >> 24) & 0xff);
  *demo_p++ = (byte)((rngseed >> 16) & 0xff);
  *demo_p++ = (byte)((rngseed >>  8) & 0xff);
  *demo_p++ = (byte)( rngseed        & 0xff);

  // Options new to v2.03 begin here

  *demo_p++ = monster_infighting;   // killough 7/19/98

#ifdef DOGS
  *demo_p++ = dogs;                 // killough 7/19/98
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = 0;
  *demo_p++ = 0;

  *demo_p++ = (distfriend >> 8) & 0xff;  // killough 8/8/98  
  *demo_p++ =  distfriend       & 0xff;  // killough 8/8/98  

  *demo_p++ = monster_backing;         // killough 9/8/98

  *demo_p++ = monster_avoid_hazards;    // killough 9/9/98

  *demo_p++ = monster_friction;         // killough 10/98

  *demo_p++ = help_friends;             // killough 9/9/98

#ifdef DOGS
  *demo_p++ = dog_jumping;
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = monkeys;

  {   // killough 10/98: a compatibility vector now
    int i;
    for (i=0; i < COMP_TOTAL; i++)
      *demo_p++ = comp[i] != 0;
  }

  *demo_p++ = (compatibility_level >= prboom_2_compatibility) && forceOldBsp; // cph 2002/07/20

  //----------------
  // Padding at end
  //----------------
  while (demo_p < target)
    *demo_p++ = 0;

  if (demo_p != target)
    I_Error("G_WriteOptions: GAME_OPTION_SIZE is too small");

  return target;
}

/* Same, but read instead of write
 * cph - const byte*'s
 */

const byte *G_ReadOptions(const byte *demo_p)
{
  const byte *target = demo_p + GAME_OPTION_SIZE;

  monsters_remember = *demo_p++;

  variable_friction = *demo_p;  // ice & mud
  demo_p++;

  weapon_recoil = *demo_p;       // weapon recoil
  demo_p++;

  allow_pushers = *demo_p;      // MT_PUSH Things
  demo_p++;

  demo_p++;

  player_bobbing = *demo_p;     // whether player bobs or not
  demo_p++;

  // killough 3/6/98: add parameters to savegame, move from demo
  respawnparm = *demo_p++;
  fastparm = *demo_p++;
  nomonsters = *demo_p++;

  demo_insurance = *demo_p++;              // killough 3/31/98

  // killough 3/26/98: Added rngseed to demos; 3/31/98: moved here

  rngseed  = *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;

  // Options new to v2.03
  if (mbf_features)
    {
      monster_infighting = *demo_p++;   // killough 7/19/98

#ifdef DOGS
      dogs = *demo_p++;                 // killough 7/19/98
#else
      demo_p++;
#endif

      demo_p += 2;

      distfriend = *demo_p++ << 8;      // killough 8/8/98
      distfriend+= *demo_p++;

      monster_backing = *demo_p++;     // killough 9/8/98

      monster_avoid_hazards = *demo_p++; // killough 9/9/98

      monster_friction = *demo_p++;      // killough 10/98

      help_friends = *demo_p++;          // killough 9/9/98

#ifdef DOGS
      dog_jumping = *demo_p++;           // killough 10/98
#else
      demo_p++;
#endif

      monkeys = *demo_p++;

      {   // killough 10/98: a compatibility vector now
	int i;
	for (i=0; i < COMP_TOTAL; i++)
	  comp[i] = *demo_p++;
      }

      forceOldBsp = *demo_p++; // cph 2002/07/20
    }
  else  /* defaults for versions <= 2.02 */
    {
      monster_infighting = 1;           // killough 7/19/98

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98

      help_friends = 0;                 // killough 9/9/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monkeys = 0;
    }

  G_Compatibility();
  return target;
}

void G_BeginRecording (void)
{
  int i;
  byte *demostart, *demo_p;
  demostart = demo_p = malloc(1000);

  /* cph - 3 demo record formats supported: MBF+, BOOM, and Doom v1.9 */
  if (mbf_features) {
    { /* Write version code into demo */
      unsigned char v;
      switch(compatibility_level) {
	      case mbf_compatibility: v = 204; break;
	      case prboom_2_compatibility: v = 210; break;
	      case prboom_3_compatibility: v = 211; break;
	      case prboom_4_compatibility: v = 212; break;
      }
      *demo_p++ = v;
    }
    
    // signature
    *demo_p++ = 0x1d;
    *demo_p++ = 'M';
    *demo_p++ = 'B';
    *demo_p++ = 'F';
    *demo_p++ = 0xe6;
    *demo_p++ = '\0';
    
    /* killough 2/22/98: save compatibility flag in new demos
     * cph - FIXME? MBF demos will always be not in compat. mode */
    *demo_p++ = 0;
    
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = consoleplayer;
    
    demo_p = G_WriteOptions(demo_p); // killough 3/1/98: Save game options
    
    for (i=0 ; i<MAXPLAYERS ; i++)
      *demo_p++ = playeringame[i];
    
    // killough 2/28/98:
    // We always store at least MIN_MAXPLAYERS bytes in demo, to
    // support enhancements later w/o losing demo compatibility
    
    for (; i<MIN_MAXPLAYERS; i++)
      *demo_p++ = 0;

  } else if (compatibility_level >= boom_compatibility_compatibility) {
    byte v, c; /* Nominally, version and compatibility bits */
    switch (compatibility_level) {
    case boom_compatibility_compatibility: v = 202, c = 1; break;
    case boom_201_compatibility: v = 201; c = 0; break;
    case boom_202_compatibility: v = 202, c = 0; break;
    default: I_Error("G_BeginRecording: Boom compatibility level unrecognised?");
    }
    *demo_p++ = v;
    
    // signature
    *demo_p++ = 0x1d;
    *demo_p++ = 'B';
    *demo_p++ = 'o';
    *demo_p++ = 'o';
    *demo_p++ = 'm';
    *demo_p++ = 0xe6;
    
    /* CPhipps - save compatibility level in demos */
    *demo_p++ = c; 
    
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = consoleplayer;
    
    demo_p = G_WriteOptions(demo_p); // killough 3/1/98: Save game options
    
    for (i=0 ; i<MAXPLAYERS ; i++)
      *demo_p++ = playeringame[i];

    // killough 2/28/98:
    // We always store at least MIN_MAXPLAYERS bytes in demo, to
    // support enhancements later w/o losing demo compatibility
    
    for (; i<MIN_MAXPLAYERS; i++)
      *demo_p++ = 0;
  } else { // cph - write old v1.9 demos (might even sync)
    *demo_p++ = longtics ? 111 : 109; // v1.9 has best chance of syncing these
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;
    for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
      *demo_p++ = playeringame[i];
  }

  if (fwrite(demostart, 1, demo_p-demostart, demofp) != (size_t)(demo_p-demostart))
    I_Error("G_BeginRecording: Error writing demo header");
  free(demostart);
}

//
// G_PlayDemo
//

static char *defdemoname = NULL;

static set_defdemoname(const char *name)
{
	if (defdemoname)
		free(defdemoname);
	defdemoname = malloc(strlen(name)+1);
	if (!defdemoname)
		I_Error("set_defdemoname: not enough memory!");
	strcpy(defdemoname, name);
}

void G_DeferedPlayDemo (const char* name)
{
  set_defdemoname(name);
  gameaction = ga_playdemo;
}

static int demolumpnum = -1;

static int G_GetOriginalDoomCompatLevel(int ver)
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

static const byte* G_ReadDemoHeader(const byte *demo_p)
{
  skill_t skill;
  int i, episode, map;
  int demover;
  const byte *option_p = NULL;      /* killough 11/98 */

  basetic = gametic;  // killough 9/29/98

  // killough 2/22/98, 2/28/98: autodetect old demos and act accordingly.
  // Old demos turn on demo_compatibility => compatibility; new demos load
  // compatibility flag, and other flags as well, as a part of the demo.

  demover = *demo_p++;

  if (demover < 200)     // Autodetect old demos
    {
      // killough 3/2/98: force these variables to be 0 in demo_compatibility
      longtics = (demover >= 111);

      variable_friction = 0;

      weapon_recoil = 0;

      allow_pushers = 0;

      monster_infighting = 1;           // killough 7/19/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98
      help_friends = 0;                 // killough 9/9/98
      monkeys = 0;

      // killough 3/6/98: rearrange to fix savegame bugs (moved fastparm,
      // respawnparm, nomonsters flags to G_LoadOptions()/G_SaveOptions())

      if ((skill=demover) >= 100)         // For demos from versions >= 1.4
        {
	  compatibility_level = G_GetOriginalDoomCompatLevel(demover);
          skill = *demo_p++;
          episode = *demo_p++;
          map = *demo_p++;
          deathmatch = *demo_p++;
          respawnparm = *demo_p++;
          fastparm = *demo_p++;
          nomonsters = *demo_p++;
          consoleplayer = *demo_p++;
        }
      else
        {
	  compatibility_level = doom_12_compatibility;
          episode = *demo_p++;
          map = *demo_p++;
          deathmatch = respawnparm = fastparm =
            nomonsters = consoleplayer = 0;
        }
      G_Compatibility();
    }
  else    // new versions of demos
    {
      demo_p += 6;               // skip signature;
      switch (demover) {
      case 200: /* BOOM */
      case 201:
        if (!*demo_p++)
	  compatibility_level = boom_201_compatibility;
        else
	  compatibility_level = boom_compatibility_compatibility;
	break;
      case 202:
        if (!*demo_p++)
	  compatibility_level = boom_202_compatibility; 
        else
	  compatibility_level = boom_compatibility_compatibility;
	break;
      case 203:
	/* LxDoom or MBF - determine from signature
	 * cph - load compatibility level */
	switch (demobuffer[2]) {
	case 'B': /* LxDoom */
	/* cph - DEMOSYNC - LxDoom demos recorded in compatibility modes support dropped */
	  compatibility_level = lxdoom_1_compatibility;
	  break;
	case 'M':
	  compatibility_level = mbf_compatibility;
	  demo_p++;
	  break;
	}
	break;
      case 210:
	compatibility_level = prboom_2_compatibility;
	demo_p++;
	break;
      case 211:
	compatibility_level = prboom_3_compatibility;
	demo_p++;
	break;
      case 212:
	compatibility_level = prboom_4_compatibility;
	demo_p++;
	break;
      }
      skill = *demo_p++;
      episode = *demo_p++;
      map = *demo_p++;
      deathmatch = *demo_p++;
      consoleplayer = *demo_p++;

      /* killough 11/98: save option pointer for below */
      if (mbf_features)
	option_p = demo_p;

      demo_p = G_ReadOptions(demo_p);  // killough 3/1/98: Read game options

      if (demover == 200)              // killough 6/3/98: partially fix v2.00 demos
        demo_p += 128-GAME_OPTION_SIZE;
    }

  if (sizeof(comp_lev_str)/sizeof(comp_lev_str[0]) != MAX_COMPATIBILITY_LEVEL)
    I_Error("G_ReadDemoHeader: compatibility level strings incomplete");
  lprintf(LO_INFO, "G_DoPlayDemo: playing demo with %s compatibility\n", 
	  comp_lev_str[compatibility_level]);

  if (demo_compatibility)  // only 4 players can exist in old demos
    {
      for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
        playeringame[i] = *demo_p++;
      for (;i < MAXPLAYERS; i++)
        playeringame[i] = 0;
    }
  else
    {
      for (i=0 ; i < MAXPLAYERS; i++)
        playeringame[i] = *demo_p++;
      demo_p += MIN_MAXPLAYERS - MAXPLAYERS;
    }

  if (playeringame[1])
    {
      netgame = true;
      netdemo = true;
    }

  // proff - set default colours for demo playback
  for (i=0; i<MAXPLAYERS;i++)
    players[i].colormap = i % TRANSLATIONCOLOURS;

  if (gameaction != ga_loadgame) { /* killough 12/98: support -loadgame */
    G_InitNewNum(skill, episode, map);
  }

  for (i=0; i<MAXPLAYERS;i++)         // killough 4/24/98
    players[i].cheats = 0;

  return demo_p;
}

void G_DoPlayDemo(void) 
{
  char basename[9];

  ExtractFileBase(defdemoname,basename);           // killough
  basename[8] = 0;
  printf("basename: %s\n",basename);
  demobuffer = demo_p = W_CacheLumpNum(demolumpnum = W_GetNumForName(basename));  
  /* cph - store lump number for unlocking later */

  demo_p = G_ReadDemoHeader(demo_p);

  gameaction = ga_nothing;
  usergame = false;

  demoplayback = true;
}

//
// G_TimeDemo
//

void G_TimeDemo(const char *name) // CPhipps - const char*
{
  starttime = I_GetTime_RealTime();
  startgametic = gametic;
  timingdemo = true;
  singletics = true;
  set_defdemoname(name);
  gameaction = ga_playdemo;
  timedemo_menuscreen = cmdtype == c_menu;
}

void G_StopDemo()
{
  if(!demorecording && !demoplayback) return;
  
  G_CheckDemoStatus();
  advancedemo = false;
  //C_SetConsole();
}

/* G_CheckDemoStatus
 *
 * Called after a death or level completion to allow demos to be cleaned up
 * Returns true if a new demo loop action will take place
 */
boolean G_CheckDemoStatus (void)
{
  if (demorecording)
    {
      demorecording = false;
      fprintf(demofp, "%c", DEMOMARKER);
      I_Error("G_CheckDemoStatus: Demo recorded");
      return false;  // killough
    }

#ifdef COMPILE_VIDD
  else if (VIDD_checkDemoStatus())
	  return true; // POPE
#endif

  else if (demoplayback) {
	if (timingdemo)
    {
	  int gametics = gametic - startgametic;
      int endtime = I_GetTime_RealTime();
	  int realtics = endtime - starttime;
	  
	  timingdemo = singletics = false;

	  if(realtics <= 0)     // catch div-by-zero?
	    realtics = 1;

	  if(timedemo_menuscreen)
	    MN_ShowFrameRate((gametics * TICRATE * 10) / realtics);
	  else
	    C_Printf("%i fps\n", (gametics * TICRATE) / realtics);

	  C_Printf("%i %i %i\n", starttime, endtime, gametics);
      C_Printf("Timed %u gametics in %u realtics = %-.1f frames per second",
               (unsigned) gametic,realtics,
               (unsigned) gametic * (double) TICRATE / realtics);
    }

    if (demolumpnum != -1) {
	  // cph - unlock the demo lump
	  W_UnlockLumpNum(demolumpnum);
	  demolumpnum = -1;
	}
    G_ReloadDefaults();    // killough 3/1/98
    netgame = netdemo = false;       // killough 3/29/98
    deathmatch = false;
    demoplayback = false;
    if (singledemo)// || !advance)
	{
	  demoplayback = false;
	  C_SetConsole();
	  return false;
	}

    // next demo      

    D_AdvanceDemo();

    return true;
  }

  return false;
}

