// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// EDF Core Module
//
// EDF is the answer to moving most of the remaining static data
// out of the executable and into user-editable data files. Uses
// the customized version of libConfuse to allow for ease of editing
// and parsing.
//
// Current layout of EDF modules and their contents follows. Note
// that these names are just the defaults; user EDF replacements
// can use whatever names and layouts they so wish.
//
// * root.edf ..... This file includes the others, and is opened by
//                  E_ProcessEDF by default
// * sprites.edf .. Includes the sprnames array and sprite-based
//                  pickup item definitions
// * sounds.edf ... Contains the sfxinfo structures
//                  See e_sound.c for implementation.
// * frames.edf ... Contains the states structures
//                  See e_states.c for implementation.
// * things.edf ... Contains the mobjinfo structures
//                  See e_things.c for implementation.
// * cast.edf ..... Contains DOOM II cast call definitions
// * misc.edf ..... Miscellaneous stuff
//
// EDF can also be loaded from WAD lumps, starting from the newest
// lump named "EDFROOT". EDF lumps currently take total precedence
// over any EDF file specified via GFS or command-line.
//
// Other sources of EDF data include:
// * ESTRINGS ..... Lump chain that can define more string objects.
//                  See e_string.c for implementation.
//
// By James Haley
//
//----------------------------------------------------------------------------

#include <errno.h>

#include "z_zone.h"
#include "w_wad.h"
#include "i_system.h"
#include "d_main.h"
#include "d_dehtbl.h"
#include "d_gi.h"
#include "d_io.h"
#include "doomdef.h"
#include "info.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_enemy.h"
#include "p_pspr.h"
#include "f_finale.h"
#include "m_qstr.h"

#include "Confuse/confuse.h"

#define NEED_EDF_DEFINITIONS

#include "e_lib.h"
#include "e_edf.h"
#include "e_sound.h"
#include "e_string.h"
#include "e_things.h"
#include "e_states.h"
#include "e_ttypes.h"

// EDF Keywords used by features implemented in this module

// Sprite names list
#define SEC_SPRITE "spritenames"

// Sprite variables
#define ITEM_PLAYERSPRITE "playersprite"
#define ITEM_BLANKSPRITE "blanksprite"

// Sprite pick-up effects
#define SEC_PICKUPFX "pickupitem"
#define ITEM_PICKUPFX "effect"

// Cast call
#define SEC_CAST "castinfo"
#define ITEM_CAST_TYPE "type"
#define ITEM_CAST_NAME "name"
#define ITEM_CAST_SA   "stopattack"
#define ITEM_CAST_SOUND "sound"
#define ITEM_CAST_SOUNDFRAME "frame"
#define ITEM_CAST_SOUNDNAME "sfx"

// Cast order array
#define SEC_CASTORDER "castorder"

// Boss types
#define SEC_BOSSTYPES "boss_spawner_types"
#define SEC_BOSSPROBS "boss_spawner_probs"  // schepe

// Miscellaneous variables
#define ITEM_D2TITLETICS "doom2_title_tics"
#define ITEM_INTERPAUSE  "intermission_pause"
#define ITEM_INTERFADE   "intermission_fade"
#define ITEM_INTERTL     "intermission_tl"

// sprite variables (global)

int playerSpriteNum;
int blankSpriteNum;

// pickup variables

// pickup effect names (these are currently searched linearly)
// matching enum values are defined in e_edf.h

const char *pickupnames[PFX_NUMFX] =
{
   "PFX_NONE",
   "PFX_GREENARMOR",
   "PFX_BLUEARMOR",
   "PFX_POTION",
   "PFX_ARMORBONUS",
   "PFX_SOULSPHERE",
   "PFX_MEGASPHERE",
   "PFX_BLUEKEY",
   "PFX_YELLOWKEY",
   "PFX_REDKEY",
   "PFX_BLUESKULL",
   "PFX_YELLOWSKULL",
   "PFX_REDSKULL",
   "PFX_STIMPACK",
   "PFX_MEDIKIT",
   "PFX_INVULNSPHERE",
   "PFX_BERZERKBOX",
   "PFX_INVISISPHERE",
   "PFX_RADSUIT",
   "PFX_ALLMAP",
   "PFX_LIGHTAMP",
   "PFX_CLIP",
   "PFX_CLIPBOX",
   "PFX_ROCKET",
   "PFX_ROCKETBOX",
   "PFX_CELL",
   "PFX_CELLPACK",
   "PFX_SHELL",
   "PFX_SHELLBOX",
   "PFX_BACKPACK",
   "PFX_BFG",
   "PFX_CHAINGUN",
   "PFX_CHAINSAW",
   "PFX_LAUNCHER",
   "PFX_PLASMA",
   "PFX_SHOTGUN",
   "PFX_SSG",
   "PFX_HGREENKEY",
   "PFX_HBLUEKEY",
   "PFX_HYELLOWKEY",
   "PFX_HPOTION",
   "PFX_SILVERSHIELD",
   "PFX_ENCHANTEDSHIELD",
   "PFX_BAGOFHOLDING",
   "PFX_HMAP",
   "PFX_GWNDWIMPY",
   "PFX_GWNDHEFTY",
   "PFX_MACEWIMPY",
   "PFX_MACEHEFTY",
   "PFX_CBOWWIMPY",
   "PFX_CBOWHEFTY",
   "PFX_BLSRWIMPY",
   "PFX_BLSRHEFTY",
   "PFX_PHRDWIMPY",
   "PFX_PHRDHEFTY",
   "PFX_SKRDWIMPY",
   "PFX_SKRDHEFTY",
   "PFX_TOTALINVIS",
};

// pickupfx lookup table used in P_TouchSpecialThing (is allocated
// with size NUMSPRITES)
int *pickupfx = NULL;

// Note:
// For maximum efficiency, the number of chains in these hash tables
// should be a prime number at least as large as the number of
// elements to be contained in each respective table. Since definitions
// can be added by the user, it is best to choose a number a bit larger
// than necessary, which allows more of a buffer for user-defined types.

// Temporary hash tables (used only during EDF processing)

// Sprite hashing
#define NUMSPRCHAINS 257
static int *sprchains = NULL;
static int *sprnext = NULL;

// prototype of libConfuse parser inclusion function
extern int cfg_lexer_include(cfg_t *cfg, const char *filename, int data);

// 02/09/05: prototype of custom source type query function
extern int cfg_lexer_source_type(cfg_t *cfg);

// function prototypes for libConfuse callbacks (aka EDF functions)

static int bex_include(cfg_t *cfg, cfg_opt_t *opt, int argc, 
                       const char **argv);

static int edf_ifenabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                         const char **argv);

static int edf_ifenabledany(cfg_t *cfg, cfg_opt_t *opt, int argc,
                            const char **argv);

static int edf_ifdisabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv);

static int edf_ifdisabledany(cfg_t *cfg, cfg_opt_t *opt, int argc,
                             const char **argv);

static int edf_enable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                      const char **argv);

static int edf_disable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv);

static int edf_ifgametype(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv);

static int edf_ifngametype(cfg_t *cfg, cfg_opt_t *opt, int argc,
                           const char **argv);

// EDF libConfuse option structures

// sprite-based pickup items

static cfg_opt_t pickup_opts[] =
{
   CFG_STR(ITEM_PICKUPFX, "PFX_NONE", CFGF_NONE),
   CFG_END()
};

// cast call
static cfg_opt_t cast_sound_opts[] =
{
   CFG_STR(ITEM_CAST_SOUNDFRAME, "S_NULL", CFGF_NONE),
   CFG_STR(ITEM_CAST_SOUNDNAME,  "none",   CFGF_NONE),
   CFG_END()
};

static cfg_opt_t cast_opts[] =
{
   CFG_STR(ITEM_CAST_TYPE, NULL, CFGF_NONE),
   CFG_STR(ITEM_CAST_NAME, "unknown", CFGF_NONE),
   CFG_BOOL(ITEM_CAST_SA,  cfg_false, CFGF_NONE),
   CFG_SEC(ITEM_CAST_SOUND, cast_sound_opts, CFGF_MULTI|CFGF_NOCASE),
   CFG_END()
};

//
// EDF Root Options
//

static cfg_opt_t edf_opts[] =
{
   CFG_STR(SEC_SPRITE,        0,                 CFGF_LIST),
   CFG_STR(ITEM_PLAYERSPRITE, "PLAY",            CFGF_NONE),
   CFG_STR(ITEM_BLANKSPRITE,  "TNT1",            CFGF_NONE),
   CFG_SEC(SEC_PICKUPFX,      pickup_opts,       CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_SOUND,     edf_sound_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_FRAME,     edf_frame_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_THING,     edf_thing_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(SEC_CAST,          cast_opts,         CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_SPLASH,    edf_splash_opts,   CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERRAIN,   edf_terrn_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERDELTA,  edf_terdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_FLOOR,     edf_floor_opts,    CFGF_MULTI | CFGF_NOCASE),
   CFG_STR(SEC_CASTORDER,     0,                 CFGF_LIST),
   CFG_STR(SEC_BOSSTYPES,     0,                 CFGF_LIST),
   CFG_INT(SEC_BOSSPROBS,     0,                 CFGF_LIST), // schepe
   CFG_SEC(EDF_SEC_FRMDELTA,  edf_fdelta_opts,   CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TNGDELTA,  edf_tdelta_opts,   CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_SDELTA,    edf_sdelta_opts,   CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_STRING,    edf_string_opts,   CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_INT(ITEM_D2TITLETICS,  0,                 CFGF_NONE),
   CFG_INT(ITEM_INTERPAUSE,   0,                 CFGF_NONE),
   CFG_INT(ITEM_INTERFADE,   -1,                 CFGF_NONE),
   CFG_INT_CB(ITEM_INTERTL,   0,                 CFGF_NONE, E_TranslucCB),
   CFG_FUNC("include",        E_Include),
   CFG_FUNC("lumpinclude",    E_LumpInclude),
   CFG_FUNC("include_prev",   E_IncludePrev),
   CFG_FUNC("stdinclude",     E_StdInclude),
   CFG_FUNC("bexinclude",     bex_include),
   CFG_FUNC("ifenabled",      edf_ifenabled),
   CFG_FUNC("ifenabledany",   edf_ifenabledany),
   CFG_FUNC("ifdisabled",     edf_ifdisabled),
   CFG_FUNC("ifdisabledany",  edf_ifdisabledany),
   CFG_FUNC("endif",          E_Endif),
   CFG_FUNC("enable",         edf_enable),
   CFG_FUNC("disable",        edf_disable),
   CFG_FUNC("ifgametype",     edf_ifgametype),
   CFG_FUNC("ifngametype",    edf_ifngametype),
   CFG_END()
};

// These EDF functions should be available in all defaults
#define DEF_FUNCTIONS \
   CFG_FUNC("include",       E_Include), \
   CFG_FUNC("lumpinclude",   E_LumpInclude), \
   CFG_FUNC("include_prev",  E_IncludePrev), \
   CFG_FUNC("stdinclude",    E_StdInclude), \
   CFG_FUNC("bexinclude",    bex_include), \
   CFG_FUNC("ifenabled",     edf_ifenabled), \
   CFG_FUNC("ifenabledany",  edf_ifenabledany), \
   CFG_FUNC("ifdisabled",    edf_ifdisabled), \
   CFG_FUNC("ifdisabledany", edf_ifdisabledany), \
   CFG_FUNC("endif",         E_Endif), \
   CFG_FUNC("enable",        edf_enable), \
   CFG_FUNC("disable",       edf_disable), \
   CFG_FUNC("ifgametype",    edf_ifgametype), \
   CFG_FUNC("ifngametype",   edf_ifngametype)

// Default opt arrays -- these are for detection and correction
// of missing required definitions. They rely on the documented
// structure and presence of the default EDF files.

// Options for stuff in sprites.edf only.
static cfg_opt_t sprite_only_opts[] =
{
   CFG_STR(SEC_SPRITE,   0,           CFGF_LIST),
   CFG_SEC(SEC_PICKUPFX, pickup_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   DEF_FUNCTIONS,
   CFG_END()
};

// Options for stuff in frames.edf only.
static cfg_opt_t frame_only_opts[] =
{
   CFG_SEC(EDF_SEC_FRAME, edf_frame_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   DEF_FUNCTIONS,
   CFG_END()
};

// Options for stuff in things.edf only.
static cfg_opt_t thing_only_opts[] =
{
   CFG_SEC(EDF_SEC_THING, edf_thing_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   DEF_FUNCTIONS,
   CFG_END()
};

// Options for stuff in cast.edf only
static cfg_opt_t cast_only_opts[] =
{
   CFG_SEC(SEC_CAST,      cast_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_STR(SEC_CASTORDER, 0,         CFGF_LIST),
   DEF_FUNCTIONS,
   CFG_END()
};

// Options for stuff in sounds.edf only
static cfg_opt_t sound_only_opts[] =
{
   CFG_SEC(EDF_SEC_SOUND, edf_sound_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   DEF_FUNCTIONS,
   CFG_END()
};

// Options for stuff in terrain.edf only
static cfg_opt_t terrain_only_opts[] =
{
   CFG_SEC(EDF_SEC_SPLASH,    edf_splash_opts,   CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERRAIN,   edf_terrn_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERDELTA,  edf_terdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_FLOOR,     edf_floor_opts,    CFGF_MULTI | CFGF_NOCASE),
   DEF_FUNCTIONS,
   CFG_END()
};

//
// Separate Lump opt Arrays. These are for lumps that can be parsed
// on their own to add some types of definitions to EDF.
//

// These EDF functions should be available in all separate lumps.
#define LUMP_FUNCTIONS \
   CFG_FUNC("include",       E_Include), \
   CFG_FUNC("lumpinclude",   E_LumpInclude), \
   CFG_FUNC("include_prev",  E_IncludePrev), \
   CFG_FUNC("stdinclude",    E_StdInclude), \
   CFG_FUNC("ifenabled",     edf_ifenabled), \
   CFG_FUNC("ifenabledany",  edf_ifenabledany), \
   CFG_FUNC("ifdisabled",    edf_ifdisabled), \
   CFG_FUNC("ifdisabledany", edf_ifdisabledany), \
   CFG_FUNC("endif",         E_Endif), \
   CFG_FUNC("enable",        edf_enable), \
   CFG_FUNC("disable",       edf_disable), \
   CFG_FUNC("ifgametype",    edf_ifgametype), \
   CFG_FUNC("ifngametype",   edf_ifngametype)


// Options for stuff in ESTRINGS lump
static cfg_opt_t string_only_opts[] =
{
   CFG_SEC(EDF_SEC_STRING, edf_string_opts, CFGF_MULTI|CFGF_TITLE|CFGF_NOCASE),
   LUMP_FUNCTIONS,
   CFG_END()
};

// Options for stuff in ETERRAIN lump
static cfg_opt_t terrain_lump_opts[] =
{
   CFG_SEC(EDF_SEC_SPLASH,    edf_splash_opts,   CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERRAIN,   edf_terrn_opts,    CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_TERDELTA,  edf_terdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(EDF_SEC_FLOOR,     edf_floor_opts,    CFGF_MULTI | CFGF_NOCASE),
   LUMP_FUNCTIONS,
   CFG_END()
};

//
// Error Reporting and Logging
//
// haleyjd 08/09/05: Functionalized EDF verbose logging.
//

// verbose logging, toggled with -edfout cmdline param
static FILE *edf_output = NULL;

//
// E_EDFOpenVerboseLog
//
// Opens the verbose log file, if one isn't already open.
// haleyjd 08/10/05: now works like screenshots, so it won't
// overwrite old log files.
//
static void E_EDFOpenVerboseLog(void)
{
   if(edf_output)
      return;

   if(!access(".", W_OK))
   {
      static int lognum;
      char fn[16];
      int tries = 100;

      do
         psnprintf(fn, sizeof(fn), "edfout%.2d.txt", lognum++);
      while(!access(fn, F_OK) && --tries);

      if(tries)
         edf_output = fopen(fn, "w");
      else
      {
         if(in_textmode)
            puts("Warning: Couldn't open EDF verbose log!\n");
      }
   }
}

//
// E_EDFCloseVerboseLog
//
// Closes the verbose log, if one is open.
//
static void E_EDFCloseVerboseLog(void)
{
   if(edf_output)
   {
      E_EDFLogPuts("Closing log file\n");      
      fclose(edf_output);
   }

   edf_output = NULL;
}

//
// E_EDFLogPuts
//
// Calls fputs on the verbose log with the provided message.
//
void E_EDFLogPuts(const char *msg)
{
   if(edf_output)
      fputs(msg, edf_output);
}

//
// E_EDFLogPrintf
//
// Calls vfprintf on the verbose log for formatted messages.
//
void E_EDFLogPrintf(const char *msg, ...)
{
   if(edf_output)
   {
      va_list v;
      
      va_start(v, msg);
      vfprintf(edf_output, msg, v);
      va_end(v);
   }
}

void E_EDFLoggedErr(int lv, const char *msg, ...)
{
   va_list v;
      
   va_start(v, msg);

   if(edf_output)
   {
      while(lv--)
         putc('\t', edf_output);
      
      vfprintf(edf_output, msg, v);
   }

   I_ErrorVA(msg, v);

   va_end(v); // for formality only, is unreachable.
}

//
// EDF-Specific Callback Functions
//

//
// edf_error
//
// This function is given to all cfg_t structures as the error
// callback.
//
static void edf_error(cfg_t *cfg, const char *fmt, va_list ap)
{
   E_EDFLogPuts("Exiting due to parser error\n");

   // 12/16/03: improved error messages
   if(cfg && cfg->filename)
   {
      if(cfg->line)
         fprintf(stderr, "Error at %s:%d:\n", cfg->filename, cfg->line);
      else
         fprintf(stderr, "Error in %s:\n", cfg->filename);
   }

   I_ErrorVA(fmt, ap);
}

//
// bex_include
//
// 12/12/03: New include function that allows EDF to queue
// DeHackEd/BEX files for later processing.  This helps to
// integrate BEX features such as string editing into the
// EDF/BEX superlanguage.
//
// This function interprets paths relative to the current 
// file.
//
static int bex_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv)
{
   char currentpath[PATH_MAX + 1];
   char filename[PATH_MAX + 1];

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to bexinclude()");
      return 1;
   }
   if(!cfg->filename)
   {
      cfg_error(cfg, "bexinclude: cfg_t filename is undefined");
      return 1;
   }
   if(cfg_lexer_source_type(cfg) >= 0)
   {
      cfg_error(cfg, "bexinclude: cannot call from a wad lump");
      return 1;
   }

   M_GetFilePath(cfg->filename, currentpath, sizeof(currentpath));
   psnprintf(filename, sizeof(filename), "%s/%s", currentpath, argv[0]);
   NormalizeSlashes(filename);

   // queue the file for later processing
   D_QueueDEH(filename, 0);

   return 0;
}

//
// "Enables" code
//

//
// The enables values table -- this can be fiddled with by the
// game engine using E_EDFSetEnableValue below.
//
static E_Enable_t edf_enables[] =
{
   // all game modes are enabled by default
   { "DOOM",     1 },
   { "HERETIC",  1 },
   
   // terminator
   { NULL }
};

// 
// E_EDFSetEnableValue
//
// This function lets the rest of the engine be able to set
// EDF enable values before parsing begins. This is used to
// turn DOOM and HERETIC modes on and off when loading the
// default root.edf. This saves time and memory. Note that
// they are enabled when user EDFs are loaded, but users
// can use the disable function to turn them off explicitly
// in that case when the definitions are not needed.
//
void E_EDFSetEnableValue(const char *name, int value)
{
   int idx = E_EnableNumForName(name, edf_enables);

   if(idx != -1)
      edf_enables[idx].enabled = value;
}

static void E_EchoEnables(void)
{
   E_Enable_t *enable = edf_enables;

   E_EDFLogPuts("\t* Final enable values:\n");

   while(enable->name)
   {
      E_EDFLogPrintf("\t\t%s is %s\n", 
                     enable->name, 
                     enable->enabled ? "enabled" : "disabled");
      ++enable;
   }
}

//
// edf_ifenabled
//
// haleyjd 01/14/04: Causes the parser to skip forward, looking 
// for the next endif function and then calling it, if the 
// parameter isn't defined. I hacked the support for this
// into libConfuse without too much ugliness.
//
// haleyjd 09/06/05: Altered to work on N parameters, to make
// nil the issue of not being able to nest enable tests ^_^
//
static int edf_ifenabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                         const char **argv)
{
   int i, idx;
   boolean enabled = true;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifenabled()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      if((idx = E_EnableNumForName(argv[i], edf_enables)) == -1)
      {
         cfg_error(cfg, "invalid enable value '%s'", argv[i]);
         return 1;
      }

      // use AND logic: the block will only be evaluated if ALL
      // options are enabled
      // use short circuit for efficiency
      if(!(enabled = enabled && edf_enables[idx].enabled))
         break;
   }

   // some option was disabled: skip the block
   if(!enabled)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_ifenabledany
//
// haleyjd 09/06/05: Exactly as above, but uses OR logic.
//
static int edf_ifenabledany(cfg_t *cfg, cfg_opt_t *opt, int argc,
                            const char **argv)
{
   int i, idx;
   boolean enabled = false;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifenabled()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      if((idx = E_EnableNumForName(argv[i], edf_enables)) == -1)
      {
         cfg_error(cfg, "invalid enable value '%s'", argv[i]);
         return 1;
      }

      // use OR logic: the block will be evaluated if ANY
      // option is enabled
      // use short circuit for efficiency
      if((enabled = enabled || edf_enables[idx].enabled))
         break;
   }

   // no options were enabled: skip the block
   if(!enabled)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_ifdisabled
//
// haleyjd 09/06/05: Exactly the same as ifenabled, but parses the
// section if all provided enable values are disabled. Why did I
// not provide this from the beginning? o_O
//
static int edf_ifdisabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                         const char **argv)
{
   int i, idx;
   boolean disabled = true;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifdisabled()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      if((idx = E_EnableNumForName(argv[i], edf_enables)) == -1)
      {
         cfg_error(cfg, "invalid enable value '%s'", argv[i]);
         return 1;
      }

      // use AND logic: the block will be evalued if ALL 
      // options are disabled.
      if(!(disabled = disabled && !edf_enables[idx].enabled))
         break;
   }

   if(!disabled)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_ifdisabledany
//
// haleyjd 09/06/05: Exactly as above, but uses OR logic.
//
static int edf_ifdisabledany(cfg_t *cfg, cfg_opt_t *opt, int argc,
                             const char **argv)
{
   int i, idx;
   boolean disabled = false;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifdisabled()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      if((idx = E_EnableNumForName(argv[i], edf_enables)) == -1)
      {
         cfg_error(cfg, "invalid enable value '%s'", argv[i]);
         return 1;
      }

      // use OR logic: the block will be evalued if ANY
      // option is disabled.
      if((disabled = disabled || !edf_enables[idx].enabled))
         break;
   }

   if(!disabled)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_enable
//
// Enables a builtin option from within EDF.
//
static int edf_enable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                      const char **argv)
{
   int idx;

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to enable()");
      return 1;
   }

   if((idx = E_EnableNumForName(argv[0], edf_enables)) == -1)
   {
      cfg_error(cfg, "unknown enable value '%s'", argv[0]);
      return 1;
   }

   edf_enables[idx].enabled = 1;
   return 0;
}

//
// edf_disable
//
// Disables a builtin option from within EDF.
//
static int edf_disable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv)
{
   int idx;

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to disable()");
      return 1;
   }

   if((idx = E_EnableNumForName(argv[0], edf_enables)) == -1)
   {
      cfg_error(cfg, "unknown enable value '%s'", argv[0]);
      return 1;
   }

   edf_enables[idx].enabled = 0;
   return 0;
}

//
// Game type functions
//
// haleyjd 09/06/05:
// These are for things which must vary strictly on game type and not 
// simply whether or not a given game's definitions are enabled.
//

static const char *e_typenames[] =
{
   "DOOM",
   "HERETIC",
};

//
// edf_ifgametype
//
// haleyjd 09/06/05: Just like ifenabled, but considers the game type
// from the game mode info instead of enable values.
//
static int edf_ifgametype(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv)
{
   int i, type;
   boolean type_match = false;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifgametype()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      type = E_StrToNumLinear(e_typenames, NumGameModeTypes, argv[i]);

      // if the gametype matches ANY specified, we will process the
      // block in question (can short circuit after first match)
      if((type_match = (type_match || type == gameModeInfo->type)))
         break;
   }

   // no type was matched?
   if(!type_match)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_ifngametype
//
// haleyjd 09/06/05: As above, but negated.
//
static int edf_ifngametype(cfg_t *cfg, cfg_opt_t *opt, int argc,
                           const char **argv)
{
   int i, type;
   boolean type_nomatch = true;

   if(argc < 1)
   {
      cfg_error(cfg, "wrong number of args to ifngametype()");
      return 1;
   }

   for(i = 0; i < argc; ++i)
   {
      type = E_StrToNumLinear(e_typenames, NumGameModeTypes, argv[i]);

      // if gametype equals NONE of the parameters, we will process
      // the block (can short circuit after first failure)
      if(!(type_nomatch = (type_nomatch && type != gameModeInfo->type)))
         break;
   }

   // did it match some type?
   if(!type_nomatch)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}


//
// EDF processing routines, mostly in order of execution in E_ProcessEDF
//

//
// E_ParseEDFFile
//
// Initializes a libConfuse cfg object, and then loads and parses
// the requested EDF file. Returns a pointer to the cfg object.
//
static cfg_t *E_ParseEDFFile(const char *filename, cfg_opt_t *opts)
{
   int err;
   cfg_t *cfg;

   cfg = cfg_init(opts, CFGF_NOCASE);
   cfg_set_error_function(cfg, edf_error);

   if((err = cfg_parse(cfg, filename)))
   {
      E_EDFLoggedErr(1, 
         "E_ParseEDFFile: failed to parse EDF file %s (code %d)\n",
         filename, err);
   }

   return cfg;
}

//
// E_ParseEDFLump
//
// Initializes a libConfuse cfg object, and then loads and parses
// the requested EDF lump. Returns a pointer to the cfg object.
//
static cfg_t *E_ParseEDFLump(const char *lumpname, cfg_opt_t *opts)
{
   int err;
   cfg_t *cfg;

   cfg = cfg_init(opts, CFGF_NOCASE);
   cfg_set_error_function(cfg, edf_error);

   if((err = cfg_parselump(cfg, lumpname)))
   {
      E_EDFLoggedErr(1, 
         "E_ParseEDFLump: failed to parse EDF lump %s (code %d)\n",
         lumpname, err);
   }

   return cfg;
}

//
// E_ParseEDFLumpOptional
//
// Calls the function above, but checks to make sure the lump exists
// first so that an error will not occur. Returns NULL if the lump
// wasn't found, so you must check the return value before using it.
//
static cfg_t *E_ParseEDFLumpOptional(const char *lumpname, cfg_opt_t *opts)
{
   if(W_CheckNumForName(lumpname) == -1)
      return NULL;

   return E_ParseEDFLump(lumpname, opts);
}

//
// E_ProcessStringLump
//
// Parses the ESTRINGS lump, which may optionally include the next
// ESTRINGS lump down the chain by using the include_prev function.
// This allows cascading behavior to be optional at the discretion
// of the EDF author.
//
static void E_ProcessStringLump(void)
{
   cfg_t *cfg;

   E_EDFLogPuts("\tParsing ESTRINGS lump...\n");
   
   if(!(cfg = E_ParseEDFLumpOptional("ESTRINGS", string_only_opts)))
   {
      E_EDFLogPuts("\tNo ESTRINGS lump found\n");
      return;
   }

   E_ProcessStrings(cfg);

   cfg_free(cfg);
}

//
// E_ProcessTerrainLump
//
// Parses the ETERRAIN lump, which may optionally include the next
// ETERRAIN lump down the chain by using the include_prev function.
// This allows cascading behavior to be optional at the discretion
// of the EDF author.
//
static void E_ProcessTerrainLump(void)
{
   cfg_t *cfg;

   E_EDFLogPuts("\tParsing ETERRAIN lump...\n");
   
   if(!(cfg = E_ParseEDFLumpOptional("ETERRAIN", terrain_lump_opts)))
   {
      E_EDFLogPuts("\tNo ETERRAIN lump found.\n");
      return;
   }

   E_ProcessTerrainTypes(cfg);

   cfg_free(cfg);
}

//
// E_TryDefaultTerrain
//
// Loads the default terrain.edf as a last-chance default when zero
// splash, terrain, and floor objects have been defined.
//
void E_TryDefaultTerrain(void)
{
   cfg_t *tcfg;
   const char *tfn;

   E_EDFLogPuts("\t\tAttempting to load default terrain.edf\n");

   tfn  = E_BuildDefaultFn("terrain.edf");
   tcfg = E_ParseEDFFile(tfn, terrain_only_opts);

   E_ProcessTerrainTypes(tcfg);

   // free the temporary cfg
   cfg_free(tcfg);
}

// These are needed by E_ProcessSprites:
static void E_TryDefaultSprites(void);
static void E_ProcessItems(cfg_t *);

int E_SpriteNumForName(const char *name);

//
// E_ProcessSprites
//
// Sets NUMSPRITES, allocates sprnames, sprchains, and sprnext,
// initializes the sprite hash table, and reads in all sprite names,
// adding each to the hash table as it is read.
//
static void E_ProcessSprites(cfg_t *cfg)
{
   char *spritestr;
   int i;

   E_EDFLogPuts("\t* Processing sprites\n");

   // set NUMSPRITES and allocate tables
   NUMSPRITES = cfg_size(cfg, SEC_SPRITE);

   E_EDFLogPrintf("\t\t%d sprite name(s) defined\n", NUMSPRITES);

   // at least one sprite is required -- if zero, try the defaults
   if(!NUMSPRITES)
   {
      E_TryDefaultSprites();
      return;
   }

   // 10/17/03: allocate a single sprite string instead of a bunch
   // of separate ones to save tons of memory and some time
   spritestr = Z_Malloc(5 * NUMSPRITES, PU_STATIC, 0);
   memset(spritestr, 0, 5 * NUMSPRITES);

   // allocate with size+1 for the NULL terminator
   sprnames  = Z_Malloc((NUMSPRITES + 1) * sizeof(char *),PU_STATIC,0);
   sprchains = Z_Malloc(NUMSPRCHAINS * sizeof(int),PU_STATIC,0);
   sprnext   = Z_Malloc((NUMSPRITES + 1) * sizeof(int),PU_STATIC,0);

   // initialize the sprite hash table
   for(i = 0; i < NUMSPRCHAINS; ++i)
      sprchains[i] = NUMSPRITES;
   for(i = 0; i < NUMSPRITES + 1; ++i)
      sprnext[i] = NUMSPRITES;

   for(i = 0; i < NUMSPRITES; ++i)
   {
      unsigned int key;
      // read in all sprite names
      const char *sprname = cfg_getnstr(cfg, SEC_SPRITE, i);

      if(strlen(sprname) != 4)
      {
         E_EDFLoggedErr(2, 
            "E_ProcessSprites: invalid sprite mnemonic '%s'\n", sprname);
      }

      // initialize sprnames[i] to point into the single string
      // allocation above, then copy the EDF value into that location
      sprnames[i] = spritestr + i * 5;
      
      strncpy(sprnames[i], sprname, 4);
      
      // build sprite name hash table
      key = D_HashTableKey(sprnames[i]) % NUMSPRCHAINS;
      sprnext[i] = sprchains[key];
      sprchains[key] = i;
   }

   // set the pointer at index NUMSPRITES to NULL (used by the
   // renderer when it iterates over the sprites)
   sprnames[NUMSPRITES] = NULL;

   E_EDFLogPrintf("\t\tFirst sprite = %s\n\t\tLast sprite = %s\n",
                  sprnames[0], sprnames[NUMSPRITES-1]);

   // haleyjd: call this from here now so that if the default
   // sprites.edf is loaded, these options will come from it

   // process sprite-related pickup item effects
   E_ProcessItems(cfg);
}

//
// E_TryDefaultSprites
//
// When there are zero sprite definitions, this function will
// try to load and parse the default sprites.edf module using
// options limited to the items in that file. It will then
// test the number of sprites again. If it's still zero, it's
// a fatal error. Otherwise, it calls E_ProcessSprites again
// to set the new sprites.
//
static void E_TryDefaultSprites(void)
{
   cfg_t *sprcfg;
   const char *sprfn;

   E_EDFLogPuts("\t\tAttempting to load default sprites.edf\n");

   sprfn = E_BuildDefaultFn("sprites.edf");

   sprcfg = E_ParseEDFFile(sprfn, sprite_only_opts);

   // Test NUMSPRITES again -- if it's still zero, fatal error time.
   NUMSPRITES = cfg_size(sprcfg, SEC_SPRITE);

   if(!NUMSPRITES)
      E_EDFLoggedErr(2, "E_TryDefaultSprites: missing default sprites.\n");

   // call E_ProcessSprites again (note this cannot cause another
   // recursive call to this function, since we verified that 
   // NUMSPRITES is not zero).

   E_ProcessSprites(sprcfg);

   // free the temporary cfg
   cfg_free(sprcfg);
}

//
// E_ProcessSpriteVars
//
// Sets the sprite numbers to be used for the player and blank
// sprites by looking up a provided name in the sprite hash
// table.
//
static void E_ProcessSpriteVars(cfg_t *cfg)
{
   int sprnum;
   const char *str;

   E_EDFLogPuts("\t* Processing sprite variables\n");

   // load player and blank sprite numbers
   str = cfg_getstr(cfg, ITEM_PLAYERSPRITE);
   sprnum = E_SpriteNumForName(str);
   if(sprnum == NUMSPRITES)
   {
      E_EDFLoggedErr(2, 
         "E_ProcessSpriteVars: invalid player sprite name: '%s'\n", str);
   }
   E_EDFLogPrintf("\t\tSet sprite %s(#%d) as player sprite\n", str, sprnum);
   playerSpriteNum = sprnum;

   str = cfg_getstr(cfg, ITEM_BLANKSPRITE);
   sprnum = E_SpriteNumForName(str);
   if(sprnum == NUMSPRITES)
   {
      E_EDFLoggedErr(2, 
         "E_ProcessSpriteVars: invalid blank sprite name: '%s'\n", str);
   }
   E_EDFLogPrintf("\t\tSet sprite %s(#%d) as blank sprite\n", str, sprnum);
   blankSpriteNum = sprnum;
}

//
// E_ProcessItems
//
// Allocates the pickupfx array used in P_TouchSpecialThing,
// and loads all pickupitem definitions, using the sprite hash
// table to resolve what sprite owns the specified effect.
//
static void E_ProcessItems(cfg_t *cfg)
{
   int i, numpickups;

   E_EDFLogPuts("\t* Processing pickup items\n");

   // allocate and initialize pickup effects array
   pickupfx  = Z_Malloc(NUMSPRITES * sizeof(int), PU_STATIC, 0);
   
   for(i = 0; i < NUMSPRITES; ++i)
      pickupfx[i] = PFX_NONE;
   
   // load pickupfx
   numpickups = cfg_size(cfg, SEC_PICKUPFX);
   E_EDFLogPrintf("\t\t%d pickup item(s) defined\n", numpickups);
   for(i = 0; i < numpickups; ++i)
   {
      int fxnum, sprnum;
      cfg_t *sec = cfg_getnsec(cfg, SEC_PICKUPFX, i);
      const char *title = cfg_title(sec);
      const char *pfx = cfg_getstr(sec, ITEM_PICKUPFX);

      // validate the sprite name given in the section title and
      // resolve to a sprite number (hashed)
      sprnum = E_SpriteNumForName(title);

      if(sprnum == NUMSPRITES)
      {
         E_EDFLoggedErr(2, 
            "E_ProcessItems: invalid sprite mnemonic for pickup item: '%s'\n",
            title);
      }

      // find the proper pickup effect number (linear search)
      fxnum = E_StrToNumLinear(pickupnames, PFX_NUMFX, pfx);
      if(fxnum == PFX_NUMFX)
      {
         E_EDFLoggedErr(2, 
            "E_ProcessItems: invalid pickup effect: '%s'\n", pfx);
      }
      
      E_EDFLogPrintf("\t\tSet sprite %s(#%d) to pickup effect %s(#%d)\n",
                     title, sprnum, pfx, fxnum);

      pickupfx[sprnum] = fxnum;
   }
}

static void E_AllocSounds(cfg_t *cfg);

//
// E_TryDefaultSounds
//
// Tries to load the default sounds.edf file.
//
static void E_TryDefaultSounds(void)
{
   cfg_t *soundcfg;
   const char *soundfn;

   E_EDFLogPuts("\t\tAttempting to load defaults from sounds.edf\n");

   soundfn = E_BuildDefaultFn("sounds.edf");

   soundcfg = E_ParseEDFFile(soundfn, sound_only_opts);

   NUMSFX = cfg_size(soundcfg, EDF_SEC_SOUND);

   if(!NUMSFX)
      E_EDFLoggedErr(2, "E_TryDefaultSounds: missing default sounds.\n");

   E_AllocSounds(soundcfg);

   cfg_free(soundcfg);
}

//
// E_AllocSounds
//
// Allocates and initializes the S_sfx table. This is here because it
// needs to interact with defaults loading.
//
static void E_AllocSounds(cfg_t *cfg)
{
   E_EDFLogPuts("\t* Processing sound definitions\n");

   // find out how many sounds are defined
   NUMSFX = cfg_size(cfg, EDF_SEC_SOUND);

   // try defaults if zero
   if(!NUMSFX)
   {
      E_EDFLogPuts("\t\tNo sounds defined, trying defaults\n");
      E_TryDefaultSounds();
      return;
   }

   E_EDFLogPrintf("\t\t%d sound(s) defined\n", NUMSFX);

   // add one to make room for S_sfx[0]
   ++NUMSFX;

   // let's allocate & initialize the sounds...
   S_sfx = malloc(NUMSFX * sizeof(sfxinfo_t));
   memset(S_sfx, 0, NUMSFX * sizeof(sfxinfo_t));

   // 08/10/05: must call this from here now, so that defaults
   // can kick in when no sounds exist

   E_ProcessSounds(cfg); // see e_sounds.c
}

//
// E_CollectNames
//
// Pre-creates and hashes by name the states and things, for purpose 
// of mutual and forward references.
//
// Note: scfg and tcfg may or may not point to the same cfg_t, since
// E_ProcessStatesAndThings now loads defaults from the standard EDF
// files when the number of one or both types is zero.
//
static void E_CollectNames(cfg_t *scfg, cfg_t *tcfg)
{
   E_EDFLogPuts("\t* Allocating states and things\n");

   E_EDFLogPrintf("\t\tNUMSTATES = %d, NUMMOBJTYPES = %d\n",
                  NUMSTATES, NUMMOBJTYPES);

   E_CollectStates(scfg); // see e_states.c
   E_CollectThings(tcfg); // see e_things.c
}

//
// E_TryDefaultStates
//
// This is called below when no frames are found in the root EDF.
// This function will attempt to load and parse frames.edf directly,
// and then returns a new cfg_t on success.
//
static cfg_t *E_TryDefaultStates(void)
{
   cfg_t *statecfg;
   const char *statefn;

   E_EDFLogPuts("\t\tZero frames found, attempting to load default frames.edf\n");

   statefn = E_BuildDefaultFn("frames.edf");

   statecfg = E_ParseEDFFile(statefn, frame_only_opts);

   // Reset NUMSTATES -- it will be tested again below
   NUMSTATES = cfg_size(statecfg, EDF_SEC_FRAME);

   return statecfg;
}

//
// E_TryDefaultThings
//
// This is called below when no things are found in the root EDF.
// This function will attempt to load and parse things.edf directly,
// and then returns a new cfg_t on success.
//
static cfg_t *E_TryDefaultThings(void)
{
   cfg_t *thingcfg;
   const char *thingfn;

   E_EDFLogPuts("\t\tZero things found, attempting to load default things.edf\n");

   thingfn = E_BuildDefaultFn("things.edf");

   thingcfg = E_ParseEDFFile(thingfn, thing_only_opts);

   // Reset NUMMOBJTYPES -- it will be tested again below
   NUMMOBJTYPES = cfg_size(thingcfg, EDF_SEC_THING);

   return thingcfg;
}

//
// E_ProcessStatesAndThings
//
// E_ProcessEDF now calls this function to accomplish all state
// and thing processing. This is necessary to facilitate loading
// of defaults in the event that there are zero frame or thing
// definitions.
//
static void E_ProcessStatesAndThings(cfg_t *cfg)
{
   cfg_t *thingcfg, *framecfg;
   boolean thingdefs = false, framedefs = false;

   E_EDFLogPuts("\t* Beginning state and thing processing\n");

   // start out pointers pointing at the parameter cfg_t
   framecfg = cfg;
   thingcfg = cfg;

   // check number of states
   NUMSTATES = cfg_size(cfg, EDF_SEC_FRAME);
   if(NUMSTATES == 0)
   {
      // try the defaults
      framecfg = E_TryDefaultStates();
      framedefs = true;
      if(NUMSTATES == 0)
         E_EDFLoggedErr(2, "E_ProcessStatesAndThings: zero frames defined\n");
   }

   // check number of things
   NUMMOBJTYPES = cfg_size(cfg, EDF_SEC_THING);
   if(NUMMOBJTYPES == 0)
   {
      // try the defaults
      thingcfg = E_TryDefaultThings();
      thingdefs = true;
      if(NUMMOBJTYPES == 0)
         E_EDFLoggedErr(2, "E_ProcessStatesAndThings: zero things defined\n");
   }

   // allocate structures, build mnemonic and dehnum hash tables
   E_CollectNames(framecfg, thingcfg);

   // process states: see e_states.c
   E_ProcessStates(framecfg);

   // process things: see e_things.c
   E_ProcessThings(thingcfg);

   // free default cfgs if they were loaded
   if(framedefs)
      cfg_free(framecfg);

   if(thingdefs)
      cfg_free(thingcfg);
}

static void E_TryDefaultCast(void);

//
// E_ProcessCast
//
// Creates the DOOM II cast call information
//
static void E_ProcessCast(cfg_t *cfg)
{
   int i, ci_size, cs_size;
   cfg_t **ci_order;

   E_EDFLogPuts("\t* Processing cast call\n");
   
   // get number of cast sections
   cs_size = cfg_size(cfg, SEC_CAST);

   if(!cs_size)
   {
      E_EDFLogPuts("\t\tNo cast members defined\n");

      // 11/04/03: try the default cast.edf definitions
      E_TryDefaultCast();
      return;
   }

   E_EDFLogPrintf("\t\t%d cast member(s) defined\n", cs_size);

   // check if the "castorder" array is defined for imposing an
   // order on the castinfo sections
   ci_size = cfg_size(cfg, SEC_CASTORDER);

   E_EDFLogPrintf("\t\t%d cast member(s) in castorder\n", ci_size);

   // determine size of castorder
   max_castorder = (ci_size > 0) ? ci_size : cs_size;

   // allocate with size+1 for an end marker
   castorder = malloc(sizeof(castinfo_t)*(max_castorder + 1));
   ci_order  = malloc(sizeof(cfg_t *) * max_castorder);

   if(ci_size > 0)
   {
      for(i = 0; i < ci_size; ++i)
      {
         const char *title = cfg_getnstr(cfg, SEC_CASTORDER, i);         
         cfg_t *section    = cfg_gettsec(cfg, SEC_CAST, title);

         if(!section)
         {
            E_EDFLoggedErr(2, 
               "E_ProcessCast: unknown cast member '%s' in castorder\n", 
               title);
         }

         ci_order[i] = section;
      }
   }
   else
   {
      // no castorder array is defined, so use the cast members
      // in the order they are encountered (for backward compatibility)
      for(i = 0; i < cs_size; ++i)
         ci_order[i] = cfg_getnsec(cfg, SEC_CAST, i);
   }


   for(i = 0; i < max_castorder; ++i)
   {
      int j;
      const char *tempstr;
      int tempint = 0;
      cfg_t *castsec = ci_order[i];

      // resolve thing type
      tempstr = cfg_getstr(castsec, ITEM_CAST_TYPE);
      if(!tempstr || 
         (tempint = E_ThingNumForName(tempstr)) == NUMMOBJTYPES)
      {
         E_EDFLogPrintf("\t\tWarning: cast %d: unknown thing type %s\n",
                        i, tempstr);

         tempint = UnknownThingType;
      }
      castorder[i].type = tempint;

      // get cast name, if any -- the first seventeen entries can
      // default to using the internal string editable via BEX strings
      tempstr = cfg_getstr(castsec, ITEM_CAST_NAME);
      if(cfg_size(castsec, ITEM_CAST_NAME) == 0 && i < 17)
         castorder[i].name = NULL; // set from DeHackEd
      else
         castorder[i].name = strdup(tempstr); // store provided value

      // get stopattack flag (used by player)
      castorder[i].stopattack = cfg_getbool(castsec, ITEM_CAST_SA);

      // process sound blocks (up to four will be processed)
      tempint = cfg_size(castsec, ITEM_CAST_SOUND);
      for(j = 0; j < 4; ++j)
      {
         castorder[i].sounds[j].frame = 0;
         castorder[i].sounds[j].sound = 0;
      }
      for(j = 0; j < tempint && j < 4; ++j)
      {
         int num;
         sfxinfo_t *sfx;
         const char *name;
         cfg_t *soundsec = cfg_getnsec(castsec, ITEM_CAST_SOUND, j);

         // if these are invalid, just fill them with zero rather
         // than causing an error, as they're very unimportant

         // name of sound to play
         name = cfg_getstr(soundsec, ITEM_CAST_SOUNDNAME);
         if((sfx = E_EDFSoundForName(name)) == NULL ||
            sfx->dehackednum == -1)
            castorder[i].sounds[j].sound = 0;
         else
            castorder[i].sounds[j].sound = sfx->dehackednum;

         // name of frame that triggers sound event
         name = cfg_getstr(soundsec, ITEM_CAST_SOUNDFRAME);
         if((num = E_StateNumForName(name)) == NUMSTATES)
            num = 0;
         castorder[i].sounds[j].frame = num;
      }
   }

   // initialize the end marker to all zeroes
   memset(&castorder[max_castorder], 0, sizeof(castinfo_t));

   // free the ci_order table
   free(ci_order);
}

static void E_TryDefaultCast(void)
{
   cfg_t *castcfg;
   const char *castfn;

   E_EDFLogPuts("\t\tAttempting to load defaults from cast.edf\n");

   castfn = E_BuildDefaultFn("cast.edf");

   castcfg = E_ParseEDFFile(castfn, cast_only_opts);

   max_castorder = cfg_size(castcfg, SEC_CAST);

   if(!max_castorder)
      E_EDFLoggedErr(2, "E_TryDefaultCast: missing default cast.\n");

   E_ProcessCast(castcfg);

   cfg_free(castcfg);
}

// haleyjd 11/19/03:
// a default probability array for the boss_spawn_probs list,
// for backward compatibility

static int BossDefaults[11] = 
{
   50, 90, 120, 130, 160, 162, 172, 192, 222, 246, 256
};

//
// E_ProcessBossTypes
//
// Gets the thing type entries in the boss_spawner_types list,
// for use by the SpawnFly codepointer.
//
// modified by schepe to remove 11-type limit
//
static void E_ProcessBossTypes(cfg_t *cfg)
{
   int i, a = 0;
   int numTypes = cfg_size(cfg, SEC_BOSSTYPES);
   int numProbs = cfg_size(cfg, SEC_BOSSPROBS);
   boolean useProbs = true;

   E_EDFLogPuts("\t* Processing boss spawn types\n");

   if(!numTypes)
      E_EDFLoggedErr(2, "E_ProcessBossTypes: no boss types defined\n");

   // haleyjd 11/19/03: allow defaults for boss spawn probs
   if(!numProbs)
      useProbs = false;

   if((useProbs && numTypes != numProbs) || numTypes != 11)
   {
      E_EDFLoggedErr(2, 
         "E_ProcessBossTypes: %d boss types, %d boss probs\n",
         numTypes, useProbs ? numProbs : 11);
   }

   NumBossTypes = numTypes;
   BossSpawnTypes = malloc(numTypes * sizeof(int));
   BossSpawnProbs = malloc(numTypes * sizeof(int));

   // load boss spawn probabilities
   for(i = 0; i < numTypes; ++i)
   {
      if(useProbs)
      {
         a += cfg_getnint(cfg, SEC_BOSSPROBS, i);
         BossSpawnProbs[i] = a;
      }
      else
         BossSpawnProbs[i] = BossDefaults[i];
   }

   // check that the probabilities total 256
   if(useProbs && a != 256)
   {
      E_EDFLoggedErr(2, 
         "E_ProcessBossTypes: boss spawn probs do not total 256\n");
   }

   for(i = 0; i < numTypes; ++i)
   {
      const char *typeName = cfg_getnstr(cfg, SEC_BOSSTYPES, i);
      int typeNum = E_ThingNumForName(typeName);

      if(typeNum == NUMMOBJTYPES)
      {
         E_EDFLogPrintf("\t\tWarning: invalid boss type '%s'\n", typeName);

         typeNum = UnknownThingType;
      }

      BossSpawnTypes[i] = typeNum;

      E_EDFLogPrintf("\t\tAssigned type %s(#%d) to boss type %d\n",
                     mobjinfo[typeNum].name, typeNum, i);
   }
}

extern int wi_pause_time;
extern int wi_fade_color;
extern fixed_t wi_tl_level;

//
// E_ProcessMiscVars
//
// Miscellaneous optional EDF settings.
//
static void E_ProcessMiscVars(cfg_t *cfg)
{
   // allow setting of title length for DOOM II
   if(cfg_size(cfg, ITEM_D2TITLETICS) > 0)
      giDoomCommercial.titleTics = cfg_getint(cfg, ITEM_D2TITLETICS);

   // allow setting a pause time for the intermission
   if(cfg_size(cfg, ITEM_INTERPAUSE) > 0)
      wi_pause_time = cfg_getint(cfg, ITEM_INTERPAUSE);

   if(cfg_size(cfg, ITEM_INTERFADE) > 0)
   {
      wi_fade_color = cfg_getint(cfg, ITEM_INTERFADE);
      if(wi_fade_color < 0)
         wi_fade_color = 0;
      else if(wi_fade_color > 255)
         wi_fade_color = 255;
   }

   if(cfg_size(cfg, ITEM_INTERTL) > 0)
   {
      wi_tl_level = cfg_getint(cfg, ITEM_INTERTL);
      if(wi_tl_level < 0)
         wi_tl_level = 0;
      else if(wi_tl_level > 65536)
         wi_tl_level = 65536;
   }
}

//
// E_ProcessEDFLumps
//
// Call to process reloadable separate EDF lumps.
// Only object types that can be dynamically added to and overwritten
// with new values can be loaded in this way. Currently this does
// NOT include sprites, frames, or things.
//
void E_ProcessEDFLumps(void)
{
   E_EDFLogPuts("Processing separate lumps\n");
   
   // process ESTRINGS
   E_ProcessStringLump();

   // process ETERRAIN
   E_ProcessTerrainLump();
}

//
// E_ProcessLastChance
//
// Loads absolute-last-chance defaults, which must be tested for after
// all lump processing has finished.
//
void E_ProcessLastChance(void)
{
   E_EDFLogPuts("Processing last-chance defaults\n");

   // terrain defaults
   if(E_NeedDefaultTerrain())
      E_TryDefaultTerrain();
}

// Main EDF Routine

//
// E_ProcessEDF
//
// Public function to parse the root EDF file. Called by D_DoomInit. 
// Assumes that certain BEX data structures, especially the 
// codepointer hash table, have already been built.
//
void E_ProcessEDF(const char *filename)
{
   cfg_t *cfg;
   int lnum = -1;

   // check for -edfout to enable verbose logging
   if(M_CheckParm("-edfout"))
      E_EDFOpenVerboseLog();

   // 02/09/05: check for root EDF lump
   if((lnum = W_CheckNumForName("EDFROOT")) != -1)
   {
      puts("E_ProcessEDF: Loading root lump.\n");
      E_EDFLogPuts("Processing lump EDFROOT\n");

      cfg = E_ParseEDFLump("EDFROOT", edf_opts);
   }
   else
   {
      printf("E_ProcessEDF: Loading root file %s\n", filename);
      E_EDFLogPrintf("Processing EDF file %s\n", filename);

      cfg = E_ParseEDFFile(filename, edf_opts);
   }

   E_EchoEnables();

   // NOTE: The order of most of the following calls is extremely 
   // important and must be preserved, unless the static routines 
   // above are rewritten accordingly.

   // process strings
   E_ProcessStrings(cfg);

   // process sprites, sprite-related variables, and pickup item fx
   E_ProcessSprites(cfg);

   // process sprite-related variables
   E_ProcessSpriteVars(cfg);

   // 09/03/03: process sounds
   E_AllocSounds(cfg);

   // allocate frames and things, build name hash tables, and
   // process frame and thing definitions
   E_ProcessStatesAndThings(cfg);

   // process cast call
   E_ProcessCast(cfg);

   // process boss spawn types
   E_ProcessBossTypes(cfg);

   // 08/23/05: process TerrainTypes
   E_ProcessTerrainTypes(cfg);

   // 01/11/04: process misc vars
   E_ProcessMiscVars(cfg);

   // 08/30/03: apply deltas
   E_ProcessSoundDeltas(cfg);
   E_ProcessStateDeltas(cfg); // see e_states.c
   E_ProcessThingDeltas(cfg); // see e_things.c
   
   E_EDFLogPuts("Processing finished, freeing tables\n");

   // free unneeded hash tables and string arrays
   Z_Free(sprchains);
   Z_Free(sprnext);

   E_EDFLogPuts("Freeing cfg object\n");

   // free the config object
   cfg_free(cfg);

   // process separate optional lumps
   E_ProcessEDFLumps();

   // process last-chance defaults
   E_ProcessLastChance();

   // check heap integrity for safety
   E_EDFLogPuts("Checking zone heap integrity\n");
   Z_CheckHeap();

   // close the verbose log file
   E_EDFCloseVerboseLog();
}

// utility functions

//
// E_SpriteNumForName
//
// Sprite hashing function, valid only during EDF parsing. Returns
// the index of "name" in the sprnames array, if found. If not,
// returns NUMSPRITES.
//
int E_SpriteNumForName(const char *name)
{
   int sprnum;
   unsigned int sprkey = D_HashTableKey(name) % NUMSPRCHAINS;

   sprnum = sprchains[sprkey];
   while(sprnum != NUMSPRITES && strcasecmp(name, sprnames[sprnum]))
      sprnum = sprnext[sprnum];

   return sprnum;
}

// EOF

