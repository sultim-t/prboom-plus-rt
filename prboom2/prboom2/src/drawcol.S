// Emacs style mode select   -*- Assembler -*- 
//-----------------------------------------------------------------------------
//
// $Id: drawcol.S,v 1.1 2000/09/20 09:28:14 figgi Exp $
//
//  LxDoom, a Doom port for Linux/Unix
//  based on BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//   and Colin Phipps
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
// R_DrawColumn
//
//================
//
// 2/15/98 Lee Killough
//
// Converted C code with TFE fix to assembly and tuned
//
// 2/21/98 killough: added translucency support
//
//================
#include "drawasm.h"

 .text
 .align 8
 .globl R_DrawColumn_Normal
R_DrawColumn_Normal:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle 9f
 addl dc_x,%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 movl dc_colormap,%edi
 addl dc_texturemid,%edx
 cmpl $128,%eax
 je 7f
 testl %eax,%ecx
 je 5f
 sall $16,%eax

0:
 subl %eax,%edx
 jge 0b

1:
 addl %eax,%edx
 jl 1b

 .align 8,0x90
2:
 movl %edx,%ecx
 sarl $16,%ecx
 addl dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl $320,%ebx
 cmpl %eax,%edx
 jge 3f
 decl %esi
 jg 2b
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
3:
 subl %eax,%edx
 decl %esi
 jg 2b
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
9:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
4:
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,320(%ebx)
 addl $640,%ebx

5:
 addl $-2,%esi
 jge 4b
 jnp 9b
 sarl $16,%edx
 andl %ecx,%edx
 xorl %eax,%eax
 movb (%edx,%ebp),%al
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

7:				
; movl dc_iscale, %ecx
 movl %edi,%ecx		
 andl $0x007fffff,%edx
 movl dc_iscale, %edi
 addl $-2, %esi
 jl 8f 
 .align 8
6:
 movl %edx,%eax
 sarl $16,%edx
 addl %edi,%eax
 andl $0x007fffff,%eax
 movb (%edx,%ebp),%cl
 movl %eax,%edx
 sarl $16,%eax
 movb (%ecx),%cl
 addl %edi,%edx
 movb %cl,(%ebx)
 movb (%eax,%ebp),%cl
 addl $640,%ebx
 andl $0x007fffff,%edx
 movb (%ecx),%cl
 addl $-2,%esi
 movb %cl,-320(%ebx)
 jge 6b
8:
 jnp 9b
 sarl $16,%edx
 movl %ebx,%esi
 movb (%edx,%ebp),%cl
 popl %ebx
 popl %edi
 movb (%ecx),%cl
 movb %cl,(%esi)
 popl %esi
 popl %ebp
 ret

//================
//
// R_DrawTLColumn
//
// Translucency support
//
//================

 .align 8
 .globl R_DrawTLColumn_Normal
R_DrawTLColumn_Normal:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle 9f
 addl dc_x,%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 movl dc_colormap,%edi
 addl dc_texturemid,%edx
 testl %eax,%ecx
 pushl %ecx
 je 5f
 sall $16,%eax

0:
 subl %eax,%edx
 jge 0b

1:
 addl %eax,%edx
 jl 1b
 pushl %esi

 .align 8,0x90
2:
 xorl %ecx,%ecx
 movl %edx,%esi
 movb (%ebx),%cl
 shll $8,%ecx
 sarl $16,%esi
 addl tranmap,%ecx
 addl dc_iscale,%edx
 movzbl (%esi,%ebp),%esi
 movzbl (%edi,%esi),%esi
 movb (%ecx,%esi),%cl
 movb %cl,(%ebx)
 addl $320,%ebx
 cmpl %eax,%edx
 jge 3f
 decl (%esp)
 jg 2b
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
3:	
 subl %eax,%edx
 decl (%esp)
 jg 2b
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
9:
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
4:
 movl %edx,%eax
 xorl %ecx,%ecx
 addl dc_iscale,%edx
 movb (%ebx),%cl
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 xorl %ecx,%ecx
 movb %al,(%ebx)
 movb 320(%ebx),%cl
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,320(%ebx)
 addl $640,%ebx

5:
 addl $-2,%esi
 jge 4b
 jnp 9b
 xorl %ecx,%ecx
 sarl $16,%edx
 movb (%ebx),%cl
 andl (%esp),%edx
 shll $8,%ecx
 xorl %eax,%eax
 addl tranmap,%ecx
 movb (%edx,%ebp),%al
 movzbl (%eax,%edi),%eax
 movb (%ecx,%eax),%al 
 movb %al,(%ebx)
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//================
//
// R_DrawColumn
//
//================
//
// 2/15/98 Lee Killough
//
// Converted C code with TFE fix to assembly and tuned
//
// 2/21/98 killough: added translucency support
//
//================

 .text
 .align 8
 R_DrawColumn_Highres_NoWrap:
// cph - just to separate its profiling stats
7:				
; movl dc_iscale, %ecx
 movl %edi,%ecx		
 movl SCREENWIDTH, %edi
 subl %edi,%ebx
 addl $-2, %esi
 jl 8f 
 .align 8
6:
 movl %edx,%eax
 addl %edi,%ebx
 sarl $16,%edx
 addl dc_iscale,%eax
 movb (%edx,%ebp),%cl
 movl %eax,%edx
 sarl $16,%eax
 movb (%ecx),%cl
 addl dc_iscale,%edx
 movb %cl,(%ebx)
 movb (%eax,%ebp),%cl
 addl %edi,%ebx
 movb (%ecx),%cl
 addl $-2,%esi
 movb %cl,(%ebx)
 jge 6b
8:
 jnp 9f
 sarl $16,%edx
 leal (%edi,%ebx),%esi
 movb (%edx,%ebp),%cl
 popl %ebx
 popl %edi
 movb (%ecx),%cl
 movb %cl,(%esi)
 popl %esi
 popl %ebp
 ret

 .align 8
 .globl R_DrawColumn_HighRes
R_DrawColumn_HighRes:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle 9f
 addl dc_x,%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 addl dc_texturemid,%edx
 testl %eax,%eax
 movl dc_colormap,%edi
 jz 7b
 cmpl $128,%eax
 je 7f
 testl %eax,%ecx
 je 5f
 sall $16,%eax

0:
 subl %eax,%edx
 jge 0b

1:
 addl %eax,%edx
 jl 1b

 .align 8,0x90
2:
 movl %edx,%ecx
 sarl $16,%ecx
 addl dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl SCREENWIDTH,%ebx
 cmpl %eax,%edx
 jge 3f
 decl %esi
 jg 2b
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
3:
 subl %eax,%edx
 decl %esi
 jg 2b
9:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
4:
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 addl SCREENWIDTH,%ebx
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 addl SCREENWIDTH,%ebx

5:
 addl $-2,%esi
 jge 4b
 jnp 9b
 sarl $16,%edx
 andl %ecx,%edx
 xorl %eax,%eax
 movb (%edx,%ebp),%al
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

R_DrawColumn_Highres_128:
// cph - just to separate its profiling stats
7:
 movl %edi,%ecx		
 movl SCREENWIDTH, %edi
 andl $0x007fffff,%edx
 subl %edi,%ebx
 addl $-2, %esi
 jl 8f 
 .align 8
6:
 movl %edx,%eax
 sarl $16,%edx
 addl dc_iscale,%eax
 addl %edi,%ebx
 andl $0x007fffff,%eax
 movb (%edx,%ebp),%cl
 movl %eax,%edx
 sarl $16,%eax
 movb (%ecx),%cl
 addl dc_iscale,%edx
 movb %cl,(%ebx)
 movb (%eax,%ebp),%cl
 addl %edi,%ebx
 andl $0x007fffff,%edx
 movb (%ecx),%cl
 addl $-2,%esi
 movb %cl,(%ebx)
 jge 6b
8:
 jnp 9b
 sarl $16,%edx
 leal (%edi,%ebx),%esi
 movb (%edx,%ebp),%cl
 popl %ebx
 popl %edi
 movb (%ecx),%cl
 movb %cl,(%esi)
 popl %esi
 popl %ebp
 ret

//================
//
// R_DrawTLColumn
//
// Translucency support
//
//================

 .align 8
 .globl R_DrawTLColumn_HighRes
R_DrawTLColumn_HighRes:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle 9f
 addl dc_x,%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 addl viewwindowx,%eax
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 movl dc_colormap,%edi
 addl dc_texturemid,%edx
 testl %eax,%ecx
 pushl %ecx
 je 5f
 sall $16,%eax

0:
 subl %eax,%edx
 jge 0b

1:
 addl %eax,%edx
 jl 1b
 pushl %esi

 .align 8,0x90
2:
 xorl %ecx,%ecx
 movl %edx,%esi
 movb (%ebx),%cl
 shll $8,%ecx
 sarl $16,%esi
 addl tranmap,%ecx
 addl dc_iscale,%edx
 movzbl (%esi,%ebp),%esi
 movzbl (%edi,%esi),%esi
 movb (%ecx,%esi),%cl
 movb %cl,(%ebx)
 addl SCREENWIDTH,%ebx
 cmpl %eax,%edx
 jge 3f
 decl (%esp)
 jg 2b
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
3:	
 subl %eax,%edx
 decl (%esp)
 jg 2b
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
9:
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
4:
 movl %edx,%eax
 xorl %ecx,%ecx
 addl dc_iscale,%edx
 movb (%ebx),%cl
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 xorl %ecx,%ecx
 movb %al,(%ebx)
 addl SCREENWIDTH,%ebx
 movl %edx,%eax
 movb (%ebx),%cl
 addl dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,(%ebx)
 addl SCREENWIDTH,%ebx
5:
 addl $-2,%esi
 jge 4b
 jnp 9b
 xorl %ecx,%ecx
 sarl $16,%edx
 movb (%ebx),%cl
 andl (%esp),%edx
 shll $8,%ecx
 xorl %eax,%eax
 addl tranmap,%ecx
 movb (%edx,%ebp),%al
 movzbl (%eax,%edi),%eax
 movb (%ecx,%eax),%al 
 movb %al,(%ebx)
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//----------------------------------------------------------------------------
// $Id: drawcol.S,v 1.1 2000/09/20 09:28:14 figgi Exp $
//----------------------------------------------------------------------------
//
// $Log: drawcol.S,v $
// Revision 1.1  2000/09/20 09:28:14  figgi
// Initial revision
//
// Revision 1.1  2000/05/21 04:07:39  jessh
// Fixed assembly linking _ problem (i could have sworn i already commited this)
//
// Revision 1.1.1.1  2000/05/04 08:01:17  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.13  2000/05/01 17:50:33  Proff
// made changes to compile with VisualC and SDL
//
// Revision 1.12  1999/12/13 17:24:45  cphipps
// Fix compiling on OpenBSD (fussy assembler)
//
// Revision 1.11  1999/10/12 13:04:17  cphipps
// Added GPL header
//
// Revision 1.10  1999/09/19 10:36:03  cphipps
// Add special case in DrawColumn_Highres for non wrapping columns
//
// Revision 1.9  1999/01/29 21:35:36  cphipps
// No longer use columnofs lookup array
//
// Revision 1.8  1999/01/26 09:47:37  cphipps
// End of 128-column runs updated to be faster and display last pixel correctly
//
// Revision 1.7  1999/01/25 22:46:31  cphipps
// Optimise for 256-byte aligned colourmaps in critical loops
//
// Revision 1.6  1999/01/25 20:23:30  cphipps
// Minor change to show separate profiling for 128-byte loop
// Optimise 128-byte loop for 256-byte aligned colourmaps
//
// Revision 1.5  1998/11/17 16:38:01  cphipps
// Added alternate versions for hi-res, renamed funcs
//
// Revision 1.4  1998/10/10 20:31:25  cphipps
// Further tweaking of 128-high texture loop
//
// Revision 1.3  1998/10/07 11:39:27  cphipps
// Changed a couple more symbols to be local.
// Added rendering loop specific to 128 high textures, since that is the
// standard.
//
// Revision 1.2  1998/10/07 11:15:39  cphipps
// Changed internal labels to local symbols
//
// Revision 1.1  1998/09/21 17:30:58  cphipps
// Initial revision
//
// Revision 1.3  1998/09/13 11:22:16  cphipps
// Removed _ prefix from function names (silly me)
//
// Revision 1.2  1998/09/12 17:11:42  cphipps
// Removed prepended _'s on C variables
//
// Revision 1.1  1998/09/12 09:41:15  cphipps
// Initial revision
//
// Revision 1.3  1998/03/04  12:33:29  killough
// Fix problem with last translucent pixel being drawn
//
// Revision 1.2  1998/02/23  04:18:24  killough
// Add translucency support, more tuning
//
// Revision 1.1  1998/02/17  06:37:37  killough
// Initial version
//
//
//----------------------------------------------------------------------------
