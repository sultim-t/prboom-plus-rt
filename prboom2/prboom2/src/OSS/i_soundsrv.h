/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_soundsrv.h,v 1.1 2000/09/20 09:34:33 figgi Exp $
 *
 *  Sound server for LxDoom, based on the sound server released with the 
 *   original linuxdoom sources.
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999-2000 by Colin Phipps
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
 *  Common header for soundserver data passing
 *-----------------------------------------------------------------------------*/

typedef struct {
  unsigned int sfxid;
  signed int link;
  unsigned int datalen;
} snd_pass_t;

/* I_GetLinkNum - returns linked sound number */
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static signed int I_GetLinkNum(unsigned int i)
{
  return  ((S_sfx[i].link == NULL) ? -1 :
	    ((S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)));
}

