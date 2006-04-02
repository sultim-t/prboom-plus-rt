// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:  
//    Common utilities for "Extended Feature" modules.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_io.h"
#include "m_misc.h"
#include "w_wad.h"
#include "psnprntf.h"
#include "d_main.h"
#include "i_system.h"
#include "d_dehtbl.h"
#include "v_video.h"

#include "Confuse/confuse.h"

#include <errno.h>

#include "e_lib.h"

// prototype of libConfuse parser inclusion function
extern int cfg_lexer_include(cfg_t *cfg, const char *filename, int data);

// 02/09/05: prototype of custom source type query function
extern int cfg_lexer_source_type(cfg_t *cfg);

extern void E_EDFLoggedErr(int lv, const char *msg, ...);

//
// Basic Functionality
//

//
// Default Error Callback
//
void E_ErrorCB(cfg_t *cfg, const char *fmt, va_list ap)
{
   I_ErrorVA(fmt, ap);
}


//
// Parser File/Lump Include Callback Functions
//

//
// E_Include
//
// The normal include function. cfg_include is insufficient since it 
// looks in the current working directory unless provided a full path.
// This function interprets paths relative to the  current file when 
// called from a physical input file, and uses the argument as a lump 
// name otherwise.
//
int E_Include(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
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

   // 02/09/05: support both files and lumps in this function, but
   // only one or the other depending on the calling context
   switch(cfg_lexer_source_type(cfg))
   {
   case -1: // physical file
      M_GetFilePath(cfg->filename, currentpath, sizeof(currentpath));
      psnprintf(filename, sizeof(filename), "%s/%s", currentpath, argv[0]);
      NormalizeSlashes(filename);
      return cfg_lexer_include(cfg, filename, -1);
   
   default: // data source
      if(strlen(argv[0]) > 8)
      {
         cfg_error(cfg, "include: %s is not a valid lump name", argv[0]);
         return 1;
      }
      return cfg_lexer_include(cfg, argv[0], W_GetNumForName(argv[0]));
   }
}

//
// E_LumpInclude
//
// Includes a WAD lump. Useful if you need to include a lump from a 
// file, since include() cannot do that.
//
int E_LumpInclude(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
{
   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to lumpinclude()");
      return 1;
   }
   if(strlen(argv[0]) > 8)
   {
      cfg_error(cfg, "lumpinclude: %s is not a valid lump name", argv[0]);
      return 1;
   }

   return cfg_lexer_include(cfg, argv[0], W_GetNumForName(argv[0]));
}

//
// E_IncludePrev
//
// Includes the next WAD lump on the lumpinfo hash chain of the same 
// name as the current lump being processed (to the user, this is
// the "previous" lump of that name). Enables recursive inclusion
// of like-named lumps to enable cascading behavior.
//
int E_IncludePrev(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
{
   int i;

   if(argc != 0)
   {
      cfg_error(cfg, "wrong number of args to include_prev()");
      return 1;
   }
   if(!cfg->filename)
   {
      cfg_error(cfg, "include_prev: cfg_t filename is undefined");
      return 1;
   }
   if((i = cfg_lexer_source_type(cfg)) < 0)
   {
      cfg_error(cfg, "include_prev: cannot call from file");
      return 1;
   }

   // Go down the hash chain and look for the next lump of the same
   // name within the global namespace.
   while((i = lumpinfo[i]->next) >= 0)
   {
      if(lumpinfo[i]->li_namespace == ns_global &&
         !strncasecmp(lumpinfo[i]->name, cfg->filename, 8))
      {
         return cfg_lexer_include(cfg, cfg->filename, i);
      }
   }

   // it is not an error if no such lump is found
   return 0;
}

//
// E_BuildDefaultFn
//
// Constructs the absolute file name for a default file. Don't cache 
// the returned pointer, since it points to a static buffer.
//
const char *E_BuildDefaultFn(const char *filename)
{
   static char buffer[PATH_MAX + 1];

   psnprintf(buffer, sizeof(buffer), "%s/%s", D_DoomExeDir(), filename);
   NormalizeSlashes(buffer);

   return buffer;
}

//
// E_StdInclude
//
// An include function that looks for files in the EXE's directory, as 
// opposed to the current directory.
//
int E_StdInclude(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
{
   const char *filename;

   if(argc != 1)
   {
      cfg_error(cfg, "wrong number of args to stdinclude()");
      return 1;
   }

   filename = E_BuildDefaultFn(argv[0]);

   return cfg_lexer_include(cfg, filename, -1);
}

//
// Enables
//

//
// E_EnableNumForName
//
// Gets the index of an enable value. Linear search on a small fixed 
// set. Note: Enable sets must be terminated with an all-zeroes entry.
//
int E_EnableNumForName(const char *name, E_Enable_t *enables)
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
// E_Endif
//
// 01/14/04: Returns the parser to normal after a conditional function.
//
int E_Endif(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
{
   cfg->flags &= ~CFGF_LOOKFORFUNC;
   cfg->lookfor = NULL;

   return 0;
}

//
// Resolution Functions
//

//
// E_StrToNumLinear
//
// Looks through an array of strings until a case-insensitive match
// is found, and then returns the index the match is found at. If no
// match is found, the array size is returned.
//
int E_StrToNumLinear(const char **strings, int numstrings, const char *value)
{
   int index = 0;

   while(index != numstrings && strcasecmp(strings[index], value))
      ++index;

   return index;
}

//
// E_ParseFlags
//
// Parses a single-word options/flags field.
//
long E_ParseFlags(const char *str, dehflagset_t *flagset)
{
   char *buffer, *bufptr;

   bufptr = buffer = strdup(str);

   deh_ParseFlags(flagset, &bufptr, NULL);

   free(buffer);

   return flagset->results[0];
}

//
// Value-Parsing Callbacks
//

//
// E_SpriteFrameCB
//
// libConfuse value-parsing callback function for sprite frame values. 
// Allows use of characters A through ], corresponding to the actual 
// sprite lump names (implemented by popular demand ;)
//
// This function is also called explicitly by E_ProcessCmpState.
// When this is done, the cfg and opt parameters are set to NULL,
// and will not be used.
//
int E_SpriteFrameCB(cfg_t *cfg, cfg_opt_t *opt, const char *value, 
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
// E_IntOrFixedCB
//
// libConfuse value-parsing callback for a thing speed field.
// Allows input of either integer or floating-point values, where
// the latter are converted to fixed-point for storage.
//
int E_IntOrFixedCB(cfg_t *cfg, cfg_opt_t *opt, const char *value, 
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
// E_TranslucCB
//
// libConfuse value-parsing callback for translucency fields. Can accept 
// an integer value or a percentage.
//
int E_TranslucCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
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

      *(long *)result = (FRACUNIT * pctvalue) / 100;
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
// E_ColorStrCB
//
// Accepts either a palette index or an RGB triplet, which will be
// matched to the closest color in the game palette.
//
int E_ColorStrCB(cfg_t *cfg, cfg_opt_t *opt, const char *value, 
                 void *result)
{
   char *endptr;
   
   *(long *)result = strtol(value, &endptr, 0);

   if(*endptr != '\0')
   {
      byte *palette;
      int r, g, b, ret;

      if((ret = sscanf(value, "%d %d %d", &r, &g, &b)) != 3)
      {
         if(cfg)
         {
            cfg_error(cfg,
               "invalid color triplet for option '%s'",
               opt->name);
         }
         return -1;
      }

      palette = W_CacheLumpName("PLAYPAL", PU_STATIC);

      *(long *)result = V_FindBestColor(palette, r, g, b);

      Z_ChangeTag(palette, PU_CACHE);
   }
   else if(errno == ERANGE) 
   {
      if(cfg)
      {
         cfg_error(cfg,
            "integer value for option '%s' is out of range",
            opt->name);
      }
      return -1;
   }

   return 0;
}

//
// Prefix:Value Syntax Parsing
//

//
// E_ExtractPrefix
//
// Returns the result of strchr called on the string in value.
// If the return is non-NULL, you can expect to find the extracted
// prefix written into prefixbuf. If the return value is NULL,
// prefixbuf is unmodified.
//
char *E_ExtractPrefix(char *value, char *prefixbuf, int buflen)
{
   int i;
   char *colonloc, *rover, *strval;

   // look for a colon ending a possible prefix
   colonloc = strchr(value, ':');

   if(colonloc)
   {
      strval = colonloc + 1;
      rover = value;
      i = 0;
      while(rover != colonloc && i < buflen - 1) // leave room for \0
      {
         prefixbuf[i] = *rover;
         ++rover;
         ++i;
      }
      
      // check validity of the string value location (could be end)
      if(!(*strval))
      {
         E_EDFLoggedErr(0,
            "E_ExtractPrefix: invalid prefix:value %s\n", value);
      }
   }

   return colonloc;
}

// EOF

