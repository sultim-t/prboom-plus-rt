/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: g_config.c,v 1.2 2002/08/10 18:23:07 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2002 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright (C) 2002 by Robert Sherwood
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 * Load/Save defaults
 *
 * By Simon Howard, Revised by James Haley, added to PrBoom by Florian Schulze.
 * Split to a separate file by Robert Sherwood & Colin Phipps.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "m_misc.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "g_config.h"
#include "g_bind.h"
#include "g_bindaxes.h"

//===========================================================================
//
// Load/Save defaults
//
//===========================================================================

// default script:

static char *cfg_file = NULL; 

void G_LoadDefaults(const char *file)
{
  byte *cfg_data;

  cfg_file = strdup(file);

  if(M_ReadFile(cfg_file, &cfg_data) <= 0)
  {
      C_Printf("cfg not found.\n");
      //C_Printf("cfg not found. using default\n");
      //cfg_data = W_CacheLumpName("DEFAULT", PU_STATIC);
  } else {
    C_RunScript(cfg_data);

    free(cfg_data);
  }

  //Add G_SaveDefaults as an exit handler
  atexit(G_SaveDefaults);
}

void G_SaveDefaults()
{
  FILE *file;

  if(!cfg_file)         // check defaults have been loaded
     return;

  file = fopen(cfg_file, "w");

  // write console variables
  C_WriteVariables(file);
  
  // write key bindings
  G_WriteBindings(file);
  
  // write axis bindings
  G_WriteAxisBindings(file);
  
  fclose(file);
}

