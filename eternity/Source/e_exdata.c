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
// ExtraData
//
// The be-all, end-all extension to the DOOM map format. Uses the
// libConfuse library like EDF.
// 
// ExtraData can extend mapthings, lines, and sectors with an
// arbitrary number of fields, with data provided in more or less
// any format. The use of a textual input language will forever
// remove any future problems caused by binary format limitations.
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "doomdef.h"
#include "d_io.h"
#include "p_info.h"
#include "p_mobj.h"
#include "w_wad.h"
#include "i_system.h"
#include "d_dehtbl.h" // for dehflags parsing
#include "e_exdata.h"
#include "e_edf.h"

#include "Confuse/confuse.h"

// globals

boolean levelHasExtraData = false;

// statics

static mapthingext_t *EDThings;
static unsigned int numEDMapThings;

#define NUMMTCHAINS 1021
static unsigned int mapthing_chains[NUMMTCHAINS];

// ExtraData section names
#define SEC_MAPTHING "mapthing"

// ExtraData field names
// mapthing fields:
#define FIELD_NUM "recordnum"
#define FIELD_TID "tid"
#define FIELD_TYPE "type"
#define FIELD_OPTIONS "options"

// mapthing options and related data structures

static cfg_opt_t mapthing_opts[] =
{
   CFG_INT(FIELD_NUM,     0,  CFGF_NONE),
   CFG_INT(FIELD_TID,     0,  CFGF_NONE),
   CFG_STR(FIELD_TYPE,    "", CFGF_NONE),
   CFG_STR(FIELD_OPTIONS, "", CFGF_NONE),
   CFG_END()
};

// mapthing flag values and mnemonics

static dehflags_t mapthingflags[] =
{
   { "EASY",      MTF_EASY },
   { "NORMAL",    MTF_NORMAL },
   { "HARD",      MTF_HARD },
   { "AMBUSH",    MTF_AMBUSH },
   { "NOTSINGLE", MTF_NOTSINGLE },
   { "NOTDM",     MTF_NOTDM },
   { "NOTCOOP",   MTF_NOTCOOP },
   { "FRIEND",    MTF_FRIEND },
   { "DORMANT",   MTF_DORMANT },
   { NULL,        0 }
};

static dehflagset_t mt_flagset =
{
   mapthingflags, // flaglist
   0,             // mode
};

// primary ExtraData options table

static cfg_opt_t ed_opts[] =
{
   CFG_SEC(SEC_MAPTHING, mapthing_opts, CFGF_MULTI|CFGF_NOCASE),
   CFG_END()
};

// error callback

static void ed_error(cfg_t *cfg, const char *fmt, va_list ap)
{
   I_ErrorVA(fmt, ap);
}

//
// E_EDThingForRecordNum
//
// Returns an index into EDThings for the given record number.
// Returns numEDMapThings if no such record exists.
//
static unsigned int E_EDThingForRecordNum(int recnum)
{
   unsigned int num;
   int key = recnum % NUMMTCHAINS;

   num = mapthing_chains[key];
   while(num != numEDMapThings && EDThings[num].recordnum != recnum)
   {
      num = EDThings[num].next;
   }

   return num;
}

//
// E_ParseTypeField
//
// Parses thing type fields in ExtraData. Allows resolving of
// EDF thingtype mnemonics to their corresponding doomednums.
//
static int E_ParseTypeField(const char *value)
{
   int i;
   char prefix[16];
   const char *colonloc, *rover, *strval;

   memset(prefix, 0, 16);

   colonloc = strchr(value, ':');

   if(colonloc)
   {
      // extract prefix
      strval = colonloc + 1;
      rover = value;
      i = 0;
      while(rover != colonloc && i < 15)
      {
         prefix[i] = *rover;
         ++rover;
         ++i;
      }
      
      // make sure its not just a prefix with no value
      if(!(*strval))
         I_Error("E_ParseTypeField: invalid value %s\n", value);

      if(!strcasecmp(prefix, "thing"))
      {
         // translate from EDF mnemonic to doomednum
         int num;
         int type = E_ThingNumForName(strval);

         if(type == NUMMOBJTYPES)
            num = mobjinfo[E_UnknownThing()].doomednum;
         else
            num = mobjinfo[type].doomednum;
         
         // don't return -1, use no-op zero in that case
         // (this'll work even if somebody messed with 'Unknown')
         return (num >= 0 ? num : 0);
      }

      // invalid prefix
      I_Error("E_ParseTypeField: invalid prefix %s\n", prefix);
   }

   // integer value
   i = strtol(value, NULL, 0);
   return (i >= 0 ? i : 0);
}

//
// E_ParseFlags
//
// Parses the mapthing options field.
//
static long E_ParseFlags(const char *str)
{
   char *buffer;
   char *bufptr;

   bufptr = buffer = strdup(str);

   deh_ParseFlags(&mt_flagset, &bufptr, NULL);

   free(buffer);

   return mt_flagset.results[0];
}

//
// E_ProcessEDThings
//
// Allocates and processes ExtraData mapthing records.
//
static void E_ProcessEDThings(cfg_t *cfg)
{
   unsigned int i;

   // get the number of mapthing records
   numEDMapThings = cfg_size(cfg, SEC_MAPTHING);

   // if none, we're done
   if(!numEDMapThings)
      return;

   // allocate the mapthingext_t structures
   EDThings = Z_Malloc(numEDMapThings * sizeof(mapthingext_t),
                       PU_LEVEL, NULL);

   // initialize the hash chains
   for(i = 0; i < NUMMTCHAINS; i++)
      mapthing_chains[i] = numEDMapThings;

   // read fields
   for(i = 0; i < numEDMapThings; i++)
   {
      cfg_t *thingsec;
      const char *tempstr;
      int tempint;

      thingsec = cfg_getnsec(cfg, SEC_MAPTHING, i);

      // get the record number
      tempint = EDThings[i].recordnum = cfg_getint(thingsec, FIELD_NUM);

      // guard against duplicate record numbers
      if(E_EDThingForRecordNum(tempint) != numEDMapThings)
         I_Error("E_ProcessEDThings: duplicate record number %d\n", tempint);

      // hash this ExtraData mapthing record by its recordnum field
      tempint = EDThings[i].recordnum % NUMMTCHAINS;
      EDThings[i].next = mapthing_chains[tempint];
      mapthing_chains[tempint] = i;

      // standard fields

      // type
      tempstr = cfg_getstr(thingsec, FIELD_TYPE);
      EDThings[i].stdfields.type = (short)(E_ParseTypeField(tempstr));

      // it is not allowed to spawn an ExtraData control object via
      // ExtraData, so doomednum will be zeroed in such a case
      if(EDThings[i].stdfields.type == ED_CTRL_DOOMEDNUM)
         EDThings[i].stdfields.type = 0;

      // options
      tempstr = cfg_getstr(thingsec, FIELD_OPTIONS);
      if(*tempstr == '\0')
         EDThings[i].stdfields.options = 0;
      else
         EDThings[i].stdfields.options = (short)(E_ParseFlags(tempstr));

      // extended fields

      // get TID field
      EDThings[i].tid = (unsigned short)cfg_getint(thingsec, FIELD_TID);

      // TODO: any other new fields
   }
}

//
// E_LoadExtraData
//
// Loads the ExtraData lump for the level, if it has one
//
void E_LoadExtraData(void)
{
   cfg_t *cfg;

   // reset ExtraData variables (allocations are at PU_LEVEL
   // cache level, so anything from any earlier level has been
   // freed)
   EDThings = NULL;
   numEDMapThings = 0;

   // check to see if the ExtraData lump is defined by MapInfo
   if(!info_extradata)
   {
      levelHasExtraData = false;
      return;
   }

   levelHasExtraData = true;

   // initialize the cfg
   cfg = cfg_init(ed_opts, CFGF_NOCASE);
   cfg_set_error_function(cfg, ed_error);

   // load and parse the ED lump
   if(cfg_parselump(cfg, info_extradata))
      I_Error("E_LoadExtraData: Error finding or parsing lump\n");

   // processing

   // load mapthings
   E_ProcessEDThings(cfg);

   // TODO: more processing

   // free the cfg
   cfg_free(cfg);
}

//
// E_SpawnMapThingExt
//
// Called by P_SpawnMapThing when an ExtraData control point
// (doomednum 5004) is encountered.  This function recursively
// calls P_SpawnMapThing with the new basic mapthing data from
// the corresponding ExtraData record, and then sets ExtraData 
// fields on the returned mobj_t.
//
mobj_t *E_SpawnMapThingExt(mapthing_t *mt)
{
   unsigned int edThingIdx;
   mapthingext_t *edthing;
   mobj_t *mo;

   // The record number is stored in the control thing's options field.
   // Check to see if the record exists, and that ExtraData is loaded.
   if(!levelHasExtraData || numEDMapThings == 0 ||
      (edThingIdx = E_EDThingForRecordNum(mt->options)) == numEDMapThings)
   {
      // spawn an Unknown thing
      mo = P_SpawnMobj(mt->x << FRACBITS, mt->y << FRACBITS, ONFLOORZ,
                       E_UnknownThing());
      return mo;
   }

   // get a pointer to the proper ExtraData mapthing record
   edthing = &(EDThings[edThingIdx]);

   // propagate the control object's x, y, and angle fields to the
   // mapthing_t inside the record
   edthing->stdfields.x = mt->x;
   edthing->stdfields.y = mt->y;
   edthing->stdfields.angle = mt->angle;

   // spawn the thing normally
   mo = P_SpawnMapThing(&(edthing->stdfields));

   // set extended fields in mo from the record
   if(mo)
   {
      // 02/02/04: TID -- numeric id used for scripting
      P_AddThingTID(mo, edthing->tid);
   }

   // return the spawned object back through P_SpawnMapThing
   return mo;
}

// EOF

