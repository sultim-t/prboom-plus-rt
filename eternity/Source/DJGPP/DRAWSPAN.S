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

.text
.align 8
.globl _R_DrawSpan

_R_DrawSpan:
 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx

//
// find loop count
//
 
// count = ds_x2 - ds_x1 + 1; 

 movl _ds_x2,%eax 
 incl  %eax

// pixel count

 subl _ds_x1,%eax

// nothing to scale
 jle  hdone

// pixel count

 pushl %eax

//
// build composite position
//

 movl _ds_xfrac,%ebp
 shll $10,%ebp
 andl $0x0ffff0000,%ebp
 movl _ds_yfrac,%eax
 shrl $6,%eax
 andl $0xffff,%eax
 orl  %eax,%ebp

//
// calculate screen dest
//

//  dest = ylookup[ds_y] + columnofs[ds_x1];

 movl _ds_y,%ebx
 movl _ds_x1,%eax
 movl _ds_source,%esi
 movl _ylookup(,%ebx,4),%edi
 addl _columnofs(,%eax,4),%edi

//
// build composite step
//

 movl _ds_xstep,%ebx
 shll $10,%ebx
 andl $0xffff0000,%ebx
 movl _ds_ystep,%eax
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
 addl _ds_colormap,%eax

// offset second pixel into colormap
 addl _ds_colormap,%ebx

 subl $2,4(%esp)

// color translate first pixel
 movb (%eax),%al

// color translate second pixel
 movb (%ebx),%bl
 
 jl hchecklast
 
// at least two pixels to map
	
 .align 8,0x90

hdoubleloop:
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
 addl _ds_colormap,%eax

// offset fourth pixel into colormap
 addl _ds_colormap,%ebx

// done with loop?
 subl $2,4(%esp)

// color translate third pixel
 movb (%eax),%al

// color translate fourth pixel
 movb (%ebx),%bl
 jge hdoubleloop

// check for final pixel
hchecklast:
 popl %ecx
 popl %edx
 jnp hdone

// write final pixel
 movb %al,(%edi)
hdone:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//----------------------------------------------------------------------------
// $Id: drawspan.s,v 1.2 1998/02/23 04:18:51 killough Exp $
//----------------------------------------------------------------------------
//
// $Log: drawspan.s,v $
// Revision 1.2  1998/02/23  04:18:51  killough
// Performance tuning
//
// Revision 1.1  1998/02/17  06:37:34  killough
// Initial version
//
//
//----------------------------------------------------------------------------
