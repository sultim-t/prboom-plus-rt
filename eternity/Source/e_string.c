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
// EDF Strings Module
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "i_system.h"
#include "d_dehtbl.h"
#include "d_io.h"

#define NEED_EDF_DEFINITIONS

#include "Confuse/confuse.h"

#include "e_edf.h"
#include "e_string.h"

// 03/27/05: EDF strings!

// String section keywords
#define ITEM_STRING_NUM "num"
#define ITEM_STRING_VAL "val"

// String Options
cfg_opt_t edf_string_opts[] =
{
   CFG_STR(ITEM_STRING_VAL, "", CFGF_NONE),
   CFG_INT(ITEM_STRING_NUM, -1, CFGF_NONE),
   CFG_END()
};

#define NUM_EDFSTR_CHAINS 257

static edf_string_t *edf_str_chains[NUM_EDFSTR_CHAINS];
static edf_string_t *edf_str_numchains[NUM_EDFSTR_CHAINS];

//
// haleyjd 08/05/05: To support editing the numeric keys of existing
// string objects, the numeric hash has to become a double-linked list.
// See the code in m_dllist.h to see how a generic double-linked list
// with **prev pointer is implemented.
//

//
// E_AddStringToNumHash
//
// Add an EDF string object to the numeric hash table. More than one
// object with the same numeric id can exist, but E_StringForNum will
// only ever find the first such object, which is always the last such
// object added to the hash table.
//
static void E_AddStringToNumHash(edf_string_t *str)
{
   edf_string_t *chain;

   chain = edf_str_numchains[str->numkey % NUM_EDFSTR_CHAINS];

   M_DLListInsert((mdllistitem_t *)str, &((mdllistitem_t *)chain));
}

//
// E_DelStringFromNumHash
//
// Removes a specific EDF string object from the numeric hash table.
// This must be called on an object that's already linked before
// relinking it.
//
static void E_DelStringFromNumHash(edf_string_t *str)
{
   M_DLListRemove((mdllistitem_t *)str);
}

//
// E_CreateString
//
// Creates an EDF string object with the given value which is hashable
// by one or two different keys. The mnemonic key is required and must
// be 32 or fewer characters long. The numeric key is optional. If a
// negative value is passed as the numeric key, the object will not be 
// added to the numeric hash table.
//
edf_string_t *E_CreateString(const char *value, const char *key, int num)
{
   int keyval;
   edf_string_t *newStr;

   if((newStr = E_StringForName(key)))
   {
      // Modify existing object.
      free(newStr->string);
      newStr->string = strdup(value);

      // Modify numeric id and rehash object if necessary
      if(num != newStr->numkey)
      {
         // If old key is >= 0, must remove from hash first
         if(newStr->numkey >= 0)
            E_DelStringFromNumHash(newStr);
         
         // Set new key
         newStr->numkey = num;
         
         // If new key >= 0, add back to hash
         if(newStr->numkey >= 0)
            E_AddStringToNumHash(newStr);
      }
   }
   else
   {
      // Create a new string object
      newStr = malloc(sizeof(edf_string_t));

      // init to zero
      memset(newStr, 0, sizeof(*newStr));
      
      // copy keys into string object
      if(strlen(key) > 32)
      {
         E_EDFLoggedErr(2, 
            "E_CreateString: invalid string mnemonic '%s'\n", key);
      }
      strncpy(newStr->key, key, 33);
      
      newStr->numkey = num;
      
      // duplicate value
      newStr->string = strdup(value);
      
      // add to hash tables
      
      keyval = D_HashTableKey(newStr->key) % NUM_EDFSTR_CHAINS;
      newStr->next = edf_str_chains[keyval];
      edf_str_chains[keyval] = newStr;
      
      // numeric key is not required
      if(num >= 0)
         E_AddStringToNumHash(newStr);
   }

   return newStr;
}

//
// E_StringForName
//
// Returns a pointer to an EDF string given a mnemonic value.
// Returns NULL if not found.
//
edf_string_t *E_StringForName(const char *key)
{
   int keyval = D_HashTableKey(key) % NUM_EDFSTR_CHAINS;
   edf_string_t *cur = edf_str_chains[keyval];

   while(cur && strncasecmp(cur->key, key, 33))
      cur = cur->next;

   return cur;
}

//
// E_GetStringForName
//
// As above, but causes an error if the string doesn't exist.
//
edf_string_t *E_GetStringForName(const char *key)
{
   edf_string_t *str = E_StringForName(key);

   if(!str)
      I_Error("E_GetStringForName: no such string '%s'\n", key);

   return str;
}

//
// E_StringForNum
//
// Returns an EDF string object for a numeric key.
// Returns NULL if not found.
//
edf_string_t *E_StringForNum(int num)
{
   int keyval = num % NUM_EDFSTR_CHAINS;
   edf_string_t *cur = edf_str_numchains[keyval];

   while(cur && cur->numkey != num)
      cur = (edf_string_t *)(cur->numlinks.next);

   return cur;
}

//
// E_GetStringForNum
//
// As above, but causes an error if the string doesn't exist.
//
edf_string_t *E_GetStringForNum(int num)
{
   edf_string_t *str = E_StringForNum(num);

   if(!str)
      I_Error("E_GetStringForNum: no such string with id #%d\n", num);

   return str;
}

//
// EDF Processing
//
//
// E_ProcessStrings
//
// 03/27/05: EDF can now define custom string objects.
// These are now processed first. This is extremely simple.
//
void E_ProcessStrings(cfg_t *cfg)
{
   unsigned int i, numstrings = cfg_size(cfg, EDF_SEC_STRING);

   E_EDFLogPrintf("\t* Processing strings\n"
                  "\t\t%d string(s) defined\n", numstrings);

   for(i = 0; i < numstrings; ++i)
   {
      cfg_t *sec = cfg_getnsec(cfg, EDF_SEC_STRING, i);
      const char *mnemonic, *value;
      int number;

      mnemonic = cfg_title(sec);
      value    = cfg_getstr(sec, ITEM_STRING_VAL);
      number   = cfg_getint(sec, ITEM_STRING_NUM);

      E_CreateString(value, mnemonic, number);

      E_EDFLogPrintf("\t\tDefined string '%s' (#%d)\n"
                     "\t\t\tvalue = '%s'\n",
                     mnemonic, number, value);
   }
}


// EOF

