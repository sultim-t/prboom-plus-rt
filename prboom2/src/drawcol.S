// Emacs style mode select   -*- Assembler -*- 
//-----------------------------------------------------------------------------
//
// $Id: drawcol.S,v 1.2 2000/10/10 19:37:12 cph Exp $
//
//  LxDoom, a Doom port for Linux/Unix
//  based on BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//  Copyright (C) 1999-2000 by Colin Phipps <cphipps@doomworld.com>
//  Copyright (C) 2000 by Jess Haas <JessH@lbjhs.net>
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
 .globl R_DrawColumn
R_DrawColumn:

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
 .globl R_DrawTLColumn
R_DrawTLColumn:

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

