// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//
// GFS -- Game File Script
//
// This is a radical new way for DOOM editing projects to provide
// file information to Eternity. A single GFS script, when provided
// via the -gfs command-line parameter, will affect the loading of
// an arbitrary number of WADs, DEH/BEX's, EDF files, and console
// scripts.
//
// By James Haley
//
//--------------------------------------------------------------------------

#include "z_zone.h"
#include "i_system.h"
#include "doomtype.h"
#include "m_misc.h"
#include "g_gfs.h"

#include "Confuse/confuse.h"

static gfs_t gfs;

#define SEC_WADFILE "wadfile"
#define SEC_DEHFILE "dehfile"
#define SEC_CSCFILE "cscfile"
#define SEC_EDFFILE "edffile"
#define SEC_IWAD    "iwad"
#define SEC_BASEPATH "basepath"

static cfg_opt_t gfs_opts[] =
{
   CFG_STR(SEC_WADFILE,  0, CFGF_MULTI),
   CFG_STR(SEC_DEHFILE,  0, CFGF_MULTI),
   CFG_STR(SEC_CSCFILE,  0, CFGF_MULTI),
   CFG_STR(SEC_EDFFILE,  0, CFGF_NONE),
   CFG_STR(SEC_IWAD,     0, CFGF_NONE),
   CFG_STR(SEC_BASEPATH, 0, CFGF_NONE),
   CFG_END()
};

static void gfs_error(cfg_t *cfg, const char *fmt, va_list ap)
{
   I_ErrorVA(fmt, ap);
}

gfs_t *G_LoadGFS(const char *filename)
{
   static boolean loaded = false;
   int i;
   cfg_t *cfg;

   // only one GFS can be loaded per session
   if(loaded)
      return NULL;

   cfg = cfg_init(gfs_opts, CFGF_NOCASE);

   cfg_set_error_function(cfg, gfs_error);

   if(cfg_parse(cfg, filename))
      I_Error("G_LoadGFS: failed to parse GFS file\n");

   // count number of options
   gfs.numwads = cfg_size(cfg, SEC_WADFILE);
   gfs.numdehs = cfg_size(cfg, SEC_DEHFILE);
   gfs.numcsc  = cfg_size(cfg, SEC_CSCFILE);

   if(gfs.numwads)
      gfs.wadnames = malloc(gfs.numwads * sizeof(char *));

   if(gfs.numdehs)
      gfs.dehnames = malloc(gfs.numdehs * sizeof(char *));

   if(gfs.numcsc)
      gfs.cscnames = malloc(gfs.numcsc  * sizeof(char *));

   // load wads, dehs, csc
   for(i = 0; i < gfs.numwads; i++)
   {
      char *str = cfg_getnstr(cfg, SEC_WADFILE, i);

      gfs.wadnames[i] = strdup(str);
   }
   for(i = 0; i < gfs.numdehs; i++)
   {
      char *str = cfg_getnstr(cfg, SEC_DEHFILE, i);
      
      gfs.dehnames[i] = strdup(str);
   }
   for(i = 0; i < gfs.numcsc; i++)
   {
      char *str = cfg_getnstr(cfg, SEC_CSCFILE, i);

      gfs.cscnames[i] = strdup(str);
   }

   // haleyjd 07/05/03: support root EDF specification
   if(cfg_size(cfg, SEC_EDFFILE) >= 1)
   {
      char *str = cfg_getstr(cfg, SEC_EDFFILE);

      strncpy(gfs.edf, str, PATH_MAX + 1);

      gfs.hasEDF = true;
   }

   // haleyjd 04/16/03: support iwad specification for end-users
   // (this is not useful to mod authors, UNLESS their mod happens
   // to be an IWAD file ;)
   if(cfg_size(cfg, SEC_IWAD) >= 1)
   {
      char *str = cfg_getstr(cfg, SEC_IWAD);

      strncpy(gfs.iwad, str, PATH_MAX + 1);

      gfs.hasIWAD = true;
   }

   // haleyjd 04/16/03: basepath support
   if(cfg_size(cfg, SEC_BASEPATH) >= 1)
   {
      char *str = cfg_getstr(cfg, SEC_BASEPATH);

      strncpy(gfs.filepath, str, PATH_MAX + 1);
   }
   else
      M_GetFilePath(filename, gfs.filepath, PATH_MAX + 1);

   cfg_free(cfg);

   loaded = true;

   return &gfs;
}

void G_FreeGFS(gfs_t *gfs)
{
   int i;

   // free all filenames, and then free the arrays they were in
   // as well

   for(i = 0; i < gfs->numwads; i++)
   {
      free(gfs->wadnames[i]);
   }
   free(gfs->wadnames);

   for(i = 0; i < gfs->numdehs; i++)
   {
      free(gfs->dehnames[i]);
   }
   free(gfs->dehnames);

   for(i = 0; i < gfs->numcsc; i++)
   {
      free(gfs->cscnames[i]);
   }
   free(gfs->cscnames);

   memset(gfs->edf, 0, PATH_MAX + 1);
   gfs->hasEDF = false;

   memset(gfs->iwad, 0, PATH_MAX + 1);
   gfs->hasIWAD = false;

   memset(gfs->filepath, 0, PATH_MAX + 1);
}

//
// G_GFSCheckIWAD
//
// Checks to see if a GFS IWAD has been loaded.
// Convenience function to avoid rewriting too much of FindIWADFile
// Only valid while the GFS is still loaded; returns NULL otherwise,
// and when there is no GFS IWAD specified.
//
const char *G_GFSCheckIWAD(void)
{
   if(gfs.hasIWAD)
      return gfs.iwad;
   else
      return NULL;
}

//
// G_GFSCheckEDF
//
// Like above, but checks if an EDF file has been loaded
//
const char *G_GFSCheckEDF(void)
{
   if(gfs.hasEDF)
      return gfs.edf;
   else
      return NULL;
}

// EOF

