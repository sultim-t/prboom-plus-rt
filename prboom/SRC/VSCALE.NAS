BITS 32

; Segment/section definition macros. 

%ifdef M_TARGET_WATCOM
  SEGMENT DATA PUBLIC ALIGN=16 CLASS=DATA USE32
  SEGMENT DATA
%else
  SECTION .data
%endif

; External variables:
extern _screens
extern _ScaledVMem
extern _ViewMemPitch
extern _SCREENWIDTH
extern _SCREENHEIGHT


%ifdef M_TARGET_WATCOM
  SEGMENT CODE PUBLIC ALIGN=16 CLASS=CODE USE32
  SEGMENT CODE
%else
  SECTION .text
%endif


global _V_ScaleBy2Da
global _V_ScaleBy2Ua

_V_ScaleBy2Da: ; basic label
push     ebp
push     esi
push     edi
push     ebx

mov        esi, dword [_screens]
mov        edi, dword [_ScaledVMem]
mov        ebp, edi
add        ebp, dword [_ViewMemPitch]
mov        edx, dword [_SCREENHEIGHT]
yloopD:
mov        ecx, dword [_SCREENWIDTH]
shr        ecx, 4
xloopD:
mov        al, byte [esi+1]
mov        bl, byte [esi+3]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi]
mov        bl, byte [esi+2]
mov        ah, al
mov        bh, bl
mov        dword [edi], eax
mov        dword [edi+4], ebx
mov        dword [ebp], eax
mov        dword [ebp+4], ebx
mov        al, byte [esi+5]
mov        bl, byte [esi+7]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+4]
mov        bl, byte [esi+6]
mov        ah, al
mov        bh, bl
mov        dword [edi+8], eax
mov        dword [edi+12], ebx
mov        dword [ebp+8], eax
mov        dword [ebp+12], ebx
mov        al, byte [esi+9]
mov        bl, byte [esi+11]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+8]
mov        bl, byte [esi+10]
mov        ah, al
mov        bh, bl
mov        dword [edi+16], eax
mov        dword [edi+20], ebx
mov        dword [ebp+16], eax
mov        dword [ebp+20], ebx
mov        al, byte [esi+13]
mov        bl, byte [esi+15]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+12]
mov        bl, byte [esi+14]
mov        ah, al
mov        bh, bl
add        esi,16
mov        dword [edi+24], eax
mov        dword [edi+28], ebx
add        edi,32
mov        dword [ebp+24], eax
mov        dword [ebp+28], ebx
add        ebp,32
dec        ecx
jnz        near xloopD
add        edi,dword [_ViewMemPitch]
add        ebp,dword [_ViewMemPitch]
dec        edx
jnz        near yloopD

pop     ebx
pop     edi
pop     esi
pop     ebp
ret

_V_ScaleBy2Ua: ; basic label
push     ebp
push     esi
push     edi
push     ebx

mov        esi, dword [_SCREENHEIGHT]
dec        esi
imul       esi, dword [_SCREENWIDTH]
add        esi, dword [_screens]
mov        edi, dword [_ScaledVMem]
mov        ebp, edi
add        ebp, dword [_ViewMemPitch]
mov        edx, dword [_SCREENHEIGHT]
yloopU:
mov        ecx, dword [_SCREENWIDTH]
shr        ecx, 4
xloopU:
mov        al, byte [esi+1]
mov        bl, byte [esi+3]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi]
mov        bl, byte [esi+2]
mov        ah, al
mov        bh, bl
mov        dword [edi], eax
mov        dword [edi+4], ebx
mov        dword [ebp], eax
mov        dword [ebp+4], ebx
mov        al, byte [esi+5]
mov        bl, byte [esi+7]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+4]
mov        bl, byte [esi+6]
mov        ah, al
mov        bh, bl
mov        dword [edi+8], eax
mov        dword [edi+12], ebx
mov        dword [ebp+8], eax
mov        dword [ebp+12], ebx
mov        al, byte [esi+9]
mov        bl, byte [esi+11]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+8]
mov        bl, byte [esi+10]
mov        ah, al
mov        bh, bl
mov        dword [edi+16], eax
mov        dword [edi+20], ebx
mov        dword [ebp+16], eax
mov        dword [ebp+20], ebx
mov        al, byte [esi+13]
mov        bl, byte [esi+15]
mov        ah, al
mov        bh, bl
bswap    eax
bswap    ebx
mov        al, byte [esi+12]
mov        bl, byte [esi+14]
mov        ah, al
mov        bh, bl
add        esi,16
mov        dword [edi+24], eax
mov        dword [edi+28], ebx
add        edi,32
mov        dword [ebp+24], eax
mov        dword [ebp+28], ebx
add        ebp,32
dec        ecx
jnz        near xloopU
sub        esi,dword [_SCREENWIDTH]
sub        esi,dword [_SCREENWIDTH]
add        edi,dword [_ViewMemPitch]
add        ebp,dword [_ViewMemPitch]
dec        edx
jnz        near yloopU

pop     ebx
pop     edi
pop     esi
pop     ebp
ret
