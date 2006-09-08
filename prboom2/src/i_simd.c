// Copyright (C) 2004 Id Software, Inc.
//

//===============================================================
//
//	3DNow! implementation of idSIMDProcessor
//
//===============================================================

#ifdef _WIN32
#ifdef SIMD_INSTRUCTIONS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

#include "doomtype.h"
#include "m_argv.h"
#include "SDL_cpuinfo.h"
#include "i_simd.h"

memcpy_fast_f memcpy_fast;
memset_fast_f memset_fast;

static void* memcpy_MMX( void *dst, const void *src, size_t count );
static void* memset_MMX( void *dst, int val, size_t count );
static void* memcpy_3DNow( void *dst, const void *src, size_t count );

void I_InitSIMD(void)
{
  memcpy_fast = memcpy;
  memset_fast = memset;

  if (!M_CheckParm("-nosimd"))
  {
    if (SDL_Has3DNow() && !M_CheckParm("-no3dnow"))
    {
      memcpy_fast = memcpy_3DNow;
      fprintf(stdout, "I_Init: using MMX and 3DNow! for SIMD processing\n");
    }
    else
    {
      if (SDL_HasMMX() && !M_CheckParm("-nommx"))
      {
        memcpy_fast = memcpy_MMX;
        memset_fast = memset_MMX;
        fprintf(stdout, "I_Init: using MMX for SIMD processing\n");
      }
    }
  }
}

#define EMMS_INSTRUCTION __asm emms
#if _MSC_VER > 1300
#define PREFETCH(a) prefetchnta a
#define MOVNTQ movntq
#define SFENCE sfence
#else
#define PREFETCH(a)
#define MOVNTQ movq
#define SFENCE
#endif

static void* memcpy_MMX( void *dst, const void *src, size_t count );
static void* memset_MMX( void *dst, int val, size_t count );
static void* memcpy_3DNow( void *dst, const void *src, size_t count );

// Very optimized memcpy() routine for all AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetchnta instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#define TINY_BLOCK_COPY 64       // upper limit for movsd type copy
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 64 * 1024  // upper limit for movq/movq copy w/SW prefetch
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 197 * 1024 // upper limit for movq/movntq w/SW prefetch
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE"

#define BLOCK_PREFETCH_COPY  infinity // no limit for movq/movntq w/block prefetch 
#define CACHEBLOCK 80h // number of 64-byte blocks (cache lines) for block prefetch
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

/*
================
idSIMD_3DNow::Memcpy

  optimized memory copy routine that handles all alignment cases and block sizes efficiently
================
*/
static void* memcpy_3DNow( void *dest, const void *src, size_t n ) {
  __asm {

	mov		ecx, [n]					// number of bytes to copy
	mov		edi, [dest]					// destination
	mov		esi, [src]					// source
	mov		ebx, ecx					// keep a copy of count

	cld
	cmp		ecx, TINY_BLOCK_COPY
	jb		$memcpy_ic_3				// tiny? skip mmx copy

	cmp		ecx, 32*1024				// don't align between 32k-64k because
	jbe		$memcpy_do_align			//  it appears to be slower
	cmp		ecx, 64*1024
	jbe		$memcpy_align_done
$memcpy_do_align:
	mov		ecx, 8						// a trick that's faster than rep movsb...
	sub		ecx, edi					// align destination to qword
	and		ecx, 111b					// get the low bits
	sub		ebx, ecx					// update copy count
	neg		ecx							// set up to jump into the array
	add		ecx, offset $memcpy_align_done
	jmp		ecx							// jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_align_done:						// destination is dword aligned
	mov		ecx, ebx					// number of bytes left to copy
	shr		ecx, 6						// get 64-byte block count
	jz		$memcpy_ic_2				// finish the last few bytes

	cmp		ecx, IN_CACHE_COPY/64		// too big 4 cache? use uncached copy
	jae		$memcpy_uc_test

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_ic_1:							// 64-byte block copies, in-cache copy

	PREFETCH([esi + (200*64/34+192)])	// start reading ahead

	movq	mm0, [esi+0]				// read 64 bits
	movq	mm1, [esi+8]
	movq	[edi+0], mm0				// write 64 bits
	movq	[edi+8], mm1				//    note:  the normal movq writes the
	movq	mm2, [esi+16]				//    data to cache; a cache line will be
	movq	mm3, [esi+24]				//    allocated as needed, to store the data
	movq	[edi+16], mm2
	movq	[edi+24], mm3
	movq	mm0, [esi+32]
	movq	mm1, [esi+40]
	movq	[edi+32], mm0
	movq	[edi+40], mm1
	movq	mm2, [esi+48]
	movq	mm3, [esi+56]
	movq	[edi+48], mm2
	movq	[edi+56], mm3

	add		esi, 64						// update source pointer
	add		edi, 64						// update destination pointer
	dec		ecx							// count down
	jnz		$memcpy_ic_1				// last 64-byte block?

$memcpy_ic_2:
	mov		ecx, ebx					// has valid low 6 bits of the byte count
$memcpy_ic_3:
	shr		ecx, 2						// dword count
	and		ecx, 1111b					// only look at the "remainder" bits
	neg		ecx							// set up to jump into the array
	add		ecx, offset $memcpy_last_few
	jmp		ecx							// jump to array of movsd's

$memcpy_uc_test:
	cmp		ecx, UNCACHED_COPY/64		// big enough? use block prefetch copy
	jae		$memcpy_bp_1

$memcpy_64_test:
	or		ecx, ecx					// tail end of block prefetch will jump here
	jz		$memcpy_ic_2				// no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
align 16
$memcpy_uc_1:							// 64-byte blocks, uncached copy

	PREFETCH ([esi + (200*64/34+192)])	// start reading ahead

	movq	mm0,[esi+0]					// read 64 bits
	add		edi,64						// update destination pointer
	movq	mm1,[esi+8]
	add		esi,64						// update source pointer
	movq	mm2,[esi-48]
	MOVNTQ	[edi-64], mm0				// write 64 bits, bypassing the cache
	movq	mm0,[esi-40]				//    note: movntq also prevents the CPU
	MOVNTQ	[edi-56], mm1				//    from READING the destination address
	movq	mm1,[esi-32]				//    into the cache, only to be over-written
	MOVNTQ	[edi-48], mm2				//    so that also helps performance
	movq	mm2,[esi-24]
	MOVNTQ	[edi-40], mm0
	movq	mm0,[esi-16]
	MOVNTQ	[edi-32], mm1
	movq	mm1,[esi-8]
	MOVNTQ	[edi-24], mm2
	MOVNTQ	[edi-16], mm0
	dec		ecx
	MOVNTQ	[edi-8], mm1
	jnz		$memcpy_uc_1				// last 64-byte block?

	jmp		$memcpy_ic_2				// almost done

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
$memcpy_bp_1:							// large blocks, block prefetch copy

	cmp		ecx, CACHEBLOCK				// big enough to run another prefetch loop?
	jl		$memcpy_64_test				// no, back to regular uncached copy

	mov		eax, CACHEBLOCK / 2			// block prefetch loop, unrolled 2X
	add		esi, CACHEBLOCK * 64		// move to the top of the block
align 16
$memcpy_bp_2:
	mov		edx, [esi-64]				// grab one address per cache line
	mov		edx, [esi-128]				// grab one address per cache line
	sub		esi, 128					// go reverse order
	dec		eax							// count down the cache lines
	jnz		$memcpy_bp_2				// keep grabbing more lines into cache

	mov		eax, CACHEBLOCK				// now that it's in cache, do the copy
align 16
$memcpy_bp_3:
	movq	mm0, [esi   ]				// read 64 bits
	movq	mm1, [esi+ 8]
	movq	mm2, [esi+16]
	movq	mm3, [esi+24]
	movq	mm4, [esi+32]
	movq	mm5, [esi+40]
	movq	mm6, [esi+48]
	movq	mm7, [esi+56]
	add		esi, 64						// update source pointer
	MOVNTQ	[edi   ], mm0				// write 64 bits, bypassing cache
	MOVNTQ	[edi+ 8], mm1				//    note: movntq also prevents the CPU
	MOVNTQ	[edi+16], mm2				//    from READING the destination address 
	MOVNTQ	[edi+24], mm3				//    into the cache, only to be over-written,
	MOVNTQ	[edi+32], mm4				//    so that also helps performance
	MOVNTQ	[edi+40], mm5
	MOVNTQ	[edi+48], mm6
	MOVNTQ	[edi+56], mm7
	add		edi, 64						// update dest pointer

	dec		eax							// count down

	jnz		$memcpy_bp_3				// keep copying
	sub		ecx, CACHEBLOCK				// update the 64-byte block count
	jmp		$memcpy_bp_1				// keep processing chunks

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
align 4
	movsd
	movsd								// perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd								// perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:						// dword aligned from before movsd's
	mov		ecx, ebx					// has valid low 2 bits of the byte count
	and		ecx, 11b					// the last few cows must come home
	jz		$memcpy_final				// no more, let's leave
	rep		movsb						// the last 1, 2, or 3 bytes

$memcpy_final: 
	emms								// clean up the MMX state
	SFENCE								// flush the write buffer
	mov		eax, [dest]					// ret value = destination pointer

    }
	return dest;
}

/*
================
MMX_Memcpy8B
================
*/
static void MMX_Memcpy8B( void *dest, const void *src, const int count ) {
	_asm { 
        mov		esi, src 
        mov		edi, dest 
        mov		ecx, count 
        shr		ecx, 3			// 8 bytes per iteration 

loop1: 
        movq	mm1,  0[ESI]	// Read in source data 
        MOVNTQ	0[EDI], mm1		// Non-temporal stores 

        add		esi, 8
        add		edi, 8
        dec		ecx 
        jnz		loop1 

	} 
	EMMS_INSTRUCTION
}

/*
================
MMX_Memcpy64B

  165MB/sec
================
*/
static void MMX_Memcpy64B( void *dest, const void *src, const int count ) {
	_asm { 
        mov		esi, src 
        mov		edi, dest 
        mov		ecx, count 
        shr		ecx, 6		// 64 bytes per iteration

loop1: 
        PREFETCH (64[ESI])	// Prefetch next loop, non-temporal 
        PREFETCH (96[ESI]) 

        movq mm1,  0[ESI]	// Read in source data 
        movq mm2,  8[ESI] 
        movq mm3, 16[ESI] 
        movq mm4, 24[ESI] 
        movq mm5, 32[ESI] 
        movq mm6, 40[ESI] 
        movq mm7, 48[ESI] 
        movq mm0, 56[ESI] 

        MOVNTQ  0[EDI], mm1	// Non-temporal stores 
        MOVNTQ  8[EDI], mm2 
        MOVNTQ 16[EDI], mm3 
        MOVNTQ 24[EDI], mm4 
        MOVNTQ 32[EDI], mm5 
        MOVNTQ 40[EDI], mm6 
        MOVNTQ 48[EDI], mm7 
        MOVNTQ 56[EDI], mm0 

        add		esi, 64 
        add		edi, 64 
        dec		ecx 
        jnz		loop1 
	} 
	EMMS_INSTRUCTION
}

/*
================
MMX_Memcpy2kB

  240MB/sec
================
*/
#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

static void MMX_Memcpy2kB( void *dest, const void *src, const int count ) {
	byte *tbuf = NULL;//(byte *)_alloca16(2048);
	__asm { 
		push	ebx
        mov		esi, src
        mov		ebx, count
        shr		ebx, 11		// 2048 bytes at a time 
        mov		edi, dest

loop2k:
        push	edi			// copy 2k into temporary buffer
        mov		edi, tbuf
        mov		ecx, 32

loopMemToL1: 
        PREFETCH (64[ESI]) // Prefetch next loop, non-temporal
        PREFETCH (96[ESI])

        movq mm1,  0[ESI]	// Read in source data
        movq mm2,  8[ESI]
        movq mm3, 16[ESI]
        movq mm4, 24[ESI]
        movq mm5, 32[ESI]
        movq mm6, 40[ESI]
        movq mm7, 48[ESI]
        movq mm0, 56[ESI]

        movq  0[EDI], mm1	// Store into L1
        movq  8[EDI], mm2
        movq 16[EDI], mm3
        movq 24[EDI], mm4
        movq 32[EDI], mm5
        movq 40[EDI], mm6
        movq 48[EDI], mm7
        movq 56[EDI], mm0
        add		esi, 64
        add		edi, 64
        dec		ecx
        jnz		loopMemToL1

        pop		edi			// Now copy from L1 to system memory
        push	esi
        mov		esi, tbuf
        mov		ecx, 32

loopL1ToMem:
        movq mm1, 0[ESI]	// Read in source data from L1
        movq mm2, 8[ESI]
        movq mm3, 16[ESI]
        movq mm4, 24[ESI]
        movq mm5, 32[ESI]
        movq mm6, 40[ESI]
        movq mm7, 48[ESI]
        movq mm0, 56[ESI]

        MOVNTQ 0[EDI], mm1	// Non-temporal stores
        MOVNTQ 8[EDI], mm2
        MOVNTQ 16[EDI], mm3
        MOVNTQ 24[EDI], mm4
        MOVNTQ 32[EDI], mm5
        MOVNTQ 40[EDI], mm6
        MOVNTQ 48[EDI], mm7
        MOVNTQ 56[EDI], mm0

        add		esi, 64
        add		edi, 64
        dec		ecx
        jnz		loopL1ToMem

        pop		esi			// Do next 2k block
        dec		ebx
        jnz		loop2k
		pop		ebx
	}
	EMMS_INSTRUCTION
}


/*
================
idSIMD_MMX::Memcpy

  optimized memory copy routine that handles all alignment cases and block sizes efficiently
================
*/
static void* memcpy_MMX( void *dest0, const void *src0, size_t count0 ) {
	// if copying more than 16 bytes and we can copy 8 byte aligned
	if ( count0 > 16 && !( ( (int)dest0 ^ (int)src0 ) & 7 ) ) {
		byte *dest = (byte *)dest0;
		byte *src = (byte *)src0;

		// copy up to the first 8 byte aligned boundary
		int count = ((int)dest) & 7;
		memcpy( dest, src, count );
		dest += count;
		src += count;
		count = count0 - count;

		// if there are multiple blocks of 2kB
		if ( count & ~4095 ) {
			MMX_Memcpy2kB( dest, src, count );
			src += (count & ~2047);
			dest += (count & ~2047);
			count &= 2047;
		}

		// if there are blocks of 64 bytes
		if ( count & ~63 ) {
			MMX_Memcpy64B( dest, src, count );
			src += (count & ~63);
			dest += (count & ~63);
			count &= 63;
		}

		// if there are blocks of 8 bytes
		if ( count & ~7 ) {
			MMX_Memcpy8B( dest, src, count );
			src += (count & ~7);
			dest += (count & ~7);
			count &= 7;
		}

		// copy any remaining bytes
		memcpy( dest, src, count );
	} else {
		// use the regular one if we cannot copy 8 byte aligned
		memcpy( dest0, src0, count0 );
	}
	return dest0;
}

/*
================
idSIMD_MMX::Memset
================
*/
static void* memset_MMX( void* dest0, int val, size_t count0 ) {
	union {
		byte	bytes[8];
		unsigned short	words[4];
		unsigned int	dwords[2];
	} dat;

	byte *dest = (byte *)dest0;
	int count = count0;

	while( count > 0 && (((int)dest) & 7) ) {
		*dest = val;
		dest++;
		count--;
	}
	if ( !count ) {
  	return dest0;
	}

	dat.bytes[0] = val;
	dat.bytes[1] = val;
	dat.words[1] = dat.words[0];
	dat.dwords[1] = dat.dwords[0];

	if ( count >= 64 ) {
		__asm {
			mov edi, dest 
			mov ecx, count 
			shr ecx, 6				// 64 bytes per iteration 
			movq mm1, dat			// Read in source data 
			movq mm2, mm1
			movq mm3, mm1
			movq mm4, mm1
			movq mm5, mm1
			movq mm6, mm1
			movq mm7, mm1
			movq mm0, mm1
loop1: 
			MOVNTQ  0[EDI], mm1		// Non-temporal stores 
			MOVNTQ  8[EDI], mm2 
			MOVNTQ 16[EDI], mm3 
			MOVNTQ 24[EDI], mm4 
			MOVNTQ 32[EDI], mm5 
			MOVNTQ 40[EDI], mm6 
			MOVNTQ 48[EDI], mm7 
			MOVNTQ 56[EDI], mm0 

			add edi, 64 
			dec ecx 
			jnz loop1 
		}
		dest += ( count & ~63 );
		count &= 63;
	}

	if ( count >= 8 ) {
		__asm {
			mov edi, dest 
			mov ecx, count 
			shr ecx, 3				// 8 bytes per iteration 
			movq mm1, dat			// Read in source data 
loop2: 
			MOVNTQ  0[EDI], mm1		// Non-temporal stores 

			add edi, 8
			dec ecx 
			jnz loop2
		}
		dest += (count & ~7);
		count &= 7;
	}

	while( count > 0 ) {
		*dest = val;
		dest++;
		count--;
	}

	EMMS_INSTRUCTION 

	return dest0;
}
#endif // SIMD_INSTRUCTIONS
#endif // _WIN32
