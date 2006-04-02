// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
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
//--------------------------------------------------------------------------
//
// $Id: d_deh.c,v 1.20 1998/06/01 22:30:38 thldrmn Exp $
//
// Dehacked file support
// New for the TeamTNT "Boom" engine
//
// Author: Ty Halderman, TeamTNT
//
//--------------------------------------------------------------------

static const char
rcsid[] = "$Id: d_deh.c,v 1.20 1998/06/01 22:30:38 thldrmn Exp $";

// killough 5/2/98: fixed headers, removed rendunant external declarations:
#include "doomdef.h"
#include "doomstat.h"
#include "d_io.h"
#include "sounds.h"
#include "info.h"
#include "m_cheat.h"
#include "p_inter.h"
#include "g_game.h"
#include "d_main.h" // haleyjd
#include "d_think.h"
#include "w_wad.h"
#include "m_misc.h"
#include "d_dehtbl.h"
#include "e_states.h"
#include "e_things.h"
#include "e_sound.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// haleyjd 11/01/02: moved deh file/wad stdio emulation to d_io.c
// and generalized, strengthened encapsulation

// killough 10/98: new functions, to allow processing DEH files 
// in-memory (e.g. from wads)

// killough 10/98: emulate IO whether input really comes from a file 
// or not

// variables used in other routines
boolean deh_pars = false; // in wi_stuff to allow pars in modified games
boolean deh_loaded = false; // sf

// Function prototypes
void    lfstrip(char *);     // strip the \r and/or \n off of a line
void    rstrip(char *);      // strip trailing whitespace
char *  ptr_lstrip(char *);  // point past leading whitespace
boolean deh_GetData(char *, char *, long *, char **, FILE *);
boolean deh_procStringSub(char *, char *, char *, FILE *);
static char *dehReformatStr(char *);

// Prototypes for block processing functions
// Pointers to these functions are used as the blocks are encountered.

void deh_procThing(DWFILE *, FILE*, char *);
void deh_procFrame(DWFILE *, FILE*, char *);
void deh_procPointer(DWFILE *, FILE*, char *);
void deh_procSounds(DWFILE *, FILE*, char *);
void deh_procAmmo(DWFILE *, FILE*, char *);
void deh_procWeapon(DWFILE *, FILE*, char *);
void deh_procSprite(DWFILE *, FILE*, char *);
void deh_procCheat(DWFILE *, FILE*, char *);
void deh_procMisc(DWFILE *, FILE*, char *);
void deh_procText(DWFILE *, FILE*, char *);
void deh_procPars(DWFILE *, FILE*, char *);
void deh_procStrings(DWFILE *, FILE*, char *);
void deh_procError(DWFILE *, FILE*, char *);
void deh_procBexCodePointers(DWFILE *, FILE*, char *);
void deh_procHelperThing(DWFILE *, FILE *, char *); // haleyjd 9/22/99
// haleyjd: handlers to fully deprecate the DeHackEd text section
void deh_procBexSounds(DWFILE *, FILE *, char *);
void deh_procBexMusic(DWFILE *, FILE *, char *);
void deh_procBexSprites(DWFILE *, FILE *, char *);

// Structure deh_block is used to hold the block names that can
// be encountered, and the routines to use to decipher them

typedef struct
{
  char *key;       // a mnemonic block code name
  void (*const fptr)(DWFILE *, FILE*, char *); // handler
} deh_block;

#define DEH_BUFFERMAX 1024 // input buffer area size, hardcodedfor now
// killough 8/9/98: make DEH_BLOCKMAX self-adjusting
#define DEH_BLOCKMAX (sizeof deh_blocks/sizeof*deh_blocks)  // size of array
#define DEH_MAXKEYLEN 32 // as much of any key as we'll look at
#define DEH_MOBJINFOMAX 27 // number of ints in the mobjinfo_t structure (!) haleyjd: 27

// Put all the block header values, and the function to be called when that
// one is encountered, in this array:
deh_block deh_blocks[] = {
   /* 0 */  {"Thing", deh_procThing},
   /* 1 */  {"Frame", deh_procFrame},
   /* 2 */  {"Pointer", deh_procPointer},
   /* 3 */  {"Sound", deh_procSounds},  // Ty 03/16/98 corrected from "Sounds"
   /* 4 */  {"Ammo", deh_procAmmo},
   /* 5 */  {"Weapon", deh_procWeapon},
   /* 6 */  {"Sprite", deh_procSprite},
   /* 7 */  {"Cheat", deh_procCheat},
   /* 8 */  {"Misc", deh_procMisc},
   /* 9 */  {"Text", deh_procText},  // --  end of standard "deh" entries,

   //     begin BOOM Extensions (BEX)

   /* 10 */ {"[STRINGS]", deh_procStrings},    // new string changes
   /* 11 */ {"[PARS]", deh_procPars},          // par times
   /* 12 */ {"[CODEPTR]", deh_procBexCodePointers}, // bex codepointers by mnemonic
   /* 13 */ {"[HELPER]", deh_procHelperThing}, // helper thing substitution haleyjd 9/22/99
   /* 14 */ {"[SPRITES]", deh_procBexSprites}, // bex style sprites
   /* 15 */ {"[SOUNDS]", deh_procBexSounds},   // bex style sounds
   /* 16 */ {"[MUSIC]", deh_procBexMusic},     // bex style music
   /* 17 */ {"", deh_procError} // dummy to handle anything else
};

// flag to skip included deh-style text, used with INCLUDE NOTEXT directive
static boolean includenotext = false;

// MOBJINFO - Dehacked block name = "Thing"
// Usage: Thing nn (name)
// These are for mobjinfo_t types.  Each is an integer
// within the structure, so we can use index of the string in this
// array to offset by sizeof(int) into the mobjinfo_t array at [nn]
// * things are base zero but dehacked considers them to start at #1. ***

char *deh_mobjinfo[DEH_MOBJINFOMAX] =
{
  "ID #",                // .doomednum
  "Initial frame",       // .spawnstate
  "Hit points",          // .spawnhealth
  "First moving frame",  // .seestate
  "Alert sound",         // .seesound
  "Reaction time",       // .reactiontime
  "Attack sound",        // .attacksound
  "Injury frame",        // .painstate
  "Pain chance",         // .painchance
  "Pain sound",          // .painsound
  "Close attack frame",  // .meleestate
  "Far attack frame",    // .missilestate
  "Death frame",         // .deathstate
  "Exploding frame",     // .xdeathstate
  "Death sound",         // .deathsound
  "Speed",               // .speed
  "Width",               // .radius
  "Height",              // .height
  "Mass",                // .mass
  "Missile damage",      // .damage
  "Action sound",        // .activesound
  "Bits",                // .flags
  "Bits2",               // .flags2 haleyjd 04/09/99
  "Respawn frame",       // .raisestate
  "Translucency",        // .translucency  haleyjd 09/01/02
  "Bits3",               // .flags3 haleyjd 02/02/03
  "Blood color",         // .bloodcolor haleyjd 05/08/03
};

// Strings that are used to indicate flags ("Bits" in mobjinfo)
// This is an array of bit masks that are related to p_mobj.h
// values, using the same names without the MF_ in front.
// Ty 08/27/98 new code
//
// killough 10/98:
//
// Convert array to struct to allow multiple values, make array size variable

// haleyjd 11/03/02: isolated struct into a non-anonymous type
// haleyjd 04/10/03: moved struct to d_dehtbl.h for global visibility
// haleyjd 02/19/04: combined into one array for new cflags support --
//                   also changed to be terminated by a zero entry

dehflags_t deh_mobjflags[] =
{
  {"SPECIAL",          0x00000001}, // call  P_Specialthing when touched
  {"SOLID",            0x00000002}, // block movement
  {"SHOOTABLE",        0x00000004}, // can be hit
  {"NOSECTOR",         0x00000008}, // invisible but touchable
  {"NOBLOCKMAP",       0x00000010}, // inert but displayable
  {"AMBUSH",           0x00000020}, // deaf monster
  {"JUSTHIT",          0x00000040}, // will try to attack right back
  {"JUSTATTACKED",     0x00000080}, // take at least 1 step before attacking
  {"SPAWNCEILING",     0x00000100}, // initially hang from ceiling
  {"NOGRAVITY",        0x00000200}, // don't apply gravity during play
  {"DROPOFF",          0x00000400}, // can jump from high places
  {"PICKUP",           0x00000800}, // will pick up items
  {"NOCLIP",           0x00001000}, // goes through walls

  // haleyjd: for combined flags fields, the flags3 SLIDE must take
  // precedence, and thus has to be listed up here. This is a bit of
  // a kludge, but it will preserve compatibility perfectly. The flags
  // SLIDE bit has no effect, as it is never used. I will make a point
  // of avoiding duplicate flag names in the future ;)

  {"SLIDE",            0x00000100, 2}, // mobj slides against walls (for real)  
  {"SLIDE",            0x00002000}, // keep info about sliding along walls
  
  {"FLOAT",            0x00004000}, // allow movement to any height
  {"TELEPORT",         0x00008000}, // don't cross lines or look at heights
  {"MISSILE",          0x00010000}, // don't hit same species, explode on block
  {"DROPPED",          0x00020000}, // dropped, not spawned (like ammo clip)
  {"SHADOW",           0x00040000}, // use fuzzy draw like spectres
  {"NOBLOOD",          0x00080000}, // puffs instead of blood when shot
  {"CORPSE",           0x00100000}, // so it will slide down steps when dead
  {"INFLOAT",          0x00200000}, // float but not to target height
  {"COUNTKILL",        0x00400000}, // count toward the kills total
  {"COUNTITEM",        0x00800000}, // count toward the items total
  {"SKULLFLY",         0x01000000}, // special handling for flying skulls
  {"NOTDMATCH",        0x02000000}, // do not spawn in deathmatch
  
  // killough 10/98: TRANSLATION consists of 2 bits, not 1:

  {"TRANSLATION",      0x04000000}, // for Boom bug-compatibility
  {"TRANSLATION1",     0x04000000}, // use translation table for color (players)
  {"TRANSLATION2",     0x08000000}, // use translation table for color (players)
  {"UNUSED1",          0x08000000}, // unused bit # 1 -- For Boom bug-compatibility
  {"UNUSED2",          0x10000000}, // unused bit # 2 -- For Boom compatibility
  {"UNUSED3",          0x20000000}, // unused bit # 3 -- For Boom compatibility
  {"UNUSED4",          0x40000000}, // unused bit # 4 -- For Boom compatibility
  {"TOUCHY",           0x10000000}, // dies on contact with solid objects (MBF)
  {"BOUNCES",          0x20000000}, // bounces off floors, ceilings and maybe walls
  {"FRIEND",           0x40000000}, // a friend of the player(s) (MBF)
  {"TRANSLUCENT",      0x80000000}, // apply translucency to sprite (BOOM)

  // flags2 bits

  {"LOGRAV",           0x00000001, 1}, // low gravity
  {"NOSPLASH",         0x00000002, 1}, // no splash object
  {"NOSTRAFE",         0x00000004, 1}, // never uses strafing logic
  {"NORESPAWN",        0x00000008, 1}, // never respawns
  {"ALWAYSRESPAWN",    0x00000010, 1}, // always respawns
  {"REMOVEDEAD",       0x00000020, 1}, // removes self after death
  {"NOTHRUST",         0x00000040, 1}, // not affected by external pushers
  {"NOCROSS",          0x00000080, 1}, // cannot trigger special lines
  {"JUMPDOWN",         0x00000100, 1}, // can jump down to follow player
  {"PUSHABLE",         0x00000200, 1}, // pushable
  {"MAP07BOSS1",       0x00000400, 1}, // triggers map07 special 1
  {"MAP07BOSS2",       0x00000800, 1}, // triggers map07 special 2
  {"E1M8BOSS",         0x00001000, 1}, // triggers e1m8 special
  {"E2M8BOSS",         0x00002000, 1}, // triggers e2m8 end
  {"E3M8BOSS",         0x00004000, 1}, // triggers e3m8 end
  {"BOSS",             0x00008000, 1}, // mobj is a boss
  {"E4M6BOSS",         0x00010000, 1}, // triggers e4m6 special
  {"E4M8BOSS",         0x00020000, 1}, // triggers e4m8 special
  {"FOOTCLIP",         0x00040000, 1}, // feet clipped by liquids
  {"FLOATBOB",         0x00080000, 1}, // use floatbob z movement
  {"DONTDRAW",         0x00100000, 1}, // don't draw vissprite
  {"SHORTMRANGE",      0x00200000, 1}, // has shorter missile range
  {"LONGMELEE",        0x00400000, 1}, // has longer melee range
  {"RANGEHALF",        0x00800000, 1}, // considers 1/2 distance
  {"HIGHERMPROB",      0x01000000, 1}, // higher min. missile attack prob.
  {"CANTLEAVEFLOORPIC",0x02000000, 1}, // can't leave floor type
  {"SPAWNFLOAT",       0x04000000, 1}, // spawn @ random float z
  {"INVULNERABLE",     0x08000000, 1}, // mobj is invincible
  {"DORMANT",          0x10000000, 1}, // mobj is dormant
  {"SEEKERMISSILE",    0x20000000, 1}, // internal, may be a tracer
  {"DEFLECTIVE",       0x40000000, 1}, // if reflective, deflects projectiles
  {"REFLECTIVE",       0x80000000, 1}, // mobj reflects projectiles

  // flags3 bits

  {"GHOST",            0x00000001, 2}, // heretic-style ghost
  {"THRUGHOST",        0x00000002, 2}, // passes through ghosts
  {"NODMGTHRUST",      0x00000004, 2}, // doesn't inflict thrust
  {"ACTSEESOUND",      0x00000008, 2}, // uses see sound randomly
  {"LOUDACTIVE",       0x00000010, 2}, // has full-volume activesnd
  {"E5M8BOSS",         0x00000020, 2}, // boss of E5M8
  {"DMGIGNORED",       0x00000040, 2}, // damage is ignored
  {"BOSSIGNORE",       0x00000080, 2}, // ignores damage by others with flag
  // See above for flags3 SLIDE flag
  {"TELESTOMP",        0x00000200, 2}, // can telestomp
  {"WINDTHRUST",       0x00000400, 2}, // affected by heretic wind
  {"FIREDAMAGE",       0x00000800, 2}, // does fire damage
  {"KILLABLE",         0x00001000, 2}, // is killable, but doesn't count
  {"DEADFLOAT",        0x00002000, 2}, // keeps NOGRAVITY when dead
  {"NOTHRESHOLD",      0x00004000, 2}, // has no target threshold
  {"FLOORMISSILE",     0x00008000, 2}, // is a floor missile
  {"SUPERITEM",        0x00010000, 2}, // is a super powerup
  {"NOITEMRESP",       0x00020000, 2}, // won't item respawn
  {"SUPERFRIEND",      0x00040000, 2}, // won't attack other friends
  {"INVULNCHARGE",     0x00080000, 2}, // invincible when skull flying
  {"EXPLOCOUNT",       0x00100000, 2}, // doesn't explode until count expires
  {"CANNOTPUSH",       0x00200000, 2}, // can't push other things
  {"TLSTYLEADD",       0x00400000, 2}, // uses additive translucency
  {"SPACMONSTER",      0x00800000, 2}, // monster that can activate param lines
  {"SPACMISSILE",      0x01000000, 2}, // missile that can activate param lines
  {"NOFRIENDDMG",      0x02000000, 2}, // object isn't hurt by friends
  {"3DDECORATION",     0x04000000, 2}, // object is a decor. with 3D height info

  { NULL,              0 }             // NULL terminator
};

// haleyjd 02/19/04: new dehflagset for combined flags

static dehflagset_t dehacked_flags =
{
   deh_mobjflags, // flaglist
};

// STATE - Dehacked block name = "Frame" and "Pointer"
// Usage: Frame nn
// Usage: Pointer nn (Frame nn)
// These are indexed separately, for lookup to the actual
// function pointers.  Here we'll take whatever Dehacked gives
// us and go from there.  The (Frame nn) after the pointer is the
// real place to put this value.  The "Pointer" value is an xref
// that Dehacked uses and is useless to us.
// * states are base zero and have a dummy #0 (TROO)

char *deh_state[] =
{
  "Sprite number",    // .sprite (spritenum_t) // an enum
  "Sprite subnumber", // .frame (long)
  "Duration",         // .tics (long)
  "Next frame",       // .nextstate (statenum_t)
  // This is set in a separate "Pointer" block from Dehacked
  "Codep Frame",      // pointer to first use of action (actionf_t)
  "Unknown 1",        // .misc1 (long)
  "Unknown 2",        // .misc2 (long)
  "Particle event",   // haleyjd 08/09/02: particle event num
  "Args1",            // haleyjd 08/09/02: arguments
  "Args2",
  "Args3",
  "Args4",
  "Args5",
};

// SFXINFO_STRUCT - Dehacked block name = "Sounds"
// Sound effects, typically not changed (redirected, and new sfx put
// into the pwad, but not changed here.  Can you tell that Greg didn't
// know what they were for, mostly?  Can you tell that I don't either?
// Mostly I just put these into the same slots as they are in the struct.
// This may not be supported in our -deh option if it doesn't make sense by then.

// * sounds are base zero but have a dummy #0

char *deh_sfxinfo[] =
{
  "Offset",     // pointer to a name string, changed in text
  "Zero/One",   // .singularity (int, one at a time flag)
  "Value",      // .priority
  "Zero 1",     // .link (sfxinfo_t*) referenced sound if linked
  "Zero 2",     // .pitch
  "Zero 3",     // .volume
  "Zero 4",     // .data (SAMPLE*) sound data
  "Neg. One 1", // .usefulness
  "Neg. One 2"  // .lumpnum
};

// MUSICINFO is not supported in Dehacked.  Ignored here.
// * music entries are base zero but have a dummy #0

// SPRITE - Dehacked block name = "Sprite"
// Usage = Sprite nn
// Sprite redirection by offset into the text area - unsupported by BOOM
// * sprites are base zero and dehacked uses it that way.

char *deh_sprite[] =
{
  "Offset"      // supposed to be the offset into the text section
};

// AMMO - Dehacked block name = "Ammo"
// usage = Ammo n (name)
// Ammo information for the few types of ammo

char *deh_ammo[] =
{
  "Max ammo",   // maxammo[]
  "Per ammo"    // clipammo[]
};

// WEAPONS - Dehacked block name = "Weapon"
// Usage: Weapon nn (name)
// Basically a list of frames and what kind of ammo (see above)it uses.

char *deh_weapon[] =
{
  "Ammo type",      // .ammo
  "Deselect frame", // .upstate
  "Select frame",   // .downstate
  "Bobbing frame",  // .readystate
  "Shooting frame", // .atkstate
  "Firing frame",   // .flashstate
  "Ammo per shot",  // haleyjd 08/10/02: .ammopershot 
};

// CHEATS - Dehacked block name = "Cheat"
// Usage: Cheat 0
// Always uses a zero in the dehacked file, for consistency.  No meaning.
// These are just plain funky terms compared with id's
//
// killough 4/18/98: integrated into main cheat table now (see st_stuff.c)

// MISC - Dehacked block name = "Misc"
// Usage: Misc 0
// Always uses a zero in the dehacked file, for consistency.  No meaning.

char *deh_misc[] =
{
  "Initial Health",    // initial_health
  "Initial Bullets",   // initial_bullets
  "Max Health",        // maxhealth
  "Max Armor",         // max_armor
  "Green Armor Class", // green_armor_class
  "Blue Armor Class",  // blue_armor_class
  "Max Soulsphere",    // max_soul
  "Soulsphere Health", // soul_health
  "Megasphere Health", // mega_health
  "God Mode Health",   // god_health
  "IDFA Armor",        // idfa_armor
  "IDFA Armor Class",  // idfa_armor_class
  "IDKFA Armor",       // idkfa_armor
  "IDKFA Armor Class", // idkfa_armor_class
  "BFG Cells/Shot",    // BFGCELLS
  "Monsters Infight"   // Unknown--not a specific number it seems, but
  // the logic has to be here somewhere or
  // it'd happen always
};

// TEXT - Dehacked block name = "Text"
// Usage: Text fromlen tolen
// Dehacked allows a bit of adjustment to the length (why?)

// haleyjd: moved text table to d_dehtbl.c

// BEX extension [CODEPTR]
// Usage: Start block, then each line is:
// FRAME nnn = PointerMnemonic

// haleyjd: moved BEX ptr table to d_dehtbl.c

// to hold startup code pointers from INFO.C
// haleyjd: now dynamic for EDF
actionf_t *deh_codeptr;

// ====================================================================
// ProcessDehFile
// Purpose: Read and process a DEH or BEX file
// Args:    filename    -- name of the DEH/BEX file
//          outfilename -- output file (DEHOUT.TXT), appended to here
// Returns: void
//
// killough 10/98:
// substantially modified to allow input from wad lumps instead of .deh files.
//
// haleyjd 09/07/01: this can be called while in video mode now,
// so printf calls needed to be converted to usermsg calls
//
void ProcessDehFile(char *filename, const char *outfilename, int lumpnum)
{
   static FILE *fileout;       // In case -dehout was used
   DWFILE infile, *filein = &infile;    // killough 10/98
   char inbuffer[DEH_BUFFERMAX];  // Place to put the primary infostring

   // Open output file if we're writing output
   if(outfilename && *outfilename && !fileout)
   {
      static boolean firstfile = true; // to allow append to output log
      if(!strcmp(outfilename, "-"))
      {
         fileout = stdout;
      }
      else if(!(fileout=fopen(outfilename, firstfile ? "wt" : "at")))
      {
         usermsg("Could not open -dehout file %s\n... using stdout.",
            outfilename);
         fileout = stdout;
      }
      firstfile = false;
   }

   // killough 10/98: allow DEH files to come from wad lumps
   
   if(filename)
   {
      D_OpenFile(filein, filename, "rt");

      if(!D_IsOpen(filein))
      {
         usermsg("-deh file %s not found", filename);
         return;  // should be checked up front anyway
      }
   }
   else  // DEH file comes from lump indicated by third argument
   {
      D_OpenLump(filein, lumpnum);
      filename = "(WAD)";
   }

   usermsg("Loading DEH file %s",filename);
   if(fileout)
      fprintf(fileout,"\nLoading DEH file %s\n\n",filename);

   {
      static int i;   // killough 10/98: only run once, by keeping index static

      if(!i)
      {
         // haleyjd: allocate dynamically for EDF support
         deh_codeptr = Z_Malloc(sizeof(actionf_t)*NUMSTATES,
                                PU_STATIC, NULL);
      }

      for(; i<NUMSTATES; i++)  // remember what they start as for deh xref
         deh_codeptr[i] = states[i].action;
   }

   deh_loaded = true;
   
   // loop until end of file

   while(D_Fgets(inbuffer, sizeof(inbuffer), filein))
   {
      int i;
      
      lfstrip(inbuffer);
      if(fileout)
         fprintf(fileout,"Line='%s'\n",inbuffer);
      if(!*inbuffer || *inbuffer == '#' || *inbuffer == ' ')
         continue; /* Blank line or comment line */

      // -- If DEH_BLOCKMAX is set right, the processing is independently
      // -- handled based on data in the deh_blocks[] structure array

      // killough 10/98: INCLUDE code rewritten to allow arbitrary nesting,
      // and to greatly simplify code, fix memory leaks, other bugs

      if(!strnicmp(inbuffer,"INCLUDE",7)) // include a file
      {
         // preserve state while including a file
         // killough 10/98: moved to here
         
         char *nextfile;
         boolean oldnotext = includenotext;       // killough 10/98
         
         // killough 10/98: exclude if inside wads (only to discourage
         // the practice, since the code could otherwise handle it)

         if(D_IsLump(filein))
         {
            if(fileout)
               fprintf(fileout,
               "No files may be included from wads: %s\n",inbuffer);
            continue;
         }

         // check for no-text directive, used when including a DEH
         // file but using the BEX format to handle strings

         if(!strnicmp(nextfile = ptr_lstrip(inbuffer+7),"NOTEXT",6))
         {
            includenotext = true; 
            nextfile = ptr_lstrip(nextfile+6);
         }

         if(fileout)
            fprintf(fileout,"Branching to include file %s...\n", nextfile);

         // killough 10/98:
         // Second argument must be NULL to prevent closing fileout too soon
         
         ProcessDehFile(nextfile,NULL,0); // do the included file
         
         includenotext = oldnotext;
         if(fileout)
            fprintf(fileout, "...continuing with %s\n", filename);
         continue;
      }

      for(i = 0; i < DEH_BLOCKMAX; i++)
      {
         if(!strncasecmp(inbuffer,deh_blocks[i].key,strlen(deh_blocks[i].key)))
         { // matches one
            if(fileout)
               fprintf(fileout,"Processing function [%d] for %s\n",
                       i, deh_blocks[i].key);
            deh_blocks[i].fptr(filein,fileout,inbuffer);  // call function
            break;  // we got one, that's enough for this block
         }
      }
   }

   D_Fclose(filein);

   // killough 10/98: only at top recursion level
   // haleyjd 05/21/02: must check fileout for validity!
   if(outfilename)   
   {
      // do not fclose(NULL)
      if(fileout && fileout != stdout)
         fclose(fileout);
      fileout = NULL;
   }
}

// ====================================================================
// deh_procBexCodePointers
// Purpose: Handle [CODEPTR] block, BOOM Extension
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
// haleyjd 03/14/03: rewritten to replace linear search on deh_bexptrs
// table with in-table chained hashing -- table is now in d_dehtbl.c
//
void deh_procBexCodePointers(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   int indexnum;
   char mnemonic[DEH_MAXKEYLEN];  // to hold the codepointer mnemonic
   deh_bexptr *bexptr = NULL; // haleyjd 03/14/03

   // Ty 05/16/98 - initialize it to something, dummy!
   strncpy(inbuffer,line,DEH_BUFFERMAX);

   // for this one, we just read 'em until we hit a blank line
   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if(!*inbuffer)
         break;   // killough 11/98: really exit on blank line

      // killough 8/98: allow hex numbers in input:
      if((3 != sscanf(inbuffer,"%s %i = %s", key, &indexnum, mnemonic))
         || (stricmp(key,"FRAME")) )  // NOTE: different format from normal
      {
         if(fpout) 
         {
            fprintf(fpout,
               "Invalid BEX codepointer line - must start with 'FRAME': '%s'\n",
               inbuffer);
         }
         return;  // early return
      }
      
      // haleyjd: resolve DeHackEd num of state through EDF
      indexnum = E_GetStateNumForDEHNum(indexnum);

      if(fpout)
      {
         fprintf(fpout,"Processing pointer at index %d: %s\n",
                 indexnum, mnemonic);
      }
      
      if(indexnum < 0 || indexnum >= NUMSTATES)
      {
         if(fpout)
         {
            fprintf(fpout,"Bad pointer number %d of %d\n",
               indexnum, NUMSTATES);
         }
         return; // killough 10/98: fix SegViol
      }
      // haleyjd 03/14/03: why do this? how wasteful and useless...
      //strcpy(key,"A_");  // reusing the key area to prefix the mnemonic
      memset(key, 0, DEH_MAXKEYLEN);
      strcat(key, ptr_lstrip(mnemonic));

      // haleyjd 03/14/03: rewrite for hash chaining begins here
      bexptr = D_GetBexPtr(key);

      if(!bexptr)
      {
         if(fpout) 
         {
            fprintf(fpout,
                    "Invalid pointer mnemonic '%s' for frame %d\n",
                    key, indexnum);
         }
      }
      else
      {
         // copy codepointer to state
         states[indexnum].action = bexptr->cptr;
         if(fpout)
         {
            fprintf(fpout,
                    "- applied codepointer %p to states[%d]\n",
                    bexptr->cptr, indexnum);
         }
      }
   }
   return;
}

// ============================================================
// deh_ParseFlags
// Purpose: Handle thing flag fields in a general manner
// Args:    flagset -- pointer to a dehflagset_t object
//          strval  -- ptr-to-ptr to string containing flags
//                     Note: MUST be a mutable string pointer!
//          fpout   -- output file stream
// Returns: Nothing. Results for each parsing mode are written
//          into the corresponding index of the results array
//          within the flagset object.
//
// haleyjd 11/03/02: generalized from code that was previously below
// haleyjd 04/10/03: made global for use in EDF and ExtraData
// haleyjd 02/19/04: rewrote for combined flags support
//
void deh_ParseFlags(dehflagset_t *flagset, char **strval, FILE *fpout)
{
   dehflags_t *flaglist = flagset->flaglist; // get flag list
   long       *results  = flagset->results;  // pointer to results array
   int        mode      = flagset->mode;     // get mode

   // haleyjd: init all results to zero
   memset(results, 0, MAXFLAGFIELDS * sizeof(long));

   // killough 10/98: replace '+' kludge with strtok() loop
   // Fix error-handling case ('found' var wasn't being reset)
   //
   // Use OR logic instead of addition, to allow repetition

   for(;(*strval = strtok(*strval, ",+| \t\f\r")); *strval = NULL)
   {
      int iy = 0;

      while(flaglist[iy].name != NULL)
      {
         int index = flaglist[iy].index;

         if(!strcasecmp(*strval, flaglist[iy].name) &&
            (index == mode || mode == DEHFLAGS_MODE_ALL))
         {
            if(fpout)
            {
               fprintf(fpout, 
                  "ORed value 0x%08lx %s\n",
                  flaglist[iy].value, *strval);
            }
            results[index] |= flaglist[iy].value;
            break;
         }
         ++iy;
      }
      if(flaglist[iy].name == NULL && fpout)
      {
         fprintf(fpout, "Could not find bit mnemonic %s\n", *strval);
      }
   }
}

//
// Functions for external use (ie EDF) -- this prevents the need for
// the flags data above to be global, and simplifies the external
// interface.
//
long deh_ParseFlagsSingle(const char *strval, int mode)
{
   char *buffer;
   char *bufferptr;

   bufferptr = buffer = strdup(strval);

   dehacked_flags.mode = mode;

   deh_ParseFlags(&dehacked_flags, &bufferptr, NULL);

   free(buffer);

   return dehacked_flags.results[mode];
}

long *deh_ParseFlagsCombined(const char *strval)
{
   char *buffer;
   char *bufferptr;

   bufferptr = buffer = strdup(strval);

   dehacked_flags.mode = DEHFLAGS_MODE_ALL;

   deh_ParseFlags(&dehacked_flags, &bufferptr, NULL);

   free(buffer);

   return dehacked_flags.results;
}

#define MOBJFLAGSINDEX  21
#define MOBJFLAGS2INDEX 22
#define MOBJTRANSINDEX  24
#define MOBJFLAGS3INDEX 25

static void SetMobjInfoValue(int mobjInfoIndex, int keyIndex, long value)
{
   mobjinfo_t *mi;

   if(mobjInfoIndex < 0 || mobjInfoIndex >= NUMMOBJTYPES)
      return;

   mi = &mobjinfo[mobjInfoIndex];

   // haleyjd 07/05/03: field resolution adjusted for EDF

   switch(keyIndex)
   {
   case 0: mi->doomednum = (int)value; return;
   case 1: mi->spawnstate = E_GetStateNumForDEHNum(value); return;
   case 2: mi->spawnhealth = (int)value; return;
   case 3: mi->seestate = E_GetStateNumForDEHNum(value); return;
   case 4: mi->seesound = (int)value; return;
   case 5: mi->reactiontime = (int)value; return;
   case 6: mi->attacksound = (int)value; return;
   case 7: mi->painstate = E_GetStateNumForDEHNum(value); return;
   case 8: mi->painchance = (int)value; return;
   case 9: mi->painsound = (int)value; return;
   case 10: mi->meleestate = E_GetStateNumForDEHNum(value); return;
   case 11: mi->missilestate = E_GetStateNumForDEHNum(value); return;
   case 12: mi->deathstate = E_GetStateNumForDEHNum(value); return;
   case 13: mi->xdeathstate = E_GetStateNumForDEHNum(value); return;
   case 14: mi->deathsound = (int)value; return;
   case 15: mi->speed = (int)value; return;
   case 16: mi->radius = (int)value; return;
   case 17: mi->height = (int)value; return;
   case 18: mi->mass = (int)value; return;
   case 19: mi->damage = (int)value; return;
   case 20: mi->activesound = (int)value; return;
   case MOBJFLAGSINDEX: mi->flags = (int)value; return;
   case MOBJFLAGS2INDEX: mi->flags2 = (int)value; return;
   case 23: mi->raisestate = E_GetStateNumForDEHNum(value); return;
   case MOBJTRANSINDEX: mi->translucency = (int)value; return;
   case MOBJFLAGS3INDEX: mi->flags3 = (int)value; return;
   case 26: mi->bloodcolor = (int)value; return;
   default: return;
   } 
}

// ============================================================
// deh_procThing
// Purpose: Handle DEH Thing block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
// Ty 8/27/98 - revised to also allow mnemonics for
// bit masks for monster attributes
//

void deh_procThing(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   int indexnum;
   int ix;
   char *strval;

   strncpy(inbuffer,line,DEH_BUFFERMAX);
   if(fpout) fprintf(fpout,"Thing line: '%s'\n",inbuffer);

   // killough 8/98: allow hex numbers in input:
   ix = sscanf(inbuffer,"%s %i",key, &indexnum);
   if(fpout) fprintf(fpout,"count=%d, Thing %d\n",ix, indexnum);

   // Note that the mobjinfo[] array is base zero, but object numbers
   // in the dehacked file start with one.  Grumble.   
   // haleyjd: not as big an issue with EDF, as it uses a hash lookup
   // --indexnum;  <-- old code

   indexnum = E_GetThingNumForDEHNum(indexnum);

   // now process the stuff
   // Note that for Things we can look up the key and use its offset
   // in the array of key strings as an int offset in the structure
   
   // get a line until a blank or end of file--it's not
   // blank now because it has our incoming key in it
   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);  // toss the end of line

      // killough 11/98: really bail out on blank lines (break != continue)
      if(!*inbuffer) break;  // bail out with blank line between sections
      if(!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
      {
         if(fpout) fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }
      for(ix=0; ix < DEH_MOBJINFOMAX; ix++)
      {
         // haleyjd 02/02/03: restructured to use SetMobjInfoValue,
         // to eliminate indexing of mobjinfo_t structure as an
         // integer array (thanks to prboom for parts of fix)

         if(strcasecmp(key, deh_mobjinfo[ix]))
            continue;

         if(!strcasecmp(key, "bits"))
         {
            if(!value)
            {
               dehacked_flags.mode = DEHFLAGS_MODE1;

               deh_ParseFlags(&dehacked_flags, &strval, fpout);

               value = dehacked_flags.results[DEHFLAGS_MODE1];

               // Don't worry about conversion -- simply print values
               if(fpout)
               {
                  fprintf(fpout, "Bits = 0x%08lX = %ld \n",
                     value, value);
               }
            }

            SetMobjInfoValue(indexnum, MOBJFLAGSINDEX, value);
         }
         else if(!strcasecmp(key, "bits2"))
         {
            // haleyjd 04/09/99: flags2 support
            if(!value)
            {
               dehacked_flags.mode = DEHFLAGS_MODE2;

               deh_ParseFlags(&dehacked_flags, &strval, fpout);

               value = dehacked_flags.results[DEHFLAGS_MODE2];

               if(fpout)
               {
                  fprintf(fpout, "Bits2 = 0x%08lX = %ld \n",
                     value, value);
               }
            }
            
            SetMobjInfoValue(indexnum, MOBJFLAGS2INDEX, value);
         }
         else if(!strcasecmp(key, "bits3"))
         {
            // haleyjd 02/02/03: flags3 support
            if(!value)
            {
               dehacked_flags.mode = DEHFLAGS_MODE3;

               deh_ParseFlags(&dehacked_flags, &strval, fpout);

               value = dehacked_flags.results[DEHFLAGS_MODE3];
               
               if(fpout)
               {
                  fprintf(fpout, "Bits3 = 0x%08lX = %ld \n",
                     value, value);
               }
            }

            SetMobjInfoValue(indexnum, MOBJFLAGS3INDEX, value);
         }
         else
         {
            SetMobjInfoValue(indexnum, ix, value);
         }
         
         if(fpout) fprintf(fpout,"Assigned %d to %s(%d) at index %d\n",
                           (int)value, key, indexnum, ix);

      }
   }
   return;
}

// ====================================================================
// deh_procFrame
// Purpose: Handle DEH Frame block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procFrame(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   int indexnum;

   strncpy(inbuffer,line,DEH_BUFFERMAX);
   
   // killough 8/98: allow hex numbers in input:
   sscanf(inbuffer,"%s %i",key, &indexnum);
   // haleyjd: resolve state number through EDF
   indexnum = E_GetStateNumForDEHNum(indexnum);
   if(fpout)
      fprintf(fpout,"Processing Frame at index %d: %s\n",indexnum,key);
   if(indexnum < 0 || indexnum >= NUMSTATES)
   {
      if(fpout)
         fprintf(fpout,"Bad frame number %d of %d\n",indexnum, NUMSTATES);
   }
   
   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
         break;
      lfstrip(inbuffer);
      if(!*inbuffer) break;         // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
         if(fpout)
            fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }

      // haleyjd 08/09/02: significant reformatting, added new
      // fields
      
      if(!strcasecmp(key,deh_state[0]))  // Sprite number
      {
         if(fpout) fprintf(fpout," - sprite = %ld\n",value);
         states[indexnum].sprite = (spritenum_t)value;
      }
      else if(!strcasecmp(key,deh_state[1]))  // Sprite subnumber
      {
         if(fpout) fprintf(fpout," - frame = %ld\n",value);
         states[indexnum].frame = value; // long
      }
      else if(!strcasecmp(key,deh_state[2]))  // Duration
      {
         if(fpout) fprintf(fpout," - tics = %ld\n",value);
         states[indexnum].tics = value; // long
      }
      else if(!strcasecmp(key,deh_state[3]))  // Next frame
      {
         if(fpout) fprintf(fpout," - nextstate = %ld\n",value);

         // haleyjd: resolve state number through EDF
         //states[indexnum].nextstate = (statenum_t)value;

         states[indexnum].nextstate = E_GetStateNumForDEHNum(value);

      }
      else if(!strcasecmp(key,deh_state[4]))  // Codep frame (not set in Frame deh block)
      {
         if(fpout) fprintf(fpout," - codep, should not be set in Frame section!\n");
         /* nop */ ;
      }
      else if(!strcasecmp(key,deh_state[5]))  // Unknown 1
      {
         if (fpout) fprintf(fpout," - misc1 = %ld\n",value);
         states[indexnum].misc1 = value; // long
      }
      else if(!strcasecmp(key,deh_state[6]))  // Unknown 2
      {
         if(fpout) fprintf(fpout," - misc2 = %ld\n",value);
         states[indexnum].misc2 = value; // long
      }
      else if(!strcasecmp(key,deh_state[7])) // Particle event
      {
         // haleyjd 08/09/02: particle event setting
         if(fpout) fprintf(fpout," - particle_evt = %ld\n",value);
         states[indexnum].particle_evt = value;
      }
      else if(!strcasecmp(key,deh_state[8])) // Args1
      {
         if(fpout) fprintf(fpout," - args[0] = %ld\n",value);
         states[indexnum].args[0] = value;
      }
      else if(!strcasecmp(key,deh_state[9])) // Args2
      {
         if(fpout) fprintf(fpout," - args[1] = %ld\n",value);
         states[indexnum].args[1] = value;
      }
      else if(!strcasecmp(key,deh_state[10])) // Args3
      {
         if(fpout) fprintf(fpout," - args[2] = %ld\n",value);
         states[indexnum].args[2] = value;
      }
      else if(!strcasecmp(key,deh_state[11])) // Args4
      {
         if(fpout) fprintf(fpout," - args[3] = %ld\n",value);
         states[indexnum].args[3] = value;
      }
      else if(!strcasecmp(key,deh_state[12])) // Args5
      {
         if(fpout) fprintf(fpout," - args[4] = %ld\n",value);
         states[indexnum].args[4] = value;
      }
      else if(fpout) 
      {
         fprintf(fpout,"Invalid frame string index for '%s'\n",key);
      }
   }
   return;
}

// ====================================================================
// deh_procPointer
// Purpose: Handle DEH Code pointer block, can use BEX [CODEPTR] instead
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procPointer(DWFILE *fpin, FILE* fpout, char *line) // done
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   int indexnum;
   int i; // looper
   int oldindex; // haleyjd 7/10/03 - preserve for output

   strncpy(inbuffer,line,DEH_BUFFERMAX);
   // NOTE: different format from normal

   // killough 8/98: allow hex numbers in input, fix error case:
   if(sscanf(inbuffer,"%*s %*i (%s %i)",key, &indexnum) != 2)
   {
      if(fpout) fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
      return;
   }

   // haleyjd: resolve state num through EDF; preserve old for output
   oldindex = indexnum;
   indexnum = E_GetStateNumForDEHNum(indexnum);

   if(fpout)
      fprintf(fpout,"Processing Pointer at index %d: %s\n",indexnum, key);
   if(indexnum < 0 || indexnum >= NUMSTATES)
   {
      if (fpout)
         fprintf(fpout,"Bad pointer number %d of %d\n",indexnum, NUMSTATES);
      return;
   }

   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
         break;
      lfstrip(inbuffer);
      if(!*inbuffer) 
         break;       // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
         if(fpout) fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }

      // haleyjd: resolve xref state number through EDF
      value = E_GetStateNumForDEHNum(value);

      if(value < 0 || value >= NUMSTATES)
      {
         if(fpout)
            fprintf(fpout,"Bad pointer number %ld of %d\n",value, NUMSTATES);
         return;
      }

      if(!strcasecmp(key, deh_state[4])) // Codep frame (not set in Frame deh block)
      {
         states[indexnum].action = deh_codeptr[value];
         if(fpout)
            fprintf(fpout," - applied %p from codeptr[%ld] to states[%d]\n",deh_codeptr[value],value,indexnum);
         
         // Write BEX-oriented line to match:
         
         // haleyjd 03/14/03: It's amazing what you can catch just by
         // reformatting some code -- the below line is COMPLETELY
         // incorrect. Must use NUMBEXPTRS, not NUMSTATES.

         // for(i=0;i<NUMSTATES;i++)
         
         for(i = 0; i < num_bexptrs; i++)
         {
            if(deh_bexptrs[i].cptr == deh_codeptr[value])
            {
               if(fpout)
               {
                  // haleyjd 07/05/03: use oldindex for proper #
                  fprintf(fpout,"BEX [CODEPTR] -> FRAME %d = %s\n",
                          oldindex, deh_bexptrs[i].lookup);
               }
               break;
            }
         }
      }
      else
      {
        if(fpout) 
           fprintf(fpout,"Invalid frame pointer index for '%s' at %ld, xref %p\n",
                   key, value, deh_codeptr[value]);
      }
   }
   return;
}

// ====================================================================
// deh_procSounds
// Purpose: Handle DEH Sounds block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procSounds(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   int indexnum;
   sfxinfo_t *sfx;  // haleyjd 09/03/03
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);
   
   // killough 8/98: allow hex numbers in input:
   sscanf(inbuffer,"%s %i",key, &indexnum);

   if(fpout) 
      fprintf(fpout,"Processing Sounds at index %d: %s\n",
              indexnum, key);

   // haleyjd 09/03/03: translate indexnum to sfxinfo_t
   sfx = E_SoundForDEHNum(indexnum);

   if(!sfx)
   {
      if(fpout)
         fprintf(fpout, "Bad sound number %d\n", indexnum);
      return; // haleyjd: bugfix!
   }

   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
         break;
      lfstrip(inbuffer);
      if(!*inbuffer)
         break;         // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
         if(fpout) 
            fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }
      if (!strcasecmp(key,deh_sfxinfo[0]))  // Offset
      {
         /* nop */ ;  // we don't know what this is, I don't think
      }
      else if(!strcasecmp(key,deh_sfxinfo[1]))  // Zero/One
      {
         sfx->singularity = value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[2]))  // Value
      {
         sfx->priority = value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[3]))  // Zero 1
      {
         ; // haleyjd: NO!
         // S_sfx[indexnum].link = (sfxinfo_t *)value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[4]))  // Zero 2
      {
         sfx->pitch = value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[5]))  // Zero 3
      {
         sfx->volume = value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[6]))  // Zero 4
      {
         ; // haleyjd: NO!
         //S_sfx[indexnum].data = (void *)value; // killough 5/3/98: changed cast
      }
      else if(!strcasecmp(key,deh_sfxinfo[7]))  // Neg. One 1
      {
         sfx->usefulness = value;
      }
      else if(!strcasecmp(key,deh_sfxinfo[8]))  // Neg. One 2
      {
         ; // sf: pointless and no longer works
         //S_sfx[indexnum].lumpnum = value;
      }
      else
      {
         if(fpout) 
            fprintf(fpout, "Invalid sound string index for '%s'\n",key);
      }
   }
   return;
}

// ====================================================================
// deh_procAmmo
// Purpose: Handle DEH Ammo block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procAmmo(DWFILE *fpin, FILE* fpout, char *line)
{
  char key[DEH_MAXKEYLEN];
  char inbuffer[DEH_BUFFERMAX];
  long value;      // All deh values are ints or longs
  int indexnum;

  strncpy(inbuffer,line,DEH_BUFFERMAX);

  // killough 8/98: allow hex numbers in input:
  sscanf(inbuffer,"%s %i",key, &indexnum);
  if (fpout) fprintf(fpout,"Processing Ammo at index %d: %s\n",
                     indexnum, key);
  if (indexnum < 0 || indexnum >= NUMAMMO)
    if (fpout) fprintf(fpout,"Bad ammo number %d of %d\n",
                       indexnum,NUMAMMO);

  while (!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
    {
      if (!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if (!*inbuffer) break;       // killough 11/98
      if (!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
        {
          if (fpout) fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
          continue;
        }
      if (!strcasecmp(key,deh_ammo[0]))  // Max ammo
        maxammo[indexnum] = value;
      else
        if (!strcasecmp(key,deh_ammo[1]))  // Per ammo
          clipammo[indexnum] = value;
        else
          if (fpout) fprintf(fpout,"Invalid ammo string index for '%s'\n",key);
    }
  return;
}

// ====================================================================
// deh_procWeapon
// Purpose: Handle DEH Weapon block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procWeapon(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   int indexnum;

   // haleyjd 08/10/02: significant reformatting
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);
   
   // killough 8/98: allow hex numbers in input:
   sscanf(inbuffer,"%s %i",key, &indexnum);
   if(fpout) 
      fprintf(fpout,"Processing Weapon at index %d: %s\n",
              indexnum, key);
   if(indexnum < 0 || indexnum >= NUMWEAPONS)
   {
      if(fpout)
         fprintf(fpout,"Bad weapon number %d of %d\n",
                 indexnum, NUMWEAPONS);
   }
      
   while (!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if(!*inbuffer) break;       // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
         if(fpout) 
            fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }
      // haleyjd: resolution adjusted for EDF
      if(!strcasecmp(key,deh_weapon[0]))  // Ammo type
         weaponinfo[indexnum].ammo = value;
      else if(!strcasecmp(key,deh_weapon[1]))  // Deselect frame
         weaponinfo[indexnum].upstate = E_GetStateNumForDEHNum(value);
      else if(!strcasecmp(key,deh_weapon[2]))  // Select frame
         weaponinfo[indexnum].downstate = E_GetStateNumForDEHNum(value);
      else if(!strcasecmp(key,deh_weapon[3]))  // Bobbing frame
         weaponinfo[indexnum].readystate = E_GetStateNumForDEHNum(value);
      else if(!strcasecmp(key,deh_weapon[4]))  // Shooting frame
         weaponinfo[indexnum].atkstate = E_GetStateNumForDEHNum(value);
      else if(!strcasecmp(key,deh_weapon[5]))  // Firing frame
         weaponinfo[indexnum].flashstate = E_GetStateNumForDEHNum(value);
      else if(!strcasecmp(key,deh_weapon[6])) // haleyjd: Ammo per shot
      {
         weaponinfo[indexnum].ammopershot = value;
         // enable ammo per shot value usage for this weapon
         weaponinfo[indexnum].enableaps = 1;
      }
      else if(fpout)
         fprintf(fpout,"Invalid weapon string index for '%s'\n",key);
   }
   return;
}

// ====================================================================
// deh_procSprite
// Purpose: Dummy - we do not support the DEH Sprite block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procSprite(DWFILE *fpin, FILE* fpout, char *line) // Not supported
{
  char key[DEH_MAXKEYLEN];
  char inbuffer[DEH_BUFFERMAX];
  int indexnum;

  // Too little is known about what this is supposed to do, and
  // there are better ways of handling sprite renaming.  Not supported.

  strncpy(inbuffer,line,DEH_BUFFERMAX);

  // killough 8/98: allow hex numbers in input:
  sscanf(inbuffer,"%s %i",key, &indexnum);
  if (fpout) fprintf(fpout,
                     "Ignoring Sprite offset change at index %d: %s\n",indexnum, key);
  while (!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
    {
      if (!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if (!*inbuffer) break;      // killough 11/98
      // ignore line
      if (fpout) fprintf(fpout,"- %s\n",inbuffer);
    }
  return;
}

// ====================================================================
// deh_procPars
// Purpose: Handle BEX extension for PAR times
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procPars(DWFILE *fpin, FILE* fpout, char *line) // extension
{
  char key[DEH_MAXKEYLEN];
  char inbuffer[DEH_BUFFERMAX];
  int indexnum;
  int episode, level, partime, oldpar;

  // new item, par times
  // usage: After [PARS] Par 0 section identifier, use one or more of these
  // lines:
  //  par 3 5 120
  //  par 14 230
  // The first would make the par for E3M5 be 120 seconds, and the
  // second one makes the par for MAP14 be 230 seconds.  The number
  // of parameters on the line determines which group of par values
  // is being changed.  Error checking is done based on current fixed
  // array sizes of[4][10] and [32]

  strncpy(inbuffer,line,DEH_BUFFERMAX);

  // killough 8/98: allow hex numbers in input:
  sscanf(inbuffer,"%s %i",key, &indexnum);
  if (fpout) fprintf(fpout,
                     "Processing Par value at index %d: %s\n",indexnum, key);
  // indexnum is a dummy entry
  while (!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
    {
      if (!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(M_Strlwr(inbuffer)); // lowercase it
      if (!*inbuffer) break;      // killough 11/98
      if (3 != sscanf(inbuffer,"par %i %i %i",&episode, &level, &partime))
        { // not 3
          if (2 != sscanf(inbuffer,"par %i %i",&level, &partime))
            { // not 2
              if (fpout) fprintf(fpout,"Invalid par time setting string: %s\n",inbuffer);
            }
          else
            { // is 2
              // Ty 07/11/98 - wrong range check, not zero-based
              if (level < 1 || level > 32) // base 0 array (but 1-based parm)
                {
                  if (fpout) fprintf(fpout,"Invalid MAPnn value MAP%d\n",level);
                }
              else
                {
                  oldpar = cpars[level-1];
                  if (fpout) fprintf(fpout,"Changed par time for MAP%02d from %d to %d\n",level,oldpar,partime);
                  cpars[level-1] = partime;
                  deh_pars = true;
                }
            }
        }
      else
        { // is 3
          // note that though it's a [4][10] array, the "left" and "top" aren't used,
          // effectively making it a base 1 array.
          // Ty 07/11/98 - level was being checked against max 3 - dumb error
          // Note that episode 4 does not have par times per original design
          // in Ultimate DOOM so that is not supported here.
          if (episode < 1 || episode > 3 || level < 1 || level > 9)
            {
              if (fpout) fprintf(fpout,
                                 "Invalid ExMx values E%dM%d\n",episode, level);
            }
          else
            {
              oldpar = pars[episode][level];
              pars[episode][level] = partime;
              if (fpout) fprintf(fpout,
                                 "Changed par time for E%dM%d from %d to %d\n",
                                 episode,level,oldpar,partime);
              deh_pars = true;
            }
        }
    }
  return;
}

// ====================================================================
// deh_procCheat
// Purpose: Handle DEH Cheat block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procCheat(DWFILE *fpin, FILE* fpout, char *line) // done
{
  char key[DEH_MAXKEYLEN];
  char inbuffer[DEH_BUFFERMAX];
  long value;      // All deh values are ints or longs
  char *strval = "";  // pointer to the value area
  int ix, iy;   // array indices
  char *p;  // utility pointer

  if (fpout) fprintf(fpout,"Processing Cheat: %s\n",line);

  strncpy(inbuffer,line,DEH_BUFFERMAX);
  while (!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
    {
      if (!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if (!*inbuffer) break;       // killough 11/98
      if (!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
        {
          if (fpout) fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
          continue;
        }
      // Otherwise we got a (perhaps valid) cheat name,
      // so look up the key in the array

      // killough 4/18/98: use main cheat code table in st_stuff.c now
      for (ix=0; cheat[ix].cheat; ix++)
        if (cheat[ix].deh_cheat)   // killough 4/18/98: skip non-deh
          {
            if (!stricmp(key,cheat[ix].deh_cheat))  // found the cheat, ignored case
              {
                // replace it but don't overflow it.  Use current length as limit.
                // Ty 03/13/98 - add 0xff code
                // Deal with the fact that the cheats in deh files are extended
                // with character 0xFF to the original cheat length, which we don't do.
                for (iy=0; strval[iy]; iy++)
                  strval[iy] = (strval[iy]==(char)0xff) ? '\0' : strval[iy];

                iy = ix;     // killough 4/18/98

                // Ty 03/14/98 - skip leading spaces
                p = strval;
                while (*p == ' ') ++p;
                // Ty 03/16/98 - change to use a strdup and orphan the original
                // Also has the advantage of allowing length changes.
                // strncpy(cheat[iy].cheat,p,strlen(cheat[iy].cheat));
                {    // killough 9/12/98: disable cheats which are prefixes of this one
                  int i;
                  for (i=0; cheat[i].cheat; i++)
                    if (cheat[i].when & not_deh &&
                        !strncasecmp(cheat[i].cheat,
                                     cheat[iy].cheat,
                                     strlen(cheat[i].cheat)) && i != iy)
                      cheat[i].deh_modified = true;
                }
                cheat[iy].cheat = strdup(p);
                if (fpout) fprintf(fpout,
                                   "Assigned new cheat '%s' to cheat '%s'at index %d\n",
                                   p, cheat[ix].deh_cheat, iy); // killough 4/18/98
              }
          }
      if (fpout) fprintf(fpout,"- %s\n",inbuffer);
    }
  return;
}

// ====================================================================
// deh_procMisc
// Purpose: Handle DEH Misc block
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procMisc(DWFILE *fpin, FILE* fpout, char *line) // done
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);
   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin)) break;
      lfstrip(inbuffer);
      if(!*inbuffer)
         break;    // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
         if(fpout)
            fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
         continue;
      }
      
      // Otherwise it's ok
      if(fpout)
         fprintf(fpout,"Processing Misc item '%s'\n", key);
      
      if(!strcasecmp(key,deh_misc[0]))       // Initial Health
         initial_health = value;
      else if(!strcasecmp(key,deh_misc[1]))  // Initial Bullets
         initial_bullets = value;
      else if(!strcasecmp(key,deh_misc[2]))  // Max Health
         maxhealth = value;
      else if(!strcasecmp(key,deh_misc[3]))  // Max Armor
         max_armor = value;
      else if(!strcasecmp(key,deh_misc[4]))  // Green Armor Class
         green_armor_class = value;
      else if(!strcasecmp(key,deh_misc[5]))  // Blue Armor Class
         blue_armor_class = value;
      else if(!strcasecmp(key,deh_misc[6]))  // Max Soulsphere
         max_soul = value;
      else if(!strcasecmp(key,deh_misc[7]))  // Soulsphere Health
         soul_health = value;
      else if(!strcasecmp(key,deh_misc[8]))  // Megasphere Health
         mega_health = value;
      else if(!strcasecmp(key,deh_misc[9]))  // God Mode Health
         god_health = value;
      else if(!strcasecmp(key,deh_misc[10])) // IDFA Armor
         idfa_armor = value;
      else if(!strcasecmp(key,deh_misc[11])) // IDFA Armor Class
         idfa_armor_class = value;
      else if(!strcasecmp(key,deh_misc[12])) // IDKFA Armor
         idkfa_armor = value;
      else if(!strcasecmp(key,deh_misc[13])) // IDKFA Armor Class
         idkfa_armor_class = value;
      else if(!strcasecmp(key,deh_misc[14])) // BFG Cells/Shot
      {
         // haleyjd 08/10/02: propagate to weapon info
         bfgcells = weaponinfo[wp_bfg].ammopershot = value;
      }
      else if(!strcasecmp(key,deh_misc[15])) // Monsters Infight
         /* No such switch in DOOM - nop */ 
         ;
      else if(fpout)
         fprintf(fpout, 
                 "Invalid misc item string index for '%s'\n",key);
   }
   return;
}

// ====================================================================
// deh_procText
// Purpose: Handle DEH Text block
// Notes:   We look things up in the current information and if found
//          we replace it.  At the same time we write the new and
//          improved BEX syntax to the log file for future use.
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procText(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX*2];  // can't use line -- double size buffer too.
   int i;               // loop variable
   unsigned int fromlen, tolen;  // as specified on the text block line
   int usedlen;         // shorter of fromlen and tolen if not matched
   boolean found = FALSE;  // to allow early exit once found
   char* line2 = NULL;     // duplicate line for rerouting

   // Ty 04/11/98 - Included file may have NOTEXT skip flag set
   if(includenotext) // flag to skip included deh-style text
   {
      if(fpout) 
      {
         fprintf(fpout,
            "Skipped text block because of notext directive\n");
      }
      strcpy(inbuffer,line);
      while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
         D_Fgets(inbuffer, sizeof(inbuffer), fpin);  // skip block
      // Ty 05/17/98 - don't care if this fails
      return; // early return
   }

   // killough 8/98: allow hex numbers in input:
   sscanf(line,"%s %i %i",key,&fromlen,&tolen);
   if(fpout)
   {
      fprintf(fpout,
         "Processing Text (key=%s, from=%d, to=%d)\n",
         key, fromlen, tolen);
   }

   // killough 10/98: fix incorrect usage of feof
   {
      int c;
      unsigned int totlen = 0;

      while(totlen < fromlen + tolen && (c = D_Fgetc(fpin)) != EOF)
         inbuffer[totlen++] = c;
      inbuffer[totlen]='\0';
   }

   // if the from and to are 4, this may be a sprite rename.  Check it
   // against the array and process it as such if it matches.  Remember
   // that the original names are (and should remain) uppercase.
   if (fromlen==4 && tolen==4)
   {
      i = 0;
      while (sprnames[i])  // null terminated list in info.c //jff 3/19/98
      {                                                      //check pointer
         if(!strnicmp(sprnames[i],inbuffer,fromlen))         //not first char
         {
            if(fpout)
            {
               fprintf(fpout,
                  "Changing name of sprite at index %d from %s to %*s\n",
                  i,sprnames[i],tolen,&inbuffer[fromlen]);
            }
            // Ty 03/18/98 - not using strdup because length is fixed
            
            // killough 10/98: but it's an array of pointers, so we must
            // use strdup unless we redeclare sprnames and change all else
            
            // haleyjd 03/11/03: can now use the original
            // sprnames[i] = strdup(sprnames[i]);

            strncpy(sprnames[i],&inbuffer[fromlen],tolen);
            found = TRUE;
            break;  // only one will match--quit early
         }
         ++i;  // next array element
      }
   }
   else if(fromlen < 7 && tolen < 7) // lengths of music and sfx are 6 or shorter
   {
      usedlen = (fromlen < tolen) ? fromlen : tolen;
      if (fromlen != tolen)
      {
         if (fpout)
         {
            fprintf(fpout,
               "Warning: Mismatched lengths from=%d, to=%d, used %d\n",
               fromlen, tolen, usedlen);
          }
      }
      // Try sound effects entries - see sounds.c
      for(i = 1; i < NUMSFX; i++)
      {
         // avoid short prefix erroneous match
         if(strlen(S_sfx[i].name) != fromlen)
            continue;
         if(!strnicmp(S_sfx[i].name,inbuffer,fromlen))
         {
            if(fpout)
            {
               fprintf(fpout,
                  "Changing name of sfx from %s to %*s\n",
                  S_sfx[i].name,usedlen,&inbuffer[fromlen]);
            }

            // haleyjd 09/03/03: changed to strncpy
            memset(S_sfx[i].name, 0, 9);
            strncpy(S_sfx[i].name, &inbuffer[fromlen], 9);
            found = TRUE;
            break;  // only one matches, quit early
         }
      }
      if(!found)  // not yet
      {
         // Try music name entries - see sounds.c
         for(i = 1; i < NUMMUSIC; i++)
         {
            // avoid short prefix erroneous match
            if(strlen(S_music[i].name) != fromlen) continue;
            if(!strnicmp(S_music[i].name,inbuffer,fromlen))
            {
               if(fpout)
               {
                  fprintf(fpout,
                     "Changing name of music from %s to %*s\n",
                     S_music[i].name,usedlen,&inbuffer[fromlen]);
               }

               S_music[i].name = strdup(&inbuffer[fromlen]);
               found = TRUE;
               break;  // only one matches, quit early
            }
         }
      }  // end !found test
   }

   if(!found) // Nothing we want to handle here--see if strings can deal with it.
   {
      if(fpout)
      {
         fprintf(fpout,
            "Checking text area through strings for '%.12s%s' from=%d to=%d\n",
            inbuffer, 
            (strlen(inbuffer) > 12) ? "..." : "",
            fromlen,tolen);
      }
      if(fromlen <= strlen(inbuffer))
      {
         line2 = strdup(&inbuffer[fromlen]);
         inbuffer[fromlen] = '\0';
      }
      
      deh_procStringSub(NULL, inbuffer, line2, fpout);
   }
   free(line2); // may be NULL, ignored by free()
   return;
}

void deh_procError(DWFILE *fpin, FILE* fpout, char *line)
{
   char inbuffer[DEH_BUFFERMAX];
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);
   if(fpout)
      fprintf(fpout, "Unmatched Block: '%s'\n", inbuffer);
   return;
}
   
// ============================================================
// deh_procStrings
// Purpose: Handle BEX [STRINGS] extension
// Args:    fpin  -- input file stream
//          fpout -- output file stream (DEHOUT.TXT)
//          line  -- current line in file to process
// Returns: void
//
void deh_procStrings(DWFILE *fpin, FILE* fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;    // All deh values are ints or longs
   char *strval;      // holds the string value of the line
   // holds the final result of the string after concatenation
   static char *holdstring = NULL;
   static unsigned int maxstrlen = 128; // maximum string length, bumped 128 at
                                        // a time as needed
   boolean found = false;  // looking for string continuation

   if(fpout)
      fprintf(fpout, "Processing extended string substitution\n");

   if(!holdstring)
      holdstring = malloc(maxstrlen * sizeof(*holdstring));

   *holdstring = '\0'; // empty string to start with

   strncpy(inbuffer,line,DEH_BUFFERMAX);

   // Ty 04/24/98 - have to allow inbuffer to start with a blank for
   // the continuations of C1TEXT etc.
   while(!D_Feof(fpin) && *inbuffer)  /* && (*inbuffer != ' ') */
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
         break;
      if(*inbuffer == '#')
         continue;  // skip comment lines
      lfstrip(inbuffer);
      if(!*inbuffer) 
         break;  // killough 11/98
      if(!*holdstring) // first one--get the key
      {
         if(!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
         {
            if(fpout)
               fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
            continue;
         }
      }
      while(strlen(holdstring) + strlen(inbuffer) > maxstrlen) // Ty03/29/98 - fix stupid error
      {
         // killough 11/98: allocate enough the first time
         maxstrlen += strlen(holdstring) + strlen(inbuffer) - maxstrlen;
         if(fpout)
         {
            fprintf(fpout,
               "* increased buffer from to %d for buffer size %d\n",
               maxstrlen,(int)strlen(inbuffer));
         }
         holdstring = realloc(holdstring,maxstrlen*sizeof(*holdstring));
      }
      // concatenate the whole buffer if continuation or the value iffirst
      strcat(holdstring,ptr_lstrip(((*holdstring) ? inbuffer : strval)));
      rstrip(holdstring);
      // delete any trailing blanks past the backslash
      // note that blanks before the backslash will be concatenated
      // but ones at the beginning of the next line will not, allowing
      // indentation in the file to read well without affecting the
      // string itself.
      if(holdstring[strlen(holdstring)-1] == '\\')
      {
         holdstring[strlen(holdstring)-1] = '\0';
         continue; // ready to concatenate
      }
      if(*holdstring) // didn't have a backslash, trap above would catch that
      {
         // go process the current string
         found = deh_procStringSub(key, NULL, holdstring, fpout);  // supply key and not search string

          if(!found)
          {
             if(fpout) 
             {
                fprintf(fpout,
                   "Invalid string key '%s', substitution skipped.\n",
                   key);
             }
          }

          *holdstring = '\0';  // empty string for the next one
      }
   }

   return;
}

// ====================================================================
// deh_procStringSub
// Purpose: Common string parsing and handling routine for DEH and BEX
// Args:    key       -- place to put the mnemonic for the string if found
//          lookfor   -- original value string to look for
//          newstring -- string to put in its place if found
//          fpout     -- file stream pointer for log file (DEHOUT.TXT)
// Returns: boolean: True if string found, false if not
//
// haleyjd 11/02/02: rewritten to replace linear search on string
// table with in-table chained hashing -- table is now in d_dehtbl.c
//
boolean deh_procStringSub(char *key, char *lookfor, char *newstring,
                          FILE *fpout)
{
   deh_strs *dehstr = NULL;

   if(lookfor)
      dehstr = D_GetDEHStr(lookfor);
   else
      dehstr = D_GetBEXStr(key);

   if(!dehstr)
   {
      if(fpout)
      {
         fprintf(fpout,
            "Could not find '%.12s'\n", key ? key : lookfor);
      }
      return false;
   }

   *dehstr->ppstr = strdup(newstring); // orphan original string

   // Handle embedded \n's in the incoming string, convert to 0x0a's
   {
      char *s, *t;
      for(s=t=*dehstr->ppstr; *s; ++s, ++t)
      {
         if (*s == '\\' && (s[1] == 'n' || s[1] == 'N')) //found one
            ++s, *t = '\n';  // skip one extra for second character
         else
            *t = *s;
      }
      *t = '\0';  // cap off the target string
   }

   if(key)
   {
      if(fpout)
      {
         fprintf(fpout,
            "Assigned key %s => '%s'\n",key,newstring);
      }
   }
   else
   {
      if(fpout)
      {
         fprintf(fpout,
            "Assigned '%.12s%s' to'%.12s%s' at key %s\n",
            lookfor, (strlen(lookfor) > 12) ? "..." : "",
            newstring, (strlen(newstring) > 12) ? "..." :"",
            dehstr->lookup);

         // must have passed an old style string so show BEX
         fprintf(fpout,
            "*BEX FORMAT:\n%s=%s\n*END BEX\n",
            dehstr->lookup,
            dehReformatStr(newstring));
      }
   }

   return true;
}

//=============================================================
// haleyjd 9/22/99
//
// deh_procHelperThing
//
// Allows handy substitution of any thing for helper dogs.  DEH 
// patches are being made frequently for this purpose and it 
// requires a complete rewiring of the DOG thing.  I feel this 
// is a waste of effort, and so have added this new [HELPER] 
// BEX block

void deh_procHelperThing(DWFILE *fpin, FILE *fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;      // All deh values are ints or longs

   strncpy(inbuffer,line,DEH_BUFFERMAX);
   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
         break;
      
      lfstrip(inbuffer);
      if(!*inbuffer)
         break;    
      
      if(!deh_GetData(inbuffer,key,&value,NULL,fpout)) // returns TRUE if ok
      {
          if(fpout)
             fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
          continue;
      }
      // Otherwise it's ok
      if(fpout)
      {
        fprintf(fpout,"Processing Helper Thing item '%s'\n", key);
        fprintf(fpout,"value is %i", (int)value);
      }      
      if(!strncasecmp(key, "type", 4))
        HelperThing = E_ThingNumForDEHNum((int)value);
  }
  return;
}

//
// deh_procBexSprites
//
// Supports sprite name substitutions without requiring use
// of the DeHackEd Text block
//
void deh_procBexSprites(DWFILE *fpin, FILE *fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;    // All deh values are ints or longs
   char *strval;  // holds the string value of the line
   char candidate[5];
   int  rover;

   if(fpout)
      fprintf(fpout,"Processing sprite name substitution\n");
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);

   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
	 break;
      if(*inbuffer == '#')
	 continue;  // skip comment lines
      lfstrip(inbuffer);
      if(!*inbuffer) 
	 break;  // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
      {
	 if(fpout)
	    fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
	 continue;
      }
      // do it
      memset(candidate, 0, 7);
      strncpy(candidate, ptr_lstrip(strval), 4);
      if(strlen(candidate) != 4)
      {
	 if(fpout)
	    fprintf(fpout, "Bad length for sprite name '%s'\n",
	            candidate);
	 continue;
      }

      rover = 0;
      while(deh_spritenames[rover])
      {
	 if(!strncasecmp(deh_spritenames[rover], key, 4))
	 {
	    if(fpout)
	       fprintf(fpout, "Substituting '%s' for sprite '%s'\n",
	               candidate, deh_spritenames[rover]);

            // haleyjd 03/11/03: can now use original due to EDF
	    // sprnames[rover] = strdup(candidate);
            strncpy(sprnames[rover], candidate, 4);
	    break;
	 }
	 rover++;
      }
   }
}

// ditto for sound names
void deh_procBexSounds(DWFILE *fpin, FILE *fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;    // All deh values are ints or longs
   char *strval;  // holds the string value of the line
   char candidate[9];
   int  len;
   sfxinfo_t *sfx;

   // haleyjd 09/03/03: rewritten to work with EDF
   
   if(fpout)
      fprintf(fpout,"Processing sound name substitution\n");
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);

   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
	 break;
      if(*inbuffer == '#')
	 continue;  // skip comment lines
      lfstrip(inbuffer);
      if(!*inbuffer) 
	 break;  // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
      {
	 if(fpout)
	    fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
	 continue;
      }
      // do it
      memset(candidate, 0, 9);
      strncpy(candidate, ptr_lstrip(strval), 9);
      len = strlen(candidate);
      if(len < 1 || len > 8)
      {
	 if(fpout)
	    fprintf(fpout, "Bad length for sound name '%s'\n",
	            candidate);
	 continue;
      }

      sfx = E_SoundForName(key);

      if(!sfx)
      {
         if(fpout)
            fprintf(fpout, "Bad sound mnemonic '%s'\n", key);
         continue;
      }

      if(fpout)
         fprintf(fpout, "Substituting '%s' for sound '%s'\n",
                 candidate, sfx->mnemonic);

      strncpy(sfx->name, candidate, 9);
   }
}

// ditto for music names
void deh_procBexMusic(DWFILE *fpin, FILE *fpout, char *line)
{
   char key[DEH_MAXKEYLEN];
   char inbuffer[DEH_BUFFERMAX];
   long value;    // All deh values are ints or longs
   char *strval;  // holds the string value of the line
   char candidate[7];
   int  rover, len;
   
   if(fpout)
      fprintf(fpout,"Processing music name substitution\n");
   
   strncpy(inbuffer,line,DEH_BUFFERMAX);

   while(!D_Feof(fpin) && *inbuffer && (*inbuffer != ' '))
   {
      if(!D_Fgets(inbuffer, sizeof(inbuffer), fpin))
	 break;
      if(*inbuffer == '#')
	 continue;  // skip comment lines
      lfstrip(inbuffer);
      if(!*inbuffer) 
	 break;  // killough 11/98
      if(!deh_GetData(inbuffer,key,&value,&strval,fpout)) // returns TRUE if ok
      {
	 if(fpout)
	    fprintf(fpout,"Bad data pair in '%s'\n",inbuffer);
	 continue;
      }
      // do it
      memset(candidate, 0, 7);
      strncpy(candidate, ptr_lstrip(strval), 6);
      len = strlen(candidate);
      if(len < 1 || len > 6)
      {
	 if(fpout)
	    fprintf(fpout, "Bad length for music name '%s'\n",
	            candidate);
	 continue;
      }

      rover = 1;
      while(deh_musicnames[rover])
      {
	 if(!strncasecmp(deh_musicnames[rover], key, 6))
	 {
	    if(fpout)
	       fprintf(fpout, "Substituting '%s' for music '%s'\n",
	               candidate, deh_musicnames[rover]);

	    S_music[rover].name = strdup(candidate);
	    break;
	 }
	 rover++;
      }
   }
}

// ====================================================================
// General utility function(s)
// ====================================================================

// ====================================================================
// dehReformatStr
// Purpose: Convert a string into a continuous string with embedded
//          linefeeds for "\n" sequences in the source string
// Args:    string -- the string to convert
// Returns: the converted string (converted in a static buffer)
//
static char *dehReformatStr(char *string)
{
  static char buff[DEH_BUFFERMAX]; // only processing the changed string,
  //  don't need double buffer
  char *s, *t;

  s = string;  // source
  t = buff;    // target
  // let's play...

  while (*s)
    {
      if (*s == '\n')
        ++s, *t++ = '\\', *t++ = 'n', *t++ = '\\', *t++='\n'; 
      else
        *t++ = *s++;
    }
  *t = '\0';
  return buff;
}

// ====================================================================
// lfstrip
// Purpose: Strips CR/LF off the end of a string
// Args:    s -- the string to work on
// Returns: void -- the string is modified in place
//
// killough 10/98: only strip at end of line, not entire string

void lfstrip(char *s)  // strip the \r and/or \n off of a line
{
  char *p = s+strlen(s);
  while (p > s && (*--p=='\r' || *p=='\n'))
    *p = 0;
}

// ====================================================================
// rstrip
// Purpose: Strips trailing blanks off a string
// Args:    s -- the string to work on
// Returns: void -- the string is modified in place
//
void rstrip(char *s)  // strip trailing whitespace
{
  char *p = s+strlen(s);         // killough 4/4/98: same here
  while (p > s && isspace(*--p)) // break on first non-whitespace
    *p='\0';
}

// ====================================================================
// ptr_lstrip
// Purpose: Points past leading whitespace in a string
// Args:    s -- the string to work on
// Returns: char * pointing to the first nonblank character in the
//          string.  The original string is not changed.
//
char *ptr_lstrip(char *p)  // point past leading whitespace
{
  while (isspace(*p))
    p++;
  return p;
}

// ====================================================================
// deh_GetData
// Purpose: Get a key and data pair from a passed string
// Args:    s -- the string to be examined
//          k -- a place to put the key
//          l -- pointer to a long integer to store the number
//          strval -- a pointer to the place in s where the number
//                    value comes from.  Pass NULL to not use this.
//          fpout  -- stream pointer to output log (DEHOUT.TXT)
// Notes:   Expects a key phrase, optional space, equal sign,
//          optional space and a value, mostly an int but treated
//          as a long just in case.  The passed pointer to hold
//          the key must be DEH_MAXKEYLEN in size.

boolean deh_GetData(char *s, char *k, long *l, char **strval, FILE *fpout)
{
  char *t;  // current char
  long val; // to hold value of pair
  char buffer[DEH_MAXKEYLEN];  // to hold key in progress
  boolean okrc = TRUE;  // assume good unless we have problems
  int i;  // iterator

  *buffer = '\0';
  val = 0;  // defaults in case not otherwise set
  for (i=0, t=s; *t && i < DEH_MAXKEYLEN; t++, i++)
    {
      if (*t == '=') break;
      buffer[i] = *t;  // copy it
    }
  buffer[--i] = '\0';  // terminate the key before the '='
  if (!*t)  // end of string with no equal sign
    {
      okrc = FALSE;
    }
  else
    {
      if (!*++t)
        {
          val = 0;  // in case "thiskey =" with no value
          okrc = FALSE;
        }
      // we've incremented t
      val = strtol(t,NULL,0);  // killough 8/9/98: allow hex or octal input
    }

  // go put the results in the passed pointers
  *l = val;  // may be a faked zero

  // if spaces between key and equal sign, strip them
  strcpy(k,ptr_lstrip(buffer));  // could be a zero-length string

  if (strval != NULL) // pass NULL if you don't want this back
    *strval = t;      // pointer, has to be somewhere in s,
  // even if pointing at the zero byte.

  return(okrc);
}

//---------------------------------------------------------------------
//
// $Log: d_deh.c,v $
// Revision 1.20  1998/06/01  22:30:38  thldrmn
// fix .acv pointer for new GCC version
//
// Revision 1.19  1998/05/17  09:39:48  thldrmn
// Bug fix to avoid processing last line twice
//
// Revision 1.17  1998/05/04  21:36:21  thldrmn
// commenting, reformatting and savegamename change
//
// Revision 1.16  1998/05/03  22:09:59  killough
// use p_inter.h for extern declarations and fix a pointer cast
//
// Revision 1.15  1998/04/26  14:46:24  thldrmn
// BEX code pointer additions
//
// Revision 1.14  1998/04/24  23:49:35  thldrmn
// Strings continuation fix
//
// Revision 1.13  1998/04/19  01:18:58  killough
// Change deh cheat code handling to use new cheat table
//
// Revision 1.12  1998/04/11  14:47:31  thldrmn
// Added include, fixed pars
//
// Revision 1.11  1998/04/10  06:49:15  killough
// Fix CVS stuff
//
// Revision 1.10  1998/04/09  09:17:00  thldrmn
// Update to text handling
//
// Revision 1.00  1998/04/07  04:43:59  ty
// First time with cvs revision info
//
//---------------------------------------------------------------------
