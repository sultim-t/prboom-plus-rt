// Emacs style mode select   -*- Assembler -*- 
//-----------------------------------------------------------------------------
//
// $Id: drawspan.S,v 1.2 2000/10/10 19:37:12 cph Exp $
//
//  LxDoom, a Doom port for Linux/Unix
//  based on BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//  Copyright 1999-2000 by Colin Phipps <cphipps@doomworld.com>
//  
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
//================
//
// R_DrawSpan
//
// Horizontal texture mapping
//
//================
//
// 2/14/98 Lee Killough
//
// Ported from the released linux source
// Converted from Intel to AT&T syntax
// Replaced self-modifying code with code that uses stack
// Removed dependence on 256-byte-aligned colormaps
//
//================
#include "drawasm.h"

.text
.align 8
.globl R_DrawSpan

R_DrawSpan:
 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx

//
// find loop count
//
 
// count = ds_x2 - ds_x1 + 1; 

 movl ds_x2,%eax 
 incl  %eax

// pixel count

 subl ds_x1,%eax

// nothing to scale
 jle  9f

// pixel count

 pushl %eax

//
// build composite position
//

 movl ds_xfrac,%ebp
 shll $10,%ebp
 andl $0x0ffff0000,%ebp
 movl ds_yfrac,%eax
 shrl $6,%eax
 andl $0xffff,%eax
 orl  %eax,%ebp

//
// calculate screen dest
//

//  dest = ylookup[ds_y] + columnofs[ds_x1];

 movl ds_y,%ebx
 movl ds_x1,%edi
 movl ds_source,%esi
 addl ylookup(,%ebx,4),%edi

//
// build composite step
//

 movl ds_xstep,%ebx
 shll $10,%ebx
 andl $0xffff0000,%ebx
 movl ds_ystep,%eax
 shrl $6,%eax
 andl $0xffff,%eax
 orl  %eax,%ebx
 pushl %ebx

// %eax, %ebx, %ecx,%edx scratch
// %esi  virtual source
// %edi  moving destination pointer
// %ebp  frac
 
// begin calculating third pixel (y units)
 shldl $22,%ebp,%ecx

// begin calculating third pixel (x units)
 shldl $6,%ebp,%ecx

// advance frac pointer
 addl  %ebx,%ebp

// finish calculation for third pixel
 andl  $4095,%ecx

// begin calculating fourth pixel (y units)
 shldl $22,%ebp,%edx

// begin calculating fourth pixel (x units)
 shldl $6,%ebp,%edx

// advance frac pointer
 addl %ebx,%ebp

// finish calculation for fourth pixel
 andl $4095,%edx

// get first pixel
 xorl %eax,%eax
 movb (%esi,%ecx),%al

// get second pixel
 xorl %ebx,%ebx
 movb (%esi,%edx),%bl

// offset first pixel into colormap
 addl ds_colormap,%eax

// offset second pixel into colormap
 addl ds_colormap,%ebx

 subl $2,4(%esp)

// color translate first pixel
 movb (%eax),%al

// color translate second pixel
 movb (%ebx),%bl
 
 jl 8f
 
// at least two pixels to map
	
 .align 8,0x90

1:
// begin calculating third pixel (y units)
 shldl $22,%ebp,%ecx

// begin calculating third pixel (x units)
 shldl $6,%ebp,%ecx

// advance frac pointer
 addl (%esp),%ebp

// write first pixel
 movb %al,(%edi)

// finish calculation for third pixel
 andl $4095,%ecx

// begin calculating fourth pixel (y units)
 shldl $22,%ebp,%edx

// begin calculating fourth pixel (x units)
 shldl $6,%ebp,%edx

// advance frac pointer
 addl (%esp),%ebp

// finish calculation for fourth pixel
 andl $4095,%edx

// write second pixel
 movb %bl,1(%edi)

// get third pixel
 xorl %eax,%eax
 movb (%esi,%ecx),%al

// get fourth pixel
 xorl %ebx,%ebx
 movb (%esi,%edx),%bl

// advance to third pixel destination
 addl $2,%edi

// offset third pixel into colormap
 addl ds_colormap,%eax

// offset fourth pixel into colormap
 addl ds_colormap,%ebx

// done with loop?
 subl $2,4(%esp)

// color translate third pixel
 movb (%eax),%al

// color translate fourth pixel
 movb (%ebx),%bl
 jge 1b

// check for final pixel
8:
 popl %ecx
 popl %edx
 jnp 9f

// write final pixel
 movb %al,(%edi)
9:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

