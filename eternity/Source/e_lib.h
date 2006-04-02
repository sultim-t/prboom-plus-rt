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

#ifndef E_LIB_H__
#define E_LIB_H__

typedef struct E_Enable_s
{
   const char *name;
   int enabled;
} E_Enable_t;

// basic stuff
void E_ErrorCB(cfg_t *cfg, const char *fmt, va_list ap);
const char *E_BuildDefaultFn(const char *filename);

// function callbacks
int E_Include    (cfg_t *, cfg_opt_t *, int, const char **);
int E_LumpInclude(cfg_t *, cfg_opt_t *, int, const char **);
int E_IncludePrev(cfg_t *, cfg_opt_t *, int, const char **);
int E_StdInclude (cfg_t *, cfg_opt_t *, int, const char **);
int E_Endif      (cfg_t *, cfg_opt_t *, int, const char **);

// value-parsing callbacks
int E_SpriteFrameCB(cfg_t *, cfg_opt_t *, const char *, void *);
int E_IntOrFixedCB (cfg_t *, cfg_opt_t *, const char *, void *);
int E_TranslucCB   (cfg_t *, cfg_opt_t *, const char *, void *);
int E_ColorStrCB   (cfg_t *, cfg_opt_t *, const char *, void *);

// misc utilities
int E_EnableNumForName(const char *name, E_Enable_t *enables);
int E_StrToNumLinear(const char **strings, int numstrings, const char *value);
long E_ParseFlags(const char *str, dehflagset_t *flagset);
char *E_ExtractPrefix(char *value, char *prefixbuf, int buflen);

#endif

// EOF


