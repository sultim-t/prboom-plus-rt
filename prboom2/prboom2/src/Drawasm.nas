BITS 32

; Segment/section definition macros. 

%ifdef M_TARGET_WATCOM
  SEGMENT DATA PUBLIC ALIGN=16 CLASS=DATA USE32
  SEGMENT DATA
%else
  SECTION .data
%endif

; External variables:
extern _ds_y
extern _ds_x1
extern _ds_x2
extern _ds_xstep
extern _ds_ystep
extern _ds_xfrac
extern _ds_yfrac
extern _ds_colormap
extern _ds_source
extern _ylookup
extern _topleft
extern _dc_yh
extern _dc_yl
extern _dc_x
extern _dc_texheight
extern _dc_source
extern _dc_iscale
extern _dc_colormap
extern _dc_texturemid
extern _centery
extern _tranmap
extern _SCREENWIDTH

%ifdef M_TARGET_WATCOM
  SEGMENT CODE PUBLIC ALIGN=16 CLASS=CODE USE32
  SEGMENT CODE
%else
  SECTION .text
%endif


 ; //================
 ; //
 ; // R_DrawSpan
 ; //
 ; // Horizontal texture mapping
 ; //
 ; //================
 ; //
 ; // 2/14/98 Lee Killough
 ; //
 ; // Ported from the released linux source
 ; // Converted from Intel to AT&T syntax
 ; // Replaced self-modifying code with code that uses stack
 ; // Removed dependence on 256-byte-aligned colormaps
 ; //
 ; //================

global _R_DrawSpan

global _R_DrawColumn_Normal

global _R_DrawTLColumn_Normal

_R_DrawSpan: ; basic label
push     ebp
push     esi
push     edi
push     ebx

 ; //
 ; // find loop count
 ; //

 ; // count = ds_x2 - ds_x1 + 1; 

mov     ecx, [_ds_x2]
inc     ecx
sub     ecx, [_ds_x1]
push    ecx

 ; // nothing to scale
jle     near hdone

 ; //
 ; // build composite position
 ; //
mov     ebp, [_ds_xfrac]
mov     eax, [_ds_yfrac]
shl     ebp, 10
shr     eax, 6
and     ebp, 00ffff0000h
and     eax, 0ffffh
or      ebp, eax

 ; //
 ; // calculate screen dest
 ; //
 ; //  dest = ds_y*SCREENWIDTH + ds_x1 + topleft;
mov     ebx, [_ds_y]
mov			edi, [_SCREENWIDTH]
imul		edi, ebx
add			edi, [_ds_x1]
add			edi, [_topleft]

 ; //
 ; // build composite step
 ; //
mov     esi, [_ds_xstep]
mov     eax, [_ds_ystep]
shl     esi, 10
shr     eax, 6
and     esi, 0ffff0000h
and     eax, 0ffffh
or      esi, eax

 ; // eax, ebx,
 ; // ecx, edx    scratch
 ; // esi         step
 ; // edi         moving destination pointer
 ; // ebp         position
 ; // [esp]       count

hquadloop:
cmp dword [esp],8
jl near hsingleloopstart

shld    ebx, ebp, 22
shld    ebx, ebp,  6
add     ebp, esi
and     ebx, 4095
shld    ecx, ebp, 22
add     ebx, [_ds_source]
shld    ecx, ebp,  6
add     ebp, esi
and     ecx, 4095
add     ecx, [_ds_source]

xor     edx, edx
mov     dl,  byte [ebx]
xor     ebx, ebx
mov     bl,  byte [ecx]
add     edx, [_ds_colormap]
add     ebx, [_ds_colormap]
mov     ah,  byte [edx]
mov     al,  byte [ebx]
mov     byte [edi], ah
mov     byte [1+edi], al

shld    ebx, ebp, 22
shld    ebx, ebp,  6
add     ebp, esi
and     ebx, 4095
shld    ecx, ebp, 22
add     ebx, [_ds_source]
shld    ecx, ebp,  6
add     ebp, esi
and     ecx, 4095
add     ecx, [_ds_source]

xor     edx, edx
mov     dl,  byte [ebx]
xor     ebx, ebx
mov     bl,  byte [ecx]
add     edx, [_ds_colormap]
add     ebx, [_ds_colormap]
mov     ah,  byte [edx]
mov     al,  byte [ebx]
mov     byte [2+edi], ah
mov     byte [3+edi], al

shld    ebx, ebp, 22
shld    ebx, ebp,  6
add     ebp, esi
and     ebx, 4095
shld    ecx, ebp, 22
add     ebx, [_ds_source]
shld    ecx, ebp,  6
add     ebp, esi
and     ecx, 4095
add     ecx, [_ds_source]

xor     edx, edx
mov     dl,  byte [ebx]
xor     ebx, ebx
mov     bl,  byte [ecx]
add     edx, [_ds_colormap]
add     ebx, [_ds_colormap]
mov     ah,  byte [edx]
mov     al,  byte [ebx]
mov     byte [4+edi], ah
mov     byte [5+edi], al

shld    ebx, ebp, 22
shld    ebx, ebp,  6
add     ebp, esi
and     ebx, 4095
shld    ecx, ebp, 22
add     ebx, [_ds_source]
shld    ecx, ebp,  6
add     ebp, esi
and     ecx, 4095
add     ecx, [_ds_source]

xor     edx, edx
mov     dl,  byte [ebx]
xor     ebx, ebx
mov     bl,  byte [ecx]
add     edx, [_ds_colormap]
add     ebx, [_ds_colormap]
mov     ah,  byte [edx]
mov     al,  byte [ebx]
mov     byte [6+edi], ah
mov     byte [7+edi], al

add     edi,8
sub     dword [esp],8
jmp     hquadloop

hsingleloopstart:
 ; // ecx         count
 ; // edx         virtual source
mov     edx, [_ds_source]
pop     ecx
hsingleloop:
or ecx,ecx
jz near hdone

 ; // begin calculating pixel (y units)
shld    ebx, ebp, 22
 ; // begin calculating pixel (x units)
shld    ebx, ebp,  6
 ; // advance frac pointer
add     ebp, esi
 ; // finish calculation for pixel
and     ebx, 4095

 ; // get pixel
xor     eax, eax
mov     al,  byte [edx+ebx]
 ; // offset pixel into colormap
add     eax, [_ds_colormap]
 ; // color translate third pixel
mov     al,  byte [eax]
 ; // write pixel
mov     byte [edi], al

inc     edi
dec     ecx
jmp     hsingleloop

hdone:
pop     ebx
pop     edi
pop     esi
pop     ebp
ret

 ; //================
 ; //
 ; // R_DrawColumn
 ; //
 ; //================
 ; //
 ; // 2/15/98 Lee Killough
 ; //
 ; // Converted C code with TFE fix to assembly and tuned
 ; //
 ; // 2/21/98 killough: added translucency support
 ; //
 ; //================

_R_DrawColumn_Normal: ; basic label

push     ebp
push     esi
push     edi
push     ebx
mov     esi,    dword [_dc_yh]
inc     esi
mov     edx,    dword [_dc_yl]
mov     ebx,    dword [_SCREENWIDTH]
sub     esi,    edx
imul    ebx, edx
add     ebx, [_dc_x]
add     ebx, [_topleft]
jle     near end
mov     eax,    dword [_dc_texheight]
sub     edx,    dword [_centery]
mov     ebp,    dword [_dc_source]
imul     edx,    dword [_dc_iscale]
lea     ecx,    [-1+eax]
mov     edi,    dword [_dc_colormap]
add     edx,    dword [_dc_texturemid]
test     ecx,    eax
je         near powerof2
sal     eax,    16

red1: ; basic label
sub     edx,    eax
jge     near red1

red2: ; basic label
add     edx,    eax
jl         near red2


nonp2loop: ; basic label
mov     ecx,    edx
sar     ecx,    16
add     edx,    dword [_dc_iscale]
movzx     ecx,    byte [ecx+ebp]
mov     cl,    byte [edi+ecx]
mov     byte [ebx],    cl
add     ebx,    dword [_SCREENWIDTH]
cmp     edx,    eax
jge     near wraparound
dec     esi
jg         near nonp2loop
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


wraparound: ; basic label
sub     edx,    eax
dec     esi
jg         near nonp2loop
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


end: ; basic label
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


p2loop: ; basic label
mov     eax,    edx
add     edx,    dword [_dc_iscale]
sar     eax,    16
and     eax,    ecx
movzx     eax,    byte [eax+ebp]
mov     al,    byte [eax+edi]
mov     byte [ebx],    al
add     ebx,    dword [_SCREENWIDTH]
mov     eax,    edx
add     edx,    dword [_dc_iscale]
sar     eax,    16
and     eax,    ecx
movzx     eax,    byte [eax+ebp]
mov     al,    byte [eax+edi]
mov     byte [ebx],    al
add     ebx,    dword [_SCREENWIDTH]

powerof2: ; basic label
add     esi,    -2
jge     near p2loop
jnp     end
sar     edx,    16
and     edx,    ecx
xor     eax,    eax
mov     al,    byte [edx+ebp]
mov     al,    byte [eax+edi]
mov     byte [ebx],    al
pop     ebx
pop     edi
pop     esi
pop     ebp
ret

 ; //================
 ; //
 ; // R_DrawTLColumn
 ; //
 ; // Translucency support
 ; //
 ; //================


_R_DrawTLColumn_Normal: ; basic label

push     ebp
push     esi
push     edi
push     ebx
mov     esi,    dword [_dc_yh]
inc     esi
mov     edx,    dword [_dc_yl]
mov     ebx,    dword [_SCREENWIDTH]
sub     esi,    edx
imul    ebx, edx
add     ebx, [_dc_x]
add     ebx, [_topleft]
jle     near end_tl
mov     eax,    dword [_dc_texheight]
sub     edx,    dword [_centery]
mov     ebp,    dword [_dc_source]
imul     edx,    dword [_dc_iscale]
lea     ecx,    [-1+eax]
mov     edi,    dword [_dc_colormap]
add     edx,    dword [_dc_texturemid]
test     ecx,    eax
push     ecx
je         near powerof2_tl
sal     eax,    16

red1_tl: ; basic label
sub     edx,    eax
jge     near red1_tl

red2_tl: ; basic label
add     edx,    eax
jl         near red2_tl
push     esi


nonp2loop_tl: ; basic label
xor     ecx,    ecx
mov     esi,    edx
mov     cl,    byte [ebx]
shl     ecx,    8
sar     esi,    16
add     ecx,    [_tranmap]
add     edx,    dword [_dc_iscale]
movzx     esi,    byte [esi+ebp]
movzx     esi,    byte [edi+esi]
mov     cl,    byte [ecx+esi]
mov     byte [ebx],    cl
add     ebx,    dword [_SCREENWIDTH]
cmp     edx,    eax
jge     wraparound_tl
dec     dword [esp]
jg         near nonp2loop_tl
pop     eax
pop     ecx
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


wraparound_tl: ; basic label
sub     edx,    eax
dec     dword [esp]
jg         near nonp2loop_tl
pop     eax
pop     ecx
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


end_tl: ; basic label
pop     ecx
pop     ebx
pop     edi
pop     esi
pop     ebp
ret


p2loop_tl: ; basic label
mov     eax,    edx
xor     ecx,    ecx
add     edx,    dword [_dc_iscale]
mov     cl,    byte [ebx]
sar     eax,    16
shl     ecx,    8
and     eax,    dword [esp]
add     ecx,    dword [_tranmap]
movzx     eax,    byte [eax+ebp]
movzx     eax,    byte [edi+eax]
mov     al,    byte [ecx+eax]
xor     ecx,    ecx
mov     byte [ebx],    al
add     ebx, dword [_SCREENWIDTH]
mov     cl,    byte [ebx]
mov     eax,    edx
add     edx,    dword [_dc_iscale]
sar     eax,    16
shl     ecx,    8
and     eax,    dword [esp]
add     ecx,    dword [_tranmap]
movzx     eax,    byte [eax+ebp]
movzx     eax,    byte [edi+eax]
mov     al,    byte [ecx+eax]
mov     byte [ebx],    al
add     ebx, dword [_SCREENWIDTH]

powerof2_tl: ; basic label
add     esi,    -2
jge     near p2loop_tl
jnp     near end_tl
xor     ecx,    ecx
sar     edx,    16
mov     cl,    byte [ebx]
and     edx,    dword [esp]
shl     ecx,    8
xor     eax,    eax
add     ecx,    dword [_tranmap]
mov     al,    byte [edx+ebp]
movzx     eax,    byte [eax+edi]
mov     al ,    byte [ecx+eax]
mov     byte [ebx],    al
pop     ecx
pop     ebx
pop     edi
pop     esi
pop     ebp
ret
