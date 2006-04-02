// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
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
// EDF States Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "i_system.h"
#include "p_partcl.h"
#include "d_io.h"
#include "d_dehtbl.h"
#include "info.h"
#include "m_qstr.h"
#include "p_pspr.h"

#define NEED_EDF_DEFINITIONS

#include "Confuse/confuse.h"

#include "e_lib.h"
#include "e_edf.h"
#include "e_things.h"
#include "e_sound.h"
#include "e_string.h"
#include "e_states.h"

extern int E_SpriteNumForName(const char *name);

// 7/24/05: This is now global, for efficiency's sake

// The "S_NULL" state, which is required, has its number resolved
// in E_CollectStates
int NullStateNum;

// Frame section keywords
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

#define ITEM_DELTA_NAME "name"

//
// Frame Options
//

#define FRAME_FIELDS \
   CFG_STR(ITEM_FRAME_SPRITE,      "BLANK",     CFGF_NONE), \
   CFG_INT_CB(ITEM_FRAME_SPRFRAME, 0,           CFGF_NONE, E_SpriteFrameCB), \
   CFG_BOOL(ITEM_FRAME_FULLBRT,    cfg_false,   CFGF_NONE), \
   CFG_INT(ITEM_FRAME_TICS,        1,           CFGF_NONE), \
   CFG_STR(ITEM_FRAME_ACTION,      "NULL",      CFGF_NONE), \
   CFG_STR(ITEM_FRAME_NEXTFRAME,   "S_NULL",    CFGF_NONE), \
   CFG_STR(ITEM_FRAME_MISC1,       "0",         CFGF_NONE), \
   CFG_STR(ITEM_FRAME_MISC2,       "0",         CFGF_NONE), \
   CFG_STR(ITEM_FRAME_PTCLEVENT,   "pevt_none", CFGF_NONE), \
   CFG_STR(ITEM_FRAME_ARGS,        0,           CFGF_LIST), \
   CFG_INT(ITEM_FRAME_DEHNUM,      -1,          CFGF_NONE), \
   CFG_END()

cfg_opt_t edf_frame_opts[] =
{
   CFG_STR(ITEM_FRAME_CMP, 0, CFGF_NONE),
   FRAME_FIELDS
};

cfg_opt_t edf_fdelta_opts[] =
{
   CFG_STR(ITEM_DELTA_NAME, 0, CFGF_NONE),
   FRAME_FIELDS
};

//
// State Hash Lookup Functions
//

// State hash tables
// Note: Keep some buffer space between this prime number and the
// number of default states defined, so that users can add a number 
// of types without causing significant hash collisions. I do not 
// recommend changing the hash table to be dynamically allocated.

// State Hashing
#define NUMSTATECHAINS 2003
static int state_namechains[NUMSTATECHAINS];
static int state_dehchains[NUMSTATECHAINS];

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
   while(statenum != NUMSTATES && strcasecmp(name, states[statenum].name))
      statenum = states[statenum].namenext;
   
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

//
// EDF Processing Routines
//

//
// E_CollectStates
//
// Pre-creates and hashes by name the states, for purpose of mutual 
// and forward references.
//
void E_CollectStates(cfg_t *scfg)
{
   int i;

   // allocate array
   states = Z_Malloc(sizeof(state_t)*NUMSTATES, PU_STATIC, NULL);

   // initialize hash slots
   for(i = 0; i < NUMSTATECHAINS; ++i)
      state_namechains[i] = state_dehchains[i] = NUMSTATES;

   // build hash table
   E_EDFLogPuts("\t* Building state hash tables\n");

   for(i = 0; i < NUMSTATES; ++i)
   {
      unsigned int key;
      cfg_t *statecfg = cfg_getnsec(scfg, EDF_SEC_FRAME, i);
      const char *name = cfg_title(statecfg);
      int tempint;

      // verify length
      if(strlen(name) > 40)
      {
         E_EDFLoggedErr(2, 
            "E_CollectStates: invalid frame mnemonic '%s'\n", name);
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
         int cnum;
         
         // make sure it doesn't exist yet
         if((cnum = E_StateNumForDEHNum(tempint)) != NUMSTATES)
         {
            E_EDFLoggedErr(2, 
               "E_CollectStates: frame '%s' reuses dehackednum %d\n"
               "                 Conflicting frame: '%s'\n",
               states[i].name, tempint, states[cnum].name);
         }
         
         states[i].dehnext = state_dehchains[dehkey];
         state_dehchains[dehkey] = i;
      }
   }

   // verify the existence of the S_NULL frame
   NullStateNum = E_StateNumForName("S_NULL");
   if(NullStateNum == NUMSTATES)
      E_EDFLoggedErr(2, "E_CollectStates: 'S_NULL' frame must be defined!\n");
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
         E_EDFLoggedErr(2, 
            "E_ProcessState: frame '%s': invalid sprite '%s'\n",
            states[i].name, tempstr);
      }
      states[i].sprite = sprnum;
   }
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
      E_EDFLoggedErr(2, "E_ProcessState: frame '%s': bad action '%s'\n",
                     states[i].name, tempstr);
   }

   states[i].action = dp->cptr;
}

enum
{
   PREFIX_FRAME,
   PREFIX_THING,
   PREFIX_SOUND,
   PREFIX_FLAGS,
   PREFIX_FLAGS2,
   PREFIX_FLAGS3,
   PREFIX_BEXPTR,
   PREFIX_STRING,
   NUM_MISC_PREFIXES
};

static const char *misc_prefixes[NUM_MISC_PREFIXES] =
{
   "frame",  "thing",  "sound", "flags", "flags2", "flags3",
   "bexptr", "string"
};

//
// E_ParseMiscField
//
// This function implements the quite powerful prefix:value syntax
// for misc and args fields in frames. Some of the code within may
// be candidate for generalization, since other fields may need
// this syntax in the near future.
//
static void E_ParseMiscField(char *value, long *target)
{
   int i;
   char prefix[16];
   char *colonloc, *strval;
   
   memset(prefix, 0, 16);

   // look for a colon ending a possible prefix
   colonloc = E_ExtractPrefix(value, prefix, 16);
   
   if(colonloc)
   {
      // a colon was found, so identify the prefix
      strval = colonloc + 1;

      i = E_StrToNumLinear(misc_prefixes, NUM_MISC_PREFIXES, prefix);

      switch(i)
      {
      case PREFIX_FRAME:
         {
            int framenum = E_StateNumForName(strval);
            if(framenum == NUMSTATES)
            {
               E_EDFLoggedErr(2, "E_ParseMiscField: invalid state '%s'\n",
                              strval);
            }
            
            // 09/19/03: add check for no dehacked number
            if(states[framenum].dehnum == -1)
            {
               E_EDFLogPrintf("\t\tWarning: frame %s has no DeHackEd number\n",
                              strval);
               *target = NullStateNum;
            }
            else
               *target = states[framenum].dehnum;
         }
         break;
      case PREFIX_THING:
         {
            int thingnum = E_ThingNumForName(strval);
            if(thingnum == NUMMOBJTYPES)
            {
               E_EDFLoggedErr(2, "E_ParseMiscField: invalid thing '%s'\n",
                              strval);
            }

            // 09/19/03: add check for no dehacked number
            if(mobjinfo[thingnum].dehnum == -1)
            {
               E_EDFLogPrintf("\t\tWarning: thing %s has no DeHackEd number\n",
                              strval);
               *target = UnknownThingType;
            }
            else
               *target = mobjinfo[thingnum].dehnum;
         }
         break;
      case PREFIX_SOUND:
         {
            sfxinfo_t *sfx = E_EDFSoundForName(strval);
            if(!sfx)
            {
               E_EDFLoggedErr(2, "E_ParseMiscField: invalid sound '%s'\n",
                              strval);
            }

            if(sfx->dehackednum == -1)
            {
               // print a warning in this case, and set the sound to zero
               E_EDFLogPrintf("\t\tWarning: sound %s has no DeHackEd number\n",
                              sfx->mnemonic);
               *target = 0;
            }
            else
               *target = sfx->dehackednum;         
         }
         break;
      case PREFIX_FLAGS:
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE1);
         break;
      case PREFIX_FLAGS2:
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE2);
         break;
      case PREFIX_FLAGS3:
         *target = deh_ParseFlagsSingle(strval, DEHFLAGS_MODE3);
         break;
      case PREFIX_BEXPTR:
         {
            deh_bexptr *dp = D_GetBexPtr(strval);
            
            if(!dp)
            {
               E_EDFLoggedErr(2, "E_ParseMiscField: bad bexptr '%s'\n",
                              strval);
            }
            
            // get the index of this deh_bexptr in the master
            // deh_bexptrs array, and store it in the arg field
            *target = dp - deh_bexptrs;
         }
         break;
      case PREFIX_STRING:
         {
            edf_string_t *str = E_StringForName(strval);
            
            if(!str || str->numkey < 0)
            {
               E_EDFLogPrintf("\t\tWarning: bad string %s\n", strval);
               *target = 0;
            }
            else
               *target = str->numkey;
         }
         break;
      default:
         E_EDFLoggedErr(2, "E_ParseMiscField: unknown value prefix '%s'\n",
                        prefix);
      }
   }
   else      
      // must just be an integer value
      // 11/11/03: use strtol to support hex and oct input
      *target = strtol(value, NULL, 0);
}

enum
{
   NSPEC_NEXT,
   NSPEC_PREV,
   NSPEC_THIS,
   NSPEC_NULL,
   NUM_NSPEC_KEYWDS
};

static const char *nspec_keywds[NUM_NSPEC_KEYWDS] =
{
   "next", "prev", "this", "null"
};

//
// E_SpecialNextState
//
// 11/07/03:
// Returns a frame number for a special nextframe value.
//
static int E_SpecialNextState(const char *string, int framenum)
{
   int i, nextnum = 0;
   const char *value = string + 1;

   i = E_StrToNumLinear(nspec_keywds, NUM_NSPEC_KEYWDS, value);

   switch(i)
   {
   case NSPEC_NEXT:
      if(framenum == NUMSTATES - 1) // can't do it
      {
         E_EDFLoggedErr(2, "E_SpecialNextState: invalid frame #%d\n",
                        NUMSTATES);
      }
      nextnum = framenum + 1;
      break;
   case NSPEC_PREV:
      if(framenum == 0) // can't do it
         E_EDFLoggedErr(2, "E_SpecialNextState: invalid frame -1\n");
      nextnum = framenum - 1;
      break;
   case NSPEC_THIS:
      nextnum = framenum;
      break;
   case NSPEC_NULL:
      nextnum = NullStateNum;
      break;
   default: // ???
      E_EDFLoggedErr(2, "E_SpecialNextState: invalid specifier '%s'\n",
                     value);
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
         E_EDFLoggedErr(2, 
            "E_ProcessState: frame '%s': bad nextframe '%s'\n",
            states[i].name, tempstr);

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
      E_EDFLoggedErr(2, 
         "E_ProcessState: frame '%s': bad ptclevent '%s'\n",
         states[i].name, tempstr);
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
static char *E_CmpTokenizer(const char *text, int *index, qstring_t *token)
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
         case '\'':
            state = 2; // enter quoted part (single quote support)
            continue;
         case '|':     // end of current token
         case ',':     // 03/01/05: added by user request
            return M_QStrBuffer(token);
         default:      // everything else == part of value
            M_QStrPutc(token, c);
            continue;
         }
      case 1: // in quoted area (double quotes)
         if(c == '"') // end of quoted area
            state = 0;
         else
            M_QStrPutc(token, c); // everything inside is literal
         continue;
      case 2: // in quoted area (single quotes)
         if(c == '\'') // end of quoted area
            state = 0;
         else
            M_QStrPutc(token, c); // everything inside is literal
         continue;
      default:
         E_EDFLoggedErr(0, "E_CmpTokenizer: internal error - undefined lexer state\n");
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
   char *curtoken = NULL;
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
         E_EDFLoggedErr(2, 
            "E_ProcessCmpState: frame '%s': bad spriteframe '%s'\n",
            states[i].name, curtoken);
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
   char *tempstr;

   // 11/14/03:
   // In definitions only, see if the cmp field is defined. If so,
   // we go into it with E_ProcessCmpState above, and ignore most
   // other fields in the frame block.
   if(def)
   {
      if(cfg_size(framesec, ITEM_FRAME_CMP) > 0)
      {
         tempstr = cfg_getstr(framesec, ITEM_FRAME_CMP);
         
         E_ProcessCmpState(tempstr, i);
         def = false; // process remainder as if a frame delta
         goto hitcmp;
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

   // 03/30/05: the following fields are now also allowed in cmp frames
hitcmp:
      
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
void E_ProcessStates(cfg_t *cfg)
{
   int i;

   E_EDFLogPuts("\t* Processing frame data\n");

   for(i = 0; i < NUMSTATES; ++i)
   {
      cfg_t *framesec = cfg_getnsec(cfg, EDF_SEC_FRAME, i);

      E_ProcessState(i, framesec, true);

      E_EDFLogPrintf("\t\tFinished frame %s(#%d)\n", states[i].name, i);
   }
}

//
// E_ProcessStateDeltas
//
// Does processing for framedelta sections, which allow cascading
// editing of existing frames. The framedelta shares most of its
// fields and processing code with the frame section.
//
void E_ProcessStateDeltas(cfg_t *cfg)
{
   int i, numdeltas;

   E_EDFLogPuts("\t* Processing frame deltas\n");

   numdeltas = cfg_size(cfg, EDF_SEC_FRMDELTA);

   E_EDFLogPrintf("\t\t%d framedelta(s) defined\n", numdeltas);

   for(i = 0; i < numdeltas; ++i)
   {
      const char *tempstr;
      int stateNum;
      cfg_t *deltasec = cfg_getnsec(cfg, EDF_SEC_FRMDELTA, i);

      // get state to edit
      if(!cfg_size(deltasec, ITEM_DELTA_NAME))
         E_EDFLoggedErr(2, "E_ProcessFrameDeltas: framedelta requires name field\n");

      tempstr = cfg_getstr(deltasec, ITEM_DELTA_NAME);
      stateNum = E_GetStateNumForName(tempstr);

      E_ProcessState(stateNum, deltasec, false);

      E_EDFLogPrintf("\t\tApplied framedelta #%d to %s(#%d)\n",
                     i, states[stateNum].name, stateNum);
   }
}

// EOF

