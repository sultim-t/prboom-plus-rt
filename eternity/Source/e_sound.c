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
// EDF Sound Module
//
// Maintains the globally-used sound hash tables, for lookup by
// assigned mnemonics and DeHackEd numbers. EDF-defined sounds are
// processed and linked into the tables first. Any wad lumps with 
// names starting with DS* are later added as the wads that contain 
// them are loaded.
//
// Note that wad sounds can't be referred to via DeHackEd, which
// hasn't changed since this functionality was implemented in s_sound.c.
// The functions in s_sound.c for sound hashing now call down to these
// functions.
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_io.h"
#include "i_system.h"
#include "w_wad.h"
#include "d_dehtbl.h"
#include "sounds.h"
#include "i_sound.h"
#include "p_mobj.h"
#include "s_sound.h"

#define NEED_EDF_DEFINITIONS

#include "Confuse/confuse.h"
#include "e_edf.h"
#include "e_sound.h"

//
// Sound keywords
//
#define ITEM_SND_LUMP "lump"
#define ITEM_SND_PREFIX "prefix"
#define ITEM_SND_SINGULARITY "singularity"
#define ITEM_SND_PRIORITY "priority"
#define ITEM_SND_LINK "link"
#define ITEM_SND_SKININDEX "skinindex"
#define ITEM_SND_LINKVOL "linkvol"
#define ITEM_SND_LINKPITCH "linkpitch"
#define ITEM_SND_CLIPPING_DIST "clipping_dist"
#define ITEM_SND_CLOSE_DIST "close_dist"
#define ITEM_SND_DEHNUM "dehackednum"

#define ITEM_DELTA_NAME "name"

//
// Static sound hash tables
//
#define NUMSFXCHAINS 257
static sfxinfo_t *sfxchains[NUMSFXCHAINS];
static sfxinfo_t *sfx_dehchains[NUMSFXCHAINS];

//
// Singularity types
//
// This must reflect the enumeration in sounds.h
//
static const char *singularities[] =
{
   "sg_none",
   "sg_itemup",
   "sg_wpnup",
   "sg_oof",
   "sg_getpow",
   NULL
};

//
// Skin sound indices
//
// This must reflect the enumeration in p_skin.h, with the
// exception of the addition of "sk_none" to bump up everything
// by one and to provide for a mnemonic for value zero.
//
static const char *skinindices[] =
{
   "sk_none", // Note that sfxinfo stores the true index + 1
   "sk_plpain",
   "sk_pdiehi",
   "sk_oof",
   "sk_slop",
   "sk_punch",
   "sk_radio",
   "sk_pldeth",
   "sk_plfall",
   "sk_plfeet",
   "sk_fallht",
   NULL
};

#define SOUND_OPTIONS \
   CFG_STR(ITEM_SND_LUMP,          NULL,              CFGF_NONE), \
   CFG_BOOL(ITEM_SND_PREFIX,       cfg_true,          CFGF_NONE), \
   CFG_STR(ITEM_SND_SINGULARITY,   "sg_none",         CFGF_NONE), \
   CFG_INT(ITEM_SND_PRIORITY,      64,                CFGF_NONE), \
   CFG_STR(ITEM_SND_LINK,          "none",            CFGF_NONE), \
   CFG_STR(ITEM_SND_SKININDEX,     "sk_none",         CFGF_NONE), \
   CFG_INT(ITEM_SND_LINKVOL,       -1,                CFGF_NONE), \
   CFG_INT(ITEM_SND_LINKPITCH,     -1,                CFGF_NONE), \
   CFG_INT(ITEM_SND_CLIPPING_DIST, S_CLIPPING_DIST_I, CFGF_NONE), \
   CFG_INT(ITEM_SND_CLOSE_DIST,    S_CLOSE_DIST_I,    CFGF_NONE), \
   CFG_INT(ITEM_SND_DEHNUM,        -1,                CFGF_NONE), \
   CFG_END()

//
// Sound cfg options array (used in e_edf.c)
//
cfg_opt_t edf_sound_opts[] =
{
   SOUND_OPTIONS
};

cfg_opt_t edf_sdelta_opts[] =
{
   CFG_STR(ITEM_DELTA_NAME, NULL, CFGF_NONE),
   SOUND_OPTIONS
};

//
// E_SoundForName
//
// Returns a sfxinfo_t pointer given the EDF mnemonic for that
// sound. Will return NULL if the requested sound is not found.
//
sfxinfo_t *E_SoundForName(const char *name)
{
   unsigned int hash = D_HashTableKey(name) % NUMSFXCHAINS;
   sfxinfo_t  *rover = sfxchains[hash];

   while(rover && strcasecmp(name, rover->mnemonic))
      rover = rover->next;

   return rover;
}

//
// E_EDFSoundForName
//
// This version returns a pointer to S_sfx[0] if the special
// mnemonic "none" is passed in. This allows EDF to get the reserved
// DeHackEd number zero for it. Most other code segments should not
// use this function.
//
sfxinfo_t *E_EDFSoundForName(const char *name)
{
   if(!strcasecmp(name, "none"))
      return &S_sfx[0];

   return E_SoundForName(name);
}

//
// E_SoundForDEHNum
//
// Returns a sfxinfo_t pointer given the DeHackEd number for that
// sound. Will return NULL if the requested sound is not found.
//
sfxinfo_t *E_SoundForDEHNum(int dehnum)
{
   unsigned int hash = dehnum % NUMSFXCHAINS;
   sfxinfo_t  *rover = sfx_dehchains[hash];

   while(rover && rover->dehackednum != dehnum)
      rover = rover->dehnext;

   return rover;
}

//
// E_AddSoundToHash
//
// Adds a new sfxinfo_t structure to the hash table.
//
static void E_AddSoundToHash(sfxinfo_t *sfx)
{
   // compute the hash code using the sound mnemonic
   unsigned int hash;

   // make sure it doesn't exist already -- if it does, this
   // insertion must be ignored
   if(E_EDFSoundForName(sfx->mnemonic))
      return;

   hash = D_HashTableKey(sfx->mnemonic) % NUMSFXCHAINS;

   // link it in
   sfx->next = sfxchains[hash];
   sfxchains[hash] = sfx;
}

//
// E_AddSoundToDEHHash
//
// Only used locally. This adds a sound to the DeHackEd number hash
// table, so that both old and new sounds can be referred to by
// use of a number. This avoids major code rewrites and compatibility
// issues. It also naturally extends DeHackEd, too.
//
static void E_AddSoundToDEHHash(sfxinfo_t *sfx)
{
   unsigned int hash = sfx->dehackednum % NUMSFXCHAINS;

   if(E_SoundForDEHNum(sfx->dehackednum))
      return;

   if(sfx->dehackednum == 0)
      E_EDFLoggedErr(2, "E_AddSoundToDEHHash: dehackednum zero is reserved!\n");

   sfx->dehnext = sfx_dehchains[hash];
   sfx_dehchains[hash] = sfx;
}

//
// E_NewWadSound
//
// Creates a sfxinfo_t structure for a new wad sound and
// hashes it.
//
void E_NewWadSound(const char *name)
{
   sfxinfo_t *sfx;
   char mnemonic[9];

   memset(mnemonic, 0, sizeof(mnemonic));
   strncpy(mnemonic, name+2, 9);

   sfx = E_EDFSoundForName(mnemonic);
   
   if(!sfx)
   {
      // create a new one and hook into hashchain
      sfx = Z_Malloc(sizeof(sfxinfo_t), PU_STATIC, 0);

      memset(sfx, 0, sizeof(sfxinfo_t));
      
      strncpy(sfx->name, name, 9);
      strncpy(sfx->mnemonic, mnemonic, 9);
      sfx->prefix = false;  // do not add another DS prefix      
      
      sfx->singularity = sg_none;
      sfx->priority = 64;
      sfx->link = NULL;
      sfx->pitch = sfx->volume = -1;
      sfx->clipping_dist = S_CLIPPING_DIST;
      sfx->close_dist = S_CLOSE_DIST;
      sfx->skinsound = 0;
      sfx->data = NULL;
      sfx->dehackednum = -1; // not accessible to DeHackEd
      
      E_AddSoundToHash(sfx);

      return;
   }

   if(sfx->data)
   {
      // free it if cached
      Z_Free(sfx->data);      // free
      sfx->data = NULL;
   }
}

//
// E_PreCacheSounds
//
// Runs down the sound mnemonic hash table chains and caches all 
// sounds. This is improved from the code that was in SMMU, which 
// only precached entries in the S_sfx array. This is called at 
// startup when sound precaching is enabled.
//
void E_PreCacheSounds(void)
{
   int i;
   sfxinfo_t *cursfx;

   // run down all the mnemonic hash chains so that we precache 
   // all sounds, not just ones stored in S_sfx

   for(i = 0; i < NUMSFXCHAINS; ++i)
   {
      cursfx = sfxchains[i];

      while(cursfx)
      {
         I_CacheSound(cursfx);
         cursfx = cursfx->next;
      }
   }
}

//
// EDF Processing Functions
//

#define IS_SET(name) (def || cfg_size(section, name) > 0)

//
// E_ProcessSound
//
// Processes an EDF sound definition
//
static void E_ProcessSound(sfxinfo_t *sfx, cfg_t *section, boolean def)
{
   int i;

   // preconditions: 
   
   // sfx->mnemonic is valid, and this sfxinfo_t has already been 
   // added to the sound hash table earlier by E_ProcessSounds

   // process the lump name
   if(IS_SET(ITEM_SND_LUMP))
   {
      const char *lumpname;

      // if this is the definition, and the lump name is not
      // defined, duplicate the mnemonic as the sound name
      if(def && cfg_size(section, ITEM_SND_LUMP) == 0)
         strncpy(sfx->name, sfx->mnemonic, 9);
      else
      {
         lumpname = cfg_getstr(section, ITEM_SND_LUMP);

         strncpy(sfx->name, lumpname, 9);
      }
   }

   // process the prefix flag
   if(IS_SET(ITEM_SND_PREFIX))
      sfx->prefix = cfg_getbool(section, ITEM_SND_PREFIX);

   // process the singularity
   if(IS_SET(ITEM_SND_SINGULARITY))
   {
      const char *s = cfg_getstr(section, ITEM_SND_SINGULARITY);

      i = 0;

      while(singularities[i])
      {
         if(!strcasecmp(singularities[i], s))
         {
            sfx->singularity = i;
            break;
         }
         ++i;
      }
   }

   // process the priority value
   if(IS_SET(ITEM_SND_PRIORITY))
      sfx->priority = cfg_getint(section, ITEM_SND_PRIORITY);

   // process the link
   if(IS_SET(ITEM_SND_LINK))
   {
      const char *name = cfg_getstr(section, ITEM_SND_LINK);

      // will be automatically nullified if name is not found
      // (this includes the default value of "none")
      sfx->link = E_SoundForName(name);
   }

   // process the skin index
   if(IS_SET(ITEM_SND_SKININDEX))
   {
      const char *s = cfg_getstr(section, ITEM_SND_SKININDEX);

      i = 0;

      while(skinindices[i])
      {
         if(!strcasecmp(skinindices[i], s))
         {
            sfx->skinsound = i;
            break;
         }
         ++i;
      }
   }

   // process link volume
   if(IS_SET(ITEM_SND_LINKVOL))
      sfx->volume = cfg_getint(section, ITEM_SND_LINKVOL);

   // process link pitch
   if(IS_SET(ITEM_SND_LINKPITCH))
      sfx->pitch = cfg_getint(section, ITEM_SND_LINKPITCH);

   // haleyjd 07/13/05: process clipping_dist
   if(IS_SET(ITEM_SND_CLIPPING_DIST))
      sfx->clipping_dist = cfg_getint(section, ITEM_SND_CLIPPING_DIST) << FRACBITS;

   // haleyjd 07/13/05: process close_dist
   if(IS_SET(ITEM_SND_CLOSE_DIST))
      sfx->close_dist = cfg_getint(section, ITEM_SND_CLOSE_DIST) << FRACBITS;

   // process dehackednum -- not in deltas!
   if(def)
   {
      sfx->dehackednum = cfg_getint(section, ITEM_SND_DEHNUM);

      if(sfx->dehackednum != -1)
         E_AddSoundToDEHHash(sfx); // add to DeHackEd num hash table
   }
}

//
// E_ProcessSounds
//
// Collects all the sound definitions and builds the sound hash
// tables.
//
void E_ProcessSounds(cfg_t *cfg)
{
   int i;

   E_EDFLogPuts("\t\tHashing sounds\n");

   // initialize S_sfx[0]
   strcpy(S_sfx[0].name, "none");
   strcpy(S_sfx[0].mnemonic, "none");

   // now, let's collect the mnemonics (this must be done ahead of time)
   for(i = 1; i < NUMSFX; ++i)
   {
      const char *mnemonic;
      cfg_t *sndsection = cfg_getnsec(cfg, EDF_SEC_SOUND, i - 1);

      mnemonic = cfg_title(sndsection);

      // verify the length
      if(strlen(mnemonic) > 16)
      {
         E_EDFLoggedErr(2, 
            "E_ProcessSounds: invalid sound mnemonic '%s'\n", mnemonic);
      }

      // copy it to the sound
      strncpy(S_sfx[i].mnemonic, mnemonic, 17);

      // add this sound to the hash table
      E_AddSoundToHash(&S_sfx[i]);
   }

   E_EDFLogPuts("\t\tProcessing data\n");

   // finally, process the individual sounds
   for(i = 1; i < NUMSFX; ++i)
   {
      cfg_t *section = cfg_getnsec(cfg, EDF_SEC_SOUND, i - 1);

      E_ProcessSound(&S_sfx[i], section, true);

      E_EDFLogPrintf("\t\tFinished sound %s(#%d)\n", S_sfx[i].mnemonic, i);
   }

   E_EDFLogPuts("\t\tFinished sound processing\n");
}

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

   E_EDFLogPuts("\t* Processing sound deltas\n");

   numdeltas = cfg_size(cfg, EDF_SEC_SDELTA);

   E_EDFLogPrintf("\t\t%d sounddelta(s) defined\n", numdeltas);

   for(i = 0; i < numdeltas; i++)
   {
      const char *tempstr;
      sfxinfo_t *sfx;
      cfg_t *deltasec = cfg_getnsec(cfg, EDF_SEC_SDELTA, i);

      // get thingtype to edit
      if(!cfg_size(deltasec, ITEM_DELTA_NAME))
         E_EDFLoggedErr(2, "E_ProcessSoundDeltas: sounddelta requires name field\n");

      tempstr = cfg_getstr(deltasec, ITEM_DELTA_NAME);
      sfx = E_SoundForName(tempstr);

      if(!sfx)
      {
         E_EDFLoggedErr(2, 
            "E_ProcessSoundDeltas: sound '%s' does not exist\n", tempstr);
      }

      E_ProcessSound(sfx, deltasec, false);

      E_EDFLogPrintf("\t\tApplied sounddelta #%d to sound %s\n",
                     i, tempstr);
   }
}

// EOF


