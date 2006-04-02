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
// EDF
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
// * root.edf ..... this file includes the others, and is opened by
//                  E_ProcessEDF by default
// * sprites.edf .. includes the sprnames array and sprite-based
//                  pickup item definitions
// * sounds.edf ... contains the sfxinfo structures
// * frames.edf ... contains the states structures
// * things.edf ... contains the mobjinfo structures
// * cast.edf ..... contains DOOM II cast call definitions
// * misc.edf ..... miscellaneous stuff
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "w_wad.h"
#include "i_system.h"
#include "d_main.h"
#include "d_dehtbl.h"
#include "d_gi.h"
#include "d_io.h"
#include "doomdef.h"
#include "g_game.h"
#include "info.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "p_enemy.h"
#include "p_inter.h"
#include "p_pspr.h"
#include "p_partcl.h"
#include "f_finale.h"
#include "e_edf.h"
#include "e_sound.h"
#include "r_draw.h"
#include "m_qstr.h"

#include "Confuse/confuse.h"

#include <errno.h>

// stuff defined in e_sound.c that is intentionally not in the header:
extern cfg_opt_t sound_opts[];
extern cfg_opt_t sdelta_opts[];
void E_ProcessSound(sfxinfo_t *, cfg_t *, boolean);

// verbose logging, toggled with -edfout cmdline param
static FILE *edf_output = NULL;

// The "S_NULL" state, which is required, has its number resolved
// in E_CollectThings
static int NullStateNum;

// The "Unknown" thing type, which is required, has its type
// number resolved in E_CollectThings
static int UnknownThingType;

// EDF keywords

#define SEC_SPRITE "spritenames"

#define ITEM_PLAYERSPRITE "playersprite"
#define ITEM_BLANKSPRITE "blanksprite"

#define SEC_PICKUPFX "pickupitem"
#define ITEM_PICKUPFX "effect"

#define SEC_FRAME "frame"
#define ITEM_FRAME_SPRITE "sprite"
#define ITEM_FRAME_SPRFRAME "spriteframe"
#define ITEM_FRAME_FULLBRT "fullbright"
#define ITEM_FRAME_TICS "tics"
#define ITEM_FRAME_ACTION "action"
#define ITEM_FRAME_NEXTFRAME "nextframe"
#define ITEM_FRAME_MISC1 "misc1"
#define ITEM_FRAME_MISC2 "misc2"
#define ITEM_FRAME_PTCLEVENT "particle_event"
#define ITEM_FRAME_ARGS "args"
#define ITEM_FRAME_DEHNUM "dehackednum"
#define ITEM_FRAME_CMP "cmp"

#define SEC_FRAMEDELTA "framedelta"

#define SEC_THING "thingtype"
#define ITEM_TNG_INHERITS "inherits"
#define ITEM_TNG_DOOMEDNUM "doomednum"
#define ITEM_TNG_SPAWNSTATE "spawnstate"
#define ITEM_TNG_SPAWNHEALTH "spawnhealth"
#define ITEM_TNG_SEESTATE "seestate"
#define ITEM_TNG_SEESOUND "seesound"
#define ITEM_TNG_REACTTIME "reactiontime"
#define ITEM_TNG_ATKSOUND "attacksound"
#define ITEM_TNG_PAINSTATE "painstate"
#define ITEM_TNG_PAINCHANCE "painchance"
#define ITEM_TNG_PAINSOUND "painsound"
#define ITEM_TNG_MELEESTATE "meleestate"
#define ITEM_TNG_MISSILESTATE "missilestate"
#define ITEM_TNG_DEATHSTATE "deathstate"
#define ITEM_TNG_XDEATHSTATE "xdeathstate"
#define ITEM_TNG_DEATHSOUND "deathsound"
#define ITEM_TNG_SPEED "speed"
#define ITEM_TNG_RADIUS "radius"
#define ITEM_TNG_HEIGHT "height"
#define ITEM_TNG_MASS "mass"
#define ITEM_TNG_DAMAGE "damage"
#define ITEM_TNG_ACTIVESOUND "activesound"
#define ITEM_TNG_FLAGS "flags"
#define ITEM_TNG_FLAGS2 "flags2"
#define ITEM_TNG_RAISESTATE "raisestate"
#define ITEM_TNG_TRANSLUC "translucency"
#define ITEM_TNG_FLAGS3 "flags3"
#define ITEM_TNG_BLOODCOLOR "bloodcolor"
#define ITEM_TNG_FASTSPEED "fastspeed"
#define ITEM_TNG_NUKESPEC  "nukespecial"
#define ITEM_TNG_PARTICLEFX "particlefx"
#define ITEM_TNG_DROPTYPE "droptype"
#define ITEM_TNG_MOD      "mod"
#define ITEM_TNG_OBIT1  "obituary_normal"
#define ITEM_TNG_OBIT2  "obituary_melee"
#define ITEM_TNG_COLOR  "translation"
#define ITEM_TNG_CFLAGS "cflags"
#define ITEM_TNG_ADDFLAGS "addflags"
#define ITEM_TNG_REMFLAGS "remflags"
#define ITEM_TNG_DMGSPECIAL "dmgspecial"
#define ITEM_TNG_CRASHSTATE "crashstate"
#define ITEM_TNG_SKINSPRITE "skinsprite"
#define ITEM_TNG_DEHNUM "dehackednum"

#define SEC_THINGDELTA  "thingdelta"
#define ITEM_DELTA_NAME "name"

#define SEC_CAST "castinfo"
#define ITEM_CAST_TYPE "type"
#define ITEM_CAST_NAME "name"
#define ITEM_CAST_SA   "stopattack"
#define ITEM_CAST_SOUND "sound"
#define ITEM_CAST_SOUNDFRAME "frame"
#define ITEM_CAST_SOUNDNAME "sfx"

#define SEC_CASTORDER "castorder"

#define SEC_BOSSTYPES "boss_spawner_types"
#define SEC_BOSSPROBS "boss_spawner_probs"  // schepe

#define SEC_SOUND "sound"
#define SEC_SDELTA "sounddelta"

#define ITEM_D2TITLETICS "doom2_title_tics"

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

// particle effects flags

static dehflags_t particlefx[] =
{
   { "ROCKET",         FX_ROCKET },
   { "GRENADE",        FX_GRENADE },
   { "FLIES",          FX_FLIES },
   { "BFG",            FX_BFG },
   { "REDFOUNTAIN",    FX_REDFOUNTAIN },
   { "GREENFOUNTAIN",  FX_GREENFOUNTAIN },
   { "BLUEFOUNTAIN",   FX_BLUEFOUNTAIN },
   { "YELLOWFOUNTAIN", FX_YELLOWFOUNTAIN },
   { "PURPLEFOUNTAIN", FX_PURPLEFOUNTAIN },
   { "BLACKFOUNTAIN",  FX_BLACKFOUNTAIN },
   { "WHITEFOUNTAIN",  FX_WHITEFOUNTAIN },
   { NULL,             0 }
};

static dehflagset_t particle_flags =
{
   particlefx,  // flaglist
   0,           // mode
};

// special damage inflictor types
// currently searched linearly
// matching enum values in p_inter.h

const char *inflictorTypes[INFLICTOR_NUMTYPES] =
{
   "none",
   "MinotaurCharge",
   "Whirlwind",
};

// Note on all the below:
// For maximum efficiency, the number of chains in these hash tables
// should be a prime number at least as large as the number of
// elements to be contained in each respective table. Since things,
// sprites, and frames can be added, it is best to choose a number
// a bit larger, to allow more of a buffer for user-defined types.

// temporary hash tables (used only during EDF processing)

// sprite hashing
#define NUMSPRCHAINS 257
static int *sprchains = NULL;
static int *sprnext = NULL;

// permanent hash tables (needed for in-game lookups)
// * "next" fields are contained in the structures themselves
// * sound hashing is handled in e_sound.c

// state hashing
#define NUMSTATECHAINS 2003
static int state_namechains[NUMSTATECHAINS];
static int state_dehchains[NUMSTATECHAINS];

// thing hashing
#define NUMTHINGCHAINS 307
static int thing_namechains[NUMTHINGCHAINS];
static int thing_dehchains[NUMTHINGCHAINS];

// prototype of libConfuse parser inclusion function
extern int cfg_lexer_include(cfg_t *cfg, const char *filename);

// function prototypes for libConfuse callbacks (aka EDF functions)

static int edf_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv);

static int edf_stdinclude(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv);

static int bex_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv);

static int edf_ifenabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                         const char **argv);

static int edf_endif(cfg_t *cfg, cfg_opt_t *opt, int argc,
                     const char **argv);

static int edf_enable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                      const char **argv);

static int edf_disable(cfg_t *cfg, cfg_opt_t *opt, int argc,
                     const char **argv);

// EDF libConfuse option structures

// sprite-based pickup items

static cfg_opt_t pickup_opts[] =
{
   CFG_STR(ITEM_PICKUPFX, "PFX_NONE", CFGF_NONE),
   CFG_END()
};

// spriteframe value-parsing callback prototype
static int E_SpriteFrameCB(cfg_t *, cfg_opt_t *, const char *, void *);

// states

#define FRAME_FIELDS \
   CFG_STR(ITEM_FRAME_SPRITE,    "BLANK",     CFGF_NONE), \
   CFG_INT_CB(ITEM_FRAME_SPRFRAME,  0,        CFGF_NONE, E_SpriteFrameCB), \
   CFG_BOOL(ITEM_FRAME_FULLBRT,  cfg_false,   CFGF_NONE), \
   CFG_INT(ITEM_FRAME_TICS,      1,           CFGF_NONE), \
   CFG_STR(ITEM_FRAME_ACTION,    "NULL",      CFGF_NONE), \
   CFG_STR(ITEM_FRAME_NEXTFRAME, "S_NULL",    CFGF_NONE), \
   CFG_STR(ITEM_FRAME_MISC1,     "0",         CFGF_NONE), \
   CFG_STR(ITEM_FRAME_MISC2,     "0",         CFGF_NONE), \
   CFG_STR(ITEM_FRAME_PTCLEVENT, "pevt_none", CFGF_NONE), \
   CFG_STR(ITEM_FRAME_ARGS,      0,           CFGF_LIST), \
   CFG_INT(ITEM_FRAME_DEHNUM,    -1,          CFGF_NONE), \
   CFG_END()

static cfg_opt_t frame_opts[] =
{
   CFG_STR(ITEM_FRAME_CMP, 0, CFGF_NONE),
   FRAME_FIELDS
};

static cfg_opt_t fdelta_opts[] =
{
   CFG_STR(ITEM_DELTA_NAME, 0, CFGF_NONE),
   FRAME_FIELDS
};

// speed value-parsing callback prototype
static int E_SpeedCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                     void *result);

// translation value-parsing callback
static int E_ColorCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                     void *result);

// translucency value-parsing callback (10/04/04)
static int E_TranslucCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                        void *result);

// things

#define THINGTYPE_FIELDS \
   CFG_INT(ITEM_TNG_DOOMEDNUM,    -1,       CFGF_NONE), \
   CFG_STR(ITEM_TNG_SPAWNSTATE,   "S_NULL", CFGF_NONE), \
   CFG_INT(ITEM_TNG_SPAWNHEALTH,  1000,     CFGF_NONE), \
   CFG_STR(ITEM_TNG_SEESTATE,     "S_NULL", CFGF_NONE), \
   CFG_STR(ITEM_TNG_SEESOUND,     "none",   CFGF_NONE), \
   CFG_INT(ITEM_TNG_REACTTIME,    8,        CFGF_NONE), \
   CFG_STR(ITEM_TNG_ATKSOUND,     "none",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_PAINSTATE,    "S_NULL", CFGF_NONE), \
   CFG_INT(ITEM_TNG_PAINCHANCE,   0,        CFGF_NONE), \
   CFG_STR(ITEM_TNG_PAINSOUND,    "none",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_MELEESTATE,   "S_NULL", CFGF_NONE), \
   CFG_STR(ITEM_TNG_MISSILESTATE, "S_NULL", CFGF_NONE), \
   CFG_STR(ITEM_TNG_DEATHSTATE,   "S_NULL", CFGF_NONE), \
   CFG_STR(ITEM_TNG_XDEATHSTATE,  "S_NULL", CFGF_NONE), \
   CFG_STR(ITEM_TNG_DEATHSOUND,   "none",   CFGF_NONE), \
   CFG_INT_CB(ITEM_TNG_SPEED,     0,        CFGF_NONE, E_SpeedCB), \
   CFG_FLOAT(ITEM_TNG_RADIUS,     20.0f,    CFGF_NONE), \
   CFG_FLOAT(ITEM_TNG_HEIGHT,     16.0f,    CFGF_NONE), \
   CFG_INT(ITEM_TNG_MASS,         100,      CFGF_NONE), \
   CFG_INT(ITEM_TNG_DAMAGE,       0,        CFGF_NONE), \
   CFG_STR(ITEM_TNG_ACTIVESOUND,  "none",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_FLAGS,        "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_FLAGS2,       "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_RAISESTATE,   "S_NULL", CFGF_NONE), \
   CFG_INT_CB(ITEM_TNG_TRANSLUC,  65536,    CFGF_NONE, E_TranslucCB), \
   CFG_STR(ITEM_TNG_FLAGS3,       "",       CFGF_NONE), \
   CFG_INT(ITEM_TNG_BLOODCOLOR,   0,        CFGF_NONE), \
   CFG_INT_CB(ITEM_TNG_FASTSPEED, 0,        CFGF_NONE, E_SpeedCB), \
   CFG_STR(ITEM_TNG_NUKESPEC,     "NULL",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_PARTICLEFX,   "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_DROPTYPE,     "NONE",   CFGF_NONE), \
   CFG_INT(ITEM_TNG_MOD,          0,        CFGF_NONE), \
   CFG_STR(ITEM_TNG_OBIT1,        "NONE",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_OBIT2,        "NONE",   CFGF_NONE), \
   CFG_INT_CB(ITEM_TNG_COLOR,     0,        CFGF_NONE, E_ColorCB), \
   CFG_STR(ITEM_TNG_CFLAGS,       "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_ADDFLAGS,     "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_REMFLAGS,     "",       CFGF_NONE), \
   CFG_STR(ITEM_TNG_DMGSPECIAL,   "NONE",   CFGF_NONE), \
   CFG_STR(ITEM_TNG_CRASHSTATE,   "S_NULL", CFGF_NONE), \
   CFG_INT(ITEM_TNG_DEHNUM,       -1,       CFGF_NONE), \
   CFG_STR(ITEM_TNG_SKINSPRITE,   "noskin", CFGF_NONE), \
   CFG_END()

static cfg_opt_t thing_opts[] =
{
   CFG_STR(ITEM_TNG_INHERITS, 0, CFGF_NONE),
   THINGTYPE_FIELDS
};

static cfg_opt_t tdelta_opts[] =
{
   CFG_STR(ITEM_DELTA_NAME, 0, CFGF_NONE),
   THINGTYPE_FIELDS
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

// root cfg

static cfg_opt_t edf_opts[] =
{
   CFG_STR(SEC_SPRITE,        0,           CFGF_LIST),
   CFG_STR(ITEM_PLAYERSPRITE, "PLAY",      CFGF_NONE),
   CFG_STR(ITEM_BLANKSPRITE,  "TNT1",      CFGF_NONE),
   CFG_SEC(SEC_PICKUPFX,      pickup_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(SEC_SOUND,         sound_opts,  CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(SEC_FRAME,         frame_opts,  CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(SEC_THING,         thing_opts,  CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_SEC(SEC_CAST,          cast_opts,   CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_STR(SEC_CASTORDER,     0,           CFGF_LIST),
   CFG_STR(SEC_BOSSTYPES,     0,           CFGF_LIST),
   CFG_INT(SEC_BOSSPROBS,     0,           CFGF_LIST),  // schepe
   CFG_SEC(SEC_FRAMEDELTA,    fdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(SEC_THINGDELTA,    tdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_SEC(SEC_SDELTA,        sdelta_opts, CFGF_MULTI | CFGF_NOCASE),
   CFG_INT(ITEM_D2TITLETICS,  0,           CFGF_NONE),
   CFG_FUNC("include",        edf_include),
   CFG_FUNC("stdinclude",     edf_stdinclude),
   CFG_FUNC("bexinclude",     bex_include),
   CFG_FUNC("ifenabled",      edf_ifenabled),
   CFG_FUNC("endif",          edf_endif),
   CFG_FUNC("enable",         edf_enable),
   CFG_FUNC("disable",        edf_disable),
   CFG_END()
};

// Default opt arrays -- these are for detection and correction
// of missing required definitions. They rely on the documented
// structure and presence of the default EDF files.

// Options for stuff in sprites.edf only.
static cfg_opt_t sprite_only_opts[] =
{
   CFG_STR(SEC_SPRITE,   0,           CFGF_LIST),
   CFG_SEC(SEC_PICKUPFX, pickup_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_END()
};

// Options for stuff in frames.edf only.
static cfg_opt_t frame_only_opts[] =
{
   CFG_SEC(SEC_FRAME, frame_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_END()
};

// Options for stuff in things.edf only.
static cfg_opt_t thing_only_opts[] =
{
   CFG_SEC(SEC_THING, thing_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_END()
};

// Options for stuff in cast.edf only
static cfg_opt_t cast_only_opts[] =
{
   CFG_SEC(SEC_CAST,      cast_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_STR(SEC_CASTORDER, 0,         CFGF_LIST),
   CFG_END()
};

// Options for stuff in sounds.edf only
static cfg_opt_t sound_only_opts[] =
{
   CFG_SEC(SEC_SOUND, sound_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NOCASE),
   CFG_END()
};

//
// Callback functions
//

//
// edf_error
//
// This function is given to all cfg_t structures as the error
// callback.
//
static void edf_error(cfg_t *cfg, const char *fmt, va_list ap)
{
   if(edf_output)
      fputs("Exiting due to parser error\n", edf_output);

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
// edf_include
//
// The normal EDF include function. cfg_include is insufficient
// since it looks in the current working directory unless provided
// a full path. This function interprets paths relative to the 
// current file.
//
static int edf_include(cfg_t *cfg, cfg_opt_t *opt, int argc,
                       const char **argv)
{
   char currentpath[PATH_MAX + 1];
   char filename[PATH_MAX + 1];

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to include()");
      return 1;
   }
   if(!cfg->filename)
   {
      cfg_error(cfg, "include: cfg_t filename is undefined");
      return 1;
   }

   M_GetFilePath(cfg->filename, currentpath, sizeof(currentpath));
   psnprintf(filename, sizeof(filename), "%s/%s", currentpath, argv[0]);
   NormalizeSlashes(filename);

   return cfg_lexer_include(cfg, filename);
}

//
// E_BuildDefaultFn
//
// Constructs the absolute file name for a default EDF file.
// Don't cache the returned pointer, since it points to a static
// buffer.
//
static const char *E_BuildDefaultFn(const char *filename)
{
   static char buffer[PATH_MAX + 1];

   psnprintf(buffer, sizeof(buffer), "%s/%s",
             D_DoomExeDir(), filename);
   NormalizeSlashes(buffer);

   return buffer;
}

//
// edf_stdinclude
//
// An EDF include function that looks for files in the EXE's
// directory, as opposed to the current directory.
//
static int edf_stdinclude(cfg_t *cfg, cfg_opt_t *opt, int argc,
                          const char **argv)
{
   const char *filename;

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to stdinclude()");
      return 1;
   }

   filename = E_BuildDefaultFn(argv[0]);

   return cfg_lexer_include(cfg, filename);
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

typedef struct edf_enable_s
{
   const char *name;
   int enabled;
} edf_enable_t;

//
// The enables values table -- this can be fiddled with by the
// game engine using E_EDFSetEnableValue below.
//
static edf_enable_t enables[] =
{
   // all game modes are enabled by default
   { "DOOM",    1 },
   { "HERETIC", 1 },
   { NULL },
};

//
// E_EnableNumForName
//
// Gets the index of an enable value. Linear search on a
// small fixed set.
//
static int E_EnableNumForName(const char *name)
{
   int i = 0;

   while(enables[i].name)
   {
      if(!strcasecmp(enables[i].name, name))
         return i;

      ++i;
   }

   return -1;
}

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
   int idx = E_EnableNumForName(name);

   if(idx != -1)
      enables[idx].enabled = value;
}

//
// edf_ifenabled
//
// haleyjd 01/14/04: Causes the parser to skip forward, looking 
// for the next endif function and then calling it, if the 
// parameter isn't defined. I hacked the support for this
// into libConfuse without too much ugliness.
//
static int edf_ifenabled(cfg_t *cfg, cfg_opt_t *opt, int argc,
                         const char **argv)
{
   int idx;

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to ifenabled()");
      return 1;
   }

   if((idx = E_EnableNumForName(argv[0])) == -1)
   {
      cfg_error(cfg, "invalid enable value '%s'", argv[0]);
      return 1;
   }

   if(!enables[idx].enabled)
   {
      // force libConfuse to look for an endif function
      cfg->flags |= CFGF_LOOKFORFUNC;
      cfg->lookfor = "endif";
   }

   return 0;
}

//
// edf_endif
//
// 01/14/04: Returns the parser to normal after an ifenabled.
//
static int edf_endif(cfg_t *cfg, cfg_opt_t *opt, int argc,
                     const char **argv)
{
   cfg->flags &= ~CFGF_LOOKFORFUNC;
   cfg->lookfor = NULL;

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

   if((idx = E_EnableNumForName(argv[0])) == -1)
   {
      cfg_error(cfg, "unknown enable value '%s'", argv[0]);
      return 1;
   }

   enables[idx].enabled = 1;
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

   if((idx = E_EnableNumForName(argv[0])) == -1)
   {
      cfg_error(cfg, "unknown enable value '%s'", argv[0]);
      return 1;
   }

   enables[idx].enabled = 0;
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
      if(edf_output)
         fprintf(edf_output, "cfg_parse failed with code %d\n", err);

      I_Error("E_ParseEDFFile: failed to parse EDF (code %d)\n", err);
   }

   return cfg;
}

// These are needed by E_ProcessSprites:
static void E_TryDefaultSprites(void);
static void E_ProcessItems(cfg_t *);
static int  E_SpriteNumForName(const char *name);

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

   if(edf_output)
      fputs("\t* Processing sprites\n", edf_output);

   // set NUMSPRITES and allocate tables
   NUMSPRITES = cfg_size(cfg, SEC_SPRITE);

   if(edf_output)
      fprintf(edf_output, "\t\t%d sprite name(s) defined\n", NUMSPRITES);

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
         if(edf_output)
         {
            fprintf(edf_output, "\t\tInvalid sprite mnemonic: %s\n", 
                    sprname);
         }

         I_Error("E_ProcessSprites: invalid sprite name length: %s", 
                 sprname);
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

   if(edf_output)
   {
      fprintf(edf_output, 
              "\t\tFirst sprite = %s\n\t\tLast sprite = %s\n",
              sprnames[0], sprnames[NUMSPRITES-1]);
   }

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

   if(edf_output)
      fputs("\t\tAttempting to load default sprites.edf\n", edf_output);

   sprfn = E_BuildDefaultFn("sprites.edf");

   sprcfg = E_ParseEDFFile(sprfn, sprite_only_opts);

   // Test NUMSPRITES again -- if it's still zero, fatal error time.
   NUMSPRITES = cfg_size(sprcfg, SEC_SPRITE);

   if(!NUMSPRITES)
   {
      if(edf_output)
         fputs("\t\tError: no default sprites exist!\n", edf_output);
    
      I_Error("E_TryDefaultSprites: missing default sprites.\n");
   }

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

   if(edf_output)
      fputs("\t* Processing sprite variables\n", edf_output);

   // load player and blank sprite numbers
   str = cfg_getstr(cfg, ITEM_PLAYERSPRITE);
   sprnum = E_SpriteNumForName(str);
   if(sprnum == NUMSPRITES)
   {
      if(edf_output)
      {
         fprintf(edf_output, "\t\tInvalid player sprite name: %s\n", 
                 str);
      }

      I_Error("E_ProcessSpriteVars: invalid player sprite name: %s\n", 
              str);
   }
   if(edf_output)
   {
      fprintf(edf_output, "\t\tSet sprite %s(#%d) as player sprite\n",
              str, sprnum);
   }
   playerSpriteNum = sprnum;

   str = cfg_getstr(cfg, ITEM_BLANKSPRITE);
   sprnum = E_SpriteNumForName(str);
   if(sprnum == NUMSPRITES)
   {
      if(edf_output)
      {
         fprintf(edf_output, "\t\tInvalid blank sprite name: %s\n", str);
      }

      I_Error("E_ProcessSpriteVars: invalid blank sprite name: %s\n", str);
   }
   if(edf_output)
   {
      fprintf(edf_output, "\t\tSet sprite %s(#%d) as blank sprite\n",
              str, sprnum);
   }
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

   if(edf_output)
      fputs("\t* Processing pickup items\n", edf_output);

   // allocate and initialize pickup effects array
   pickupfx  = Z_Malloc(NUMSPRITES * sizeof(int), PU_STATIC, 0);
   
   for(i = 0; i < NUMSPRITES; ++i)
      pickupfx[i] = PFX_NONE;
   
   // load pickupfx
   numpickups = cfg_size(cfg, SEC_PICKUPFX);
   if(edf_output)
      fprintf(edf_output, "\t\t%d pickup item(s) defined\n", numpickups);
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
         if(edf_output)
         {
            fprintf(edf_output, 
                    "\t\tInvalid sprite mnemonic for pickup item: %s",
                    title);
         }

         I_Error("E_ProcessItems: invalid sprite mnemonic for pickup item: %s\n", title);
      }

      // find the proper pickup effect number (linear search)
      fxnum = 0;
      while(fxnum != PFX_NUMFX && strcasecmp(pickupnames[fxnum], pfx))
         fxnum++;

      if(fxnum == PFX_NUMFX)
      {
         if(edf_output)
         {
            fprintf(edf_output, "\t\tInvalid pickup effect: %s\n", pfx);
         }

         I_Error("E_ProcessItems: invalid pickup effect: %s\n", pfx);
      }
      if(edf_output)
      {
         fprintf(edf_output,
            "\t\tSet sprite %s(#%d) to pickup effect %s(#%d)\n",
            title, sprnum, pfx, fxnum);
      }

      pickupfx[sprnum] = fxnum;
   }
}

// haleyjd 09/03/03: E_BuildSoundHash removed

static void E_TryDefaultSounds(void);

//
// E_ProcessSounds
//
// Collects all the sound definitions and builds the sound hash
// tables.
//
static void E_ProcessSounds(cfg_t *cfg)
{
   int i;

   if(edf_output)
      fputs("\t* Processing sound definitions\n", edf_output);

   // find out how many sounds are defined
   NUMSFX = cfg_size(cfg, SEC_SOUND);

   // try defaults if zero
   if(!NUMSFX)
   {
      if(edf_output)
         fputs("\t\tNo sounds defined, trying defaults\n", edf_output);

      E_TryDefaultSounds();
      return;
   }

   if(edf_output)
      fprintf(edf_output, "\t\t%d sound(s) defined\n", NUMSFX);

   // add one to make room for S_sfx[0]
   ++NUMSFX;

   // let's allocate & initialize the sounds...
   S_sfx = malloc(NUMSFX * sizeof(sfxinfo_t));
   memset(S_sfx, 0, NUMSFX * sizeof(sfxinfo_t));

   if(edf_output)
      fputs("\t\tHashing sounds\n", edf_output);

   // initialize S_sfx[0]
   strcpy(S_sfx[0].name, "none");
   strcpy(S_sfx[0].mnemonic, "none");

   // now, let's collect the mnemonics (this must be done ahead of time)
   for(i = 1; i < NUMSFX; ++i)
   {
      const char *mnemonic;
      cfg_t *sndsection = cfg_getnsec(cfg, SEC_SOUND, i - 1);

      mnemonic = cfg_title(sndsection);

      // verify the length
      if(strlen(mnemonic) > 16)
      {
         if(edf_output)
         {
            fprintf(edf_output, "\t\tError: invalid sound mnemonic %s\n", 
                    mnemonic);
         }

         I_Error("E_ProcessSounds: invalid sound mnemonic %s\n", mnemonic);
      }

      // copy it to the sound
      strncpy(S_sfx[i].mnemonic, mnemonic, 17);

      // add this sound to the hash table
      E_AddSoundToHash(&S_sfx[i]);
   }

   if(edf_output)
      fputs("\t\tProcessing data\n", edf_output);

   // finally, process the individual sounds
   for(i = 1; i < NUMSFX; ++i)
   {
      cfg_t *section = cfg_getnsec(cfg, SEC_SOUND, i - 1);

      E_ProcessSound(&S_sfx[i], section, true);

      if(edf_output)
         fprintf(edf_output, "\t\tFinished sound %s(#%d)\n",
                 S_sfx[i].mnemonic, i);
   }

   if(edf_output)
      fputs("\t\tFinished sound processing\n", edf_output);
}

//
// E_TryDefaultSounds
//
// Tries to load the default sounds.edf file.
//
static void E_TryDefaultSounds(void)
{
   cfg_t *soundcfg;
   const char *soundfn;

   if(edf_output)
      fputs("\t\tAttempting to load defaults from sounds.edf\n", edf_output);

   soundfn = E_BuildDefaultFn("sounds.edf");

   soundcfg = E_ParseEDFFile(soundfn, sound_only_opts);

   NUMSFX = cfg_size(soundcfg, SEC_SOUND);

   if(!NUMSFX)
   {
      if(edf_output)
         fputs("\t\tError: no default sounds exist!\n", edf_output);

      I_Error("E_TryDefaultSounds: missing default sounds.\n");
   }

   E_ProcessSounds(soundcfg);

   cfg_free(soundcfg);
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
   int i;

   if(edf_output)
      fputs("\t* Allocating states and things\n", edf_output);

   if(edf_output)
      fprintf(edf_output, "\t\tNUMSTATES = %d, NUMMOBJTYPES = %d\n",
              NUMSTATES, NUMMOBJTYPES);

   // allocate arrays
   states   = Z_Malloc(sizeof(state_t)*NUMSTATES, PU_STATIC, NULL);
   mobjinfo = Z_Malloc(sizeof(mobjinfo_t)*NUMMOBJTYPES, PU_STATIC, NULL);

   // initialize hash slots

   for(i = 0; i < NUMSTATECHAINS; ++i)
   {
      state_namechains[i] = state_dehchains[i] = NUMSTATES;
   }
   for(i = 0; i < NUMTHINGCHAINS; ++i)
   {
      thing_namechains[i] = thing_dehchains[i] = NUMMOBJTYPES;
   }

   // build hash tables
   if(edf_output)
      fputs("\t* Building state/thing hash tables\n", edf_output);

   // states
   for(i = 0; i < NUMSTATES; ++i)
   {
      unsigned int key;
      cfg_t *statecfg = cfg_getnsec(scfg, SEC_FRAME, i);
      const char *name = cfg_title(statecfg);
      int tempint;

      // verify length
      if(strlen(name) > 40)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tFrame mnemonic %s too long\n", name);

         I_Error("E_CollectNames: invalid frame mnemonic '%s'\n", name);
      }

      // copy it to the state
      memset(states[i].name, 0, 41);
      strncpy(states[i].name, name, 41);

      // hash it
      key = D_HashTableKey(name) % NUMSTATECHAINS;
      states[i].namenext = state_namechains[key];
      state_namechains[key] = i;

      // process dehackednum and add state to dehacked hash table,
      // if appropriate
      tempint = cfg_getint(statecfg, ITEM_FRAME_DEHNUM);
      states[i].dehnum = tempint;
      if(tempint != -1)
      {
         int dehkey = tempint % NUMSTATECHAINS;
         
         // make sure it doesn't exist yet
         if(E_StateNumForDEHNum(tempint) != NUMSTATES)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tFrame %d has duplicate deh num %d\n",
                       i, tempint);

            I_Error("E_CollectNames: frame %d: duplicate deh num %d\n",
                    i, tempint);
         }
         
         states[i].dehnext = state_dehchains[dehkey];
         state_dehchains[dehkey] = i;
      }
   }

   // things
   for(i = 0; i < NUMMOBJTYPES; ++i)
   {
      unsigned int key;
      cfg_t *thingcfg = cfg_getnsec(tcfg, SEC_THING, i);
      const char *name = cfg_title(thingcfg);
      int tempint;

      // verify length
      if(strlen(name) > 40)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tThing mnemonic %s too long\n", name);

         I_Error("E_CollectNames: invalid thing mnemonic '%s'\n", name);
      }

      // copy it to the thing
      memset(mobjinfo[i].name, 0, 41);
      strncpy(mobjinfo[i].name, name, 41);

      // hash it
      key = D_HashTableKey(name) % NUMTHINGCHAINS;
      mobjinfo[i].namenext = thing_namechains[key];
      thing_namechains[key] = i;

      // process dehackednum and add thing to dehacked hash table,
      // if appropriate
      tempint = cfg_getint(thingcfg, ITEM_TNG_DEHNUM);
      mobjinfo[i].dehnum = tempint;
      if(tempint != -1)
      {
         int dehkey = tempint % NUMTHINGCHAINS;
         
         // make sure it doesn't exist yet
         if(E_ThingNumForDEHNum(tempint) != NUMMOBJTYPES)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tThing %d has duplicate deh num %d\n",
                       i, tempint);
            
            I_Error("E_CollectNames: thing %d: duplicate deh num %d\n",
                    i, tempint);
         }
   
         mobjinfo[i].dehnext = thing_dehchains[dehkey];
         thing_dehchains[dehkey] = i;
      }
   }

   // verify the existence of the S_NULL frame
   NullStateNum = E_StateNumForName("S_NULL");
   if(NullStateNum == NUMSTATES)
      I_Error("E_CollectNames: 'S_NULL' frame must be defined!\n");

   // verify the existence of the Unknown thing type
   UnknownThingType = E_ThingNumForName("Unknown");
   if(UnknownThingType == NUMMOBJTYPES)
      I_Error("E_CollectNames: 'Unknown' thing type must be defined!\n");
}

// frame field parsing routines

//
// E_StateSprite
//
// Isolated code to process the frame sprite field.
//
static void E_StateSprite(const char *tempstr, int i)
{
   // check for special 'BLANK' identifier
   if(!strcasecmp(tempstr, "BLANK"))
      states[i].sprite = blankSpriteNum;
   else
   {
      // resolve normal sprite name
      int sprnum = E_SpriteNumForName(tempstr);
      if(sprnum == NUMSPRITES)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tFrame %d: Invalid sprite %s\n", 
                    i, tempstr);
         
         I_Error("E_ProcessState: frame %d: invalid sprite name %s\n", 
                 i, tempstr);
      }
      states[i].sprite = sprnum;
   }
}

//
// E_SpriteFrameCB
//
// libConfuse value-parsing callback function for the spriteframe
// field of the frame section. Allows use of characters A through ]
// corresponding to the actual sprite lump names (implemented by 
// popular demand ;)
//
// This function is also called explicitly by E_ProcessCmpState.
// When this is done, the cfg and opt parameters are set to NULL,
// and will not be used.
//
static int E_SpriteFrameCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                           void *result)
{
   if(strlen(value) == 1 && value[0] >= 'A' && value[0] <= ']')
   {
      *(long *)result = value[0] - 'A';
   }
   else
   {
      char *endptr;

      *(long *)result = strtol(value, &endptr, 0);
      
      if(*endptr != '\0')
      {
         if(cfg)
         {
            cfg_error(cfg, "invalid integer value for option '%s'",
                      opt->name);
         }
         return -1;
      }
      if(errno == ERANGE) 
      {
         if(cfg)
         {
            cfg_error(cfg,
               "integer value for option '%s' is out of range",
               opt->name);
         }
         return -1;
      }
   }

   return 0;
}

//
// E_StateAction
//
// Isolated code to process the frame action field.
//
static void E_StateAction(const char *tempstr, int i)
{
   deh_bexptr *dp = D_GetBexPtr(tempstr);
   
   if(!dp)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tFrame %d: bad action %s\n", i, tempstr);
      
      I_Error("E_ProcessState: frame %d: bad action %s\n", i, tempstr);
   }

   states[i].action = dp->cptr;
}

//
// E_ParseMiscField
//
// This function implements the quite powerful prefix:value syntax
// for misc and args fields in frames. Some of the code within may
// be candidate for generalization, since other fields may need
// this syntax in the near future.
//
static void E_ParseMiscField(const char *value, long *target)
{
   int i;
   char prefix[16];
   const char *colonloc;
   const char *rover, *strval;
   
   memset(prefix, 0, 16);

   // look for a colon ending a possible prefix
   colonloc = strchr(value, ':');
   
   if(colonloc)
   {
      // a colon was found, so extract and identify the prefix
      strval = colonloc + 1;
      rover = value;
      i = 0;
      while(rover != colonloc && i < 15) // leave room for \0
      {
         prefix[i] = *rover;
         ++rover;
         ++i;
      }

      // check validity of the string value location (could be end)
      if(!(*strval))
      {
         if(edf_output)
            fprintf(edf_output, "\t\tInvalid misc value %s\n", value);

         I_Error("E_ParseMiscField: invalid misc value %s\n", value);
      }

      if(!strcasecmp(prefix, "frame"))
      {
         int framenum = E_StateNumForName(strval);
         if(framenum == NUMSTATES)
         {
            I_Error("E_ParseMiscField: invalid state name %s\n", strval);
         }
         // 09/19/03: add check for no dehacked number
         if(states[framenum].dehnum == -1)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tWarning: frame %s has no DeHackEd number\n",
                       strval);
            *target = NullStateNum;
         }
         else
            *target = states[framenum].dehnum;
         return;
      }
      if(!strcasecmp(prefix, "thing"))
      {
         int thingnum = E_ThingNumForName(strval);
         if(thingnum == NUMMOBJTYPES)
         {
            I_Error("E_ParseMiscField: invalid thing name %s\n", strval);
         }
         // 09/19/03: add check for no dehacked number
         if(mobjinfo[thingnum].dehnum == -1)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tWarning: thing %s has no DeHackEd number\n",
                       strval);
            *target = UnknownThingType;
         }
         else
            *target = mobjinfo[thingnum].dehnum;
         return;
      }
      if(!strcasecmp(prefix, "sound"))
      {
         sfxinfo_t *sfx = E_EDFSoundForName(strval);
         if(!sfx)
         {
            I_Error("E_ParseMiscField: invalid sound name %s\n", strval);
         }         
         if(sfx->dehackednum == -1)
         {
            // print a warning in this case, and set the sound to zero
            if(edf_output)
               fprintf(edf_output, "\t\tWarning: sound %s has no DeHackEd number\n",
                       sfx->mnemonic);
            *target = 0;
         }
         else
            *target = sfx->dehackednum;         
         return;
      }
      if(!strcasecmp(prefix, "flags"))
      {
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE1);
         return;
      }
      if(!strcasecmp(prefix, "flags2"))
      {
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE2);
         return;
      }
      if(!strcasecmp(prefix, "flags3"))
      {
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE3);
         return;
      }
      if(!strcasecmp(prefix, "bexptr"))
      {
         deh_bexptr *dp = D_GetBexPtr(strval);
         
         if(!dp)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tBad bexptr %s\n", strval);
            
            I_Error("E_ParseMiscField: bad bexptr %s\n", strval);
         }

         // get the index of this deh_bexptr in the master
         // deh_bexptrs array, and store it in the arg field
         *target = dp - deh_bexptrs;
         return;
      }

      I_Error("E_ParseMiscField: unknown value prefix %s\n", prefix);
   }

   // must just be an integer value
   // 11/11/03: use strtol to support hex and oct input
   *target = strtol(value, NULL, 0);
}

//
// E_SpecialNextState
//
// 11/07/03:
// Returns a frame number for a special nextframe value.
//
static int E_SpecialNextState(const char *string, int framenum)
{
   int nextnum = 0;
   const char *value = string + 1;

   if(!strcasecmp(value, "next"))
   {
      if(framenum == NUMSTATES - 1) // can't do it
      {
         if(edf_output)
         {
            fprintf(edf_output, 
               "\t\tError: special next field references invalid frame #%d\n",
               NUMSTATES);
         }

         I_Error("E_SpecialNextState: invalid frame #%d\n", NUMSTATES);
      }
      nextnum = framenum + 1;
   }
   else if(!strcasecmp(value, "prev"))
   {
      if(framenum == 0) // can't do it
      {
         if(edf_output)
         {
            fputs(
               "\t\tError: special next field references invalid frame -1\n",
               edf_output);
         }

         I_Error("E_SpecialNextState: invalid frame -1\n");
      }
      nextnum = framenum - 1;
   }
   else if(!strcasecmp(value, "this"))
   {
      nextnum = framenum;
   }
   else if(!strcasecmp(value, "null"))
   {
      nextnum = NullStateNum;
   }
   else
   {
      if(edf_output)
      {
         fprintf(edf_output,
            "\t\tError: special next field has invalid specifier %s\n",
            value);
      }
      
      I_Error("E_SpecialNextState: invalid specifier %s\n", value);
   }

   return nextnum;
}

//
// E_StateNextFrame
//
// Isolated code to process the frame nextframe field.
//
static void E_StateNextFrame(const char *tempstr, int i)
{
   int tempint = 0;

   // 11/07/03: allow special values in the nextframe field
   if(tempstr[0] == '@')
   {
      tempint = E_SpecialNextState(tempstr, i);
   }
   else if((tempint = E_StateNumForName(tempstr)) == NUMSTATES)
   {
      // check for DeHackEd num specification
      if(!strncasecmp(tempstr, "dehnum:", 7) && strlen(tempstr) > 7)
      {
         // use strtol on the remaining portion of the string;
         // the resulting value must be a valid frame deh number
         tempint = E_GetStateNumForDEHNum(strtol(tempstr + 7, NULL, 0));
      }
      else
      {
         // error
         if(edf_output)
            fprintf(edf_output, "\t\tFrame %d: bad nextframe %s\n",
                    i, tempstr);
      
         I_Error("E_ProcessState: frame %d: bad nextframe %s\n",
                 i, tempstr);
      }
   }

   states[i].nextstate = tempint;
}

//
// E_StatePtclEvt
//
// Isolated code to process the frame particle event field.
//
static void E_StatePtclEvt(const char *tempstr, int i)
{
   int tempint = 0;

   while(tempint != P_EVENT_NUMEVENTS &&
         strcasecmp(tempstr, particleEvents[tempint].name))
   {
      ++tempint;
   }
   if(tempint == P_EVENT_NUMEVENTS)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tFrame %d: bad ptclevent %s\n",
                 i, tempstr);
      
      I_Error("E_ProcessState: frame %d: bad ptclevent %s\n",
              i, tempstr);
   }

   states[i].particle_evt = tempint;
}

//
// E_CmpTokenizer
//
// haleyjd 06/24/04:
// A lexer function for the frame cmp field.
// Used by E_ProcessCmpState below.
//
static const char *E_CmpTokenizer(const char *text, int *index, qstring_t *token)
{
   char c;
   int state = 0;

   // if we're already at the end, return NULL
   if(text[*index] == '\0')
      return NULL;

   M_QStrClear(token);

   while((c = text[*index]) != '\0')
   {
      *index += 1;
      switch(state)
      {
      case 0: // default state
         switch(c)
         {
         case ' ':
         case '\t':
            continue;  // skip whitespace
         case '"':
            state = 1; // enter quoted part
            continue;
         case '|':     // end of current token
            return M_QStrBuffer(token);
         default:      // everything else == part of value
            M_QStrPutc(token, c);
            continue;
         }
      case 1: // in quoted area
         if(c == '"') // end of quoted area
            state = 0;
         else
            M_QStrPutc(token, c); // everything inside is literal
         continue;
      default:
         I_Error("E_CmpTokenizer: internal error - undefined lexer state\n");
      }
   }

   // return final token, next call will return NULL
   return M_QStrBuffer(token);
}

// macros for E_ProcessCmpState:

// NEXTTOKEN: calls E_CmpTokenizer to get the next token

#define NEXTTOKEN() curtoken = E_CmpTokenizer(value, &tok_index, &buffer)

// DEFAULTS: tests if the string value is either NULL or equal to "*"

#define DEFAULTS(value)  (!(value) || (value)[0] == '*')

//
// E_ProcessCmpState
//
// Code to process a compressed state definition. Compressed state
// definitions are just a string with each frame field in a set order,
// delimited by pipes. This is very similar to DDF's frame specification,
// and has been requested by multiple users.
//
// Compressed format:
// "sprite|spriteframe|fullbright|tics|action|nextframe|ptcl|misc|args"
//
// Fields at the end can be left off. "*" in a field means to use
// the normal default value.
//
// haleyjd 06/24/04: rewritten to use a finite-state-automaton lexer, 
// making the format MUCH more flexible than it was under the former 
// strtok system. The E_CmpTokenizer function above performs the 
// lexing, returning each token in the qstring provided to it.
//
static void E_ProcessCmpState(const char *value, int i)
{
   qstring_t buffer;
   const char *curtoken = NULL;
   int tok_index = 0, j;

   // first things first, we have to initialize the qstring
   M_QStrInitCreate(&buffer);

   // process sprite
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].sprite = blankSpriteNum;
   else
      E_StateSprite(curtoken, i);

   // process spriteframe
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].frame = 0;
   else
   {
      // call the value-parsing callback explicitly
      if(E_SpriteFrameCB(NULL, NULL, curtoken, &(states[i].frame)) == -1)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tFrame %d: bad spriteframe %s\n",
                    i, curtoken);

         I_Error("E_ProcessCmpState: frame %d: bad spriteframe %s\n",
                 i, curtoken);
      }
   }

   // process fullbright
   NEXTTOKEN();
   if(DEFAULTS(curtoken) == 0)
   {
      if(curtoken[0] == 't' || curtoken[0] == 'T')
         states[i].frame |= FF_FULLBRIGHT;
   }

   // process tics
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].tics = 1;
   else
      states[i].tics = strtol(curtoken, NULL, 0);

   // process action
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].action = NULL;
   else
      E_StateAction(curtoken, i);

   // process nextframe
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].nextstate = NullStateNum;
   else
      E_StateNextFrame(curtoken, i);

   // process particle event
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].particle_evt = 0;
   else
      E_StatePtclEvt(curtoken, i);

   // process misc1, misc2
   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].misc1 = 0;
   else
      E_ParseMiscField(curtoken, &(states[i].misc1));

   NEXTTOKEN();
   if(DEFAULTS(curtoken))
      states[i].misc2 = 0;
   else
      E_ParseMiscField(curtoken, &(states[i].misc2));

   // process args
   for(j = 0; j < 5; ++j) // hard-coded args max
   {
      NEXTTOKEN();
      if(DEFAULTS(curtoken))
         states[i].args[j] = 0;
      else
         E_ParseMiscField(curtoken, &(states[i].args[j]));
   }

   // free the qstring
   M_QStrFree(&buffer);
}

#undef NEXTTOKEN
#undef DEFAULTS

// IS_SET: this macro tests whether or not a particular field should
// be set. When applying deltas, we should not retrieve defaults.

#undef  IS_SET
#define IS_SET(name) (def || cfg_size(framesec, (name)) > 0)

//
// E_ProcessState
//
// Generalized code to process the data for a single state
// structure. Doubles as code for frame and framedelta.
//
static void E_ProcessState(int i, cfg_t *framesec, boolean def)
{
   int j;
   int tempint;
   const char *tempstr;

   // 11/14/03:
   // In definitions only, see if the cmp field is defined. If so,
   // we go into it with E_ProcessCmpState above, and ignore any
   // other fields defined in the frame block.
   if(def)
   {
      if(cfg_size(framesec, ITEM_FRAME_CMP) > 0)
      {
         tempstr = cfg_getstr(framesec, ITEM_FRAME_CMP);
         
         E_ProcessCmpState(tempstr, i);
         return;
      }
   }

   // process sprite
   if(IS_SET(ITEM_FRAME_SPRITE))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_SPRITE);

      E_StateSprite(tempstr, i);
   }

   // process spriteframe
   if(IS_SET(ITEM_FRAME_SPRFRAME))
      states[i].frame = cfg_getint(framesec, ITEM_FRAME_SPRFRAME);

   // check for fullbright
   if(IS_SET(ITEM_FRAME_FULLBRT))
   {
      if(cfg_getbool(framesec, ITEM_FRAME_FULLBRT) == cfg_true)
         states[i].frame |= FF_FULLBRIGHT;
   }

   // process tics
   if(IS_SET(ITEM_FRAME_TICS))
      states[i].tics = cfg_getint(framesec, ITEM_FRAME_TICS);

   // resolve codepointer
   if(IS_SET(ITEM_FRAME_ACTION))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_ACTION);

      E_StateAction(tempstr, i);
   }

   // process nextframe
   if(IS_SET(ITEM_FRAME_NEXTFRAME))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_NEXTFRAME);
      
      E_StateNextFrame(tempstr, i);
   }

   // process particle event
   if(IS_SET(ITEM_FRAME_PTCLEVENT))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_PTCLEVENT);

      E_StatePtclEvt(tempstr, i);
   }
      
   // misc field parsing (complicated)

   if(IS_SET(ITEM_FRAME_MISC1))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_MISC1);
      E_ParseMiscField(tempstr, &(states[i].misc1));
   }

   if(IS_SET(ITEM_FRAME_MISC2))
   {
      tempstr = cfg_getstr(framesec, ITEM_FRAME_MISC2);
      E_ParseMiscField(tempstr, &(states[i].misc2));
   }

   // args field parsing (even more complicated, but similar)
   // Note: deltas can only set the entire args list at once, not
   // just parts of it.
   if(IS_SET(ITEM_FRAME_ARGS))
   {
      tempint = cfg_size(framesec, ITEM_FRAME_ARGS);
      for(j = 0; j < 5; ++j) // hard-coded args max
         states[i].args[j] = 0;
      for(j = 0; j < tempint && j < 5; ++j) // hard-coded args max
      {
         tempstr = cfg_getnstr(framesec, ITEM_FRAME_ARGS, j);
         E_ParseMiscField(tempstr, &(states[i].args[j]));
      }
   }
}

//
// E_ProcessStates
//
// Resolves and loads all information for the state_t structures.
//
static void E_ProcessStates(cfg_t *cfg)
{
   int i;

   if(edf_output)
      fputs("\t* Processing frame data\n", edf_output);

   for(i = 0; i < NUMSTATES; ++i)
   {
      cfg_t *framesec = cfg_getnsec(cfg, SEC_FRAME, i);

      E_ProcessState(i, framesec, true);

      if(edf_output)
         fprintf(edf_output, "\t\tFinished frame %s(#%d)\n",
                 states[i].name, i);
   }
}

//
// E_ThingSound
//
// Does sound name lookup & verification and then stores the resulting
// sound DeHackEd number into *target.
//
static void E_ThingSound(const char *data, const char *fieldname, 
                         int thingnum, int *target)
{
   sfxinfo_t *sfx;

   if((sfx = E_EDFSoundForName(data)) == NULL)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tThing %d: invalid %s %s\n",
                 thingnum, fieldname, data);
      
      I_Error("E_ThingSound: thing %d: invalid %s %s\n",
              thingnum, fieldname, data);
   }
   
   if(sfx->dehackednum == -1)
   {
      // print a warning and set the sound to zero
      if(edf_output)
         fprintf(edf_output, "\t\tWarning: sound %s has no dehacked number\n",
                 sfx->mnemonic);
      *target = 0;
   }
   else
      *target = sfx->dehackednum;
}

//
// E_ThingFrame
//
// Does frame name lookup & verification and then stores the resulting
// frame index into *target.
//
static void E_ThingFrame(const char *data, const char *fieldname,
                         int thingnum, int *target)
{
   int index;

   if((index = E_StateNumForName(data)) == NUMSTATES)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tThing %d: invalid %s %s\n",
                 thingnum, fieldname, data);

      I_Error("E_ThingFrame: thing %d: invalid %s %s\n",
              thingnum, fieldname, data);
   }
   *target = index;
}

//
// E_SpeedCB
//
// libConfuse value-parsing callback for the thing speed field.
// Allows input of either integer or floating-point values, where
// the latter are converted to fixed-point for storage.
//
static int E_SpeedCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                     void *result)
{
   char *endptr;
   const char *dotloc;

   // test for a decimal point
   dotloc = strchr(value, '.');

   if(dotloc)
   {
      // process a float and convert to fixed-point
      double tmp;

      tmp = strtod(value, &endptr);

      if(*endptr != '\0')
      {
         if(cfg)
         {
            cfg_error(cfg, "invalid floating point value for option '%s'",
                      opt->name);
         }
         return -1;
      }
      if(errno == ERANGE) 
      {
         if(cfg)
         {
            cfg_error(cfg,
               "floating point value for option '%s' is out of range",
               opt->name);
         }
         return -1;
      }

      *(long *)result = (long)(tmp * FRACUNIT);
   }
   else
   {
      // process an integer
      *(long *)result = strtol(value, &endptr, 0);
      
      if(*endptr != '\0')
      {
         if(cfg)
         {
            cfg_error(cfg, "invalid integer value for option '%s'",
                      opt->name);
         }
         return -1;
      }
      if(errno == ERANGE) 
      {
         if(cfg)
         {
            cfg_error(cfg,
               "integer value for option '%s' is out of range",
               opt->name);
         }
         return -1;
      }
   }

   return 0;
}

//
// E_ColorCB
//
// libConfuse value-parsing callback for the thingtype translation
// field. Can accept an integer value which indicates one of the 14
// builtin player translations, or a lump name, which will be looked
// up within the translations namespace (T_START/T_END), allowing for
// custom sprite translations.
//
static int E_ColorCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                     void *result)
{
   int num;
   char *endptr;

   num = strtol(value, &endptr, 0);

   // try lump name
   if(*endptr != '\0')
   {
      int markernum = W_GetNumForName("T_START");
      int lumpnum   = (W_CheckNumForName)(value, ns_translations);

      if(lumpnum == -1)
      {
         if(cfg)
         {
            cfg_error(cfg, "bad translation lump '%s'", value);
         }
         return -1;
      }
         
      *(long *)result = lumpnum - markernum + TRANSLATIONCOLOURS;
   }
   else
   {
      *(long *)result = num % TRANSLATIONCOLOURS;
   }

   return 0;
}


//
// E_ColorCB
//
// libConfuse value-parsing callback for the thingtype translucency
// field. Can accept an integer value or a percentage.
//
static int E_TranslucCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                        void *result)
{
   char *endptr, *pctloc;

   // test for a percent sign (start looking at end)
   pctloc = strrchr(value, '%');

   if(pctloc)
   {
      long pctvalue;
      
      // get the percentage value (base 10 only)
      pctvalue = strtol(value, &endptr, 10);

      // strtol should stop at the percentage sign
      if(endptr != pctloc)
      {
         if(cfg)
         {
            cfg_error(cfg, "invalid percentage value for option '%s'",
                      opt->name);
         }
         return -1;
      }
      if(errno == ERANGE || pctvalue < 0 || pctvalue > 100) 
      {
         if(cfg)
         {
            cfg_error(cfg,
               "percentage value for option '%s' is out of range",
               opt->name);
         }
         return -1;
      }

      *(long *)result = (65536 * pctvalue) / 100;
   }
   else
   {
      // process an integer
      *(long *)result = strtol(value, &endptr, 0);
      
      if(*endptr != '\0')
      {
         if(cfg)
         {
            cfg_error(cfg, "invalid integer value for option '%s'",
                      opt->name);
         }
         return -1;
      }
      if(errno == ERANGE) 
      {
         if(cfg)
         {
            cfg_error(cfg,
               "integer value for option '%s' is out of range",
               opt->name);
         }
         return -1;
      }
   }

   return 0;
}

// Thing type inheritance code -- 01/27/04

// thing_hitlist: keeps track of what thingtypes are initialized
static byte *thing_hitlist = NULL;

// thing_pstack: used by recursive E_ProcessThing to track inheritance
static int  *thing_pstack  = NULL;
static int  thing_pindex   = 0;

//
// E_CheckThingInherit
//
// Makes sure the thing type being inherited from has not already
// been inherited during the current inheritance chain. Returns
// false if the check fails, and true if it succeeds.
//
static boolean E_CheckThingInherit(int pnum)
{
   int i;

   for(i = 0; i < NUMMOBJTYPES; ++i)
   {
      // circular inheritance
      if(thing_pstack[i] == pnum)
         return false;

      // found end of list
      if(thing_pstack[i] == -1)
         break;
   }

   return true;
}

//
// E_AddThingToPStack
//
// Adds a type number to the inheritance stack.
//
static void E_AddThingToPStack(int num)
{
   // Overflow shouldn't happen since it would require cyclic
   // inheritance as well, but I'll guard against it anyways.
   
   if(thing_pindex >= NUMMOBJTYPES)
      I_Error("E_AddThingToPStack: max inheritance depth exceeded\n");

   thing_pstack[thing_pindex++] = num;
}

//
// E_ResetThingPStack
//
// Resets the thingtype inheritance stack, setting all the pstack
// values to -1, and setting pindex back to zero.
//
static void E_ResetThingPStack(void)
{
   int i;

   for(i = 0; i < NUMMOBJTYPES; ++i)
      thing_pstack[i] = -1;

   thing_pindex = 0;
}

//
// E_CopyThing
//
// Copies one thingtype into another.
//
static void E_CopyThing(int num, int pnum)
{
   
   char name[41];
   mobjinfo_t *this_mi;
   int dehnum, dehnext, namenext;
   
   this_mi = &mobjinfo[num];

   // must save the following fields in the destination thing
   
   dehnum   = this_mi->dehnum;
   dehnext  = this_mi->dehnext;
   namenext = this_mi->namenext;
   memcpy(name, this_mi->name, 41);

   // copy from source to destination
   memcpy(this_mi, &mobjinfo[pnum], sizeof(mobjinfo_t));

   // normalize special fields

   // must duplicate obituaries if they exist
   if(this_mi->obituary)
      this_mi->obituary = strdup(this_mi->obituary);
   if(this_mi->meleeobit)
      this_mi->meleeobit = strdup(this_mi->meleeobit);

   // copy speedset if one exists for parent type
   G_CopySpeedSet(num, pnum);

   // copy nukespec if one exists for parent type
   M_CopyNukeSpec(num, pnum);

   // must restore name and dehacked num data
   this_mi->dehnum   = dehnum;
   this_mi->dehnext  = dehnext;
   this_mi->namenext = namenext;
   memcpy(this_mi->name, name, 41);

   // other fields not inherited:

   // force doomednum of inheriting type to -1
   this_mi->doomednum = -1;
}

// IS_SET: this macro tests whether or not a particular field should
// be set. When applying deltas, we should not retrieve defaults.
// 01/27/04: Also, if inheritance is taking place, we should not
// retrieve defaults.

#undef  IS_SET
#define IS_SET(name) ((def && !inherits) || cfg_size(thingsec, (name)) > 0)

//
// E_ProcessThing
//
// Generalized code to process the data for a single thing type
// structure. Doubles as code for thingtype and thingdelta.
//
static void E_ProcessThing(int i, cfg_t *thingsec, 
                           cfg_t *pcfg, boolean def)
{
   double tempfloat;
   int tempint;
   const char *tempstr;
   boolean inherits = false;
   boolean cflags   = false;

   // 01/27/04: added inheritance -- not in deltas
   if(def)
   {
      // if this thingtype is already processed via recursion due to
      // inheritance, don't process it again
      if(thing_hitlist[i])
         return;
      
      if(cfg_size(thingsec, ITEM_TNG_INHERITS) > 0)
      {
         cfg_t *parent_tngsec;
         
         // resolve parent thingtype
         int pnum = E_GetThingNumForName(cfg_getstr(thingsec, ITEM_TNG_INHERITS));

         // check against cyclic inheritance
         if(!E_CheckThingInherit(pnum))
         {
            if(edf_output)
               fprintf(edf_output, 
                       "\t\tError: cyclic inheritance detected in thing %s\n",
                       mobjinfo[i].name);

            I_Error("E_ProcessThing: cyclic inheritance detected in thing %s\n",
                    mobjinfo[i].name);
         }
         
         // add to inheritance stack
         E_AddThingToPStack(pnum);

         // process parent recursively
         parent_tngsec = cfg_getnsec(pcfg, SEC_THING, pnum);
         E_ProcessThing(pnum, parent_tngsec, pcfg, true);
         
         // copy parent to this thing
         E_CopyThing(i, pnum);
         
         // we inherit, so treat defaults as no value
         inherits = true;
      }

      // mark this thingtype as processed
      thing_hitlist[i] = 1;
   }

   // process doomednum
   if(IS_SET(ITEM_TNG_DOOMEDNUM))
      mobjinfo[i].doomednum = cfg_getint(thingsec, ITEM_TNG_DOOMEDNUM);

   // process spawnstate
   if(IS_SET(ITEM_TNG_SPAWNSTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_SPAWNSTATE);
      E_ThingFrame(tempstr, ITEM_TNG_SPAWNSTATE, i, 
                   &(mobjinfo[i].spawnstate));
   }

   // process spawnhealth
   if(IS_SET(ITEM_TNG_SPAWNHEALTH))
      mobjinfo[i].spawnhealth = cfg_getint(thingsec, ITEM_TNG_SPAWNHEALTH);

   // process seestate
   if(IS_SET(ITEM_TNG_SEESTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_SEESTATE);
      E_ThingFrame(tempstr, ITEM_TNG_SEESTATE, i,
                   &(mobjinfo[i].seestate));
   }

   // process seesound
   if(IS_SET(ITEM_TNG_SEESOUND))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_SEESOUND);
      E_ThingSound(tempstr, ITEM_TNG_SEESOUND, i,
                   &(mobjinfo[i].seesound));
   }

   // process reactiontime
   if(IS_SET(ITEM_TNG_REACTTIME))
      mobjinfo[i].reactiontime = cfg_getint(thingsec, ITEM_TNG_REACTTIME);

   // process attacksound
   if(IS_SET(ITEM_TNG_ATKSOUND))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_ATKSOUND);
      E_ThingSound(tempstr, ITEM_TNG_ATKSOUND, i,
                   &(mobjinfo[i].attacksound));
   }

   // process painstate
   if(IS_SET(ITEM_TNG_PAINSTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_PAINSTATE);
      E_ThingFrame(tempstr, ITEM_TNG_PAINSTATE, i,
                   &(mobjinfo[i].painstate));
   }

   // process painchance
   if(IS_SET(ITEM_TNG_PAINCHANCE))
      mobjinfo[i].painchance = cfg_getint(thingsec, ITEM_TNG_PAINCHANCE);

   // process painsound
   if(IS_SET(ITEM_TNG_PAINSOUND))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_PAINSOUND);
      E_ThingSound(tempstr, ITEM_TNG_PAINSOUND, i,
                   &(mobjinfo[i].painsound));
   }

   // process meleestate
   if(IS_SET(ITEM_TNG_MELEESTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_MELEESTATE);
      E_ThingFrame(tempstr, ITEM_TNG_MELEESTATE, i,
                   &(mobjinfo[i].meleestate));
   }

   // process missilestate
   if(IS_SET(ITEM_TNG_MISSILESTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_MISSILESTATE);
      E_ThingFrame(tempstr, ITEM_TNG_MISSILESTATE, i,
                   &(mobjinfo[i].missilestate));
   }

   // process deathstate
   if(IS_SET(ITEM_TNG_DEATHSTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_DEATHSTATE);
      E_ThingFrame(tempstr, ITEM_TNG_DEATHSTATE, i,
                   &(mobjinfo[i].deathstate));
   }

   // process xdeathstate
   if(IS_SET(ITEM_TNG_XDEATHSTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_XDEATHSTATE);
      E_ThingFrame(tempstr, ITEM_TNG_XDEATHSTATE, i,
                   &(mobjinfo[i].xdeathstate));
   }

   // process deathsound
   if(IS_SET(ITEM_TNG_DEATHSOUND))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_DEATHSOUND);
      E_ThingSound(tempstr, ITEM_TNG_DEATHSOUND, i,
                   &(mobjinfo[i].deathsound));
   }

   // process speed
   if(IS_SET(ITEM_TNG_SPEED))
      mobjinfo[i].speed = cfg_getint(thingsec, ITEM_TNG_SPEED);

   // process radius
   if(IS_SET(ITEM_TNG_RADIUS))
   {
      tempfloat = cfg_getfloat(thingsec, ITEM_TNG_RADIUS);
      mobjinfo[i].radius = (int)(tempfloat * FRACUNIT);
   }

   // process height
   if(IS_SET(ITEM_TNG_HEIGHT))
   {
      tempfloat = cfg_getfloat(thingsec, ITEM_TNG_HEIGHT);
      mobjinfo[i].height = (int)(tempfloat * FRACUNIT);
   }

   // process mass
   if(IS_SET(ITEM_TNG_MASS))
      mobjinfo[i].mass = cfg_getint(thingsec, ITEM_TNG_MASS);

   // process damage
   if(IS_SET(ITEM_TNG_DAMAGE))
      mobjinfo[i].damage = cfg_getint(thingsec, ITEM_TNG_DAMAGE);

   // process activesound
   if(IS_SET(ITEM_TNG_ACTIVESOUND))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_ACTIVESOUND);
      E_ThingSound(tempstr, ITEM_TNG_ACTIVESOUND, i,
                   &(mobjinfo[i].activesound));
   }

   // 02/19/04: process combined flags first
   if(IS_SET(ITEM_TNG_CFLAGS))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_CFLAGS);
      if(*tempstr == '\0')
      {
         mobjinfo[i].flags = mobjinfo[i].flags2 = mobjinfo[i].flags3 = 0;
      }
      else
      {
         long *results = deh_ParseFlagsCombined(tempstr);

         mobjinfo[i].flags  = results[0];
         mobjinfo[i].flags2 = results[1];
         mobjinfo[i].flags3 = results[2];
         
         cflags = true; // values were set from cflags
      }
   }

   if(!cflags) // skip if cflags are defined
   {
      // process flags
      if(IS_SET(ITEM_TNG_FLAGS))
      {
         tempstr = cfg_getstr(thingsec, ITEM_TNG_FLAGS);
         if(*tempstr == '\0')
            mobjinfo[i].flags = 0;
         else
            mobjinfo[i].flags = deh_ParseFlagsSingle(tempstr, DEHFLAGS_MODE1);
      }
      
      // process flags2
      if(IS_SET(ITEM_TNG_FLAGS2))
      {
         tempstr = cfg_getstr(thingsec, ITEM_TNG_FLAGS2);
         if(*tempstr == '\0')
            mobjinfo[i].flags2 = 0;
         else
            mobjinfo[i].flags2 = deh_ParseFlagsSingle(tempstr, DEHFLAGS_MODE2);
      }

      // process flags3
      if(IS_SET(ITEM_TNG_FLAGS3))
      {
         tempstr = cfg_getstr(thingsec, ITEM_TNG_FLAGS3);
         if(*tempstr == '\0')
            mobjinfo[i].flags3 = 0;
         else
            mobjinfo[i].flags3 = deh_ParseFlagsSingle(tempstr, DEHFLAGS_MODE3);
      }
   }

   // process addflags and remflags modifiers

   if(cfg_size(thingsec, ITEM_TNG_ADDFLAGS) > 0)
   {
      long *results;

      tempstr = cfg_getstr(thingsec, ITEM_TNG_ADDFLAGS);
         
      results = deh_ParseFlagsCombined(tempstr);

      mobjinfo[i].flags  |= results[0];
      mobjinfo[i].flags2 |= results[1];
      mobjinfo[i].flags3 |= results[2];
   }

   if(cfg_size(thingsec, ITEM_TNG_REMFLAGS) > 0)
   {
      long *results;

      tempstr = cfg_getstr(thingsec, ITEM_TNG_REMFLAGS);

      results = deh_ParseFlagsCombined(tempstr);

      mobjinfo[i].flags  &= ~(results[0]);
      mobjinfo[i].flags2 &= ~(results[1]);
      mobjinfo[i].flags3 &= ~(results[2]);
   }

   // process raisestate
   if(IS_SET(ITEM_TNG_RAISESTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_RAISESTATE);
      E_ThingFrame(tempstr, ITEM_TNG_RAISESTATE, i,
                   &(mobjinfo[i].raisestate));
   }

   // process translucency
   if(IS_SET(ITEM_TNG_TRANSLUC))
      mobjinfo[i].translucency = cfg_getint(thingsec, ITEM_TNG_TRANSLUC);

   // process bloodcolor
   if(IS_SET(ITEM_TNG_BLOODCOLOR))
      mobjinfo[i].bloodcolor = cfg_getint(thingsec, ITEM_TNG_BLOODCOLOR);

   // 07/13/03: process fastspeed
   // get the fastspeed and, if non-zero, add the thing
   // to the speedset list in g_game.c

   if(IS_SET(ITEM_TNG_FASTSPEED))
   {
      tempint = cfg_getint(thingsec, ITEM_TNG_FASTSPEED);         
      if(tempint)
         G_SpeedSetAddThing(i, mobjinfo[i].speed, tempint);
   }

   // 07/13/03: process nukespecial
   // get the nukespecial, and if not NULL, add the thing
   // to the nukespec hash table in m_cheat.c

   if(IS_SET(ITEM_TNG_NUKESPEC))
   {
      deh_bexptr *dp;

      tempstr = cfg_getstr(thingsec, ITEM_TNG_NUKESPEC);
      
      if(!(dp = D_GetBexPtr(tempstr)))
      {
         if(edf_output)
            fprintf(edf_output, "\t\tThing %d: bad nukespecial %s\n",
            i, tempstr);
         
         I_Error("E_ProcessThing: thing %d: bad nukespecial %s\n",
            i, tempstr);
      }
      
      if(dp->cptr != NULL)
         M_AddNukeSpec(i, (void (*)(mobj_t*))(dp->cptr)); // yuck!
   }

   // 07/13/03: process particlefx
   if(IS_SET(ITEM_TNG_PARTICLEFX))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_PARTICLEFX);
      if(*tempstr == '\0')
         mobjinfo[i].particlefx = 0;
      else
      {
         char *buffer;
         char *bufferptr;

         bufferptr = buffer = strdup(tempstr);

         deh_ParseFlags(&particle_flags, &bufferptr, NULL);

         free(buffer);
         
         mobjinfo[i].particlefx = particle_flags.results[0];            
      }
   }
      
   // 07/13/03: process droptype
   if(IS_SET(ITEM_TNG_DROPTYPE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_DROPTYPE);
      if(!strcasecmp(tempstr, "NONE"))
         mobjinfo[i].droptype = -1;
      else
      {
         tempint = E_ThingNumForName(tempstr);

         if(tempint == NUMMOBJTYPES)
         {
            if(edf_output)
               fprintf(edf_output, "\t\tError: bad drop type %s\n", tempstr);

            I_Error("E_ProcessThing: thing %d: bad drop type %s\n",
                    i, tempstr);
         }

         mobjinfo[i].droptype = tempint;
      }
   }

   // 07/13/03: process mod
   if(IS_SET(ITEM_TNG_MOD))
      mobjinfo[i].mod = cfg_getint(thingsec, ITEM_TNG_MOD);

   // 07/13/03: process obituaries
   if(IS_SET(ITEM_TNG_OBIT1))
   {
      // if this is a delta or the thing type inherited obits
      // from its parent, we need to free any old obituary
      if((!def || inherits) && mobjinfo[i].obituary)
         free(mobjinfo[i].obituary);

      tempstr = cfg_getstr(thingsec, ITEM_TNG_OBIT1);
      if(strcasecmp(tempstr, "NONE"))
         mobjinfo[i].obituary = strdup(tempstr);
      else
         mobjinfo[i].obituary = NULL;
   }

   if(IS_SET(ITEM_TNG_OBIT2))
   {
      // if this is a delta or the thing type inherited obits
      // from its parent, we need to free any old obituary
      if((!def || inherits) && mobjinfo[i].meleeobit)
         free(mobjinfo[i].meleeobit);

      tempstr = cfg_getstr(thingsec, ITEM_TNG_OBIT2);
      if(strcasecmp(tempstr, "NONE"))
         mobjinfo[i].meleeobit = strdup(tempstr);
      else
         mobjinfo[i].meleeobit = NULL;
   }

   // 01/12/04: process translation
   if(IS_SET(ITEM_TNG_COLOR))
      mobjinfo[i].colour = cfg_getint(thingsec, ITEM_TNG_COLOR);

   // 08/01/04: process dmgspecial
   if(IS_SET(ITEM_TNG_DMGSPECIAL))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_DMGSPECIAL);
      
      // find the proper dmgspecial number (linear search)
      tempint = 0;
      while(tempint != INFLICTOR_NUMTYPES && 
            strcasecmp(inflictorTypes[tempint], tempstr))
      {
         ++tempint;
      }

      if(tempint == INFLICTOR_NUMTYPES)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tInvalid dmgspecial: %s\n", tempstr);

         I_Error("E_ProcessThing: thing %d: invalid dmgspecial %s\n", 
                 i, tempstr);
      }

      mobjinfo[i].dmgspecial = tempint;
   }

   // 08/07/04: process crashstate
   if(IS_SET(ITEM_TNG_CRASHSTATE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_CRASHSTATE);
      E_ThingFrame(tempstr, ITEM_TNG_CRASHSTATE, i,
                   &(mobjinfo[i].crashstate));
   }

   // 09/26/04: process alternate sprite
   if(IS_SET(ITEM_TNG_SKINSPRITE))
   {
      tempstr = cfg_getstr(thingsec, ITEM_TNG_SKINSPRITE);
      mobjinfo[i].altsprite = E_SpriteNumForName(tempstr);
   }

   // output end message if processing a definition
   if(def && edf_output)
      fprintf(edf_output, "\t\tFinished thing %s(#%d)\n",
              mobjinfo[i].name, i);
}

//
// E_ProcessThings
//
// Resolves and loads all information for the mobjinfo_t structures.
//
static void E_ProcessThings(cfg_t *cfg)
{
   int i;

   if(edf_output)
      fputs("\t* Processing thing data\n", edf_output);

   // allocate inheritance stack and hitlist
   thing_hitlist = Z_Malloc(NUMMOBJTYPES*sizeof(byte), PU_STATIC, 0);
   thing_pstack  = Z_Malloc(NUMMOBJTYPES*sizeof(int),  PU_STATIC, 0);

   // initialize hitlist
   memset(thing_hitlist, 0, NUMMOBJTYPES*sizeof(byte));

   for(i = 0; i < NUMMOBJTYPES; ++i)
   {
      cfg_t *thingsec = cfg_getnsec(cfg, SEC_THING, i);

      // reset the inheritance stack
      E_ResetThingPStack();

      // add this thing to the stack
      E_AddThingToPStack(i);

      E_ProcessThing(i, thingsec, cfg, true);
   }

   // free tables
   Z_Free(thing_hitlist);
   Z_Free(thing_pstack);
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

   if(edf_output)
      fputs("\t\tZero frames found, attempting to load default frames.edf\n", edf_output);

   statefn = E_BuildDefaultFn("frames.edf");

   statecfg = E_ParseEDFFile(statefn, frame_only_opts);

   // Reset NUMSTATES -- it will be tested again below
   NUMSTATES = cfg_size(statecfg, SEC_FRAME);

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

   if(edf_output)
      fputs("\t\tZero things found, attempting to load default things.edf\n", edf_output);

   thingfn = E_BuildDefaultFn("things.edf");

   thingcfg = E_ParseEDFFile(thingfn, thing_only_opts);

   // Reset NUMMOBJTYPES -- it will be tested again below
   NUMMOBJTYPES = cfg_size(thingcfg, SEC_THING);

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

   if(edf_output)
      fputs("\t* Beginning state and thing processing\n", edf_output);

   // start out pointers pointing at the parameter cfg_t
   framecfg = cfg;
   thingcfg = cfg;

   // check number of states
   NUMSTATES = cfg_size(cfg, SEC_FRAME);
   if(NUMSTATES == 0)
   {
      // try the defaults
      framecfg = E_TryDefaultStates();
      framedefs = true;
      if(NUMSTATES == 0)
      {
         if(edf_output)
            fputs("\t\tError: zero frames defined\n", edf_output);

         I_Error("E_ProcessStatesAndThings: zero frames defined\n");
      }
   }

   // check number of things
   NUMMOBJTYPES = cfg_size(cfg, SEC_THING);
   if(NUMMOBJTYPES == 0)
   {
      // try the defaults
      thingcfg = E_TryDefaultThings();
      thingdefs = true;
      if(NUMMOBJTYPES == 0)
      {
         if(edf_output)
            fputs("\t\tError: zero things defined\n", edf_output);

         I_Error("E_ProcessStatesAndThings: zero things defined\n");
      }
   }

   // allocate structures, build mnemonic and dehnum hash tables
   E_CollectNames(framecfg, thingcfg);

   // process states
   E_ProcessStates(framecfg);

   // process things
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

   if(edf_output)
      fputs("\t* Processing cast call\n", edf_output);
   
   // get number of cast sections
   cs_size = cfg_size(cfg, SEC_CAST);

   if(!cs_size)
   {
      if(edf_output)
         fputs("\t\tNo cast members defined\n", edf_output);

      // 11/04/03: try the default cast.edf definitions
      E_TryDefaultCast();
      return;
   }

   if(edf_output)
      fprintf(edf_output, "\t\t%d cast member(s) defined\n", cs_size);

   // check if the "castorder" array is defined for imposing an
   // order on the castinfo sections
   ci_size = cfg_size(cfg, SEC_CASTORDER);

   if(edf_output)
      fprintf(edf_output, "\t\t%d cast member(s) in castorder\n", ci_size);

   // determine size of castorder
   max_castorder = (ci_size > 0) ? ci_size : cs_size;

   // allocate with size+1 for an end marker
   castorder = malloc(sizeof(castinfo_t)*(max_castorder + 1));
   ci_order  = malloc(sizeof(cfg_t *) * max_castorder);

   if(ci_size > 0)
   {
      for(i = 0; i < ci_size; i++)
      {
         const char *title = cfg_getnstr(cfg, SEC_CASTORDER, i);         
         cfg_t *section    = cfg_gettsec(cfg, SEC_CAST, title);

         if(!section)
         {
            if(edf_output)
               fputs("\t\tError: unknown cast member in castorder\n", edf_output);

            I_Error("E_ProcessCast: unknown cast member in castorder\n");
         }

         ci_order[i] = section;
      }
   }
   else
   {
      // no castorder array is defined, so use the cast members
      // in the order they are encountered (for backward compatibility)
      for(i = 0; i < cs_size; i++)
         ci_order[i] = cfg_getnsec(cfg, SEC_CAST, i);
   }


   for(i = 0; i < max_castorder; i++)
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
         if(edf_output)
            fprintf(edf_output, "\t\tWarning: cast %d: unknown thing type %s\n",
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

   if(edf_output)
      fputs("\t\tAttempting to load defaults from cast.edf\n", edf_output);

   castfn = E_BuildDefaultFn("cast.edf");

   castcfg = E_ParseEDFFile(castfn, cast_only_opts);

   max_castorder = cfg_size(castcfg, SEC_CAST);

   if(!max_castorder)
   {
      if(edf_output)
         fputs("\t\tError: no default cast members exist!\n", edf_output);

      I_Error("E_TryDefaultCast: missing default cast.\n");
   }

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

   if(edf_output)
      fputs("\t* Processing boss spawn types\n", edf_output);

   if(!numTypes)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tError: no boss types defined\n");

      I_Error("E_ProcessBossTypes: no boss types defined\n");
   }

   // haleyjd 11/19/03: allow defaults for boss spawn probs
   if(!numProbs)
      useProbs = false;

   if((useProbs && numTypes != numProbs) || numTypes != 11)
   {
      if(edf_output)
         fprintf(edf_output, "\t\tError: %d boss types, %d boss probs\n",
                 numTypes, useProbs ? numProbs : 11);

      I_Error("E_ProcessBossTypes: %d boss types, %d boss probs\n",
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
      if(edf_output)
         fputs("\t\tError: boss spawn probs don't total 256\n", edf_output);

      I_Error("E_ProcessBossTypes: boss spawn probs don't total 256\n");
   }

   for(i = 0; i < numTypes; ++i)
   {
      const char *typeName = cfg_getnstr(cfg, SEC_BOSSTYPES, i);
      int typeNum = E_ThingNumForName(typeName);

      if(typeNum == NUMMOBJTYPES)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tWarning: invalid type %s\n", typeName);

         typeNum = UnknownThingType;
      }

      BossSpawnTypes[i] = typeNum;

      if(edf_output)
         fprintf(edf_output, 
                 "\t\tAssigned type %s(#%d) to boss type %d\n",
                 mobjinfo[typeNum].name, typeNum, i);
   }
}

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
}

//
// Delta Structure Processing Functions
//

//
// E_ProcessSoundDeltas
//
// Does processing for sounddelta sections, which allow cascading
// editing of existing sounds. The sounddelta shares most of its
// fields and processing code with the sound section.
//
void E_ProcessSoundDeltas(cfg_t *cfg)
{
   int i, numdeltas;

   if(edf_output)
      fputs("\t* Processing sound deltas\n", edf_output);

   numdeltas = cfg_size(cfg, SEC_SDELTA);

   if(edf_output)
      fprintf(edf_output, "\t\t%d sounddelta(s) defined\n", numdeltas);

   for(i = 0; i < numdeltas; i++)
   {
      const char *tempstr;
      sfxinfo_t *sfx;
      cfg_t *deltasec = cfg_getnsec(cfg, SEC_SDELTA, i);

      // get thingtype to edit
      if(!cfg_size(deltasec, ITEM_DELTA_NAME))
      {
         if(edf_output)
            fputs("\t\tError: missing name in sounddelta\n", edf_output);

         I_Error("E_ProcessSoundDeltas: sounddelta requires name field\n");
      }

      tempstr = cfg_getstr(deltasec, ITEM_DELTA_NAME);
      sfx = E_SoundForName(tempstr);

      if(!sfx)
      {
         if(edf_output)
            fprintf(edf_output, "\t\tError: sound %s doesn't exist\n",
                    tempstr);
         I_Error("E_ProcessSoundDeltas: sound %s does not exist\n",
                 tempstr);
      }

      E_ProcessSound(sfx, deltasec, false);

      if(edf_output)
         fprintf(edf_output, "\t\tApplied sounddelta #%d to sound %s\n",
                 i, tempstr);
   }
}

//
// E_ProcessStateDeltas
//
// Does processing for framedelta sections, which allow cascading
// editing of existing frames. The framedelta shares most of its
// fields and processing code with the frame section.
//
static void E_ProcessStateDeltas(cfg_t *cfg)
{
   int i, numdeltas;

   if(edf_output)
      fputs("\t* Processing frame deltas\n", edf_output);

   numdeltas = cfg_size(cfg, SEC_FRAMEDELTA);

   if(edf_output)
      fprintf(edf_output, "\t\t%d framedelta(s) defined\n", numdeltas);

   for(i = 0; i < numdeltas; i++)
   {
      const char *tempstr;
      int stateNum;
      cfg_t *deltasec = cfg_getnsec(cfg, SEC_FRAMEDELTA, i);

      // get state to edit
      if(!cfg_size(deltasec, ITEM_DELTA_NAME))
      {
         if(edf_output)
            fputs("\t\tError: missing name in framedelta\n", edf_output);

         I_Error("E_ProcessFrameDeltas: framedelta requires name field\n");
      }

      tempstr = cfg_getstr(deltasec, ITEM_DELTA_NAME);
      stateNum = E_GetStateNumForName(tempstr);

      E_ProcessState(stateNum, deltasec, false);

      if(edf_output)
         fprintf(edf_output, "\t\tApplied framedelta #%d to %s(#%d)\n",
                 i, states[stateNum].name, stateNum);
   }
}

//
// E_ProcessThingDeltas
//
// Does processing for thingdelta sections, which allow cascading
// editing of existing things. The thingdelta shares most of its
// fields and processing code with the thingtype section.
//
static void E_ProcessThingDeltas(cfg_t *cfg)
{
   int i, numdeltas;

   if(edf_output)
      fputs("\t* Processing thing deltas\n", edf_output);

   numdeltas = cfg_size(cfg, SEC_THINGDELTA);

   if(edf_output)
      fprintf(edf_output, "\t\t%d thingdelta(s) defined\n", numdeltas);

   for(i = 0; i < numdeltas; i++)
   {
      const char *tempstr;
      int mobjType;
      cfg_t *deltasec = cfg_getnsec(cfg, SEC_THINGDELTA, i);

      // get thingtype to edit
      if(!cfg_size(deltasec, ITEM_DELTA_NAME))
      {
         if(edf_output)
            fputs("\t\tError: missing name in thingdelta\n", edf_output);

         I_Error("E_ProcessThingDeltas: thingdelta requires name field\n");
      }

      tempstr = cfg_getstr(deltasec, ITEM_DELTA_NAME);
      mobjType = E_GetThingNumForName(tempstr);

      E_ProcessThing(mobjType, deltasec, cfg, false);

      if(edf_output)
         fprintf(edf_output, "\t\tApplied thingdelta #%d to %s(#%d)\n",
                 i, mobjinfo[mobjType].name, mobjType);
   }
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

   // check for -edfout to enable verbose logging
   if(M_CheckParm("-edfout"))
   {
      edf_output = fopen("edfout.txt", "w");
   }

   printf("E_ProcessEDF: Loading root file %s\n", filename);
   if(edf_output)
      fprintf(edf_output, "Processing EDF file %s\n", filename);

   cfg = E_ParseEDFFile(filename, edf_opts);

   // NOTE: The order of most of the following calls is extremely 
   // important and must be preserved, unless the static routines 
   // above are rewritten accordingly.

   // process sprites, sprite-related variables, and pickup item fx
   E_ProcessSprites(cfg);

   // process sprite-related variables
   E_ProcessSpriteVars(cfg);

   // 09/03/03: process sounds
   E_ProcessSounds(cfg);

   // allocate frames and things, build name hash tables, and
   // process frame and thing definitions
   E_ProcessStatesAndThings(cfg);

   // process cast call
   E_ProcessCast(cfg);

   // process boss spawn types
   E_ProcessBossTypes(cfg);

   // 01/11/04: process misc vars
   E_ProcessMiscVars(cfg);

   // 08/30/03: apply deltas
   E_ProcessSoundDeltas(cfg);
   E_ProcessStateDeltas(cfg);
   E_ProcessThingDeltas(cfg);
   
   if(edf_output)
      fputs("Processing finished, freeing tables\n", edf_output);

   // free unneeded hash tables and string arrays
   Z_Free(sprchains);
   Z_Free(sprnext);

   if(edf_output)
      fputs("Freeing cfg object\n", edf_output);

   // free the config object
   cfg_free(cfg);

   if(edf_output)
      fputs("Checking zone heap integrity\n", edf_output);

   // check heap integrity for safety
   Z_CheckHeap();

   // close the verbose log file
   if(edf_output)
   {
      fputs("Closing log file\n", edf_output);
      fclose(edf_output);
      edf_output = NULL;
   }
}

// utility functions

//
// E_SpriteNumForName
//
// Sprite hashing function, valid only during EDF parsing. Returns
// the index of "name" in the sprnames array, if found. If not,
// returns NUMSPRITES.
//
static int E_SpriteNumForName(const char *name)
{
   int sprnum;
   unsigned int sprkey = D_HashTableKey(name) % NUMSPRCHAINS;

   sprnum = sprchains[sprkey];
   while(sprnum != NUMSPRITES && strcasecmp(name, sprnames[sprnum]))
      sprnum = sprnext[sprnum];

   return sprnum;
}

// haleyjd 09/03/03: removed E_SoundNumForName

// Global hashing functions

// States

//
// E_StateNumForDEHNum
//
// State DeHackEd numbers *were* simply the actual, internal state
// number, but we have to actually store and hash them for EDF to
// remain cross-version compatible. If a state with the requested
// dehnum isn't found, NUMSTATES is returned.
//
int E_StateNumForDEHNum(int dehnum)
{
   int statenum;
   int statekey = dehnum % NUMSTATECHAINS;

   // 08/31/03: return null state for negative numbers, to
   // please some old, incorrect DeHackEd patches
   if(dehnum < 0)
      return NullStateNum;

   statenum = state_dehchains[statekey];
   while(statenum != NUMSTATES && states[statenum].dehnum != dehnum)
      statenum = states[statenum].dehnext;

   return statenum;
}

//
// E_GetStateNumForDEHNum
//
// As above, but causes a fatal error if the state isn't found,
// rather than returning NUMSTATES. This keeps error checking code
// from being necessitated all over the source code.
//
int E_GetStateNumForDEHNum(int dehnum)
{
   int statenum = E_StateNumForDEHNum(dehnum);

   if(statenum == NUMSTATES)
      I_Error("E_GetStateNumForDEHNum: invalid deh num %d\n", dehnum);

   return statenum;
}

//
// E_SafeState
//
// As above, but returns index of S_NULL state if the requested
// one was not found.
//
int E_SafeState(int dehnum)
{
   int statenum = E_StateNumForDEHNum(dehnum);

   if(statenum == NUMSTATES)
      statenum = NullStateNum;

   return statenum;
}

//
// E_NullState
//
// Returns the number of the null state
//
int E_NullState(void)
{
   return NullStateNum;
}

//
// E_StateNumForName
//
// Returns the number of a state given its name. Returns NUMSTATES
// if the state is not found.
//
int E_StateNumForName(const char *name)
{
   int statenum;
   unsigned int statekey = D_HashTableKey(name) % NUMSTATECHAINS;
   
   statenum = state_namechains[statekey];
   while(statenum != NUMSTATES && 
         strcasecmp(name, states[statenum].name))
   {
      statenum = states[statenum].namenext;
   }
   
   return statenum;
}

//
// E_GetStateNumForName
//
// As above, but causes a fatal error if the state doesn't exist.
//
int E_GetStateNumForName(const char *name)
{
   int statenum = E_StateNumForName(name);

   if(statenum == NUMSTATES)
      I_Error("E_GetStateNumForName: bad frame %s\n", name);

   return statenum;
}

// Things

//
// E_ThingNumForDEHNum
//
// As with states, things need to store their DeHackEd number now.
// Returns NUMMOBJTYPES if a thing type is not found. This is used
// extensively by parameterized codepointers.
//
int E_ThingNumForDEHNum(int dehnum)
{
   int thingnum;
   int thingkey = dehnum % NUMTHINGCHAINS;

   if(dehnum < 0)
      return NUMMOBJTYPES;

   thingnum = thing_dehchains[thingkey];
   while(thingnum != NUMMOBJTYPES && 
         mobjinfo[thingnum].dehnum != dehnum)
   {
      thingnum = mobjinfo[thingnum].dehnext;
   }

   return thingnum;
}

//
// E_GetThingNumForDEHNum
//
// As above, but causes a fatal error if a thing type is not found.
//
int E_GetThingNumForDEHNum(int dehnum)
{
   int thingnum = E_ThingNumForDEHNum(dehnum);

   if(thingnum == NUMMOBJTYPES)
      I_Error("E_GetThingNumForDEHNum: invalid deh num %d\n", dehnum);

   return thingnum;
}

//
// E_SafeThingType
//
// As above, but returns the 'Unknown' type if the requested
// one was not found.
//
int E_SafeThingType(int dehnum)
{
   int thingnum = E_ThingNumForDEHNum(dehnum);

   if(thingnum == NUMMOBJTYPES)
      thingnum = UnknownThingType;

   return thingnum;
}

//
// E_SafeThingName
//
// As above, but for names
//
int E_SafeThingName(const char *name)
{
   int thingnum = E_ThingNumForName(name);

   if(thingnum == NUMMOBJTYPES)
      thingnum = UnknownThingType;

   return thingnum;
}

//
// E_ThingNumForName
//
// Returns a thing type index given its name. Returns NUMMOBJTYPES
// if a thing type is not found.
//
int E_ThingNumForName(const char *name)
{
   int thingnum;
   unsigned int thingkey = D_HashTableKey(name) % NUMTHINGCHAINS;
   
   thingnum = thing_namechains[thingkey];
   while(thingnum != NUMMOBJTYPES && 
         strcasecmp(name, mobjinfo[thingnum].name))
   {
      thingnum = mobjinfo[thingnum].namenext;
   }
   
   return thingnum;
}

//
// E_GetThingNumForName
//
// As above, but causes a fatal error if the thing type isn't found.
//
int E_GetThingNumForName(const char *name)
{
   int thingnum = E_ThingNumForName(name);

   if(thingnum == NUMMOBJTYPES)
      I_Error("E_GetThingNumForName: bad thing type %s\n", name);

   return thingnum;
}

//
// E_UnknownThing
//
// Returns the 'Unknown' thing type.
//
int E_UnknownThing(void)
{
   return UnknownThingType;
}

// EOF

