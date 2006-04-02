// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley
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
// DESCRIPTION:
//
// Fixed-point support functions for Small
//
//----------------------------------------------------------------------------

#include "a_small.h"
#include "m_fixed.h"

static cell AMX_NATIVE_CALL sm_floattofixed(AMX *amx, cell *params)
{
   return (cell)(amx_ctof(params[1]) * FRACUNIT);
}

static cell AMX_NATIVE_CALL sm_fmul(AMX *amx, cell *params)
{
   return FixedMul(params[1], params[2]);
}

static cell AMX_NATIVE_CALL sm_fdiv(AMX *amx, cell *params)
{
   if(params[2] == 0)
   {
      amx_RaiseError(amx, AMX_ERR_DIVIDE);
      return -1;
   }

   return FixedDiv(params[1], params[2]);
}

static cell AMX_NATIVE_CALL sm_fabs(AMX *amx, cell *params)
{
   return D_abs(params[1]);
}

AMX_NATIVE_INFO fixed_Natives[] =
{
   { "_ffloat", sm_floattofixed },
   { "_fmul",   sm_fmul },
   { "_fdiv",   sm_fdiv },
   { "_fabs",   sm_fabs },
   { NULL,           NULL }
};

// EOF

