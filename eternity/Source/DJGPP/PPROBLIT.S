// Doom screen copy, optimized for PPro and higher CPUs,
// where REP MOVSD is unoptimal. Uses 64-bit integers for
// the fastest data bandwidth. By Lee Killough 1/25/98

.text
.globl _ppro_blit
        .align 8
_ppro_blit:

        movl 8(%esp),%eax
        movl _screens,%ecx
        movl 4(%esp),%edx
        addl %eax,%ecx
        addl %eax,%edx
        shrl $3,%eax
        negl %eax

        .align 8,0x90

lp:     fildq   0(%ecx,%eax,8)
        fistpq  0(%edx,%eax,8)
        fildq   8(%ecx,%eax,8)
        fistpq  8(%edx,%eax,8)
        fildq  16(%ecx,%eax,8)
        fistpq 16(%edx,%eax,8)
        fildq  24(%ecx,%eax,8)
        fistpq 24(%edx,%eax,8)
        addl   $4,%eax
        js     lp
        ret

///////////////////////////////////////////////////////////////////////////////
//
// Same, but for Pentium(TM)
//

        .align 8
.globl _pent_blit
_pent_blit:
        pushl %esi
        pushl %edi
        movl 16(%esp),%ecx
        movl _screens,%esi
        movl 12(%esp),%edi
        shrl $4,%ecx
        .align 8,0x90
lp2:
        fildq   0(%esi)
        fildq   8(%esi)
        fxch
        fistpq  0(%edi)
        fistpq  8(%edi)
        addl    $16,%esi
        addl    $16,%edi
        decl    %ecx
        jne     lp2
        popl  %edi
        popl  %esi
        ret

///////////////////////////////////////////////////////////////////////////////
//
// killough 8/15/98:
//
// Blasts the framebuffer to a 320x200 planar screen. Used for page-flipping.
//

        .align 8
.globl _blast
_blast:
        pushl %ebp
        pushl %edi
        pushl %esi
        pushl %ebx

// Index 2
        movl $0x3c4,%edx
        movb $2,%al
        outb %al,%dx
        incl %edx

// Plane #0

        movl 20(%esp),%edi
        movb $1,%al
        movl 24(%esp),%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L0:
        movb  0(%esi),%al
        movb  8(%esi),%bl
        movb 16(%esi),%cl
        movb 24(%esi),%dl

        movb  4(%esi),%ah
        movb 12(%esi),%bh
        movb 20(%esi),%ch
        movb 28(%esi),%dh

        movw %ax,0(%edi)
        movw %bx,2(%edi)
        movw %cx,4(%edi)
        movw %dx,6(%edi)

        decl %ebp

        movb 32(%esi),%al
        movb 40(%esi),%bl
        movb 48(%esi),%cl
        movb 56(%esi),%dl
        movb 36(%esi),%ah
        movb 44(%esi),%bh
        movb 52(%esi),%ch
        movb 60(%esi),%dh

        leal 64(%esi),%esi

        movw %ax,8(%edi)
        movw %bx,10(%edi)
        movw %cx,12(%edi)
        movw %dx,14(%edi)

        leal 16(%edi),%edi
        jne L0

// Plane #1

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $2,%al
        movl 24(%esp),%esi
        incl %esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L1:
        movb  0(%esi),%al
        movb  8(%esi),%bl
        movb 16(%esi),%cl
        movb 24(%esi),%dl

        movb  4(%esi),%ah
        movb 12(%esi),%bh
        movb 20(%esi),%ch
        movb 28(%esi),%dh

        movw %ax,0(%edi)
        movw %bx,2(%edi)
        movw %cx,4(%edi)
        movw %dx,6(%edi)

        decl %ebp

        movb 32(%esi),%al
        movb 40(%esi),%bl
        movb 48(%esi),%cl
        movb 56(%esi),%dl
        movb 36(%esi),%ah
        movb 44(%esi),%bh
        movb 52(%esi),%ch
        movb 60(%esi),%dh

        leal 64(%esi),%esi

        movw %ax,8(%edi)
        movw %bx,10(%edi)
        movw %cx,12(%edi)
        movw %dx,14(%edi)

        leal 16(%edi),%edi
        jne L1

// Plane #2

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $4,%al
        movl 24(%esp),%esi
        addl $2,%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L2:
        movb  0(%esi),%al
        movb  8(%esi),%bl
        movb 16(%esi),%cl
        movb 24(%esi),%dl

        movb  4(%esi),%ah
        movb 12(%esi),%bh
        movb 20(%esi),%ch
        movb 28(%esi),%dh

        movw %ax,0(%edi)
        movw %bx,2(%edi)
        movw %cx,4(%edi)
        movw %dx,6(%edi)

        decl %ebp

        movb 32(%esi),%al
        movb 40(%esi),%bl
        movb 48(%esi),%cl
        movb 56(%esi),%dl
        movb 36(%esi),%ah
        movb 44(%esi),%bh
        movb 52(%esi),%ch
        movb 60(%esi),%dh

        leal 64(%esi),%esi

        movw %ax,8(%edi)
        movw %bx,10(%edi)
        movw %cx,12(%edi)
        movw %dx,14(%edi)

        leal 16(%edi),%edi
        jne L2

// Plane #3

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $8,%al
        movl 24(%esp),%esi
        addl $3,%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L3:
        movb  0(%esi),%al
        movb  8(%esi),%bl
        movb 16(%esi),%cl
        movb 24(%esi),%dl

        movb  4(%esi),%ah
        movb 12(%esi),%bh
        movb 20(%esi),%ch
        movb 28(%esi),%dh

        movw %ax,0(%edi)
        movw %bx,2(%edi)
        movw %cx,4(%edi)
        movw %dx,6(%edi)

        decl %ebp

        movb 32(%esi),%al
        movb 40(%esi),%bl
        movb 48(%esi),%cl
        movb 56(%esi),%dl
        movb 36(%esi),%ah
        movb 44(%esi),%bh
        movb 52(%esi),%ch
        movb 60(%esi),%dh

        leal 64(%esi),%esi

        movw %ax,8(%edi)
        movw %bx,10(%edi)
        movw %cx,12(%edi)
        movw %dx,14(%edi)

        leal 16(%edi),%edi
        jne L3

        popl %ebx
        popl %esi
        popl %edi
        popl %ebp
        ret

///////////////////////////////////////////////////////////////////////////////
//
// killough 8/15/98:
//
// Same, but optimized for Pentium Pro, to avoid partial stalls and limit
// memory acesses, by dealing only with whole doublewords.
//

        .align 8
.globl _ppro_blast
_ppro_blast:
        pushl %ebp
        pushl %edi
        pushl %esi
        pushl %ebx

// Index 2
        movl $0x3c4,%edx
        movb $2,%al
        outb %al,%dx
        incl %edx

// Plane #0

        movl 20(%esp),%edi
        movb $1,%al
        movl 24(%esp),%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L0p:
        movzbl 12(%esi),%eax
        shll   $8,%eax
        movzbl  8(%esi),%ebx
        addl   %ebx,%eax
        movzbl  4(%esi),%ecx
        shll   $8,%eax
        movzbl  0(%esi),%edx
        addl   %ecx,%eax
        movzbl 28(%esi),%ebx
        shll   $8,%eax
        movzbl 24(%esi),%ecx
        shll   $8,%ebx
        addl   %edx,%eax
        movzbl 20(%esi),%edx
        addl   %ecx,%ebx
        movl   %eax,0(%edi)
        movzbl 16(%esi),%eax
        shll   $8,%ebx
        addl   %edx,%ebx
        movzbl 44(%esi),%ecx
        shll   $8,%ebx
        movzbl 40(%esi),%edx
        addl   %eax,%ebx
        shll   $8,%ecx
        movzbl 36(%esi),%eax
        addl   %edx,%ecx
        movl   %ebx,4(%edi)
        shll   $8,%ecx
        movzbl 32(%esi),%ebx
        addl   %eax,%ecx
        shll   $8,%ecx
        movzbl 60(%esi),%edx
        addl   %ebx,%ecx
        movl   %ecx,8(%edi)
        shll   $8,%edx
        movzbl 56(%esi),%eax
        addl   %eax,%edx
        movzbl 52(%esi),%ebx
        shll   $8,%edx
        movzbl 48(%esi),%ecx
        addl   %ebx,%edx
        leal 64(%esi),%esi
        shll   $8,%edx
        addl   %ecx,%edx
        movl   %edx,12(%edi)
        decl %ebp
        leal 16(%edi),%edi
        jne L0p

// Plane #1

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $2,%al
        movl 24(%esp),%esi
        incl %esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L1p:
        movzbl 12(%esi),%eax
        shll   $8,%eax
        movzbl  8(%esi),%ebx
        addl   %ebx,%eax
        movzbl  4(%esi),%ecx
        shll   $8,%eax
        movzbl  0(%esi),%edx
        addl   %ecx,%eax
        movzbl 28(%esi),%ebx
        shll   $8,%eax
        movzbl 24(%esi),%ecx
        shll   $8,%ebx
        addl   %edx,%eax
        movzbl 20(%esi),%edx
        addl   %ecx,%ebx
        movl   %eax,0(%edi)
        movzbl 16(%esi),%eax
        shll   $8,%ebx
        addl   %edx,%ebx
        movzbl 44(%esi),%ecx
        shll   $8,%ebx
        movzbl 40(%esi),%edx
        addl   %eax,%ebx
        shll   $8,%ecx
        movzbl 36(%esi),%eax
        addl   %edx,%ecx
        movl   %ebx,4(%edi)
        shll   $8,%ecx
        movzbl 32(%esi),%ebx
        addl   %eax,%ecx
        shll   $8,%ecx
        movzbl 60(%esi),%edx
        addl   %ebx,%ecx
        movl   %ecx,8(%edi)
        shll   $8,%edx
        movzbl 56(%esi),%eax
        addl   %eax,%edx
        movzbl 52(%esi),%ebx
        shll   $8,%edx
        movzbl 48(%esi),%ecx
        addl   %ebx,%edx
        leal 64(%esi),%esi
        shll   $8,%edx
        addl   %ecx,%edx
        movl   %edx,12(%edi)
        decl %ebp
        leal 16(%edi),%edi
        jne L1p

// Plane #2

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $4,%al
        movl 24(%esp),%esi
        addl $2,%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L2p:
        movzbl 12(%esi),%eax
        shll   $8,%eax
        movzbl  8(%esi),%ebx
        addl   %ebx,%eax
        movzbl  4(%esi),%ecx
        shll   $8,%eax
        movzbl  0(%esi),%edx
        addl   %ecx,%eax
        movzbl 28(%esi),%ebx
        shll   $8,%eax
        movzbl 24(%esi),%ecx
        shll   $8,%ebx
        addl   %edx,%eax
        movzbl 20(%esi),%edx
        addl   %ecx,%ebx
        movl   %eax,0(%edi)
        movzbl 16(%esi),%eax
        shll   $8,%ebx
        addl   %edx,%ebx
        movzbl 44(%esi),%ecx
        shll   $8,%ebx
        movzbl 40(%esi),%edx
        addl   %eax,%ebx
        shll   $8,%ecx
        movzbl 36(%esi),%eax
        addl   %edx,%ecx
        movl   %ebx,4(%edi)
        shll   $8,%ecx
        movzbl 32(%esi),%ebx
        addl   %eax,%ecx
        shll   $8,%ecx
        movzbl 60(%esi),%edx
        addl   %ebx,%ecx
        movl   %ecx,8(%edi)
        shll   $8,%edx
        movzbl 56(%esi),%eax
        addl   %eax,%edx
        movzbl 52(%esi),%ebx
        shll   $8,%edx
        movzbl 48(%esi),%ecx
        addl   %ebx,%edx
        leal 64(%esi),%esi
        shll   $8,%edx
        addl   %ecx,%edx
        movl   %edx,12(%edi)
        decl %ebp
        leal 16(%edi),%edi
        jne L2p

// Plane #3

        movl $0x3c5,%edx
        movl 20(%esp),%edi
        movb $8,%al
        movl 24(%esp),%esi
        addl $3,%esi
        outb %al,%dx
        movl $1000,%ebp
        .align 8,0x90
L3p:
        movzbl 12(%esi),%eax
        shll   $8,%eax
        movzbl  8(%esi),%ebx
        addl   %ebx,%eax
        movzbl  4(%esi),%ecx
        shll   $8,%eax
        movzbl  0(%esi),%edx
        addl   %ecx,%eax
        movzbl 28(%esi),%ebx
        shll   $8,%eax
        movzbl 24(%esi),%ecx
        shll   $8,%ebx
        addl   %edx,%eax
        movzbl 20(%esi),%edx
        addl   %ecx,%ebx
        movl   %eax,0(%edi)
        movzbl 16(%esi),%eax
        shll   $8,%ebx
        addl   %edx,%ebx
        movzbl 44(%esi),%ecx
        shll   $8,%ebx
        movzbl 40(%esi),%edx
        addl   %eax,%ebx
        shll   $8,%ecx
        movzbl 36(%esi),%eax
        addl   %edx,%ecx
        movl   %ebx,4(%edi)
        shll   $8,%ecx
        movzbl 32(%esi),%ebx
        addl   %eax,%ecx
        shll   $8,%ecx
        movzbl 60(%esi),%edx
        addl   %ebx,%ecx
        movl   %ecx,8(%edi)
        shll   $8,%edx
        movzbl 56(%esi),%eax
        addl   %eax,%edx
        movzbl 52(%esi),%ebx
        shll   $8,%edx
        movzbl 48(%esi),%ecx
        addl   %ebx,%edx
        leal 64(%esi),%esi
        shll   $8,%edx
        addl   %ecx,%edx
        movl   %edx,12(%edi)
        decl %ebp
        leal 16(%edi),%edi
        jne L3p

        popl %ebx
        popl %esi
        popl %edi
        popl %ebp
        ret

//----------------------------------------------------------------------------
// $Id: pproblit.s,v 1.4 1998/02/23 04:53:25 killough Exp $
//----------------------------------------------------------------------------
//
// $Log: pproblit.s,v $
// Revision 1.4  1998/02/23  04:53:25  killough
// Performance tuning, add Pentium routine
//
// Revision 1.3  1998/02/09  03:12:04  killough
// Change blit to forward direction
//
// Revision 1.2  1998/01/26  19:31:17  phares
// First rev w/o ^Ms
//
// Revision 1.1  1998/01/26  05:51:52  killough
// PPro tuned blit
//
//----------------------------------------------------------------------------
